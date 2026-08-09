#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "NxException.h"
#include "NxFoundationSDK.h"

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

int main(int argc, char** argv)
	{
	if(argc != 3 || (strcmp(argv[1], "exception") && strcmp(argv[1], "observable")))
		{
		fprintf(stderr, "usage: NxFoundationClusterTests <exception|observable> <NxFoundation.dll>\n");
		return 2;
		}
	HMODULE module = LoadLibraryA(argv[2]);
	if(!module)
		return fprintf(stderr, "FAIL LoadLibrary %lu\n", GetLastError()), 2;
	int result = !strcmp(argv[1], "exception") ? runException(module) : runObservable(module);
	FreeLibrary(module);
	return result;
	}
