// Prints what the pinned NxNormalToTangents returns for each axis the joint
// differential uses, so the oracle's localNormal values can be attributed to a
// specific output rather than guessed at.

#include "PhysicsPairLoader.h"

#include <string.h>

#include "NxVec3.h"

// NxNormalToTangents is exported by the Foundation half of the pair, and this
// harness links against neither side's import library, so it is resolved from
// the loaded Foundation module like every other address these tests call.
typedef void (NX_CALL_CONV *NormalToTangentsFn)(const NxVec3&, NxVec3&, NxVec3&);

static NormalToTangentsFn nxNormalToTangents = 0;

static NxU32 nxU(float v) { NxU32 b; memcpy(&b, &v, 4); return b; }

static void show(const char* tag, const NxVec3& n)
	{
	NxVec3 t1(0.0f, 0.0f, 0.0f);
	NxVec3 t2(0.0f, 0.0f, 0.0f);
	nxNormalToTangents(n, t1, t2);
	printf("%s n=%08x.%08x.%08x t1=%08x.%08x.%08x t2=%08x.%08x.%08x\n", tag,
		nxU(n.x), nxU(n.y), nxU(n.z),
		nxU(t1.x), nxU(t1.y), nxU(t1.z),
		nxU(t2.x), nxU(t2.y), nxU(t2.z));
	}

int wmain(int argc, wchar_t** argv)
	{
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxNormalToTangentsProbe", pairDirectory, &physics);
	if(status)
		return status;

	HMODULE foundation = GetModuleHandleW(L"NxFoundation.dll");
	if(!foundation)
		{
		FreeLibrary(physics);
		return nxFail("NxFoundation.dll is not loaded");
		}
	nxNormalToTangents = reinterpret_cast<NormalToTangentsFn>(
		GetProcAddress(foundation, "NxNormalToTangents"));
	printf("export=NxNormalToTangents present=%s\n", nxNormalToTangents ? "yes" : "no");
	if(!nxNormalToTangents)
		{
		FreeLibrary(physics);
		return nxFail("NxNormalToTangents is missing; the probe cannot run");
		}

	// Case 0's normalised axis: (0.5,0.5,0.5) / |.| = 0x3f13cd3a each.
	show("case0", NxVec3(0x3f13cd3au ? 0.57735026f : 0.0f, 0.57735026f, 0.57735026f));
	// Case 1's axis is zero, so the row leaves it unnormalised.
	show("case1", NxVec3(0.0f, 0.0f, 0.0f));
	// Case 3's axis: (0,1,0).
	show("case3", NxVec3(0.0f, 1.0f, 0.0f));

	return nxReportPairIdentity(pairDirectory);
	}