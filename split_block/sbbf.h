#include <algorithm>
#include <bitset>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <vector>

// SIMD backends
#if defined(__AVX2__)
#include <immintrin.h>
#define SBBF_HAS_AVX2 1
#elif defined(__SSE2__)
#include <emmintrin.h>
#define SBBF_HAS_SSE2 1
#elif defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__)
#include <arm_neon.h>
#define SBBF_HAS_NEON 1
#endif

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
  // 4x uint64_t = 256 bits; AVX2-friendly on x86, still fine on ARM.
  struct alignas(32) Block {
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

    size_t blocks = (m_bits + (LANES * 64 - 1)) / (LANES * 64);
    if (blocks == 0)
      blocks = 1;

    m_ = round_up_pow2(blocks);

    const double k_f =
        (static_cast<double>(m_ * LANES * 64) / static_cast<double>(n)) * ln2;

    size_t kk = static_cast<size_t>(std::lround(k_f));
    if (kk < 1)
      kk = 1;
    const size_t kmax = LANES * 64;
    if (kk > kmax)
      kk = kmax;
    k_ = kk;
    // Allocate filter
    filter_.assign(m_, Block{{0, 0, 0, 0}});
  }

  static inline bool simd_block_contains(const Block &a,
                                         const Block &b) noexcept {
#if defined(__AVX2__)
    // Check (a & b) == b for the full 256-bit block.
    __m256i va = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(a.w));
    __m256i vm = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(b.w));
    __m256i vd = _mm256_xor_si256(_mm256_and_si256(va, vm), vm);
    return _mm256_testz_si256(vd, vd) != 0;
#elif defined(__SSE2__)
    // SSE2 fallback (2x 128-bit halves).
    const __m128i zero = _mm_setzero_si128();

    __m128i a01 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&a.w[0]));
    __m128i m01 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&b.w[0]));
    __m128i d01 = _mm_xor_si128(_mm_and_si128(a01, m01), m01);

    __m128i a23 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&a.w[2]));
    __m128i m23 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&b.w[2]));
    __m128i d23 = _mm_xor_si128(_mm_and_si128(a23, m23), m23);

    __m128i d = _mm_or_si128(d01, d23);
    __m128i eq = _mm_cmpeq_epi8(d, zero);
    return _mm_movemask_epi8(eq) == 0xFFFF;
#elif defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__)
    // NEON path (2x 128-bit halves).
    uint64x2_t b01 = vld1q_u64(&a.w[0]);
    uint64x2_t m01 = vld1q_u64(&b.w[0]);
    uint64x2_t b23 = vld1q_u64(&a.w[2]);
    uint64x2_t m23 = vld1q_u64(&b.w[2]);

    uint64x2_t d01 = veorq_u64(vandq_u64(b01, m01), m01);
    uint64x2_t d23 = veorq_u64(vandq_u64(b23, m23), m23);

    uint64x2_t or01 = vorrq_u64(d01, d23);
    uint64_t x = vgetq_lane_u64(or01, 0) | vgetq_lane_u64(or01, 1);
    return x == 0;
#else
    // Scalar fallback.
    return ((a.w[0] & b.w[0]) == b.w[0]) && ((a.w[1] & b.w[1]) == b.w[1]) &&
           ((a.w[2] & b.w[2]) == b.w[2]) && ((a.w[3] & b.w[3]) == b.w[3]);
#endif
  }

  static inline void simd_block_or(Block &a, const Block &b) noexcept {
#if defined(__AVX2__)
    __m256i va = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(a.w));
    __m256i vm = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(b.w));
    va = _mm256_or_si256(va, vm);
    _mm256_storeu_si256(reinterpret_cast<__m256i *>(a.w), va);
#elif defined(__SSE2__)
    __m128i a01 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&a.w[0]));
    __m128i m01 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&b.w[0]));
    __m128i a23 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&a.w[2]));
    __m128i m23 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&b.w[2]));

    a01 = _mm_or_si128(a01, m01);
    a23 = _mm_or_si128(a23, m23);

    _mm_storeu_si128(reinterpret_cast<__m128i *>(&a.w[0]), a01);
    _mm_storeu_si128(reinterpret_cast<__m128i *>(&a.w[2]), a23);
#elif defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__)
    uint64x2_t b01 = vld1q_u64(&a.w[0]);
    uint64x2_t m01 = vld1q_u64(&b.w[0]);
    uint64x2_t b23 = vld1q_u64(&a.w[2]);
    uint64x2_t m23 = vld1q_u64(&b.w[2]);

    b01 = vorrq_u64(b01, m01);
    b23 = vorrq_u64(b23, m23);

    vst1q_u64(&a.w[0], b01);
    vst1q_u64(&a.w[2], b23);
#else
    a.w[0] |= b.w[0];
    a.w[1] |= b.w[1];
    a.w[2] |= b.w[2];
    a.w[3] |= b.w[3];
#endif
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

  void insert(const std::string_view &key) noexcept {
    uint64_t h = hash64(key);
    uint64_t h1 = mix64(h);
    uint64_t h2 = mix64(h1) | 1ULL;

    const size_t idx = static_cast<size_t>(h1) & (m_ - 1);

    Block mask = make_mask_block(h1, h2);
    simd_block_or(filter_[idx], mask);
  }

  bool possiblyContains(const std::string_view &key) const noexcept {
    uint64_t h = hash64(key);
    uint64_t h1 = mix64(h);
    uint64_t h2 = mix64(h1) | 1ULL;

    const size_t idx = static_cast<size_t>(h1) & (m_ - 1);

    Block mask = make_mask_block(h1, h2);
    return simd_block_contains(filter_[idx], mask);
  }

private:
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
