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

#include <cstdio>
#include <cstdarg>
#include <pthread.h>
#include <vector>
#include <string>

// Forward declare Color class to match SDK signature
class Color;

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

	// C linkage console functions
	void Msg(const char* pMsg, ...)
	{
		va_list args;
		va_start(args, pMsg);
		vprintf(pMsg, args);
		va_end(args);
	}

	void Warning(const char* pMsg, ...)
	{
		va_list args;
		va_start(args, pMsg);
		vprintf(pMsg, args);
		va_end(args);
	}

	void Error(const char* pMsg, ...)
	{
		va_list args;
		va_start(args, pMsg);
		vfprintf(stderr, pMsg, args);
		va_end(args);
	}

	// COM_TimestampedLog - logging with timestamps, stub silently
	void COM_TimestampedLog(const char* pMsg, ...)
	{
	}

	// HushAsserts - silence asserts
	void HushAsserts()
	{
	}

	// GetCPUInformation - return null (not needed for plugin operation)
	void* GetCPUInformation()
	{
		return nullptr;
	}

	// CommandLine - return null (plugin doesn't need command line access)
	void* CommandLine_Tier0()
	{
		return nullptr;
	}

	// KeyValuesSystem - return null
	void* KeyValuesSystem()
	{
		return nullptr;
	}

	// Thread ID
	unsigned long ThreadGetCurrentId()
	{
		return static_cast<unsigned long>(pthread_self());
	}

	// DevMsg with C linkage (tier1 static library references this)
	void DevMsg(const char* pMsg, ...)
	{
		// DevMsg is typically only shown in developer mode, stub it silently
	}
}

// DevMsg with C++ linkage - use asm label to specify the mangled name
// Symbol: _Z6DevMsgPKcz
void __attribute__((used)) DevMsg_cpp(const char* pMsg, ...) asm("_Z6DevMsgPKcz");
void DevMsg_cpp(const char* pMsg, ...)
{
	// DevMsg is typically only shown in developer mode, stub it silently
}

void ConMsg(const char* pMsg, ...)
{
	va_list args;
	va_start(args, pMsg);
	vprintf(pMsg, args);
	va_end(args);
}

void ConColorMsg(const Color& clr, const char* pMsg, ...)
{
	va_list args;
	va_start(args, pMsg);
	vprintf(pMsg, args);  // Ignore color, just print
	va_end(args);
}

// CThreadFastMutex::Lock stub
// Symbol: _ZNV16CThreadFastMutex4LockEjj = CThreadFastMutex::Lock(unsigned int, unsigned int) volatile
// The 'V' in the mangled name indicates the method is volatile-qualified on 'this'
class CThreadFastMutex
{
public:
	// Use __attribute__((noinline, used)) to force symbol emission
	__attribute__((noinline, used))
	void Lock(unsigned int spinCount, unsigned int timeout) volatile
	{
		// Stub - do nothing, single-threaded assumption for older engines
		(void)spinCount;
		(void)timeout;
	}
};

// Create a global instance to ensure class methods are emitted
static volatile CThreadFastMutex g_stubMutex;

// Force the symbol to be used
__attribute__((constructor))
static void __force_CThreadFastMutex_symbol_init()
{
	g_stubMutex.Lock(0, 0);
}

#endif