#include <windows.h>

struct NxFluidEmitterFlagCallbackRecord
	{
	unsigned kind;
	unsigned argument0;
	unsigned argument1;
	unsigned argument2;
	unsigned enabled;
	unsigned pose[12];
	};

static NxFluidEmitterFlagCallbackRecord gRecords[16];
static unsigned gRecordCount;
static float gEmitterGlobalPose[12];

static int nxRecord(unsigned kind, unsigned argument0, unsigned argument1,
	unsigned argument2, unsigned char enabled)
	{
	if(gRecordCount < sizeof(gRecords) / sizeof(gRecords[0]))
		{
		NxFluidEmitterFlagCallbackRecord& record = gRecords[gRecordCount++];
		ZeroMemory(&record, sizeof(record));
		record.kind = kind;
		record.argument0 = argument0;
		record.argument1 = argument1;
		record.argument2 = argument2;
		record.enabled = enabled;
		}
	return static_cast<int>(kind ^ argument0 ^ argument1 ^ argument2 ^ enabled);
	}

extern "C" __declspec(dllexport) void __cdecl NxSetEmitterGlobalPose(
	const float* pose)
	{
	if(pose)
		CopyMemory(gEmitterGlobalPose, pose, sizeof(gEmitterGlobalPose));
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

extern "C" __declspec(dllexport) const float* __cdecl EmitterGetGlobalPose(
	unsigned argument0, unsigned argument1, unsigned argument2)
	{
	nxRecord(32, argument0, argument1, argument2, 0);
	return gEmitterGlobalPose;
	}

extern "C" __declspec(dllexport) int __cdecl EmitterSetGlobalPose(
	unsigned argument0, unsigned argument1, unsigned argument2, const float* pose)
	{
	const int result = nxRecord(64, argument0, argument1, argument2, 0);
	if(gRecordCount && pose)
		CopyMemory(gRecords[gRecordCount - 1].pose, pose, sizeof(gRecords[0].pose));
	if(pose)
		CopyMemory(gEmitterGlobalPose, pose, sizeof(gEmitterGlobalPose));
	return result;
	}

extern "C" __declspec(dllexport) int __cdecl EmitterSetAttachedFlag(
	unsigned argument0, unsigned argument1, unsigned argument2, unsigned char attached)
	{
	return nxRecord(128, argument0, argument1, argument2, attached);
	}
