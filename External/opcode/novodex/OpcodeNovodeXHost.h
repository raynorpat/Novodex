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

// [3] The classes whose storage the image takes from the allocator singleton
//     carry these four operators, so every `new`/`delete` of them -- including
//     the compiler-generated ones: the array cookie of `new T[n]`, the vector
//     and scalar deleting destructors -- reaches the seam, and nothing else
//     does. The image draws the line by class, not by call site: the model,
//     tree, node, builder and sweep-and-prune classes go through the getter;
//     the colliders' deleting destructors and PlanesCollider's Plane array go
//     through the CRT's operator new[]/delete (0x000f48c0, 0x000f41f0).
//     established at 0x000f0890 (AABBTreeNode's vector deleting destructor
//     frees through [getter+0x0c]), 0x000f24c4..0x000f24ed (`new
//     AABBCollisionNode[n]`: getter, (28n+4, 0), cookie, vector constructor
//     iterator), 0x000e935b (`new AABBQuantizedNoLeafTree`), 0x000538c0 (the
//     AABBTreeBuilder deleting destructor), 0x000f3fb0 (a tree's scalar deleting
//     destructor); against 0x000ba6c0 (RayCollider's, CRT operator delete) and
//     0x000e159a (PlanesCollider::InitQuery, CRT operator new[]).
#define OPC_NOVODEX_ALLOCATEABLE																	static void*	operator new(size_t size)			{ return opcNovodeXAlloc(size);	}		static void*	operator new[](size_t size)			{ return opcNovodeXAlloc(size);	}		static void		operator delete(void* memory)		{ opcNovodeXFree(memory);		}		static void		operator delete[](void* memory)		{ opcNovodeXFree(memory);		}

#endif // __OPCODE_NOVODEX_HOST_H__
