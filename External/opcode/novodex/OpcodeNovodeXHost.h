/*
 * NOVODEX LOCAL MODIFICATION
 * upstream: none -- this file has no counterpart in OPCODE 1.3.
 *
 * The seam between vendored OPCODE and the host engine. Two things cross it in
 * the shipped DLL, and neither exists upstream:
 *
 * [1] allocation. Where stock OPCODE emits `operator new`/`operator delete`,
 *     the image calls the engine's allocator singleton and dispatches through
 *     its vtable.
 *     established at 0x000b4000, the getter, which lazily installs a default
 *     object at .data:0x00122368 into .data:0x0012845c and hands it back;
 *     allocation is `call dword ptr [edx]` with (size, 0) -- 0x000b4eef,
 *     0x000e919e -- and release is `call dword ptr [edx+0x0c]` -- 0x000b4db7,
 *     0x000b4eb7, 0x000b4f77, 0x000e3342, 0x000e9258.
 *
 * [2] error reporting. SetIceError, a two-argument no-op macro upstream,
 *     became a real reporter carrying __FILE__ and __LINE__.
 *     established at 0x000539b0 (31 bytes), which forwards
 *     (2, file, line, 0, message) to the error-stream pointer at
 *     .data:0x001041b4 and returns false. Its callers push message, file, line:
 *     0x000e903b pushes 230, 0x000e912f pushes 147.
 *
 * The implementations live on the host side (Physics/src/ThirdPartyHost.cpp)
 * because both are engine rows, not OPCODE rows. This header is the only place
 * the vendored tree names them.
 */
#ifndef __OPCODE_NOVODEX_HOST_H__
#define __OPCODE_NOVODEX_HOST_H__

#include <new>
#include <stddef.h>

// 0x000b4000 -> [vtable+0x00] (size, 0)
void*	opcNovodeXAlloc(size_t size);
// 0x000b4000 -> [vtable+0x0c] (pointer)
void	opcNovodeXFree(void* memory);
// 0x000539b0
bool	opcNovodeXSetIceError(const char* message, const char* file, int line);

template<class T> inline T* opcNovodeXNew()
{
	void* memory = opcNovodeXAlloc(sizeof(T));
	return memory ? new (memory) T : 0;
}

template<class T> inline void opcNovodeXDelete(T* object)
{
	if(object) { object->~T(); opcNovodeXFree(object); }
}

#endif // __OPCODE_NOVODEX_HOST_H__
