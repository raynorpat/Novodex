#include <windows.h>

struct NxFluidEmitterFlagCallbackRecord
	{
	unsigned kind;
	unsigned argument0;
	unsigned argument1;
	unsigned argument2;
	unsigned enabled;
	};

static NxFluidEmitterFlagCallbackRecord gRecords[16];
static unsigned gRecordCount;

static int nxRecord(unsigned kind, unsigned argument0, unsigned argument1,
	unsigned argument2, unsigned char enabled)
	{
	if(gRecordCount < sizeof(gRecords) / sizeof(gRecords[0]))
		{
		NxFluidEmitterFlagCallbackRecord& record = gRecords[gRecordCount++];
		record.kind = kind;
		record.argument0 = argument0;
		record.argument1 = argument1;
		record.argument2 = argument2;
		record.enabled = enabled;
		}
	return static_cast<int>(kind ^ argument0 ^ argument1 ^ argument2 ^ enabled);
	}

extern "C" __declspec(dllexport) void __cdecl NxResetFluidEmitterFlagCallbacks()
	{
	gRecordCount = 0;
	}

extern "C" __declspec(dllexport) unsigned __cdecl NxGetFluidEmitterFlagCallbackCount()
	{
	return gRecordCount;
	}

extern "C" __declspec(dllexport) int __cdecl NxGetFluidEmitterFlagCallbackRecord(
	unsigned index, NxFluidEmitterFlagCallbackRecord* record)
	{
	if(index >= gRecordCount || !record)
		return 0;
	*record = gRecords[index];
	return 1;
	}

extern "C" __declspec(dllexport) int __cdecl EmitterSetBodyRepulsionFlag(
	unsigned argument0, unsigned argument1, unsigned argument2, unsigned char enabled)
	{
	return nxRecord(4, argument0, argument1, argument2, enabled);
	}

extern "C" __declspec(dllexport) int __cdecl EmitterSetAddBodyVelocityFlag(
	unsigned argument0, unsigned argument1, unsigned argument2, unsigned char enabled)
	{
	return nxRecord(8, argument0, argument1, argument2, enabled);
	}

extern "C" __declspec(dllexport) int __cdecl EmitterSetEnabledFlag(
	unsigned argument0, unsigned argument1, unsigned argument2, unsigned char enabled)
	{
	return nxRecord(16, argument0, argument1, argument2, enabled);
	}
