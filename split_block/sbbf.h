#include <algorithm>
#include <arm_neon.h>
#include <bitset>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <vector>

/*
Filter hashes "lmao" and "god" into the same block
filter - 1 0 1 0 1 0 1 0 1
lmao -   1 0 0 0 0 0 1 0 1
AND      1 0 0 0 0 0 1 0 1

OR
AND
XOR
*/

class SBBF {
  static constexpr size_t LANES = 4;
  struct alignas(16) Block {
    uint64_t w[LANES];
  };

public:
  explicit SBBF(size_t n, double p) {
    if (n == 0)
      n = 1;
    if (!(p > 0.0 && p < 1.0))
      p = 0.01;

    const double ln2 = std::log(2.0);

    const double m_bits_f =
        (-static_cast<double>(n) * std::log(p)) / (ln2 * ln2);
    size_t m_bits = static_cast<size_t>(ceil(m_bits_f));
    if (m_bits == 0)
      m_bits = 64;

    size_t blocks = (m_bits + 63) / 64;
    if (blocks == 0)
      blocks = 1;

    m_ = round_up_pow2(blocks);

    const double k_f =
        (static_cast<double>(m_ * 64) / static_cast<double>(n)) * ln2;
    k_ = std::clamp<std::size_t>(static_cast<std::size_t>(std::lround(k_f)),
                                 1ULL, 64ULL);

    // Allocate filter
    filter_.assign(m_, Block{{0, 0, 0, 0}});
  }

  static inline bool neon_block_contains(const Block &a,
                                         const Block &b) noexcept {
    uint64x2_t b01 = vld1q_u64(&a.w[0]);
    uint64x2_t m01 = vld1q_u64(&b.w[0]);
    uint64x2_t b23 = vld1q_u64(&a.w[2]);
    uint64x2_t m23 = vld1q_u64(&b.w[2]);

    uint64x2_t d01 = veorq_u64(vandq_u64(b01, m01), m01);
    uint64x2_t d23 = veorq_u64(vandq_u64(b23, m23), m23);

    uint64x2_t or01 = vorrq_u64(d01, d23);
    uint64_t x = vgetq_lane_u64(or01, 0) | vgetq_lane_u64(or01, 1);
    return x == 0;
  }

  static inline void neon_block_or(Block &a, const Block &b) noexcept {
    uint64x2_t b01 = vld1q_u64(&a.w[0]);
    uint64x2_t m01 = vld1q_u64(&b.w[0]);
    uint64x2_t b23 = vld1q_u64(&a.w[2]);
    uint64x2_t m23 = vld1q_u64(&b.w[2]);

    b01 = vorrq_u64(b01, m01);
    b23 = vorrq_u64(b23, m23);

    vst1q_u64(&a.w[0], b01);
    vst1q_u64(&a.w[2], b23);
  }

  static uint64_t mix64(uint64_t x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4eb59ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
  }

  static inline uint64_t hash64(std::string_view s) noexcept {
    uint64_t h = 14695981039346656037ULL;
    for (unsigned char c : s) {
      h ^= static_cast<uint64_t>(c);
      h *= 1099511628211ULL;
    }
    return h;
  }

  void insert(const std::string_view &key) {
    uint64_t h = hash64(key);
    uint64_t h1 = mix64(h);
    uint64_t h2 = mix64(h1) | 1ULL;

    const size_t idx = static_cast<size_t>(h1) & (m_ - 1);

    Block mask = make_mask_block(h1, h2);
    neon_block_or(filter_[idx], mask);
  }

  bool possiblyContains(const std::string_view &key) {
    uint64_t h = hash64(key);
    uint64_t h1 = mix64(h);
    uint64_t h2 = mix64(h1) | 1ULL;

    const size_t idx = static_cast<size_t>(h1) & (m_ - 1);

    Block mask = make_mask_block(h1, h2);
    return neon_block_contains(filter_[idx], mask);
  }

  Block make_mask_block(uint64_t h1, uint64_t h2) const noexcept {
    Block m{{0, 0, 0, 0}};
    uint64_t x = h1;

    for (size_t i = 0; i < k_; ++i) {
      x += h2;
      const uint32_t bit = static_cast<uint32_t>(x) & 63u;
      const uint32_t lane = static_cast<uint32_t>(x >> 6) & 3u;
      m.w[lane] |= (1ULL << bit);
    }
    return m;
  }

private:
  static size_t round_up_pow2(size_t x) noexcept {
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    if constexpr (sizeof(size_t) == 8)
      x |= x >> 32;
    return x + 1;
  }
  size_t m_;
  size_t k_;
  std::vector<Block> filter_;
};

/*
int main() {
  auto sbf = SBBF(10000, .001);
  sbf.insert("hello");
  sbf.insert("username");
  sbf.insert("application");
  std::cout << sbf.possiblyContains("hello") << std::endl;
  std::cout << sbf.possiblyContains("app") << std::endl;

  double golden_ratio = (1.0 + std::sqrt(5.0)) / 2.0;
  // long double ans = (1 << 64) / golden_ratio;
  return 0;
}
*/
