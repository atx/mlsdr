
#include <argp.h>
#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <signal.h>
#include <fcntl.h>

#include "utils.h"
#include "regmap.h"
#include "r820t.h"
#include "mlsdr.h"

const char *argp_program_version = "mlsdr_test 1.0";
const char *argp_program_bug_address = "<atx@atx.name>";

static char doc[] = "Simple tool for connecting to mlsdr device and performing some basic tests.";

enum mlsdr_sdr_opts {
	OPTION_ADCMODE = 1000
};

static struct argp_option argp_options[] = {
	{ "verbose",		'v', NULL,		0,		"Increase verbosity", 0 },
	{ "quiet",			'q', NULL,		0,		"Decrease verbosity", 0 },
	{ "serno",			's', "SERNO",	0,		"Connect to a device with the given serial number", 0 },
	{ NULL,				0,		0,	0,			NULL,	0 }
};

struct arguments {
	enum mlsdr_loglevel loglevel;
	char *serno;

	struct mlsdr *mlsdr;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	struct arguments *args = state->input;

	switch (key) {
	case 'v':
		if (args->loglevel > MLSDR_LOG_DEBUG) {
			args->loglevel--;
		}
		break;
	case 'q':
		if (args->loglevel <= MLSDR_LOG_ERROR) {
			args->loglevel++;
		}
		break;
	case 's':
		args->serno = arg;
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

static struct argp argp_parser = { argp_options, parse_opt, NULL, NULL, NULL, NULL, NULL };

static struct arguments args = {
	.loglevel = MLSDR_LOG_INFO,
	.serno = NULL,
};

enum test_result {
	RESULT_SUCCESS = 0,
	RESULT_FAILURE = 1,
	RESULT_CRITICAL = 2
};

struct test {
	const char *name;
	enum test_result (*fn)(struct mlsdr *);
};

static enum test_result test_scratchpad(struct mlsdr *mlsdr)
{
	uint8_t val = 0x34;
	mlsdr_write_register(mlsdr, MLSDR_REG_SCRATCH, val);
	uint8_t read;
	mlsdr_read_register(mlsdr, MLSDR_REG_SCRATCH, &read);
	if (read != val) {
		mlsdr_log_error(mlsdr->logctx,
						"Scratchpad read does not contain expected value (%02x != %02x)",
						read, val);
		return RESULT_CRITICAL;
	}
	return RESULT_SUCCESS;
}

static enum test_result test_sawtooth(struct mlsdr *mlsdr)
{
	enum test_result ret = RESULT_SUCCESS;
	// Sawtooth pattern
	mlsdr_adc_set_mode(mlsdr, MLSDR_ADCTL_MODE_PAT1);
	mlsdr_adc_enable(mlsdr);

	ssize_t chunk = 1000000;
	ssize_t samples_max = 15000000L * 20; // 15Msps for 20 seconds
	ssize_t nsamples = 0;
	struct timespec start;
	int16_t *rawdata = calloc(chunk, sizeof(int16_t));
	bool first = true;
	int16_t at = 0;
	while (nsamples < samples_max) {
		ssize_t ret = mlsdr_read(mlsdr, rawdata, chunk, 2000);
		if (ret < 0) {
			mlsdr_log_error(mlsdr->logctx, "Failed with %ld\n", ret);
			break;
		}
		if (ret == 0) {
			mlsdr_log_error(mlsdr->logctx, "Timed out while waiting for samples\n");
			break;
		}
		if (first) {
			// There is no good way of figuring out the point at which the ADC
			// actually starts up, so just drop the first chunk.
			clock_gettime(CLOCK_MONOTONIC, &start);
		} else {
			nsamples += ret;
		}

		// Verify that we are getting sawtooth
		for (int i = 0; i < ret; i++) {
			if (first) {
				at = rawdata[i];
				first = false;
			} else {
				if (at != rawdata[i]) {
					mlsdr_log_warn(mlsdr->logctx, "Expected %d, got %d ([%d])", at, rawdata[i], i);
					ret = RESULT_FAILURE;
					at = rawdata[i];
				}
			}
			at++;
			if (at >= 2048) {
				at = -2048;
			}
		}
	}

	struct timespec end;
	clock_gettime(CLOCK_MONOTONIC, &end);

	mlsdr_adc_disable(mlsdr);

	long timediff_ms = (end.tv_sec - start.tv_sec) * 1000 +
					   (end.tv_nsec - start.tv_nsec) / 1000000;
	float sample_rate = (float)nsamples / timediff_ms * 1e3;
	mlsdr_log_info(mlsdr->logctx, "Measured %.6f Ms/s", sample_rate / 1e6);
	// We are not expecting this to be exactly on point here, as the measuring
	// interval is fairly short.
	if (fabs(sample_rate - 15e6) > 0.04e6) {
		ret = RESULT_FAILURE;
	}

	free(rawdata);

	return ret;
}

const char *test_result_name[] = {
	[RESULT_SUCCESS] = "Success",
	[RESULT_FAILURE] = "Failure",
	[RESULT_CRITICAL] = "Critical"
};

int main(int argc, char *argv[])
{
	argp_parse(&argp_parser, argc, argv, 0, 0, &args);

	struct mlsdr_connect_cfg cfg = MLSDR_DEFAULT_CFG;
	cfg.loglevel = args.loglevel;
	cfg.serno = args.serno;

	struct mlsdr *mlsdr = mlsdr_connect(cfg);
	if (mlsdr == NULL) {
		// We already get an error message from the library
		return EXIT_FAILURE;
	}

	struct test tests[] = {
		{
			.name = "Scratchpad read/write",
			.fn = test_scratchpad,
		},
		{
			.name = "Sawtooth pattern generator",
			.fn = test_sawtooth,
		}
	};

	for (int i = 0; i < ARRAY_SIZE(tests); i++) {
		struct test *test = &tests[i];
		mlsdr_log_info(mlsdr->logctx, "========== %s ==========", test->name);
		enum test_result result = test->fn(mlsdr);
		mlsdr_log_info(mlsdr->logctx, "=========> %s", test_result_name[result]);
		if (result == RESULT_CRITICAL) {
			break;
		}
	}

	mlsdr_destroy(mlsdr);
}
