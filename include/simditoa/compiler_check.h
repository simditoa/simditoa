#ifndef SIMDITOA_COMPILER_CHECK_H
#define SIMDITOA_COMPILER_CHECK_H

#ifndef __cplusplus
  #error simditoa requires a C++ compiler
#endif

#ifndef SIMDITOA_CPLUSPLUS
  #if defined(_MSVC_LANG) && !defined(__clang__)
    #define SIMDITOA_CPLUSPLUS (_MSC_VER == 1900 ? 201103L : _MSVC_LANG)
  #else
    #define SIMDITOA_CPLUSPLUS __cplusplus
  #endif
#endif

#if !defined(SIMDITOA_CPLUSPLUS20) && (SIMDITOA_CPLUSPLUS >= 202002L)
  #define SIMDITOA_CPLUSPLUS20 1
#endif

#if !defined(SIMDITOA_CPLUSPLUS17) && (SIMDITOA_CPLUSPLUS >= 201703L)
  #define SIMDITOA_CPLUSPLUS17 1
#endif

#ifndef SIMDITOA_CPLUSPLUS17
  #error simditoa requires a compiler compliant with the C++17 standard
#endif

#endif // SIMDITOA_COMPILER_CHECK_H
