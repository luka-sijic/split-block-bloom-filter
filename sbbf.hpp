// split_block_bloom.hpp
#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <vector>

#if defined(__GNUC__) || defined(__clang__)
#define SBBF_FORCEINLINE __attribute__((always_inline)) inline
#else
#define SBBF_FORCEINLINE inline
#endif

namespace sbbf {

// -------------------------------
// 64-bit mixing (SplitMix64)
// -------------------------------
static SBBF_FORCEINLINE uint64_t splitmix64(uint64_t x) {
  x += 0x9e3779b97f4a7c15ULL;
  x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
  x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
  return x ^ (x >> 31);
}

// Fast "reduce" to [0, n) without modulo bias using high-multiply.
// Returns floor((x * n) / 2^64).
static SBBF_FORCEINLINE uint32_t fast_range_u32(uint64_t x, uint32_t n) {
  return static_cast<uint32_t>((__uint128_t)x * n >> 64);
}

// -------------------------------
// Cache-line block (64 bytes)
// -------------------------------
struct alignas(64) Block {
  uint64_t w[8]; // 8 * 8 = 64 bytes = 512 bits
};

// -------------------------------
// Split-Block Bloom Filter
// -------------------------------
class SplitBlockBloomFilter {
public:
  // bits_per_key: controls size (approx). Typical: 8-16 for many workloads.
  // k_bits: number of bits set per key *within a block* (typical: 8-16).
  SplitBlockBloomFilter(uint64_t expected_keys, double bits_per_key = 10.0,
                        uint32_t k_bits = 8)
      : k_(k_bits) {
    if (expected_keys == 0)
      expected_keys = 1;
    if (bits_per_key <= 0.0)
      throw std::invalid_argument("bits_per_key must be > 0");
    if (k_ == 0 || k_ > 32)
      throw std::invalid_argument("k_bits must be in [1, 32]");

    // Total bits requested
    const long double total_bits_ld =
        (long double)expected_keys * (long double)bits_per_key;

    // Each block holds 512 bits. Round up blocks.
    uint64_t blocks = static_cast<uint64_t>((total_bits_ld + 511.0L) / 512.0L);
    if (blocks == 0)
      blocks = 1;

    // For cheap masking, you may choose power-of-two blocks. Keep general by
    // default. If you want strict power-of-two, uncomment: blocks =
    // round_up_pow2(blocks);

    num_blocks_ = static_cast<uint32_t>(blocks);
    allocate_blocks(num_blocks_);
  }

  ~SplitBlockBloomFilter() { deallocate_blocks(); }

  SplitBlockBloomFilter(const SplitBlockBloomFilter &) = delete;
  SplitBlockBloomFilter &operator=(const SplitBlockBloomFilter &) = delete;

  SplitBlockBloomFilter(SplitBlockBloomFilter &&other) noexcept {
    *this = std::move(other);
  }
  SplitBlockBloomFilter &operator=(SplitBlockBloomFilter &&other) noexcept {
    if (this == &other)
      return *this;
    deallocate_blocks();
    blocks_ = other.blocks_;
    num_blocks_ = other.num_blocks_;
    k_ = other.k_;
    other.blocks_ = nullptr;
    other.num_blocks_ = 0;
    other.k_ = 0;
    return *this;
  }

  void clear() {
    if (!blocks_)
      return;
    std::memset(blocks_, 0, sizeof(Block) * num_blocks_);
  }

  uint32_t num_blocks() const { return num_blocks_; }
  uint32_t k_bits() const { return k_; }
  uint64_t bytes() const { return uint64_t(num_blocks_) * sizeof(Block); }

  // Insert a 64-bit key hash. You can hash your keys externally (recommended).
  SBBF_FORCEINLINE void insert_hash(uint64_t h) {
    Block &b = blocks_[pick_block(h)];
    set_bits_in_block(b, h);
  }

  // Query using a 64-bit key hash.
  SBBF_FORCEINLINE bool contains_hash(uint64_t h) const {
    const Block &b = blocks_[pick_block(h)];
    return test_bits_in_block(b, h);
  }

  // Convenience: hash arbitrary trivially-copyable key by bytes (not
  // cryptographic).
  template <class T> SBBF_FORCEINLINE void insert(const T &key) {
    static_assert(std::is_trivially_copyable_v<T>,
                  "T must be trivially copyable");
    insert_hash(hash_bytes(&key, sizeof(T)));
  }

  template <class T> SBBF_FORCEINLINE bool contains(const T &key) const {
    static_assert(std::is_trivially_copyable_v<T>,
                  "T must be trivially copyable");
    return contains_hash(hash_bytes(&key, sizeof(T)));
  }

  // Hash bytes (simple) -> 64-bit, then mixed. Replace with xxHash/wyhash if
  // desired.
  static SBBF_FORCEINLINE uint64_t hash_bytes(const void *data, size_t len) {
    const uint8_t *p = static_cast<const uint8_t *>(data);
    // FNV-1a 64-bit
    uint64_t h = 1469598103934665603ULL;
    for (size_t i = 0; i < len; i++) {
      h ^= (uint64_t)p[i];
      h *= 1099511628211ULL;
    }
    return splitmix64(h);
  }

private:
  Block *blocks_ = nullptr;
  uint32_t num_blocks_ = 0;
  uint32_t k_ = 0;

  static uint64_t round_up_pow2(uint64_t x) {
    if (x <= 1)
      return 1;
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    return x + 1;
  }

  void allocate_blocks(uint32_t n) {
    // 64-byte alignment to match cache line
#if defined(_MSC_VER)
    blocks_ = static_cast<Block *>(_aligned_malloc(sizeof(Block) * n, 64));
    if (!blocks_)
      throw std::bad_alloc();
#else
    void *mem = nullptr;
    if (posix_memalign(&mem, 64, sizeof(Block) * n) != 0)
      throw std::bad_alloc();
    blocks_ = static_cast<Block *>(mem);
#endif
    std::memset(blocks_, 0, sizeof(Block) * n);
  }

  void deallocate_blocks() {
    if (!blocks_)
      return;
#if defined(_MSC_VER)
    _aligned_free(blocks_);
#else
    free(blocks_);
#endif
    blocks_ = nullptr;
  }

  SBBF_FORCEINLINE uint32_t pick_block(uint64_t h) const {
    // Use high-multiply range reduction to map to [0, num_blocks_)
    // This avoids slow modulo and is uniform.
    return fast_range_u32(splitmix64(h ^ 0xD6E8FEB86659FD93ULL), num_blocks_);
  }

  // Generate k bit positions in [0, 511] and set them.
  // Implementation: double-hashing: pos_i = (a + i*b) mod 512
  // We use 9-bit positions (0..511). Keep distribution decent by mixing.
  static SBBF_FORCEINLINE uint32_t bitpos9(uint64_t x) {
    // Take 9 bits after mixing
    return static_cast<uint32_t>(splitmix64(x) & 511ULL);
  }

  SBBF_FORCEINLINE void set_bits_in_block(Block &b, uint64_t h) {
    // Two streams for double-hashing
    const uint64_t a0 = splitmix64(h ^ 0xA5A5A5A5A5A5A5A5ULL);
    const uint64_t b0 =
        splitmix64(h ^ 0x0123456789ABCDEFULL) | 1ULL; // ensure odd step

    // Unrolled-ish loop: touches only this block
    for (uint32_t i = 0; i < k_; i++) {
      const uint64_t x = a0 + uint64_t(i) * b0;
      const uint32_t pos = static_cast<uint32_t>(x & 511ULL); // 0..511
      b.w[pos >> 6] |= (1ULL << (pos & 63));
    }
  }

  SBBF_FORCEINLINE bool test_bits_in_block(const Block &b, uint64_t h) const {
    const uint64_t a0 = splitmix64(h ^ 0xA5A5A5A5A5A5A5A5ULL);
    const uint64_t b0 = splitmix64(h ^ 0x0123456789ABCDEFULL) | 1ULL;

    for (uint32_t i = 0; i < k_; i++) {
      const uint64_t x = a0 + uint64_t(i) * b0;
      const uint32_t pos = static_cast<uint32_t>(x & 511ULL);
      if ((b.w[pos >> 6] & (1ULL << (pos & 63))) == 0ULL)
        return false;
    }
    return true;
  }
};

} // namespace sbbf
