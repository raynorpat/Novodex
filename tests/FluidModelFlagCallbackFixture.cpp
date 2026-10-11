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

struct NxFluidPostStepCallbackRecord
	{
	unsigned kind;
	unsigned system;
	unsigned handle;
	unsigned values[6];
	};

struct NxFluidPostStepParticleStream
	{
	unsigned* data;
	unsigned count;
	};

struct NxFluidRunCallbackRecord
	{
	unsigned system;
	unsigned fluid;
	unsigned state;
	unsigned vector[3];
	unsigned timestep;
	unsigned option;
	};

struct NxFluidRepulsionCallbackRecord
	{
	unsigned kind;
	unsigned system;
	unsigned fluid;
	unsigned emitter;
	unsigned vectors[6];
	unsigned mass;
	};

struct NxImplicitFluidMeshFixture
	{
	unsigned* leadingVectors;
	unsigned leadingCount;
	unsigned unused0;
	unsigned* surfaceVertices;
	unsigned surfaceCount;
	unsigned unused1;
	unsigned* surfaceNormals;
	};

static NxFluidPostStepCallbackRecord gPostStepRecords[16];
static unsigned gPostStepRecordCount;
static NxFluidRunCallbackRecord gFluidRunRecords[16];
static unsigned gFluidRunRecordCount;
static NxFluidRepulsionCallbackRecord gFluidRepulsionRecords[16];
static unsigned gFluidRepulsionRecordCount;
static unsigned gPostStepParticles[2][2][23];
static NxFluidPostStepParticleStream gPostStepStreams[2];
static unsigned gPostStepLeadingVectors[2][3] = {
	{ 0x3e800001u, 0x3e800002u, 0x3e800003u },
	{ 0x3f000001u, 0x3f000002u, 0x3f000003u }
	};
static unsigned gPostStepSurfaceVertices[2][3] = {
	{ 0x3f400001u, 0x3f400002u, 0x3f400003u },
	{ 0x3f800001u, 0x3f800002u, 0x3f800003u }
	};
static unsigned gPostStepSurfaceNormals[2][3] = {
	{ 0x3fc00001u, 0x3fc00002u, 0x3fc00003u },
	{ 0x40000001u, 0x40000002u, 0x40000003u }
	};
static NxImplicitFluidMeshFixture gPostStepMeshes[2];

extern "C" __declspec(dllexport) void __cdecl NxResetFluidPostStepCallbacks()
	{
	gPostStepRecordCount = 0;
	}

extern "C" __declspec(dllexport) void __cdecl NxResetFluidRunCallbacks()
	{
	gFluidRunRecordCount = 0;
	}

extern "C" __declspec(dllexport) unsigned __cdecl NxGetFluidRunCallbackCount()
	{
	return gFluidRunRecordCount;
	}

extern "C" __declspec(dllexport) int __cdecl NxGetFluidRunCallbackRecord(
	unsigned index, NxFluidRunCallbackRecord* record)
	{
	if(index >= gFluidRunRecordCount || !record)
		return 0;
	*record = gFluidRunRecords[index];
	return 1;
	}

extern "C" __declspec(dllexport) float __cdecl FluidRun(unsigned system,
	unsigned fluid, unsigned state, const float* vector, float timestep,
	unsigned option)
	{
	if(gFluidRunRecordCount < sizeof(gFluidRunRecords) / sizeof(gFluidRunRecords[0]))
		{
		NxFluidRunCallbackRecord& record = gFluidRunRecords[gFluidRunRecordCount++];
		ZeroMemory(&record, sizeof(record));
		record.system = system;
		record.fluid = fluid;
		record.state = state;
		if(vector)
			CopyMemory(record.vector, vector, sizeof(record.vector));
		record.timestep = *reinterpret_cast<unsigned*>(&timestep);
		record.option = option;
		}
	return 0.375f;
	}

extern "C" __declspec(dllexport) void __cdecl NxResetFluidRepulsionCallbacks()
	{
	gFluidRepulsionRecordCount = 0;
	}

extern "C" __declspec(dllexport) unsigned __cdecl NxGetFluidRepulsionCallbackCount()
	{
	return gFluidRepulsionRecordCount;
	}

extern "C" __declspec(dllexport) int __cdecl NxGetFluidRepulsionCallbackRecord(
	unsigned index, NxFluidRepulsionCallbackRecord* record)
	{
	if(index >= gFluidRepulsionRecordCount || !record)
		return 0;
	*record = gFluidRepulsionRecords[index];
	return 1;
	}

static NxFluidRepulsionCallbackRecord& nxNextFluidRepulsionRecord(unsigned kind,
	unsigned system, unsigned fluid, unsigned emitter)
	{
	static NxFluidRepulsionCallbackRecord overflow = {};
	NxFluidRepulsionCallbackRecord& record = gFluidRepulsionRecordCount
		< sizeof(gFluidRepulsionRecords) / sizeof(gFluidRepulsionRecords[0])
		? gFluidRepulsionRecords[gFluidRepulsionRecordCount++] : overflow;
	ZeroMemory(&record, sizeof(record));
	record.kind = kind;
	record.system = system;
	record.fluid = fluid;
	record.emitter = emitter;
	return record;
	}

extern "C" __declspec(dllexport) int __cdecl EmitterGetEnabledFlag(
	unsigned system, unsigned fluid, unsigned emitter)
	{
	nxNextFluidRepulsionRecord(1, system, fluid, emitter);
	return 1;
	}

extern "C" __declspec(dllexport) int __cdecl EmitterGetBodyRepulsion(
	unsigned system, unsigned fluid, unsigned emitter, float* force, float* torque)
	{
	NxFluidRepulsionCallbackRecord& record = nxNextFluidRepulsionRecord(
		2, system, fluid, emitter);
	static const unsigned values[6] = {
		0x3e800001u, 0xbf000002u, 0x3f400003u,
		0x40000004u, 0xc0400005u, 0x40800006u };
	memcpy(record.vectors, values, sizeof(values));
	memcpy(force, values, 3 * sizeof(float));
	memcpy(torque, values + 3, 3 * sizeof(float));
	return 0;
	}

extern "C" __declspec(dllexport) float __cdecl FluidGetParticleMass(
	unsigned system, unsigned fluid)
	{
	NxFluidRepulsionCallbackRecord& record = nxNextFluidRepulsionRecord(
		3, system, fluid, 0);
	const unsigned mass = 0x3f2aaaab;
	record.mass = mass;
	return *reinterpret_cast<const float*>(&mass);
	}

extern "C" __declspec(dllexport) unsigned __cdecl NxGetFluidPostStepCallbackCount()
	{
	return gPostStepRecordCount;
	}

extern "C" __declspec(dllexport) int __cdecl NxGetFluidPostStepCallbackRecord(
	unsigned index, NxFluidPostStepCallbackRecord* record)
	{
	if(index >= gPostStepRecordCount || !record)
		return 0;
	*record = gPostStepRecords[index];
	return 1;
	}

extern "C" __declspec(dllexport) void __cdecl NxSetFluidPostStepParticle(
	unsigned index, const unsigned* words, unsigned count)
	{
	if(index >= 2 || !words || count > 2)
		return;
	for(unsigned word = 0; word != count * 23; ++word)
		reinterpret_cast<unsigned*>(gPostStepParticles[index])[word] = words[word];
	gPostStepStreams[index].data = gPostStepParticles[index][0];
	gPostStepStreams[index].count = count;
	}

static void nxRecordPostStepCallback(unsigned kind, unsigned system,
	unsigned handle, const unsigned* values, unsigned valueCount)
	{
	if(gPostStepRecordCount >= sizeof(gPostStepRecords) / sizeof(gPostStepRecords[0]))
		return;
	NxFluidPostStepCallbackRecord& record = gPostStepRecords[gPostStepRecordCount++];
	ZeroMemory(&record, sizeof(record));
	record.kind = kind;
	record.system = system;
	record.handle = handle;
	for(unsigned index = 0; values && index < valueCount && index < 6; ++index)
		record.values[index] = values[index];
	}

extern "C" __declspec(dllexport) void __cdecl FluidGetBounds(
	unsigned system, unsigned handle, float* minimum, float* maximum)
	{
	static const unsigned values[2][6] = {
		{ 0x3f800000u, 0x40000000u, 0x40400000u, 0x40800000u, 0x40a00000u, 0x40c00000u },
		{ 0x41300000u, 0x41400000u, 0x41500000u, 0x41600000u, 0x41700000u, 0x41800000u }
		};
	const unsigned index = system & 1u;
	if(minimum)
		for(unsigned axis = 0; axis != 3; ++axis)
			reinterpret_cast<unsigned*>(minimum)[axis] = values[index][axis];
	if(maximum)
		for(unsigned axis = 0; axis != 3; ++axis)
			reinterpret_cast<unsigned*>(maximum)[axis] = values[index][axis + 3];
	nxRecordPostStepCallback(4096, system, handle, values[index], 6);
	}

extern "C" __declspec(dllexport) NxFluidPostStepParticleStream* __cdecl FluidGetParticles(
	unsigned system, unsigned handle)
	{
	nxRecordPostStepCallback(8192, system, handle, 0, 0);
	return &gPostStepStreams[system & 1u];
	}

extern "C" __declspec(dllexport) NxImplicitFluidMeshFixture* __cdecl ImplicitFluidMeshGetMesh(
	unsigned system, unsigned handle)
	{
	const unsigned index = system & 1u;
	nxRecordPostStepCallback(16384, system, handle, 0, 0);
	NxImplicitFluidMeshFixture& mesh = gPostStepMeshes[index];
	mesh.leadingVectors = gPostStepLeadingVectors[index];
	mesh.leadingCount = 1;
	mesh.unused0 = 0;
	mesh.surfaceVertices = gPostStepSurfaceVertices[index];
	mesh.surfaceCount = 1;
	mesh.unused1 = 0;
	mesh.surfaceNormals = gPostStepSurfaceNormals[index];
	return &mesh;
	}

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
	const unsigned emitterHandle = argument2;
	nxRecordPostStepCallback(32768, argument0, argument1, &emitterHandle, 1);
	return gEmitterGlobalPose;
	}

extern "C" __declspec(dllexport) int __cdecl EmitterSetGlobalPose(
	unsigned argument0, unsigned argument1, unsigned argument2, const float* pose)
	{
	nxRecord(64, argument0, argument1, argument2, 0);
	if(gRecordCount && pose)
		CopyMemory(gRecords[gRecordCount - 1].pose, pose, sizeof(gRecords[0].pose));
	if(pose)
		CopyMemory(gEmitterGlobalPose, pose, sizeof(gEmitterGlobalPose));
	return 0;
	}

extern "C" __declspec(dllexport) int __cdecl EmitterSetAttachedFlag(
	unsigned argument0, unsigned argument1, unsigned argument2, unsigned char attached)
	{
	return nxRecord(128, argument0, argument1, argument2, attached);
	}

static int nxRecordBodyVector(unsigned kind, unsigned argument0, unsigned argument1,
	unsigned argument2, const float* vector)
	{
	const int result = nxRecord(kind, argument0, argument1, argument2, 0);
	if(gRecordCount && vector)
		CopyMemory(gRecords[gRecordCount - 1].pose, vector, 3 * sizeof(float));
	return result;
	}

extern "C" __declspec(dllexport) int __cdecl EmitterSetBodyPosition(
	unsigned argument0, unsigned argument1, unsigned argument2, const float* position)
	{
	return nxRecordBodyVector(256, argument0, argument1, argument2, position);
	}

extern "C" __declspec(dllexport) int __cdecl EmitterSetBodyLinearVelocity(
	unsigned argument0, unsigned argument1, unsigned argument2, const float* velocity)
	{
	return nxRecordBodyVector(512, argument0, argument1, argument2, velocity);
	}

extern "C" __declspec(dllexport) int __cdecl EmitterSetBodyAngularVelocity(
	unsigned argument0, unsigned argument1, unsigned argument2, const float* velocity)
	{
	return nxRecordBodyVector(1024, argument0, argument1, argument2, velocity);
	}

extern "C" __declspec(dllexport) int __cdecl ImplicitFluidMeshRun(
	unsigned argument0, unsigned argument1, unsigned argument2, unsigned argument3)
	{
	if(gRecordCount < sizeof(gRecords) / sizeof(gRecords[0]))
		{
		NxFluidEmitterFlagCallbackRecord& record = gRecords[gRecordCount++];
		ZeroMemory(&record, sizeof(record));
		record.kind = 2048;
		record.argument0 = argument0;
		record.argument1 = argument1;
		record.argument2 = argument2;
		record.enabled = argument3;
		}
	return static_cast<int>(2048u ^ argument0 ^ argument1 ^ argument2 ^ argument3);
	}
