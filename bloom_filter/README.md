# Initial Bench
````
2025-12-17T17:44:54-07:00
Running ./bench
Run on (16 X 1754.31 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 512 KiB (x8)
  L3 Unified 32768 KiB (x1)
Load Average: 0.00, 0.02, 0.00
----------------------------------------------------------------------------------------
Benchmark                              Time             CPU   Iterations UserCounters...
----------------------------------------------------------------------------------------
SBBF_Fixture/BM_Query/8192          27.3 ns         27.2 ns     25755959 items_per_second=36.7929M/s
SBBF_Fixture/BM_Query/32768         27.3 ns         27.2 ns     25760532 items_per_second=36.7933M/s
SBBF_Fixture/BM_Query/262144        27.2 ns         27.1 ns     25784943 items_per_second=36.8358M/s
SBBF_Fixture/BM_Query/1048576       27.2 ns         27.1 ns     25853721 items_per_second=36.9304M/s
````

Changing
```
bool result = filter->possiblyContains(keys[i++ % 1000]);
to
bool result = filter->possiblyContains(keys[i++ & 1023]);
````

````
2025-12-17T17:46:53-07:00
Running ./bench
Run on (16 X 3771.84 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 512 KiB (x8)
  L3 Unified 32768 KiB (x1)
Load Average: 0.08, 0.04, 0.00
----------------------------------------------------------------------------------------
Benchmark                              Time             CPU   Iterations UserCounters...
----------------------------------------------------------------------------------------
SBBF_Fixture/BM_Query/8192          26.1 ns         26.0 ns     26919069 items_per_second=38.4559M/s
SBBF_Fixture/BM_Query/32768         26.1 ns         26.0 ns     26948578 items_per_second=38.4962M/s
SBBF_Fixture/BM_Query/262144        26.0 ns         25.9 ns     26982815 items_per_second=38.5487M/s
SBBF_Fixture/BM_Query/1048576       25.8 ns         25.7 ns     27211113 items_per_second=38.8727M/s
````
