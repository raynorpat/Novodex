// Prints what NxMath::sqrt returns for the degenerate operands, and what
// 1.0/sqrt(0) is, so a change to NxMath::sqrt can be shown to have taken effect
// rather than merely to have compiled.

#include "PhysicsPairLoader.h"

#include <string.h>
#include <math.h>

#include "NxMath.h"

static NxU32 nxU(float v) { NxU32 b; memcpy(&b, &v, 4); return b; }
static NxU32 nxU64(double v) { unsigned long long b; memcpy(&b, &v, 8); return static_cast<NxU32>(b & 0xffffffffu); }

int wmain(int argc, wchar_t** argv)
	{
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxMathSqrtProbe", pairDirectory, &physics);
	if(status)
		return status;

	const NxF32 zero32 = 0.0f;
	const NxF64 zero64 = 0.0;
	const NxF32 negzero32 = -0.0f;
	const NxF64 negzero64 = -0.0;

	printf("sqrt32(+0)=%08x crt_sqrtf(+0)=%08x\n",
		nxU(NxMath::sqrt(zero32)), nxU(::sqrtf(zero32)));
	printf("sqrt32(-0)=%08x crt_sqrtf(-0)=%08x\n",
		nxU(NxMath::sqrt(negzero32)), nxU(::sqrtf(negzero32)));
	printf("sqrt64(+0)=%08x crt_sqrt(+0)=%08x\n",
		nxU64(NxMath::sqrt(zero64)), nxU64(::sqrt(zero64)));

	// What 1/sqrt(0) is, both through NxMath and through the CRT.
	const NxF64 inv = 1.0 / NxMath::sqrt(zero64);
	const NxF64 invc = 1.0 / ::sqrt(zero64);
	printf("1/sqrt64(+0)=%08x 1/crt_sqrt(+0)=%08x\n", nxU64(inv), nxU64(invc));

	// A negative operand, where fsqrt and the CRT are known to disagree on NaN.
	const NxF32 neg = -1.0f;
	printf("sqrt32(-1)=%08x crt_sqrtf(-1)=%08x\n",
		nxU(NxMath::sqrt(neg)), nxU(::sqrtf(neg)));

	return nxReportPairIdentity(pairDirectory);
	}