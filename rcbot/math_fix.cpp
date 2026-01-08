// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <cmath>
#include <ctime>

#ifdef MATH_LIB_FIX
// GCC9 Developers broke compatibility with math options so we need to redeclare them
// https://stackoverflow.com/questions/63261220/link-errors-with-ffast-math-ffinite-math-only-and-glibc-2-31
// https://forums.developer.nvidia.com/t/problems-on-ubuntu-20-04-linuxpower/136507/3
// from https://gitlab.com/counterstrikesource/sm-exts/sm-ext-common/-/blob/master/mathstubs.c
// -sappho

// Fix for missing functions in mathlib
// Thanks to sappho from AM discord
// Is bot.cpp the best place for this? IDK. It just works. -caxanga334
extern "C"
{
	double __acos_finite(double x) { return acos(x); }
	float __acosf_finite(float x) { return acosf(x); }
	double __acosh_finite(double x) { return acosh(x); }
	float __acoshf_finite(float x) { return acoshf(x); }
	double __asin_finite(double x) { return asin(x); }
	float __asinf_finite(float x) { return asinf(x); }
	double __atanh_finite(double x) { return atanh(x); }
	float __atanhf_finite(float x) { return atanhf(x); }
	double __cosh_finite(double x) { return cosh(x); }
	float __coshf_finite(float x) { return coshf(x); }
	double __sinh_finite(double x) { return sinh(x); }
	float __sinhf_finite(float x) { return sinhf(x); }
	double __exp_finite(double x) { return exp(x); }
	float __expf_finite(float x) { return expf(x); }
	double __log10_finite(double x) { return log10(x); }
	float __log10f_finite(float x) { return log10f(x); }
	double __log_finite(double x) { return log(x); }
	float __logf_finite(float x) { return logf(x); }
	double __atan2_finite(double x, double y) { return atan2(x, y); }
	float __atan2f_finite(float x, double y) { return atan2f(x, y); }
	double __pow_finite(double x, double y) { return pow(x, y); }
	float __powf_finite(float x, double y) { return powf(x, y); }
	double __remainder_finite(double x, double y) { return remainder(x, y); }
	float __remainderf_finite(float x, double y) { return remainderf(x, y); }
}
#endif

// Stub implementations for tier0 platform functions that aren't exported by older Source engines
// These are needed because tier1 static library references them but the engine doesn't provide them
#if defined(_LINUX) || defined(__linux__)
extern "C"
{
	// Thread-safe localtime wrapper - used by tier1/strtools.cpp
	struct tm* Plat_localtime(const time_t* timep, struct tm* result)
	{
		return localtime_r(timep, result);
	}

	// Thread-safe gmtime wrapper
	struct tm* Plat_gmtime(const time_t* timep, struct tm* result)
	{
		return gmtime_r(timep, result);
	}

	// Check if running in debugger - always return false for plugins
	int Plat_IsInDebugSession()
	{
		return 0;
	}
}
#endif