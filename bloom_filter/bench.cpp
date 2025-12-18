#include "sbbf.h"
#include <benchmark/benchmark.h>
#include <string>
#include <vector>

std::string get_random_string(size_t len) {
  static const char alphanum[] =
      "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  std::string s = "";
  for (size_t i = 0; i < len; ++i)
    s += alphanum[rand() % (sizeof(alphanum) - 1)];
  return s;
}

class SBBF_Fixture : public benchmark::Fixture {
public:
  SBBF<std::string> *filter;
  std::vector<std::string> keys;

  void SetUp(const ::benchmark::State &state) {
    // Create a filter with size based on benchmark arguments
    filter = new SBBF<std::string>(static_cast<double>(state.range(0)), 0.01);
    for (int i = 0; i < 1024; ++i)
      keys.push_back(get_random_string(10));
  }

  void TearDown(const ::benchmark::State &state) { delete filter; }
};

// Benchmark Query Performance
BENCHMARK_DEFINE_F(SBBF_Fixture, BM_Query)(benchmark::State &state) {
  size_t i = 0;
  for (auto _ : state) {
    // Cycle through our pre-generated keys
    bool result = filter->possiblyContains(keys[i++ & 1023]);
    benchmark::DoNotOptimize(result);
  }
  // Track how many items we processed per second
  state.SetItemsProcessed(state.iterations());
}

// Run for different filter sizes (8k to 1M bits)
BENCHMARK_REGISTER_F(SBBF_Fixture, BM_Query)
    ->RangeMultiplier(8)
    ->Range(1 << 13, 1 << 30)
    ->Complexity();
BENCHMARK_MAIN();
