#include <stdio.h>

extern "C" int __stdcall phys_fn_000002(
	int first, int stride, int count, int (__thiscall *callback)(int));

static int gValues[8];
static int gCount;

static int __fastcall nxCaptureRangeValue(int value, int)
	{
	if(gCount < sizeof(gValues) / sizeof(gValues[0]))
		gValues[gCount++] = value;
	return value ^ 0x13579bdf;
	}

static int nxCheck(const char* name, int condition)
	{
	if(!condition)
		{
		fprintf(stderr, "FAIL %s\n", name);
		return 0;
		}
	return 1;
	}

int main()
	{
	int passed = 1;
	int (__thiscall *callback)(int) = reinterpret_cast<int (__thiscall *)(int)>(
		nxCaptureRangeValue);

	gCount = 0;
	const int result = phys_fn_000002(100, 4, 3, callback);
	passed &= nxCheck("three values are visited in descending address order",
		gCount == 3 && gValues[0] == 108 && gValues[1] == 104 && gValues[2] == 100);
	passed &= nxCheck("the last callback result is returned",
		result == (100 ^ 0x13579bdf));

	gCount = 0;
	const int negativeStrideResult = phys_fn_000002(-20, -4, 1, callback);
	passed &= nxCheck("negative stride is applied before the callback",
		gCount == 1 && gValues[0] == -20 && negativeStrideResult == (-20 ^ 0x13579bdf));

	gCount = 0;
	const int emptyResult = phys_fn_000002(7, 3, 0, callback);
	passed &= nxCheck("zero count returns count minus one without calling back",
		gCount == 0 && emptyResult == -1);

	if(!passed)
		return 1;
	puts("phys_fn_000002 range iteration checks passed");
	return 0;
	}
