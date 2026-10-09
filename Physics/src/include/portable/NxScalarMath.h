#ifndef NX_PHYSICS_SCALAR_MATH_H
#define NX_PHYSICS_SCALAR_MATH_H
#include <math.h>
#include "../../../../Foundation/src/include/NxScalarConversions.h"
/* Binary64 intermediates, grouped as the recovered instruction sequences.
 * Require round-to-nearest, gradual underflow, no contraction/reassociation.
 * The entry point owns the environment; helpers never change caller state.
 * Float-facing stores stay explicit at their existing call sites. Inputs
 * outside the binary64 intermediate range follow standard math classes,
 * rather than x87's wider exponent range or FSIN/FCOS hardware range limit.
 */
static inline float nxScalarStoreFloat(double x) { return (float)x; }
static inline double nxScalarSqrt(double x) { return sqrt(x); }
static inline double nxScalarSqrtSum2(double a,double b) { return sqrt(a+b); }
static inline double nxScalarSqrtSum3(double a,double b,double c) { return sqrt((a+b)+c); }
static inline double nxScalarSqrtSum4(double a,double b,double c,double d) { return sqrt(((a+b)+c)+d); }
static inline double nxScalarSqrtDiffSum(double a,double b,double c) { return sqrt((a-b)+c); }
static inline double nxScalarSqrtDiag(double a,double b,double c) { return sqrt((a-(b+c))+1.0); }
static inline double nxScalarSqrtMulSub(double a,double b,double c) { return sqrt(a*b-c); }
static inline double nxScalarSqrtDot2(double a,double b,double c,double d) { return sqrt(a*b+c*d); }
static inline double nxScalarSqrtDot3(double a,double b,double c,double d,double e,double f) { return sqrt((a*b+c*d)+e*f); }
static inline double nxScalarSqrtDot4(double a,double b,double c,double d,double e,double f,double g,double h) { return sqrt(((a*b+c*d)+e*f)+g*h); }
static inline double nxScalarSqrtQuotDot3(double n,double a,double b,double c,double d,double e,double f) { return sqrt(n/((a*b+c*d)+e*f)); }
static inline double nxScalarSinHalfOverNorm3(double dt,double half,double x,double y,double z)
{
    const double length=sqrt((x*x+y*y)+z*z);
    return sin((dt*length)*half)/length;
}
static inline double nxScalarCosHalfNorm3(double dt,double half,double x,double y,double z)
{
    const double length=sqrt((x*x+y*y)+z*z);
    return cos((dt*length)*half);
}
static inline double nxScalarRateOverRoot(double a,double r,double w) { return (a*r+a*r)/sqrt(1.0-w*w); }
static inline double nxScalarAcos(double x) { return acos(x); }
static inline double nxScalarAcosRateOverRoot(double w,double r) { return nxScalarRateOverRoot(acos(w),r,w); }
/* Joint core preserves its recovered atan2 grouping. Its float-facing wrapper
 * clamps first and returns the original float pi promoted to double. */
static inline double nxScalarJointCIacos(double x) { return atan2(sqrt((1.0+x)*(1.0-x)),x); }
static inline double nxScalarJointAcos(float x)
{
    if (x >= 1.0f) return 0.0f;
    if (x <= -1.0f) return 3.14159265358979323846f;
    return nxScalarJointCIacos(x);
}
#endif
