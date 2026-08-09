#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "NxException.h"

typedef void* (__thiscall *ExceptionValueCtorFn)(void*, NxErrorCode, const char*, int);
typedef void* (__thiscall *ExceptionCopyCtorFn)(void*, const void*);
typedef void* (__thiscall *ExceptionAssignFn)(void*, const void*);
typedef NxErrorCode (__thiscall *ExceptionGetErrorCodeFn)(void*);
typedef const char* (__thiscall *ExceptionGetFileFn)(void*);
typedef int (__thiscall *ExceptionGetLineFn)(void*);

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

int main(int argc, char** argv)
	{
	if(argc != 3 || strcmp(argv[1], "exception"))
		{
		fprintf(stderr, "usage: NxFoundationClusterTests exception <NxFoundation.dll>\n");
		return 2;
		}
	HMODULE module = LoadLibraryA(argv[2]);
	if(!module)
		return fprintf(stderr, "FAIL LoadLibrary %lu\n", GetLastError()), 2;
	int result = runException(module);
	FreeLibrary(module);
	return result;
	}
