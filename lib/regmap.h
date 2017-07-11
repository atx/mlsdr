
#ifndef REGMAP_H
#define REGMAP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// This is a fairly limited implementation of what the Linux kernel has

#define REGMAP_SIZE		(UINT8_MAX + 1)

struct regmap;

typedef int (regmap_readfn)(struct regmap *, uint8_t, uint8_t *);
typedef int (regmap_writefn)(struct regmap *, uint8_t, uint8_t);

struct regmap {
	regmap_readfn *readfn;
	regmap_writefn *writefn;

	struct {
		uint8_t masks[REGMAP_SIZE];
		uint8_t values[REGMAP_SIZE];
	} cache;

	uint64_t dirtyflags[4];

	bool paused;
	void *userdata;
};

struct regmap *regmap_new(regmap_readfn *readfn, regmap_writefn *writefn,
						  const uint8_t *masks, size_t maskslen, void *userdata);

int regmap_read(struct regmap *regmap, uint8_t address, uint8_t *value);
int regmap_read_mask(struct regmap *regmap, uint8_t address, uint8_t mask, uint8_t *value);
int regmap_write(struct regmap *regmap, uint8_t address, uint8_t value);
int regmap_write_mask(struct regmap *regmap, uint8_t address, uint8_t mask,
					  uint8_t value);

void regmap_pause(struct regmap *regmap);
void regmap_flush(struct regmap *regmap);

void regmap_destroy(struct regmap *regmap);

#ifdef __cplusplus
}
#endif

#endif
