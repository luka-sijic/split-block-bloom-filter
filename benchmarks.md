Tested on (Ryzen 7 5800X, 32GB RAM, SSD)

Split-Block
````
2026-01-14T18:53:58-07:00
Running ./run_bench
Run on (16 X 4849.77 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 512 KiB (x8)
  L3 Unified 32768 KiB (x1)
Load Average: 0.55, 0.56, 0.54
---------------------------------------------------------------------------------------------
Benchmark                                   Time             CPU   Iterations UserCounters...
---------------------------------------------------------------------------------------------
SBBF_Fixture/BM_QueryHit/4194304          262 ns          261 ns      3141816 items_per_second=30.593M/s
SBBF_Fixture/BM_QueryHit/8388608          351 ns          350 ns      2023942 items_per_second=22.863M/s
SBBF_Fixture/BM_QueryHit/16777216         389 ns          388 ns      1800253 items_per_second=20.6209M/s
SBBF_Fixture/BM_QueryHit/33554432         405 ns          404 ns      1772438 items_per_second=19.8192M/s
SBBF_Fixture/BM_QueryMiss/4194304         232 ns          231 ns      3019178 items_per_second=34.6026M/s
SBBF_Fixture/BM_QueryMiss/8388608         323 ns          321 ns      2168530 items_per_second=24.8859M/s
SBBF_Fixture/BM_QueryMiss/16777216        376 ns          375 ns      1815832 items_per_second=21.3567M/s
SBBF_Fixture/BM_QueryMiss/33554432        409 ns          407 ns      1737222 items_per_second=19.6464M/s
````
perf stat -r 10 -e cycles,instructions,cache-misses,branch-misses \
        taskset -c 2 ./run_bench
        
 Performance counter stats for 'taskset -c 2 ./run_bench' (10 runs):

   182,175,356,378      cycles:u                                                                ( +-  0.14% )
   130,327,919,507      instructions:u                   #    0.72  insn per cycle              ( +-  0.06% )
     2,789,876,120      cache-misses:u                                                          ( +-  0.11% )
         1,225,652      branch-misses:u                                                         ( +-  0.18% )

           39.1876 +- 0.0534 seconds time elapsed  ( +-  0.14% )



Bloom Filter
```
Running ./run_bench
Run on (16 X 1754.31 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 512 KiB (x8)
  L3 Unified 32768 KiB (x1)
Load Average: 0.78, 0.65, 0.57
-------------------------------------------------------------------------------------------
Benchmark                                 Time             CPU   Iterations UserCounters...
-------------------------------------------------------------------------------------------
BF_Fixture/BM_QueryHit/4194304          558 ns          556 ns      1120576 items_per_second=14.3951M/s
BF_Fixture/BM_QueryHit/8388608          631 ns          629 ns      1110920 items_per_second=12.721M/s
BF_Fixture/BM_QueryHit/16777216         775 ns          773 ns       940369 items_per_second=10.3531M/s
BF_Fixture/BM_QueryHit/33554432         843 ns          840 ns       795998 items_per_second=9.52626M/s
BF_Fixture/BM_QueryMiss/4194304         294 ns          293 ns      2554673 items_per_second=27.2854M/s
BF_Fixture/BM_QueryMiss/8388608         351 ns          349 ns      2086942 items_per_second=22.8917M/s
BF_Fixture/BM_QueryMiss/16777216        404 ns          403 ns      1751578 items_per_second=19.8487M/s
BF_Fixture/BM_QueryMiss/33554432        506 ns          504 ns      1000000 items_per_second=15.8583M/s
```

