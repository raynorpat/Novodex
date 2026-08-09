#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <xmmintrin.h>

#include "NxException.h"
#include "NxFoundationSDK.h"
#include "NxProfiler.h"
#include "NxUtilities.h"
#include "NxMat33.h"
#include "NxBox.h"
#include "NxBounds3.h"
#include "NxPlane.h"
#include "NxCapsule.h"
#include "NxSphere.h"
#include "NxRay.h"
#include "NxSegment.h"
#include "NxVolumeIntegration.h"
#include "NxSimpleTriangleMesh.h"
#include "NxDebugRenderable.h"
#include "NxUserDebugRenderer.h"
#include "NxUserAllocator.h"

typedef void* (__thiscall *ExceptionValueCtorFn)(void*, NxErrorCode, const char*, int);
typedef void* (__thiscall *ExceptionCopyCtorFn)(void*, const void*);
typedef void* (__thiscall *ExceptionAssignFn)(void*, const void*);
typedef NxErrorCode (__thiscall *ExceptionGetErrorCodeFn)(void*);
typedef const char* (__thiscall *ExceptionGetFileFn)(void*);
typedef int (__thiscall *ExceptionGetLineFn)(void*);
typedef NxFoundationSDK* (NX_CALL_CONV *CreateFoundationSDKFn)(NxU32, NxUserOutputStream*, NxUserAllocator*);
typedef void* (__thiscall *ObservableCtorFn)(void*);
typedef void* (__thiscall *ObservableCopyCtorFn)(void*, const void*);
typedef void (__thiscall *ObservableDtorFn)(void*);
typedef void* (__thiscall *ObservableAssignFn)(void*, const void*);
typedef void (__thiscall *ObservableObserverFn)(void*, void*);
typedef void (__thiscall *ObservableNotifyFn)(void*, NxU32);
typedef unsigned (__thiscall *ObservableCountFn)(void*);
typedef void (__thiscall *ObservableEventFn)(void*, NxU32, void*);
typedef void* (__thiscall *ProfilerCtorFn)(void*, const char*);
typedef void* (__thiscall *ProfilerDefaultCtorFn)(void*);
typedef void* (__thiscall *ProfilerCopyFn)(void*, const void*);
typedef void (__thiscall *ProfilerDtorFn)(void*);
typedef void (__thiscall *ProfilerZoneMethodFn)(void*);
typedef const char* (__thiscall *ProfilerGetNameFn)(void*);
typedef void (__cdecl *ProfilerStaticVoidFn)();
typedef void (__cdecl *ProfilerGetTimeFn)(NxI64*);
typedef void (__cdecl *ProfilerSetModeFn)(NxDisplayMode);
typedef double (__cdecl *ProfilerGetStdevFn)(NxProfiler::History_Scalar*, int);
typedef int (__cdecl *ProfilerSortFn)(const void*, const void*);
typedef void* (__thiscall *SetCurrentZoneCtorFn)(void*, void*);
typedef void* (__thiscall *TimeCtorFn)(void*);
typedef void* (__thiscall *TimeAssignFn)(void*, const void*);
typedef double (__thiscall *TimeElapsedFn)(void*);
typedef double (__cdecl *TimeStaticFn)();
typedef int (__cdecl *FpuIntFn)(const float*);
typedef void (__cdecl *FpuVoidFn)();
typedef void (__cdecl *FpuExceptionsFn)(bool);
typedef void (__cdecl *ComputeBoundsFn)(NxVec3*, NxVec3*, NxU32, const NxVec3*);
typedef NxU32 (__cdecl *Crc32Fn)(const void*, NxU32);
typedef bool (__cdecl *DiagonalizeFn)(const NxMat33*, NxVec3*, NxMat33*);
typedef void (__cdecl *RotationFn)(const NxVec3*, const NxVec3*, NxMat33*);
typedef void (__cdecl *TangentsFn)(const NxVec3*, NxVec3*, NxVec3*);
typedef bool (__cdecl *BoxContainsFn)(const NxBox*, const NxVec3*);
typedef const NxU32* (__cdecl *BoxVertexToQuadFn)(NxU32);
typedef bool (__cdecl *BoxOutputFn)(const NxBox*, void*);
typedef void (__cdecl *BoxWorldNormalFn)(const NxBox*, NxU32, NxVec3*);
typedef void (__cdecl *CreateBoxFn)(NxBox*, const NxBounds3*, const NxMat34*);
typedef const NxU32* (__cdecl *GetU32TableFn)();
typedef const NxI32* (__cdecl *GetI32TableFn)();
typedef const NxVec3* (__cdecl *GetVec3TableFn)();
typedef bool (__cdecl *BoxInsideFn)(const NxBox*, const NxBox*);
typedef void (__cdecl *BoxAroundCapsuleFn)(const NxCapsule*, NxBox*);
typedef void (__cdecl *CapsuleAroundBoxFn)(const NxBox*, NxCapsule*);
typedef NxBSphereMethod (__cdecl *ComputeSphereFn)(NxSphere*, unsigned, const NxVec3*);
typedef bool (__cdecl *FastSphereFn)(NxSphere*, unsigned, const NxVec3*);
typedef void (__cdecl *MergeSpheresFn)(NxSphere*, const NxSphere*, const NxSphere*);
typedef NxF32 (__cdecl *RayDistanceFn)(const NxRay*, const NxVec3*, NxF32*);
typedef NxF32 (__cdecl *SegmentDistanceFn)(const NxSegment*, const NxVec3*, NxF32*);
typedef bool (__cdecl *VolumeIntegralsFn)(const NxSimpleTriangleMesh*, NxReal, NxIntegrals*);
typedef NxDebugRenderable* (__thiscall *CreateDebugRenderableFn)(NxFoundationSDK*);
typedef void (__thiscall *ReleaseDebugRenderableFn)(NxFoundationSDK*, NxDebugRenderable**);
typedef void (__thiscall *RenderDebugDataFn)(const NxFoundationSDK*, const NxUserDebugRenderer*);

class ClusterRecordingAllocator : public NxUserAllocator
	{
	public:
	ClusterRecordingAllocator(): calls(0), frees(0) { memset(sizes, 0, sizeof(sizes)); }
	void* mallocDEBUG(size_t size, const char*, int) { return allocate(size); }
	void* mallocDEBUG(size_t size, const char*, int, const char*, NxMemoryType) { return allocate(size); }
	void* malloc(size_t size) { return allocate(size); }
	void* malloc(size_t size, NxMemoryType) { return allocate(size); }
	void* realloc(void* memory, size_t size) { if(calls < 64) sizes[calls] = size; ++calls; return ::realloc(memory, size); }
	void free(void* memory) { ++frees; ::free(memory); }
	unsigned calls, frees;
	size_t sizes[64];
	private:
	void* allocate(size_t size) { if(calls < 64) sizes[calls] = size; ++calls; return ::malloc(size); }
	};

class ClusterDebugRenderer : public NxUserDebugRenderer
	{
	public:
	ClusterDebugRenderer(): calls(0) { memset(seen, 0, sizeof(seen)); }
	void renderData(const NxDebugRenderable& data) const
		{
		if(calls < 4) seen[calls] = &data;
		++calls;
		}
	mutable unsigned calls;
	mutable const NxDebugRenderable* seen[4];
	};

struct CallbackObserver
	{
	void** vftable;
	unsigned calls;
	NxU32 lastEvent;
	void* lastSender;
	};

static void __fastcall recordObserverEvent(CallbackObserver* self, void*, NxU32 event, void* sender)
	{
	++self->calls;
	self->lastEvent = event;
	self->lastSender = sender;
	}

static unsigned subjectEventCalls;
static NxU32 subjectLastEvent;
static void* subjectLastSender;

static void __fastcall recordSubjectEvent(void*, void*, NxU32 event, void* sender)
	{
	++subjectEventCalls;
	subjectLastEvent = event;
	subjectLastSender = sender;
	}

static FARPROC requireExport(HMODULE module, const char* name)
	{
	FARPROC address = GetProcAddress(module, name);
	if(!address)
		fprintf(stderr, "FAIL missing export %s\n", name);
	return address;
	}

static int runException(HMODULE module)
	{
	ExceptionValueCtorFn valueCtor = reinterpret_cast<ExceptionValueCtorFn>(requireExport(module, "??0Exception@NxFoundation@@QAE@W4NxErrorCode@@PBDH@Z"));
	ExceptionCopyCtorFn copyCtor = reinterpret_cast<ExceptionCopyCtorFn>(requireExport(module, "??0Exception@NxFoundation@@QAE@ABV01@@Z"));
	ExceptionAssignFn assign = reinterpret_cast<ExceptionAssignFn>(requireExport(module, "??4Exception@NxFoundation@@QAEAAV01@ABV01@@Z"));
	ExceptionGetErrorCodeFn getErrorCode = reinterpret_cast<ExceptionGetErrorCodeFn>(requireExport(module, "?getErrorCode@Exception@NxFoundation@@UAE?AW4NxErrorCode@@XZ"));
	ExceptionGetFileFn getFile = reinterpret_cast<ExceptionGetFileFn>(requireExport(module, "?getFile@Exception@NxFoundation@@UAEPBDXZ"));
	ExceptionGetLineFn getLine = reinterpret_cast<ExceptionGetLineFn>(requireExport(module, "?getLine@Exception@NxFoundation@@UAEHXZ"));
	void** exportedVftable = reinterpret_cast<void**>(requireExport(module, "??_7Exception@NxFoundation@@6B@"));
	if(!valueCtor || !copyCtor || !assign || !getErrorCode || !getFile || !getLine || !exportedVftable)
		return 1;

	unsigned char first[16] = {};
	unsigned char second[16] = {};
	unsigned char third[16] = {};
	const char* file = "exception_gate.cpp";
	if(valueCtor(first, NXE_INVALID_PARAMETER, file, 321) != first)
		return fprintf(stderr, "FAIL Exception value constructor return\n"), 1;
	if(*reinterpret_cast<void***>(first) != exportedVftable ||
		exportedVftable[0] != reinterpret_cast<void*>(getErrorCode) ||
		exportedVftable[1] != reinterpret_cast<void*>(getFile) ||
		exportedVftable[2] != reinterpret_cast<void*>(getLine))
		return fprintf(stderr, "FAIL Exception vftable order\n"), 1;
	if(getErrorCode(first) != NXE_INVALID_PARAMETER || getFile(first) != file || getLine(first) != 321)
		return fprintf(stderr, "FAIL Exception getters\n"), 1;
	if(copyCtor(second, first) != second || memcmp(first, second, sizeof(first)))
		return fprintf(stderr, "FAIL Exception copy constructor\n"), 1;
	valueCtor(third, NXE_NO_ERROR, 0, -1);
	if(assign(third, first) != third || memcmp(first, third, sizeof(first)))
		return fprintf(stderr, "FAIL Exception assignment\n"), 1;

	printf("exception size=16 fields=4,8,12 getters=%d,%s,%d copy=exact assign=exact vftable=3_ordered\n",
		static_cast<int>(getErrorCode(first)), getFile(first), getLine(first));
	return 0;
	}

static int runObservable(HMODULE module)
	{
	CreateFoundationSDKFn createSDK = reinterpret_cast<CreateFoundationSDKFn>(requireExport(module, "NxCreateFoundationSDK"));
	ObservableCtorFn ctor = reinterpret_cast<ObservableCtorFn>(requireExport(module, "??0Observable@NxFoundation@@QAE@XZ"));
	ObservableCopyCtorFn copyCtor = reinterpret_cast<ObservableCopyCtorFn>(requireExport(module, "??0Observable@NxFoundation@@QAE@ABV01@@Z"));
	ObservableDtorFn dtor = reinterpret_cast<ObservableDtorFn>(requireExport(module, "??1Observable@NxFoundation@@QAE@XZ"));
	ObservableAssignFn assign = reinterpret_cast<ObservableAssignFn>(requireExport(module, "??4Observable@NxFoundation@@QAEAAV01@ABV01@@Z"));
	ObservableObserverFn addObserver = reinterpret_cast<ObservableObserverFn>(requireExport(module, "?addObserver@Observable@NxFoundation@@QAEXAAV12@@Z"));
	ObservableEventFn event = reinterpret_cast<ObservableEventFn>(requireExport(module, "?event@Observable@NxFoundation@@UAEXIAAV12@@Z"));
	ObservableCountFn getCount = reinterpret_cast<ObservableCountFn>(requireExport(module, "?getNumObservers@Observable@NxFoundation@@QAEIXZ"));
	ObservableNotifyFn notify = reinterpret_cast<ObservableNotifyFn>(requireExport(module, "?notifyObservers@Observable@NxFoundation@@QAEXI@Z"));
	ObservableObserverFn removeObserver = reinterpret_cast<ObservableObserverFn>(requireExport(module, "?removeObserver@Observable@NxFoundation@@QAEXAAV12@@Z"));
	void** exportedVftable = reinterpret_cast<void**>(requireExport(module, "??_7Observable@NxFoundation@@6B@"));
	if(!createSDK || !ctor || !copyCtor || !dtor || !assign || !addObserver || !event || !getCount || !notify || !removeObserver || !exportedVftable)
		return 1;
	NxFoundationSDK* sdk = createSDK(NX_FOUNDATION_SDK_VERSION, 0, 0);
	if(!sdk)
		return fprintf(stderr, "FAIL Observable SDK setup\n"), 1;

	void* callbackVftable[] = { reinterpret_cast<void*>(&recordObserverEvent) };
	CallbackObserver observerA = { callbackVftable, 0, 0, 0 };
	CallbackObserver observerB = { callbackVftable, 0, 0, 0 };
	unsigned char subject[16] = {};
	unsigned char copied[16] = {};
	unsigned char assigned[16] = {};
	if(ctor(subject) != subject || *reinterpret_cast<void***>(subject) != exportedVftable ||
		exportedVftable[0] != reinterpret_cast<void*>(event) || getCount(subject) != 0)
		return fprintf(stderr, "FAIL Observable construction/vftable\n"), 1;
	event(subject, 17, subject);
	addObserver(subject, &observerA);
	addObserver(subject, &observerB);
	if(getCount(subject) != 2)
		return fprintf(stderr, "FAIL Observable add/count\n"), 1;
	notify(subject, 0x1234);
	if(observerA.calls != 1 || observerB.calls != 1 || observerA.lastEvent != 0x1234 ||
		observerB.lastEvent != 0x1234 || observerA.lastSender != subject || observerB.lastSender != subject)
		return fprintf(stderr, "FAIL Observable notify callback\n"), 1;
	removeObserver(subject, &observerA);
	if(getCount(subject) != 1)
		return fprintf(stderr, "FAIL Observable remove first\n"), 1;
	notify(subject, 0x5678);
	if(observerA.calls != 1 || observerB.calls != 2 || observerB.lastEvent != 0x5678)
		return fprintf(stderr, "FAIL Observable removal routing\n"), 1;

	if(copyCtor(copied, subject) != copied || getCount(copied) != 1 ||
		reinterpret_cast<void**>(copied)[1] == reinterpret_cast<void**>(subject)[1])
		return fprintf(stderr, "FAIL Observable copy ownership\n"), 1;
	ctor(assigned);
	if(assign(assigned, subject) != assigned || getCount(assigned) != 1 ||
		reinterpret_cast<void**>(assigned)[1] == reinterpret_cast<void**>(subject)[1])
		return fprintf(stderr, "FAIL Observable assignment ownership\n"), 1;
	void* subjectCallbackVftable[] = { reinterpret_cast<void*>(&recordSubjectEvent) };
	*reinterpret_cast<void***>(subject) = subjectCallbackVftable;
	removeObserver(subject, &observerB);
	if(getCount(subject) != 0 || subjectEventCalls != 1 || subjectLastEvent != 2 || subjectLastSender != subject)
		return fprintf(stderr, "FAIL Observable remove last\n"), 1;

	printf("observable size=16 fields=4,8,12 vftable=1 count=0,2,1,0 callbacks=1,2 copy=deep assign=deep last_remove=empty\n");
	dtor(assigned);
	dtor(copied);
	dtor(subject);
	sdk->release();
	return 0;
	}

static int runProfiler(HMODULE module)
	{
	static const char* exports[] = {
		"??0DefineZone@NxProfiler@@QAE@ABV01@@Z", "??0DefineZone@NxProfiler@@QAE@PBD@Z",
		"??0DefineZone@NxProfiler@@QAE@XZ", "??0SetCurrentZone@NxProfiler@@QAE@AAVDefineZone@1@@Z",
		"??1DefineZone@NxProfiler@@QAE@XZ", "??1SetCurrentZone@NxProfiler@@QAE@XZ",
		"??4DefineZone@NxProfiler@@QAEAAV01@ABV01@@Z", "??4NxProfiler@@QAEAAV0@ABV0@@Z",
		"??4SetCurrentZone@NxProfiler@@QAEAAV01@ABV01@@Z", "??_7DefineZone@NxProfiler@@6B@",
		"?Enter_Zone@NxProfiler@@CAXAAVDefineZone@1@@Z", "?Exit_Zone@NxProfiler@@CAXXZ",
		"?createProfilingZone@FoundationSDK@NxFoundation@@UAEPAVNxProfilingZone@@PBD@Z",
		"?current_integer_timestamp@NxProfiler@@0_JA", "?current_zone@NxProfiler@@0HA",
		"?data_records@NxProfiler@@0PAUProfile_Tracker_Data_Record@1@A", "?defaultZone@NxProfiler@@0VDefineZone@1@A",
		"?displayed_quantity@NxProfiler@@2W4NxDisplayMode@@A", "?dt@NxProfiler@@0NA",
		"?dt_per_integer_timestamp@NxProfiler@@0NA", "?enter@DefineZone@NxProfiler@@UAEXXZ",
		"?frame_time@NxProfiler@@2UHistory_Scalar@1@A", "?getName@DefineZone@NxProfiler@@QAEPBDXZ",
		"?getTime@NxProfiler@@CAXPA_J@Z", "?get_stdev@NxProfiler@@CANPAUHistory_Scalar@1@H@Z",
		"?initialize@NxProfiler@@SAXXZ", "?initializeHighLevel@NxProfiler@@CAXXZ",
		"?initialized@NxProfiler@@0_NA", "?integer_timestamps_per_second@NxProfiler@@2UHistory_Scalar@1@A",
		"?last_integer_timestamp@NxProfiler@@0_JA", "?last_update_time@NxProfiler@@0NA",
		"?leave@DefineZone@NxProfiler@@UAEXXZ", "?maxZones@NxProfiler@@2HA",
		"?num_active_zones@NxProfiler@@2HA", "?num_zones@NxProfiler@@0HA",
		"?pointersData@NxProfiler@@0PAPAVDefineZone@1@A", "?precomputed_factors@NxProfiler@@0PANA",
		"?release@DefineZone@NxProfiler@@UAEXXZ", "?setMode@NxProfiler@@SAXW4NxDisplayMode@@@Z",
		"?sort_records@NxProfiler@@CAHPBX0@Z", "?sorted_pointers@NxProfiler@@2PAPAUProfile_Tracker_Data_Record@1@A",
		"?stackData@NxProfiler@@0PAHA", "?stackSize@NxProfiler@@0HA", "?stack_pos@NxProfiler@@0HA",
		"?times_to_reach_90_percent@NxProfiler@@0PANA", "?update@NxProfiler@@SAXXZ",
		"?update_index@NxProfiler@@0HA", "?zone_pointers_by_index@NxProfiler@@2PAPAVDefineZone@1@A",
		"?zone_stack@NxProfiler@@0PAHA"
	};
	for(unsigned i = 0; i < sizeof(exports) / sizeof(exports[0]); ++i)
		if(!requireExport(module, exports[i])) return 1;

	CreateFoundationSDKFn createSDK = reinterpret_cast<CreateFoundationSDKFn>(requireExport(module, "NxCreateFoundationSDK"));
	ProfilerCtorFn ctor = reinterpret_cast<ProfilerCtorFn>(requireExport(module, exports[1]));
	ProfilerDefaultCtorFn defaultCtor = reinterpret_cast<ProfilerDefaultCtorFn>(requireExport(module, exports[2]));
	ProfilerCopyFn copyCtor = reinterpret_cast<ProfilerCopyFn>(requireExport(module, exports[0]));
	SetCurrentZoneCtorFn scopeCtor = reinterpret_cast<SetCurrentZoneCtorFn>(requireExport(module, exports[3]));
	ProfilerDtorFn dtor = reinterpret_cast<ProfilerDtorFn>(requireExport(module, exports[4]));
	ProfilerDtorFn scopeDtor = reinterpret_cast<ProfilerDtorFn>(requireExport(module, exports[5]));
	ProfilerCopyFn assign = reinterpret_cast<ProfilerCopyFn>(requireExport(module, exports[6]));
	ProfilerCopyFn profilerAssign = reinterpret_cast<ProfilerCopyFn>(requireExport(module, exports[7]));
	ProfilerCopyFn scopeAssign = reinterpret_cast<ProfilerCopyFn>(requireExport(module, exports[8]));
	void** vftable = reinterpret_cast<void**>(requireExport(module, exports[9]));
	ProfilerStaticVoidFn enterZone = reinterpret_cast<ProfilerStaticVoidFn>(requireExport(module, exports[10]));
	ProfilerStaticVoidFn exitZone = reinterpret_cast<ProfilerStaticVoidFn>(requireExport(module, exports[11]));
	ProfilerZoneMethodFn enter = reinterpret_cast<ProfilerZoneMethodFn>(requireExport(module, exports[20]));
	ProfilerGetNameFn getName = reinterpret_cast<ProfilerGetNameFn>(requireExport(module, exports[22]));
	ProfilerGetTimeFn getTime = reinterpret_cast<ProfilerGetTimeFn>(requireExport(module, exports[23]));
	ProfilerGetStdevFn getStdev = reinterpret_cast<ProfilerGetStdevFn>(requireExport(module, exports[24]));
	ProfilerStaticVoidFn initialize = reinterpret_cast<ProfilerStaticVoidFn>(requireExport(module, exports[25]));
	ProfilerStaticVoidFn initializeHighLevel = reinterpret_cast<ProfilerStaticVoidFn>(requireExport(module, exports[26]));
	ProfilerZoneMethodFn leave = reinterpret_cast<ProfilerZoneMethodFn>(requireExport(module, exports[31]));
	ProfilerZoneMethodFn release = reinterpret_cast<ProfilerZoneMethodFn>(requireExport(module, exports[37]));
	ProfilerSetModeFn setMode = reinterpret_cast<ProfilerSetModeFn>(requireExport(module, exports[38]));
	ProfilerSortFn sortRecords = reinterpret_cast<ProfilerSortFn>(requireExport(module, exports[39]));
	ProfilerStaticVoidFn update = reinterpret_cast<ProfilerStaticVoidFn>(requireExport(module, exports[45]));
	if(!createSDK) return 1;

	bool* initialized = reinterpret_cast<bool*>(requireExport(module, exports[27]));
	int* currentZone = reinterpret_cast<int*>(requireExport(module, exports[14]));
	NxDisplayMode* mode = reinterpret_cast<NxDisplayMode*>(requireExport(module, exports[17]));
	int* maxZones = reinterpret_cast<int*>(requireExport(module, exports[32]));
	int* numActive = reinterpret_cast<int*>(requireExport(module, exports[33]));
	int* numZones = reinterpret_cast<int*>(requireExport(module, exports[34]));
	int* stackSize = reinterpret_cast<int*>(requireExport(module, exports[42]));
	int* stackPos = reinterpret_cast<int*>(requireExport(module, exports[43]));
	int* updateIndex = reinterpret_cast<int*>(requireExport(module, exports[46]));
	void*** zonePointersVariable = reinterpret_cast<void***>(requireExport(module, exports[47]));
	void* defaultZone = reinterpret_cast<void*>(requireExport(module, exports[16]));
	if(!*initialized || *currentZone != 0 || *stackPos != 0 || *numZones != 0 ||
		*maxZones != NX_MAX_PROFILING_STACK_DEPTH || *stackSize != NX_MAX_PROFILING_ZONES ||
		!zonePointersVariable || !*zonePointersVariable || (*zonePointersVariable)[0] != defaultZone)
		return fprintf(stderr, "FAIL Profiler initialized globals\n"), 1;
	initialize();
	int initialUpdateIndex = *updateIndex;
	initializeHighLevel();
	if(*updateIndex != 0 || *mode != NX_SELF_TIME)
		return fprintf(stderr, "FAIL Profiler high-level initialization\n"), 1;
	setMode(NX_HIERARCHICAL_STDEV);
	if(*mode != NX_HIERARCHICAL_STDEV)
		return fprintf(stderr, "FAIL Profiler mode\n"), 1;

	unsigned char zone[56] = {};
	unsigned char copied[56] = {};
	unsigned char assigned[56] = {};
	if(ctor(zone, "profiler_gate") != zone || *reinterpret_cast<void***>(zone) != vftable ||
		strcmp(getName(zone), "profiler_gate") || *numZones != 1 || reinterpret_cast<int*>(zone)[2] != 1)
		return fprintf(stderr, "FAIL DefineZone construction\n"), 1;
	if(vftable[0] != reinterpret_cast<void*>(release) || vftable[1] != reinterpret_cast<void*>(enter) ||
		vftable[2] != reinterpret_cast<void*>(leave))
		return fprintf(stderr, "FAIL DefineZone vftable order\n"), 1;
	if(copyCtor(copied, zone) != copied || assign(assigned, zone) != assigned ||
		memcmp(copied, zone, 12) || memcmp(copied + 16, zone + 16, 36) ||
		memcmp(assigned + 4, zone + 4, 8) || memcmp(assigned + 16, zone + 16, 36))
		return fprintf(stderr, "FAIL DefineZone copy/assignment\n"), 1;
	unsigned char profilerA[1] = {}, profilerB[1] = {};
	if(profilerAssign(profilerA, profilerB) != profilerA)
		return fprintf(stderr, "FAIL NxProfiler assignment\n"), 1;

	NxI64 tickA = 0, tickB = 0;
	getTime(&tickA);
	enter(zone);
	leave(zone);
	getTime(&tickB);
	if(tickA <= 0 || tickB < tickA || *currentZone != 0 || *stackPos != 0 || reinterpret_cast<int*>(zone)[12] != 1)
		return fprintf(stderr, "FAIL Profiler enter/leave\n"), 1;
	unsigned char scope[1] = {}, scopeCopy[1] = {};
	if(scopeCtor(scope, zone) != scope || *currentZone != 1 || *stackPos != 1 ||
		scopeAssign(scopeCopy, scope) != scopeCopy)
		return fprintf(stderr, "FAIL SetCurrentZone construction/assignment\n"), 1;
	scopeDtor(scope);
	if(*currentZone != 0 || *stackPos != 0 || reinterpret_cast<int*>(zone)[12] != 2)
		return fprintf(stderr, "FAIL SetCurrentZone destruction\n"), 1;

	NxProfiler::History_Scalar scalar = {};
	scalar.values[1] = 2.0;
	scalar.variances[1] = 13.0;
	double stdev = getStdev(&scalar, 1);
	if(stdev < 2.999999 || stdev > 3.000001)
		return fprintf(stderr, "FAIL Profiler stdev\n"), 1;
	NxProfiler::Profile_Tracker_Data_Record recordA = {}, recordB = {};
	recordA.displayed_quantity = 2.0;
	recordB.displayed_quantity = 7.0;
	NxProfiler::Profile_Tracker_Data_Record* pointerA = &recordA;
	NxProfiler::Profile_Tracker_Data_Record* pointerB = &recordB;
	if(sortRecords(&pointerA, &pointerB) != 1 || sortRecords(&pointerB, &pointerA) != -1 || sortRecords(&pointerA, &pointerA) != 0)
		return fprintf(stderr, "FAIL Profiler sort\n"), 1;

	update();
	if(*updateIndex != 1 || *numActive < 1)
		return fprintf(stderr, "FAIL Profiler update\n"), 1;
	dtor(zone);
	if(*numZones != 0)
		return fprintf(stderr, "FAIL DefineZone destruction\n"), 1;
	unsigned char defaultConstructed[56] = {};
	if(defaultCtor(defaultConstructed) != defaultConstructed || *reinterpret_cast<void***>(defaultConstructed) != vftable)
		return fprintf(stderr, "FAIL DefineZone default construction\n"), 1;

	NxFoundationSDK* sdk = createSDK(NX_FOUNDATION_SDK_VERSION, 0, 0);
	if(!sdk)
		return fprintf(stderr, "FAIL Profiler SDK setup\n"), 1;
	NxProfilingZone* dynamicZone = sdk->createProfilingZone("dynamic_gate");
	if(!dynamicZone || *numZones != 1)
		return fprintf(stderr, "FAIL Profiler dynamic zone\n"), 1;
	release(dynamicZone);
	if(*numZones != 0)
		return fprintf(stderr, "FAIL Profiler release\n"), 1;
	sdk->release();

	printf("profiler exports=49 define_zone_size=56 vftable=3_ordered globals=initialized,0,0,0,256,64 zone=define,enter,leave,scope,release copy=fields_exact assign=fields_exact stdev=3 sort=1,-1,0 update=%d_to_1 active=positive\n", initialUpdateIndex);
	return 0;
	}

static int runTime(HMODULE module)
	{
	TimeCtorFn ctor = reinterpret_cast<TimeCtorFn>(requireExport(module, "??0Time@NxFoundation@@QAE@XZ"));
	TimeAssignFn assign = reinterpret_cast<TimeAssignFn>(requireExport(module, "??4Time@NxFoundation@@QAEAAV01@ABV01@@Z"));
	TimeStaticFn frequency = reinterpret_cast<TimeStaticFn>(requireExport(module, "?GetClockFrequency@Time@NxFoundation@@CANXZ"));
	TimeElapsedFn elapsed = reinterpret_cast<TimeElapsedFn>(requireExport(module, "?GetElapsedSeconds@Time@NxFoundation@@QAENXZ"));
	TimeStaticFn ticks = reinterpret_cast<TimeStaticFn>(requireExport(module, "?GetTimeTicks@Time@NxFoundation@@CANXZ"));
	TimeElapsedFn peek = reinterpret_cast<TimeElapsedFn>(requireExport(module, "?PeekElapsedSeconds@Time@NxFoundation@@QAENXZ"));
	if(!ctor || !assign || !frequency || !elapsed || !ticks || !peek)
		return 1;
	double hzA = frequency();
	double hzB = frequency();
	double tickA = ticks();
	double tickB = ticks();
	if(hzA <= 0 || hzB != hzA || tickA <= 0 || tickB < tickA)
		return fprintf(stderr, "FAIL Time clock source\n"), 1;

	unsigned char timer[8] = {};
	unsigned char assigned[8] = {};
	if(ctor(timer) != timer)
		return fprintf(stderr, "FAIL Time construction\n"), 1;
	Sleep(15);
	double peekA = peek(timer);
	Sleep(15);
	double peekB = peek(timer);
	double elapsedA = elapsed(timer);
	double resetPeek = peek(timer);
	if(peekA < 0.005 || peekA > 1.0 || peekB < peekA || peekB > 1.0 ||
		elapsedA < peekA || elapsedA > 1.0 || resetPeek < 0 || resetPeek > 0.25)
		return fprintf(stderr, "FAIL Time elapsed/peek invariants\n"), 1;
	if(assign(assigned, timer) != assigned || memcmp(assigned, timer, sizeof(timer)))
		return fprintf(stderr, "FAIL Time assignment\n"), 1;
	Sleep(5);
	double originalPeek = peek(timer);
	double assignedPeek = peek(assigned);
	double difference = originalPeek > assignedPeek ? originalPeek - assignedPeek : assignedPeek - originalPeek;
	if(originalPeek < 0 || assignedPeek < 0 || difference > 0.05)
		return fprintf(stderr, "FAIL Time assigned epoch\n"), 1;

	printf("time size=8 frequency=positive_stable ticks=positive_monotonic peek=nonreset_monotonic elapsed=resets assignment=epoch_exact windows=bounded\n");
	return 0;
	}

static unsigned short getX87ControlWord()
	{
	unsigned short controlWord;
	__asm
		{
		fnstcw controlWord
		}
	return controlWord;
	}

static int runFpu(HMODULE module)
	{
	FpuIntFn ceilFn = reinterpret_cast<FpuIntFn>(requireExport(module, "NxIntCeil"));
	FpuIntFn chopFn = reinterpret_cast<FpuIntFn>(requireExport(module, "NxIntChop"));
	FpuIntFn floorFn = reinterpret_cast<FpuIntFn>(requireExport(module, "NxIntFloor"));
	FpuExceptionsFn exceptions = reinterpret_cast<FpuExceptionsFn>(requireExport(module, "NxSetFPUExceptions"));
	FpuVoidFn precision24 = reinterpret_cast<FpuVoidFn>(requireExport(module, "NxSetFPUPrecision24"));
	FpuVoidFn precision53 = reinterpret_cast<FpuVoidFn>(requireExport(module, "NxSetFPUPrecision53"));
	FpuVoidFn precision64 = reinterpret_cast<FpuVoidFn>(requireExport(module, "NxSetFPUPrecision64"));
	FpuVoidFn roundingChop = reinterpret_cast<FpuVoidFn>(requireExport(module, "NxSetFPURoundingChop"));
	FpuVoidFn roundingDown = reinterpret_cast<FpuVoidFn>(requireExport(module, "NxSetFPURoundingDown"));
	FpuVoidFn roundingNear = reinterpret_cast<FpuVoidFn>(requireExport(module, "NxSetFPURoundingNear"));
	FpuVoidFn roundingUp = reinterpret_cast<FpuVoidFn>(requireExport(module, "NxSetFPURoundingUp"));
	if(!ceilFn || !chopFn || !floorFn || !exceptions || !precision24 || !precision53 || !precision64 ||
		!roundingChop || !roundingDown || !roundingNear || !roundingUp)
		return 1;

	const float values[] = { -3.75f, -1.0f, -0.25f, 0.0f, 0.25f, 1.0f, 3.75f };
	const int expectedChop[] = { -3, -1, 0, 0, 0, 1, 3 };
	const int expectedFloor[] = { -4, -1, -1, 0, 0, 1, 3 };
	const int expectedCeil[] = { -3, -1, 0, 0, 1, 1, 4 };
	for(unsigned i = 0; i < sizeof(values) / sizeof(values[0]); ++i)
		if(chopFn(&values[i]) != expectedChop[i] || floorFn(&values[i]) != expectedFloor[i] || ceilFn(&values[i]) != expectedCeil[i])
			return fprintf(stderr, "FAIL FPU integer conversion vector %u\n", i), 1;

	unsigned savedControl = _controlfp(0, 0);
	unsigned savedMxcsr = _mm_getcsr();
	precision24();
	if((getX87ControlWord() & 0x0300) != 0x0000 || (_mm_getcsr() & 0x6000) != (savedMxcsr & 0x6000))
		return fprintf(stderr, "FAIL FPU precision24\n"), 1;
	precision53();
	if((getX87ControlWord() & 0x0300) != 0x0200)
		return fprintf(stderr, "FAIL FPU precision53\n"), 1;
	precision64();
	if((getX87ControlWord() & 0x0300) != 0x0300)
		return fprintf(stderr, "FAIL FPU precision64\n"), 1;
	roundingChop();
	if((getX87ControlWord() & 0x0c00) != 0x0c00 || _mm_getcsr() != savedMxcsr)
		return fprintf(stderr, "FAIL FPU rounding chop x87=%04x mxcsr=%08x\n", getX87ControlWord(), _mm_getcsr()), 1;
	roundingUp();
	if((getX87ControlWord() & 0x0c00) != 0x0800 || _mm_getcsr() != savedMxcsr)
		return fprintf(stderr, "FAIL FPU rounding up\n"), 1;
	roundingDown();
	if((getX87ControlWord() & 0x0c00) != 0x0400 || _mm_getcsr() != savedMxcsr)
		return fprintf(stderr, "FAIL FPU rounding down\n"), 1;
	roundingNear();
	if((getX87ControlWord() & 0x0c00) != 0 || _mm_getcsr() != savedMxcsr)
		return fprintf(stderr, "FAIL FPU rounding near\n"), 1;
	exceptions(true);
	if((getX87ControlWord() & 0x003f) != 0x003f || _mm_getcsr() != savedMxcsr)
		return fprintf(stderr, "FAIL FPU exception true masks\n"), 1;
	exceptions(false);
	if((getX87ControlWord() & 0x003f) != 0x0002 || _mm_getcsr() != savedMxcsr)
		return fprintf(stderr, "FAIL FPU exception false masks x87=%04x mxcsr=%08x\n", getX87ControlWord(), _mm_getcsr()), 1;

	_controlfp(savedControl, _MCW_PC | _MCW_RC | _MCW_EM);
	_mm_setcsr(savedMxcsr);
	if((_controlfp(0, 0) & (_MCW_PC | _MCW_RC | _MCW_EM)) != (savedControl & (_MCW_PC | _MCW_RC | _MCW_EM)) ||
		_mm_getcsr() != savedMxcsr)
		return fprintf(stderr, "FAIL FPU restoration\n"), 1;
	printf("fpu exports=11 ints=7_vectors precision=x87_24,53,64 rounding=x87_chop,up,down,near exceptions=x87_true_masked,false_unmasked mxcsr=unchanged restoration=exact\n");
	return 0;
	}

static bool nearFloat(float a, float b, float tolerance = 1.0e-4f)
	{
	float difference = a > b ? a - b : b - a;
	return difference <= tolerance;
	}

static bool nearVector(const NxVec3& a, const NxVec3& b, float tolerance = 1.0e-4f)
	{
	return nearFloat(a.x, b.x, tolerance) && nearFloat(a.y, b.y, tolerance) && nearFloat(a.z, b.z, tolerance);
	}

static NxVec3 multiplyMatrix(const NxMat33& matrix, const NxVec3& vector)
	{
	return matrix * vector;
	}

static int runUtil(HMODULE module)
	{
	ComputeBoundsFn bounds = reinterpret_cast<ComputeBoundsFn>(requireExport(module, "NxComputeBounds"));
	Crc32Fn crc = reinterpret_cast<Crc32Fn>(requireExport(module, "NxCrc32"));
	DiagonalizeFn diagonalize = reinterpret_cast<DiagonalizeFn>(requireExport(module, "NxDiagonalizeInertiaTensor"));
	RotationFn rotation = reinterpret_cast<RotationFn>(requireExport(module, "NxFindRotationMatrix"));
	TangentsFn tangents = reinterpret_cast<TangentsFn>(requireExport(module, "NxNormalToTangents"));
	if(!bounds || !crc || !diagonalize || !rotation || !tangents)
		return 1;

	unsigned char bytes256[256];
	for(unsigned i = 0; i < 256; ++i) bytes256[i] = static_cast<unsigned char>(i);
	unsigned char ff = 0xff;
	const char embedded[] = "abc\0def";
	char repeated[4096];
	memset(repeated, 'a', sizeof(repeated));
	if(crc(0, 0) != 0 || crc("123456789", 9) != 0x2dfd2d88 || crc(bytes256, 256) != 0x2493092b ||
		crc(&ff, 1) != 0x2d02ef8d || crc(embedded, 7) != 0x45eadcee || crc(repeated, sizeof(repeated)) != 0x5b85dc62)
		return fprintf(stderr, "FAIL utility CRC vectors\n"), 1;

	NxVec3 minimum(9, 8, 7), maximum(6, 5, 4);
	bounds(&minimum, &maximum, 0, 0);
	if(!nearVector(minimum, NxVec3(9, 8, 7)) || !nearVector(maximum, NxVec3(6, 5, 4)))
		return fprintf(stderr, "FAIL utility empty bounds\n"), 1;
	NxVec3 vertices[] = { NxVec3(3, -2, 9), NxVec3(-4, 8, 1), NxVec3(3, 8, -5), NxVec3(-4, -2, 9) };
	bounds(&minimum, &maximum, 4, vertices);
	if(!nearVector(minimum, NxVec3(-4, -2, -5)) || !nearVector(maximum, NxVec3(3, 8, 9)))
		return fprintf(stderr, "FAIL utility bounds\n"), 1;

	NxVec3 normals[] = { NxVec3(0, 0, 1), NxVec3(0.6f, 0.8f, 0) };
	for(unsigned i = 0; i < 2; ++i)
		{
		NxVec3 tangent1, tangent2;
		tangents(&normals[i], &tangent1, &tangent2);
		if(!nearFloat(tangent1.magnitude(), 1.0f) || !nearFloat(tangent2.magnitude(), 1.0f) ||
			!nearFloat(normals[i].dot(tangent1), 0) || !nearFloat(normals[i].dot(tangent2), 0) || !nearFloat(tangent1.dot(tangent2), 0))
			return fprintf(stderr, "FAIL utility tangent branch %u\n", i), 1;
		}

	NxVec3 from(1, 0, 0);
	NxVec3 targets[] = { NxVec3(0, 1, 0), NxVec3(1, 0, 0), NxVec3(-1, 0, 0) };
	for(unsigned i = 0; i < 3; ++i)
		{
		NxMat33 matrix;
		rotation(&from, &targets[i], &matrix);
		if(!nearVector(multiplyMatrix(matrix, from), targets[i], 2.0e-4f))
			return fprintf(stderr, "FAIL utility rotation branch %u matrix=%g,%g,%g;%g,%g,%g;%g,%g,%g product=%g,%g,%g\n", i,
				matrix(0,0), matrix(1,0), matrix(2,0), matrix(0,1), matrix(1,1), matrix(2,1), matrix(0,2), matrix(1,2), matrix(2,2),
				multiplyMatrix(matrix, from).x, multiplyMatrix(matrix, from).y, multiplyMatrix(matrix, from).z), 1;
		}

	NxMat33 diagonalMatrix;
	diagonalMatrix.zero();
	diagonalMatrix(0, 0) = 2; diagonalMatrix(1, 1) = 3; diagonalMatrix(2, 2) = 5;
	NxVec3 diagonal;
	NxMat33 axes;
	if(!diagonalize(&diagonalMatrix, &diagonal, &axes) || !nearVector(diagonal, NxVec3(2, 3, 5)))
		return fprintf(stderr, "FAIL utility diagonal inertia\n"), 1;
	NxMat33 symmetric;
	symmetric(0, 0) = 4; symmetric(1, 0) = 1; symmetric(2, 0) = 0;
	symmetric(0, 1) = 1; symmetric(1, 1) = 3; symmetric(2, 1) = 0;
	symmetric(0, 2) = 0; symmetric(1, 2) = 0; symmetric(2, 2) = 2;
	if(!diagonalize(&symmetric, &diagonal, &axes) || diagonal.x <= 0 || diagonal.y <= 0 || diagonal.z <= 0 ||
		!nearFloat(diagonal.x + diagonal.y + diagonal.z, 9.0f, 2.0e-3f))
		return fprintf(stderr, "FAIL utility symmetric inertia\n"), 1;

	printf("util exports=5 crc=empty,canonical,byte_domain,ff,embedded_nul,4096 bounds=empty_unchanged,extrema tangents=both_branches_orthonormal rotation=general,parallel,antiparallel inertia=diagonal,symmetric_positive_trace\n");
	return 0;
	}

static int runBox(HMODULE module)
	{
	BoxContainsFn contains = reinterpret_cast<BoxContainsFn>(requireExport(module, "NxBoxContainsPoint"));
	BoxVertexToQuadFn vertexToQuad = reinterpret_cast<BoxVertexToQuadFn>(requireExport(module, "NxBoxVertexToQuad"));
	BoxOutputFn planesFn = reinterpret_cast<BoxOutputFn>(requireExport(module, "NxComputeBoxPlanes"));
	BoxOutputFn pointsFn = reinterpret_cast<BoxOutputFn>(requireExport(module, "NxComputeBoxPoints"));
	BoxOutputFn vertexNormalsFn = reinterpret_cast<BoxOutputFn>(requireExport(module, "NxComputeBoxVertexNormals"));
	BoxWorldNormalFn worldNormalFn = reinterpret_cast<BoxWorldNormalFn>(requireExport(module, "NxComputeBoxWorldEdgeNormal"));
	CreateBoxFn create = reinterpret_cast<CreateBoxFn>(requireExport(module, "NxCreateBox"));
	GetU32TableFn edgesFn = reinterpret_cast<GetU32TableFn>(requireExport(module, "NxGetBoxEdges"));
	GetI32TableFn axesFn = reinterpret_cast<GetI32TableFn>(requireExport(module, "NxGetBoxEdgesAxes"));
	GetVec3TableFn localNormalsFn = reinterpret_cast<GetVec3TableFn>(requireExport(module, "NxGetBoxLocalEdgeNormals"));
	GetU32TableFn quadsFn = reinterpret_cast<GetU32TableFn>(requireExport(module, "NxGetBoxQuads"));
	GetU32TableFn trianglesFn = reinterpret_cast<GetU32TableFn>(requireExport(module, "NxGetBoxTriangles"));
	BoxInsideFn inside = reinterpret_cast<BoxInsideFn>(requireExport(module, "NxIsBoxAInsideBoxB"));
	if(!contains || !vertexToQuad || !planesFn || !pointsFn || !vertexNormalsFn || !worldNormalFn || !create ||
		!edgesFn || !axesFn || !localNormalsFn || !quadsFn || !trianglesFn || !inside)
		return 1;

	NxMat33 identity; identity.id();
	NxBox box(NxVec3(1, 2, 3), NxVec3(2, 3, 4), identity);
	NxVec3 center(1, 2, 3), interior(2.999f, 2, 3), boundary(3, 2, 3), exterior(3.001f, 2, 3);
	if(!contains(&box, &center) || !contains(&box, &interior) || contains(&box, &boundary) || contains(&box, &exterior))
		return fprintf(stderr, "FAIL box point containment\n"), 1;
	NxBox degenerate(NxVec3(0,0,0), NxVec3(0,1,1), identity);
	NxVec3 origin(0,0,0);
	if(contains(&degenerate, &origin))
		return fprintf(stderr, "FAIL box degenerate boundary\n"), 1;

	NxBounds3 bounds;
	bounds.set(NxVec3(-1,-2,-3), NxVec3(3,4,5));
	NxMat34 transform(identity, NxVec3(10,20,30));
	NxBox created;
	create(&created, &bounds, &transform);
	if(!nearVector(created.center, NxVec3(11,21,31)) || !nearVector(created.extents, NxVec3(2,3,4)))
		return fprintf(stderr, "FAIL box creation\n"), 1;

	NxPlane planes[6]; NxVec3 points[8], normals[8];
	if(planesFn(&box, 0) || pointsFn(&box, 0) || vertexNormalsFn(&box, 0) ||
		!planesFn(&box, planes) || !pointsFn(&box, points) || !vertexNormalsFn(&box, normals))
		return fprintf(stderr, "FAIL box output null/success\n"), 1;
	if(!nearVector(points[0], NxVec3(-1,-1,-1)) || !nearVector(points[6], NxVec3(3,5,7)) ||
		!nearVector(planes[0].normal, NxVec3(1,0,0)) || !nearFloat(planes[0].distance(center), -2.0f) ||
		!nearFloat(normals[0].magnitude(), 1.0f))
		return fprintf(stderr, "FAIL box points/planes/normals\n"), 1;

	static const NxU32 expectedEdges[24] = {0,1,1,2,2,3,3,0,7,6,6,5,5,4,4,7,1,5,6,2,3,7,4,0};
	static const NxI32 expectedAxes[12] = {1,2,-1,-2,1,-2,-1,2,3,-3,3,-3};
	static const NxU32 expectedQuads[24] = {1,2,6,5,2,3,7,6,4,5,6,7,0,4,7,3,0,1,5,4,0,3,2,1};
	static const NxU32 expectedTriangles[36] = {0,2,1,0,3,2,1,6,5,1,2,6,5,7,4,5,6,7,4,3,0,4,7,3,3,6,2,3,7,6,5,0,1,5,4,0};
	if(memcmp(edgesFn(), expectedEdges, sizeof(expectedEdges)) || memcmp(axesFn(), expectedAxes, sizeof(expectedAxes)) ||
		memcmp(quadsFn(), expectedQuads, sizeof(expectedQuads)) || memcmp(trianglesFn(), expectedTriangles, sizeof(expectedTriangles)))
		return fprintf(stderr, "FAIL box topology tables\n"), 1;
	for(NxU32 vertex = 0; vertex < 8; ++vertex)
		for(unsigned j = 0; j < 3; ++j)
			if(vertexToQuad(vertex)[j] > 5)
				return fprintf(stderr, "FAIL box vertex-to-quad table\n"), 1;
	NxVec3 worldNormal;
	worldNormalFn(&box, 0, &worldNormal);
	if(!nearVector(worldNormal, localNormalsFn()[0]) || !nearFloat(worldNormal.magnitude(), 1.0f))
		return fprintf(stderr, "FAIL box world edge normal\n"), 1;

	NxBox outer(NxVec3(0,0,0), NxVec3(5,5,5), identity);
	NxBox inner(NxVec3(1,1,1), NxVec3(2,2,2), identity);
	NxBox touching(NxVec3(3,0,0), NxVec3(2,2,2), identity);
	NxBox outside(NxVec3(3.01f,0,0), NxVec3(2,2,2), identity);
	if(!inside(&inner, &outer) || !inside(&touching, &outer) || inside(&outside, &outer) || inside(&outer, &inner))
		return fprintf(stderr, "FAIL box-box containment\n"), 1;

	printf("box exports=13 contains=interior,boundary_excluded,exterior,degenerate create=translated_aabb outputs=null_rejected,planes,points,vertex_normals tables=edges,axes,quads,triangles,vertex_to_quad world_normal=identity containment=inside,touching,outside,reverse\n");
	return 0;
	}

static int runCapsule(HMODULE module)
	{
	BoxAroundCapsuleFn boxAround = reinterpret_cast<BoxAroundCapsuleFn>(requireExport(module, "NxComputeBoxAroundCapsule"));
	CapsuleAroundBoxFn capsuleAround = reinterpret_cast<CapsuleAroundBoxFn>(requireExport(module, "NxComputeCapsuleAroundBox"));
	if(!boxAround || !capsuleAround) return 1;
	NxMat33 identity; identity.id();
	const NxVec3 directions[] = { NxVec3(4,0,0), NxVec3(0,6,0), NxVec3(0,0,8) };
	for(unsigned i = 0; i < 3; ++i)
		{
		NxCapsule capsule;
		capsule.p0 = NxVec3(1,2,3) - directions[i] * 0.5f;
		capsule.p1 = NxVec3(1,2,3) + directions[i] * 0.5f;
		capsule.radius = 1.5f;
		NxBox box;
		boxAround(&capsule, &box);
		NxVec3 axis;
		box.rot.getRow(0, axis);
		NxVec3 expectedAxis = directions[i]; expectedAxis.normalize();
		if(!nearVector(box.center, NxVec3(1,2,3)) ||
			!nearVector(box.extents, NxVec3(1.5f + directions[i].magnitude() * 0.5f, 1.5f, 1.5f)) ||
			!nearVector(axis, expectedAxis) || !nearFloat(box.rot.determinant(), 1.0f, 2.0e-4f))
			return fprintf(stderr, "FAIL capsule-to-box axis %u\n", i), 1;
		}

	const NxVec3 extents[] = { NxVec3(5,2,1), NxVec3(1,6,2), NxVec3(2,1,7), NxVec3(4,4,1), NxVec3(0,0,0) };
	for(unsigned i = 0; i < 5; ++i)
		{
		NxBox box(NxVec3(10,20,30), extents[i], identity);
		NxCapsule capsule;
		capsuleAround(&box, &capsule);
		if(!nearVector((capsule.p0 + capsule.p1) * 0.5f, box.center) || capsule.radius < 0)
			return fprintf(stderr, "FAIL box-to-capsule center %u\n", i), 1;
		if(i == 0 && (!nearFloat(capsule.radius, 1.5f) || !nearVector(capsule.p0, NxVec3(13.5f,20,30)) || !nearVector(capsule.p1, NxVec3(6.5f,20,30))))
			return fprintf(stderr, "FAIL box-to-capsule x\n"), 1;
		if(i == 1 && (!nearFloat(capsule.radius, 1.5f) || !nearVector(capsule.p0, NxVec3(10,24.5f,30)) || !nearVector(capsule.p1, NxVec3(10,15.5f,30))))
			return fprintf(stderr, "FAIL box-to-capsule y\n"), 1;
		if(i == 2 && (!nearFloat(capsule.radius, 1.5f) || !nearVector(capsule.p0, NxVec3(10,20,35.5f)) || !nearVector(capsule.p1, NxVec3(10,20,24.5f))))
			return fprintf(stderr, "FAIL box-to-capsule z\n"), 1;
		if(i == 3 && !nearFloat(capsule.radius, 2.5f))
			return fprintf(stderr, "FAIL box-to-capsule tie\n"), 1;
		if(i == 4 && (!nearFloat(capsule.radius, 0) || !nearVector(capsule.p0, box.center) || !nearVector(capsule.p1, box.center)))
			return fprintf(stderr, "FAIL box-to-capsule zero\n"), 1;
		}
	printf("capsule exports=2 capsule_to_box=axis_x,y,z_center_extents_orthonormal box_to_capsule=largest_x,y,z,tie_x,zero_center_radius\n");
	return 0;
	}

static int runSphere(HMODULE module)
	{
	ComputeSphereFn compute = reinterpret_cast<ComputeSphereFn>(requireExport(module, "NxComputeSphere"));
	FastSphereFn fast = reinterpret_cast<FastSphereFn>(requireExport(module, "NxFastComputeSphere"));
	MergeSpheresFn merge = reinterpret_cast<MergeSpheresFn>(requireExport(module, "NxMergeSpheres"));
	if(!compute || !fast || !merge) return 1;
	NxSphere sentinel(NxVec3(9,8,7), 6);
	if(compute(&sentinel, 0, 0) != NX_BS_NONE || !nearVector(sentinel.center, NxVec3(9,8,7)) || !nearFloat(sentinel.radius, 6) || fast(&sentinel, 1, 0))
		return fprintf(stderr, "FAIL sphere empty/null\n"), 1;
	NxVec3 one[] = { NxVec3(2,3,4) };
	NxSphere sphere;
	if(!fast(&sphere, 1, one) || !nearVector(sphere.center, one[0]) || !nearFloat(sphere.radius, 0))
		return fprintf(stderr, "FAIL sphere fast single\n"), 1;
	NxVec3 two[] = { NxVec3(-2,0,0), NxVec3(4,0,0) };
	if(!fast(&sphere, 2, two) || !nearVector(sphere.center, NxVec3(1,0,0)) || !nearFloat(sphere.radius, 3))
		return fprintf(stderr, "FAIL sphere fast pair\n"), 1;
	NxVec3 points[] = { NxVec3(-2,0,0), NxVec3(4,0,0), NxVec3(1,2,0), NxVec3(1,0,-2) };
	if(!fast(&sphere, 4, points))
		return fprintf(stderr, "FAIL sphere fast multi\n"), 1;
	for(unsigned i = 0; i < 4; ++i)
		if(sphere.center.distance(points[i]) > sphere.radius + 1.0e-4f)
			return fprintf(stderr, "FAIL sphere fast containment\n"), 1;
	NxSphere robust;
	NxBSphereMethod method = compute(&robust, 4, points);
	if(method != NX_BS_GEMS || !nearVector(robust.center, sphere.center) || !nearFloat(robust.radius, sphere.radius))
		return fprintf(stderr, "FAIL sphere robust selection\n"), 1;

	NxSphere a(NxVec3(0,0,0), 1), b(NxVec3(4,0,0), 2), merged;
	merge(&merged, &a, &b);
	if(!nearVector(merged.center, NxVec3(2.5f,0,0)) || !nearFloat(merged.radius, 3.5f))
		return fprintf(stderr, "FAIL sphere disjoint merge\n"), 1;
	NxSphere outer(NxVec3(1,2,3), 5), inner(NxVec3(2,2,3), 1);
	merge(&merged, &outer, &inner);
	if(!nearVector(merged.center, outer.center) || !nearFloat(merged.radius, outer.radius))
		return fprintf(stderr, "FAIL sphere containment merge\n"), 1;
	merge(&merged, &inner, &outer);
	if(!nearVector(merged.center, outer.center) || !nearFloat(merged.radius, outer.radius))
		return fprintf(stderr, "FAIL sphere reverse containment merge\n"), 1;
	NxSphere coincident0(NxVec3(3,4,5), 2), coincident1(NxVec3(3,4,5), 2);
	merge(&merged, &coincident0, &coincident1);
	if(!nearVector(merged.center, coincident1.center) || !nearFloat(merged.radius, 2))
		return fprintf(stderr, "FAIL sphere coincident merge\n"), 1;
	NxSphere touching0(NxVec3(0,0,0), 1), touching1(NxVec3(2,0,0), 1);
	merge(&merged, &touching0, &touching1);
	if(!nearVector(merged.center, NxVec3(1,0,0)) || !nearFloat(merged.radius, 2))
		return fprintf(stderr, "FAIL sphere touching merge\n"), 1;
	NxSphere alias0(NxVec3(0,0,0), 1), alias1(NxVec3(4,0,0), 2);
	merge(&alias0, &alias0, &alias1);
	if(!nearVector(alias0.center, NxVec3(2.5f,0,0)) || !nearFloat(alias0.radius, 3.5f))
		return fprintf(stderr, "FAIL sphere alias output\n"), 1;

	printf("sphere exports=3 fast=null,single,pair,multi_contains compute=empty_none,multi_gems merge=disjoint,containment,both_orders,coincident,touching,alias_first\n");
	return 0;
	}

static int runRaySegment(HMODULE module)
	{
	RayDistanceFn rayDistance = reinterpret_cast<RayDistanceFn>(requireExport(module, "NxComputeDistanceSquared"));
	SegmentDistanceFn segmentDistance = reinterpret_cast<SegmentDistanceFn>(requireExport(module, "NxComputeSquareDistance"));
	if(!rayDistance || !segmentDistance) return 1;
	NxRay ray(NxVec3(1,2,3), NxVec3(1,0,0));
	NxF32 t = -9;
	NxVec3 interior(5,5,3);
	NxF32 rayInteriorDistance = rayDistance(&ray, &interior, &t);
	if(!nearFloat(rayInteriorDistance, 9) || !nearFloat(t, 4))
		return fprintf(stderr, "FAIL ray interior projection distance=%g t=%g\n", rayInteriorDistance, t), 1;
	NxVec3 behind(-1,4,3);
	if(!nearFloat(rayDistance(&ray, &behind, &t), 8) || !nearFloat(t, 0))
		return fprintf(stderr, "FAIL ray origin clamp\n"), 1;
	NxVec3 onRay(7,2,3);
	if(!nearFloat(rayDistance(&ray, &onRay, 0), 0))
		return fprintf(stderr, "FAIL ray null t\n"), 1;
	NxRay zeroRay(NxVec3(1,1,1), NxVec3(0,0,0));
	NxVec3 zeroPoint(2,3,1);
	if(!nearFloat(rayDistance(&zeroRay, &zeroPoint, &t), 5) || !nearFloat(t, 0))
		return fprintf(stderr, "FAIL ray zero direction\n"), 1;
	NxVec3 aliasPoint(5,5,3);
	if(!nearFloat(rayDistance(&ray, &aliasPoint, &aliasPoint.x), 9) || !nearFloat(aliasPoint.x, 4))
		return fprintf(stderr, "FAIL ray output alias\n"), 1;

	NxSegment segment(NxVec3(1,2,3), NxVec3(5,2,3));
	NxVec3 middle(3,5,3), before(-1,4,3), after(7,4,3);
	if(!nearFloat(segmentDistance(&segment, &middle, &t), 9) || !nearFloat(t, 0.5f))
		return fprintf(stderr, "FAIL segment interior\n"), 1;
	if(!nearFloat(segmentDistance(&segment, &before, &t), 8) || !nearFloat(t, 0))
		return fprintf(stderr, "FAIL segment start clamp\n"), 1;
	if(!nearFloat(segmentDistance(&segment, &after, &t), 8) || !nearFloat(t, 1))
		return fprintf(stderr, "FAIL segment end clamp\n"), 1;
	NxSegment zeroSegment(NxVec3(1,1,1), NxVec3(1,1,1));
	if(!nearFloat(segmentDistance(&zeroSegment, &zeroPoint, &t), 5) || !nearFloat(t, 0))
		return fprintf(stderr, "FAIL segment zero length\n"), 1;
	NxVec3 aliasSegmentPoint(3,5,3);
	if(!nearFloat(segmentDistance(&segment, &aliasSegmentPoint, &aliasSegmentPoint.y), 9) || !nearFloat(aliasSegmentPoint.y, 0.5f))
		return fprintf(stderr, "FAIL segment output alias\n"), 1;
	printf("ray_seg exports=2 ray=unit_interior,behind,on,null_t,zero_dir,output_alias segment=interior,start,end,zero_length,output_alias\n");
	return 0;
	}

static bool nearDouble(double a, double b, double tolerance = 1e-9)
	{
	return fabs(a - b) <= tolerance;
	}

static bool checkTetraIntegrals(const NxIntegrals& result, double mass, const NxVec3& center)
	{
	if(!nearDouble(result.mass, mass) || !nearFloat(result.COM.x, center.x) ||
		!nearFloat(result.COM.y, center.y) || !nearFloat(result.COM.z, center.z)) return false;
	const double originDiag[3] = {
		mass * (0.075 + center.y*center.y + center.z*center.z),
		mass * (0.075 + center.z*center.z + center.x*center.x),
		mass * (0.075 + center.x*center.x + center.y*center.y)
	};
	for(unsigned i = 0; i < 3; ++i)
		for(unsigned j = 0; j < 3; ++j)
		{
			double comExpected = mass * (i == j ? 0.075 : 0.0125);
			double originExpected = i == j ? originDiag[i] :
				mass * (0.0125 - center[i] * center[j]);
			if(!nearDouble(result.COMInertiaTensor[i][j], comExpected, 1e-4) ||
				!nearDouble(result.inertiaTensor[i][j], originExpected, 1e-4)) return false;
		}
	return true;
	}

static int runVolume(HMODULE module)
	{
	VolumeIntegralsFn compute = reinterpret_cast<VolumeIntegralsFn>(requireExport(module, "NxComputeVolumeIntegrals"));
	CreateFoundationSDKFn createSDK = reinterpret_cast<CreateFoundationSDKFn>(requireExport(module, "NxCreateFoundationSDK"));
	if(!compute || !createSDK) return 1;
	NxVec3 points[] = { NxVec3(0,0,0), NxVec3(1,0,0), NxVec3(0,1,0), NxVec3(0,0,1) };
	NxU32 indices[] = { 0,2,1, 0,1,3, 0,3,2, 1,2,3 };
	NxSimpleTriangleMesh mesh;
	mesh.numVertices = 4; mesh.numTriangles = 4;
	mesh.pointStrideBytes = sizeof(NxVec3); mesh.triangleStrideBytes = 3 * sizeof(NxU32);
	mesh.points = points; mesh.triangles = indices;
	NxIntegrals untouched;
	memset(&untouched, 0x5a, sizeof(untouched));
	NxIntegrals sentinel;
	memcpy(&sentinel, &untouched, sizeof(sentinel));
	if(compute(&mesh, 6.0f, &untouched) || memcmp(&untouched, &sentinel, sizeof(untouched)))
		return fprintf(stderr, "FAIL volume allocator guard\n"), 1;
	NxFoundationSDK* sdk = createSDK(NX_FOUNDATION_SDK_VERSION, 0, 0);
	if(!sdk) return fprintf(stderr, "FAIL volume SDK create\n"), 1;
	NxIntegrals result;
	if(!compute(&mesh, 6.0f, &result) || !checkTetraIntegrals(result, 1.0, NxVec3(0.25f,0.25f,0.25f)))
		return fprintf(stderr, "FAIL volume tetra32\n"), 1;

	NxVec3 translated[] = { NxVec3(2,3,4), NxVec3(3,3,4), NxVec3(2,4,4), NxVec3(2,3,5) };
	mesh.points = translated;
	if(!compute(&mesh, 12.0f, &result) || !checkTetraIntegrals(result, 2.0, NxVec3(2.25f,3.25f,4.25f)))
		{
		fprintf(stderr, "FAIL volume translated density mass=%.17g com=%.9g,%.9g,%.9g\n", result.mass, result.COM.x, result.COM.y, result.COM.z);
		for(unsigned i=0;i<3;++i) fprintf(stderr, " origin[%u]=%.17g,%.17g,%.17g comI[%u]=%.17g,%.17g,%.17g\n", i,
			result.inertiaTensor[i][0],result.inertiaTensor[i][1],result.inertiaTensor[i][2],i,
			result.COMInertiaTensor[i][0],result.COMInertiaTensor[i][1],result.COMInertiaTensor[i][2]);
		return 1;
		}

	NxU16 reversed16[] = { 0,1,2, 0,3,1, 0,2,3, 1,3,2 };
	mesh.points = points; mesh.triangles = reversed16;
	mesh.triangleStrideBytes = 3 * sizeof(NxU16);
	mesh.flags = NX_MF_16_BIT_INDICES | NX_MF_FLIPNORMALS;
	if(!compute(&mesh, 6.0f, &result) || !checkTetraIntegrals(result, 1.0, NxVec3(0.25f,0.25f,0.25f)))
		return fprintf(stderr, "FAIL volume tetra16 flip\n"), 1;
	sdk->release();
	printf("volume exports=1 allocator_guard=unchanged tetra32=analytic translated=density12 tetra16_flip=analytic\n");
	return 0;
	}

static bool executableAddress(const void* address)
	{
	MEMORY_BASIC_INFORMATION info;
	if(!VirtualQuery(address, &info, sizeof(info))) return false;
	DWORD protection = info.Protect & 0xff;
	return protection == PAGE_EXECUTE || protection == PAGE_EXECUTE_READ ||
		protection == PAGE_EXECUTE_READWRITE || protection == PAGE_EXECUTE_WRITECOPY;
	}

static int runDebug(HMODULE module)
	{
	CreateFoundationSDKFn createSDK = reinterpret_cast<CreateFoundationSDKFn>(requireExport(module, "NxCreateFoundationSDK"));
	CreateDebugRenderableFn createDebug = reinterpret_cast<CreateDebugRenderableFn>(requireExport(module, "?createDebugRenderable@FoundationSDK@NxFoundation@@UAEPAVNxDebugRenderable@@XZ"));
	ReleaseDebugRenderableFn releaseDebug = reinterpret_cast<ReleaseDebugRenderableFn>(requireExport(module, "?releaseDebugRenderable@FoundationSDK@NxFoundation@@UAEXAAPAVNxDebugRenderable@@@Z"));
	RenderDebugDataFn renderDebug = reinterpret_cast<RenderDebugDataFn>(requireExport(module, "?renderDebugData@FoundationSDK@NxFoundation@@UBEXABVNxUserDebugRenderer@@@Z"));
	if(!createSDK || !createDebug || !releaseDebug || !renderDebug) return 1;
	ClusterRecordingAllocator allocator;
	NxFoundationSDK* sdk = createSDK(NX_FOUNDATION_SDK_VERSION, 0, &allocator);
	if(!sdk || allocator.calls != 1 || allocator.sizes[0] != 56)
		return fprintf(stderr, "FAIL debug SDK allocation\n"), 1;
	unsigned beforeCreate = allocator.calls;
	NxDebugRenderable* first = createDebug(sdk);
	if(!first || allocator.calls < beforeCreate + 2 || allocator.sizes[beforeCreate] != 52)
		return fprintf(stderr, "FAIL debug object size calls=%u size=%u\n", allocator.calls-beforeCreate, unsigned(allocator.sizes[beforeCreate])), 1;
	void** vftable = *reinterpret_cast<void***>(first);
	for(unsigned i=0; i<15; ++i)
		if(!executableAddress(vftable[i])) return fprintf(stderr, "FAIL debug vftable slot %u\n", i), 1;
	if(first->getNbPoints() || first->getNbLines() || first->getNbTriangles() ||
		first->getPoints() || first->getLines() || first->getTriangles())
		return fprintf(stderr, "FAIL debug initial buffers\n"), 1;
	first->addPoint(NxVec3(1,2,3), 0x11223344);
	first->addLine(NxVec3(0,0,0), NxVec3(1,2,3), 0x55667788);
	first->addTriangle(NxVec3(0,0,0), NxVec3(1,0,0), NxVec3(0,1,0), 0x99aabbcc);
	if(first->getNbPoints()!=1 || first->getNbLines()!=1 || first->getNbTriangles()!=1 ||
		first->getPoints()[0].color != 0x11223344 || first->getPoints()[0].p != NxVec3(1,2,3) ||
		first->getLines()[0].color != 0x55667788 || first->getTriangles()[0].color != 0x99aabbcc)
		return fprintf(stderr, "FAIL debug primitive buffers\n"), 1;
	const NxDebugPoint* pointBuffer = first->getPoints();
	first->clear();
	if(first->getNbPoints() || first->getNbLines() || first->getNbTriangles() || first->getPoints()!=pointBuffer)
		return fprintf(stderr, "FAIL debug clear retention\n"), 1;
	first->addPoint(NxVec3(4,5,6), 7);
	if(first->getPoints()!=pointBuffer) return fprintf(stderr, "FAIL debug buffer reuse\n"), 1;

	NxBounds3 bounds; bounds.set(NxVec3(-1,-2,-3), NxVec3(1,2,3));
	first->addAABB(bounds, 0x01020304, true);
	if(first->getNbLines()!=15) return fprintf(stderr, "FAIL debug AABB/frame count=%u\n", first->getNbLines()), 1;
	for(unsigned i=0;i<12;++i) if(first->getLines()[i].color!=0x01020304) return fprintf(stderr, "FAIL debug AABB color\n"), 1;
	if(first->getLines()[12].color!=0x00ff0000 || first->getLines()[13].color!=0x0000ff00 || first->getLines()[14].color!=0x000000ff)
		return fprintf(stderr, "FAIL debug frame colors\n"), 1;
	NxMat33 identity; identity.id();
	NxBox box(NxVec3(0,0,0), NxVec3(1,1,1), identity);
	first->addOBB(box, 9, false);
	first->addArrow(NxVec3(0,0,0), NxVec3(1,0,0), 2, 1, 10);
	first->addArrow(NxVec3(0,0,0), NxVec3(1,0,0), 0, 1, 10);
	NxU32 colors[3] = { 11, 12, 13 };
	first->addBasis(NxVec3(0,0,0), identity, NxVec3(1,1,1), 1, colors);
	NxMat34 transform; transform.id();
	first->addCircle(8, transform, 14, 2, false);
	first->addCircle(8, transform, 15, 2, true);
	if(first->getNbLines()!=59) return fprintf(stderr, "FAIL debug high-level count=%u\n", first->getNbLines()), 1;

	NxDebugRenderable* second = createDebug(sdk);
	if(!second) return fprintf(stderr, "FAIL debug second create\n"), 1;
	second->addPoint(NxVec3(9,8,7), 6);
	ClusterDebugRenderer renderer;
	renderDebug(sdk, &renderer);
	if(renderer.calls!=2 || renderer.seen[0]!=first || renderer.seen[1]!=second)
		return fprintf(stderr, "FAIL debug render append order\n"), 1;
	releaseDebug(sdk, &first);
	if(first) return fprintf(stderr, "FAIL debug release nulling\n"), 1;
	renderer.calls=0; memset(renderer.seen,0,sizeof(renderer.seen));
	renderDebug(sdk, &renderer);
	if(renderer.calls!=1 || renderer.seen[0]!=second) return fprintf(stderr, "FAIL debug release ownership\n"), 1;
	NxDebugRenderable* nullRenderable = 0;
	releaseDebug(sdk, &nullRenderable);
	releaseDebug(sdk, &second);
	if(second) return fprintf(stderr, "FAIL debug second release\n"), 1;
	sdk->release();
	printf("debug exports=3 object_size=52 vftable=15 primitives=point,line,triangle clear=retains_buffer aabb=12+frame3 high_level=59 append=2,1 release=nulls\n");
	return 0;
	}

int main(int argc, char** argv)
	{
	if(argc != 3 || (strcmp(argv[1], "exception") && strcmp(argv[1], "observable") && strcmp(argv[1], "profiler") && strcmp(argv[1], "time") && strcmp(argv[1], "fpu") && strcmp(argv[1], "util") && strcmp(argv[1], "box") && strcmp(argv[1], "capsule") && strcmp(argv[1], "sphere") && strcmp(argv[1], "ray_seg") && strcmp(argv[1], "volume") && strcmp(argv[1], "debug")))
		{
		fprintf(stderr, "usage: NxFoundationClusterTests <exception|observable|profiler|time|fpu|util|box|capsule|sphere|ray_seg|volume|debug> <NxFoundation.dll>\n");
		return 2;
		}
	HMODULE module = LoadLibraryA(argv[2]);
	if(!module)
		return fprintf(stderr, "FAIL LoadLibrary %lu\n", GetLastError()), 2;
	int result = !strcmp(argv[1], "exception") ? runException(module) :
		(!strcmp(argv[1], "observable") ? runObservable(module) :
		(!strcmp(argv[1], "profiler") ? runProfiler(module) :
		(!strcmp(argv[1], "time") ? runTime(module) :
		(!strcmp(argv[1], "fpu") ? runFpu(module) :
		(!strcmp(argv[1], "util") ? runUtil(module) :
		(!strcmp(argv[1], "box") ? runBox(module) :
		(!strcmp(argv[1], "capsule") ? runCapsule(module) :
		(!strcmp(argv[1], "sphere") ? runSphere(module) :
		(!strcmp(argv[1], "ray_seg") ? runRaySegment(module) :
		(!strcmp(argv[1], "volume") ? runVolume(module) : runDebug(module)))))))))));
	FreeLibrary(module);
	return result;
	}
