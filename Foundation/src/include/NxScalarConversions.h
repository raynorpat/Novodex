#ifndef NX_SCALAR_CONVERSIONS_H
#define NX_SCALAR_CONVERSIONS_H
#include <stdint.h>
#include <float.h>
#include <math.h>
#include <fenv.h>

/* Private C-compatible conversions. No platform, SDK, or Physics dependency.
 * Arithmetic callers require FE_TONEAREST; these functions never change it.
 * Explicit floor/ceil/chop policies do not depend on the caller's rounding.
 */
enum NxScalarRounding { NX_SCALAR_NEAREST, NX_SCALAR_CHOP, NX_SCALAR_FLOOR, NX_SCALAR_CEIL };
static inline int nxScalarIsRoundToNearest(void) { return fegetround() == FE_TONEAREST; }
static inline int nxScalarFinite(double x) { return x == x && x <= DBL_MAX && x >= -DBL_MAX; }
static inline double nxScalarRound(double x, enum NxScalarRounding policy)
{
    if (policy == NX_SCALAR_FLOOR) return floor(x);
    if (policy == NX_SCALAR_CEIL) return ceil(x);
    if (policy == NX_SCALAR_CHOP) return x < 0.0 ? ceil(x) : floor(x);
    /* Explicit ties to even, including when called outside FE_TONEAREST.
     * Above 2^52 every finite binary64 value is already integral. */
    if (fabs(x) >= 4503599627370496.0) return x;
    {
        double lower = floor(x), fraction = x - lower;
        if (fraction < 0.5) return lower;
        if (fraction > 0.5) return lower + 1.0;
        return fmod(lower, 2.0) == 0.0 ? lower : lower + 1.0;
    }
}
/* Failure does not write output. Check the rounded value before any cast. */
static inline int nxScalarInt32(double x, enum NxScalarRounding policy, int32_t* output)
{
    double rounded;
    if (!nxScalarFinite(x) || !output) return 0;
    rounded = nxScalarRound(x, policy);
    if (rounded < -2147483648.0 || rounded > 2147483647.0) return 0;
    *output = (int32_t)rounded;
    return 1;
}
/* FISTP qword followed by signed low-dword observation. The x87 invalid
 * integer is INT64_MIN, whose low word is zero. This is a storage contract,
 * not saturation to int32. Avoid int64 overflow and unsigned-to-signed casts.
 * The upper bound is exclusive because binary64 cannot represent INT64_MAX.
 */
static inline int32_t nxScalarFistpLow32(double x)
{
    double rounded, low;
    if (!nxScalarFinite(x)) return 0;
    rounded = nxScalarRound(x, NX_SCALAR_NEAREST);
    if (rounded < -9223372036854775808.0 || rounded >= 9223372036854775808.0) return 0;
    low = fmod(rounded, 4294967296.0);
    if (low < 0.0) low += 4294967296.0;
    if (low >= 2147483648.0) low -= 4294967296.0;
    return (int32_t)low;
}
/* Public NxInt* cannot report failure separately. Portable invalid-input
 * convention: INT32_MIN (also a valid result). Private callers use the
 * checked form above to distinguish failure. No legacy invalid-input claim.
 */
static inline int32_t nxScalarInt32OrIndefinite(double x, enum NxScalarRounding policy)
{
    int32_t result;
    return nxScalarInt32(x, policy, &result) ? result : INT32_MIN;
}
#endif
