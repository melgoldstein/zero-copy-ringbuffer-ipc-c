#include "spsc_ring.h"
#include <assert.h>
#include <stdint.h>
static unsigned char storage[4096] __attribute__((aligned(64)));
int main(void) {
    assert(!spsc_init(storage+1,sizeof storage-1,4,8));
    assert(!spsc_bytes(3,8));
    assert(!spsc_bytes(4,3));
    assert(!spsc_init(storage,2,4,8));
    assert(spsc_init(storage,sizeof storage,4,8));
    SpscRing *ring=(SpscRing *)storage;
    assert(!spsc_peek(ring));
    ring->producer=ring->consumer=UINT32_MAX-2;
    for (uint64_t value=0; value<4; ++value) {
        uint64_t *slot=spsc_reserve(ring);
        assert(slot); *slot=value; spsc_commit(ring);
    }
    assert(!spsc_reserve(ring));
    for (uint64_t value=0; value<4; ++value) {
        const uint64_t *slot=spsc_peek(ring);
        assert(slot && *slot==value); spsc_release(ring);
    }
    assert(!spsc_peek(ring));
    return 0;
}
