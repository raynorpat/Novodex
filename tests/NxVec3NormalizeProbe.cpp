// Prints what NxVec3::magnitude and NxVec3::normalize produce for the vectors the
// degenerate tangent path builds, so the NaN sign can be attributed to one call
// rather than to the whole tangent function.

#include "PhysicsPairLoader.h"

#include <string.h>

#include "NxVec3.h"

static NxU32 nxU(float v) { NxU32 b; memcpy(&b, &v, 4); return b; }

static void show(const char* tag, float x, float y, float z)
	{
	NxVec3 v(x, y, z);
	NxVec3 m = v;
	const NxReal mag = m.magnitude();
	NxReal n = v.normalize();
	printf("%s in=%08x.%08x.%08x magnitude=%08x normalize_ret=%08x out=%08x.%08x.%08x\n",
		tag, nxU(x), nxU(y), nxU(z), nxU(mag), nxU(n),
		nxU(v.x), nxU(v.y), nxU(v.z));
	}

int wmain(int argc, wchar_t** argv)
	{
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxVec3NormalizeProbe", pairDirectory, &physics);
	if(status)
		return status;

	// The zero vector, which is what the degenerate tangent path normalises.
	show("zero", 0.0f, 0.0f, 0.0f);
	// The vectors the else-branch builds from a zero axis: (-0*k, 0*k, 0) where
	// k is infinite, so the components are NaN before normalize runs.
	float inf = 0.0f;
	{
		NxU32 bits = 0x7f800000u;
		memcpy(&inf, &bits, 4);
	}
	show("t1", -0.0f * inf, 0.0f * inf, 0.0f);
	show("t2", -0.0f * inf, 0.0f * inf, 0.0f * inf);
	// A single negative NaN, to see whether normalize's own arithmetic flips it.
	{
		NxU32 neg = 0xffc00000u;
		float nx;
		memcpy(&nx, &neg, 4);
		show("negNaN", nx, nx, nx);
	}
	return nxReportPairIdentity(pairDirectory);
	}