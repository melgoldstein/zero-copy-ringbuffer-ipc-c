# Zero-copy SPSC ring buffer

Fixed-capacity queue for one producer and one consumer in shared memory. Slots
are accessed in place. The queue stores offsets implicitly, so mappings may live
at different virtual addresses. No heap allocation occurs inside the library.

```mermaid
flowchart LR
  P[Producer reserves slot] --> W[Write payload]
  W --> C[Release commit]
  C --> R[Acquire peek]
  R --> S[Consumer reads slot]
  S --> F[Release slot]
```

## Quickstart (30 Seconds)

```sh
make all test
./throughput_bench
make ipc   # Linux: forked producer and consumer, one million checked messages
```

Allocate `spsc_bytes(capacity, slot_size)` bytes at a 64-byte-aligned address and
initialize once before publishing the mapping. Capacity must be a power of two;
slot size must be a multiple of eight. The returned slot has eight-byte alignment.
Call commit only after a successful reserve; call release only after a successful
peek. Exactly one outstanding reservation per side is supported. Full/empty
returns NULL immediately; waiting/backpressure policy belongs to the caller.

The producer and consumer counters occupy separate 64-byte cache lines. GCC/Clang
32-bit atomic intrinsics must be always lock-free on the target. Acquire/release
orders payload access; unsigned arithmetic handles counter wrap. This is C99 with
compiler extensions, tested on Linux x86-64. It is not an MPMC queue. Process death
does not recover an outstanding slot. Initialize and destroy only while quiescent.

## Measurements

See `benchmarks/results.csv` for the measured 200-million-message local loop.
This loop measures reserve/write/commit/peek/read/release on one core, not transport
latency across cores. Only eight payload bytes are written in a 64-byte slot.

| Test | Workload | Guarantee checked |
|---|---|---|
| Local benchmark | 200 million messages | Payload sequence and checksum |
| Linux IPC | 1 million messages | Ordering and payload visibility across processes |
| Unit tests | Boundary cases | Full, empty, alignment and uint32 wraparound |

| Local measurement | Value |
|---|---:|
| Messages | 200,000,000 |
| Elapsed process time | 0.768 seconds |
| Throughput | 260.4 million messages/second |
| Roundtrip | 0.00384 microseconds/message |

Windows x86-64, GCC -O3, single-thread queue loop. Cross-process throughput depends
on placement and scheduling; `make ipc` prints its own wall-clock result on Linux.

| Linux cross-process measurement | Throughput | Microseconds/message |
|---|---:|---:|
| GCC | 22.16 million/s | 0.045117 |
| Clang | 20.42 million/s | 0.048976 |

[CI run 40970677663](https://github.com/cankaan120/zero-copy-ringbuffer-ipc-c/actions/runs/40970677663)
checked one million ordered messages with two payload words per slot. Wall time
includes fork and wait; CPU placement was not pinned. These numbers are a runner
measurement, not a throughput guarantee on other hardware.

MIT. The caller owns storage lifetime and is responsible for payload schema/version.

## Producer and consumer API

```c
/* Producer: one successful reservation followed by one commit. */
uint64_t *outgoing = spsc_reserve(ring);
if (outgoing) {
    outgoing[0] = sequence;
    spsc_commit(ring);
}
/* Consumer: the pointer expires when this slot is released. */
const uint64_t *incoming = spsc_peek(ring);
if (incoming) {
    handle_sequence(incoming[0]);
    spsc_release(ring);
}
```

Never put process-local pointers in shared slots. Use fixed-width scalar fields or
offsets relative to a shared mapping. Do not retain a slot pointer after commit or
release: the other process can reuse it. The caller must arrange mapping ownership,
startup synchronization and shutdown. `tests/test_ipc.c` is a complete Linux example.
