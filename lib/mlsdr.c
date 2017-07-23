

#define _GNU_SOURCE
#include <errno.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <stdbool.h>
#include <libusb-1.0/libusb.h>
#include <ftdi.h>
#include <bsd/string.h>

#include "regmap.h"
#include "utils.h"
#include "r820t.h"
#include "iqconv.h"

#include "mlsdr.h"

enum parser_state {
	PARSER_IDLE,
	PARSER_TYPE,
	PARSER_SAMPLES,
	PARSER_ESCAPE,
	PARSER_UNKNOWN,
};

#define PACKET_MAGIC		0xfe
#define PACKET_SAMPLE		0x01
#define PACKET_PPS			0x02
#define PACKET_RESP			0x03
#define PACKET_END			0x88

#define CMD_WRITE			0xaa
#define CMD_READ			0x55

#define SAMPLE_BUFFER_LEN		(32768 * 16)

#define PACKETS_PER_TRANSFER	16
#define TRANSFER_CNT			512

#define REG_CNT					256

#define I2C_REG_LEN				1024

const uint8_t mlsdr_reg_cache_masks[REG_CNT] = {
	[MLSDR_REG_ADCTL] = MLSDR_ADCTL_ENABLE | MLSDR_ADCTL_MODE_MASK,
};

struct mlsdr_priv {
	struct regmap *regmap;
	struct {
		int type;
		bool escaped;
		enum parser_state state;

		int unpackcnt;
		uint_fast32_t unpack;
		size_t samplecnt;
		int16_t samples[SAMPLE_BUFFER_LEN];

		struct mlsdr_pps_event ppsev;

		unsigned int bytecnt;
	} p;
	struct mlsdr_ring_uint8_t *ringresp;
	uint8_t regcache[REG_CNT];
	unsigned int i2ccnt;
	bool own_logctx;
	atomic_bool adc_enabled;
	atomic_bool should_run;

	sem_t recvthread_sem;

	int16_t *iqtmp;
	size_t iqtmp_len;
	struct mlsdr_iqconv *iqconv;
};

#define log_debug(ml, format, ...) mlsdr_log_debug((ml)->logctx, format, ##__VA_ARGS__)
#define log_info(ml, format, ...) mlsdr_log_info((ml)->logctx, format, ##__VA_ARGS__)
#define log_warn(ml, format, ...) mlsdr_log_warn((ml)->logctx, format, ##__VA_ARGS__)
#define log_error(ml, format, ...) mlsdr_log_error((ml)->logctx, format, ##__VA_ARGS__)
#define log_error_errno(ml, format, ...) mlsdr_log_error_errno((ml)->logctx, format, ##__VA_ARGS__)

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
void mlsdr_default_log(struct mlsdr_logctx *ctx, enum mlsdr_loglevel level,
					   const char *func, unsigned int lineno, const char *format, ...)
{
#define MAKE_PREFIX(color, level) color "[" level "]:%s():%d:"
	const char *level_to_prefix[] = {
		[MLSDR_LOG_DEBUG]	= MAKE_PREFIX("\x1b[0;37m", "DEBUG"),
		[MLSDR_LOG_INFO]	= MAKE_PREFIX("\x1b[1;34m", "INFO "),
		[MLSDR_LOG_WARN]	= MAKE_PREFIX("\x1b[1;33m", "WARN "),
		[MLSDR_LOG_ERROR]	= MAKE_PREFIX("\x1b[1;31m", "ERROR"),
	};
#undef MAKE_PREFIX

	if (level < ctx->level) {
		return;
	}

	char *prefix;
	int ret = asprintf(&prefix, level_to_prefix[level], func, lineno);
	if (ret < 0) {
		return;
	}
	char *content;
	va_list ap;
	va_start(ap, format);
	ret = vasprintf(&content, format, ap);
	if (ret < 0) {
		free(prefix);
		return;
	}
	va_end(ap);
	size_t buflen = strlen(prefix) + strlen(content) + 2;
	char *str = malloc(buflen);
	bzero(str, buflen);
	strlcat(str, prefix, buflen);
	strlcat(str, content, buflen);
	strlcat(str, "\n", buflen);

	// We want to fprintf this in a single call - otherwise lines could interleave
	fwrite(str, 1, buflen - 1, stderr);

	free(prefix);
	free(content);
	free(str);
}
#pragma GCC diagnostic pop

static inline int16_t to_signed(int16_t a)
{
	if (a >= 2048)
		return a - 4096;
	return a;
}

static inline void mlsdr_push_unpack(struct mlsdr_priv *priv, uint8_t c)
{
	priv->p.unpackcnt++;
	priv->p.unpack = (priv->p.unpack << 8) | c;

	// We should have at least two empty spaces here under all circumstances
	if (priv->p.unpackcnt == 3) {
		uint32_t v = priv->p.unpack;

		priv->p.samples[priv->p.samplecnt++] =
			to_signed(BYTE(v, 2) | ((BYTE(v, 1) & 0x0f) << 8));
		priv->p.samples[priv->p.samplecnt++] =
			to_signed((BYTE(v, 0) << 4) | ((BYTE(v, 1) & 0xf0) >> 4));

		priv->p.unpack = 0;
		priv->p.unpackcnt = 0;
	}
}

static void mlsdr_data_callback(struct mlsdr *mlsdr, uint8_t *buffer, int length)
{
	struct mlsdr_priv *priv = mlsdr->priv;

	for (int i = 0; i < length; i++) {
		uint8_t c = buffer[i];
		//uint8_t d = c - priv->p.type;
		//if (d != 255 && d != 1) {
		//	fprintf(stderr, "\n");
		//	for (int k = 0; k < length; k++) {
		//		if (k == i) {
		//			fprintf(stderr, "__");
		//		}
		//		//if (buffer[k] == 0x55) {
		//		//	fprintf(stderr, "?");
		//		//}
		//		fprintf(stderr, "%02x ", buffer[k]);
		//	}
		//}
		//priv->p.type = c;
		//continue;
		//printf("%02x \n", c);
		if (priv->p.escaped) {
			priv->p.escaped = false;
			if (c == PACKET_SAMPLE) {
				priv->p.type = PACKET_SAMPLE;
				// We are guaranteed to have packet starting (and ending) on
				// a pack boundary.
				// unpackcnt should be 0 here anyway, but be robust and unfuck it
				// automatically.
				if (priv->p.unpackcnt != 0) {
					log_warn(mlsdr, "A sample packet with invalid length, probably some bytes got dropped!");
					priv->p.unpackcnt = 0;
				}
			} else if (c == PACKET_RESP) {
				priv->p.type = PACKET_RESP;
			} else if (c == PACKET_PPS) {
				priv->p.type = PACKET_PPS;
				priv->p.ppsev.timestamp = 0;
				priv->p.bytecnt = 0;
			} else if (c != PACKET_MAGIC){
				// This happens when the comment above applies
				log_warn(mlsdr, "Unknown escape code %02x", c);
			}
			if (c != PACKET_MAGIC) {
				continue;
			}
		} else if (c == PACKET_MAGIC) {
			priv->p.escaped = true;
			continue;
		}

		switch (priv->p.type) {
		case PACKET_SAMPLE:
			mlsdr_push_unpack(priv, c);
			break;
		case PACKET_RESP:
			// There should never be more than 1 element in the buffer
			mlsdr_ring_uint8_t_push(priv->ringresp, &c, 1);
			break;
		case PACKET_PPS:
			priv->p.ppsev.timestamp = (priv->p.ppsev.timestamp << 8) | c;
			priv->p.bytecnt++;
			if (priv->p.bytecnt == 4) {
				mlsdr_ring_mlsdr_pps_event_push(mlsdr->ringpps, &priv->p.ppsev, 1);
			} else if (priv->p.bytecnt == 5) {
				log_error(mlsdr, "PPS packet too long");
			}
			break;
		default:
			// Trying to latch onto a packet start, ignore
			break;
		}

		if (priv->p.samplecnt >= SAMPLE_BUFFER_LEN - 2) {
			// We drop data if the adc has been disabled
			if (atomic_load(&mlsdr->priv->adc_enabled)) {
				mlsdr_ring_int16_t_push(mlsdr->ringsamples, priv->p.samples, priv->p.samplecnt);
			}
			priv->p.samplecnt = 0;
		}
	}
}

struct mlsdr_loop_status {
	struct mlsdr *mlsdr;
	int in_flight;
	unsigned long total_bytes;
};

static void LIBUSB_CALL mlsdr_libusb_cb(struct libusb_transfer *transfer)
{
	struct mlsdr_loop_status *status = transfer->user_data;
	struct mlsdr *mlsdr = status->mlsdr;
	int packet_size = status->mlsdr->ftdi->max_packet_size;

	bool should_cancel = false;
	if (transfer->status == LIBUSB_TRANSFER_COMPLETED) {
		uint8_t *ptr = transfer->buffer;
		int length = transfer->actual_length;
		int numpack = (length + packet_size - 1) / packet_size;

		for (int i = 0; i < numpack; i++) {
			int packet_len = length;
			if (packet_len > packet_size) {
				packet_len = packet_size;
			}

			int payload_len = packet_len - 2;
			status->total_bytes += payload_len;

			mlsdr_data_callback(mlsdr, ptr + 2, payload_len);
			ptr += packet_len;
			length -= packet_len;
		}
		if (atomic_load(&mlsdr->priv->should_run)) {
			// Resubmit
			transfer->status = -1;
			int ret = libusb_submit_transfer(transfer);
			if (ret < 0) {
				atomic_store(&mlsdr->priv->should_run, false);
				should_cancel = true;
			}
		} else {
			should_cancel = true;
		}
	} else {
		log_error(mlsdr, "Unknown transfer status %d", transfer->status);
		should_cancel = true;
	}
	if (should_cancel) {
		free(transfer->buffer);
		libusb_free_transfer(transfer);
		status->in_flight--;
	}

}

static void mlsdr_loop(struct mlsdr *mlsdr)
{
	// Note that mlsdr->priv->should_run needs to be set manually before calling this directly,
	// as otherwise we could run into a race condition where mlsdr_stop() is called before
	// we get to setting it to true here.

	mlsdr->priv->p.state = PARSER_IDLE;
	mlsdr->priv->p.type = 0;
	mlsdr->priv->p.samplecnt = 0;
	mlsdr->priv->p.unpackcnt = 0;
	mlsdr->priv->p.escaped = false;
	ftdi_usb_purge_buffers(mlsdr->ftdi);
	log_info(mlsdr, "Entering read loop");
	// Okay, the following is essentially copied from ftdi_stream.c with one
	// essential (and some minor) difference(s): we allow sending other USB request while
	// this function is running.

	struct ftdi_context *ftdi = mlsdr->ftdi;

	int ret = ftdi_set_bitmode(ftdi, 0xff, BITMODE_RESET);
	if (ret < 0) {
		log_error(mlsdr, "Failed to reset!");
		return;
	}

	struct mlsdr_loop_status status = {
		.mlsdr = mlsdr,
		.in_flight = 0,
		.total_bytes = 0,
	};

	int buffer_size = PACKETS_PER_TRANSFER * TRANSFER_CNT;
	struct libusb_transfer **transfers = calloc(TRANSFER_CNT, sizeof(transfers[0]));
	size_t i;
	for (i = 0; i < TRANSFER_CNT; i++) {
		struct libusb_transfer *transfer = libusb_alloc_transfer(0);

		if (transfer == NULL) {
			log_error(mlsdr, "Failed to allocate a transfer");
			break;
		}

		libusb_fill_bulk_transfer(
				transfer, ftdi->usb_dev, ftdi->out_ep, malloc(buffer_size),
				buffer_size, mlsdr_libusb_cb, &status, 0);

		if (transfer->buffer == NULL) {
			log_error(mlsdr, "Failed to allocate a transfer buffer");
			libusb_free_transfer(transfer);
			break;
		}

		transfer->status = -1;
		int err = libusb_submit_transfer(transfer);
		if (err != 0) {
			log_error_errno(mlsdr, "Failed to submit a transfer");
			libusb_free_transfer(transfer);
			break;
		}

		transfers[i] = transfer;
		status.in_flight++;
	}

	if (i != TRANSFER_CNT) {
		atomic_store(&mlsdr->priv->should_run, false);
	}

	ret = ftdi_set_bitmode(ftdi, 0xff, BITMODE_SYNCFF);
	if (ret < 0) {
		log_error(mlsdr, "Failed to enable SYNCFF mode");
		atomic_store(&mlsdr->priv->should_run, false);
	}

	sem_post(&mlsdr->priv->recvthread_sem);
	// Even if we failed to start properly, we need to get rid of the transfers
	ret = 0;
	while (ret == 0 && status.in_flight > 0) {
		ret = libusb_handle_events(ftdi->usb_ctx);
		if (ret == LIBUSB_ERROR_INTERRUPTED) {
			ret = 0;
		}
	}

	free(transfers);

	log_info(mlsdr, "Exiting read loop (%d)", ret);
}

static void *mlsdr_loop_thread(void *arg)
{
	struct mlsdr *mlsdr = arg;

	mlsdr_loop(mlsdr);

	return NULL;
}

static void mlsdr_run(struct mlsdr *mlsdr)
{
	atomic_store(&mlsdr->priv->should_run, true);
	pthread_create(&mlsdr->thread, NULL, mlsdr_loop_thread, mlsdr);
}

static void mlsdr_stop(struct mlsdr *mlsdr)
{
	atomic_store(&mlsdr->priv->should_run, false);
	pthread_join(mlsdr->thread, NULL);
}

ssize_t mlsdr_read(struct mlsdr *mlsdr, int16_t *samples, size_t len, int timeout)
{
	// TODO: Handle mlsdr being destroyed when here
	return mlsdr_ring_int16_t_pop(mlsdr->ringsamples, samples, len, timeout);
}

ssize_t mlsdr_read_iq(struct mlsdr *mlsdr, complex float *samples, size_t len,
					  int timeout)
{
	size_t ilen = len * 2; // We need two real samples per one complex
	if (ilen > mlsdr->priv->iqtmp_len) {
		mlsdr->priv->iqtmp_len = ilen;
		mlsdr->priv->iqtmp = realloc(mlsdr->priv->iqtmp,
									 mlsdr->priv->iqtmp_len *
										sizeof(mlsdr->priv->iqtmp[0]));
	}

	ssize_t ret = mlsdr_read(mlsdr, mlsdr->priv->iqtmp, ilen, timeout);
	if (ret <= 0) {
		return ret;
	}

	// TODO: If we get an odd short read, then we drop samples, this probably does
	// not really matter much but would be nice to solve.
	mlsdr_iqconv_push(mlsdr->priv->iqconv, mlsdr->priv->iqtmp, samples, ret);

	return ret / 2;
}

int mlsdr_read_register(struct mlsdr *mlsdr, uint8_t address, uint8_t *value)
{
	int ret = regmap_read(mlsdr->priv->regmap, address, value);
	log_debug(mlsdr, "Register[%02x] = %02x", address, *value);
	return ret;
}

int mlsdr_write_register(struct mlsdr *mlsdr, uint8_t address, uint8_t value)
{
	return regmap_write(mlsdr->priv->regmap, address, value);
}

int mlsdr_write_register_mask(struct mlsdr *mlsdr, uint8_t addr, uint8_t mask,
							  uint8_t val)
{
	return regmap_write_mask(mlsdr->priv->regmap, addr, mask, val);
}

static inline int mlsdr_wait_for_i2c_complete(struct mlsdr *mlsdr)
{
	struct mlsdr_priv *priv = mlsdr->priv;
	uint8_t stat;
	int ret;
	do {
		ret = mlsdr_read_register(mlsdr, MLSDR_REG_I2CSTAT, &stat);
		usleep(1000);
	} while ((stat & MLSDR_I2CSTAT_BUSY) && ret == 0);
	if (!(stat & MLSDR_I2CSTAT_BUSY)) {
		priv->i2ccnt = 0;
	}
	return ret;
}

int mlsdr_i2c_dev_read(struct mlsdr *mlsdr, uint8_t dev, uint8_t *val, uint8_t len)
{
	struct mlsdr_priv *priv = mlsdr->priv;

	if (len > 127) {
		log_error(mlsdr, "Trying to read %d from I2C, maximum is 127", len);
		return -1;
	}

	// Wait for the command queue to be empty
	int ret = mlsdr_wait_for_i2c_complete(mlsdr);
	if (ret != 0) {
		return ret;
	}

	// Now write the read command
	uint8_t data[2] = {
		len,
		(dev << 1) | 1
	};

	for (size_t i = 0; i < ARRAY_SIZE(data); i++) {
		ret = mlsdr_write_register(mlsdr, MLSDR_REG_I2CCMD, data[i]);
		priv->i2ccnt++;
		if (ret != 0) {
			return ret;
		}
	}

	ret = mlsdr_wait_for_i2c_complete(mlsdr);
	if (ret != 0) {
		return ret;
	}

	for (int i = 0; i < len; i++) {
		ret = mlsdr_read_register(mlsdr, MLSDR_REG_I2CREAD, &val[i]);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

int mlsdr_i2c_dev_write(struct mlsdr *mlsdr, uint8_t dev, uint8_t addr, uint8_t val)
{
	struct mlsdr_priv *priv = mlsdr->priv;

	if (priv->i2ccnt + 4 >= I2C_REG_LEN) {
		int ret = mlsdr_wait_for_i2c_complete(mlsdr);
		if (ret != 0) {
			return ret;
		}
	}

	uint8_t data[4] = {
		0x80 | 2, // Write 2 bytes (+ address)
		dev << 1,
		addr,
		val,
	};

	for (size_t i = 0; i < ARRAY_SIZE(data); i++) {
		int ret = mlsdr_write_register(mlsdr, MLSDR_REG_I2CCMD, data[i]);
		priv->i2ccnt++;
		if (ret != 0) {
			// Well, if we fail in the middle of this, the I2C state machine
			// is irreversibly fucked. TODO: Possibly add reset bit or something?
			return ret;
		}
	}

	return 0;
}

int mlsdr_adc_enable(struct mlsdr *mlsdr)
{
	atomic_store(&mlsdr->priv->adc_enabled, true);
	return mlsdr_write_register_mask(mlsdr, MLSDR_REG_ADCTL,
									 MLSDR_ADCTL_ENABLE, MLSDR_ADCTL_ENABLE);
}

int mlsdr_adc_disable(struct mlsdr *mlsdr)
{
	atomic_store(&mlsdr->priv->adc_enabled, false);
	mlsdr_ring_int16_t_flush(mlsdr->ringsamples);
	return mlsdr_write_register_mask(mlsdr, MLSDR_REG_ADCTL,
									 MLSDR_ADCTL_ENABLE, 0x00);
}

int mlsdr_adc_set_mode(struct mlsdr *mlsdr, uint8_t mode)
{
	return mlsdr_write_register_mask(mlsdr, MLSDR_REG_ADCTL,
									 MLSDR_ADCTL_MODE_MASK, mode);
}

unsigned long mlsdr_adc_get_sample_rate(struct mlsdr *mlsdr)
{
	UNUSED(mlsdr);
	// This might expand in the future
	return 15000000;
}

static int mlsdr_regmap_read(struct regmap *regmap, uint8_t address, uint8_t *value)
{
	struct mlsdr *mlsdr = regmap->userdata;
	uint8_t buf[] = { CMD_READ, address };

	// TODO: We probably want a timeout here, if the reponse gets lost,
	// this could break the entire application
	struct ftdi_transfer_control *ctl =
		ftdi_write_data_submit(mlsdr->ftdi, buf, sizeof(buf));
	if (ctl == NULL) {
		return -1;
	}
	// TODO: We probably want to timeout this, however just adding a timeout here
	// does not account for proper transaction handling
	ssize_t ret = mlsdr_ring_uint8_t_pop(mlsdr->priv->ringresp, value, 1, -1);
	if (ret != 1) {
		log_error(mlsdr, "Failed to read a register value from the device");
	}

	libusb_free_transfer(ctl->transfer);
	free(ctl);

	return 0; // TODO: Handle failures
}

static void LIBUSB_CALL mlsdr_write_libusb_cb(struct libusb_transfer *transfer)
{
	struct mlsdr *mlsdr = transfer->user_data;

	if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
		log_error(mlsdr, "libusb write failed!");
	}

	free(transfer->buffer);
	libusb_free_transfer(transfer);
}

static int mlsdr_regmap_write(struct regmap *regmap, uint8_t address, uint8_t value)
{
	// Okay, so we are basically copying ftdi_write_data_submit except that
	// the transaction frees its associated buffer in the callback which
	// libftdi does not do.
	// Note that we don't need it for the read case as we are waiting for the
	// return data anyway.
	struct mlsdr *mlsdr = regmap->userdata;

	size_t len = 3;
	uint8_t *buf = malloc(len);
	if (buf == NULL) {
		return -1;
	}
	buf[0] = CMD_WRITE;
	buf[1] = address;
	buf[2] = value;

	struct libusb_transfer *transfer = libusb_alloc_transfer(0);

	if (transfer == NULL) {
		log_error(mlsdr, "Failed to allocate libusb transfer");
		free(buf);
		return -1;
	}

	libusb_fill_bulk_transfer(transfer, mlsdr->ftdi->usb_dev, mlsdr->ftdi->in_ep,
							  buf, len, mlsdr_write_libusb_cb, mlsdr,
							  1000);

	int ret = libusb_submit_transfer(transfer);
	if (ret != 0) {
		log_error_errno(mlsdr, "Failed to submit libusb transfer");
		free(buf);
		libusb_free_transfer(transfer);
	}

	log_debug(mlsdr, "Wrote %02x to %02x", value, address);

	return 0;
}

static struct mlsdr_logctx mlsdr_default_logctx = {
	.level = MLSDR_LOG_INFO,
	.log_cb = mlsdr_default_log,
};

static int mlsdr_fpga_reset(struct mlsdr *mlsdr)
{
	// Reset the FPGA
	const uint8_t bitseq[][2] = {
		{ 0xff, BITMODE_RESET },
		{ 0x40, BITMODE_CBUS }, /* AC8 = 0 */
		{ 0x44, BITMODE_CBUS }, /* AC8 = 1 */
		{ 0x40, BITMODE_CBUS }, /* AC8 = 0 */
		{ 0xff, BITMODE_RESET },
		{ 0xff, BITMODE_SYNCFF }
	};

	int ret = 0;
	for (size_t i = 0; i < ARRAY_SIZE(bitseq) && ret == 0; i++) {
		ret = ftdi_set_bitmode(mlsdr->ftdi, bitseq[i][0], bitseq[i][1]);
		usleep(10000);
	}

	return ret;
}

struct mlsdr *mlsdr_connect(const struct mlsdr_connect_cfg cfg)
{
	struct mlsdr *mlsdr = malloc(sizeof(struct mlsdr));
	bzero(mlsdr, sizeof(*mlsdr));

	mlsdr->ringsamples = mlsdr_ring_int16_t_new(10000000);
	mlsdr->ringpps = mlsdr_ring_mlsdr_pps_event_new(1000);
	mlsdr->priv = malloc(sizeof(struct mlsdr_priv));
	mlsdr->priv->ringresp = mlsdr_ring_uint8_t_new(100);
	atomic_init(&mlsdr->priv->adc_enabled, false);
	atomic_init(&mlsdr->priv->should_run, false);
	mlsdr->priv->iqtmp_len = 100000;
	mlsdr->priv->iqtmp = calloc(mlsdr->priv->iqtmp_len,
								sizeof(mlsdr->priv->iqtmp[0]));
	mlsdr->priv->iqconv = mlsdr_iqconv_new();

	if (cfg.logctx != NULL) {
		// Use the default log context
		mlsdr->logctx = cfg.logctx;
		mlsdr->priv->own_logctx = false;
	} else {
		mlsdr->logctx = malloc(sizeof(struct mlsdr_logctx));
		mlsdr->logctx->level = cfg.loglevel;
		mlsdr->logctx->log_cb = mlsdr_default_log;
		mlsdr->priv->own_logctx = true;
	}

	mlsdr->ftdi = ftdi_new();

	int ret = ftdi_usb_open_desc(mlsdr->ftdi, MLSDR_VID, MLSDR_PID,
								 "mlaticka-sdr", cfg.serno);
	if (ret < 0) {
		log_error_errno(mlsdr, "device not found");
		goto error;
	}

	// Initialize the FTDI
	// TODO: Error checking
	ftdi_usb_purge_buffers(mlsdr->ftdi);
	ftdi_set_bitmode(mlsdr->ftdi, 0xff, BITMODE_RESET);
	ftdi_set_latency_timer(mlsdr->ftdi, 2);
	ftdi_read_data_set_chunksize(mlsdr->ftdi, 0x10000);
	ftdi_write_data_set_chunksize(mlsdr->ftdi, 0x10000);

	mlsdr_fpga_reset(mlsdr);

	sem_init(&mlsdr->priv->recvthread_sem, 0, 0);
	mlsdr_run(mlsdr);
	sem_wait(&mlsdr->priv->recvthread_sem);

	// Check whether we can read/write the registers
	mlsdr->priv->regmap = regmap_new(mlsdr_regmap_read, mlsdr_regmap_write,
									 mlsdr_reg_cache_masks, ARRAY_SIZE(mlsdr_reg_cache_masks),
									 mlsdr);
	char regids[][2] = {
		{ MLSDR_REG_ID1, 'a' },
		{ MLSDR_REG_ID2, 't' },
		{ MLSDR_REG_ID3, 'x' },
	};
	for (size_t i = 0; i < ARRAY_SIZE(regids); i++) {
		uint8_t val;
		mlsdr_read_register(mlsdr, regids[i][0], &val);
		if (val != regids[i][1]) {
			log_error(mlsdr, "The ID register sequence does not match (%02x != %02x)",
					  val, regids[i][1]);
			goto error;
		}
	}

	mlsdr_read_register(mlsdr, MLSDR_REG_FEATURES, &mlsdr->features);
	if (mlsdr->features & MLSDR_FEATURES_RADIO) {
		// For now, we only support the external oscillator connector
		// (the board does not have an oscillator with sufficient frequency
		// for the tuner)
		mlsdr->r820t = r820t_new(mlsdr, cfg.ext_osc_freq);
	}

	return mlsdr;

error:

	mlsdr_destroy(mlsdr);

	return NULL;
}

void mlsdr_destroy(struct mlsdr *mlsdr)
{
	if (mlsdr->r820t != NULL) {
		r820t_destroy(mlsdr->r820t);
	}
	if (atomic_load(&mlsdr->priv->should_run)) {
		mlsdr_stop(mlsdr);
	}
	if (mlsdr->priv->iqtmp) {
		free(mlsdr->priv->iqtmp);
	}
	if (mlsdr->priv->iqconv) {
		mlsdr_iqconv_destroy(mlsdr->priv->iqconv);
	}
	if (mlsdr->ringsamples != NULL) {
		mlsdr_ring_int16_t_destroy(mlsdr->ringsamples);
	}
	if (mlsdr->ringpps != NULL) {
		mlsdr_ring_mlsdr_pps_event_destroy(mlsdr->ringpps);
	}
	if (mlsdr->priv->ringresp != NULL) {
		mlsdr_ring_uint8_t_destroy(mlsdr->priv->ringresp);
	}
	if (mlsdr->ftdi != NULL) {
		mlsdr_fpga_reset(mlsdr);
		ftdi_free(mlsdr->ftdi);
	}
	if (mlsdr->priv->regmap) {
		regmap_destroy(mlsdr->priv->regmap);
	}
	if (mlsdr->priv->own_logctx) {
		free(mlsdr->logctx);
	}
	free(mlsdr->priv);
	free(mlsdr);
}

struct mlsdr_desc *mlsdr_enumerate()
{
	struct ftdi_context *ctx = ftdi_new();
	struct ftdi_device_list *list;
	struct mlsdr_desc *start = NULL;

	int ret = ftdi_usb_find_all(ctx, &list, MLSDR_VID, MLSDR_PID);
	if (ret <= 0) {
		goto ftdi_free;
	}

	struct ftdi_device_list *ftat = list;
	struct mlsdr_desc *tail = NULL;
	do {
		struct mlsdr_desc *this = malloc(sizeof(struct mlsdr_desc));
		this->next = NULL;

		if (start == NULL) {
			start = this;
			tail = this;
		} else {
			tail->next = this;
			tail = tail->next;
		}

		ret = ftdi_usb_get_strings(ctx, ftat->dev,
				this->manufacturer, ARRAY_SIZE(this->manufacturer),
				this->description, ARRAY_SIZE(this->description),
				this->serno, ARRAY_SIZE(this->serno));
	} while ((ftat = ftat ->next) && ret == 0);

	ftdi_list_free(&list);

	if (ret != 0) {
		mlsdr_enumerate_free(start);
		start = NULL;
	}

ftdi_free:
	ftdi_free(ctx);

	return start;
}

void mlsdr_enumerate_free(struct mlsdr_desc *list)
{
	while (list != NULL) {
		struct mlsdr_desc *next = list->next;
		free(list);
		list = next;
	}
}
