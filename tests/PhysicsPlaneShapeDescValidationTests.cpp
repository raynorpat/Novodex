#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

static bool sha256File(const wchar_t* path, char result[65])
	{
	HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, 0,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if(file == INVALID_HANDLE_VALUE) return false;
	LARGE_INTEGER size;
	if(!GetFileSizeEx(file, &size) || size.QuadPart < 0 ||
		static_cast<unsigned long long>(size.QuadPart) > 0xffffffffull)
		{ CloseHandle(file); return false; }
	std::vector<unsigned char> bytes(static_cast<size_t>(size.QuadPart));
	DWORD read = 0;
	const bool readOk = bytes.empty() ||
		(ReadFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &read, 0) && read == bytes.size());
	CloseHandle(file);
	if(!readOk) return false;
	BCRYPT_ALG_HANDLE algorithm = 0;
	BCRYPT_HASH_HANDLE hash = 0;
	unsigned char digest[32];
	DWORD objectBytes = 0, hashBytes = 0, received = 0;
	if(BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, 0, 0) < 0) return false;
	const bool ok = BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH,
		reinterpret_cast<PUCHAR>(&objectBytes), sizeof(objectBytes), &received, 0) >= 0 &&
		BCryptGetProperty(algorithm, BCRYPT_HASH_LENGTH,
		reinterpret_cast<PUCHAR>(&hashBytes), sizeof(hashBytes), &received, 0) >= 0 && hashBytes == sizeof(digest) &&
		objectBytes != 0;
	std::vector<unsigned char> object(objectBytes);
	const bool hashed = ok &&
		BCryptCreateHash(algorithm, &hash, object.data(), objectBytes, 0, 0, 0) >= 0 &&
		BCryptHashData(hash, bytes.empty() ? 0 : bytes.data(), static_cast<ULONG>(bytes.size()), 0) >= 0 &&
		BCryptFinishHash(hash, digest, sizeof(digest), 0) >= 0;
	if(hash) BCryptDestroyHash(hash);
	BCryptCloseAlgorithmProvider(algorithm, 0);
	if(!hashed) return false;
	static const char hex[] = "0123456789abcdef";
	for(unsigned i = 0; i < sizeof(digest); ++i)
		{ result[2*i] = hex[digest[i] >> 4]; result[2*i+1] = hex[digest[i] & 15]; }
	result[64] = 0;
	return true;
	}

static bool resolveCandidateMethod(const wchar_t* executableDirectory,
	unsigned& rva)
	{
	wchar_t candidatePath[MAX_PATH], aliasPath[MAX_PATH], mapPath[MAX_PATH];
	if(swprintf_s(candidatePath, L"%s\\NxPhysics.dll", executableDirectory) < 0 ||
		swprintf_s(aliasPath, L"%s\\NxPhysicsPlaneShapeDescCandidate.dll", executableDirectory) < 0 ||
		swprintf_s(mapPath, L"%s\\NxPhysics.map", executableDirectory) < 0)
		return false;
	if(!CopyFileW(candidatePath, aliasPath, FALSE)) return false;
	std::ifstream map(mapPath);
	if(!map) return false;
	const char* symbol = "?isValid@NxPlaneShapeDesc@@UBE_NXZ";
	unsigned mapVa = 0;
	std::string line;
	while(std::getline(map, line))
		{
		if(line.find(symbol) == std::string::npos) continue;
		unsigned segment = 0, offset = 0, address = 0;
		char name[256] = {};
		if(sscanf_s(line.c_str(), "%x:%x %255s %x", &segment, &offset,
			name, (unsigned)_countof(name), &address) == 4 && strcmp(name, symbol) == 0)
			{
			mapVa = address;
			break;
			}
		}
	if(!mapVa) return false;
	HANDLE file = CreateFileW(candidatePath, GENERIC_READ, FILE_SHARE_READ, 0,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if(file == INVALID_HANDLE_VALUE) return false;
	unsigned char headers[4096] = {};
	DWORD bytes = 0;
	const bool read = ReadFile(file, headers, sizeof(headers), &bytes, 0) != 0;
	CloseHandle(file);
	if(!read || bytes < sizeof(IMAGE_DOS_HEADER)) return false;
	const PIMAGE_DOS_HEADER dos = reinterpret_cast<PIMAGE_DOS_HEADER>(headers);
	if(dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 0 ||
		dos->e_lfanew + sizeof(IMAGE_NT_HEADERS32) > bytes) return false;
	const PIMAGE_NT_HEADERS32 nt = reinterpret_cast<PIMAGE_NT_HEADERS32>(headers + dos->e_lfanew);
	if(nt->Signature != IMAGE_NT_SIGNATURE || mapVa < nt->OptionalHeader.ImageBase) return false;
	rva = mapVa - nt->OptionalHeader.ImageBase;
	return rva < nt->OptionalHeader.SizeOfImage;
	}

static void makeDefaultPlane(unsigned char* desc)
	{
	memset(desc, 0, 0x5c);
	*reinterpret_cast<unsigned*>(desc + 4) = 0; // NX_SHAPE_PLANE
	*reinterpret_cast<unsigned*>(desc + 8) = 0x3f800000u;
	*reinterpret_cast<unsigned*>(desc + 0x18) = 0x3f800000u;
	*reinterpret_cast<unsigned*>(desc + 0x28) = 0x3f800000u;
	*reinterpret_cast<unsigned*>(desc + 0x38) = 8; // NX_SF_VISUALIZATION
	*reinterpret_cast<unsigned short*>(desc + 0x3c) = 0;
	*reinterpret_cast<unsigned short*>(desc + 0x3e) = 0;
	// NX_SHAPE_DESC_LIST adds its `next` pointer at +0x48. The plane normal
	// and distance therefore begin at +0x4c, +0x50, +0x54, and +0x58.
	*reinterpret_cast<unsigned*>(desc + 0x4c) = 0;
	*reinterpret_cast<unsigned*>(desc + 0x50) = 0x3f800000u;
	*reinterpret_cast<unsigned*>(desc + 0x54) = 0;
	*reinterpret_cast<unsigned*>(desc + 0x58) = 0;
	}

int wmain(int argc, wchar_t** argv)
	{
	if(argc != 2 && argc != 3)
		{
		fprintf(stderr, "usage: NxPhysicsPlaneShapeDescValidationTests <oracle directory> [oracle sha256]\n");
		return 2;
		}
	wchar_t oraclePath[MAX_PATH], exePath[MAX_PATH];
	if(swprintf_s(oraclePath, L"%s\\NxPhysics.dll", argv[1]) < 0 ||
		!GetModuleFileNameW(0, exePath, MAX_PATH))
		{ fprintf(stderr, "plane validator path setup failed\n"); return 2; }
	wchar_t oracleFoundationPath[MAX_PATH];
	char oracleHash[65], oracleFoundationHash[65], oraclePathUtf8[MAX_PATH], oracleFoundationPathUtf8[MAX_PATH];
	size_t converted = 0;
	if(!sha256File(oraclePath, oracleHash) ||
		swprintf_s(oracleFoundationPath, L"%s\\NxFoundation.dll", argv[1]) < 0 ||
		!sha256File(oracleFoundationPath, oracleFoundationHash) ||
		wcstombs_s(&converted, oraclePathUtf8, sizeof(oraclePathUtf8), oraclePath, _TRUNCATE) ||
		wcstombs_s(&converted, oracleFoundationPathUtf8, sizeof(oracleFoundationPathUtf8), oracleFoundationPath, _TRUNCATE))
		{ fprintf(stderr, "plane validator hash/path conversion failed\n"); return 2; }
	if(argc == 3)
		{
		char expectedHash[65];
		if(wcstombs_s(&converted, expectedHash, sizeof(expectedHash), argv[2], _TRUNCATE) ||
			strcmp(expectedHash, oracleHash) != 0)
			{ fprintf(stderr, "plane validator oracle SHA-256 mismatch\n"); return 2; }
		}
	wchar_t* slash = wcsrchr(exePath, L'\\');
	if(!slash) return 2;
	*slash = 0;
	unsigned candidateRva = 0;
	if(!resolveCandidateMethod(exePath, candidateRva) ||
		!SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32) ||
		!AddDllDirectory(argv[1]) || !AddDllDirectory(exePath))
		{
		fprintf(stderr, "plane validator candidate setup failed rva=%x\n", candidateRva);
		return 2;
		}
	HMODULE oracle = LoadLibraryExW(oraclePath, 0,
		LOAD_LIBRARY_SEARCH_USER_DIRS | LOAD_LIBRARY_SEARCH_SYSTEM32);
	if(!oracle)
		{
		fprintf(stderr, "cannot load oracle: %lu\n", GetLastError());
		return 2;
		}
	printf("loaded module=NxPhysics.dll path=%s sha256=%s\n", oraclePathUtf8, oracleHash);
	printf("loaded module=NxFoundation.dll path=%s sha256=%s\n",
		oracleFoundationPathUtf8, oracleFoundationHash);
	wchar_t aliasPath[MAX_PATH];
	if(swprintf_s(aliasPath, L"%s\\NxPhysicsPlaneShapeDescCandidate.dll", exePath) < 0)
		return 2;
	HMODULE candidate = LoadLibraryExW(aliasPath, 0,
		LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_USER_DIRS | LOAD_LIBRARY_SEARCH_SYSTEM32);
	if(!candidate)
		{
		fprintf(stderr, "cannot load candidate: %lu\n", GetLastError());
		FreeLibrary(oracle);
		return 2;
		}
	typedef bool (__thiscall* IsValidFn)(const void*);
	IsValidFn oracleFn = reinterpret_cast<IsValidFn>(
		reinterpret_cast<unsigned char*>(oracle) + 0x10ab0);
	IsValidFn candidateFn = reinterpret_cast<IsValidFn>(
		reinterpret_cast<unsigned char*>(candidate) + candidateRva);
	struct Case { const char* name; unsigned offset; unsigned value; };
	static const Case cases[] = {
		{"valid", 0, 0}, {"normal_nan", 0x50, 0x7fc00000u},
		{"normal_infinite", 0x54, 0x7f800000u}, {"distance_nan", 0x58, 0x7fc00000u},
		{"distance_infinite", 0x58, 0x7f800000u}, {"pose_nan", 8, 0x7fc00000u},
		{"group_limit", 0x3c, 32u}, {"flags_high", 0x38, 0x00010000u},
		{"type_invalid", 4, 6u}, {"material_reserved", 0x3e, 0xffffu}
		};
	unsigned mismatches = 0;
	for(unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
		{
		unsigned char original[0x5c], candidateInput[0x5c];
		makeDefaultPlane(original);
		if(cases[i].offset)
			memcpy(original + cases[i].offset, &cases[i].value, sizeof(cases[i].value));
		memcpy(candidateInput, original, sizeof(original));
		const bool expected = oracleFn(original);
		const bool actual = candidateFn(candidateInput);
		if(expected != actual || memcmp(original, candidateInput, sizeof(original)) != 0)
			{
			++mismatches;
			fprintf(stderr, "planevalid case=%s oracle=%u candidate=%u\n",
				cases[i].name, expected ? 1u : 0u, actual ? 1u : 0u);
			}
		}
	printf("planevalid row=phys_fn_000585 candidate_dll_vs_oracle cases=%u mismatches=%u\n",
		(unsigned)(sizeof(cases) / sizeof(cases[0])), mismatches);
	if(mismatches)
		{
		FreeLibrary(candidate);
		FreeLibrary(oracle);
		return 1;
		}
	// A row-local false-return mutant must disagree on the valid plane. This
	// negative control confirms the candidate call really enters the mapped DLL row.
	DWORD oldProtect = 0;
	unsigned char* candidateBase = reinterpret_cast<unsigned char*>(candidate);
	unsigned char* method = candidateBase + candidateRva;
	if(!VirtualProtect(method, 3, PAGE_EXECUTE_READWRITE, &oldProtect)) return 2;
	const unsigned char mutant[] = {0x33, 0xc0, 0xc3};
	unsigned char saved[sizeof(mutant)];
	memcpy(saved, method, sizeof(saved));
	memcpy(method, mutant, sizeof(mutant));
	FlushInstructionCache(GetCurrentProcess(), method, sizeof(mutant));
	unsigned char valid[0x5c];
	makeDefaultPlane(valid);
	const bool oracleValid = oracleFn(valid);
	const bool mutantValid = candidateFn(valid);
	memcpy(method, saved, sizeof(saved));
	FlushInstructionCache(GetCurrentProcess(), method, sizeof(saved));
	DWORD ignored = 0;
	VirtualProtect(method, sizeof(saved), oldProtect, &ignored);
	const bool mutantCaught = oracleValid && !mutantValid;
	printf("planevalid mutant row=phys_fn_000585 caught=%u mismatches=%u\n",
		mutantCaught ? 1u : 0u, mutantCaught ? 1u : 0u);
	FreeLibrary(candidate);
	FreeLibrary(oracle);
	return mutantCaught ? 0 : 1;
	}
