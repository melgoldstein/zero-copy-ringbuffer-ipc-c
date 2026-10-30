#include "spsc_ring.h"
#include <string.h>

size_t spsc_bytes(uint32_t capacity, uint32_t slot_size) {
    if (!capacity || (capacity & (capacity-1)) || capacity > (1u<<30) ||
        !slot_size || slot_size % 8 ||
        capacity > (SIZE_MAX-sizeof(SpscRing))/slot_size) return 0;
    return sizeof(SpscRing)+(size_t)capacity*slot_size;
}
int spsc_init(void *storage, size_t bytes, uint32_t capacity, uint32_t slot_size) {
    size_t required=spsc_bytes(capacity,slot_size);
    if (!storage || (uintptr_t)storage % 64 || !required || bytes<required ||
        !__atomic_always_lock_free(sizeof(uint32_t), 0)) return 0;
    memset(storage,0,required);
    SpscRing *ring=storage;
    ring->capacity=capacity;
    ring->slot_size=slot_size;
    return 1;
}
void *spsc_reserve(SpscRing *ring) {
    uint32_t head=__atomic_load_n(&ring->producer,__ATOMIC_RELAXED);
    uint32_t tail=__atomic_load_n(&ring->consumer,__ATOMIC_ACQUIRE);
    if ((uint32_t)(head-tail)>=ring->capacity) return NULL;
    return ring->slots+(size_t)(head & (ring->capacity-1))*ring->slot_size;
}
void spsc_commit(SpscRing *ring) {
    uint32_t head=__atomic_load_n(&ring->producer,__ATOMIC_RELAXED);
    __atomic_store_n(&ring->producer,head+1,__ATOMIC_RELEASE);
}
const void *spsc_peek(SpscRing *ring) {
    uint32_t tail=__atomic_load_n(&ring->consumer,__ATOMIC_RELAXED);
    uint32_t head=__atomic_load_n(&ring->producer,__ATOMIC_ACQUIRE);
    if (tail==head) return NULL;
    return ring->slots+(size_t)(tail & (ring->capacity-1))*ring->slot_size;
}
void spsc_release(SpscRing *ring) {
    uint32_t tail=__atomic_load_n(&ring->consumer,__ATOMIC_RELAXED);
    __atomic_store_n(&ring->consumer,tail+1,__ATOMIC_RELEASE);
}
