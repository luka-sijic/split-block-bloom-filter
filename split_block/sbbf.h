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
  explicit SBBF(int n, double p) {
    size_t total_bits =
        static_cast<size_t>(-n * std::log(p) / (std::log(2) * std::log(2)));
    m_ = (total_bits + 63) / 64;
    k_ = 8;
    filter_.resize(m_);
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

    size_t block_idx = h1 % m_;

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

    size_t block_idx = h1 % m_;

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
  size_t m_;
  size_t k_;
  std::vector<uint64_t> filter_;
};

/*
int main() {
  auto sbf = SBBF(10000, .001);
  sbf.insert("hello");
  sbf.insert("god");
  sbf.insert("gg");
  std::cout << sbf.possiblyContains("hello") << std::endl;
  std::cout << sbf.possiblyContains("LMFAO") << std::endl;

  double golden_ratio = (1.0 + std::sqrt(5.0)) / 2.0;
  // long double ans = (1 << 64) / golden_ratio;
  return 0;
}
*/
