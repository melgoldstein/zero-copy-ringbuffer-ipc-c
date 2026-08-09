#ifndef SPSC_RING_H
#define SPSC_RING_H
#include <stddef.h>
#include <stdint.h>
#if !defined(__GNUC__) && !defined(__clang__)
#error GCC or Clang atomic intrinsics required
#endif
typedef struct __attribute__((aligned(64))) {
    uint32_t producer;
    unsigned char producer_padding[60];
    uint32_t consumer;
    unsigned char consumer_padding[60];
    uint32_t capacity;
    uint32_t slot_size;
    unsigned char configuration_padding[56];
    unsigned char slots[];
} SpscRing;
size_t spsc_bytes(uint32_t capacity, uint32_t slot_size);
int spsc_init(void *storage, size_t bytes, uint32_t capacity, uint32_t slot_size);
void *spsc_reserve(SpscRing *ring);
void spsc_commit(SpscRing *ring);
const void *spsc_peek(SpscRing *ring);
void spsc_release(SpscRing *ring);
#endif
