/*----------------------------------------------------------------------------*\
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/

/*
The Scene's pruning engine (the object at Scene+0x624): its add and remove
(scene-raycast block, Task 3), its pruner factory for the two types shapes use,
and what Scene.cpp and SceneRaycast.cpp reach without OPCODE types. The
pruners are IcePruner.cpp.

	0x000b5180	AddObject
	0x000b5260	RemoveObject

NOT RECONSTRUCTED. The factory (0x000b5090) builds four types; shapes use types
0 and 2 only, and only those two are built here. Not claimed.

Runs at API time under 0x027f like IcePruner.cpp; default architecture.
*/

#include "IcePruner.h"

#include <string.h>
#include <new>

// Scene.cpp's emulation of the process-wide object 0x000b4cc0 creates.
bool nxOpcodeEnsurePool();

///////////////////////////////////////////////////////////////////////////////
// The engine: the object at Scene+0x624. Its bounds are the six floats at
// +0x04 (the minimum) and +0x10 (the maximum); its pruners are the four words
// at +0x1c, one per pruning type.

static inline_ Pruner*& nxEnginePruner(void* engine, udword type)
{
	return *reinterpret_cast<Pruner**>(static_cast<ubyte*>(engine) + 0x1c + type*4);
}

// The factory, types 0 and 2 (see the file comment). The base constructor's
// first-use creation of the process-wide object (0x000b4cc0) is Scene.cpp's
// emulation, run where the constructor would run it: after the pruner's own
// allocation.
Pruner* nxPruningEngineCreatePruner(udword type)
{
	if(type == 0)
	{
		void* memory = opcNovodeXAlloc(sizeof(StaticPruner));
		if(!memory)
			return null;
		nxOpcodeEnsurePool();
		return new(memory) StaticPruner;
	}
	if(type == 2)
	{
		void* memory = opcNovodeXAlloc(sizeof(DynamicPruner));
		if(!memory)
			return null;
		nxOpcodeEnsurePool();
		return new(memory) DynamicPruner;
	}
	return null;
}

void nxPruningEngineDestroyPruner(Pruner* pruner)
{
	if(!pruner)
		return;
	pruner->~Pruner();
	opcNovodeXFree(pruner);
}

// phys_fn_004857 (0x000b5180, 213 B)
// A prunable with no handle and a type below 4 goes to its type's pruner,
// created on first use. The engine's bounds are copied into the pruner when
// they are not empty (every min not above its max, 0x000b51ba-0x000b51df).
// After the pruner's AddObject the prunable points at the pruner; if it got a
// handle, its box is marked stale and the pruner's UpdateObject runs.
__declspec(noinline) bool nxPruningEngineAddObject(void* engine, Prunable* object)
{
	if(object->mHandle != PRUNABLE_INVALID_HANDLE || object->mPruningType >= 4)
		return false;

	if(!nxEnginePruner(engine, object->mPruningType))
		nxEnginePruner(engine, object->mPruningType) = nxPruningEngineCreatePruner(object->mPruningType);

	const float* bounds = reinterpret_cast<const float*>(static_cast<ubyte*>(engine) + 4);
	if(!(bounds[0] > bounds[3]) && !(bounds[1] > bounds[4]) && !(bounds[2] > bounds[5]))
		memcpy(&nxEnginePruner(engine, object->mPruningType)->mBounds, bounds, sizeof(AABB));

	nxEnginePruner(engine, object->mPruningType)->AddObject(object);

	Pruner* pruner = nxEnginePruner(engine, object->mPruningType);
	object->mPruner = pruner;
	if(object->mHandle == PRUNABLE_INVALID_HANDLE || object->mPruningType >= 4)
		return false;
	object->mFlags &= ~PRUNABLE_FLAG_WORLD_AABB_VALID;
	return pruner->UpdateObject(object);
}

// phys_fn_004859 (0x000b5260, 55 B)
__declspec(noinline) bool nxPruningEngineRemoveObject(void* engine, Prunable* object)
{
	if(object->mHandle == PRUNABLE_INVALID_HANDLE || object->mPruningType >= 4)
		return false;
	Pruner* pruner = nxEnginePruner(engine, object->mPruningType);
	if(!pruner)
		return false;
	return pruner->RemoveObject(object);
}

///////////////////////////////////////////////////////////////////////////////
// What Scene.cpp and SceneRaycast.cpp reach, without OPCODE types. Not rows:
// the Scene's own registration (whose rows are not reconstructed) and the two
// table slots the engine loops phys_fn_004861 and phys_fn_004864 dispatch
// through.

// The member-function slot host of ObjectModel.h, declared identically.
struct NxSlotCtx { };
typedef void (NxSlotCtx::*NxSlotMfp2)(unsigned, unsigned);
typedef void (NxSlotCtx::*NxSlotMfp5)(unsigned, unsigned, unsigned, unsigned, unsigned);

// Pointers to virtual members: each is a thunk that dispatches through the
// object's own table (slot 4 at +0x10, slot 6 at +0x18), which is what the
// engine loops do.
NxSlotMfp2 nxPrunerSetExternalBufferSlot()
{
	typedef void (Pruner::*Slot)(udword, udword*);
	Slot slot = &Pruner::SetExternalBuffer;
	return reinterpret_cast<NxSlotMfp2&>(slot);
}

NxSlotMfp5 nxPrunerRaycastSlot()
{
	typedef bool (Pruner::*Slot)(Container&, const Ray&, float, bool, udword);
	Slot slot = &Pruner::Raycast;
	return reinterpret_cast<NxSlotMfp5&>(slot);
}

// A shape's prunable is at +0xa4. Its pruning type and section are set by
// the caller's registration: a single root shape in section 1, a compound's
// children in section 0 and its group shape in section 2 (measured on the
// pinned DLL's pools).
bool nxSceneEngineAddShape(void* engine, void* shape, udword type, udword section)
{
	Prunable* prunable = reinterpret_cast<Prunable*>(static_cast<ubyte*>(shape) + 0xa4);
	prunable->mPruningType = ubyte(type);
	prunable->mPruningSection = ubyte(section);
	return nxPruningEngineAddObject(engine, prunable);
}

bool nxSceneEngineRemoveShape(void* engine, void* shape)
{
	return nxPruningEngineRemoveObject(engine,
		reinterpret_cast<Prunable*>(static_cast<ubyte*>(shape) + 0xa4));
}

void nxSceneEngineDestroyPruners(void* engine)
{
	for(udword type = 0; type < 4; type++)
	{
		nxPruningEngineDestroyPruner(nxEnginePruner(engine, type));
		nxEnginePruner(engine, type) = null;
	}
}

// Row 000503's last call (0x0001017e): the engine's four-pointer loop
// (phys_fn_004861) hands every pruner the Scene's shared buffer through slot 4.
void nxFourSlotLoop4861(void* self, unsigned a, unsigned b, NxSlotMfp2 slot);

void nxSceneEngineSetExternalBuffer(void* engine, unsigned capacity, void* entries)
{
	nxFourSlotLoop4861(engine, capacity, unsigned(entries), nxPrunerSetExternalBufferSlot());
}
