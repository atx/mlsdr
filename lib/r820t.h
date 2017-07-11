

#ifndef R820T_H
#define R820T_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mlsdr.h"
#include "regmap.h"

struct r820t {
	struct regmap *regmap;
	unsigned int refclk;
	struct mlsdr *mlsdr;

	uint_least32_t int_freq;

	uint8_t address;
};

struct r820t *r820t_new(struct mlsdr *mlsdr, unsigned int refclk);

int r820t_read_registers(struct r820t *rt, uint8_t *value, uint8_t len);
int r820t_write_register(struct r820t *rt, uint8_t address, uint8_t value);
int r820t_write_register_mask(struct r820t *rt, uint8_t address, uint8_t mask, uint8_t value);

enum r820t_gain_stage {
	R820T_GAIN_LNA = 0,
	R820T_GAIN_MIXER = 1,
	R820T_GAIN_VGA = 2,
};
size_t r820t_get_gain_stage_steps(enum r820t_gain_stage stage, const int **values);
int r820t_set_gain_stage(struct r820t *rt, enum r820t_gain_stage stage,
						 int gain, int *actual_gain);
int r820t_autogain(struct r820t *rt);
int r820t_set_gain(struct r820t *rt, int gain);
int r820t_tune(struct r820t *rt, uint_least32_t freq);

void r820t_destroy(struct r820t *rt);

#ifdef __cplusplus
}
#endif

#endif
