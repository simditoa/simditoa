include(${PROJECT_SOURCE_DIR}/cmake/add_cpp_test.cmake)

add_cpp_test(basic_test)

if(NOT MSVC AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
  add_library(riscv_compile_smoke OBJECT
      ${PROJECT_SOURCE_DIR}/src/simditoa.cpp
      riscv_compile_smoke.cpp
  )
  target_include_directories(riscv_compile_smoke PRIVATE
      ${PROJECT_SOURCE_DIR}/src
      ${PROJECT_SOURCE_DIR}/include
  )
  set_target_properties(riscv_compile_smoke PROPERTIES
      CXX_STANDARD ${SIMDITOA_CXX_STANDARD}
      CXX_STANDARD_REQUIRED ON
      CXX_EXTENSIONS OFF
  )
  target_compile_options(riscv_compile_smoke PRIVATE
      -D__riscv=1
      -D__riscv_xlen=64
      -U__x86_64__
      -U__amd64__
      -U__AVX512IFMA__
      -U__AVX512VBMI__
  )
endif()
