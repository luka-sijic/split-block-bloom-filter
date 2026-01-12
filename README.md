# split-block-bloom-filter

Two C++ implementations of a Bloom filter, focused on **high-throughput membership queries** and **benchmark-driven performance work**.

This repo contains:
- `bloom_filter/`: a “classic” Bloom filter backed by a packed bit-array.
- `split_block/`: a split-block Bloom filter variant designed for better locality and easier SIMD acceleration.

## What this is
A Bloom filter is a probabilistic set membership data structure:
- `insert(key)`: sets a small number of bits derived from the key’s hash.
- `possiblyContains(key)`: returns **false** if definitely not present, or **true** if “maybe” present.
- There are **no false negatives** (assuming correct implementation), but there are **false positives**.

The split-block approach groups bits into fixed-size blocks; queries touch a smaller, more predictable memory region per key. That tends to improve cache behavior and makes it natural to use SIMD to combine multiple word operations.

## SIMD support (split-block)
`split_block/sbbf.h` includes an accelerated block check/or routine with compile-time selection:
- x86: AVX2 (fast path), SSE2 (fallback)
- ARM: NEON
- Scalar fallback if no SIMD backend is available

## Build + run benchmarks
Dependencies:
- CMake 3.10+
- C++17 compiler
- Google Benchmark (`libbenchmark` + headers)

### Linux (example)
Install Google Benchmark (package names vary):
- Debian/Ubuntu: `sudo apt-get install libbenchmark-dev`
- Arch: `sudo pacman -S benchmark`

Build and run classic Bloom filter benchmark:
- `cmake -S bloom_filter -B build_bloom_filter -DCMAKE_BUILD_TYPE=Release`
- `cmake --build build_bloom_filter -j`
- `./build_bloom_filter/run_bench`

Build and run split-block benchmark:
- `cmake -S split_block -B build_split_block -DCMAKE_BUILD_TYPE=Release`
- `cmake --build build_split_block -j`
- `./build_split_block/run_bench`

### Notes on benchmark stability
- Disable CPU frequency scaling / set performance governor for cleaner numbers.
- Pin the process to a core and reduce background noise if you’re comparing small deltas.
- These benchmarks are primarily measuring **query throughput** under large working sets (intended to exceed LLC).

