#include <algorithm>
#include <bitset>
#include <cmath>
#include <cstdint>
#include <iostream>
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
  /*
    struct Block {
      std::array<uint64_t, LANES> w{};
    };
  */
public:
  explicit SBBF(size_t n, double p) {
    if (n == 0)
      n = 1;
    if (!(p > 0.0 && p < 1.0))
      p = 0.01;

    const double ln2 = std::log(2.0);

    const double m_bits_f = (-static_cast<double>(n) * log(p)) / (ln2 * ln2);
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
    filter_.assign(m_, 0ULL);
  }

  static uint64_t mix64(uint64_t x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4eb59ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
  }

  void insert(const std::string &key) {
    uint64_t h = static_cast<uint64_t>(std::hash<std::string>{}(key));
    uint64_t h1 = mix64(h);
    uint64_t h2 = mix64(h1) | 1ULL;

    size_t block_idx = h1 & (m_ - 1);
    // size_t block_idx = h1 % m_;

    uint64_t mask = 0;
    for (size_t i = 0; i < k_; i++) {
      uint64_t salt = i * 0x9e3779b97f4a7c15ULL;
      uint64_t bit_pos = mix64(h2 + salt) >> 58;
      mask |= (1ULL << bit_pos);
    }
    filter_[block_idx] |= mask;
  }

  bool possiblyContains(const std::string &key) {
    uint64_t h = static_cast<uint64_t>(std::hash<std::string>{}(key));
    uint64_t h1 = mix64(h);
    uint64_t h2 = mix64(h1) | 1ULL;

    size_t block_idx = h1 & (m_ - 1);
    // size_t block_idx = h1 % m_;

    uint64_t mask = 0;
    for (size_t i = 0; i < k_; i++) {
      uint64_t salt = i * 0x9e3779b97f4a7c15ULL;
      uint64_t bit_pos = mix64(h2 + salt) >> 58;
      mask |= (1ULL << bit_pos);
    }
    if ((filter_[block_idx] & mask) == mask)
      return true;
    return false;
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
  std::vector<uint64_t> filter_;
};

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
