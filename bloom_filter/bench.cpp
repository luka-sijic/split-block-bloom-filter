#include "sbbf.h"
#include <benchmark/benchmark.h>

#include <cstdint>
#include <vector>

// xorshift64* PRNG
static inline uint64_t xorshift64star(uint64_t &x) {
  x ^= x >> 12;
  x ^= x << 25;
  x ^= x >> 27;
  return x * 2685821657736338717ULL;
}

static void shuffle_indices(std::vector<uint32_t> &idx, uint64_t seed) {
  for (size_t i = idx.size(); i > 1; --i) {
    const size_t j = static_cast<size_t>(xorshift64star(seed)) % i;
    std::swap(idx[i - 1], idx[j]);
  }
}

class BF_Fixture : public benchmark::Fixture {
public:
  SBBF<uint64_t> *filter{nullptr};
  std::vector<uint64_t> hit_keys;
  std::vector<uint64_t> miss_keys;
  std::vector<uint32_t> order;

  void SetUp(const ::benchmark::State &state) override {
    const size_t n = static_cast<size_t>(state.range(0));

    filter = new SBBF<uint64_t>(static_cast<double>(n), 0.01);

    hit_keys.resize(n);
    miss_keys.resize(n);
    order.resize(n);

    uint64_t s1 = 0x123456789abcdef0ULL;
    uint64_t s2 = 0xfedcba9876543210ULL;

    for (size_t i = 0; i < n; ++i) {
      hit_keys[i] = xorshift64star(s1);
      miss_keys[i] = xorshift64star(s2);
      order[i] = static_cast<uint32_t>(i);
    }

    shuffle_indices(order, 0xdeadbeefcafebabeULL);

    for (const uint64_t k : hit_keys) {
      filter->insert(k);
    }
  }

  void TearDown(const ::benchmark::State &) override { delete filter; }
};

BENCHMARK_DEFINE_F(BF_Fixture, BM_QueryHit)(benchmark::State &state) {
  constexpr int BATCH = 8;

  size_t pos = 0;
  uint32_t sink = 0;

  for (auto _ : state) {
    for (int j = 0; j < BATCH; ++j) {
      const uint64_t k = hit_keys[order[pos++]];
      if (pos == order.size())
        pos = 0;
      sink += static_cast<uint32_t>(filter->possiblyContains(k));
    }
    benchmark::DoNotOptimize(sink);
  }

  state.SetItemsProcessed(state.iterations() * BATCH);
}

BENCHMARK_DEFINE_F(BF_Fixture, BM_QueryMiss)(benchmark::State &state) {
  constexpr int BATCH = 8;

  size_t pos = 0;
  uint32_t sink = 0;

  for (auto _ : state) {
    for (int j = 0; j < BATCH; ++j) {
      const uint64_t k = miss_keys[order[pos++]];
      if (pos == order.size())
        pos = 0;
      sink += static_cast<uint32_t>(filter->possiblyContains(k));
    }
    benchmark::DoNotOptimize(sink);
  }

  state.SetItemsProcessed(state.iterations() * BATCH);
}

BENCHMARK_REGISTER_F(BF_Fixture, BM_QueryHit)
    ->RangeMultiplier(2)
    // 2^25 with p=0.01 is ~40MB of bits; exceeds 5800X L3 (32MB).
    ->Range(1LL << 22, 1LL << 25)
    ->Unit(benchmark::kNanosecond);

BENCHMARK_REGISTER_F(BF_Fixture, BM_QueryMiss)
    ->RangeMultiplier(2)
    ->Range(1LL << 22, 1LL << 25)
    ->Unit(benchmark::kNanosecond);

BENCHMARK_MAIN();
