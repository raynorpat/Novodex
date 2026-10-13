#include <windows.h>
#include <math.h>
#include <stdlib.h>

struct NxFluidCollisionArrayFixture
	{
	void* data;
	unsigned count;
	unsigned capacity;
	};

struct NxFluidCollisionGridFixture
	{
	void* buckets;
	unsigned bucketCapacity;
	unsigned bucketCount;
	void* cells;
	unsigned cellCount;
	unsigned cellCapacity;
	void* indices;
	unsigned indexCount;
	unsigned indexCapacity;
	};

struct NxFluidCollisionInt16Fixture
	{
	short* data;
	int count;
	unsigned capacity;
	};

struct NxFluidManagerCallbackSummary
	{
	unsigned managerCreate;
	unsigned managerDestroy;
	unsigned fluidCreate;
	unsigned fluidDestroy;
	unsigned staticCollisionCreate;
	unsigned staticCollisionGet;
	unsigned fluidGetCellSize;
	unsigned staticCollisionDestroy;
	unsigned fluidCreateArguments[15];
	unsigned fluidGetRestParticleDistance;
	unsigned emitterCreate;
	unsigned emitterDestroy;
	unsigned emitterSetGlobalPose;
	unsigned emitterGetGlobalPose;
	unsigned emitterCreateArguments[4];
	unsigned fluidGetStaticCollisionRestitution;
	unsigned fluidGetStaticCollisionAdhesion;
	};

struct NxFluidManagerCallbackRecord
	{
	unsigned kind;
	unsigned arguments[4];
	};

static NxFluidManagerCallbackSummary gFluidManagerCallbackSummary;
static NxFluidManagerCallbackRecord gFluidManagerRecords[8];
static unsigned gFluidManagerRecordCount;
static unsigned gFluidManagerGetCallCount;
static bool gFluidManagerCallbackProbe;
static unsigned gFluidGetDampingCalls;
static unsigned gFluidGetStaticCollisionRestitutionCalls;
static unsigned gFluidGetStaticCollisionAdhesionCalls;
static unsigned char gFluidStaticCollision[0x200];
static void* gFluidManagerOldStaticCollision = gFluidStaticCollision;
static void* gFluidManagerReplacementStaticCollision = gFluidStaticCollision;
static unsigned gFluidManagerStaticCollisionGetIndex;
static unsigned char gFluidManagerBackend;
static unsigned char gFluidBackendHandles[16];
static unsigned char gFluidEmitterBackendHandles[16][16];
static unsigned gFluidEmitterCreateNullIndex = ~0u;

static void nxRecordFluidManagerCallback(unsigned kind, unsigned argument0,
	unsigned argument1, unsigned argument2, unsigned argument3)
	{
	if(gFluidManagerRecordCount < sizeof(gFluidManagerRecords) / sizeof(gFluidManagerRecords[0]))
		{
		NxFluidManagerCallbackRecord* const record =
			&gFluidManagerRecords[gFluidManagerRecordCount++];
		record->kind = kind;
		record->arguments[0] = argument0;
		record->arguments[1] = argument1;
		record->arguments[2] = argument2;
		record->arguments[3] = argument3;
		}
	}

static unsigned nxFluidCollisionFixtureGrow(NxFluidCollisionArrayFixture* array,
	unsigned requested, unsigned fill, unsigned stride)
	{
	// StaticCollisionCreate first counts each collision shape into this field,
	// then the FluidModel array-grow callbacks prepare the destination for the
	// second traversal that packs those shapes.
	array->count = 0;
	if(requested <= array->capacity)
		return array->capacity;
	void* const resized = realloc(array->data,
		static_cast<size_t>(requested) * stride);
	if(!resized)
		return 0;
	if(requested > array->capacity)
		memset(static_cast<unsigned char*>(resized)
			+ static_cast<size_t>(array->capacity) * stride, 0,
			(static_cast<size_t>(requested) - array->capacity) * stride);
	array->data = resized;
	array->capacity = requested;
	(void)fill;
	return requested;
	}

extern "C" __declspec(dllexport) void __cdecl StaticCollisionCreate(void* state)
	{
	++gFluidManagerCallbackSummary.staticCollisionCreate;
	ZeroMemory(state, 0xa0);
	}

extern "C" __declspec(dllexport) void __cdecl StaticCollisionDestroy(void* state)
	{
	nxRecordFluidManagerCallback(2, reinterpret_cast<unsigned>(state), 0, 0, 0);
	if(state == gFluidManagerOldStaticCollision)
		++gFluidManagerCallbackSummary.staticCollisionDestroy;
	}

extern "C" __declspec(dllexport) unsigned __cdecl CDMeshArrayGrow(
	void* array, unsigned count, unsigned fill)
	{ return nxFluidCollisionFixtureGrow(static_cast<NxFluidCollisionArrayFixture*>(array), count, fill, 96); }
extern "C" __declspec(dllexport) unsigned __cdecl CDPlaneArrayGrow(
	void* array, unsigned count, unsigned fill)
	{ return nxFluidCollisionFixtureGrow(static_cast<NxFluidCollisionArrayFixture*>(array), count, fill, 96); }
extern "C" __declspec(dllexport) unsigned __cdecl CDSphereArrayGrow(
	void* array, unsigned count, unsigned fill)
	{ return nxFluidCollisionFixtureGrow(static_cast<NxFluidCollisionArrayFixture*>(array), count, fill, 84); }
extern "C" __declspec(dllexport) unsigned __cdecl CDCapsuleArrayGrow(
	void* array, unsigned count, unsigned fill)
	{ return nxFluidCollisionFixtureGrow(static_cast<NxFluidCollisionArrayFixture*>(array), count, fill, 88); }
extern "C" __declspec(dllexport) unsigned __cdecl CDBoxArrayGrow(
	void* array, unsigned count, unsigned fill)
	{ return nxFluidCollisionFixtureGrow(static_cast<NxFluidCollisionArrayFixture*>(array), count, fill, 92); }

extern "C" __declspec(dllexport) void __cdecl CDBucketAttributesArrayCreate(
	void* array, unsigned count)
	{
	NxFluidCollisionGridFixture* const grid =
		static_cast<NxFluidCollisionGridFixture*>(array);
	grid->buckets = calloc(count, 12);
	grid->bucketCapacity = count;
	grid->bucketCount = count;
	}

extern "C" __declspec(dllexport) void __cdecl CDBucketAttributesArrayInit(
	void* array)
	{
	NxFluidCollisionGridFixture* const grid =
		static_cast<NxFluidCollisionGridFixture*>(array);
	if(grid->buckets)
		memset(grid->buckets, 0,
			static_cast<size_t>(grid->bucketCount) * 12);
	}

extern "C" __declspec(dllexport) void __cdecl CDBucketAttributesArrayCopy(
	void* destination, const void* source)
	{
	NxFluidCollisionGridFixture* const output =
		static_cast<NxFluidCollisionGridFixture*>(destination);
	const NxFluidCollisionGridFixture* const input =
		static_cast<const NxFluidCollisionGridFixture*>(source);
	output->buckets = input->bucketCount
		? malloc(static_cast<size_t>(input->bucketCount) * 12) : 0;
	output->bucketCapacity = input->bucketCapacity;
	output->bucketCount = input->bucketCount;
	if(input->bucketCount)
		memcpy(output->buckets, input->buckets,
			static_cast<size_t>(input->bucketCount) * 12);
	}

extern "C" __declspec(dllexport) void __cdecl CDCellAttributesArrayCreate(
	void* array, unsigned count)
	{
	NxFluidCollisionArrayFixture* const values =
		static_cast<NxFluidCollisionArrayFixture*>(array);
	values->data = count ? calloc(count, 16) : 0;
	values->count = 0;
	values->capacity = count;
	}

extern "C" __declspec(dllexport) void __cdecl CDIndexArrayCreate(
	void* array, unsigned count)
	{
	NxFluidCollisionArrayFixture* const values =
		static_cast<NxFluidCollisionArrayFixture*>(array);
	values->data = count ? calloc(count, 8) : 0;
	values->count = 0;
	values->capacity = count;
	}

extern "C" __declspec(dllexport) void __cdecl Int16Vector3ArrayCreate(
	void* array, unsigned count)
	{
	NxFluidCollisionInt16Fixture* const values =
		static_cast<NxFluidCollisionInt16Fixture*>(array);
	values->data = count ? static_cast<short*>(calloc(count * 3, sizeof(short))) : 0;
	values->count = 0;
	values->capacity = count;
	}

extern "C" __declspec(dllexport) void __cdecl CDHashDestroy(void* array)
	{
	NxFluidCollisionGridFixture* const grid =
		static_cast<NxFluidCollisionGridFixture*>(array);
	free(grid->buckets);
	free(grid->cells);
	free(grid->indices);
	grid->buckets = grid->cells = grid->indices = 0;
	}

extern "C" __declspec(dllexport) int __cdecl Int16Vector3ArrayDestroy(void* array)
	{
	NxFluidCollisionInt16Fixture* const values =
		static_cast<NxFluidCollisionInt16Fixture*>(array);
	const int result = values->count;
	free(values->data);
	values->data = 0;
	values->count = 0;
	values->capacity = 0;
	return result;
	}

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

extern "C" __declspec(dllexport) void __cdecl NxResetFluidManagerCallbacks()
	{
	ZeroMemory(&gFluidManagerCallbackSummary, sizeof(gFluidManagerCallbackSummary));
	ZeroMemory(gFluidManagerRecords, sizeof(gFluidManagerRecords));
	gFluidManagerRecordCount = 0;
	gFluidManagerGetCallCount = 0;
	gFluidManagerCallbackProbe = false;
	gFluidGetDampingCalls = 0;
	gFluidGetStaticCollisionRestitutionCalls = 0;
	gFluidGetStaticCollisionAdhesionCalls = 0;
	gFluidEmitterCreateNullIndex = ~0u;
	ZeroMemory(gFluidStaticCollision, sizeof(gFluidStaticCollision));
	gFluidManagerStaticCollisionGetIndex = 0;
	}

extern "C" __declspec(dllexport) void __cdecl NxSetFluidManagerStaticCollisions(
	void* oldCollision, void* replacementCollision)
	{
	gFluidManagerOldStaticCollision = oldCollision;
	gFluidManagerReplacementStaticCollision = replacementCollision;
	gFluidManagerStaticCollisionGetIndex = 0;
	gFluidManagerCallbackProbe = true;
	}

extern "C" __declspec(dllexport) void __cdecl NxSetFluidManagerStaticCollision(
	void* collision)
	{
	gFluidManagerOldStaticCollision = collision;
	gFluidManagerReplacementStaticCollision = collision;
	gFluidManagerStaticCollisionGetIndex = 0;
	gFluidManagerCallbackProbe = true;
	}

extern "C" __declspec(dllexport) unsigned __cdecl NxGetFluidManagerCallbackCount()
	{
	return gFluidManagerRecordCount;
	}

extern "C" __declspec(dllexport) int __cdecl NxGetFluidManagerCallbackRecord(
	unsigned index, NxFluidManagerCallbackRecord* output)
	{
	if(!output || index >= gFluidManagerRecordCount)
		return 0;
	*output = gFluidManagerRecords[index];
	return 1;
	}

extern "C" __declspec(dllexport) void __cdecl NxGetFluidManagerCallbackSummary(
	NxFluidManagerCallbackSummary* summary)
	{
	if(summary)
		*summary = gFluidManagerCallbackSummary;
	}

extern "C" __declspec(dllexport) void __cdecl NxSetFluidEmitterCreateNullIndex(
	unsigned index)
	{
	gFluidEmitterCreateNullIndex = index;
	}

extern "C" __declspec(dllexport) void* __cdecl FluidManagerCreate()
	{
	++gFluidManagerCallbackSummary.managerCreate;
	return &gFluidManagerBackend;
	}

extern "C" __declspec(dllexport) void __cdecl FluidManagerDestroy(void* manager)
	{
	if(manager == &gFluidManagerBackend)
		++gFluidManagerCallbackSummary.managerDestroy;
	}

static void* __cdecl nxFluidManagerGetStaticCollisionDispatch(void* manager,
	unsigned argument1, unsigned argument2, unsigned argument3)
	{
	const unsigned call = gFluidManagerGetCallCount++;
	++gFluidManagerCallbackSummary.staticCollisionGet;
	if(call == 0)
		argument1 = argument2 = argument3 = 0;
	nxRecordFluidManagerCallback(1, reinterpret_cast<unsigned>(manager),
		argument1, argument2, argument3);
	if(manager != &gFluidManagerBackend && !gFluidManagerCallbackProbe)
		return 0;
	return gFluidManagerStaticCollisionGetIndex++ == 0
		? gFluidManagerOldStaticCollision : gFluidManagerReplacementStaticCollision;
	}

// This slot has a one-argument first call and a four-argument replacement
// lookup on the dirty refresh path. Preserve the original caller-clean ABI.
extern "C" __declspec(dllexport) __declspec(naked) void* __cdecl
FluidManagerGetStaticCollision()
	{
	__asm
		{
		push dword ptr [esp+16]
		push dword ptr [esp+16]
		push dword ptr [esp+16]
		push dword ptr [esp+16]
		call nxFluidManagerGetStaticCollisionDispatch
		add esp, 10h
		ret
		}
	}

extern "C" __declspec(dllexport) float __cdecl FluidGetCellSize(
	void* manager, void* fluid)
	{
	if(manager != &gFluidManagerBackend || !fluid)
		return 0.0f;
	++gFluidManagerCallbackSummary.fluidGetCellSize;
	return 0.5f;
	}

extern "C" __declspec(dllexport) void* __cdecl FluidCreate(void* manager,
	unsigned restParticleDistance, unsigned maxParticles, unsigned simulationMethod,
	unsigned restDensity, unsigned stiffness, unsigned viscosity, unsigned damping,
	unsigned kernelRadiusMultiplier, unsigned packetSizeMultiplier,
	unsigned initialParticleCapacity, unsigned collisionMethod,
	unsigned particleWriteData, unsigned sparse, unsigned hardware)
	{
	++gFluidManagerCallbackSummary.fluidCreate;
	const unsigned arguments[15] = {
		reinterpret_cast<unsigned>(manager), restParticleDistance, maxParticles,
		simulationMethod, restDensity, stiffness, viscosity, damping,
		kernelRadiusMultiplier, packetSizeMultiplier, initialParticleCapacity,
		collisionMethod, particleWriteData, sparse, hardware };
	memcpy(gFluidManagerCallbackSummary.fluidCreateArguments, arguments,
		sizeof(arguments));
	if(manager != &gFluidManagerBackend)
		return 0;
	const unsigned index = (gFluidManagerCallbackSummary.fluidCreate - 1)
		% (sizeof(gFluidBackendHandles) / sizeof(gFluidBackendHandles[0]));
	return &gFluidBackendHandles[index];
	}

extern "C" __declspec(dllexport) void __cdecl FluidDestroy(void* manager, void* fluid)
	{
	if(manager == &gFluidManagerBackend && fluid)
		++gFluidManagerCallbackSummary.fluidDestroy;
	}

extern "C" __declspec(dllexport) float __cdecl FluidGetDamping(
	void* manager, void* fluid)
	{
	++gFluidGetDampingCalls;
	if(manager != &gFluidManagerBackend || !fluid)
		return -1.0f;
	for(unsigned i = 0; i != sizeof(gFluidBackendHandles) / sizeof(gFluidBackendHandles[0]); ++i)
		if(fluid == &gFluidBackendHandles[i])
			return 0.375f;
	return -1.0f;
	}

extern "C" __declspec(dllexport) unsigned __cdecl NxGetFluidDampingCalls()
	{
	return gFluidGetDampingCalls;
	}

static bool nxFluidFixtureHasBackendFluid(void* manager, void* fluid)
	{
	if(manager != &gFluidManagerBackend || !fluid)
		return false;
	for(unsigned i = 0; i != sizeof(gFluidBackendHandles) / sizeof(gFluidBackendHandles[0]); ++i)
		if(fluid == &gFluidBackendHandles[i])
			return true;
	return false;
	}

extern "C" __declspec(dllexport) float __cdecl FluidGetStaticCollisionRestitution(
	void* manager, void* fluid)
	{
	if(!nxFluidFixtureHasBackendFluid(manager, fluid))
		return -1.0f;
	++gFluidManagerCallbackSummary.fluidGetStaticCollisionRestitution;
	++gFluidGetStaticCollisionRestitutionCalls;
	return 0.125f;
	}

extern "C" __declspec(dllexport) unsigned __cdecl
NxGetFluidStaticCollisionRestitutionCalls()
	{
	return gFluidGetStaticCollisionRestitutionCalls;
	}

extern "C" __declspec(dllexport) float __cdecl FluidGetStaticCollisionAdhesion(
	void* manager, void* fluid)
	{
	if(!nxFluidFixtureHasBackendFluid(manager, fluid))
		return -1.0f;
	++gFluidManagerCallbackSummary.fluidGetStaticCollisionAdhesion;
	++gFluidGetStaticCollisionAdhesionCalls;
	return 0.625f;
	}

extern "C" __declspec(dllexport) unsigned __cdecl
NxGetFluidStaticCollisionAdhesionCalls()
	{
	return gFluidGetStaticCollisionAdhesionCalls;
	}

extern "C" __declspec(dllexport) float __cdecl FluidGetRestParticleDistance(
	unsigned context, unsigned fluid, ...)
	{
	if(context && fluid)
		++gFluidManagerCallbackSummary.fluidGetRestParticleDistance;
	return 0.02f;
	}

// Keep the oracle's optional emitter path callable in differential tests. The
// production DLL discovers these exports by name from FluidModel.dll.
extern "C" __declspec(dllexport) void* __cdecl EmitterCreate(
	unsigned context, unsigned fluid, ...)
	{
	++gFluidManagerCallbackSummary.emitterCreate;
	unsigned* const args = gFluidManagerCallbackSummary.emitterCreateArguments;
	args[0] = context;
	args[1] = fluid;
	args[2] = 0;
	args[3] = 0;
	const unsigned index = (gFluidManagerCallbackSummary.emitterCreate - 1)
		% (sizeof(gFluidEmitterBackendHandles) / sizeof(gFluidEmitterBackendHandles[0]));
	if(index == gFluidEmitterCreateNullIndex)
		return 0;
	return gFluidEmitterBackendHandles[index];
	}

extern "C" __declspec(dllexport) void __cdecl EmitterDestroy(
	unsigned context, unsigned fluid, unsigned emitter)
	{
	if(context && fluid && emitter)
		++gFluidManagerCallbackSummary.emitterDestroy;
	}

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

// The pinned NxPhysics.dll late-binds this extension export and does not ship
// FluidModel.dll alongside the oracle binary. The ABI test binds this
// independent SAT reference through the oracle's triBoxOverlap slot so its
// triangle-cell row can be exercised without silently skipping the predicate.
extern "C" __declspec(dllexport) int __cdecl triBoxOverlap(
	const float* center, const float* halfSize, const float* first,
	const float* second, const float* third)
	{
	float vertices[3][3] = {
		{ first[0] - center[0], first[1] - center[1], first[2] - center[2] },
		{ second[0] - center[0], second[1] - center[1], second[2] - center[2] },
		{ third[0] - center[0], third[1] - center[1], third[2] - center[2] }
		};
	static const float basis[3][3] = {
		{ 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f },
		{ 0.0f, 0.0f, 1.0f }
		};
	float edges[3][3];
	for(unsigned edge = 0; edge != 3; ++edge)
		for(unsigned axis = 0; axis != 3; ++axis)
			edges[edge][axis] = vertices[(edge + 1) % 3][axis]
				- vertices[edge][axis];
	float axes[13][3];
	for(unsigned axis = 0; axis != 3; ++axis)
		for(unsigned component = 0; component != 3; ++component)
			axes[axis][component] = basis[axis][component];
	unsigned next = 3;
	for(unsigned edge = 0; edge != 3; ++edge)
		for(unsigned axis = 0; axis != 3; ++axis)
			{
			const unsigned other = (axis + 1) % 3;
			const unsigned last = (axis + 2) % 3;
			axes[next][axis] = 0.0f;
			axes[next][other] = edges[edge][last];
			axes[next][last] = -edges[edge][other];
			++next;
			}
	const float* const edge0 = edges[0];
	const float* const edge1 = edges[1];
	axes[12][0] = edge0[1] * edge1[2] - edge0[2] * edge1[1];
	axes[12][1] = edge0[2] * edge1[0] - edge0[0] * edge1[2];
	axes[12][2] = edge0[0] * edge1[1] - edge0[1] * edge1[0];
	for(unsigned axis = 0; axis != 13; ++axis)
		{
		float projection[3];
		for(unsigned vertex = 0; vertex != 3; ++vertex)
			projection[vertex] = vertices[vertex][0] * axes[axis][0]
				+ vertices[vertex][1] * axes[axis][1]
				+ vertices[vertex][2] * axes[axis][2];
		float minimum = projection[0];
		float maximum = projection[0];
		for(unsigned vertex = 1; vertex != 3; ++vertex)
			{
			if(projection[vertex] < minimum) minimum = projection[vertex];
			if(projection[vertex] > maximum) maximum = projection[vertex];
			}
		const float radius = halfSize[0] * fabsf(axes[axis][0])
			+ halfSize[1] * fabsf(axes[axis][1])
			+ halfSize[2] * fabsf(axes[axis][2]);
		if(minimum > radius || maximum < -radius)
			return 0;
		}
	return 1;
	}

extern "C" __declspec(dllexport) unsigned __cdecl Int16Vector3ArrayGrow(
	void* array, unsigned requestedCapacity, unsigned fillValue)
	{
	struct Int16Vector3Array
		{
		short* data;
		int count;
		unsigned capacity;
		};
	Int16Vector3Array* const values = static_cast<Int16Vector3Array*>(array);
	if(requestedCapacity <= values->capacity)
		return values->capacity;
	short* const resized = static_cast<short*>(realloc(values->data,
		static_cast<size_t>(requestedCapacity) * 3 * sizeof(short)));
	if(!resized)
		return 0;
	const unsigned oldCapacity = values->capacity;
	values->data = resized;
	values->capacity = requestedCapacity;
	const short fill = static_cast<short>(fillValue);
	for(size_t i = static_cast<size_t>(oldCapacity) * 3;
		i < static_cast<size_t>(requestedCapacity) * 3; ++i)
		resized[i] = fill;
	return requestedCapacity;
	}

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

static int gFluidEmitterEnabledResult = 1;

extern "C" __declspec(dllexport) void __cdecl NxSetFluidEmitterEnabledResult(
	int result)
	{
	gFluidEmitterEnabledResult = result;
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
	return gFluidEmitterEnabledResult;
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
	++gFluidManagerCallbackSummary.emitterGetGlobalPose;
	nxRecord(32, argument0, argument1, argument2, 0);
	const unsigned emitterHandle = argument2;
	nxRecordPostStepCallback(32768, argument0, argument1, &emitterHandle, 1);
	return gEmitterGlobalPose;
	}

extern "C" __declspec(dllexport) int __cdecl EmitterSetGlobalPose(
	unsigned argument0, unsigned argument1, unsigned argument2, const float* pose)
	{
	++gFluidManagerCallbackSummary.emitterSetGlobalPose;
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

struct NxFluidEmitterAttributeCallbackRecord
	{
	unsigned kind;
	unsigned context;
	unsigned fluid;
	unsigned emitter;
	unsigned values[3];
	};

static NxFluidEmitterAttributeCallbackRecord gAttributeRecords[16];
static unsigned gAttributeRecordCount;
static const unsigned gRandomPositionWords[3] = {
	0x3f000001u, 0xc0000002u, 0x40400003u };

static NxFluidEmitterAttributeCallbackRecord& nxNextAttributeRecord(
	unsigned kind, unsigned context, unsigned fluid, unsigned emitter)
	{
	static NxFluidEmitterAttributeCallbackRecord overflow = {};
	NxFluidEmitterAttributeCallbackRecord& record = gAttributeRecordCount
		< sizeof(gAttributeRecords) / sizeof(gAttributeRecords[0])
		? gAttributeRecords[gAttributeRecordCount++] : overflow;
	ZeroMemory(&record, sizeof(record));
	record.kind = kind;
	record.context = context;
	record.fluid = fluid;
	record.emitter = emitter;
	return record;
	}

extern "C" __declspec(dllexport) void __cdecl NxResetFluidEmitterAttributeCallbacks()
	{ gAttributeRecordCount = 0; }

extern "C" __declspec(dllexport) unsigned __cdecl NxGetFluidEmitterAttributeCallbackCount()
	{ return gAttributeRecordCount; }

extern "C" __declspec(dllexport) int __cdecl NxGetFluidEmitterAttributeCallbackRecord(
	unsigned index, NxFluidEmitterAttributeCallbackRecord* record)
	{
	if(index >= gAttributeRecordCount || !record)
		return 0;
	*record = gAttributeRecords[index];
	return 1;
	}

extern "C" __declspec(dllexport) float __cdecl EmitterGetDimensionX(
	unsigned context, unsigned fluid, unsigned emitter)
	{
	nxNextAttributeRecord(1, context, fluid, emitter);
	const unsigned bits = 0x40500001u;
	return *reinterpret_cast<const float*>(&bits);
	}

extern "C" __declspec(dllexport) float __cdecl EmitterGetDimensionY(
	unsigned context, unsigned fluid, unsigned emitter)
	{
	nxNextAttributeRecord(2, context, fluid, emitter);
	const unsigned bits = 0x3fc00002u;
	return *reinterpret_cast<const float*>(&bits);
	}

extern "C" __declspec(dllexport) int __cdecl EmitterSetRandomPos(
	unsigned context, unsigned fluid, unsigned emitter, const float* value)
	{
	NxFluidEmitterAttributeCallbackRecord& record = nxNextAttributeRecord(
		3, context, fluid, emitter);
	if(value) CopyMemory(record.values, value, sizeof(record.values));
	return 3;
	}

extern "C" __declspec(dllexport) const float* __cdecl EmitterGetRandomPos(
	unsigned context, unsigned fluid, unsigned emitter)
	{
	NxFluidEmitterAttributeCallbackRecord& record = nxNextAttributeRecord(
		4, context, fluid, emitter);
	CopyMemory(record.values, gRandomPositionWords, sizeof(record.values));
	return reinterpret_cast<const float*>(gRandomPositionWords);
	}

extern "C" __declspec(dllexport) int __cdecl EmitterSetRandomAngle(
	unsigned context, unsigned fluid, unsigned emitter, float value)
	{
	NxFluidEmitterAttributeCallbackRecord& record = nxNextAttributeRecord(
		5, context, fluid, emitter);
	record.values[0] = *reinterpret_cast<unsigned*>(&value);
	return 5;
	}

extern "C" __declspec(dllexport) float __cdecl EmitterGetRandomAngle(
	unsigned context, unsigned fluid, unsigned emitter)
	{
	nxNextAttributeRecord(6, context, fluid, emitter);
	const unsigned bits = 0x3e000006u;
	return *reinterpret_cast<const float*>(&bits);
	}

extern "C" __declspec(dllexport) int __cdecl EmitterSetFluidVelocityMagnitude(
	unsigned context, unsigned fluid, unsigned emitter, float value)
	{
	NxFluidEmitterAttributeCallbackRecord& record = nxNextAttributeRecord(
		7, context, fluid, emitter);
	record.values[0] = *reinterpret_cast<unsigned*>(&value);
	return 7;
	}

extern "C" __declspec(dllexport) float __cdecl EmitterGetFluidVelocityMagnitude(
	unsigned context, unsigned fluid, unsigned emitter)
	{
	nxNextAttributeRecord(8, context, fluid, emitter);
	const unsigned bits = 0x40e00008u;
	return *reinterpret_cast<const float*>(&bits);
	}

extern "C" __declspec(dllexport) int __cdecl EmitterSetRate(
	unsigned context, unsigned fluid, unsigned emitter, float value)
	{
	NxFluidEmitterAttributeCallbackRecord& record = nxNextAttributeRecord(
		9, context, fluid, emitter);
	record.values[0] = *reinterpret_cast<unsigned*>(&value);
	return 9;
	}

extern "C" __declspec(dllexport) float __cdecl EmitterGetRate(
	unsigned context, unsigned fluid, unsigned emitter)
	{
	nxNextAttributeRecord(10, context, fluid, emitter);
	const unsigned bits = 0x42b4000au;
	return *reinterpret_cast<const float*>(&bits);
	}

extern "C" __declspec(dllexport) int __cdecl EmitterSetParticleLifetime(
	unsigned context, unsigned fluid, unsigned emitter, float value)
	{
	NxFluidEmitterAttributeCallbackRecord& record = nxNextAttributeRecord(
		11, context, fluid, emitter);
	record.values[0] = *reinterpret_cast<unsigned*>(&value);
	return 11;
	}

extern "C" __declspec(dllexport) float __cdecl EmitterGetParticleLifetime(
	unsigned context, unsigned fluid, unsigned emitter)
	{
	nxNextAttributeRecord(12, context, fluid, emitter);
	const unsigned bits = 0x4020000cu;
	return *reinterpret_cast<const float*>(&bits);
	}
