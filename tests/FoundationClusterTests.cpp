#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "NxException.h"
#include "NxFoundationSDK.h"
#include "NxProfiler.h"

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

int main(int argc, char** argv)
	{
	if(argc != 3 || (strcmp(argv[1], "exception") && strcmp(argv[1], "observable") && strcmp(argv[1], "profiler") && strcmp(argv[1], "time")))
		{
		fprintf(stderr, "usage: NxFoundationClusterTests <exception|observable|profiler|time> <NxFoundation.dll>\n");
		return 2;
		}
	HMODULE module = LoadLibraryA(argv[2]);
	if(!module)
		return fprintf(stderr, "FAIL LoadLibrary %lu\n", GetLastError()), 2;
	int result = !strcmp(argv[1], "exception") ? runException(module) :
		(!strcmp(argv[1], "observable") ? runObservable(module) :
		(!strcmp(argv[1], "profiler") ? runProfiler(module) : runTime(module)));
	FreeLibrary(module);
	return result;
	}
