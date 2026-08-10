#ifndef NX_PHYSICS_PAIR_LOADER_H
#define NX_PHYSICS_PAIR_LOADER_H

// Loads the shipped Physics/Foundation DLL pair out of one isolated directory.
// Every dependency must come from that directory or from the trusted Windows
// system set below, so a stray Novodex build elsewhere on the machine can never
// satisfy the load.

#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <bcrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

// Measured by loading the oracle pair on Windows 11 26100 in a Win32 process.
// The first seven are the shipped pair's own Windows dependency closure, the
// last four are the loader shim and this harness's C runtime and SHA-256
// provider. Any other module, or any of these from any other directory, fails
// the audit.
static const wchar_t* const nxTrustedSystemModules[] =
	{
	L"ntdll.dll",
	L"KERNEL32.DLL",
	L"KERNELBASE.dll",
	L"ADVAPI32.dll",
	L"msvcrt.dll",
	L"sechost.dll",
	L"RPCRT4.dll",
	L"apphelp.dll",
	L"ucrtbase.dll",
	L"VCRUNTIME140.dll",
	L"bcrypt.dll"
	};

static int nxFail(const char* message)
	{
	fprintf(stderr, "FAIL %s\n", message);
	return 1;
	}

static bool nxSha256(const wchar_t* path, char* text)
	{
	HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if(file == INVALID_HANDLE_VALUE)
		return false;

	LARGE_INTEGER size;
	BYTE digest[32];
	bool ok = GetFileSizeEx(file, &size) != 0 && size.QuadPart > 0 && size.QuadPart < 0x08000000;
	BYTE* bytes = ok ? static_cast<BYTE*>(malloc(static_cast<size_t>(size.QuadPart))) : 0;
	DWORD read = 0;
	ok = bytes != 0
		&& ReadFile(file, bytes, static_cast<DWORD>(size.QuadPart), &read, 0) != 0
		&& read == size.QuadPart
		&& BCryptHash(BCRYPT_SHA256_ALG_HANDLE, 0, 0, bytes, read, digest, sizeof(digest)) == 0;
	free(bytes);
	CloseHandle(file);
	if(!ok)
		return false;

	for(int i = 0; i < 32; ++i)
		sprintf_s(text + i * 2, 3, "%02x", digest[i]);
	return true;
	}

static bool nxInDirectory(const wchar_t* path, const wchar_t* directory)
	{
	const wchar_t* slash = wcsrchr(path, L'\\');
	if(!slash)
		return false;
	size_t length = static_cast<size_t>(slash - path);
	return wcslen(directory) == length && _wcsnicmp(path, directory, length) == 0;
	}

static bool nxTrustedSystemModule(const wchar_t* name)
	{
	for(size_t i = 0; i < sizeof(nxTrustedSystemModules) / sizeof(nxTrustedSystemModules[0]); ++i)
		if(_wcsicmp(name, nxTrustedSystemModules[i]) == 0)
			return true;
	return false;
	}

// Rejects any loaded module that is neither the test executable, nor a pair DLL
// inside the pair directory, nor a trusted module inside the system directory.
static int nxAuditModules(const wchar_t* pairDirectory)
	{
	wchar_t systemDirectory[MAX_PATH];
	wchar_t self[MAX_PATH];
	if(!GetSystemDirectoryW(systemDirectory, MAX_PATH) || !GetModuleFileNameW(0, self, MAX_PATH))
		return nxFail("cannot resolve system directory or test executable");

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
	if(snapshot == INVALID_HANDLE_VALUE)
		return nxFail("module snapshot unavailable");

	MODULEENTRY32W entry;
	entry.dwSize = sizeof(entry);
	unsigned pair = 0;
	unsigned system = 0;
	unsigned rejected = 0;
	for(BOOL more = Module32FirstW(snapshot, &entry); more; more = Module32NextW(snapshot, &entry))
		{
		if(_wcsicmp(entry.szExePath, self) == 0)
			continue;
		if(nxInDirectory(entry.szExePath, pairDirectory))
			{
			if(_wcsicmp(entry.szModule, L"NxPhysics.dll") == 0 || _wcsicmp(entry.szModule, L"NxFoundation.dll") == 0)
				++pair;
			else
				{
				printf("module_rejected path=%S reason=unexpected_module_in_pair_directory\n", entry.szExePath);
				++rejected;
				}
			continue;
			}
		if(nxInDirectory(entry.szExePath, systemDirectory) && nxTrustedSystemModule(entry.szModule))
			{
			++system;
			continue;
			}
		printf("module_rejected path=%S reason=outside_pair_and_trusted_system\n", entry.szExePath);
		++rejected;
		}
	CloseHandle(snapshot);

	printf("modules pair=%u trusted_system=%u rejected=%u\n", pair, system, rejected);
	if(rejected)
		return nxFail("module loaded outside the isolated pair and the trusted system set");
	return 0;
	}

// argv[1] must already be an absolute canonical directory; anything else is a
// caller error we refuse rather than silently resolve.
static bool nxPairDirectory(const char* argument, wchar_t* directory)
	{
	wchar_t given[MAX_PATH];
	wchar_t resolved[MAX_PATH];
	if(!MultiByteToWideChar(CP_ACP, 0, argument, -1, given, MAX_PATH))
		return false;
	if(!GetFullPathNameW(given, MAX_PATH, resolved, 0) || _wcsicmp(given, resolved) != 0)
		return false;
	if(GetFileAttributesW(resolved) == INVALID_FILE_ATTRIBUTES)
		return false;
	wcscpy_s(directory, MAX_PATH, resolved);
	return true;
	}

static HMODULE nxLoadPhysics(const wchar_t* pairDirectory)
	{
	wchar_t physics[MAX_PATH];
	if(swprintf_s(physics, L"%s\\NxPhysics.dll", pairDirectory) < 0)
		return 0;
	// Drop the application directory, the current directory and PATH from the
	// search order, then add the pair directory back as the only non-system source.
	if(!SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32) || !AddDllDirectory(pairDirectory))
		return 0;
	return LoadLibraryExW(physics, 0, LOAD_LIBRARY_SEARCH_USER_DIRS | LOAD_LIBRARY_SEARCH_SYSTEM32);
	}

// Loads the pair, audits the module set, and reports the absolute path and
// SHA-256 the differential runner matches against the directory it staged.
static int nxOpenPair(int argc, char** argv, const char* usage, wchar_t* pairDirectory, HMODULE* physics)
	{
	if(argc != 2)
		{
		fprintf(stderr, "usage: %s <absolute pair directory>\n", usage);
		return 2;
		}
	if(!nxPairDirectory(argv[1], pairDirectory))
		{
		fprintf(stderr, "FAIL pair directory is not an existing absolute canonical path\n");
		return 2;
		}

	printf("pair_directory=%S\n", pairDirectory);
	*physics = nxLoadPhysics(pairDirectory);
	if(!*physics)
		{
		fprintf(stderr, "FAIL isolated LoadLibraryEx failed: %lu\n", GetLastError());
		return 1;
		}
	return nxAuditModules(pairDirectory);
	}

static int nxReportPairIdentity(const wchar_t* pairDirectory)
	{
	static const wchar_t* const names[] = { L"NxPhysics.dll", L"NxFoundation.dll" };
	for(int i = 0; i < 2; ++i)
		{
		HMODULE module = GetModuleHandleW(names[i]);
		if(!module)
			return nxFail("pair module is not loaded");

		wchar_t path[MAX_PATH];
		char hash[65];
		if(!GetModuleFileNameW(module, path, MAX_PATH))
			return nxFail("pair module path unavailable");
		if(!nxInDirectory(path, pairDirectory))
			return nxFail("pair module loaded from outside the pair directory");
		if(!nxSha256(path, hash))
			return nxFail("pair module hash unavailable");
		printf("loaded module=%S path=%S sha256=%s\n", names[i], path, hash);
		}
	return 0;
	}

#endif
