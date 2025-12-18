#include "sbbf.hpp"
#include <iostream>
#include <string>

int main() {
  sbbf::SplitBlockBloomFilter bf(/*expected_keys=*/1'000'000,
                                 /*bits_per_key=*/10.0, /*k_bits=*/8);

  uint64_t x = 123456789;
  bf.insert(x);

  std::cout << bf.contains(x) << "\n";           // likely 1
  std::cout << bf.contains(uint64_t(7)) << "\n"; // likely 0
}
