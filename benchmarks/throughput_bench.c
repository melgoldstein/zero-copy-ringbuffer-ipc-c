#include "spsc_ring.h"
#include <stdio.h>
#include <time.h>
#include <stdint.h>
static unsigned char storage[192+1024*64] __attribute__((aligned(64)));
int main(void) {
    const uint64_t iterations=200000000;
    if (!spsc_init(storage,sizeof storage,1024,64)) return 1;
    SpscRing *ring=(SpscRing *)storage;
    clock_t start=clock();
    uint64_t checksum=0;
    for (uint64_t index=0;index<iterations;++index) {
        uint64_t *slot=spsc_reserve(ring);
        if (!slot) return 1;
        *slot=index; spsc_commit(ring);
        const uint64_t *received=spsc_peek(ring);
        if (!received || *received!=index) return 1;
        checksum+=*received; spsc_release(ring);
    }
    double elapsed=(double)(clock()-start)/CLOCKS_PER_SEC;
    if (elapsed<=0 || checksum!=iterations*(iterations-1)/2) return 1;
    printf("messages,seconds,messages_per_second,roundtrip_us\n%llu,%.6f,%.0f,%.6f\n",
        (unsigned long long)iterations,elapsed,iterations/elapsed,elapsed*1e6/iterations);
    return 0;
}
