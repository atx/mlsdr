
#include <stdlib.h>
#include <volk/volk.h>
#include <string.h>
#include <strings.h>

#include "utils.h"

#include "iqconv.h"

// We do http://stackoverflow.com/a/3784668 here, as some software (*cough* gqrx)
// does not support purely real signals

// Generated with:
//
// from scipy import signal
// 
// taps = signal.firwin(101, 0.5)

static float filter_taps[] = {
	-2.49987663489e-19, 0.000525321768513, -1.62914673387e-18, -0.000596685568496,
	4.16085294926e-18, 0.00072477344403, -7.95756411498e-18, -0.000915824084066,
	-1.01725916817e-18, 0.00117644916056, -3.26993744565e-18, -0.0015137721111,
	1.00752337994e-17, 0.00193562596982, -4.8076312426e-18, -0.0024508328738,
	-4.03968671585e-18, 0.00306959861107, -6.70423967523e-18, -0.00380407257264,
	2.27402575533e-17, 0.00466915104356, -8.84059186048e-18, -0.00568364775444,
	-1.22626169909e-17, 0.00687203475026, -1.10824529336e-17, -0.00826709774535,
	4.43710161255e-17, 0.00991411224998, -1.32889585184e-17, -0.0118776572506,
	1.43352987261e-17, 0.0142532343146, -1.53214657539e-17, -0.017188172462,
	1.62319071587e-17, 0.0209218135869, -1.70522647357e-17, -0.02586951138,
	1.77696009557e-17, 0.0328185913255, -1.83726029981e-17, -0.0434601859854,
	1.88517611605e-17, 0.0621973865063, -1.91995188323e-17, -0.105185838598,
	1.94103916675e-17, 0.317860970856, 0.499748469598, 0.317860970856,
	1.94103916675e-17, -0.105185838598, -1.91995188323e-17, 0.0621973865063,
	1.88517611605e-17, -0.0434601859854, -1.83726029981e-17, 0.0328185913255,
	1.77696009557e-17, -0.02586951138, -1.70522647357e-17, 0.0209218135869,
	1.62319071587e-17, -0.017188172462, -1.53214657539e-17, 0.0142532343146,
	1.43352987261e-17, -0.0118776572506, -1.32889585184e-17, 0.00991411224998,
	4.43710161255e-17, -0.00826709774535, -1.10824529336e-17, 0.00687203475026,
	-1.22626169909e-17, -0.00568364775444, -8.84059186048e-18, 0.00466915104356,
	2.27402575533e-17, -0.00380407257264, -6.70423967523e-18, 0.00306959861107,
	-4.03968671585e-18, -0.0024508328738, -4.8076312426e-18, 0.00193562596982,
	1.00752337994e-17, -0.0015137721111, -3.26993744565e-18, 0.00117644916056,
	-1.01725916817e-18, -0.000915824084066, -7.95756411498e-18, 0.00072477344403,
	4.16085294926e-18, -0.000596685568496, -1.62914673387e-18, 0.000525321768513,
	-2.49987663489e-19,
};

#define FIRLEN			ARRAY_SIZE(filter_taps)
#define MAX_CHUNK		1000000

static float zero_chunk[MAX_CHUNK];

struct mlsdr_iqconv {
	complex float *buffer;
	int cnt;
};

struct mlsdr_iqconv *mlsdr_iqconv_new()
{
	struct mlsdr_iqconv *iq = malloc(sizeof(struct mlsdr_iqconv));

	size_t alignment = volk_get_alignment();
	size_t bufflen = (FIRLEN + MAX_CHUNK) * sizeof(iq->buffer[0]);
	iq->buffer = volk_malloc(bufflen, alignment);
	bzero(iq->buffer, bufflen);

	return iq;
}

void mlsdr_iqconv_push(struct mlsdr_iqconv *iq, const int16_t *in, complex float *out,
					   size_t len)
{
	for (int i = 0; len > MAX_CHUNK; i++, len -= MAX_CHUNK) {
		mlsdr_iqconv_push(iq, in, out, MAX_CHUNK);
		in = &in[MAX_CHUNK];
		out = &out[MAX_CHUNK];
	}

	// This is really naive implementation TODO
	complex float *bstart = &iq->buffer[FIRLEN];
	for (size_t i = 0; i < len; i++) {
		bstart[i] = in[i] / 2048.0;
		switch (iq->cnt) {
		case 0:
			// 0
			break;
		case 1:
			// pi / 2
			bstart[i] *= I;
			break;
		case 2:
			// pi
			bstart[i] *= -1;
			break;
		case 3:
			bstart[i] *= -I;
			break;
		}
		iq->cnt = (iq->cnt + 1) % 4;
	}

	for (size_t i = 0; i < len / 2; i++) {
		volk_32fc_32f_dot_prod_32fc(&out[i],
				&bstart[-FIRLEN + 1 + i * 2],
				filter_taps,
				FIRLEN);
	}
	memmove(iq->buffer, bstart, FIRLEN);
}

void mlsdr_iqconv_destroy(struct mlsdr_iqconv *iq)
{
	volk_free(iq->buffer);
	free(iq);
}
