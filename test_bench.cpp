#include <benchmark/benchmark.h>
#include <vector>

static void BM_VectorCreation(benchmark::State &state) {
  for (auto _ : state) {
    std::vector<int> v(1000);
    benchmark::DoNotOptimize(v);
  }
}
BENCHMARK(BM_VectorCreation);

BENCHMARK_MAIN();
