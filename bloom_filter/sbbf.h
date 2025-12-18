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

// keep your mix64 as-is (placeholder here)
static inline std::uint64_t mix64(std::uint64_t x) {
  x ^= x >> 33;
  x *= 0xff51afd7ed558ccdULL;
  x ^= x >> 33;
  x *= 0xc4ceb9fe1a85ec53ULL;
  x ^= x >> 33;
  return x;
}

template <typename T> class SBBF {
public:
  SBBF(double n, double p) {
    // classic bloom sizing (keep your formulas)
    m_ = static_cast<std::size_t>(
        std::ceil(-(n * std::log(p)) / (std::log(2) * std::log(2))));
    k_ = static_cast<std::size_t>(
        std::ceil((static_cast<double>(m_) / n) * std::log(2)));

    // classic bit-array backing storage
    bits_.assign((m_ + 63) / 64, 0ULL);
  }

  void insert(const T &key) {
    const std::uint64_t h = static_cast<std::uint64_t>(std::hash<T>{}(key));
    const std::uint64_t h1 = mix64(h);
    const std::uint64_t h2 = (mix64(h1) | 1ULL); // odd step for double-hashing

    for (std::size_t i = 0; i < k_; i++) {
      const std::uint64_t salt = i * 0x9e3779b97f4a7c15ULL;
      const std::size_t bit = static_cast<std::size_t>(mix64(h2 + salt) % m_);
      bits_[bit >> 6] |= (1ULL << (bit & 63));
    }
  }

  bool possiblyContains(const T &key) const {
    const std::uint64_t h = static_cast<std::uint64_t>(std::hash<T>{}(key));
    const std::uint64_t h1 = mix64(h);
    const std::uint64_t h2 = (mix64(h1) | 1ULL);

    for (std::size_t i = 0; i < k_; i++) {
      const std::uint64_t salt = i * 0x9e3779b97f4a7c15ULL;
      const std::size_t bit = static_cast<std::size_t>(mix64(h2 + salt) % m_);
      if ((bits_[bit >> 6] & (1ULL << (bit & 63))) == 0ULL) {
        return false;
      }
    }
    return true;
  }

  std::size_t m_bits() const { return m_; }
  std::size_t k_hashes() const { return k_; }

private:
  std::size_t k_{0};
  std::size_t m_{0};                // number of bits
  std::vector<std::uint64_t> bits_; // packed bits
};
