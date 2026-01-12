cmake_minimum_required(VERSION 3.10)
project(MyBenchmark)

set(CMAKE_CXX_STANDARD 17)

# Find the package on your system
find_package(benchmark REQUIRED)

add_executable(run_bench bench.cpp)

# Link the library
target_link_libraries(run_bench PRIVATE benchmark::benchmark)

# Enable x86 SIMD when available (Ryzen 5800X supports AVX2/FMA).
# Target-scoped so consumers can override easily.
include(CheckCXXCompilerFlag)

if(MSVC)
  check_cxx_compiler_flag("/arch:AVX2" HAS_AVX2_FLAG)
  if(HAS_AVX2_FLAG)
    target_compile_options(run_bench PRIVATE "/arch:AVX2")
  endif()
else()
  check_cxx_compiler_flag("-mavx2" HAS_AVX2_FLAG)
  if(HAS_AVX2_FLAG)
    target_compile_options(run_bench PRIVATE -mavx2)
  endif()

  check_cxx_compiler_flag("-mfma" HAS_FMA_FLAG)
  if(HAS_FMA_FLAG)
    target_compile_options(run_bench PRIVATE -mfma)
  endif()
endif()
