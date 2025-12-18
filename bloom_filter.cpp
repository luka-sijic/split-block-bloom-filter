#include <bitset>
#include <cmath>
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

  struct Block {
    std::array<uint64_t, LANES> w{};
  };

public:
  explicit SBBF(int n, double p) {
    m_ = n * 4;
    k_ = 3;
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

    // size_t block_idx

    for (size_t i = 0; i < k_; i++) {
      uint64_t pos = (h1 + i * h2) % m_;
      filter_[pos >> 6] |= (1ULL << (pos & 63));
    }
  }

  bool possiblyContains(const std::string &key) {
    uint64_t h = static_cast<uint64_t>(std::hash<std::string>{}(key));
    uint64_t h1 = mix64(h);
    uint64_t h2 = mix64(h1) | 1ULL;

    for (size_t i = 0; i < k_; i++) {
      uint64_t pos = (h1 + i * h2) % m_;
      if ((filter_[pos >> 6] & (1ULL << (pos & 63))) == 0)
        return false;
    }
    return true;
  }

private:
  size_t m_;
  size_t k_;
  std::vector<uint64_t> filter_;
};

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
