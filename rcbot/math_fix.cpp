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

	// GetCPUInformation - return valid pointer to static CPUInformation
	// Code in the SDK dereferences this, so it cannot be nullptr
	struct CPUInformationStub
	{
		int m_Size;
		bool m_bRDTSC : 1, m_bCMOV : 1, m_bFCMOV : 1, m_bSSE : 1,
		     m_bSSE2 : 1, m_b3DNow : 1, m_bMMX : 1, m_bHT : 1;
		unsigned char m_nLogicalProcessors;
		unsigned char m_nPhysicalProcessors;
		bool m_bSSE3 : 1, m_bSSSE3 : 1, m_bSSE4a : 1, m_bSSE41 : 1,
		     m_bSSE42 : 1, m_bAVX : 1;
		long long m_Speed;
		char* m_szProcessorID;
		unsigned int m_nModel;
		unsigned int m_nFeatures[3];
		char* m_szProcessorBrand;
		unsigned int m_nL1CacheSizeKb;
		unsigned int m_nL1CacheDesc;
		unsigned int m_nL2CacheSizeKb;
		unsigned int m_nL2CacheDesc;
		unsigned int m_nL3CacheSizeKb;
		unsigned int m_nL3CacheDesc;
	};
	static CPUInformationStub g_CPUInfo = {
		sizeof(CPUInformationStub), // m_Size
		true, true, true, true,     // RDTSC, CMOV, FCMOV, SSE
		true, false, true, false,   // SSE2, 3DNow, MMX, HT
		1, 1,                        // 1 logical, 1 physical processor
		true, true, false, true,    // SSE3, SSSE3, SSE4a, SSE41
		true, false,                // SSE42, AVX
		2000000000LL,               // 2 GHz
		nullptr, 0, {0, 0, 0},      // processor ID, model, features
		nullptr, 0, 0, 0, 0, 0, 0   // brand, cache info
	};
	void* GetCPUInformation()
	{
		return &g_CPUInfo;
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

//=============================================================================
// IKeyValuesSystem stub implementation
// KeyValuesSystem() MUST return a valid object, not nullptr!
// The SDK's KeyValues class calls this for memory allocation.
// Using simple arrays to avoid static initialization order issues with std::map
//=============================================================================
#include <cstdlib>
#include <cstring>

typedef int HKeySymbol;
#define INVALID_KEY_SYMBOL (-1)
#define MAX_KEY_SYMBOLS 4096

class CKeyValuesSystemStub
{
public:
	CKeyValuesSystemStub() : m_nSymbolCount(0) {
		std::memset(m_Symbols, 0, sizeof(m_Symbols));
	}

	virtual void RegisterSizeofKeyValues(int size) { (void)size; }

	virtual void* AllocKeyValuesMemory(int size)
	{
		return std::malloc(static_cast<size_t>(size));
	}

	virtual void FreeKeyValuesMemory(void* pMem)
	{
		std::free(pMem);
	}

	virtual HKeySymbol GetSymbolForString(const char* name, bool bCreate = true)
	{
		if (!name || !*name)
			return INVALID_KEY_SYMBOL;

		// Simple linear search (OK for limited symbols)
		for (int i = 0; i < m_nSymbolCount; i++) {
			if (m_Symbols[i] && std::strcmp(m_Symbols[i], name) == 0)
				return i;
		}

		if (!bCreate || m_nSymbolCount >= MAX_KEY_SYMBOLS)
			return INVALID_KEY_SYMBOL;

		// Create new symbol
		size_t len = std::strlen(name) + 1;
		m_Symbols[m_nSymbolCount] = static_cast<char*>(std::malloc(len));
		if (m_Symbols[m_nSymbolCount]) {
			std::memcpy(m_Symbols[m_nSymbolCount], name, len);
		}
		return m_nSymbolCount++;
	}

	virtual const char* GetStringForSymbol(HKeySymbol symbol)
	{
		if (symbol >= 0 && symbol < m_nSymbolCount && m_Symbols[symbol])
			return m_Symbols[symbol];
		return "";
	}

	virtual void AddKeyValuesToMemoryLeakList(void* pMem, HKeySymbol name)
	{
		(void)pMem; (void)name;
	}

	virtual void RemoveKeyValuesFromMemoryLeakList(void* pMem)
	{
		(void)pMem;
	}

	virtual void AddFileKeyValuesToCache(const void* _kv, const char* resourceName, const char* pathID)
	{
		(void)_kv; (void)resourceName; (void)pathID;
	}

	virtual bool LoadFileKeyValuesFromCache(void* _outKv, const char* resourceName, const char* pathID, void* filesystem) const
	{
		(void)_outKv; (void)resourceName; (void)pathID; (void)filesystem;
		return false; // Never load from cache
	}

	virtual void InvalidateCache() {}

	virtual void InvalidateCacheForFile(const char* resourceName, const char* pathID)
	{
		(void)resourceName; (void)pathID;
	}

	virtual void SetKeyValuesExpressionSymbol(const char* name, bool bValue)
	{
		(void)name; (void)bValue; // Simplified stub
	}

	virtual bool GetKeyValuesExpressionSymbol(const char* name)
	{
		(void)name;
		return false; // Simplified stub
	}

	virtual HKeySymbol GetSymbolForStringCaseSensitive(HKeySymbol& hCaseInsensitiveSymbol, const char* name, bool bCreate = true)
	{
		hCaseInsensitiveSymbol = GetSymbolForString(name, bCreate);
		return hCaseInsensitiveSymbol;
	}

private:
	char* m_Symbols[MAX_KEY_SYMBOLS];
	int m_nSymbolCount;
};

static CKeyValuesSystemStub g_KeyValuesSystemStub;

// KeyValuesSystem must return valid object
void* KeyValuesSystem()
{
	return &g_KeyValuesSystemStub;
}

//=============================================================================
// ICommandLine stub implementation
// CommandLine_Tier0() must return a valid object pointer, not nullptr
//=============================================================================
class CCommandLineStub
{
public:
	virtual void CreateCmdLine(const char* commandline) {}
	virtual void CreateCmdLine(int argc, char** argv) {}
	virtual const char* GetCmdLine(void) const { return ""; }
	virtual const char* CheckParm(const char* psz, const char** ppszValue = nullptr) const { return nullptr; }
	virtual void RemoveParm(const char* parm) {}
	virtual void AppendParm(const char* pszParm, const char* pszValues) {}
	virtual const char* ParmValue(const char* psz, const char* pDefaultVal = nullptr) const { return pDefaultVal; }
	virtual int ParmValue(const char* psz, int nDefaultVal) const { return nDefaultVal; }
	virtual float ParmValue(const char* psz, float flDefaultVal) const { return flDefaultVal; }
	virtual int ParmCount() const { return 0; }
	virtual int FindParm(const char* psz) const { return 0; }
	virtual const char* GetParm(int nIndex) const { return ""; }
	virtual void SetParm(int nIndex, const char* pNewParm) {}
	virtual const char* ParmValueByIndex(int nIndex, const char* pDefaultVal = nullptr) const { return pDefaultVal; }
	virtual bool HasParm(const char* psz) const { return false; }
	virtual const char** GetParms() const { return nullptr; }
	virtual void CreateCmdLine1(const char* commandline, bool bParseParamFiles) {}
	virtual void CreateCmdLine1(int argc, char** argv, bool bParseParamFiles) {}
};

static CCommandLineStub g_CommandLineStub;

extern "C" void* CommandLine_Tier0()
{
	return &g_CommandLineStub;
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