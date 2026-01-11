#include "sbbf.h"
#include <benchmark/benchmark.h>
#include <cstdint>
#include <string>
#include <vector>

// Fast PRNG (xorshift64*)
static inline uint64_t xorshift64star(uint64_t &x) {
  x ^= x >> 12;
  x ^= x << 25;
  x ^= x >> 27;
  return x * 2685821657736338717ULL;
}

// Deterministic "random" string generator without rand().
// Keep len <= typical SSO threshold to avoid heap churn per string object.
static std::string make_key(uint64_t v, size_t len = 16) {
  static constexpr char alphanum[] =
      "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  static constexpr size_t A = sizeof(alphanum) - 1;

  std::string s;
  s.resize(len);
  for (size_t i = 0; i < len; ++i) {
    // mix bits a little and map to charset
    v = (v ^ (v >> 33)) * 0xff51afd7ed558ccdULL;
    s[i] = alphanum[static_cast<size_t>(v % A)];
  }
  return s;
}

class SBBF_Fixture : public benchmark::Fixture {
public:
  SBBF<std::string> *filter;
  std::vector<std::string> keys;

  void SetUp(const ::benchmark::State &state) override {
    // IMPORTANT: state.range(0) is interpreted as expected inserts "n"
    // For p=0.01, bits ~= 9.585 * n, bytes ~= 1.198 * n.
    // So n >= ~107M => >= ~128MB filter. We range from 2^27 (134M) upward.
    filter = new SBBF<std::string>(static_cast<double>(state.range(0)), 0.01);

    // Key diversity: choose a big power-of-two count so masking is valid.
    // 2^20 keys ~ 1,048,576 strings. With SSO (len=16), this is typically tens
    // of MB.
    constexpr size_t KEY_COUNT = 1u << 20;
    keys.clear();
    keys.reserve(KEY_COUNT);

    uint64_t seed = 0x123456789abcdef0ULL;
    for (size_t i = 0; i < KEY_COUNT; ++i) {
      uint64_t v = xorshift64star(seed);
      keys.push_back(make_key(v, 16));
    }

    // Optional but recommended: insert keys so membership queries have
    // realistic hit behavior. (Also removes “mostly-false” branch bias.)
    for (const auto &k : keys) {
      filter->insert(k);
    }
  }

  void TearDown(const ::benchmark::State &) override { delete filter; }
};

// Benchmark Query Performance (randomized access; no sequential reuse pattern)
BENCHMARK_DEFINE_F(SBBF_Fixture, BM_Query)(benchmark::State &state) {
  // Per-benchmark PRNG state
  uint64_t rng = 0xdeadbeefcafebabeULL ^ static_cast<uint64_t>(state.range(0));
  const size_t mask = keys.size() - 1; // keys.size() is power-of-two

  for (auto _ : state) {
    // Randomize queries: no locality / no predictable stepping
    size_t idx = static_cast<size_t>(xorshift64star(rng)) & mask;

    bool result = filter->possiblyContains(keys[idx]);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(SBBF_Fixture, BM_Query)
    ->RangeMultiplier(2)
    // Blow past LLC: start at 2^27 expected inserts (~160MB+ at p=0.01)
    ->Range(1LL << 27, 1LL << 30)
    ->Complexity();

BENCHMARK_MAIN();
