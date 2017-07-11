
#include <string.h>


#include "regmap.h"

struct regmap *regmap_new(regmap_readfn *readfn, regmap_writefn *writefn,
						  const uint8_t *masks, size_t maskslen, void *userdata)
{
	struct regmap *regmap = malloc(sizeof(struct regmap));
	bzero(regmap, sizeof(*regmap));

	regmap->readfn = readfn;
	regmap->writefn = writefn;

	memcpy(regmap->cache.masks, masks, maskslen * sizeof(masks[0]));

	regmap->paused = false;
	regmap->userdata = userdata;

	// Initialize the cache
	for (int address = 0; address < REGMAP_SIZE; address++) {
		if (regmap->cache.masks[address] != 0x00) {
			// TODO: What to do if we fail to read here? That means we have invalid
			// cached data...
			regmap->readfn(regmap, address, &regmap->cache.values[address]);
		}
	}

	return regmap;
}

static inline bool is_fully_cached(struct regmap *regmap, uint8_t address, uint8_t mask)
{
	return (regmap->cache.masks[address] & mask) == mask;
}

int regmap_read(struct regmap *regmap, uint8_t address, uint8_t *value)
{
	return regmap_read_mask(regmap, address, 0xff, value);
}

int regmap_read_mask(struct regmap *regmap, uint8_t address, uint8_t mask, uint8_t *value)
{
	if (is_fully_cached(regmap, address, mask)) {
		// We might as well return the value straight away
		*value = regmap->cache.values[address];
		return 0;
	}

	int ret = regmap->readfn(regmap, address, value);
	if (ret == 0) {
		regmap->cache.values[address] = *value & regmap->cache.masks[address];
	}

	return ret;
}

int regmap_write(struct regmap *regmap, uint8_t address, uint8_t value)
{
	return regmap_write_mask(regmap, address, 0xff, value);
}

static inline bool is_dirty(struct regmap *regmap, uint8_t address)
{
	return !!(regmap->dirtyflags[address / 64] & (1 << (address % 64)));
}

static inline void set_dirty(struct regmap *regmap, uint8_t address)
{
	regmap->dirtyflags[address / 64] |= 1 << (address % 64);
}

static inline void clear_dirty(struct regmap *regmap, uint8_t address)
{
	regmap->dirtyflags[address / 64] &= ~(1 << (address % 64));
}

int regmap_write_mask(struct regmap *regmap, uint8_t address, uint8_t mask,
					  uint8_t value)
{
	int ret = 0;
	uint8_t newval = value & mask;
	uint8_t newcache = (regmap->cache.values[address] & ~mask) | newval;
	if (!regmap->paused) {
		uint8_t oldval;
		// Non-cached != read-only
		regmap_read(regmap, address, &oldval);
		newval |= (oldval & ~mask);
		ret = regmap->writefn(regmap, address, newval);
	} else {
		set_dirty(regmap, address);
	}

	regmap->cache.values[address] = newcache;
	return ret;
}

void regmap_pause(struct regmap *regmap)
{
	regmap->paused = true;
}

void regmap_flush(struct regmap *regmap)
{
	regmap->paused = false;
	for (int address = 0; address < REGMAP_SIZE; address++) {
		if (is_dirty(regmap, address)) {
			uint8_t oldval = 0;
			if (regmap->cache.masks[address] != 0xff) {
				regmap->readfn(regmap, address, &oldval);
			}
			uint8_t mask = regmap->cache.masks[address];
			regmap->writefn(regmap, address,
							(regmap->cache.values[address] & mask) |
							(oldval & ~mask));
			clear_dirty(regmap, address);
		}
	}
}

void regmap_destroy(struct regmap *regmap)
{
	free(regmap);
}
