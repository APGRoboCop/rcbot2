
#if 0
# define printf(...) AMBUILD_CATCH_THIS __VA_ARGS__
#else
# include <stdio.h>
# include <stdlib.h>
#endif

int main()
{
#if defined __ICC
  printf("icc %d\n", __ICC);
#elif defined(__EMSCRIPTEN__)
  printf("emscripten %d.%d\n", __clang_major__, __clang_minor__);
#elif defined __clang__
# if defined(__clang_major__) && defined(__clang_minor__)
#  if defined(__apple_build_version__)
    printf("apple-clang %d.%d\n", __clang_major__, __clang_minor__);
#  else
    printf("clang %d.%d\n", __clang_major__, __clang_minor__);
#  endif
# else
  printf("clang 1.%d\n", __GNUC_MINOR__);
# endif
#elif defined __GNUC__
  printf("gcc %d.%d\n", __GNUC__, __GNUC_MINOR__);
#elif defined _MSC_VER
  printf("msvc %d\n", _MSC_VER);
#elif defined __TenDRA__
  printf("tendra 0\n");
#elif defined __SUNPRO_C
  printf("sun %d\n", __SUNPRO_C);
#elif defined __SUNPRO_CC
  printf("sun %d\n", __SUNPRO_CC);
#else
#error "Unrecognized compiler!"
#endif
#if defined __cplusplus
  printf("CXX\n");
#else
  printf("CC\n");
#endif
#if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64_) || \
    defined(_M_X64) || defined(_M_AMD64)
  printf("x86_64\n");
#elif defined(__aarch64__)
  printf("arm64\n");
#elif defined(i386) || defined(__i386) || defined(__i386__) || defined(__i686__) || \
      defined(__i386) || defined(_M_IX86)
  printf("x86\n");
#elif defined(__aarch64__) || defined(_M_ARM64)
  printf("arm64\n");
#elif defined(__arm__) || defined(_M_ARM)
  printf("arm\n");
#else
  printf("unknown\n");
#endif
  exit(0);
}
