
#include <errno.h>
#include <string.h>

#include "utils.h"

#include "mlsdr.h"

// Okay, adding to void * is apparently a GCC excension, fix that some day

struct mlsdr_ring *mlsdr_ring_new(size_t elsize, size_t nelems)
{
	struct mlsdr_ring *ring = malloc(sizeof(struct mlsdr_ring));

	ring->elsize = elsize;
	ring->nelems = nelems;
	ring->ptr = calloc(nelems, elsize);
	ring->rptr = 0;
	ring->wptr = 0;

	pthread_mutexattr_t mattr;
	pthread_mutexattr_init(&mattr);
	pthread_mutexattr_setprotocol(&mattr, PTHREAD_PRIO_INHERIT);
	pthread_mutex_init(&ring->mtx, &mattr);

	pthread_condattr_t cattr;
	pthread_condattr_init(&cattr);
	// Not sure if we should use CLOCK_MONOTONIC_RAW or CLOCK_MONOTONIC here
	// RAW seems to work fine on my Gentoo box, but on Debian Stretch it
	// all fails for some reason
	pthread_condattr_setclock(&cattr, CLOCK_MONOTONIC);
	pthread_cond_init(&ring->cond, &cattr);

	return ring;
}

static inline size_t mlsdr_ring_used(struct mlsdr_ring *ring)
{
	if (ring->wptr >= ring->rptr) {
		return ring->wptr - ring->rptr;
	}
	return ring->rptr + (ring->nelems - ring->wptr);
}

static inline size_t mlsdr_ring_free(struct mlsdr_ring *ring)
{
	return ring->nelems - mlsdr_ring_used(ring);
}

size_t mlsdr_ring_push(struct mlsdr_ring *ring, void *els, size_t nelems)
{
	pthread_mutex_lock(&ring->mtx);

	// We could wait for space to be available, but that somewhat defeats the
	// point of having a buffer, so just drop the elements and pray that this
	// does not happen often
	nelems = min(mlsdr_ring_free(ring) - 1, nelems);
	if (nelems == 0) {
		goto exit;
	}

	size_t nwptr = ring->wptr + nelems;

	size_t fr = min(nwptr, ring->nelems - 1) - ring->wptr;
	memcpy(ring->ptr + ring->wptr * ring->elsize, els, fr * ring->elsize);
	if (nwptr >= ring->nelems) {
		nwptr = nwptr % ring->nelems;
		memcpy(ring->ptr, els + fr * ring->elsize,
			   (nelems - fr) * ring->elsize);
	}

	ring->wptr = nwptr;

	pthread_cond_broadcast(&ring->cond);

exit:
	pthread_mutex_unlock(&ring->mtx);
	return nelems;
}

ssize_t mlsdr_ring_pop(struct mlsdr_ring *ring, void *els, size_t nelems,
					   int timeout)
{
	pthread_mutex_lock(&ring->mtx);

	struct timespec timespec;
	ssize_t ret = 0;
	if (timeout > 0) {
		ret = clock_add_ms(CLOCK_MONOTONIC, &timespec, timeout);
		if (ret < 0) {
			return ret;
		}
	}

	while (ret == 0 && mlsdr_ring_used(ring) < nelems) {
		if (timeout > 0) {
			ret = pthread_cond_timedwait(&ring->cond, &ring->mtx, &timespec);
		} else {
			ret = pthread_cond_wait(&ring->cond, &ring->mtx);
		}
	}

	if (ret == ETIMEDOUT) {
		// We _hold_ the mutex even though the call "failed"
		nelems = min(mlsdr_ring_used(ring), nelems);
	} else if (ret != 0) {
		// We _do not_ hold the mutex at this point
		// Also note that pthread returns _positive_ error codes
		return ret < 0 ? ret : -ret;
	}
	if (nelems == 0) {
		goto exit;
	}

	size_t fr = min(nelems, ring->nelems - ring->rptr);
	memcpy(els, ring->ptr + ring->rptr * ring->elsize, fr * ring->elsize);
	ring->rptr = (ring->rptr + fr) % ring->nelems;
	if (fr < nelems) {
		memcpy(els + fr * ring->elsize, ring->ptr, (nelems - fr) * ring->elsize);
		ring->rptr += nelems - fr;
	}

exit:
	pthread_mutex_unlock(&ring->mtx);

	return nelems;
}

void mlsdr_ring_flush(struct mlsdr_ring *ring)
{
	pthread_mutex_lock(&ring->mtx);

	ring->rptr = 0;
	ring->wptr = 0;

	pthread_mutex_unlock(&ring->mtx);
}

void mlsdr_ring_destroy(struct mlsdr_ring *ring)
{
	// Another thread waiting on the mutex/condvar => undefined behavior
	// User has to make sure that this does not happen
	pthread_cond_destroy(&ring->cond);
	pthread_mutex_destroy(&ring->mtx);
	free(ring->ptr);
	free(ring);
}
