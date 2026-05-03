cmake_minimum_required(VERSION 3.14)

project(
    simditoa
    VERSION 0.1.0
    DESCRIPTION "SIMD-accelerated integer-to-string conversion"
    LANGUAGES CXX
)

include(GNUInstallDirs)

string(
    COMPARE EQUAL
    "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}"
    is_top_project
)

# ---- Options ----

option(SIMDITOA_SANITIZE "Sanitize addresses" OFF)
option(SIMDITOA_SANITIZE_UNDEFINED "Sanitize undefined behavior" OFF)

if(NOT CMAKE_BUILD_TYPE)
  if(SIMDITOA_SANITIZE OR SIMDITOA_SANITIZE_UNDEFINED)
    message(STATUS "No build type selected, default to Debug")
    set(CMAKE_CXX_FLAGS_DEBUG "-O1" CACHE STRING "" FORCE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
  else()
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  endif()
endif()

set(SIMDITOA_CXX_STANDARD 17 CACHE STRING "the C++ standard to use for simditoa")
set(CMAKE_CXX_STANDARD ${SIMDITOA_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- simditoa library ----

add_library(simditoa src/simditoa.cpp)
add_library(simditoa::simditoa ALIAS simditoa)

target_include_directories(simditoa
    PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(NOT DEFINED CMAKE_POSITION_INDEPENDENT_CODE)
  set_target_properties(simditoa PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

if(MSVC)
  target_compile_options(simditoa PRIVATE /WX /W3 /sdl)
else()
  target_compile_options(simditoa PRIVATE
      -Wall -Wextra -Weffc++
      -Wsign-compare -Wshadow -Wwrite-strings
      -Wpointer-arith -Winit-self -Wconversion -Wno-sign-conversion
  )
  target_compile_options(simditoa PRIVATE $<$<CONFIG:DEBUG>:-Og>)
endif()

if(SIMDITOA_SANITIZE)
  target_compile_options(simditoa PUBLIC -fsanitize=address -fno-omit-frame-pointer -fno-sanitize-recover=all)
  target_link_libraries(simditoa PUBLIC -fsanitize=address -fno-omit-frame-pointer -fno-sanitize-recover=all)
endif()
if(SIMDITOA_SANITIZE_UNDEFINED)
  target_compile_options(simditoa PUBLIC -fsanitize=undefined -fno-sanitize-recover=all)
  target_link_libraries(simditoa PUBLIC -fsanitize=undefined -fno-sanitize-recover=all)
endif()

# ---- Install ----

install(
    TARGETS simditoa
    EXPORT simditoaTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
    DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    EXPORT simditoaTargets
    FILE simditoaTargets.cmake
    NAMESPACE simditoa::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/simditoa
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/simditoa-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/simditoaConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/simditoa
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/simditoaConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/simditoaConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/simditoaConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/simditoa
)

# ---- Developer mode ----

if(is_top_project)
  option(SIMDITOA_DEVELOPER_MODE "Enable targets for developing simditoa" OFF)
endif()

if(is_top_project AND NOT SIMDITOA_DEVELOPER_MODE)
  message(STATUS "Building only the library. Turn SIMDITOA_DEVELOPER_MODE ON for tests.")
  return()
endif()

enable_testing()
add_subdirectory(tests)

option(SIMDITOA_BUILD_BENCHMARKS "Build benchmarks" OFF)
if(SIMDITOA_BUILD_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()
