#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

// Keep a sink to prevent the optimizer from dropping work.
static volatile uint64_t global_sink = 0;

static uint64_t do_work(uint64_t value, uint64_t iterations) {
    uint64_t acc = value;
    for (uint64_t i = 0; i < iterations; ++i) {
        acc += (acc << 1) ^ (i | 0x9e3779b97f4a7c15ULL);
        acc ^= (acc >> 5) + 0x123456789ABCDEFULL;
    }
    return acc;
}

// Base cost: 100 units
__attribute__((noinline))
uint64_t func1(uint64_t input) {
    return do_work(input, 100);
}

// 50% more than func1: 150 units
__attribute__((noinline))
uint64_t func2(uint64_t input) {
    return do_work(input, 150);
}

// Called by func3 for about half of its extra work
__attribute__((noinline))
uint64_t func3a(uint64_t input) {
    return do_work(input ^ 0xdeadbeefcafebabeULL, 1);
}

// 50% more than func2: 225 units (150 from do_work + 75 from func3a)
__attribute__((noinline))
uint64_t func3(uint64_t input) {
    uint64_t acc = do_work(input, 150);

    for (uint64_t i = 0; i < 75; ++i) {
        acc += func3a(acc);
    }
    return acc;
}

int main(int argc, char *argv[]) {
    const uint64_t iterations = (argc > 1) ? strtoull(argv[1], NULL, 0) : 5000000ULL;
    uint64_t total = 0;

    printf("Running flamegraph workload (iterations=%" PRIu64 ")\n", iterations);

    for (uint64_t i = 0; i < iterations; ++i) {
        total += func1(i);
        total += func2(i);
        total += func3(i);

        if (i % (iterations / 10) == 0 && i > 0) {
            printf("  i = %" PRIu64 "\n", i);
        }
    }

    global_sink = total;
    printf("Done. sink=%" PRIu64 "\n", global_sink);
    return 0;
}
