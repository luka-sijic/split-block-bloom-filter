cmake_minimum_required(VERSION 3.10)
project(MyBenchmark)

set(CMAKE_CXX_STANDARD 17)

# Find the package on your system
find_package(benchmark REQUIRED)

add_executable(run_bench bench.cpp)

# Link the library
target_link_libraries(run_bench PRIVATE benchmark::benchmark)
