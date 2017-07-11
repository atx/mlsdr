
#include <errno.h>
#include <unistd.h>

#include "utils.h"

#include "r820t.h"

// See the kernel driver, librtlsdr and R820T2_Register_Description.pdf
// for reference on all of this


// TODO: #define register addresses/values properly

#define R820T_ADDRESS	0x1a
#define R820T_MAGIC		0x96

#define log_debug(rt, format, ...) mlsdr_log_debug((rt)->mlsdr->logctx, format, ##__VA_ARGS__)
#define log_info(rt, format, ...) mlsdr_log_info((rt)->mlsdr->logctx, format, ##__VA_ARGS__)
#define log_warn(rt, format, ...) mlsdr_log_warn((rt)->mlsdr->logctx, format, ##__VA_ARGS__)
#define log_error(rt, format, ...) mlsdr_log_error((rt)->mlsdr->logctx, format, ##__VA_ARGS__)
#define log_error_errno(rt, format, ...) mlsdr_log_error_errno((rt)->mlsdr->logctx, format, ##__VA_ARGS__)

struct r820t_freq_range {
	uint_least32_t freq;
	uint8_t open_d;
	uint8_t rf_mux_ploy;
	uint8_t tf_c;
};

// Note that we are missing the xtal_capXp fields as we are running from an
// oscillator (compared to librtlsdr)
static const struct r820t_freq_range r820t_freq_ranges[] = {
	{
		.freq =			0,	/* Start freq, in MHz */
		.open_d =		0x08,	/* low */
		.rf_mux_ploy =	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
		.tf_c =			0xdf,	/* R27[7:0]  band2,band0 */
	}, {
		.freq =			50,	/* Start freq, in MHz */
		.open_d =		0x08,	/* low */
		.rf_mux_ploy =	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
		.tf_c =			0xbe,	/* R27[7:0]  band4,band1  */
	}, {
		.freq =			55,	/* Start freq, in MHz */
		.open_d =		0x08,	/* low */
		.rf_mux_ploy =	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
		.tf_c =			0x8b,	/* R27[7:0]  band7,band4 */
	}, {
		.freq =			60,	/* Start freq, in MHz */
		.open_d =		0x08,	/* low */
		.rf_mux_ploy =	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
		.tf_c =			0x7b,	/* R27[7:0]  band8,band4 */
	}, {
		.freq =			65,	/* Start freq, in MHz */
		.open_d =		0x08,	/* low */
		.rf_mux_ploy =	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
		.tf_c =			0x69,	/* R27[7:0]  band9,band6 */
	}, {
		.freq =			70,	/* Start freq, in MHz */
		.open_d =		0x08,	/* low */
		.rf_mux_ploy =	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
		.tf_c =			0x58,	/* R27[7:0]  band10,band7 */
	}, {
		.freq =			75,	/* Start freq, in MHz */
		.open_d =		0x00,	/* high */
		.rf_mux_ploy =	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
		.tf_c =			0x44,	/* R27[7:0]  band11,band11 */
	}, {
		.freq =			80,	/* Start freq, in MHz */
		.open_d =		0x00,	/* high */
		.rf_mux_ploy =	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
		.tf_c =			0x44,	/* R27[7:0]  band11,band11 */
	}, {
		.freq =			90,	/* Start freq, in MHz */
		.open_d =		0x00,	/* high */
		.rf_mux_ploy =	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
		.tf_c =			0x34,	/* R27[7:0]  band12,band11 */
	}, {
		.freq =			100,	/* Start freq, in MHz */
		.open_d =		0x00,	/* high */
		.rf_mux_ploy =	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
		.tf_c =			0x34,	/* R27[7:0]  band12,band11 */
	}, {
		.freq =			110,	/* Start freq, in MHz */
		.open_d =		0x00,	/* high */
		.rf_mux_ploy =	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
		.tf_c =			0x24,	/* R27[7:0]  band13,band11 */
	}, {
		.freq =			120,	/* Start freq, in MHz */
		.open_d =		0x00,	/* high */
		.rf_mux_ploy =	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
		.tf_c =			0x24,	/* R27[7:0]  band13,band11 */
	}, {
		.freq =			140,	/* Start freq, in MHz */
		.open_d =		0x00,	/* high */
		.rf_mux_ploy =	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
		.tf_c =			0x14,	/* R27[7:0]  band14,band11 */
	}, {
		.freq =			180,	/* Start freq, in MHz */
		.open_d =		0x00,	/* high */
		.rf_mux_ploy =	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
		.tf_c =			0x13,	/* R27[7:0]  band14,band12 */
	}, {
		.freq =			220,	/* Start freq, in MHz */
		.open_d =		0x00,	/* high */
		.rf_mux_ploy =	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
		.tf_c =			0x13,	/* R27[7:0]  band14,band12 */
	}, {
		.freq =			250,	/* Start freq, in MHz */
		.open_d =		0x00,	/* high */
		.rf_mux_ploy =	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
		.tf_c =			0x11,	/* R27[7:0]  highest,highest */
	}, {
		.freq =			280,	/* Start freq, in MHz */
		.open_d =		0x00,	/* high */
		.rf_mux_ploy =	0x02,	/* R26[7:6]=0 (LPF)  R26[1:0]=2 (low) */
		.tf_c =			0x00,	/* R27[7:0]  highest,highest */
	}, {
		.freq =			310,	/* Start freq, in MHz */
		.open_d =		0x00,	/* high */
		.rf_mux_ploy =	0x41,	/* R26[7:6]=1 (bypass)  R26[1:0]=1 (middle) */
		.tf_c =			0x00,	/* R27[7:0]  highest,highest */
	}, {
		.freq =			450,	/* Start freq, in MHz */
		.open_d =		0x00,	/* high */
		.rf_mux_ploy =	0x41,	/* R26[7:6]=1 (bypass)  R26[1:0]=1 (middle) */
		.tf_c =			0x00,	/* R27[7:0]  highest,highest */
	}, {
		.freq =			588,	/* Start freq, in MHz */
		.open_d =		0x00,	/* high */
		.rf_mux_ploy =	0x40,	/* R26[7:6]=1 (bypass)  R26[1:0]=0 (highest) */
		.tf_c =			0x00,	/* R27[7:0]  highest,highest */
	}, {
		.freq =			650,	/* Start freq, in MHz */
		.open_d =		0x00,	/* high */
		.rf_mux_ploy =	0x40,	/* R26[7:6]=1 (bypass)  R26[1:0]=0 (highest) */
		.tf_c =			0x00,	/* R27[7:0]  highest,highest */
	}
};

// As the device sends the data _LSB_ first, we need to flip it around
static inline uint8_t reverse_bits(uint8_t in)
{
	// Generated with:
	// for x in range(256):
	//     print("0x%02x" % int('{:08b}'.format(x)[::-1], 2), end=", ")
	const uint8_t table[UINT8_MAX + 1] = {
		0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 0x10, 0x90, 0x50, 0xd0,
		0x30, 0xb0, 0x70, 0xf0, 0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
		0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8, 0x04, 0x84, 0x44, 0xc4,
		0x24, 0xa4, 0x64, 0xe4, 0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
		0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 0x1c, 0x9c, 0x5c, 0xdc,
		0x3c, 0xbc, 0x7c, 0xfc, 0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
		0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2, 0x0a, 0x8a, 0x4a, 0xca,
		0x2a, 0xaa, 0x6a, 0xea, 0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
		0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 0x16, 0x96, 0x56, 0xd6,
		0x36, 0xb6, 0x76, 0xf6, 0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
		0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe, 0x01, 0x81, 0x41, 0xc1,
		0x21, 0xa1, 0x61, 0xe1, 0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
		0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 0x19, 0x99, 0x59, 0xd9,
		0x39, 0xb9, 0x79, 0xf9, 0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
		0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5, 0x0d, 0x8d, 0x4d, 0xcd,
		0x2d, 0xad, 0x6d, 0xed, 0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
		0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 0x13, 0x93, 0x53, 0xd3,
		0x33, 0xb3, 0x73, 0xf3, 0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
		0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb, 0x07, 0x87, 0x47, 0xc7,
		0x27, 0xa7, 0x67, 0xe7, 0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
		0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 0x1f, 0x9f, 0x5f, 0xdf,
		0x3f, 0xbf, 0x7f, 0xff,
	};

	return table[in];
}

int r820t_read_registers(struct r820t *rt, uint8_t *value, uint8_t len)
{
	int ret = mlsdr_i2c_dev_read(rt->mlsdr, rt->address, value, len);
	if (ret != 0) {
		log_error_errno(rt, "Failed reading R820T registers!");
		return ret;
	}

	for (int i = 0; i < len; i++) {
		value[i] = reverse_bits(value[i]);
		log_debug(rt, "read[0x%02x] = 0x%02x", i, value[i]);
	}

	return 0;
}

int r820t_write_register(struct r820t *rt, uint8_t address, uint8_t value)
{
	return regmap_write(rt->regmap, address, value);
}

int r820t_write_register_mask(struct r820t *rt, uint8_t address, uint8_t mask, uint8_t value)
{
	return regmap_write_mask(rt->regmap, address, mask, value);
}

static int r820t_regmap_read(struct regmap *regmap, uint8_t address, uint8_t *value)
{
	UNUSED(regmap);
	UNUSED(address);
	// So, due to the weird way the R820T is read, we don't really have a read
	// routine and just return 0x00
	*value = 0x00;
	return 0;
}

static int r820t_regmap_write(struct regmap *regmap, uint8_t address, uint8_t value)
{
	struct r820t *rt = regmap->userdata;
	log_debug(rt, "Wrote %02x to %02x", value, address);
	return mlsdr_i2c_dev_write(rt->mlsdr, R820T_ADDRESS, address, value);
}

static int r820t_lna_auto_enable(struct r820t *rt)
{
	return r820t_write_register_mask(rt, 0x05, 0x10, 0x00);
}

static int r820t_lna_auto_disable(struct r820t *rt)
{
	return r820t_write_register_mask(rt, 0x05, 0x10, 0x10);
}

static int r820t_mixer_auto_enable(struct r820t *rt)
{
	return r820t_write_register_mask(rt, 0x07, 0x10, 0x10);
}

static int r820t_mixer_auto_disable(struct r820t *rt)
{
	return r820t_write_register_mask(rt, 0x07, 0x10, 0x00);
}

static const int r820t_lna_gain_values[] = {
	0, 9, 22, 62, 100, 113, 144, 166, 192, 223, 249, 263, 282, 287, 322, 335
};

static const int r820t_mixer_gain_values[] = {
	0, 5, 15, 25, 44, 53, 63, 88, 105, 115, 123, 139, 152, 158, 161, 153
};

// The VGA gain is specified as 3.5dB/step in the documentation, this is from
// librtlsdr and presumably is more accurate
static const int r820t_vga_gain_values[] = {
	0, 26, 52, 82, 124, 159, 183, 196, 210, 242, 278, 312, 347, 384, 419, 455
};

static const int *r820t_stage_gain_values[] = {
	[R820T_GAIN_LNA] = r820t_lna_gain_values,
	[R820T_GAIN_MIXER] = r820t_mixer_gain_values,
	[R820T_GAIN_VGA] = r820t_vga_gain_values,
};

static const size_t r820t_stage_gain_lens[] = {
	[R820T_GAIN_LNA] = ARRAY_SIZE(r820t_lna_gain_values),
	[R820T_GAIN_MIXER] = ARRAY_SIZE(r820t_mixer_gain_values),
	[R820T_GAIN_VGA] = ARRAY_SIZE(r820t_vga_gain_values),
};

size_t r820t_get_gain_stage_steps(enum r820t_gain_stage stage, const int **values)
{
	*values= r820t_stage_gain_values[stage];
	return r820t_stage_gain_lens[stage];
}

int r820t_set_gain_stage(struct r820t *rt, enum r820t_gain_stage stage,
						 int gain, int *actual_gain)
{
	// Find the closest gain
	const int *values;
	size_t len = r820t_get_gain_stage_steps(stage, &values);

	size_t gain_i = 0;
	for (size_t i = 1; i < len; i++) {
		if (abs(values[i] - gain) < abs(values[gain_i] - gain)) {
			gain_i = i;
		}
	}

	if (actual_gain != NULL) {
		*actual_gain = values[gain_i];
	}

	int ret = 0;
	switch (stage) {
	case R820T_GAIN_LNA:
		r820t_lna_auto_disable(rt);
		ret = r820t_write_register_mask(rt, 0x05, 0x0f, gain_i);
		log_info(rt, "Set LNA gain to %.2f dB", values[gain_i] * 0.1);
		break;
	case R820T_GAIN_MIXER:
		r820t_mixer_auto_disable(rt);
		ret = r820t_write_register_mask(rt, 0x07, 0x0f, gain_i);
		log_info(rt, "Set Mixer gain to %.2f dB", values[gain_i] * 0.1);
		break;
	case R820T_GAIN_VGA:
		// Note that we are setting the bits to control the VGA manually
		// instead of the vagc pin. This should not be necessary, but librtlsdr
		// thinks it is a good idea.
		ret = r820t_write_register_mask(rt, 0x0c, 0x9f, gain_i);
		log_info(rt, "Set VGA gain to %.2f dB", values[gain_i] * 0.1);
		break;
	}

	return ret;
}

int r820t_autogain(struct r820t *rt)
{
	// TODO: We probably want to have a look at the AGC parameters and tune them
	// for our use case.
	int ret = r820t_lna_auto_enable(rt);
	if (ret != 0) {
		return ret;
	}
	ret = r820t_mixer_auto_enable(rt);
	if (ret != 0) {
		return ret;
	}

	// VGA does not have AGC, so just set it to constant gain and pray that it
	// works well.
	ret = r820t_set_gain_stage(rt, R820T_GAIN_VGA, 278, NULL);
	if (ret != 0) {
		return ret;
	}

	log_info(rt, "AGC activated");

	return 0;
}

int r820t_set_gain(struct r820t *rt, int gain)
{
	int ret = r820t_lna_auto_disable(rt);
	if (ret != 0) {
		return ret;
	}
	ret = r820t_mixer_auto_disable(rt);
	if (ret != 0) {
		return ret;
	}
	// Notice that we are not properly error unrolling. A failure would probably
	// be catastrophic anyway though

	// Set VGA gain to constant 16.3 dB
	ret = r820t_write_register_mask(rt, 0x0c, 0x9f, 0x0f);
	if (ret != 0) {
		return ret;
	}
	int total_gain = 0;
	int lna_i = 0;
	int mix_i = 0;
	for (int i = 0; i < 15 && total_gain < gain; i++) {
		total_gain = r820t_lna_gain_values[++lna_i] +
					 r820t_mixer_gain_values[++mix_i];
	}

	ret = r820t_write_register_mask(rt, 0x05, 0x0f, lna_i);
	if (ret != 0) {
		return ret;
	}
	ret = r820t_write_register_mask(rt, 0x07, 0x0f, mix_i);
	if (ret != 0) {
		return ret;
	}

	log_info(rt, "Set R820T gain to %.2f", (float)total_gain / 10);

	return 0;
}

#define VCO_MIN		1770000
#define VCO_MAX		(2 * VCO_MIN)
#define VCO_POWER_REF	2

static int r820t_set_pll(struct r820t *rt, uint32_t freq)
{
	uint32_t freq_khz = (freq + 500) / 1000;
	uint32_t pll_ref = rt->refclk;

	log_debug(rt, "set_pll %d", freq);

	// If reference clock is above 24MHz, then divide it by 2 as per datasheet
	//bool refdiv2 = pll_ref > 24000000;
	bool refdiv2 = false;
	int ret = r820t_write_register_mask(rt, 0x10, 0x10, refdiv2 ? 0x10 : 0x00);
	if (ret != 0) {
		return ret;
	}
	if (refdiv2) {
		pll_ref /= 2;
	}

	// This sets "PLL auto tune clock rate"
	// Lower values could provide smaller phase noise (?)
	ret = r820t_write_register_mask(rt, 0x1a, 0x0c, 0x00);
	if (ret != 0) {
		return ret;
	}
	// Magical undefined register here, librtlsdr says that this sets "VCO current"
	ret = r820t_write_register_mask(rt, 0x12, 0xe0, 0x80);
	if (ret != 0) {
		return ret;
	}

	uint8_t mix_div = 2;
	uint8_t div_buf = 0;
	uint8_t div_num = 0;
	while (mix_div <= 64) {
		uint32_t vco_freq_khz = freq_khz * mix_div;
		if (vco_freq_khz >= VCO_MIN && vco_freq_khz < VCO_MAX) {
			div_buf = mix_div;
			while (div_buf > 2) {
				div_buf /= 2;
				div_num++;
			}
			break;
		}
		mix_div *= 2;
	}

	uint8_t data[5];
	ret = r820t_read_registers(rt, data, sizeof(data));
	if (ret != 0) {
		return ret;
	}

	uint8_t vco_fine_tune = (data[4] & 0x30) >> 4;
	if (vco_fine_tune > VCO_POWER_REF) {
		div_num--;
	} else if (vco_fine_tune < VCO_POWER_REF) {
		div_num++;
	}

	ret = r820t_write_register_mask(rt, 0x10, 0xe0, div_num << 5);
	if (ret != 0) {
		return ret;
	}

	uint64_t vco_freq = (uint64_t)freq * (uint64_t)mix_div;
	uint8_t nint = vco_freq / (2 * pll_ref);
	uint32_t vco_fra = (vco_freq - 2 * pll_ref * nint) / 1000;

	log_debug(rt, "vco_freq = %ld, nint = %d, vco_fra = %d", vco_freq, nint, vco_fra);
	// This is what librtlsdr does, TODO: Figure out if this is a good idea
	if (nint > (128 / VCO_POWER_REF - 1)) {
		log_error(rt, "No valid PLL values for %d Hz (VCO = %d Hz)", freq, vco_freq);
		return -1;
	}

	uint8_t ni = (nint - 13) / 4;
	uint8_t si = nint - 4 * ni - 13;

	ret = r820t_write_register(rt, 0x14, ni + (si << 6));
	if (ret != 0) {
		return ret;
	}

	// pw_sdm
	// Again, magical undocumented register
	ret = r820t_write_register_mask(rt, 0x12, 0x08, vco_fra ? 0x00 : 0x08);
	if (ret != 0) {
		return ret;
	}

	// Calculates PLL fractional divider
	uint32_t pll_ref_khz = (pll_ref + 500) / 1000;
	uint16_t n_sdm = 2;
	uint16_t sdm = 0;
	while (vco_fra > 1) {
		if (vco_fra > (2 * pll_ref_khz / n_sdm)) {
			sdm = sdm + 32768 / (n_sdm / 2);
			vco_fra = vco_fra - 2 * pll_ref_khz / n_sdm;
			if (n_sdm >= 0x8000) {
				break;
			}
		}
		n_sdm *= 2;
	}

	ret = r820t_write_register(rt, 0x16, sdm >> 8);
	if (ret != 0) {
		return ret;
	}
	ret = r820t_write_register(rt, 0x15, sdm & 0xff);
	if (ret != 0) {
		return ret;
	}

	// Attempt to lock the PLL
	for (int i = 0; i < 10; i++) {
		log_debug(rt, "Locking...");
		usleep(100000);
		ret = r820t_read_registers(rt, data, 3);
		if (ret != 0) {
			return ret;
		}
		// If the PLL is locked, then we are done
		if (data[2] & 0x40) {
			break;
		}

		if (i == 2) {
			// Increase VCO current
			// This register is not documented, so I have no idea whether this
			// is good value. In librtlsdr we trust.
			ret = r820t_write_register_mask(rt, 0x12, 0xe0, 0x60);
			if (ret != 0) {
				return ret;
			}
		}
	}

	if (!(data[2] & 0x40)) {
		log_error(rt, "PLL failed to lock");
		return -1;
	} else {
		log_info(rt, "PLL locked");
	}

	// Set autotune frequency to 8kHz
	return r820t_write_register_mask(rt, 0x1a, 0x08, 0x08);
}

int r820t_tune(struct r820t *rt, uint_least32_t freq)
{
	uint_least32_t lo_freq = freq -
		mlsdr_adc_get_sample_rate(rt->mlsdr) / 4 +
		mlsdr_adc_get_sample_rate(rt->mlsdr) / 2;

	// r82xx_set_mux from librtlsdr
	// Find the frequency range
	unsigned int freq_mhz = lo_freq / 1000000;
	const struct r820t_freq_range *range;
	size_t i;
	for (i = 0; i < ARRAY_SIZE(r820t_freq_ranges) - 1; i++) {
		if (freq_mhz < r820t_freq_ranges[i + 1].freq) {
			break;
		}
	}
	range = &r820t_freq_ranges[i];

	int ret = r820t_write_register_mask(rt, 0x17, 0x08, range->open_d);
	if (ret != 0) {
		return ret;
	}
	ret = r820t_write_register_mask(rt, 0x1a, 0xc3, range->rf_mux_ploy);
	if (ret != 0) {
		return ret;
	}
	ret = r820t_write_register(rt, 0x1b, range->tf_c);;
	if (ret != 0) {
		return ret;
	}
	ret = r820t_write_register_mask(rt, 0x10, 0x0b, 0x00);
	if (ret != 0) {
		return ret;
	}
	ret = r820t_write_register_mask(rt, 0x08, 0x3f, 0x00);
	if (ret != 0) {
		return ret;
	}
	ret = r820t_write_register_mask(rt, 0x08, 0x3f, 0x00);
	if (ret != 0) {
		return ret;
	}
	ret = r820t_write_register_mask(rt, 0x09, 0x3f, 0x00);
	if (ret != 0) {
		return ret;
	}

	ret = r820t_set_pll(rt, lo_freq);
	if (ret != 0) {
		return ret;
	}

	log_info(rt, "Tuned to %.3f MHz", (float)freq / 1e6);

	return 0;
}

static int r820t_init(struct r820t *rt)
{
	// Blatantly stolen from librtlsdr/tuner_r82xx.c
	// This function attempts to shift the tuner into some sort of
	// consistent state. librtlsdr does a lot of steps which seem unnecessary,
	// also based on the documentation, lot of the bits/registers being set just
	// just should not exist.
	// This could be because librtlsdr does not seem to distinguish between the
	// various R8xx tuners very much.
	const uint8_t init_vals[] = {
		0x83, 0x32, 0x75, 0xc0, 0x40, 0xd6,
		0x6c, 0xf5, 0x63, 0x75, 0x68, 0x6c,
		0x83, 0x80, 0x00, 0x0f, 0x00, 0xc0,
		0x30, 0x48, 0xcc, 0x60, 0x00, 0x54,
		0xae, 0x4a, 0xc0, 0x00, 0x00, 0x00
	};

	for (size_t i = 0; i < ARRAY_SIZE(init_vals); i++) {
		r820t_write_register(rt, 0x05 + i, init_vals[i]);
	}

	// TODO: We probably want this customizable
	// Currently, these are hard-coded values for 6MHz
	//unsigned int if_khz = 3570;
	unsigned int if_khz = 3770;
	unsigned int filt_cal_lo = 56000;
	uint8_t filt_gain = 0x10;
	uint8_t img_r = 0x00;
	uint8_t filt_q = 0x10;
	uint8_t hp_cor = 0x6b;
	uint8_t ext_enable = 0x60;
	uint8_t loop_through = BIT(7);
	uint8_t lt_att = 0x00;
	uint8_t flt_ext_widest = 0x00;
	uint8_t polyfil_cur = 0x60;

	rt->int_freq = if_khz * 1000;

	// VGA gain = 0b0000
	int ret = r820t_write_register_mask(rt, 0x0c, 0x0f, 0x00);
	if (ret != 0) {
		return ret;
	}

	// This register is not specified in the PDF, but librtlsdr sets it for some
	// reason
	ret = r820t_write_register_mask(rt, 0x13, 0x3f, 49);
	if (ret != 0) {
		return ret;
	}

	// This sets something called "Power detector take off points"
	// Maybe something to do with the LNA AGC?
	ret = r820t_write_register_mask(rt, 0x1d, 0x38, 0x00);
	if (ret != 0) {
		return ret;
	}

	uint8_t fil_cal_code = 0x00;
	int n_cals = 3;
	bool calibrated = false;
	// Calibration
	for (int i = 0; i < n_cals && !calibrated; i++) {
		// Set high pass filter
		ret = r820t_write_register_mask(rt, 0x0b, 0x60, hp_cor);
		if (ret != 0) {
			break;
		}
		// This enables some sort of "cali clk", undocumented in the Realtek PDF
		ret = r820t_write_register_mask(rt, 0x0f, 0x04, 0x04);
		if (ret != 0) {
			break;
		}
		// This sets the Xtal capacitors to 0pF, which is correct as we are
		// driving the tuner from an oscillator anyway
		ret = r820t_write_register_mask(rt, 0x10, 0x03, 0x00);
		if (ret != 0) {
			break;
		}
		ret = r820t_set_pll(rt, filt_cal_lo * 1000);
		// TODO: Check for PLL lock
		if (ret != 0) {
			break;
		}
		// librtlsdr says "Start trigger" here, but the PDF says that this
		// sets the HPF parameters, wtf
		ret = r820t_write_register_mask(rt, 0x0b, 0x10, 0x10);
		if (ret != 0) {
			break;
		}

		// Again, this is supposed to be "Stop trigger"
		ret = r820t_write_register_mask(rt, 0x0b, 0x10, 0x00);
		if (ret != 0) {
			break;
		}
		// This disables the "cali clk"
		ret = r820t_write_register_mask(rt, 0x0f, 0x04, 0x00);
		if (ret != 0) {
			break;
		}

		uint8_t data[5];
		ret = r820t_read_registers(rt, data, ARRAY_SIZE(data));
		if (ret != 0) {
			break;
		}

		// Again, the register 0x04 is undocumented in the PDF
		fil_cal_code = data[4] & 0x0f;
		calibrated = fil_cal_code != 0x00 && fil_cal_code != 0x0f;
	}

	if (ret != 0 || !calibrated) {
		log_error(rt, "Calibration failed, trying to proceed anyway");
	} else {
		log_info(rt, "Calibration succesful");
	}

	if (fil_cal_code == 0x0f) {
		fil_cal_code = 0x00;
	}

	// TODO: Fold these calls...

	ret = r820t_write_register_mask(rt, 0x0a, 0x1f, filt_q | fil_cal_code);
	if (ret != 0) {
		return ret;
	}

	// Set filter bandwidth and HPF bandwidth
	ret = r820t_write_register_mask(rt, 0x0b, 0xef, hp_cor);
	if (ret != 0) {
		return ret;
	}

	// Set "Img_R" (again, undefined bit in the specs...)
	ret = r820t_write_register_mask(rt, 0x07, 0x80, img_r);
	if (ret != 0) {
		return ret;
	}

	// Set filter gain (0db or 3db) and some undefined bit which is apparently
	// called "V6MHz"
	ret = r820t_write_register_mask(rt, 0x06, 0x30, filt_gain);
	if (ret != 0) {
		return ret;
	}

	// "Filter extension under weak signal"
	ret = r820t_write_register_mask(rt, 0x1e, 0x60, ext_enable);
	if (ret != 0) {
		return ret;
	}

	// Loop through
	ret = r820t_write_register_mask(rt, 0x05, 0x80, loop_through);
	if (ret != 0) {
		return ret;
	}

	// Loop through attenuation
	ret = r820t_write_register_mask(rt, 0x1f, 0x80, lt_att);
	if (ret != 0) {
		return ret;
	}

	// Filter extension widest (undefined bit)
	ret = r820t_write_register_mask(rt, 0x0f, 0x80, flt_ext_widest);
	if (ret != 0) {
		return ret;
	}

	// "RF poly filter current" (undefined)
	ret = r820t_write_register_mask(rt, 0x19, 0x60, polyfil_cur);
	if (ret != 0) {
		return ret;
	}

	// r82xx_sysfreq_sel from librtlsdr
	uint8_t mixer_top = 0x24;
	uint8_t lna_top = 0xe5;
	uint8_t cp_cur = 0x28;
	uint8_t div_buf_cur = 0x30;
	uint8_t lna_vth_l = 0x53;
	uint8_t mixer_vth_l = 0x75;
	//uint8_t air_cable1_in = 0x00;
	//uint8_t cable2_in = 0x00;
	//uint8_t pre_dect = 0x40;
	uint8_t lna_discharge = 14;
	uint8_t filter_cur = 0x40;

	uint8_t sysfreq_writes[][3] = {
		{ 0x1d, 0xc7, lna_top },
		{ 0x1c, 0xf8, mixer_top },
		{ 0x0d, 0xff, lna_vth_l },
		{ 0x0e, 0xff, mixer_vth_l },
		{ 0x11, 0x38, cp_cur },
		{ 0x17, 0x30, div_buf_cur },
		{ 0x0a, 0x60, filter_cur },
		{ 0x1d, 0x38, 0x00 },
		{ 0x1c, 0x04, 0x00 },
		{ 0x06, 0x40, 0x00 },
		{ 0x1a, 0x30, 0x30 },
		{ 0x1d, 0x38, 0x18 },
		{ 0x1c, 0x04, mixer_top },
		{ 0x1e, 0x1f, lna_discharge },
		{ 0x1a, 0x30, 0x20 },
		//// Set bandwidth to 6MHz, TODO: Make this configurable (see ^)
		//{ 0x0a, 0x10, 0x10 },
		//{ 0x0b, 0xef, 0x6b },
	};
	for (size_t i = 0; i < ARRAY_SIZE(sysfreq_writes); i++) {
		ret = r820t_write_register_mask(rt,
				sysfreq_writes[i][0], sysfreq_writes[i][1], sysfreq_writes[i][2]);
	}

	r820t_autogain(rt);

	// Okay, so at this point we should be initialized in some sort of consistent
	// state, hopefully
	log_info(rt, "R820T Initialized");

	return 0;
}

// The R820T2_Register_Description.pdf does not really name the registers,
// hooray for magical values!
const uint8_t r820t_reg_cache_masks[] = {
	[0x00] = 0x00, [0x01] = 0x00, [0x02] = 0xff, [0x03] = 0xff, [0x04] = 0x00,
	[0x05] = 0xff, [0x06] = 0xff, [0x07] = 0xff, [0x08] = 0xff, [0x09] = 0xff,
	[0x0a] = 0xff, [0x0b] = 0xff, [0x0c] = 0xff, [0x0d] = 0xff, [0x0e] = 0xff,
	[0x0f] = 0xff, [0x10] = 0xff, [0x11] = 0xff, [0x12] = 0xff, [0x13] = 0xff,
	[0x14] = 0xff, [0x15] = 0xff, [0x16] = 0xff, [0x17] = 0xff, [0x19] = 0xff,
	[0x1a] = 0xff, [0x1b] = 0xff, [0x1c] = 0xff, [0x1d] = 0xff, [0x1e] = 0xff,
	[0x1f] = 0xff,
};

struct r820t *r820t_new(struct mlsdr *mlsdr, unsigned int refclk)
{
	struct r820t *rt = malloc(sizeof(struct r820t));

	rt->mlsdr = mlsdr;
	rt->regmap = regmap_new(r820t_regmap_read, r820t_regmap_write,
							r820t_reg_cache_masks, ARRAY_SIZE(r820t_reg_cache_masks),
							rt);
	rt->refclk = refclk;
	// Currently, we support only the R820T, but this could change at some point
	rt->address = R820T_ADDRESS;

	// Initialization time!
	// Read and verify the ID
	uint8_t reg;
	int ret = r820t_read_registers(rt, &reg, 1);

	if (ret != 0) {
		goto fail;
	}
	if (reg != R820T_MAGIC) {
		log_error(rt, "Wrong R820T magic (0x%02x)", reg);
		goto fail;
	}

	ret = r820t_init(rt);
	if (ret != 0) {
		log_error(rt, "Failed to initialize the chip");
		goto fail;
	}

	return rt;

fail:
	r820t_destroy(rt);
	return NULL;
}

void r820t_destroy(struct r820t *rt)
{
	regmap_destroy(rt->regmap);
	free(rt);
}
