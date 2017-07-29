
#include <argp.h>
#include <unistd.h>
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
	char *ppsfile;
	uint8_t adcmode;
	uint_least32_t ext_osc_freq;

	struct mlsdr *mlsdr;
	atomic_bool terminating;
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

int main(int argc, char *argv[])
{
	argp_parse(&argp_parser, argc, argv, 0, 0, &args);

	atomic_store(&args.terminating, false);

	struct mlsdr_connect_cfg cfg = MLSDR_DEFAULT_CFG;
	cfg.loglevel = args.loglevel;
	cfg.serno = args.serno;
	if (args.ext_osc_freq != 0) {
		cfg.ext_osc_freq = args.ext_osc_freq;
	}

	struct mlsdr *mlsdr = mlsdr_connect(cfg);
	if (mlsdr == NULL) {
		// We already get an error message from the library
		return EXIT_FAILURE;
	}

	// Sawtooth pattern
	mlsdr_adc_set_mode(mlsdr, MLSDR_ADCTL_MODE_PAT1);

	mlsdr_adc_enable(mlsdr);

	ssize_t chunk = 1000000;
	ssize_t nsamples = 25000000 * 30; // 25Msps for 30 seconds
	int16_t *rawdata = calloc(chunk, sizeof(int16_t));

	bool first = true;
	int16_t at = 0;
	while (nsamples != 0 && !atomic_load(&args.terminating)) {
		size_t toread = nsamples > 0 ? min(chunk, nsamples) : chunk;
		ssize_t ret = mlsdr_read(mlsdr, rawdata, toread, 10000);
		if (ret < 0) {
			fprintf(stderr, "Failed with %d\n", (int)ret);
			break;
		}
		if (nsamples > 0) {
			nsamples -= ret;
		}
		// Verify that we are getting sawtooth
		for (int i = 0; i < ret; i++) {
			if (first) {
				at = rawdata[i];
				first = false;
			} else {
				if (at != rawdata[i]) {
					mlsdr_log_warn(mlsdr->logctx, "Expected %d, got %d ([%d])", at, rawdata[i], i);
					at = rawdata[i];
				}
			}
			at++;
			if (at >= 2048) {
				at = -2048;
			}
		}
	}

	free(rawdata);

	mlsdr_destroy(mlsdr);
}
