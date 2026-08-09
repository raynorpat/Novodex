#define NOMINMAX
#include <windows.h>
#include <math.h>
#include <stdio.h>

#include "NxSphere.h"
#include "NxUtilities.h"

typedef NxU32 (NX_CALL_CONV *Crc32Fn)(const void*, NxU32);
typedef void (NX_CALL_CONV *MergeSpheresFn)(NxSphere&, const NxSphere&, const NxSphere&);

static bool nearValue(NxReal actual, NxReal expected)
	{
	return fabsf(actual - expected) <= 1.0e-6f;
	}

static bool checkSphere(const NxSphere& sphere, NxReal x, NxReal y, NxReal z, NxReal radius)
	{
	return nearValue(sphere.center.x, x)
		&& nearValue(sphere.center.y, y)
		&& nearValue(sphere.center.z, z)
		&& nearValue(sphere.radius, radius);
	}

int main(int argc, char** argv)
	{
	if(argc != 2)
		{
		fprintf(stderr, "usage: NxFoundationExportTests <NxFoundation.dll>\n");
		return 2;
		}

	HMODULE module = LoadLibraryA(argv[1]);
	if(!module)
		{
		fprintf(stderr, "LoadLibrary failed: %lu\n", GetLastError());
		return 2;
		}

	Crc32Fn crc32 = reinterpret_cast<Crc32Fn>(GetProcAddress(module, "NxCrc32"));
	MergeSpheresFn mergeSpheres = reinterpret_cast<MergeSpheresFn>(GetProcAddress(module, "NxMergeSpheres"));
	if(!crc32 || !mergeSpheres)
		{
		fprintf(stderr, "missing exports: NxCrc32=%p NxMergeSpheres=%p\n", crc32, mergeSpheres);
		FreeLibrary(module);
		return 1;
		}

	const char check[] = "123456789";
	if(crc32(0, 0) != 0 || crc32(check, 9) != 0x2dfd2d88)
		{
		fprintf(stderr, "NxCrc32 behavior mismatch\n");
		FreeLibrary(module);
		return 1;
		}

	NxSphere sphere0(NxVec3(0, 0, 0), 1.0f);
	NxSphere sphere1(NxVec3(4, 0, 0), 2.0f);
	NxSphere merged;
	mergeSpheres(merged, sphere0, sphere1);
	if(!checkSphere(merged, 2.5f, 0, 0, 3.5f))
		{
		fprintf(stderr, "NxMergeSpheres disjoint behavior mismatch\n");
		FreeLibrary(module);
		return 1;
		}

	NxSphere outer(NxVec3(1, 2, 3), 5.0f);
	NxSphere inner(NxVec3(2, 2, 3), 1.0f);
	mergeSpheres(merged, outer, inner);
	if(!checkSphere(merged, 1, 2, 3, 5.0f))
		{
		fprintf(stderr, "NxMergeSpheres containment behavior mismatch\n");
		FreeLibrary(module);
		return 1;
		}

	FreeLibrary(module);
	return 0;
	}
