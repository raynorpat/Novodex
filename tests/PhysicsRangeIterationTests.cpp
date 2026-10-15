#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "PhysicsPairLoader.h"

extern "C" int __stdcall phys_fn_000002(
	int first, int stride, int count, int (__thiscall *callback)(int));

static const unsigned long kOracleRangeIterationRva = 0x00001030;
static int gValues[16];
static int gCount;

typedef int (__stdcall *RangeIterationFn)(int, int, int,
	int (__thiscall *)(int));

struct RangeResult
	{
	int result;
	int count;
	int values[16];
	int stackDelta;
	};

struct RangeCase
	{
	int first;
	int stride;
	int count;
	};

static int __fastcall nxCaptureRangeValue(int value, int)
	{
	if(gCount < sizeof(gValues) / sizeof(gValues[0]))
		gValues[gCount++] = value;
	return value ^ 0x13579bdf;
	}

static unsigned long long nxFold(unsigned long long state, unsigned value)
	{
	for(unsigned byte = 0; byte != 4; ++byte)
		{
		state ^= (value >> (byte * 8)) & 0xff;
		state *= 1099511628211ull;
		}
	return state;
	}

static RangeResult nxRunRange(RangeIterationFn function,
	const RangeCase& test)
	{
	RangeResult output = {};
	int (__thiscall *callback)(int) = reinterpret_cast<int (__thiscall *)(int)>(
		nxCaptureRangeValue);
	memset(gValues, 0, sizeof(gValues));
	gCount = 0;
	unsigned stackBefore = 0;
	unsigned stackAfter = 0;
	__asm mov stackBefore, esp
	output.result = function(test.first, test.stride, test.count, callback);
	__asm mov stackAfter, esp
	output.count = gCount;
	output.stackDelta = static_cast<int>(stackAfter - stackBefore);
	memcpy(output.values, gValues, sizeof(output.values));
	return output;
	}

static unsigned nxCompare(const RangeResult& oracle, const RangeResult& candidate)
	{
	unsigned mismatches = 0;
	mismatches += oracle.result != candidate.result;
	mismatches += oracle.count != candidate.count;
	mismatches += oracle.stackDelta != candidate.stackDelta;
	for(unsigned i = 0; i != static_cast<unsigned>(oracle.count); ++i)
		mismatches += oracle.values[i] != candidate.values[i];
	return mismatches;
	}

static unsigned long long nxFoldResult(unsigned long long digest,
	const RangeResult& result)
	{
	digest = nxFold(digest, static_cast<unsigned>(result.result));
	digest = nxFold(digest, static_cast<unsigned>(result.count));
	digest = nxFold(digest, static_cast<unsigned>(result.stackDelta));
	for(unsigned i = 0; i != static_cast<unsigned>(result.count); ++i)
		digest = nxFold(digest, static_cast<unsigned>(result.values[i]));
	return digest;
	}

int wmain(int argc, wchar_t** argv)
	{
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	if(argc != 3)
		{
		fprintf(stderr, "usage: NxPhysicsRangeIterationTests <oracle directory> <NxPhysics.dll sha256>\n");
		return 2;
		}
	int status = nxOpenPair(argc - 1, argv, "NxPhysicsRangeIterationTests",
		pairDirectory, &physics);
	if(status)
		return status;

	wchar_t loadedPath[MAX_PATH];
	char loadedHash[65], expectedHash[65];
	if(!GetModuleFileNameW(physics, loadedPath, MAX_PATH)
		|| !nxSha256(loadedPath, loadedHash))
		return nxFail("cannot identify the loaded oracle");
	if(wcstombs_s(0, expectedHash, sizeof(expectedHash), argv[2], _TRUNCATE) != 0
		|| strcmp(loadedHash, expectedHash) != 0)
		return nxFail("loaded NxPhysics.dll is not the pinned oracle");
	unsigned char* oracleBase = reinterpret_cast<unsigned char*>(physics);
	RangeIterationFn oracleFunction = reinterpret_cast<RangeIterationFn>(
		oracleBase + kOracleRangeIterationRva);
	RangeIterationFn candidateFunction = phys_fn_000002;
	const RangeCase cases[] = {
		{100, 4, 3},
		{-20, -4, 2},
		{7, 3, 0},
		{13, 5, -3},
		{0x12345678, 0, 4},
		{27, -9, 1}
		};
	unsigned long long oracleDigest = 14695981039346656037ull;
	unsigned long long candidateDigest = 14695981039346656037ull;
	unsigned long long inputDigest = 14695981039346656037ull;
	unsigned mismatches = 0;
	for(unsigned i = 0; i != sizeof(cases) / sizeof(cases[0]); ++i)
		{
		const RangeCase& test = cases[i];
		inputDigest = nxFold(inputDigest, static_cast<unsigned>(test.first));
		inputDigest = nxFold(inputDigest, static_cast<unsigned>(test.stride));
		inputDigest = nxFold(inputDigest, static_cast<unsigned>(test.count));
		RangeResult oracle = nxRunRange(oracleFunction, test);
		RangeResult candidate = nxRunRange(candidateFunction, test);
		oracleDigest = nxFoldResult(oracleDigest, oracle);
		candidateDigest = nxFoldResult(candidateDigest, candidate);
		mismatches += nxCompare(oracle, candidate);
		}
	printf("range_iteration cases=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		static_cast<unsigned>(sizeof(cases) / sizeof(cases[0])), oracleDigest,
		candidateDigest, mismatches);
	printf("range_iteration inputs=%u digest=%016llx\n",
		static_cast<unsigned>(sizeof(cases) / sizeof(cases[0])), inputDigest);
	if(nxReportPairIdentity(pairDirectory))
		return 1;
	return mismatches ? nxFail("oracle and candidate range results differ") : 0;
	}
