#define _GNU_SOURCE
#include "spsc_ring.h"
#include <assert.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
int main(void) {
    alarm(20);
    size_t bytes=spsc_bytes(1024,64);
    SpscRing *ring=mmap(NULL,bytes,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
    assert(ring!=MAP_FAILED && spsc_init(ring,bytes,1024,64));
    struct timespec start, finish;
    assert(clock_gettime(CLOCK_MONOTONIC,&start)==0);
    pid_t child=fork();
    assert(child>=0);
    if (!child) {
        for (uint64_t sequence=0;sequence<1000000;++sequence) {
            const uint64_t *slot;
            while (!(slot=spsc_peek(ring))) {}
            if (*slot!=sequence || slot[7]!=~sequence) _exit(2);
            spsc_release(ring);
        }
        _exit(0);
    }
    for (uint64_t sequence=0;sequence<1000000;++sequence) {
        uint64_t *slot;
        while (!(slot=spsc_reserve(ring))) {}
        *slot=sequence; slot[7]=~sequence; spsc_commit(ring);
    }
    int status;
    assert(waitpid(child,&status,0)==child);
    assert(WIFEXITED(status) && WEXITSTATUS(status)==0);
    assert(clock_gettime(CLOCK_MONOTONIC,&finish)==0);
    double elapsed=finish.tv_sec-start.tv_sec+(finish.tv_nsec-start.tv_nsec)/1e9;
    printf("IPC: %.0f messages/s, %.6f us/message, including fork and wait\n",
           1000000/elapsed,elapsed);
    assert(munmap(ring,bytes)==0);
    return 0;
}
