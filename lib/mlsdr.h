
#ifndef MLSDR_H
#define MLSDR_H

#ifdef __cplusplus

#include <atomic>
#include <complex>

// Ugly C++ hacks...
typedef std::complex<float> complex_float;

#else

#include <stdatomic.h>
#include <complex.h>

typedef complex float complex_float;

#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <bsd/sys/queue.h>

#define MLSDR_VID	0x0403
#define MLSDR_PID	0x6014

struct mlsdr_pps_event {
	uint_least32_t timestamp;
};

struct mlsdr_ring {
	void *ptr;

	size_t elsize;
	size_t nelems;
	size_t rptr;
	size_t wptr;

	pthread_mutex_t mtx;
	pthread_cond_t cond;
};

struct mlsdr_ring *mlsdr_ring_new(size_t elsize, size_t nelems);
size_t mlsdr_ring_push(struct mlsdr_ring *ring, void *els, size_t nelems);
ssize_t mlsdr_ring_pop(struct mlsdr_ring *ring, void *els, size_t nelems,
					   int timeout);
void mlsdr_ring_flush(struct mlsdr_ring *ring);
void mlsdr_ring_destroy(struct mlsdr_ring *ring);

#define MLSDR_RING_FN(x, s) mlsdr_ring_##x##s

#define MLSDR_RING_TYPED_NAME(type, name) \
struct mlsdr_ring_##name { \
	struct mlsdr_ring *sub; \
}; \
static inline struct mlsdr_ring_##name *MLSDR_RING_FN(name, _new)(size_t nelems) \
{ \
	struct mlsdr_ring_##name *ring = (struct mlsdr_ring_##name *)malloc(sizeof(struct mlsdr_ring_##name)); \
	ring->sub = mlsdr_ring_new(sizeof(type), nelems); \
	return ring; \
} \
static inline size_t MLSDR_RING_FN(name, _push)(struct mlsdr_ring_##name *ring, type *els, size_t nelems) \
{ \
	return mlsdr_ring_push(ring->sub, els, nelems); \
} \
static inline ssize_t MLSDR_RING_FN(name, _pop)(struct mlsdr_ring_##name *ring, type *els, size_t nelems, unsigned int timeout) \
{ \
	return mlsdr_ring_pop(ring->sub, els, nelems, timeout); \
} \
static inline void MLSDR_RING_FN(name, _flush)(struct mlsdr_ring_##name *ring) \
{ \
	mlsdr_ring_flush(ring->sub); \
} \
static inline void MLSDR_RING_FN(name, _destroy)(struct mlsdr_ring_##name *ring) \
{ \
	mlsdr_ring_destroy(ring->sub); \
	free(ring); \
}

#define MLSDR_RING_TYPED(type) MLSDR_RING_TYPED_NAME(type, type)

MLSDR_RING_TYPED(int16_t);
MLSDR_RING_TYPED(uint8_t);
MLSDR_RING_TYPED_NAME(struct mlsdr_pps_event, mlsdr_pps_event);

enum mlsdr_loglevel {
	MLSDR_LOG_DEBUG = 1,
	MLSDR_LOG_INFO = 2,
	MLSDR_LOG_WARN = 3,
	MLSDR_LOG_ERROR = 4,
	// Only for log level filtering
	MLSDR_LOG_NONE = 5,
};

struct mlsdr_logctx;

typedef void (mlsdr_logfn)(struct mlsdr_logctx *, enum mlsdr_loglevel, const char *,
						   unsigned int, const char *, ...);

struct mlsdr_logctx {
#ifdef __cplusplus
	std::atomic<enum mlsdr_loglevel> level;
#else
	_Atomic enum mlsdr_loglevel level;
#endif
	mlsdr_logfn *log_cb;
};

static inline void mlsdr_logctx_set_loglevel(struct mlsdr_logctx *ctx,
											 enum mlsdr_loglevel level)
{
	atomic_store(&ctx->level, level);
}

void mlsdr_default_log(struct mlsdr_logctx *mlsdr, enum mlsdr_loglevel level,
					   const char *func, unsigned int lineno,
					   const char *format, ...);


#define mlsdr_log_write(ctx__, level, format, args...) \
	{ \
		struct mlsdr_logctx *mlsdr_ = (ctx__); \
		if (mlsdr_->log_cb != NULL) { \
			mlsdr_->log_cb(mlsdr_, level, __func__, __LINE__, format, ##args); \
		} \
	}

#define mlsdr_log_debug(ctx_, format, args...) \
	mlsdr_log_write(ctx_, MLSDR_LOG_DEBUG, format, ##args)
#define mlsdr_log_info(ctx_, format, args...) \
	mlsdr_log_write(ctx_, MLSDR_LOG_INFO, format, ##args)
#define mlsdr_log_warn(ctx_, format, args...) \
	mlsdr_log_write(ctx_, MLSDR_LOG_WARN, format, ##args)
#define mlsdr_log_error(ctx_, format, args...) \
	mlsdr_log_write(ctx_, MLSDR_LOG_ERROR, format, ##args)
#define mlsdr_log_error_errno(ctx_, format, args...) \
	mlsdr_log_error(ctx_, format ": %s (%d)", ##args, strerror(errno), errno)

struct mlsdr;
struct mlsdr_priv;
struct ftdi_context;

#define MLSDR_REG_ADCTL			0x01
#define MLSDR_ADCTL_ENABLE		(1 << 0)
#define MLSDR_ADCTL_MODE_OFF	1
#define MLSDR_ADCTL_MODE_MASK	(0b11 << MLSDR_ADCTL_MODE_OFF)
#define MLSDR_ADCTL_MODE_NORMAL	(0 << MLSDR_ADCTL_MODE_OFF)
#define MLSDR_ADCTL_MODE_PAT1	(1 << MLSDR_ADCTL_MODE_OFF)
#define MLSDR_ADCTL_MODE_PAT2	(2 << MLSDR_ADCTL_MODE_OFF)
#define MLSDR_ADCTL_MODE_PAT3	(3 << MLSDR_ADCTL_MODE_OFF)
#define MLSDR_REG_I2CSTAT		0x10
#define MLSDR_I2CSTAT_BUSY		(1 << 0)
#define MLSDR_REG_I2CCMD		0x11
#define MLSDR_REG_I2CREAD		0x12
#define MLSDR_REG_ID1			0xf0
#define MLSDR_REG_ID2			0xf1
#define MLSDR_REG_ID3			0xf2
#define MLSDR_REG_SCRATCH		0xf3
#define MLSDR_REG_FEATURES		0xf5
#define MLSDR_FEATURES_RADIO	(1 << 0)
#define MLSDR_FEATURES_14BIT	(1 << 1)

struct r820t;

struct mlsdr {
	struct ftdi_context *ftdi;

	struct mlsdr_logctx *logctx;

	struct mlsdr_ring_int16_t *ringsamples;
	struct mlsdr_ring_mlsdr_pps_event *ringpps;

	pthread_t thread;

	uint8_t features;

	struct r820t *r820t;

	struct mlsdr_priv *priv;
};

struct mlsdr_connect_cfg {
	const char *serno;
	enum mlsdr_loglevel loglevel;
	struct mlsdr_logctx *logctx; // Note that logctx takes precedence over loglevel
	size_t sample_buf_size;
	uint_least32_t ext_osc_freq;
};

#define MLSDR_DEFAULT_CFG (struct mlsdr_connect_cfg)\
	{ .serno = NULL, .loglevel = MLSDR_LOG_INFO, .logctx = NULL, \
	  .sample_buf_size = 10000000, .ext_osc_freq = 30000000 }

struct mlsdr_desc {
	char manufacturer[256];
	char description[256];
	char serno[256];

	struct mlsdr_desc *next;
};

struct mlsdr_desc *mlsdr_enumerate();
void mlsdr_enumerate_free(struct mlsdr_desc *list);

struct mlsdr *mlsdr_connect(const struct mlsdr_connect_cfg);

void mlsdr_set_log_cb(struct mlsdr *mlsdr, mlsdr_logfn *cb);
void mlsdr_set_loglevel(struct mlsdr *mlsdr, enum mlsdr_loglevel loglevel);

ssize_t mlsdr_read(struct mlsdr *mlsdr, int16_t *samples, size_t len,
				   int timeout);
ssize_t mlsdr_read_iq(struct mlsdr *mlsdr, complex_float *samples, size_t len,
					  int timeout);
int mlsdr_read_register(struct mlsdr *mlsdr, uint8_t addr, uint8_t *ret);
int mlsdr_write_register(struct mlsdr *mlsdr, uint8_t addr, uint8_t val);
int mlsdr_write_register_mask(struct mlsdr *mlsdr, uint8_t addr, uint8_t mask,
							  uint8_t val);

int mlsdr_i2c_dev_read(struct mlsdr *mlsdr, uint8_t dev, uint8_t *val, uint8_t len);
int mlsdr_i2c_dev_write(struct mlsdr *mlsdr, uint8_t dev, uint8_t addr, uint8_t val);

int mlsdr_adc_enable(struct mlsdr *mlsdr);
int mlsdr_adc_disable(struct mlsdr *mlsdr);
int mlsdr_adc_set_mode(struct mlsdr *mlsdr, uint8_t mode);
unsigned long mlsdr_adc_get_sample_rate(struct mlsdr *mlsdr);

void mlsdr_destroy(struct mlsdr *mlsdr);

#ifdef __cplusplus
}
#endif

#endif
