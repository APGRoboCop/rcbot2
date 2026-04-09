// Compatibility wrapper: Clang rejects V_swap() on bit-field members in the
// SDK's CUtlBlockMemory::Swap(). The template V_swap(T&, T&) takes non-const
// references, which cannot bind to bit-fields (m_nIndexMask : 27 and
// m_nIndexShift : 5). GCC and MSVC accept this in an uninstantiated template;
// Clang does not.
//
// Fix: pre-include mathlib.h (so its include guard prevents re-entry), then
// temporarily shadow V_swap with a copy-through-temporary macro that is
// bit-field-safe. After the real header is processed, restore V_swap so the
// rest of the codebase uses the original template. [APG]RoboCop[CL]

#ifdef __clang__

// 1. Pull in mathlib.h so the V_swap template definition is parsed and its
//    include guard (MATH_LIB_H) is set.  When the real utlblockmemory.h later
//    does #include "mathlib/mathlib.h", the guard makes it a no-op.
#include "mathlib/mathlib.h"

// 2. Shadow V_swap with a bit-field-safe macro.
#pragma push_macro("V_swap")
#undef V_swap
#define V_swap(a, b) do { auto _vswap_tmp_ = (a); (a) = (b); (b) = _vswap_tmp_; } while (0)

// 3. Chain to the real SDK header (next in the include-path search order).
#include_next "tier1/utlblockmemory.h"

// 4. Restore V_swap so subsequent code uses the original template.
#pragma pop_macro("V_swap")

#else
// GCC / MSVC — no bit-field reference issue; forward directly.
#include_next "tier1/utlblockmemory.h"
#endif
