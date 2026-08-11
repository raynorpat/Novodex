// GENERATED CASE TABLES - see docs/reconstruction/novodex-physics/cases/geometry.json
//
// The Phase 3 geometry differential.  It calls each of the 27 named exports
// Phase 3 owns over a fixed matrix of cases and prints, for every case, the
// input words it passed, the return value and every output byte, all as raw
// 32-bit hexadecimal.  Nothing here decides whether a result is right: the
// oracle decides, by run_differential.ps1 comparing this transcript from the
// shipped pair against the same transcript from the rebuilt pair.
//
// Floating-point inputs are written as their IEEE-754 bit patterns rather
// than as decimal literals, because the comparison is bit-exact and decimal
// does not round-trip.  Outputs are printed the same way.
//
// Every output buffer is poisoned before the call and two words past the
// declared end of it are printed with it, so "wrote nothing", "wrote a
// wrong value" and "wrote past the end" are three different transcripts.

#include "PhysicsPairLoader.h"

#include <string.h>

#include "Nxf.h"

static NxU32 nxU(float value)
	{
	NxU32 bits;
	memcpy(&bits, &value, 4);
	return bits;
	}

static float nxF(NxU32 bits)
	{
	float value;
	memcpy(&value, &bits, 4);
	return value;
	}

// The scratch array carries the case inputs and the output buffers as raw
// words, so a value only ever passes through a float register when the
// signature says it is passed by value.
static const float* nxVec(const NxU32* s, unsigned offset)
	{
	return reinterpret_cast<const float*>(s + offset);
	}

static float* nxVecW(NxU32* s, unsigned offset)
	{
	return reinterpret_cast<float*>(s + offset);
	}

static void nxCopy(NxU32* s, unsigned offset, const NxU32* src, unsigned count)
	{
	memcpy(s + offset, src, count * 4);
	}

static void nxPoison(NxU32* s, unsigned offset, unsigned count)
	{
	for(unsigned i = 0; i < count; ++i)
		s[offset + i] = 0xcdcd0000u + i;
	}

static unsigned nxGather(NxU32* out, unsigned at, const NxU32* s, unsigned offset, unsigned count)
	{
	memcpy(out + at, s + offset, count * 4);
	return at + count;
	}

static void nxRetBits(char* ret, NxU32 bits)		{ sprintf_s(ret, 16, "%08x", bits); }
static void nxRetVoid(char* ret)					{ strcpy_s(ret, 16, "void"); }

static void nxWords(const NxU32* words, unsigned count)
	{
	if(!count)
		{
		printf("-");
		return;
		}
	for(unsigned i = 0; i < count; ++i)
		printf(i ? ",%08x" : "%08x", words[i]);
	}

static void nxLine(const char* name, unsigned index, const NxU32* in, unsigned nin,
	const char* ret, const NxU32* out, unsigned nout)
	{
	printf("case=%s.%02u in=", name, index);
	nxWords(in, nin);
	printf(" ret=%s out=", ret);
	nxWords(out, nout);
	printf("\n");
	}

static void nxMissing(const char* name, unsigned index, const NxU32* in, unsigned nin)
	{
	printf("case=%s.%02u in=", name, index);
	nxWords(in, nin);
	printf(" skipped=export_missing\n");
	}

// NxComputeSphereMass: radius[1]:float32_bits, scalar[1]:float32_bits
static const NxU32 nxCases_NxComputeSphereMass[][2] =
	{
	{ 0x3f800000u, 0x3f800000u },
	{ 0x40000000u, 0x3f000000u },
	{ 0x00000000u, 0x3f800000u },
	{ 0x3f800000u, 0x00000000u },
	{ 0xbf800000u, 0x3f800000u },
	{ 0x7f7fffffu, 0x3f800000u },
	{ 0x00000001u, 0x3f800000u },
	{ 0x3f800000u, 0x7f800000u },
	{ 0x7fc00000u, 0x3f800000u },
	};

static unsigned nxRun_NxComputeSphereMass(HMODULE module)
	{
	typedef float (NX_CALL_CONV *Fn)(float, float);
	Fn fn = (Fn) GetProcAddress(module, "NxComputeSphereMass");
	printf("export name=NxComputeSphereMass ordinal=14 present=%d\n", fn ? 1 : 0);
	const unsigned count = sizeof(nxCases_NxComputeSphereMass) / sizeof(nxCases_NxComputeSphereMass[0]);
	for(unsigned i = 0; i < count; ++i)
		{
		const NxU32* in = nxCases_NxComputeSphereMass[i];
		if(!fn)
			{
			nxMissing("NxComputeSphereMass", i, in, 2);
			continue;
			}
		NxU32 s[80];
		NxU32 out[1];
		char ret[16];
		const unsigned kOut = 40;
		nxPoison(s, 0, 80);
		float r = fn(nxF(in[0]), nxF(in[1]));
		nxRetBits(ret, nxU(r));
		nxLine("NxComputeSphereMass", i, in, 2, ret, out, 0);
		}
	return count;
	}

// NxComputeSphereDensity: radius[1]:float32_bits, scalar[1]:float32_bits
static const NxU32 nxCases_NxComputeSphereDensity[][2] =
	{
	{ 0x3f800000u, 0x3f800000u },
	{ 0x40000000u, 0x3f000000u },
	{ 0x00000000u, 0x3f800000u },
	{ 0x3f800000u, 0x00000000u },
	{ 0xbf800000u, 0x3f800000u },
	{ 0x7f7fffffu, 0x3f800000u },
	{ 0x00000001u, 0x3f800000u },
	{ 0x3f800000u, 0x7f800000u },
	{ 0x7fc00000u, 0x3f800000u },
	};

static unsigned nxRun_NxComputeSphereDensity(HMODULE module)
	{
	typedef float (NX_CALL_CONV *Fn)(float, float);
	Fn fn = (Fn) GetProcAddress(module, "NxComputeSphereDensity");
	printf("export name=NxComputeSphereDensity ordinal=12 present=%d\n", fn ? 1 : 0);
	const unsigned count = sizeof(nxCases_NxComputeSphereDensity) / sizeof(nxCases_NxComputeSphereDensity[0]);
	for(unsigned i = 0; i < count; ++i)
		{
		const NxU32* in = nxCases_NxComputeSphereDensity[i];
		if(!fn)
			{
			nxMissing("NxComputeSphereDensity", i, in, 2);
			continue;
			}
		NxU32 s[80];
		NxU32 out[1];
		char ret[16];
		const unsigned kOut = 40;
		nxPoison(s, 0, 80);
		float r = fn(nxF(in[0]), nxF(in[1]));
		nxRetBits(ret, nxU(r));
		nxLine("NxComputeSphereDensity", i, in, 2, ret, out, 0);
		}
	return count;
	}

// NxComputeBoxMass: extents[3]:float32_bits, scalar[1]:float32_bits
static const NxU32 nxCases_NxComputeBoxMass[][4] =
	{
	{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u },
	{ 0x40000000u, 0x40400000u, 0x40800000u, 0x3f000000u },
	{ 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u },
	{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u },
	{ 0xbf800000u, 0x40000000u, 0x40400000u, 0x3f800000u },
	{ 0x7f7fffffu, 0x7f7fffffu, 0x7f7fffffu, 0x3f800000u },
	{ 0x00000001u, 0x3f800000u, 0x3f800000u, 0x3f800000u },
	{ 0x7fc00000u, 0x3f800000u, 0x3f800000u, 0x3f800000u },
	{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x7f800000u },
	};

static unsigned nxRun_NxComputeBoxMass(HMODULE module)
	{
	typedef float (NX_CALL_CONV *Fn)(const float*, float);
	Fn fn = (Fn) GetProcAddress(module, "NxComputeBoxMass");
	printf("export name=NxComputeBoxMass ordinal=5 present=%d\n", fn ? 1 : 0);
	const unsigned count = sizeof(nxCases_NxComputeBoxMass) / sizeof(nxCases_NxComputeBoxMass[0]);
	for(unsigned i = 0; i < count; ++i)
		{
		const NxU32* in = nxCases_NxComputeBoxMass[i];
		if(!fn)
			{
			nxMissing("NxComputeBoxMass", i, in, 4);
			continue;
			}
		NxU32 s[80];
		NxU32 out[1];
		char ret[16];
		const unsigned kOut = 40;
		nxPoison(s, 0, 80);
		nxCopy(s, 0, in, 3);
		float r = fn(nxVec(s, 0), nxF(in[3]));
		nxRetBits(ret, nxU(r));
		nxLine("NxComputeBoxMass", i, in, 4, ret, out, 0);
		}
	return count;
	}

// NxComputeBoxDensity: extents[3]:float32_bits, scalar[1]:float32_bits
static const NxU32 nxCases_NxComputeBoxDensity[][4] =
	{
	{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u },
	{ 0x40000000u, 0x40400000u, 0x40800000u, 0x3f000000u },
	{ 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u },
	{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u },
	{ 0xbf800000u, 0x40000000u, 0x40400000u, 0x3f800000u },
	{ 0x7f7fffffu, 0x7f7fffffu, 0x7f7fffffu, 0x3f800000u },
	{ 0x00000001u, 0x3f800000u, 0x3f800000u, 0x3f800000u },
	{ 0x7fc00000u, 0x3f800000u, 0x3f800000u, 0x3f800000u },
	{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x7f800000u },
	};

static unsigned nxRun_NxComputeBoxDensity(HMODULE module)
	{
	typedef float (NX_CALL_CONV *Fn)(const float*, float);
	Fn fn = (Fn) GetProcAddress(module, "NxComputeBoxDensity");
	printf("export name=NxComputeBoxDensity ordinal=3 present=%d\n", fn ? 1 : 0);
	const unsigned count = sizeof(nxCases_NxComputeBoxDensity) / sizeof(nxCases_NxComputeBoxDensity[0]);
	for(unsigned i = 0; i < count; ++i)
		{
		const NxU32* in = nxCases_NxComputeBoxDensity[i];
		if(!fn)
			{
			nxMissing("NxComputeBoxDensity", i, in, 4);
			continue;
			}
		NxU32 s[80];
		NxU32 out[1];
		char ret[16];
		const unsigned kOut = 40;
		nxPoison(s, 0, 80);
		nxCopy(s, 0, in, 3);
		float r = fn(nxVec(s, 0), nxF(in[3]));
		nxRetBits(ret, nxU(r));
		nxLine("NxComputeBoxDensity", i, in, 4, ret, out, 0);
		}
	return count;
	}

// NxComputeEllipsoidMass: extents[3]:float32_bits, scalar[1]:float32_bits
static const NxU32 nxCases_NxComputeEllipsoidMass[][4] =
	{
	{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u },
	{ 0x40000000u, 0x40400000u, 0x40800000u, 0x3f000000u },
	{ 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u },
	{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u },
	{ 0xbf800000u, 0x40000000u, 0x40400000u, 0x3f800000u },
	{ 0x7f7fffffu, 0x7f7fffffu, 0x7f7fffffu, 0x3f800000u },
	{ 0x00000001u, 0x3f800000u, 0x3f800000u, 0x3f800000u },
	{ 0x7fc00000u, 0x3f800000u, 0x3f800000u, 0x3f800000u },
	{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x7f800000u },
	};

static unsigned nxRun_NxComputeEllipsoidMass(HMODULE module)
	{
	typedef float (NX_CALL_CONV *Fn)(const float*, float);
	Fn fn = (Fn) GetProcAddress(module, "NxComputeEllipsoidMass");
	printf("export name=NxComputeEllipsoidMass ordinal=11 present=%d\n", fn ? 1 : 0);
	const unsigned count = sizeof(nxCases_NxComputeEllipsoidMass) / sizeof(nxCases_NxComputeEllipsoidMass[0]);
	for(unsigned i = 0; i < count; ++i)
		{
		const NxU32* in = nxCases_NxComputeEllipsoidMass[i];
		if(!fn)
			{
			nxMissing("NxComputeEllipsoidMass", i, in, 4);
			continue;
			}
		NxU32 s[80];
		NxU32 out[1];
		char ret[16];
		const unsigned kOut = 40;
		nxPoison(s, 0, 80);
		nxCopy(s, 0, in, 3);
		float r = fn(nxVec(s, 0), nxF(in[3]));
		nxRetBits(ret, nxU(r));
		nxLine("NxComputeEllipsoidMass", i, in, 4, ret, out, 0);
		}
	return count;
	}

// NxComputeEllipsoidDensity: extents[3]:float32_bits, scalar[1]:float32_bits
static const NxU32 nxCases_NxComputeEllipsoidDensity[][4] =
	{
	{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u },
	{ 0x40000000u, 0x40400000u, 0x40800000u, 0x3f000000u },
	{ 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u },
	{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u },
	{ 0xbf800000u, 0x40000000u, 0x40400000u, 0x3f800000u },
	{ 0x7f7fffffu, 0x7f7fffffu, 0x7f7fffffu, 0x3f800000u },
	{ 0x00000001u, 0x3f800000u, 0x3f800000u, 0x3f800000u },
	{ 0x7fc00000u, 0x3f800000u, 0x3f800000u, 0x3f800000u },
	{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x7f800000u },
	};

static unsigned nxRun_NxComputeEllipsoidDensity(HMODULE module)
	{
	typedef float (NX_CALL_CONV *Fn)(const float*, float);
	Fn fn = (Fn) GetProcAddress(module, "NxComputeEllipsoidDensity");
	printf("export name=NxComputeEllipsoidDensity ordinal=10 present=%d\n", fn ? 1 : 0);
	const unsigned count = sizeof(nxCases_NxComputeEllipsoidDensity) / sizeof(nxCases_NxComputeEllipsoidDensity[0]);
	for(unsigned i = 0; i < count; ++i)
		{
		const NxU32* in = nxCases_NxComputeEllipsoidDensity[i];
		if(!fn)
			{
			nxMissing("NxComputeEllipsoidDensity", i, in, 4);
			continue;
			}
		NxU32 s[80];
		NxU32 out[1];
		char ret[16];
		const unsigned kOut = 40;
		nxPoison(s, 0, 80);
		nxCopy(s, 0, in, 3);
		float r = fn(nxVec(s, 0), nxF(in[3]));
		nxRetBits(ret, nxU(r));
		nxLine("NxComputeEllipsoidDensity", i, in, 4, ret, out, 0);
		}
	return count;
	}

// NxComputeCylinderMass: radius[1]:float32_bits, length[1]:float32_bits, scalar[1]:float32_bits
static const NxU32 nxCases_NxComputeCylinderMass[][3] =
	{
	{ 0x3f800000u, 0x40000000u, 0x3f800000u },
	{ 0x3f000000u, 0x40400000u, 0x40000000u },
	{ 0x00000000u, 0x40000000u, 0x3f800000u },
	{ 0x3f800000u, 0x00000000u, 0x3f800000u },
	{ 0x3f800000u, 0x40000000u, 0x00000000u },
	{ 0xbf800000u, 0x40000000u, 0x3f800000u },
	{ 0x7f7fffffu, 0x7f7fffffu, 0x3f800000u },
	{ 0x7fc00000u, 0x40000000u, 0x3f800000u },
	{ 0x00000001u, 0x40000000u, 0x3f800000u },
	};

static unsigned nxRun_NxComputeCylinderMass(HMODULE module)
	{
	typedef float (NX_CALL_CONV *Fn)(float, float, float);
	Fn fn = (Fn) GetProcAddress(module, "NxComputeCylinderMass");
	printf("export name=NxComputeCylinderMass ordinal=9 present=%d\n", fn ? 1 : 0);
	const unsigned count = sizeof(nxCases_NxComputeCylinderMass) / sizeof(nxCases_NxComputeCylinderMass[0]);
	for(unsigned i = 0; i < count; ++i)
		{
		const NxU32* in = nxCases_NxComputeCylinderMass[i];
		if(!fn)
			{
			nxMissing("NxComputeCylinderMass", i, in, 3);
			continue;
			}
		NxU32 s[80];
		NxU32 out[1];
		char ret[16];
		const unsigned kOut = 40;
		nxPoison(s, 0, 80);
		float r = fn(nxF(in[0]), nxF(in[1]), nxF(in[2]));
		nxRetBits(ret, nxU(r));
		nxLine("NxComputeCylinderMass", i, in, 3, ret, out, 0);
		}
	return count;
	}

// NxComputeCylinderDensity: radius[1]:float32_bits, length[1]:float32_bits, scalar[1]:float32_bits
static const NxU32 nxCases_NxComputeCylinderDensity[][3] =
	{
	{ 0x3f800000u, 0x40000000u, 0x3f800000u },
	{ 0x3f000000u, 0x40400000u, 0x40000000u },
	{ 0x00000000u, 0x40000000u, 0x3f800000u },
	{ 0x3f800000u, 0x00000000u, 0x3f800000u },
	{ 0x3f800000u, 0x40000000u, 0x00000000u },
	{ 0xbf800000u, 0x40000000u, 0x3f800000u },
	{ 0x7f7fffffu, 0x7f7fffffu, 0x3f800000u },
	{ 0x7fc00000u, 0x40000000u, 0x3f800000u },
	{ 0x00000001u, 0x40000000u, 0x3f800000u },
	};

static unsigned nxRun_NxComputeCylinderDensity(HMODULE module)
	{
	typedef float (NX_CALL_CONV *Fn)(float, float, float);
	Fn fn = (Fn) GetProcAddress(module, "NxComputeCylinderDensity");
	printf("export name=NxComputeCylinderDensity ordinal=8 present=%d\n", fn ? 1 : 0);
	const unsigned count = sizeof(nxCases_NxComputeCylinderDensity) / sizeof(nxCases_NxComputeCylinderDensity[0]);
	for(unsigned i = 0; i < count; ++i)
		{
		const NxU32* in = nxCases_NxComputeCylinderDensity[i];
		if(!fn)
			{
			nxMissing("NxComputeCylinderDensity", i, in, 3);
			continue;
			}
		NxU32 s[80];
		NxU32 out[1];
		char ret[16];
		const unsigned kOut = 40;
		nxPoison(s, 0, 80);
		float r = fn(nxF(in[0]), nxF(in[1]), nxF(in[2]));
		nxRetBits(ret, nxU(r));
		nxLine("NxComputeCylinderDensity", i, in, 3, ret, out, 0);
		}
	return count;
	}

// NxComputeConeMass: radius[1]:float32_bits, length[1]:float32_bits, scalar[1]:float32_bits
static const NxU32 nxCases_NxComputeConeMass[][3] =
	{
	{ 0x3f800000u, 0x40000000u, 0x3f800000u },
	{ 0x3f000000u, 0x40400000u, 0x40000000u },
	{ 0x00000000u, 0x40000000u, 0x3f800000u },
	{ 0x3f800000u, 0x00000000u, 0x3f800000u },
	{ 0x3f800000u, 0x40000000u, 0x00000000u },
	{ 0xbf800000u, 0x40000000u, 0x3f800000u },
	{ 0x7f7fffffu, 0x7f7fffffu, 0x3f800000u },
	{ 0x7fc00000u, 0x40000000u, 0x3f800000u },
	{ 0x00000001u, 0x40000000u, 0x3f800000u },
	};

static unsigned nxRun_NxComputeConeMass(HMODULE module)
	{
	typedef float (NX_CALL_CONV *Fn)(float, float, float);
	Fn fn = (Fn) GetProcAddress(module, "NxComputeConeMass");
	printf("export name=NxComputeConeMass ordinal=7 present=%d\n", fn ? 1 : 0);
	const unsigned count = sizeof(nxCases_NxComputeConeMass) / sizeof(nxCases_NxComputeConeMass[0]);
	for(unsigned i = 0; i < count; ++i)
		{
		const NxU32* in = nxCases_NxComputeConeMass[i];
		if(!fn)
			{
			nxMissing("NxComputeConeMass", i, in, 3);
			continue;
			}
		NxU32 s[80];
		NxU32 out[1];
		char ret[16];
		const unsigned kOut = 40;
		nxPoison(s, 0, 80);
		float r = fn(nxF(in[0]), nxF(in[1]), nxF(in[2]));
		nxRetBits(ret, nxU(r));
		nxLine("NxComputeConeMass", i, in, 3, ret, out, 0);
		}
	return count;
	}

// NxComputeConeDensity: radius[1]:float32_bits, length[1]:float32_bits, scalar[1]:float32_bits
static const NxU32 nxCases_NxComputeConeDensity[][3] =
	{
	{ 0x3f800000u, 0x40000000u, 0x3f800000u },
	{ 0x3f000000u, 0x40400000u, 0x40000000u },
	{ 0x00000000u, 0x40000000u, 0x3f800000u },
	{ 0x3f800000u, 0x00000000u, 0x3f800000u },
	{ 0x3f800000u, 0x40000000u, 0x00000000u },
	{ 0xbf800000u, 0x40000000u, 0x3f800000u },
	{ 0x7f7fffffu, 0x7f7fffffu, 0x3f800000u },
	{ 0x7fc00000u, 0x40000000u, 0x3f800000u },
	{ 0x00000001u, 0x40000000u, 0x3f800000u },
	};

static unsigned nxRun_NxComputeConeDensity(HMODULE module)
	{
	typedef float (NX_CALL_CONV *Fn)(float, float, float);
	Fn fn = (Fn) GetProcAddress(module, "NxComputeConeDensity");
	printf("export name=NxComputeConeDensity ordinal=6 present=%d\n", fn ? 1 : 0);
	const unsigned count = sizeof(nxCases_NxComputeConeDensity) / sizeof(nxCases_NxComputeConeDensity[0]);
	for(unsigned i = 0; i < count; ++i)
		{
		const NxU32* in = nxCases_NxComputeConeDensity[i];
		if(!fn)
			{
			nxMissing("NxComputeConeDensity", i, in, 3);
			continue;
			}
		NxU32 s[80];
		NxU32 out[1];
		char ret[16];
		const unsigned kOut = 40;
		nxPoison(s, 0, 80);
		float r = fn(nxF(in[0]), nxF(in[1]), nxF(in[2]));
		nxRetBits(ret, nxU(r));
		nxLine("NxComputeConeDensity", i, in, 3, ret, out, 0);
		}
	return count;
	}

// NxComputeBoxInertiaTensor: mass[1]:float32_bits, xlength[1]:float32_bits, ylength[1]:float32_bits, zlength[1]:float32_bits
static const NxU32 nxCases_NxComputeBoxInertiaTensor[][4] =
	{
	{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u },
	{ 0x40200000u, 0x3f800000u, 0x40000000u, 0x40400000u },
	{ 0x00000000u, 0x3f800000u, 0x40000000u, 0x40400000u },
	{ 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x3f800000u, 0xbf800000u, 0x40000000u, 0x40400000u },
	{ 0x7f7fffffu, 0x7f7fffffu, 0x3f800000u, 0x3f800000u },
	{ 0x7fc00000u, 0x3f800000u, 0x3f800000u, 0x3f800000u },
	{ 0x3f800000u, 0x7f800000u, 0x3f800000u, 0x3f800000u },
	};

static unsigned nxRun_NxComputeBoxInertiaTensor(HMODULE module)
	{
	typedef void (NX_CALL_CONV *Fn)(float*, float, float, float, float);
	Fn fn = (Fn) GetProcAddress(module, "NxComputeBoxInertiaTensor");
	printf("export name=NxComputeBoxInertiaTensor ordinal=4 present=%d\n", fn ? 1 : 0);
	const unsigned count = sizeof(nxCases_NxComputeBoxInertiaTensor) / sizeof(nxCases_NxComputeBoxInertiaTensor[0]);
	for(unsigned i = 0; i < count; ++i)
		{
		const NxU32* in = nxCases_NxComputeBoxInertiaTensor[i];
		if(!fn)
			{
			nxMissing("NxComputeBoxInertiaTensor", i, in, 4);
			continue;
			}
		NxU32 s[80];
		NxU32 out[5];
		char ret[16];
		const unsigned kOut = 40;
		nxPoison(s, 0, 80);
		nxPoison(s, kOut, 8);
		fn(nxVecW(s, kOut), nxF(in[0]), nxF(in[1]), nxF(in[2]), nxF(in[3]));
		nxRetVoid(ret);
		nxGather(out, 0, s, kOut, 5);
		nxLine("NxComputeBoxInertiaTensor", i, in, 4, ret, out, 5);
		}
	return count;
	}

// NxComputeSphereInertiaTensor: mass[1]:float32_bits, radius[1]:float32_bits, hollow[1]:integer
static const NxU32 nxCases_NxComputeSphereInertiaTensor[][3] =
	{
	{ 0x3f800000u, 0x3f800000u, 0x00000000u },
	{ 0x3f800000u, 0x3f800000u, 0x00000001u },
	{ 0x40200000u, 0x3f000000u, 0x00000000u },
	{ 0x40200000u, 0x3f000000u, 0x00000001u },
	{ 0x00000000u, 0x3f800000u, 0x00000000u },
	{ 0x3f800000u, 0x00000000u, 0x00000001u },
	{ 0x3f800000u, 0xbf800000u, 0x00000000u },
	{ 0x7fc00000u, 0x3f800000u, 0x00000001u },
	{ 0x7f7fffffu, 0x7f7fffffu, 0x00000000u },
	{ 0x3f800000u, 0x3f800000u, 0x00000002u },
	};

static unsigned nxRun_NxComputeSphereInertiaTensor(HMODULE module)
	{
	typedef void (NX_CALL_CONV *Fn)(float*, float, float, unsigned char);
	Fn fn = (Fn) GetProcAddress(module, "NxComputeSphereInertiaTensor");
	printf("export name=NxComputeSphereInertiaTensor ordinal=13 present=%d\n", fn ? 1 : 0);
	const unsigned count = sizeof(nxCases_NxComputeSphereInertiaTensor) / sizeof(nxCases_NxComputeSphereInertiaTensor[0]);
	for(unsigned i = 0; i < count; ++i)
		{
		const NxU32* in = nxCases_NxComputeSphereInertiaTensor[i];
		if(!fn)
			{
			nxMissing("NxComputeSphereInertiaTensor", i, in, 3);
			continue;
			}
		NxU32 s[80];
		NxU32 out[5];
		char ret[16];
		const unsigned kOut = 40;
		nxPoison(s, 0, 80);
		nxPoison(s, kOut, 8);
		fn(nxVecW(s, kOut), nxF(in[0]), nxF(in[1]), (unsigned char) in[2]);
		nxRetVoid(ret);
		nxGather(out, 0, s, kOut, 5);
		nxLine("NxComputeSphereInertiaTensor", i, in, 3, ret, out, 5);
		}
	return count;
	}

// NxRayPlaneIntersect: ray_orig[3]:float32_bits, ray_dir[3]:float32_bits, plane_normal[3]:float32_bits, plane_d[1]:float32_bits, mode[1]:integer
static const NxU32 nxCases_NxRayPlaneIntersect[][11] =
	{
	{ 0x00000000u, 0x41200000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x00000000u, 0x41200000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x00000000u, 0x41200000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x00000000u, 0x41200000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0xc0000000u, 0x00000000u },
	{ 0x00000000u, 0x41200000u, 0x00000000u, 0x00000000u, 0xc0000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x00000000u, 0x41200000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x00000000u, 0x41200000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0x00000000u, 0x00000000u, 0x40000000u, 0x00000000u, 0xc0800000u, 0x00000000u },
	{ 0x00000000u, 0x41200000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x00000000u, 0x41200000u, 0x00000000u, 0x00000000u, 0x7fc00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x00000000u, 0x41200000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000001u },
	};

static unsigned nxRun_NxRayPlaneIntersect(HMODULE module)
	{
	typedef unsigned char (NX_CALL_CONV *Fn)(const float*, const float*, float*, float*);
	Fn fn = (Fn) GetProcAddress(module, "NxRayPlaneIntersect");
	printf("export name=NxRayPlaneIntersect ordinal=32 present=%d\n", fn ? 1 : 0);
	const unsigned count = sizeof(nxCases_NxRayPlaneIntersect) / sizeof(nxCases_NxRayPlaneIntersect[0]);
	for(unsigned i = 0; i < count; ++i)
		{
		const NxU32* in = nxCases_NxRayPlaneIntersect[i];
		if(!fn)
			{
			nxMissing("NxRayPlaneIntersect", i, in, 11);
			continue;
			}
		NxU32 s[80];
		NxU32 out[8];
		char ret[16];
		const unsigned kOut = 40;
		nxPoison(s, 0, 80);
		nxCopy(s, 0, in, 10);
		nxPoison(s, kOut, 12);
		unsigned pointOff = in[10] == 1 ? 0u : (unsigned) kOut + 4;
		unsigned char r = fn(nxVec(s, 0), nxVec(s, 6), nxVecW(s, kOut), nxVecW(s, pointOff));
		nxRetBits(ret, r);
		nxGather(out, 0, s, kOut, 3);
		nxGather(out, 3, s, pointOff, 5);
		nxLine("NxRayPlaneIntersect", i, in, 11, ret, out, 8);
		}
	return count;
	}

// NxSegmentPlaneIntersect: v1[3]:float32_bits, v2[3]:float32_bits, plane_normal[3]:float32_bits, plane_d[1]:float32_bits, mode[1]:integer
static const NxU32 nxCases_NxSegmentPlaneIntersect[][11] =
	{
	{ 0x00000000u, 0x41200000u, 0x00000000u, 0x00000000u, 0xc1200000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x00000000u, 0x41200000u, 0x00000000u, 0x00000000u, 0x40a00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x00000000u, 0x41200000u, 0x00000000u, 0x41200000u, 0x41200000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x00000000u, 0x00000000u, 0x00000000u, 0x41200000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x00000000u, 0xc1200000u, 0x00000000u, 0x00000000u, 0x41200000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x00000000u, 0x41200000u, 0x00000000u, 0x00000000u, 0x41200000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x00000000u, 0x41200000u, 0x00000000u, 0x00000000u, 0xc1200000u, 0x00000000u, 0x00000000u, 0x40000000u, 0x00000000u, 0xc0800000u, 0x00000000u },
	{ 0x00000000u, 0x41200000u, 0x00000000u, 0x00000000u, 0xc1200000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x00000000u, 0x7fc00000u, 0x00000000u, 0x00000000u, 0xc1200000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x00000000u, 0x41200000u, 0x00000000u, 0x00000000u, 0xc1200000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000001u },
	};

static unsigned nxRun_NxSegmentPlaneIntersect(HMODULE module)
	{
	typedef void (NX_CALL_CONV *Fn)(const float*, const float*, const float*, float*, float*);
	Fn fn = (Fn) GetProcAddress(module, "NxSegmentPlaneIntersect");
	printf("export name=NxSegmentPlaneIntersect ordinal=39 present=%d\n", fn ? 1 : 0);
	const unsigned count = sizeof(nxCases_NxSegmentPlaneIntersect) / sizeof(nxCases_NxSegmentPlaneIntersect[0]);
	for(unsigned i = 0; i < count; ++i)
		{
		const NxU32* in = nxCases_NxSegmentPlaneIntersect[i];
		if(!fn)
			{
			nxMissing("NxSegmentPlaneIntersect", i, in, 11);
			continue;
			}
		NxU32 s[80];
		NxU32 out[8];
		char ret[16];
		const unsigned kOut = 40;
		nxPoison(s, 0, 80);
		nxCopy(s, 0, in, 10);
		nxPoison(s, kOut, 12);
		unsigned pointOff = in[10] == 1 ? 0u : (unsigned) kOut + 4;
		fn(nxVec(s, 0), nxVec(s, 3), nxVec(s, 6), nxVecW(s, kOut), nxVecW(s, pointOff));
		nxRetVoid(ret);
		nxGather(out, 0, s, kOut, 3);
		nxGather(out, 3, s, pointOff, 5);
		nxLine("NxSegmentPlaneIntersect", i, in, 11, ret, out, 8);
		}
	return count;
	}

// NxRaySphereIntersect: origin[3]:float32_bits, dir[3]:float32_bits, center[3]:float32_bits, radius[1]:float32_bits, mode[1]:integer
static const NxU32 nxCases_NxRaySphereIntersect[][11] =
	{
	{ 0x00000000u, 0x00000000u, 0xc0a00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u },
	{ 0x00000000u, 0x40000000u, 0xc0a00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u },
	{ 0x00000000u, 0x3f800000u, 0xc0a00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u },
	{ 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u },
	{ 0x00000000u, 0x00000000u, 0xbf800000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u },
	{ 0x00000000u, 0x00000000u, 0xc0a00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x00000000u, 0x00000000u, 0xc0a00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0x00000000u },
	{ 0x00000000u, 0x00000000u, 0xc0a00000u, 0x00000000u, 0x00000000u, 0x40000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u },
	{ 0x00000000u, 0x00000000u, 0xc0a00000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u },
	{ 0x00000000u, 0x00000000u, 0xc0a00000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u },
	{ 0x00000000u, 0x00000000u, 0xc0a00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000001u },
	{ 0x00000000u, 0x00000000u, 0xc0a00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000002u },
	{ 0x00000000u, 0x00000000u, 0xc0a00000u, 0x00000000u, 0x00000000u, 0x7fc00000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u },
	};

static unsigned nxRun_NxRaySphereIntersect(HMODULE module)
	{
	typedef unsigned char (NX_CALL_CONV *Fn)(const float*, const float*, const float*, float, float*);
	Fn fn = (Fn) GetProcAddress(module, "NxRaySphereIntersect");
	printf("export name=NxRaySphereIntersect ordinal=33 present=%d\n", fn ? 1 : 0);
	const unsigned count = sizeof(nxCases_NxRaySphereIntersect) / sizeof(nxCases_NxRaySphereIntersect[0]);
	for(unsigned i = 0; i < count; ++i)
		{
		const NxU32* in = nxCases_NxRaySphereIntersect[i];
		if(!fn)
			{
			nxMissing("NxRaySphereIntersect", i, in, 11);
			continue;
			}
		NxU32 s[80];
		NxU32 out[5];
		char ret[16];
		const unsigned kOut = 40;
		nxPoison(s, 0, 80);
		nxCopy(s, 0, in, 10);
		nxPoison(s, kOut, 8);
		unsigned coordOff = in[10] == 2 ? 0u : (unsigned) kOut;
		float* coord = in[10] == 1 ? 0 : nxVecW(s, coordOff);
		unsigned char r = fn(nxVec(s, 0), nxVec(s, 3), nxVec(s, 6), nxF(in[9]), coord);
		nxRetBits(ret, r);
		nxGather(out, 0, s, coordOff, 5);
		nxLine("NxRaySphereIntersect", i, in, 11, ret, out, 5);
		}
	return count;
	}

// NxRayTriIntersect: orig[3]:float32_bits, dir[3]:float32_bits, vert0[3]:float32_bits, vert1[3]:float32_bits, vert2[3]:float32_bits, cull[1]:integer, mode[1]:integer
static const NxU32 nxCases_NxRayTriIntersect[][17] =
	{
	{ 0x3f800000u, 0x3f800000u, 0x40a00000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000001u, 0x00000000u },
	{ 0x3f800000u, 0x3f800000u, 0xc0a00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000001u, 0x00000000u },
	{ 0x3f800000u, 0x3f800000u, 0xc0a00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x41200000u, 0x41200000u, 0x40a00000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000001u, 0x00000000u },
	{ 0x00000000u, 0x40000000u, 0x40a00000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000001u, 0x00000000u },
	{ 0x00000000u, 0x00000000u, 0x40a00000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000001u, 0x00000000u },
	{ 0x00000000u, 0x00000000u, 0x40a00000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000001u, 0x00000000u },
	{ 0x3f800000u, 0x3f800000u, 0x40a00000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000001u, 0x00000000u },
	{ 0x3f800000u, 0x3f800000u, 0x40a00000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000001u, 0x00000000u },
	{ 0x3f800000u, 0x3f800000u, 0x40a00000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000001u, 0x00000000u },
	{ 0x3f800000u, 0x3f800000u, 0x40a00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000001u, 0x00000000u },
	{ 0x3f800000u, 0x3f800000u, 0x40a00000u, 0x00000000u, 0x00000000u, 0xc0000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000001u, 0x00000000u },
	{ 0x3f800000u, 0x3f800000u, 0xc0a00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000002u, 0x00000000u },
	{ 0x3f800000u, 0x3f800000u, 0x40a00000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000001u, 0x00000001u },
	{ 0x3f800000u, 0x3f800000u, 0x40a00000u, 0x00000000u, 0x00000000u, 0x7fc00000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000001u, 0x00000000u },
	};

static unsigned nxRun_NxRayTriIntersect(HMODULE module)
	{
	typedef unsigned char (NX_CALL_CONV *Fn)(const float*, const float*, const float*, const float*, const float*, float*, float*, float*, unsigned char);
	Fn fn = (Fn) GetProcAddress(module, "NxRayTriIntersect");
	printf("export name=NxRayTriIntersect ordinal=34 present=%d\n", fn ? 1 : 0);
	const unsigned count = sizeof(nxCases_NxRayTriIntersect) / sizeof(nxCases_NxRayTriIntersect[0]);
	for(unsigned i = 0; i < count; ++i)
		{
		const NxU32* in = nxCases_NxRayTriIntersect[i];
		if(!fn)
			{
			nxMissing("NxRayTriIntersect", i, in, 17);
			continue;
			}
		NxU32 s[80];
		NxU32 out[9];
		char ret[16];
		const unsigned kOut = 40;
		nxPoison(s, 0, 80);
		nxCopy(s, 0, in, 15);
		nxPoison(s, kOut, 16);
		unsigned uOff = in[16] == 1 ? (unsigned) kOut : (unsigned) kOut + 4;
		unsigned char r = fn(nxVec(s, 0), nxVec(s, 3), nxVec(s, 6), nxVec(s, 9), nxVec(s, 12),
			nxVecW(s, kOut), nxVecW(s, uOff), nxVecW(s, kOut + 8), (unsigned char) in[15]);
		nxRetBits(ret, r);
		nxGather(out, 0, s, kOut, 3);
		nxGather(out, 3, s, uOff, 3);
		nxGather(out, 6, s, kOut + 8, 3);
		nxLine("NxRayTriIntersect", i, in, 17, ret, out, 9);
		}
	return count;
	}

// NxRayAABBIntersect: min[3]:float32_bits, max[3]:float32_bits, origin[3]:float32_bits, dir[3]:float32_bits, mode[1]:integer
static const NxU32 nxCases_NxRayAABBIntersect[][13] =
	{
	{ 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0xc0a00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0xc0a00000u, 0x41200000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0xc0a00000u, 0x3f800000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0xc0a00000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0xc0a00000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0xbf800000u, 0xbf800000u, 0xbf800000u, 0xc0a00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0xc0a00000u, 0x00000000u, 0x00000000u, 0x40000000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0xc0a00000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0xc0a00000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0xc0a00000u, 0xc0a00000u, 0xc0a00000u, 0x3f13cd3au, 0x3f13cd3au, 0x3f13cd3au, 0x00000000u },
	{ 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0xc0a00000u, 0x00000000u, 0x00000000u, 0x7fc00000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0xc0a00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000001u },
	};

static unsigned nxRun_NxRayAABBIntersect(HMODULE module)
	{
	typedef unsigned char (NX_CALL_CONV *Fn)(const float*, const float*, const float*, const float*, float*);
	Fn fn = (Fn) GetProcAddress(module, "NxRayAABBIntersect");
	printf("export name=NxRayAABBIntersect ordinal=28 present=%d\n", fn ? 1 : 0);
	const unsigned count = sizeof(nxCases_NxRayAABBIntersect) / sizeof(nxCases_NxRayAABBIntersect[0]);
	for(unsigned i = 0; i < count; ++i)
		{
		const NxU32* in = nxCases_NxRayAABBIntersect[i];
		if(!fn)
			{
			nxMissing("NxRayAABBIntersect", i, in, 13);
			continue;
			}
		NxU32 s[80];
		NxU32 out[5];
		char ret[16];
		const unsigned kOut = 40;
		nxPoison(s, 0, 80);
		nxCopy(s, 0, in, 12);
		nxPoison(s, kOut, 8);
		unsigned coordOff = in[12] == 1 ? 6u : (unsigned) kOut;
		unsigned char r = fn(nxVec(s, 0), nxVec(s, 3), nxVec(s, 6), nxVec(s, 9), nxVecW(s, coordOff));
		nxRetBits(ret, r);
		nxGather(out, 0, s, coordOff, 5);
		nxLine("NxRayAABBIntersect", i, in, 13, ret, out, 5);
		}
	return count;
	}

// NxRayAABBIntersect2: min[3]:float32_bits, max[3]:float32_bits, origin[3]:float32_bits, dir[3]:float32_bits, mode[1]:integer
static const NxU32 nxCases_NxRayAABBIntersect2[][13] =
	{
	{ 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0xc0a00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0xc0a00000u, 0x41200000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0xc0a00000u, 0x3f800000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0xc0a00000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0xc0a00000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0xbf800000u, 0xbf800000u, 0xbf800000u, 0xc0a00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0xc0a00000u, 0x00000000u, 0x00000000u, 0x40000000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0xc0a00000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0xc0a00000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0xc0a00000u, 0xc0a00000u, 0xc0a00000u, 0x3f13cd3au, 0x3f13cd3au, 0x3f13cd3au, 0x00000000u },
	{ 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0xc0a00000u, 0x00000000u, 0x00000000u, 0x7fc00000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0xc0a00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000001u },
	};

static unsigned nxRun_NxRayAABBIntersect2(HMODULE module)
	{
	typedef NxU32 (NX_CALL_CONV *Fn)(const float*, const float*, const float*, const float*, float*, float*);
	Fn fn = (Fn) GetProcAddress(module, "NxRayAABBIntersect2");
	printf("export name=NxRayAABBIntersect2 ordinal=29 present=%d\n", fn ? 1 : 0);
	const unsigned count = sizeof(nxCases_NxRayAABBIntersect2) / sizeof(nxCases_NxRayAABBIntersect2[0]);
	for(unsigned i = 0; i < count; ++i)
		{
		const NxU32* in = nxCases_NxRayAABBIntersect2[i];
		if(!fn)
			{
			nxMissing("NxRayAABBIntersect2", i, in, 13);
			continue;
			}
		NxU32 s[80];
		NxU32 out[8];
		char ret[16];
		const unsigned kOut = 40;
		nxPoison(s, 0, 80);
		nxCopy(s, 0, in, 12);
		nxPoison(s, kOut, 16);
		unsigned coordOff = in[12] == 1 ? 6u : (unsigned) kOut;
		NxU32 r = fn(nxVec(s, 0), nxVec(s, 3), nxVec(s, 6), nxVec(s, 9),
			nxVecW(s, coordOff), nxVecW(s, kOut + 8));
		nxRetBits(ret, r);
		nxGather(out, 0, s, coordOff, 5);
		nxGather(out, 5, s, kOut + 8, 3);
		nxLine("NxRayAABBIntersect2", i, in, 13, ret, out, 8);
		}
	return count;
	}

// NxSegmentBoxIntersect: p1[3]:float32_bits, p2[3]:float32_bits, bbox_min[3]:float32_bits, bbox_max[3]:float32_bits, mode[1]:integer
static const NxU32 nxCases_NxSegmentBoxIntersect[][13] =
	{
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0x40a00000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u },
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0xc0400000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u },
	{ 0xc0a00000u, 0x41200000u, 0x00000000u, 0x40a00000u, 0x41200000u, 0x00000000u, 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u },
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u },
	{ 0xbf000000u, 0x00000000u, 0x00000000u, 0x3f000000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u },
	{ 0xc0a00000u, 0x3f800000u, 0x00000000u, 0x40a00000u, 0x3f800000u, 0x00000000u, 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u },
	{ 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u },
	{ 0x40a00000u, 0x00000000u, 0x00000000u, 0x40a00000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u },
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0x40a00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x00000000u },
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0x40a00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u },
	{ 0x40a00000u, 0x00000000u, 0x00000000u, 0xc0a00000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u },
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0x7fc00000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u },
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0x40a00000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000001u },
	};

static unsigned nxRun_NxSegmentBoxIntersect(HMODULE module)
	{
	typedef unsigned char (NX_CALL_CONV *Fn)(const float*, const float*, const float*, const float*, float*);
	Fn fn = (Fn) GetProcAddress(module, "NxSegmentBoxIntersect");
	printf("export name=NxSegmentBoxIntersect ordinal=37 present=%d\n", fn ? 1 : 0);
	const unsigned count = sizeof(nxCases_NxSegmentBoxIntersect) / sizeof(nxCases_NxSegmentBoxIntersect[0]);
	for(unsigned i = 0; i < count; ++i)
		{
		const NxU32* in = nxCases_NxSegmentBoxIntersect[i];
		if(!fn)
			{
			nxMissing("NxSegmentBoxIntersect", i, in, 13);
			continue;
			}
		NxU32 s[80];
		NxU32 out[5];
		char ret[16];
		const unsigned kOut = 40;
		nxPoison(s, 0, 80);
		nxCopy(s, 0, in, 12);
		nxPoison(s, kOut, 8);
		unsigned interceptOff = in[12] == 1 ? 0u : (unsigned) kOut;
		unsigned char r = fn(nxVec(s, 0), nxVec(s, 3), nxVec(s, 6), nxVec(s, 9), nxVecW(s, interceptOff));
		nxRetBits(ret, r);
		nxGather(out, 0, s, interceptOff, 5);
		nxLine("NxSegmentBoxIntersect", i, in, 13, ret, out, 5);
		}
	return count;
	}

// NxSegmentAABBIntersect: p0[3]:float32_bits, p1[3]:float32_bits, min[3]:float32_bits, max[3]:float32_bits
static const NxU32 nxCases_NxSegmentAABBIntersect[][12] =
	{
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0x40a00000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u },
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0xc0400000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u },
	{ 0xc0a00000u, 0x41200000u, 0x00000000u, 0x40a00000u, 0x41200000u, 0x00000000u, 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u },
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u },
	{ 0xbf000000u, 0x00000000u, 0x00000000u, 0x3f000000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u },
	{ 0xc0a00000u, 0x3f800000u, 0x00000000u, 0x40a00000u, 0x3f800000u, 0x00000000u, 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u },
	{ 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u },
	{ 0x40a00000u, 0x00000000u, 0x00000000u, 0x40a00000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u },
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0x40a00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0xbf800000u, 0xbf800000u, 0xbf800000u },
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0x40a00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u },
	{ 0x40a00000u, 0x00000000u, 0x00000000u, 0xc0a00000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u },
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0x7fc00000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u },
	};

static unsigned nxRun_NxSegmentAABBIntersect(HMODULE module)
	{
	typedef unsigned char (NX_CALL_CONV *Fn)(const float*, const float*, const float*, const float*);
	Fn fn = (Fn) GetProcAddress(module, "NxSegmentAABBIntersect");
	printf("export name=NxSegmentAABBIntersect ordinal=36 present=%d\n", fn ? 1 : 0);
	const unsigned count = sizeof(nxCases_NxSegmentAABBIntersect) / sizeof(nxCases_NxSegmentAABBIntersect[0]);
	for(unsigned i = 0; i < count; ++i)
		{
		const NxU32* in = nxCases_NxSegmentAABBIntersect[i];
		if(!fn)
			{
			nxMissing("NxSegmentAABBIntersect", i, in, 12);
			continue;
			}
		NxU32 s[80];
		NxU32 out[1];
		char ret[16];
		const unsigned kOut = 40;
		nxPoison(s, 0, 80);
		nxCopy(s, 0, in, 12);
		unsigned char r = fn(nxVec(s, 0), nxVec(s, 3), nxVec(s, 6), nxVec(s, 9));
		nxRetBits(ret, r);
		nxLine("NxSegmentAABBIntersect", i, in, 12, ret, out, 0);
		}
	return count;
	}

// NxRayOBBIntersect: ray_orig[3]:float32_bits, ray_dir[3]:float32_bits, center[3]:float32_bits, extents[3]:float32_bits, rot[9]:float32_bits
static const NxU32 nxCases_NxRayOBBIntersect[][21] =
	{
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u },
	{ 0xc0a00000u, 0x41200000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u },
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f3504f3u, 0xbf3504f3u, 0x00000000u, 0x3f3504f3u, 0x3f3504f3u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u },
	{ 0xc0a00000u, 0x3f800000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u },
	{ 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u },
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u },
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u },
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x40000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40000000u },
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u },
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u },
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0x40000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u },
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x7fc00000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u },
	};

static unsigned nxRun_NxRayOBBIntersect(HMODULE module)
	{
	typedef unsigned char (NX_CALL_CONV *Fn)(const float*, const float*, const float*, const float*);
	Fn fn = (Fn) GetProcAddress(module, "NxRayOBBIntersect");
	printf("export name=NxRayOBBIntersect ordinal=31 present=%d\n", fn ? 1 : 0);
	const unsigned count = sizeof(nxCases_NxRayOBBIntersect) / sizeof(nxCases_NxRayOBBIntersect[0]);
	for(unsigned i = 0; i < count; ++i)
		{
		const NxU32* in = nxCases_NxRayOBBIntersect[i];
		if(!fn)
			{
			nxMissing("NxRayOBBIntersect", i, in, 21);
			continue;
			}
		NxU32 s[80];
		NxU32 out[1];
		char ret[16];
		const unsigned kOut = 40;
		nxPoison(s, 0, 80);
		nxCopy(s, 0, in, 21);
		unsigned char r = fn(nxVec(s, 0), nxVec(s, 6), nxVec(s, 9), nxVec(s, 12));
		nxRetBits(ret, r);
		nxLine("NxRayOBBIntersect", i, in, 21, ret, out, 0);
		}
	return count;
	}

// NxSegmentOBBIntersect: p0[3]:float32_bits, p1[3]:float32_bits, center[3]:float32_bits, extents[3]:float32_bits, rot[9]:float32_bits
static const NxU32 nxCases_NxSegmentOBBIntersect[][21] =
	{
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0x40a00000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u },
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0xc0400000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u },
	{ 0xc0a00000u, 0x41200000u, 0x00000000u, 0x40a00000u, 0x41200000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u },
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0x40a00000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f3504f3u, 0xbf3504f3u, 0x00000000u, 0x3f3504f3u, 0x3f3504f3u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u },
	{ 0xc0a00000u, 0x3f800000u, 0x00000000u, 0x40a00000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u },
	{ 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u },
	{ 0x40a00000u, 0x00000000u, 0x00000000u, 0x40a00000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u },
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0x40a00000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u },
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0x40a00000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u },
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0x40a00000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x40000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40000000u },
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0x40a00000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x40a00000u, 0x00000000u, 0x00000000u, 0xc0a00000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u },
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0x7fc00000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u },
	};

static unsigned nxRun_NxSegmentOBBIntersect(HMODULE module)
	{
	typedef unsigned char (NX_CALL_CONV *Fn)(const float*, const float*, const float*, const float*, const float*);
	Fn fn = (Fn) GetProcAddress(module, "NxSegmentOBBIntersect");
	printf("export name=NxSegmentOBBIntersect ordinal=38 present=%d\n", fn ? 1 : 0);
	const unsigned count = sizeof(nxCases_NxSegmentOBBIntersect) / sizeof(nxCases_NxSegmentOBBIntersect[0]);
	for(unsigned i = 0; i < count; ++i)
		{
		const NxU32* in = nxCases_NxSegmentOBBIntersect[i];
		if(!fn)
			{
			nxMissing("NxSegmentOBBIntersect", i, in, 21);
			continue;
			}
		NxU32 s[80];
		NxU32 out[1];
		char ret[16];
		const unsigned kOut = 40;
		nxPoison(s, 0, 80);
		nxCopy(s, 0, in, 21);
		unsigned char r = fn(nxVec(s, 0), nxVec(s, 3), nxVec(s, 6), nxVec(s, 9), nxVec(s, 12));
		nxRetBits(ret, r);
		nxLine("NxSegmentOBBIntersect", i, in, 21, ret, out, 0);
		}
	return count;
	}

// NxRayCapsuleIntersect: origin[3]:float32_bits, dir[3]:float32_bits, capsule_p0[3]:float32_bits, capsule_p1[3]:float32_bits, capsule_radius[1]:float32_bits, mode[1]:integer
static const NxU32 nxCases_NxRayCapsuleIntersect[][14] =
	{
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0xc0000000u, 0x00000000u, 0x00000000u, 0x40000000u, 0x00000000u, 0x3f800000u, 0x00000000u },
	{ 0xc0a00000u, 0x3f800000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0xc0000000u, 0x00000000u, 0x00000000u, 0x40000000u, 0x00000000u, 0x3f800000u, 0x00000000u },
	{ 0xc0a00000u, 0x41200000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0xc0000000u, 0x00000000u, 0x00000000u, 0x40000000u, 0x00000000u, 0x3f800000u, 0x00000000u },
	{ 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0xc0000000u, 0x00000000u, 0x00000000u, 0x40000000u, 0x00000000u, 0x3f800000u, 0x00000000u },
	{ 0x00000000u, 0x40a00000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0x00000000u, 0x00000000u, 0xc0000000u, 0x00000000u, 0x00000000u, 0x40000000u, 0x00000000u, 0x3f800000u, 0x00000000u },
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0xc0000000u, 0x00000000u, 0x00000000u, 0x40000000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u },
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0xc0000000u, 0x00000000u, 0x00000000u, 0x40000000u, 0x00000000u, 0x3f800000u, 0x00000000u },
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0xc0000000u, 0x00000000u, 0x00000000u, 0x40000000u, 0x00000000u, 0x3f800000u, 0x00000000u },
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0x40000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0xc0000000u, 0x00000000u, 0x00000000u, 0x40000000u, 0x00000000u, 0x3f800000u, 0x00000000u },
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0xc0000000u, 0x00000000u, 0x00000000u, 0x40000000u, 0x00000000u, 0xbf800000u, 0x00000000u },
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0x7fc00000u, 0x00000000u, 0x00000000u, 0x00000000u, 0xc0000000u, 0x00000000u, 0x00000000u, 0x40000000u, 0x00000000u, 0x3f800000u, 0x00000000u },
	{ 0xc0a00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0xc0000000u, 0x00000000u, 0x00000000u, 0x40000000u, 0x00000000u, 0x3f800000u, 0x00000001u },
	};

static unsigned nxRun_NxRayCapsuleIntersect(HMODULE module)
	{
	typedef NxU32 (NX_CALL_CONV *Fn)(const float*, const float*, const float*, float*);
	Fn fn = (Fn) GetProcAddress(module, "NxRayCapsuleIntersect");
	printf("export name=NxRayCapsuleIntersect ordinal=30 present=%d\n", fn ? 1 : 0);
	const unsigned count = sizeof(nxCases_NxRayCapsuleIntersect) / sizeof(nxCases_NxRayCapsuleIntersect[0]);
	for(unsigned i = 0; i < count; ++i)
		{
		const NxU32* in = nxCases_NxRayCapsuleIntersect[i];
		if(!fn)
			{
			nxMissing("NxRayCapsuleIntersect", i, in, 14);
			continue;
			}
		NxU32 s[80];
		NxU32 out[4];
		char ret[16];
		const unsigned kOut = 40;
		nxPoison(s, 0, 80);
		nxCopy(s, 0, in, 13);
		nxPoison(s, kOut, 8);
		unsigned tOff = in[13] == 1 ? 0u : (unsigned) kOut;
		NxU32 r = fn(nxVec(s, 0), nxVec(s, 3), nxVec(s, 6), nxVecW(s, tOff));
		nxRetBits(ret, r);
		nxGather(out, 0, s, tOff, 4);
		nxLine("NxRayCapsuleIntersect", i, in, 14, ret, out, 4);
		}
	return count;
	}

// NxSweptSpheresIntersect: sphere0_center[3]:float32_bits, sphere0_radius[1]:float32_bits, velocity0[3]:float32_bits, sphere1_center[3]:float32_bits, sphere1_radius[1]:float32_bits, velocity1[3]:float32_bits
static const NxU32 nxCases_NxSweptSpheresIntersect[][14] =
	{
	{ 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x41200000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0xbf800000u, 0x00000000u, 0x00000000u },
	{ 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0xbf800000u, 0x00000000u, 0x00000000u, 0x41200000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u },
	{ 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x41200000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x41200000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u },
	{ 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x41200000u, 0x00000000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0x00000000u, 0x00000000u },
	{ 0x00000000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x41200000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0xbf800000u, 0x00000000u, 0x00000000u },
	{ 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x41200000u, 0x41200000u, 0x00000000u, 0x3f800000u, 0xbf800000u, 0xbf800000u, 0x00000000u },
	{ 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3fc00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x7fc00000u, 0x00000000u, 0x00000000u, 0x41200000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0xbf800000u, 0x00000000u, 0x00000000u },
	};

static unsigned nxRun_NxSweptSpheresIntersect(HMODULE module)
	{
	typedef unsigned char (NX_CALL_CONV *Fn)(const float*, const float*, const float*, const float*);
	Fn fn = (Fn) GetProcAddress(module, "NxSweptSpheresIntersect");
	printf("export name=NxSweptSpheresIntersect ordinal=41 present=%d\n", fn ? 1 : 0);
	const unsigned count = sizeof(nxCases_NxSweptSpheresIntersect) / sizeof(nxCases_NxSweptSpheresIntersect[0]);
	for(unsigned i = 0; i < count; ++i)
		{
		const NxU32* in = nxCases_NxSweptSpheresIntersect[i];
		if(!fn)
			{
			nxMissing("NxSweptSpheresIntersect", i, in, 14);
			continue;
			}
		NxU32 s[80];
		NxU32 out[1];
		char ret[16];
		const unsigned kOut = 40;
		nxPoison(s, 0, 80);
		nxCopy(s, 0, in, 14);
		unsigned char r = fn(nxVec(s, 0), nxVec(s, 4), nxVec(s, 7), nxVec(s, 11));
		nxRetBits(ret, r);
		nxLine("NxSweptSpheresIntersect", i, in, 14, ret, out, 0);
		}
	return count;
	}

// NxBoxBoxIntersect: extents0[3]:float32_bits, center0[3]:float32_bits, rotation0[9]:float32_bits, extents1[3]:float32_bits, center1[3]:float32_bits, rotation1[9]:float32_bits, fullTest[1]:integer
static const NxU32 nxCases_NxBoxBoxIntersect[][31] =
	{
	{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000001u },
	{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x40a00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000001u },
	{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x40000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000001u },
	{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000001u },
	{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x4019999au, 0x00000000u, 0x00000000u, 0x3f3504f3u, 0xbf3504f3u, 0x00000000u, 0x3f3504f3u, 0x3f3504f3u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000001u },
	{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x4019999au, 0x00000000u, 0x00000000u, 0x3f3504f3u, 0xbf3504f3u, 0x00000000u, 0x3f3504f3u, 0x3f3504f3u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u },
	{ 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000001u },
	{ 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000001u },
	{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x40400000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000001u },
	{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000001u },
	{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x7fc00000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000001u },
	{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x4019999au, 0x00000000u, 0x00000000u, 0x3f3504f3u, 0xbf3504f3u, 0x00000000u, 0x3f3504f3u, 0x3f3504f3u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000002u },
	};

static unsigned nxRun_NxBoxBoxIntersect(HMODULE module)
	{
	typedef unsigned char (NX_CALL_CONV *Fn)(const float*, const float*, const float*, const float*, const float*, const float*, unsigned char);
	Fn fn = (Fn) GetProcAddress(module, "NxBoxBoxIntersect");
	printf("export name=NxBoxBoxIntersect ordinal=1 present=%d\n", fn ? 1 : 0);
	const unsigned count = sizeof(nxCases_NxBoxBoxIntersect) / sizeof(nxCases_NxBoxBoxIntersect[0]);
	for(unsigned i = 0; i < count; ++i)
		{
		const NxU32* in = nxCases_NxBoxBoxIntersect[i];
		if(!fn)
			{
			nxMissing("NxBoxBoxIntersect", i, in, 31);
			continue;
			}
		NxU32 s[80];
		NxU32 out[1];
		char ret[16];
		const unsigned kOut = 40;
		nxPoison(s, 0, 80);
		nxCopy(s, 0, in, 30);
		unsigned char r = fn(nxVec(s, 0), nxVec(s, 3), nxVec(s, 6),
			nxVec(s, 15), nxVec(s, 18), nxVec(s, 21), (unsigned char) in[30]);
		nxRetBits(ret, r);
		nxLine("NxBoxBoxIntersect", i, in, 31, ret, out, 0);
		}
	return count;
	}

// NxSeparatingAxis: extents0[3]:float32_bits, center0[3]:float32_bits, rotation0[9]:float32_bits, extents1[3]:float32_bits, center1[3]:float32_bits, rotation1[9]:float32_bits, fullTest[1]:integer
static const NxU32 nxCases_NxSeparatingAxis[][31] =
	{
	{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000001u },
	{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x40a00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000001u },
	{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x40000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000001u },
	{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000001u },
	{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x4019999au, 0x00000000u, 0x00000000u, 0x3f3504f3u, 0xbf3504f3u, 0x00000000u, 0x3f3504f3u, 0x3f3504f3u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000001u },
	{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x4019999au, 0x00000000u, 0x00000000u, 0x3f3504f3u, 0xbf3504f3u, 0x00000000u, 0x3f3504f3u, 0x3f3504f3u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u },
	{ 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000001u },
	{ 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000001u },
	{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x40400000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000001u },
	{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000001u },
	{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x7fc00000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000001u },
	{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x4019999au, 0x00000000u, 0x00000000u, 0x3f3504f3u, 0xbf3504f3u, 0x00000000u, 0x3f3504f3u, 0x3f3504f3u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000002u },
	};

static unsigned nxRun_NxSeparatingAxis(HMODULE module)
	{
	typedef int (NX_CALL_CONV *Fn)(const float*, const float*, const float*, const float*, const float*, const float*, unsigned char);
	Fn fn = (Fn) GetProcAddress(module, "NxSeparatingAxis");
	printf("export name=NxSeparatingAxis ordinal=40 present=%d\n", fn ? 1 : 0);
	const unsigned count = sizeof(nxCases_NxSeparatingAxis) / sizeof(nxCases_NxSeparatingAxis[0]);
	for(unsigned i = 0; i < count; ++i)
		{
		const NxU32* in = nxCases_NxSeparatingAxis[i];
		if(!fn)
			{
			nxMissing("NxSeparatingAxis", i, in, 31);
			continue;
			}
		NxU32 s[80];
		NxU32 out[1];
		char ret[16];
		const unsigned kOut = 40;
		nxPoison(s, 0, 80);
		nxCopy(s, 0, in, 30);
		NxU32 r = (NxU32) fn(nxVec(s, 0), nxVec(s, 3), nxVec(s, 6),
			nxVec(s, 15), nxVec(s, 18), nxVec(s, 21), (unsigned char) in[30]);
		nxRetBits(ret, r);
		nxLine("NxSeparatingAxis", i, in, 31, ret, out, 0);
		}
	return count;
	}

// NxBuildSmoothNormals: nbTris[1]:integer, nbVerts[1]:integer, index_kind[1]:integer, flip[1]:integer, mode[1]:integer, verts[24]:float32_bits, faces[36]:integer
static const NxU32 nxCases_NxBuildSmoothNormals[][65] =
	{
	{ 0x00000001u, 0x00000003u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000001u, 0x00000002u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x00000001u, 0x00000003u, 0x00000000u, 0x00000001u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000001u, 0x00000002u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x00000001u, 0x00000003u, 0x00000001u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000001u, 0x00000002u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x00000002u, 0x00000004u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000001u, 0x00000002u, 0x00000000u, 0x00000002u, 0x00000003u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x0000000cu, 0x00000008u, 0x00000000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000002u, 0x00000001u, 0x00000000u, 0x00000003u, 0x00000002u, 0x00000004u, 0x00000005u, 0x00000006u, 0x00000004u, 0x00000006u, 0x00000007u, 0x00000000u, 0x00000001u, 0x00000005u, 0x00000000u, 0x00000005u, 0x00000004u, 0x00000002u, 0x00000003u, 0x00000007u, 0x00000002u, 0x00000007u, 0x00000006u, 0x00000000u, 0x00000004u, 0x00000007u, 0x00000000u, 0x00000007u, 0x00000003u, 0x00000001u, 0x00000002u, 0x00000006u, 0x00000001u, 0x00000006u, 0x00000005u },
	{ 0x0000000cu, 0x00000008u, 0x00000001u, 0x00000001u, 0x00000000u, 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0xbf800000u, 0xbf800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0xbf800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000002u, 0x00000001u, 0x00000000u, 0x00000003u, 0x00000002u, 0x00000004u, 0x00000005u, 0x00000006u, 0x00000004u, 0x00000006u, 0x00000007u, 0x00000000u, 0x00000001u, 0x00000005u, 0x00000000u, 0x00000005u, 0x00000004u, 0x00000002u, 0x00000003u, 0x00000007u, 0x00000002u, 0x00000007u, 0x00000006u, 0x00000000u, 0x00000004u, 0x00000007u, 0x00000000u, 0x00000007u, 0x00000003u, 0x00000001u, 0x00000002u, 0x00000006u, 0x00000001u, 0x00000006u, 0x00000005u },
	{ 0x00000000u, 0x00000003u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000001u, 0x00000002u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x00000001u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000001u, 0x00000002u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x00000001u, 0x00000003u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000001u, 0x00000002u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x00000001u, 0x00000003u, 0x00000002u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000001u, 0x00000002u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x00000001u, 0x00000003u, 0x00000003u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000001u, 0x00000002u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x00000001u, 0x00000003u, 0x00000000u, 0x00000000u, 0x00000001u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000001u, 0x00000002u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x00000001u, 0x00000003u, 0x00000000u, 0x00000002u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000001u, 0x00000002u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x00000002u, 0x00000006u, 0x00000002u, 0x00000001u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x40800000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x40800000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000001u, 0x00000002u, 0x00000003u, 0x00000004u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x00000002u, 0x00000006u, 0x00000000u, 0x00000001u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x40800000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x40800000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000001u, 0x00000002u, 0x00000003u, 0x00000004u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x00000002u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000001u, 0x00000002u, 0x00000000u, 0x00000003u, 0x00000004u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
	{ 0x00000002u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000001u, 0x00000002u, 0x00000000u, 0x00000003u, 0x00000004u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
	};

static unsigned nxRun_NxBuildSmoothNormals(HMODULE module)
	{
	typedef unsigned char (NX_CALL_CONV *Fn)(NxU32, NxU32, const float*, const NxU32*, const NxU16*, float*, unsigned char);
	Fn fn = (Fn) GetProcAddress(module, "NxBuildSmoothNormals");
	printf("export name=NxBuildSmoothNormals ordinal=2 present=%d\n", fn ? 1 : 0);
	const unsigned count = sizeof(nxCases_NxBuildSmoothNormals) / sizeof(nxCases_NxBuildSmoothNormals[0]);
	for(unsigned i = 0; i < count; ++i)
		{
		const NxU32* in = nxCases_NxBuildSmoothNormals[i];
		if(!fn)
			{
			nxMissing("NxBuildSmoothNormals", i, in, 65);
			continue;
			}
		NxU32 s[140];
		NxU32 out[26];
		char ret[16];
		const unsigned kOut = 100;
		nxPoison(s, 0, 140);
		nxCopy(s, 0, in + 5, 24);
		NxU16 wFaceData[36];
		for(unsigned j = 0; j < 36; ++j)
			wFaceData[j] = (NxU16) in[29 + j];
		nxPoison(s, kOut, 32);
		unsigned normalsOff = in[4] == 1 ? 0u : (unsigned) kOut;
		const NxU32* dFaces = (in[2] == 0 || in[2] == 3) ? in + 29 : 0;
		const NxU16* wFaces = (in[2] == 1 || in[2] == 3) ? wFaceData : 0;
		unsigned char r = fn(in[0], in[1], nxVec(s, 0), dFaces, wFaces, nxVecW(s, normalsOff),
			(unsigned char) in[3]);
		nxRetBits(ret, r);
		nxGather(out, 0, s, normalsOff, 26);
		nxLine("NxBuildSmoothNormals", i, in, 65, ret, out, 26);
		}
	return count;
	}

int wmain(int argc, wchar_t** argv)
	{
	// Unbuffered, so if a case faults the transcript names the last case that
	// did not.  A fault cannot be gated by a differential, so a case that
	// faults the oracle has to be found and removed, not tolerated.
	setvbuf(stdout, 0, _IONBF, 0);

	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsGeometryTests", pairDirectory, &physics);
	if(status)
		return status;

	unsigned cases = 0;
	cases += nxRun_NxComputeSphereMass(physics);
	cases += nxRun_NxComputeSphereDensity(physics);
	cases += nxRun_NxComputeBoxMass(physics);
	cases += nxRun_NxComputeBoxDensity(physics);
	cases += nxRun_NxComputeEllipsoidMass(physics);
	cases += nxRun_NxComputeEllipsoidDensity(physics);
	cases += nxRun_NxComputeCylinderMass(physics);
	cases += nxRun_NxComputeCylinderDensity(physics);
	cases += nxRun_NxComputeConeMass(physics);
	cases += nxRun_NxComputeConeDensity(physics);
	cases += nxRun_NxComputeBoxInertiaTensor(physics);
	cases += nxRun_NxComputeSphereInertiaTensor(physics);
	cases += nxRun_NxRayPlaneIntersect(physics);
	cases += nxRun_NxSegmentPlaneIntersect(physics);
	cases += nxRun_NxRaySphereIntersect(physics);
	cases += nxRun_NxRayTriIntersect(physics);
	cases += nxRun_NxRayAABBIntersect(physics);
	cases += nxRun_NxRayAABBIntersect2(physics);
	cases += nxRun_NxSegmentBoxIntersect(physics);
	cases += nxRun_NxSegmentAABBIntersect(physics);
	cases += nxRun_NxRayOBBIntersect(physics);
	cases += nxRun_NxSegmentOBBIntersect(physics);
	cases += nxRun_NxRayCapsuleIntersect(physics);
	cases += nxRun_NxSweptSpheresIntersect(physics);
	cases += nxRun_NxBoxBoxIntersect(physics);
	cases += nxRun_NxSeparatingAxis(physics);
	cases += nxRun_NxBuildSmoothNormals(physics);

	printf("cases total=%u exports=27\n", cases);

	status = nxReportPairIdentity(pairDirectory);
	FreeLibrary(physics);
	return status;
	}
