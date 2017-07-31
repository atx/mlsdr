
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

const char *argp_program_version = "mlsdr_sdr 1.0";
const char *argp_program_bug_address = "<atx@atx.name>";

static char doc[] = "Simple tool to capture from a mlsdr device to stdout.";

enum mlsdr_sdr_opts {
	OPTION_ADCMODE = 1000
};

static struct argp_option argp_options[] = {
	{ "frequency",		'f', "FREQ",	0,		"Frequency to tune to (float)", 0 },
	{ "gain",			'g', "GAIN",	0,		"Gain to use", 0 },
	{ "lna-gain",		'1', "GAIN",	0,		"LNA gain", 0 },
	{ "mixer-gain",		'2', "GAIN",	0,		"Mixer gain", 0 },
	{ "vga-gain",		'3', "GAIN",	0,		"VGA gain", 0 },
	{ "verbose",		'v', NULL,		0,		"Increase verbosity", 0 },
	{ "quiet",			'q', NULL,		0,		"Decrease verbosity", 0 },
	{ "serno",			's', "SERNO",	0,		"Connect to a device with the given serial number", 0 },
	{ "samples",		'n', "SAMPLES",	0,		"Capture SAMPLES samples.", 0 },
	{ "data-file",		'o', "FILE",	0,		"Output samples to a file", 0 },
	{ "pps-file",		'p', "FILE",	0,		"Store PPS timestamps in a file", 0 },
	{ "ext-osc",		'e', "FREQ",	0,		"External oscillator frequency", 0 },
	{ "adc-mode",		 OPTION_ADCMODE, "MODE", 0, "ADC pattern mode to use", 0 },
	{ NULL,				0,		0,	0,			NULL,	0 }
};

struct arguments {
	int gain;
	int gain_lna;
	int gain_mixer;
	int gain_vga;
	enum mlsdr_loglevel loglevel;
	char *serno;
	ssize_t nsamples;
	uint_least32_t frequency;
	char *datafile;
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
	case 'f':
		args->frequency = atof(arg);
		break;
	case 'g':
		if (!strcmp(arg, "auto")) {
			args->gain = -1;
		} else {
			args->gain = atof(arg) * 10;
		}
		break;
	case '1':
		args->gain_lna = atof(arg) * 10;
		break;
	case '2':
		args->gain_mixer = atof(arg) * 10;
		break;
	case '3':
		args->gain_vga = atof(arg) * 10;
		break;
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
	case 'n':
		args->nsamples = atol(arg);
		break;
	case 'o':
		args->datafile = arg;
		break;
	case 'p':
		args->ppsfile = arg;
		break;
	case 'e':
		args->ext_osc_freq = atof(arg);
		break;
	case OPTION_ADCMODE:
		if (strcmp(arg, "0") == 0) {
			args->adcmode = MLSDR_ADCTL_MODE_NORMAL;
		} else if (strcmp(arg, "1") == 0) {
			args->adcmode = MLSDR_ADCTL_MODE_PAT1;
		} else if (strcmp(arg, "2") == 0) {
			args->adcmode = MLSDR_ADCTL_MODE_PAT2;
		} else if (strcmp(arg, "3") == 0) {
			args->adcmode = MLSDR_ADCTL_MODE_PAT3;
		} else {
			argp_error(state, "Invalid ADC mode '%s'", arg);
		}
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

static void *pps_thread(void *args_)
{
	struct arguments *args = args_;
	// TODO: Add a way to properly terminate this...

	FILE *fout;
	if (strcmp(args->ppsfile, "-") == 0) {
		fout = stdout;
	} else {
		fout = fopen(args->ppsfile, "w");
		if (fout == NULL) {
			mlsdr_log_error_errno(args->mlsdr->logctx, "Failed to open the PPS output file");
		}
	}

	while (!atomic_load(&args->terminating)) {
		struct mlsdr_pps_event pps;

		int ret = mlsdr_ring_mlsdr_pps_event_pop(args->mlsdr->ringpps, &pps, 1, 100);
		if (ret != 0) {
			continue;
		}

		fprintf(fout, "%d\n", pps.timestamp);
		fflush(fout);
	}

	return NULL;
}

static struct argp argp_parser = { argp_options, parse_opt, NULL, NULL, NULL, NULL, NULL };

static struct arguments args = {
	.gain = -1,
	.gain_lna = -1,
	.gain_mixer = -1,
	.gain_vga = -1,
	.loglevel = MLSDR_LOG_INFO,
	.serno = NULL,
	.nsamples = -1,
	.datafile = NULL,
	.ppsfile = NULL,
	.frequency = 100000000,
	.ext_osc_freq = 0,
	.adcmode = MLSDR_ADCTL_MODE_NORMAL
};

void sigterm_handler(int signum)
{
	UNUSED(signum);
	atomic_store(&args.terminating, true);
}

int main(int argc, char *argv[])
{
	argp_parse(&argp_parser, argc, argv, 0, 0, &args);

	signal(SIGTERM, sigterm_handler);
	signal(SIGINT, sigterm_handler);

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

	args.mlsdr = mlsdr;

	pthread_t ppsthr;
	if (args.ppsfile != NULL) {
		pthread_create(&ppsthr, NULL, pps_thread, &args);
	}

	FILE *datafile = stdout;
	if (args.datafile != NULL) {
		datafile = fopen(args.datafile, "w");
	}

	if (args.adcmode != MLSDR_ADCTL_MODE_NORMAL) {
		mlsdr_adc_set_mode(mlsdr, args.adcmode);
	}

	// Not sure if we want this or not...
	//int flags = fcntl(fileno(stdout), F_GETFL);
	//fcntl(fileno(stdout), F_SETFL, flags | O_NONBLOCK);
	if (mlsdr->r820t != NULL) {
		r820t_tune(mlsdr->r820t, args.frequency);
		if (args.gain >= 0) {
			r820t_set_gain(mlsdr->r820t, args.gain);
		}
		if (args.gain_lna >= 0) {
			r820t_set_gain_stage(mlsdr->r820t, R820T_GAIN_LNA, args.gain_lna, NULL);
		}
		if (args.gain_mixer >= 0) {
			r820t_set_gain_stage(mlsdr->r820t, R820T_GAIN_MIXER, args.gain_mixer, NULL);
		}
		if (args.gain_vga >= 0) {
			r820t_set_gain_stage(mlsdr->r820t, R820T_GAIN_VGA, args.gain_vga, NULL);
		}
	}

	mlsdr_adc_enable(mlsdr);
	ssize_t chunk = 1000000;
	ssize_t nsamples = args.nsamples;
	int16_t *rawdata = calloc(chunk, sizeof(int16_t));
	while (nsamples != 0 && !atomic_load(&args.terminating)) {
		size_t toread = nsamples > 0 ? min(chunk, nsamples) : chunk;
		ssize_t ret = mlsdr_read(mlsdr, rawdata, toread, 10000);
		if (ret < 0) {
			fprintf(stderr, "Failed with %d\n", (int)ret);
			break;
		}
		ret = fwrite(rawdata, sizeof(int16_t), ret, datafile);
		if (ret < 0) {
			break;
		}
		if (nsamples > 0) {
			nsamples -= ret;
		}
	}

	atomic_store(&args.terminating, true);
	if (args.ppsfile != NULL) {
		pthread_join(ppsthr, NULL);
	}

	free(rawdata);

	mlsdr_destroy(mlsdr);
}
