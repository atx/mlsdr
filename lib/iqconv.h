
#ifndef IQCONV_H
#define IQCONV_H

#include <complex.h>
#include <stddef.h>
#include <stdint.h>

struct mlsdr_iqconv;

struct mlsdr_iqconv *mlsdr_iqconv_new();
void mlsdr_iqconv_push(struct mlsdr_iqconv *iq, const int16_t *in, complex float *out,
					  size_t len);
void mlsdr_iqconv_destroy(struct mlsdr_iqconv *iq);


#endif
