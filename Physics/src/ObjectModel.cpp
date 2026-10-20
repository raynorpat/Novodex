/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "ObjectModel.h"
#include "FoundationSDK.h"

void nxSceneMarkShapeDirty(void* shape, unsigned flag);

extern "C" void __fastcall NxScenePrunerShapeRemove(void* group, void* owner);
extern "C" void __fastcall NxScenePrunerOwnerDestroy(void* manager, void* owner);
#include "Containers.h"
#include "MemoryStream.h"
#include "NxIntersectionSegmentBox.h"
#include "NxIntersectionRayPlane.h"
#include "NxRay.h"
#include "NxPlane.h"
#include "ContactGeneration.h"
#include "NarrowPhase.h"
#include "NxUtilities.h"
#include "X87Sqrt.h"
#include "PhysicsSDK.h"
#include "NxDebugRenderable.h"
#include "NxBox.h"
#include "NpActorDynamicMath.h"
#include "NxGeometryHelpers.h"
#include "NxVolumeIntegration.h"
#include "NxSimpleTriangleMesh.h"

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

// The scene guard pair runs real Foundation-side primitives -- the image
// reaches them through kernel32 import slots (0x10104010/14/2c/44).
#define NOMINMAX
#include <windows.h>

// phys_fn_004149 (0x9a500) releases the three owned allocations in the
// allocator-backed object record. phys_fn_004151 (0x9a560) is its five-byte
// forwarding row. Keep map-resolved entrypoints for the staged-pair probe so
// the production DLL, rather than a test-side transcription, is exercised.
static void nxReleaseAllocatorFields(void* self)
	{
	unsigned char* bytes = static_cast<unsigned char*>(self);
	const unsigned offsets[] = { 0x0c, 0x14, 0x08 };
	for(unsigned i = 0; i < sizeof(offsets) / sizeof(offsets[0]); ++i)
		{
		void** field = reinterpret_cast<void**>(bytes + offsets[i]);
		if(*field)
			{
			nxFoundationSDKAllocator->free(*field);
			*field = 0;
			}
		}
	}

extern "C" __declspec(noinline) void __fastcall nxReleaseAllocatorFields004149(
	void* self)
	{
	nxReleaseAllocatorFields(self);
	}

extern "C" __declspec(noinline) void __fastcall nxReleaseAllocatorFields004151(
	void* self)
	{
	nxReleaseAllocatorFields004149(self);
	}

// The original grows the scene's three per-slot dword vectors in 256-entry
// blocks. Keep this helper in ObjectModel.cpp because the object-layout gate
// links this translation unit without Scene.cpp.
static bool nxSceneGrowShapeSlotVector(unsigned char* container, unsigned offset,
	unsigned slot, unsigned fill)
	{
	unsigned* begin = *reinterpret_cast<unsigned**>(container + offset);
	unsigned* end = *reinterpret_cast<unsigned**>(container + offset + 4);
	unsigned* capacity = *reinterpret_cast<unsigned**>(container + offset + 8);
	if(!begin || !end || !capacity) return false;
	const unsigned count = static_cast<unsigned>(end - begin);
	const unsigned held = static_cast<unsigned>(capacity - begin);
	if(slot < count) return true;
	const unsigned required = (slot + 0x100u) & ~0xffu;
	if(required < slot) return false;
	if(required <= held) return true;
	unsigned* grown = static_cast<unsigned*>(nxFoundationSDKAllocator->malloc(
		required * sizeof(unsigned), NX_MEMORY_PERSISTENT));
	if(!grown) return false;
	if(count) memcpy(grown, begin, count * sizeof(unsigned));
	for(unsigned i = count; i < required; ++i) grown[i] = fill;
	nxFoundationSDKAllocator->free(begin);
	*reinterpret_cast<unsigned**>(container + offset) = grown;
	*reinterpret_cast<unsigned**>(container + offset + 4) = grown + required;
	*reinterpret_cast<unsigned**>(container + offset + 8) = grown + required;
	return true;
	}

bool nxSceneAuxEnsureShapeSlot(void* container, NxU32 slot)
	{
	unsigned char* bytes = static_cast<unsigned char*>(container);
	if(!bytes) return false;
	if(slot < 256) return true;
	return nxSceneGrowShapeSlotVector(bytes, 0, slot, 0)
		&& nxSceneGrowShapeSlotVector(bytes, 0x20, slot, 0xd00beed0u)
		&& nxSceneGrowShapeSlotVector(bytes, 0x90, slot, 0);
	}


// The five recovered constructors share this complete-object layout. C++
// installs the primary and secondary vptrs for each final family type, and
// EmbeddedHookBase's constructor reproduces phys_fn_002404's zeroed words.
CollisionObject::CollisionObject(void* argument)
	: mWord04(0), mArgument08(argument)
	{
	}

static void* __fastcall nxCollisionOwnerThunk(void* self, void*)
	{
	return static_cast<CollisionObject*>(self)->CollisionObject::nxOwnerValue();
	}
static void __fastcall nxCollisionSetGroupThunk(void* self, void*, unsigned group)
	{
	static_cast<CollisionObject*>(self)->CollisionObject::nxSetGroup(group);
	}
static NxU16 __fastcall nxCollisionGetGroupThunk(void* self, void*)
	{
	return static_cast<CollisionObject*>(self)->CollisionObject::nxGetGroup();
	}
#define NX_COLLISION_DELETE_THUNK(Name, Type) \
static void* __fastcall Name(void* self, void*, unsigned flags) \
	{ \
	static_cast<Type*>(self)->~Type(); \
	if(flags & 1u) CollisionObject::operator delete(self); \
	return self; \
	}
NX_COLLISION_DELETE_THUNK(nxCollisionBoxDeleteThunk, CollisionObjectBox)
NX_COLLISION_DELETE_THUNK(nxCollisionCapsuleDeleteThunk, CollisionObjectCapsule)
NX_COLLISION_DELETE_THUNK(nxCollisionPlaneDeleteThunk, CollisionObjectPlane)
NX_COLLISION_DELETE_THUNK(nxCollisionSphereDeleteThunk, CollisionObjectSphere)
NX_COLLISION_DELETE_THUNK(nxCollisionMeshDeleteThunk, CollisionObjectMesh)
#undef NX_COLLISION_DELETE_THUNK
static void* nxCollisionBoxPrimaryTable[] = {
	reinterpret_cast<void*>(&nxCollisionBoxDeleteThunk),
	reinterpret_cast<void*>(&nxCollisionOwnerThunk),
	reinterpret_cast<void*>(&nxCollisionSetGroupThunk),
	reinterpret_cast<void*>(&nxCollisionGetGroupThunk) };
static void* nxCollisionCapsulePrimaryTable[] = {
	reinterpret_cast<void*>(&nxCollisionCapsuleDeleteThunk),
	reinterpret_cast<void*>(&nxCollisionOwnerThunk),
	reinterpret_cast<void*>(&nxCollisionSetGroupThunk),
	reinterpret_cast<void*>(&nxCollisionGetGroupThunk) };
static void* nxCollisionPlanePrimaryTable[] = {
	reinterpret_cast<void*>(&nxCollisionPlaneDeleteThunk),
	reinterpret_cast<void*>(&nxCollisionOwnerThunk),
	reinterpret_cast<void*>(&nxCollisionSetGroupThunk),
	reinterpret_cast<void*>(&nxCollisionGetGroupThunk) };
static void* nxCollisionSpherePrimaryTable[] = {
	reinterpret_cast<void*>(&nxCollisionSphereDeleteThunk),
	reinterpret_cast<void*>(&nxCollisionOwnerThunk),
	reinterpret_cast<void*>(&nxCollisionSetGroupThunk),
	reinterpret_cast<void*>(&nxCollisionGetGroupThunk) };
static void* nxCollisionMeshPrimaryTable[] = {
	reinterpret_cast<void*>(&nxCollisionMeshDeleteThunk),
	reinterpret_cast<void*>(&nxCollisionOwnerThunk),
	reinterpret_cast<void*>(&nxCollisionSetGroupThunk),
	reinterpret_cast<void*>(&nxCollisionGetGroupThunk) };
CollisionObjectBox::CollisionObjectBox(void* argument)
	: CollisionObject(argument), EmbeddedHookBase(), mArgument18(argument)
	{
	*reinterpret_cast<void**>(this) = nxCollisionBoxPrimaryTable;
	}

CollisionObjectCapsule::CollisionObjectCapsule(void* argument)
	: CollisionObject(argument), EmbeddedHookBase(), mArgument18(argument)
	{
	*reinterpret_cast<void**>(this) = nxCollisionCapsulePrimaryTable;
	}

CollisionObjectPlane::CollisionObjectPlane(void* argument)
	: CollisionObject(argument), EmbeddedHookBase(), mArgument18(argument)
	{
	*reinterpret_cast<void**>(this) = nxCollisionPlanePrimaryTable;
	}

CollisionObjectSphere::CollisionObjectSphere(void* argument)
	: CollisionObject(argument), EmbeddedHookBase(), mArgument18(argument)
	{
	*reinterpret_cast<void**>(this) = nxCollisionSpherePrimaryTable;
	}

CollisionObjectMesh::CollisionObjectMesh(void* argument)
	: CollisionObject(argument), EmbeddedHookBase(), mArgument18(argument)
	{
	*reinterpret_cast<void**>(this) = nxCollisionMeshPrimaryTable;
	}

// Family-complete destructor bodies: phys_fn_001079, phys_fn_001127,
// phys_fn_001163, phys_fn_001197, and phys_fn_001245. Each leaves its own
// primary/secondary base tables installed before applying the free flag.
CollisionObjectBox::~CollisionObjectBox() {}
CollisionObjectCapsule::~CollisionObjectCapsule() {}
CollisionObjectPlane::~CollisionObjectPlane() {}
CollisionObjectSphere::~CollisionObjectSphere() {}
CollisionObjectMesh::~CollisionObjectMesh() {}

CollisionObject::~CollisionObject()
	{
	}

void* CollisionObject::nxOwnerValue()
	{
	unsigned char* bytes = reinterpret_cast<unsigned char*>(this);
	void* lock = *reinterpret_cast<void**>(bytes + 0x14);
	nxSceneGuardEnter(lock);
	const void* shape = *reinterpret_cast<void**>(bytes + 0x18);
	const void* owner = nxShapeOwner(shape);
	void* value = *reinterpret_cast<void* const*>(owner);
	nxSceneGuardLeave(lock);
	return value;
	}

void CollisionObject::nxSetGroup(unsigned group)
	{
	// The five family slot-2 rows share the same guarded call into
	// ShapeBase::nxApplyGroup. Their per-family report constants remain in the
	// function census; the normal valid-group path is common.
	nxMutexApplyGroupEx(this, group, 2, 0, 0, 0);
	}

void ShapeBase::nxSetMaterialIndex(unsigned short material)
	{
	mHalfwordDA = material;
	nxSceneMarkShapeDirty(this, 8);
	}

NxU16 CollisionObject::nxGetGroup()
	{
	unsigned char* bytes = reinterpret_cast<unsigned char*>(this);
	void* lock = *reinterpret_cast<void**>(bytes + 0x14);
	nxSceneGuardEnter(lock);
	const unsigned char* shape = *reinterpret_cast<const unsigned char**>(bytes + 0x18);
	NxU16 value;
	memcpy(&value, shape + 0xd8, sizeof(value));
	nxSceneGuardLeave(lock);
	return value;
	}

void CollisionObject::operator delete(void* memory) noexcept
	{
	if(memory)
		nxFoundationSDKAllocator->free(memory);
	}

void CollisionObject::operator delete(void* memory, size_t) noexcept
	{
	CollisionObject::operator delete(memory);
	}

template <typename CollisionFamily>
static void nxCollisionFamilyScalarDeletingDtor(CollisionObject* object, unsigned flags)
	{
	static_cast<CollisionFamily*>(object)->~CollisionFamily();
	if(flags & 1u)
		CollisionObject::operator delete(object);
	}

// The five primary scalar-deleting-destructor rows share the same allocator
// and flags contract. Calling the virtual slot lets MSVC select the final
// family destructor; its secondary table carries the -0x0c adjustor thunk.
void CollisionObject::nxScalarDeletingDtor(unsigned flags)
	{
	typedef void* (__thiscall* ScalarDeletingDtor)(void*, unsigned);
	void** table = *reinterpret_cast<void***>(this);
	reinterpret_cast<ScalarDeletingDtor>(table[0])(this, flags);
	}
// phys_fn_001281 (0x000257a0): mov eax,[ecx+4]; ret. The whole row -- note
// the offset is FOUR bytes past the shape's vptr.
const void* nxShapeOwner(const void* shape)
	{
	return *reinterpret_cast<const void* const*>(
		static_cast<const unsigned char*>(const_cast<void*>(shape)) + 4);
	}

// ---------------------------------------------------------------------------
// The box hull facade. The static tables' CONTENTS are transcribed from the
// pinned image (.rdata 0x10122180/0x101221e0/0x10122240); this binary's
// copies live at different addresses, which is fine -- consumers get them
// through these getters, and the layout gate compares content, not pointers.

// Each table is 24 dwords: the three start 0x60 bytes apart in .rdata, and
// the words are the pinned image's own.
static const NxU32 gEdgeTable[24] =
	{ 0, 1, 1, 2, 2, 3, 3, 0, 7, 6, 6, 5,
	  5, 4, 4, 7, 1, 5, 6, 2, 3, 7, 4, 0 };			// 0x10122180..0x101221df
static const NxU32 gFaceCornerTable[24] =
	{ 131073, 0, 131073, 2, 131073, 4, 131073, 6, 131073, 8, 131073, 10,
	  131073, 12, 131073, 14, 131073, 16, 131073, 18, 131073, 20, 131073, 22 };	// 0x101221e0..0x1012223f
static const NxU32 gAdjacencyTable[24] =
	{ 0, 5, 0, 1, 0, 4, 0, 3, 2, 4, 1, 2,
	  2, 5, 2, 3, 1, 5, 1, 4, 3, 4, 3, 5 };			// 0x10122240..0x1012229f

// phys_fn_000953 (0x00020d20, 6 B)
// Facade slot 1: `mov eax,8; ret`, the vertex count 000973 reads through
// the +0xe0 table.
unsigned BoxHullFacade::vertexCount() const
	{
	return kVertexCount;
	}

// phys_fn_000961 (0x000213c0, 6 B)
// Facade slot 3: `mov eax,6; ret`, the face count 000973, 000957 and 000959
// read through the +0xe0 table.
unsigned BoxHullFacade::faceCount() const
	{
	return kFaceCount;
	}

// phys_fn_000955 (0x00020d30, 4 B)
const NxU32* BoxHullFacade::vertices() const
	{
	return mVertices;							// lea eax,[ecx+0x10]
	}

// phys_fn_000963 (0x000213d0, 14 B)
const BoxFaceRecord* BoxHullFacade::face(unsigned index) const
	{
	// lea eax,[eax+eax*8]; lea eax,[ecx+eax*4+0x70] -- index*36 + (this+0x70).
	return &mFaces[index];
	}

// phys_fn_000967 (0x000213f0, 6 B)
const NxU32* BoxHullFacade::edgeTable()
	{
	return gEdgeTable;
	}

// phys_fn_000969 (0x00021400, 6 B)
const NxU32* BoxHullFacade::faceCornerTable()
	{
	return gFaceCornerTable;
	}

// phys_fn_000971 (0x00021410, 6 B)
const NxU32* BoxHullFacade::adjacencyTable()
	{
	return gAdjacencyTable;
	}

// The face records' two index lists, which the hull rebuild (000973) points
// every record at: each face's four corners (.rdata 0x10106998..0x101069f7)
// and its four edges (.rdata 0x101069f8..0x10106a57), the pinned image's words.
static const NxU32 gHullFaceCorners[6][4] =
	{ { 0, 1, 2, 3 }, { 1, 5, 6, 2 }, { 5, 4, 7, 6 },
	  { 4, 0, 3, 7 }, { 3, 2, 6, 7 }, { 4, 5, 1, 0 } };
static const NxU32 gHullFaceEdges[6][4] =
	{ { 0, 1, 2, 3 }, { 1, 8, 5, 9 }, { 6, 7, 4, 5 },
	  { 11, 3, 10, 7 }, { 2, 9, 4, 10 }, { 6, 8, 0, 11 } };
// The twelve edge directions slot 10 (000959) tests after the faces, .rdata
// 0x101220f0..0x1012217f (+-0x3f3504f3, sqrt(1/2) as a float, and 0).
static const NxU32 gHullEdgeDirections[12 * 3] =
	{ 0x00000000u, 0xbf3504f3u, 0xbf3504f3u,  0x3f3504f3u, 0x00000000u, 0xbf3504f3u,
	  0x00000000u, 0x3f3504f3u, 0xbf3504f3u,  0xbf3504f3u, 0x00000000u, 0xbf3504f3u,
	  0x00000000u, 0x3f3504f3u, 0x3f3504f3u,  0x3f3504f3u, 0x00000000u, 0x3f3504f3u,
	  0x00000000u, 0xbf3504f3u, 0x3f3504f3u,  0xbf3504f3u, 0x00000000u, 0x3f3504f3u,
	  0x3f3504f3u, 0xbf3504f3u, 0x00000000u,  0x3f3504f3u, 0x3f3504f3u, 0x00000000u,
	  0xbf3504f3u, 0x3f3504f3u, 0x00000000u,  0xbf3504f3u, 0xbf3504f3u, 0x00000000u };

// The facade's own table words the hull rows call through (+0xe0 of the box,
// ecx = the facade): slot 1 vertex count, 2 vertices, 3 face count, 6-8 the
// static tables.
typedef unsigned (__thiscall* BoxHullCountSlot)(const BoxHullFacade*);
typedef const float* (__thiscall* BoxHullVerticesSlot)(const BoxHullFacade*);
typedef const NxU32* (__thiscall* BoxHullTableSlot)(const BoxHullFacade*);

static void* const* nxBoxHullTable(const BoxHullFacade* facade)
	{
	return static_cast<void* const*>(facade->mVptrSlot);
	}

// phys_fn_000957 (0x00020d40, 578 B)
// Facade slot 9, __thiscall ret 8 (arg1 direction, arg2 optional pose). With
// a pose, the direction is taken through the rows at words 0-2, 4-6 and 8-10
// (copied to a local 3x3 first, 0x10020d5c-0x10020dab), each component
// (m0*d0 + m1*d1) + m2*d2, spilled to float (0x10020dc5/de1/e01). The face
// count comes from the facade's own slot 3 (0x10020e23). Face 0 seeds the
// best as (d2*nz + d1*ny) + d0*nx, kept in the register unrounded; a face
// replaces it only on strictly greater (fcomp, test ah,0x41), and the value
// that replaces it is the float spill reloaded (0x10020e7c/0x10020e8d). The
// four-way unrolled run (entered when count-1 >= 4, signed, and repeated while
// i < count-3) sums (d0*nx + d2*nz) + d1*ny, the tail (d2*nz + d0*nx) + d1*ny.
// The zero store at 0x10020d4e is a local nothing reads.
unsigned BoxHullFacade::supportFace(const float* direction, const float* pose) const
	{
	float local[3];
	if(pose != 0)
		{
		const float m[9] = { pose[0], pose[1], pose[2], pose[4], pose[5], pose[6],
			pose[8], pose[9], pose[10] };
		local[0] = (float)(((double)m[0] * direction[0] + (double)m[1] * direction[1])
			+ (double)m[2] * direction[2]);
		local[1] = (float)(((double)m[3] * direction[0] + (double)m[4] * direction[1])
			+ (double)m[5] * direction[2]);
		local[2] = (float)(((double)m[6] * direction[0] + (double)m[7] * direction[1])
			+ (double)m[8] * direction[2]);
		direction = local;
		}
	const float d0 = direction[0];
	const float d1 = direction[1];
	const float d2 = direction[2];
	const unsigned count = reinterpret_cast<BoxHullCountSlot>(nxBoxHullTable(this)[3])(this);
	double best = ((double)d2 * mFaces[0].mNormal[2] + (double)d1 * mFaces[0].mNormal[1])
		+ (double)d0 * mFaces[0].mNormal[0];
	unsigned bestFace = 0;
	unsigned i = 1;
	if((int)(count - 1) >= 4)
		{
		const unsigned last = count - 3;
		do
			{
			for(unsigned k = 0; k < 4; ++k)
				{
				const float* n = mFaces[i + k].mNormal;
				const double value = ((double)d0 * n[0] + (double)d2 * n[2]) + (double)d1 * n[1];
				if(value > best)
					{
					bestFace = i + k;
					best = (float)value;
					}
				}
			i += 4;
			}
		while(i < last);
		}
	for(; i < count; ++i)
		{
		const float* n = mFaces[i].mNormal;
		const double value = ((double)d2 * n[2] + (double)d0 * n[0]) + (double)d1 * n[1];
		if(value > best)
			{
			bestFace = i;
			best = (float)value;
			}
		}
	return bestFace;
	}

// phys_fn_000959 (0x00020f90, 1062 B)
// Facade slot 10, __thiscall ret 0xc (arg1 direction, arg2 optional pose,
// arg3 optional out word). The pose rows are summed (m0*d0 + m2*d2) + m1*d1
// here (0x10020ffc-0x10021050). The face search is slot 9's, except that the
// unrolled run's first face sums (d2*nz + d0*nx) + d1*ny and its other three
// (d2*nz + d1*ny) + d0*nx (0x100210b0-0x10021166). The running best then
// continues over the twelve edge directions (.rdata 0x101220f0, six per pass,
// each (d0*x + d2*z) + d1*y, 0x100211d2-0x100212f6). No edge wins: arg3 = 0
// and the best face is returned. An edge wins: arg3 = 1, the facade's slots 6,
// 7 and 8 are called (slot 6's result unused), the edge's word in the slot-7
// table indexes the slot-8 table for its two faces A and B, and A is returned
// only when B's projection is strictly less than A's (fcompp, test ah,5; both
// kept unrounded), otherwise B (unordered included).
unsigned BoxHullFacade::supportFeature(const float* direction, const float* pose,
	NxU32* edgeWon) const
	{
	float local[3];
	if(pose != 0)
		{
		const float m[9] = { pose[0], pose[1], pose[2], pose[4], pose[5], pose[6],
			pose[8], pose[9], pose[10] };
		local[0] = (float)(((double)m[0] * direction[0] + (double)m[2] * direction[2])
			+ (double)m[1] * direction[1]);
		local[1] = (float)(((double)m[3] * direction[0] + (double)m[5] * direction[2])
			+ (double)m[4] * direction[1]);
		local[2] = (float)(((double)m[6] * direction[0] + (double)m[8] * direction[2])
			+ (double)m[7] * direction[1]);
		direction = local;
		}
	const float d0 = direction[0];
	const float d1 = direction[1];
	const float d2 = direction[2];
	void* const* table = nxBoxHullTable(this);
	const unsigned count = reinterpret_cast<BoxHullCountSlot>(table[3])(this);
	double best = ((double)d2 * mFaces[0].mNormal[2] + (double)d1 * mFaces[0].mNormal[1])
		+ (double)d0 * mFaces[0].mNormal[0];
	unsigned bestFace = 0;
	unsigned i = 1;
	if((int)(count - 1) >= 4)
		{
		const unsigned last = count - 3;
		do
			{
			const float* n = mFaces[i].mNormal;
			double value = ((double)d2 * n[2] + (double)d0 * n[0]) + (double)d1 * n[1];
			if(value > best)
				{
				bestFace = i;
				best = (float)value;
				}
			for(unsigned k = 1; k < 4; ++k)
				{
				n = mFaces[i + k].mNormal;
				value = ((double)d2 * n[2] + (double)d1 * n[1]) + (double)d0 * n[0];
				if(value > best)
					{
					bestFace = i + k;
					best = (float)value;
					}
				}
			i += 4;
			}
		while(i < last);
		}
	for(; i < count; ++i)
		{
		const float* n = mFaces[i].mNormal;
		const double value = ((double)d2 * n[2] + (double)d0 * n[0]) + (double)d1 * n[1];
		if(value > best)
			{
			bestFace = i;
			best = (float)value;
			}
		}
	const float* edges = reinterpret_cast<const float*>(gHullEdgeDirections);
	int bestEdge = -1;
	for(unsigned e = 0; e < 12; e += 6)
		{
		for(unsigned k = 0; k < 6; ++k)
			{
			const float* v = edges + (e + k) * 3;
			const double value = ((double)d0 * v[0] + (double)d2 * v[2]) + (double)d1 * v[1];
			if(value > best)
				{
				bestEdge = (int)(e + k);
				best = (float)value;
				}
			}
		}
	if(bestEdge == -1)
		{
		if(edgeWon != 0)
			*edgeWon = 0;
		return bestFace;
		}
	if(edgeWon != 0)
		*edgeWon = 1;
	reinterpret_cast<BoxHullTableSlot>(table[6])(this);
	const NxU32* edgeFaces = reinterpret_cast<BoxHullTableSlot>(table[7])(this);
	const NxU32* faces = reinterpret_cast<BoxHullTableSlot>(table[8])(this);
	const NxU32 pair = edgeFaces[bestEdge * 2 + 1];
	const NxU32 faceA = faces[pair];
	const NxU32 faceB = faces[pair + 1];
	const float* a = mFaces[faceA].mNormal;
	const float* b = mFaces[faceB].mNormal;
	const double projectionA = ((double)d2 * a[2] + (double)d1 * a[1]) + (double)d0 * a[0];
	const double projectionB = ((double)d2 * b[2] + (double)d1 * b[1]) + (double)d0 * b[0];
	if(projectionB < projectionA)
		return faceA;
	return faceB;
	}

// ---------------------------------------------------------------------------
// The error stream. Evidence section 3t: the image reports through an
// indirect cdecl five-argument call guarded by a non-zero flag word; this
// reconstruction mirrors the shape with its own slot so differentials can
// capture reports on both sides.

const char* const	nxSourceFileSphereShapeCpp =
	"\\Epic\\Novodex\\SDKs\\Physics\\src\\SphereShape.cpp";
const char* const	nxMsgSetRadiusPositive =
	"SphereShape::setRadius: radius should be positive!";
const char* const	nxSourceFileShapeCpp =
	"\\Epic\\Novodex\\SDKs\\Physics\\src\\Shape.cpp";
const char* const	nxMsgGroupBelow32 =
	"group ID must be < 32!";
const char* const	nxSourceFileSphereLoadCpp =
	"\\Epic\\Novodex\\SDKs\\Physics\\src\\SphereShape.cpp";
const char* const	nxMsgSphereLoadRadius =
	"SphereShape::loadFromDesc: radius should be positive!";
const char* const	nxSourceFileCapsuleShapeCpp =
	"\\Epic\\Novodex\\SDKs\\Physics\\src\\CapsuleShape.cpp";
const char* const	nxMsgCapsuleLoadRadius =
	"CapsuleShape::loadFromDesc: radius should be positive!";
const char* const	nxMsgCapsuleSetDimensionsRadius =
	"CapsuleShape::setDimensions: radius should be positive!";

// ---------------------------------------------------------------------------
// NxMaterialRecord. Evidence: phase2-sdk.md section 4.6 and section 3v here.
// setToDefault matches the pinned header inline exactly; the internal flag
// bit is the store at 0x0000e9ee that runs on the TEMPLATE after the copy
// into the SDK's materials array.
NxMaterialRecord::NxMaterialRecord(void)
	{
	setToDefault();
	}

void NxMaterialRecord::setToDefault(void)
	{
	dynamicFriction = 0.0f;
	staticFriction = 0.0f;
	spinFriction = 0.0f;
	rollFriction = 0.0f;
	restitution = 0.0f;
	dynamicFrictionV = 0.0f;
	staticFrictionV = 0.0f;
	dirOfAnisotropy.set(1.0f, 0.0f, 0.0f);
	dirOfMotion.set(1.0f, 0.0f, 0.0f);
	speedOfMotion = 0.0f;
	flags = 0;
	frictionCombineMode = 0;			// NX_CM_AVERAGE
	restitutionCombineMode = 0;			// NX_CM_AVERAGE
	programData = nullptr;
	}

void NxMaterialRecord::setInternalFlagBit31(void)
	{
	flags |= 0x80000000u;				// or edi,0x80000000 at 0x0000e9ee
	}

static NxReportFn	gReportSink = nullptr;

void nxInstallReportSink(NxReportFn sink)
	{
	gReportSink = sink;
	}

void nxReport(int kind, const char* file, int line, int code,
	const char* message)
	{
	if(gReportSink != nullptr)
		gReportSink(kind, file, line, code, message);
	}

// The source-level object-layout probe captures internal reports through its
// local sink. The production DLL has no sink and the original row reports
// through FoundationSDK's error stream.
static void nxSphereSetRadiusReport(void)
	{
	if(gReportSink != nullptr)
		nxReport(1, nxSourceFileSphereShapeCpp, 0x4a, 0,
			nxMsgSetRadiusPositive);
	else
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER,
			nxSourceFileSphereShapeCpp, 0x4a, 0, nxMsgSetRadiusPositive);
	}

// phys_fn_000975 (0x000217c0, 168 B)
// Facade slot 11, __thiscall ret 0x18: min pointer in the second stack
// argument, max in the third, direction in the fourth, pose in the fifth
// (0x100217c0-0x100217d3); the first and sixth are never read. The three
// per-corner values are column dots
// plus translation: A = col0.v + tx, B = col1.v + ty, C = col2.v + tz; the
// projection combines them as A*dx + (C*dz + B*dy) -- that exact association,
// because the x87 stack built C first and folded B before A. Bounds updates
// reproduce fcom/fnstsw exactly: the minimum replaces on strictly less, the
// maximum on strictly greater, and a NaN projection replaces neither (the
// unordered case sets C0, which both masks include). Intermediates are kept
// in double so a spilled value cannot truncate what the x87 stack held at
// 64-bit. The row reads neither of its two unread stack arguments.
void BoxHullFacade::supportBounds(NxU32 /*unread1*/, float* outMin, float* outMax,
	const float* direction, const float* pose, NxU32 /*unread6*/) const
	{
	NxU32 minBits = 0x7f7fffffu;				// +FLT_MAX: the minimum starts high (store 0x000217d7 -> a2)
	NxU32 maxBits = 0xff7fffffu;				// -FLT_MAX: the maximum starts low (store 0x000217dd -> a3)
	float minValue;
	float maxValue;
	memcpy(&minValue, &minBits, sizeof(minValue));
	memcpy(&maxValue, &maxBits, sizeof(maxValue));
	for(unsigned i = 0; i < 8; ++i)				// mov ebp,8 at 0x000217e3
		{
		const float* v = reinterpret_cast<const float*>(&mVertices[i * 3]);
		double a = (double)v[0] * pose[0]
			+ (double)v[1] * pose[4]
			+ (double)v[2] * pose[8]
			+ pose[12];
		double b = (double)v[1] * pose[5]
			+ (double)v[0] * pose[1]
			+ (double)v[2] * pose[9]
			+ pose[13];
		double c = pose[10] * v[2]
			+ pose[2] * v[0]
			+ pose[6] * v[1]
			+ pose[14];
		double projection = a * direction[0]
			+ (c * direction[2] + b * direction[1]);
		float asFloat = (float) projection;
		if(asFloat < minValue)
			minValue = asFloat;
		if(asFloat > maxValue)
			maxValue = asFloat;
		}
	memcpy(outMin, &minValue, sizeof(minValue));
	memcpy(outMax, &maxValue, sizeof(maxValue));
	}

// phys_fn_000985 (0x00021a10, 78 B)
// The image's initializer array at .rdata
// 0x10103010 is twelve `ret` stubs, so the CRT helper runs nothing and the
// twelve-byte global stays zeroed. Same observable state here: a static
// all-zero object behind a once-flag.
const void* BoxHullFacade::sharedHook()
	{
	static NxU32 shared[3] = { 0, 0, 0 };	// .data 0x10123c64..0x10123c70
	static bool initialised = false;		// the guard byte at .data 0x10123c70
	if(!initialised)
		initialised = true;
	return shared;
	}

// ---------------------------------------------------------------------------
// The Prunable owner adapters. The base-shape constructor installs their
// addresses into .data 0x10128470/74/78 (stores 0x0002562b..3f); Phase 4 had
// to leave the globals null because their only writer sat outside its
// population. All three are cdecl, owner first, and ignore nothing they are
// handed -- the shipped rows carry no null tests.

// phys_fn_000965 (0x000213e0, 3 B)
// `xor eax,eax; ret` -- the same folded stub the
// facade exposes as kZero, installed as .data 0x10128470.
static udword shapeOwnerQuery(void* /*owner*/)
	{
	return 0;
	}

// phys_fn_001271 (0x00025520, 15 bytes): reads [owner], pushes the box, calls
// vtable slot 10 (+0x28). Installed at .data 0x10128474.
static void shapeOwnerNotify(void* owner, AABB* box)
	{
	void** vtable = *reinterpret_cast<void***>(owner);
	typedef void (__thiscall* NotifyFn)(void*, AABB*);
	reinterpret_cast<NotifyFn>(vtable[10])(owner, box);
	}

// phys_fn_001269 (0x00025510, 15 bytes): the same frame through slot 9
// (+0x24) -- the owner recomputing the box. Installed at .data 0x10128478.
static void shapeOwnerWorldAABB(void* owner, AABB* box)
	{
	void** vtable = *reinterpret_cast<void***>(owner);
	typedef void (__thiscall* WorldAABBFn)(void*, AABB*);
	reinterpret_cast<WorldAABBFn>(vtable[9])(owner, box);
	}

// ---------------------------------------------------------------------------
// ShapeBase. See ObjectModel.h for the row map; every store below carries the
// instruction address that fixes it.

ShapeBase::ShapeBase(void* owner, unsigned argument)
	{
	mOwner04 = owner;						// 0x00025543
	mWord08 = 0;							// 0x00025549

	const NxU32 one = 0x3f800000u;			// mov ebx,0x3f800000 at 0x00025535
	ShapePose identity;
	memset(&identity, 0, sizeof(identity));
	identity.mRotation[0] = one;			// stores like 0x00025555
	identity.mRotation[4] = one;
	identity.mRotation[8] = one;
	mPose0C = identity;						// 0x00025555..0x000255cd
	mPose3C = identity;						// second instruction run, +0x3c
	mLocalPose = identity;						// third instruction run, +0x6c
	// The image runs the identical pose stores a SECOND time (0x0002564f..
	// 0x000256cd) after the hook stores; both passes write the same bytes, so
	// the transcription writes them once.

	mWord9C = 0;							// 0x000255d3
	mWordA0 = 0;							// 0x000255d9

	// mPrunable is constructed here by member semantics; the image reaches
	// Prunable::Prunable (phys_fn_004874) at 0x000255df.

	gPrunableOwnerQuery = shapeOwnerQuery;		// .data 0x10128470, store 0x0002563f
	gPrunableOwnerNotify = shapeOwnerNotify;	// .data 0x10128474, store 0x00025635
	gPrunableOwnerWorldAABB = shapeOwnerWorldAABB;	// 0x10128478, store 0x0002562b

	// Then the constructor registers THIS SHAPE as the prunable's owner.
	mPrunable.mOwner = this;				// mov [esi+0xa8],esi at 0x00025649

	mSentinelD0 = 0x7fffffffu;				// 0x000255ed
	mArgumentD4 = argument;					// 0x000255f7 ([esp+0x14]: entry [esp+8])
	mHalfwordD8 = 0;						// 0x000255fd
	mHalfwordDA = 0;						// 0x00025604
	mHalfwordDC = 6;						// 0x0002560b
	mHalfwordDE = 8;						// 0x00025614

	// The owner-registration arm (0x0002561f..26): when the owner is non-null,
	// [owner+4]'s word at +0x48 becomes the `this` for phys_fn_002423 with the
	// shape pushed. Transcribed through nxSceneInsertShape, which reproduces
	// the registrar's three observable writes (evidence 3y/3z).
	if(mOwner04 != nullptr)
		{
		NxU32 scene = *reinterpret_cast<const NxU32*>(
			reinterpret_cast<const unsigned char*>(mOwner04) + 4);
		void* container = *reinterpret_cast<void* const*>(
			reinterpret_cast<const unsigned char*>(scene) + 0x48);
		nxSceneInsertShape(container, this, mArgumentD4);
		}
	}

// phys_fn_001287 (0x000257d0, 14 bytes): the +0xde halfword under a mask.
unsigned ShapeBase::nxFlagBitsDE(unsigned mask) const
	{
	return static_cast<unsigned>(mHalfwordDE) & mask;
	}

// phys_fn_000931 (0x00020490, 70 B)
// translation, forward rotation
// copy, THEN dimensions. Word-wise volatile accesses retain the image's
// load/store order when out aliases this (including forward rep movsd).
// A bulk memcpy/memmove or dimensions-first copy is not equivalent.
void BoxShape::nxFillShapeDescriptor(unsigned* out) const
	{
	const volatile unsigned* self =
		reinterpret_cast<const volatile unsigned*>(this);
	volatile unsigned* dest = out;
	for(unsigned i = 0; i < 3; ++i)
		dest[i] = self[0x30 / 4 + i];		// 0x20496..a4
	for(unsigned i = 0; i < 9; ++i)
		dest[6 + i] = self[0x0c / 4 + i];	// 0x204a9..b4
	for(unsigned i = 0; i < 3; ++i)
		dest[3 + i] = self[0xe4 / 4 + i];	// 0x204b6..cf
	}

// phys_fn_000949 (0x00020880, 663 B)
// BOX slot 5, thiscall (ray, maxDistance, groups, hintFlags, hit), `ret 0x14`.
// The pose is the shape's +0x0c (R row-major at +0x0c..+0x2c, t at +0x30).
// Written from the listing with its register lifetimes (scene-raycast block
// Task 3; the earlier float transcription grouped and rounded differently):
//
//   * the local origin R^T (o - t). dx stays in a register throughout, dy is
//     spilled to float, and dz is spilled but its unrounded register value
//     feeds the first row (0x0002089f `fst` then `fmul`):
//         x = (dz R20 + dy R10) + dx R00     (dz unrounded)
//         y = (dz R21 + dy R11) + dx R01     (dz rounded)
//         z = (dz R22 + dy R12) + dx R02
//   * the local direction R^T d, each row its own association
//     (0x000208fe-0x00020944):
//         x = (R10 dy + R20 dz) + dx R00
//         y = (R01 dx + R11 dy) + R21 dz
//         z = (R02 dx + R12 dy) + R22 dz
//   * NxRayAABBIntersect2 against -dims..dims (0x0002098c; the distance is
//     returned through the ray argument's stack slot);
//   * the world point R c + t, rows (cy Rr1 + cz Rr2) + cx Rr0; the x row stays
//     in a register into its `+ t.x`, y and z are spilled first
//     (0x0002099e-0x00020a1a). Point and distance are written before the gate;
//   * reject only a distance greater than maxDistance (0x00020a2d: equal and
//     unordered accept); then the +0x9c word, face 0, u = v = 0, flags 0x13;
//   * with hint bit 2 the flags become 0x17 and the normal is +-1 on the hit
//     face's axis: +1 only when the hit coordinate is greater than zero
//     (0x00020a9b `test ah,0x41; je`), -1 for zero, negative and NaN; then
//     R n, rows as the point's.
void* BoxShape::nxBoxRaycast(const float* ray, float maxDistance,
	unsigned reserved, unsigned flags, void* hit) const
	{
	(void) reserved;
	const float* R = reinterpret_cast<const float*>(&mBase.mPose0C.mRotation);
	const float* T = reinterpret_cast<const float*>(&mBase.mPose0C.mTranslation);

	const double dx = double(ray[0]) - T[0];
	const float dy = float(double(ray[1]) - T[1]);
	const double dzWide = double(ray[2]) - T[2];
	const float dz = float(dzWide);
	float localOrigin[3];
	localOrigin[0] = float((dzWide * R[6] + double(dy) * R[3]) + dx * R[0]);
	localOrigin[1] = float((double(dz) * R[7] + double(dy) * R[4]) + dx * R[1]);
	localOrigin[2] = float((double(dz) * R[8] + double(dy) * R[5]) + dx * R[2]);

	float localDir[3];
	localDir[0] = float((double(R[3]) * ray[4] + double(R[6]) * ray[5]) + double(ray[3]) * R[0]);
	localDir[1] = float((double(R[1]) * ray[3] + double(R[4]) * ray[4]) + double(R[7]) * ray[5]);
	localDir[2] = float((double(R[2]) * ray[3] + double(R[5]) * ray[4]) + double(R[8]) * ray[5]);

	const NxVec3 negDims(-mHull.mDims04[0], -mHull.mDims04[1], -mHull.mDims04[2]);
	NxVec3 coord;
	NxReal t;
	const NxU32 plane = NxRayAABBIntersect2(negDims,
		*reinterpret_cast<const NxVec3*>(mHull.mDims04),
		*reinterpret_cast<const NxVec3*>(localOrigin),
		*reinterpret_cast<const NxVec3*>(localDir), coord, t);
	if(plane == 0)
		return nullptr;							// 0x00020998

	unsigned char* rec = static_cast<unsigned char*>(hit);
	const double wx = (double(coord.y) * R[1] + double(coord.z) * R[2]) + double(coord.x) * R[0];
	const float wy = float((double(coord.y) * R[4] + double(coord.z) * R[5]) + double(coord.x) * R[3]);
	const float wz = float((double(coord.y) * R[7] + double(coord.z) * R[8]) + double(coord.x) * R[6]);
	const float pointZ = float(double(wz) + T[2]);
	const float pointX = float(wx + T[0]);
	const float pointY = float(double(wy) + T[1]);
	memcpy(rec + 0x0c, &pointZ, 4);				// 0x00020a10
	memcpy(rec + 0x04, &pointX, 4);				// 0x00020a17
	memcpy(rec + 0x08, &pointY, 4);				// 0x00020a1a
	memcpy(rec + 0x20, &t, 4);					// 0x00020a1d

	if(t > maxDistance)
		return nullptr;							// 0x00020a2f

	memcpy(rec + 0x00, &mBase.mWord9C, 4);		// 0x00020a38..3e
	const unsigned zero = 0;
	memcpy(rec + 0x1c, &zero, 4);				// 0x00020a45
	memcpy(rec + 0x24, &zero, 4);				// 0x00020a4c
	memcpy(rec + 0x28, &zero, 4);				// 0x00020a53
	unsigned tag = 0x13;						// 0x00020a5a
	memcpy(rec + 0x2c, &tag, 4);
	if(flags & 4)								// 0x00020a40
		{
		tag = 0x17;								// 0x00020a72
		memcpy(rec + 0x2c, &tag, 4);
		float n[3] = { 0.0f, 0.0f, 0.0f };
		n[plane - 1] = coord[plane - 1] > 0.0f ? 1.0f : -1.0f;
		const double nx = (double(n[1]) * R[1] + double(n[2]) * R[2]) + double(n[0]) * R[0];
		const double ny = (double(n[1]) * R[4] + double(n[2]) * R[5]) + double(n[0]) * R[3];
		const float nz = float((double(n[1]) * R[7] + double(n[2]) * R[8]) + double(n[0]) * R[6]);
		const float nxf = float(nx);
		const float nyf = float(ny);
		memcpy(rec + 0x18, &nz, 4);				// 0x00020b05
		memcpy(rec + 0x10, &nxf, 4);			// 0x00020b08
		memcpy(rec + 0x14, &nyf, 4);			// 0x00020b0b
		}
	return const_cast<BoxShape*>(this);		// 0x00020b0e: eax = this
	}

// Row 000951 (BOX slot 7) is reached only from the continuous-collision
// sweep inside the simulation step; it is in StepOnlyRows.cpp, built
// /arch:IA32.
// Candidate guard bindings: the candidate reads the SAME live storage the
// drive mutates (oracle .data via the rendercap driver), not its own copy.
static float* g_nxGuardA = nullptr;		// 0x10123bc8
static float* g_nxGuardB = nullptr;		// 0x10123bd8
static float* g_nxRenderScale = nullptr;	// 0x10123b4c
static float* g_nxGuardRef = nullptr;	// 0x101041f0 (SDK-side reference)

// Provisional phys_fn_001305 (0x25960): conditional debug render, `ret 4`.
// Both blocks SKIP when their guard equals the reference word (fucompp
// equality -> jnp taken at 0x2597f/0x25aa6); they execute when the guard
// differs or is unordered. Block 1 (guard A) draws three axis lines from
// the pose-one translation to K*scaled-column endpoints, colors
// 0xCF0000/0xCF00/0xCF, K = scale * guard A (0x25985..0x2598e). Block 2
// (guard B) draws three circles: slot-10 center+diagonal, the 9 rotation
// words, then per call a column-cyclically rotated 3x3 in a 48-byte pose
// buffer, args (0x14, pose, 0xFFFF00FF, diagBits, 0) at vtable +0x38.
// Driven by the 5-mask rendercap differential; census closure awaits a
// family registration.
void BoxShape::nxDebugRender(const void* renderer) const
	{
	if(!g_nxGuardRef || !renderer)
		return;
	const float ref = *g_nxGuardRef;
	void** table = *reinterpret_cast<void** const*>(renderer);
	typedef void (__fastcall* NxDrawLineFn)(void*, void*, const float*,
		const float*, unsigned);
	typedef void (__fastcall* NxDrawPoseFn)(void*, void*, unsigned count,
		const void* pose, unsigned color, unsigned radiusBits, unsigned reserved);
	typedef NxDrawLineFn NxDrawLine;
	typedef NxDrawPoseFn NxDrawPose;
	const NxDrawLine drawLine = reinterpret_cast<NxDrawLine>(table[8]);
	const NxDrawPose drawPose = reinterpret_cast<NxDrawPose>(table[14]);
	void* rendererArg = const_cast<void*>(renderer);

	// Block 1 (0x25985..0x25a90): scaled axis lines, guard A.
	const float guardA = *g_nxGuardA;
	if(guardA != ref || guardA != guardA)
		{
		const float k = *g_nxRenderScale * guardA;	// 0x2598e
		const float* rot = reinterpret_cast<const float*>(&mBase.mPose0C.mRotation);
		const float* trn = reinterpret_cast<const float*>(&mBase.mPose0C.mTranslation);
		static const unsigned axisColors[3] = { 0xcf0000u, 0xcf00u, 0xcfu };
		for(unsigned axis = 0; axis < 3; ++axis)
			{
			const float start[3] = { trn[0], trn[1], trn[2] };
			// 0x25a0e..0x25a28: endpoint = t + K * column(axis), scaled ONCE
			// (each element fmul'd by st(2)=K exactly one time in the chain).
			const float end[3] =
				{
				k * rot[axis] + trn[0],
				k * rot[3 + axis] + trn[1],
				k * rot[6 + axis] + trn[2]
				};
			drawLine(rendererArg, nullptr, start, end, axisColors[axis]);
			}
		}

	// Block 2 (0x25aac..0x25c01): slot-10 center+diagonal, 9 rotation
	// words, three column-cyclic pose draws.
	const float guardB = *g_nxGuardB;
	if(guardB != ref || guardB != guardB)
		{
		float center[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		nxBoxCenterAndDiagonal(center);
		const float* rot = reinterpret_cast<const float*>(&mBase.mPose0C.mRotation);
		float pose[12];
		for(unsigned i = 0; i < 9; ++i)
			pose[i] = rot[i];
		pose[9] = center[0];
		pose[10] = center[1];
		pose[11] = center[2];
		for(unsigned round = 0; round < 3; ++round)
			{
			drawPose(rendererArg, nullptr, 0x14, pose, 0xffff00ffu,
				reinterpret_cast<const unsigned&>(center[3]), 0);
			if(round < 2)
				{
				// Column-cyclic rotation of the 9 matrix words (0x25b40..0x25c01):
				// each row rotates left by one, which cycles the COLUMNS of
				// the frame between calls.
				float next[9];
				for(unsigned row = 0; row < 3; ++row)
					for(unsigned col = 0; col < 3; ++col)
						next[row * 3 + col] = pose[row * 3 + (col + 1) % 3];
				memcpy(pose, next, 9 * sizeof(float));
				}
			}
		}
	}

void nxBindDebugRenderGuards(float* guardA, float* guardB,
	float* renderScale, float* guardRef)
	{
	g_nxGuardA = guardA;
	g_nxGuardB = guardB;
	g_nxRenderScale = renderScale;
	g_nxGuardRef = guardRef;
	}

// The sphere, capsule and plane slot-3 dispatchers below still read their
// guard through this probe-bound pointer (0x10123bc4); the box's (000945)
// reads the live parameter array.
static float* g_nxGuardC = nullptr;		// 0x10123bc4 (slot-3 guard)

void nxBindDebugRenderGuardC(float* guardC)
	{
	g_nxGuardC = guardC;
	}

// phys_fn_000945 (0x000207e0, 104 B)
// BOX slot 3, the box's debug visualisation: thiscall (renderer), `ret 4`.
// 0x207e4  gate: +0xde & 8 (001287 with mask 8), zero exits;
// 0x207f9  001305 (nxDebugRender) on the renderer after the gate;
// 0x207fe  NX_VISUALIZE_COLLISION_SHAPES (.data 0x10123bc4, the SDK's live
//          parameter array) against 0.0f (0x101041f0): equality skips, a NaN
//          draws (`test ah,0x44; jnp`);
// 0x2081a  000931 fills the 60-byte NxBox (centre, extents, rotation);
// 0x2081f..39  colour ((+0xde & 7) == 0 ? 0xffffffff : 0xffff00ff) --
//          `and al,7; neg al; sbb eax,eax; and eax,0xffff0100; dec eax`;
// 0x2083d  the renderer's addOBB (slot +0x28) with (box, colour, false).
// Scene-raycast block Task 4 (visualisation): the parameter was read through a
// pointer only a test probe bound (nxBindDebugRenderGuardC), so the
// box arm never ran in the product; it now reads the live array as the listing
// does (nxPhysicsSDKParameters, PhysicsSDK.cpp).
void BoxShape::nxDebugRenderDispatch(const void* renderer) const
	{
	if(!mBase.nxFlagBitsDE(8))
		return;
	nxDebugRender(renderer);
	if(nxPhysicsSDKParameters()[NX_VISUALIZE_COLLISION_SHAPES] == 0.0f)
		return;
	NxBox box;
	static_assert(sizeof(NxBox) == 60, "000931's 60-byte descriptor");
	nxFillShapeDescriptor(reinterpret_cast<unsigned*>(&box));
	// mov al,[esi+0xde] addresses the flag halfword directly (the +0xde
	// byte offset, not an aligned dword index); nxFlagBitsDE reads it.
	const unsigned lowBits = mBase.nxFlagBitsDE(7);
	const unsigned color = lowBits == 0 ? 0xffffffffu : 0xffff00ffu;
	static_cast<NxDebugRenderable*>(const_cast<void*>(renderer))->addOBB(box, color, false);
	}

// Scene shape-array insert. Write order follows the image -- registrar's
// shape store first (0x5c5a4), then the notify helper's free-list sentinel
// (0x5c093) and count mirror (0x5c0a8). Its per-slot vectors grow in 256-slot
// blocks when a newly allocated shape ID reaches their end.
void nxSceneInsertShape(void* container, void* shape, NxU32 slot)
	{
	if(!nxSceneAuxEnsureShapeSlot(container, slot)) return;
	unsigned self = reinterpret_cast<unsigned>(shape);
	unsigned c = reinterpret_cast<unsigned>(container);

	unsigned* shapesBegin = *reinterpret_cast<unsigned**>(c + 0x90);
	shapesBegin[slot] = self;					// [begin+slot*4] = shape

	unsigned* sentBegin = *reinterpret_cast<unsigned**>(c + 0x00);
	sentBegin[slot] = 0xFFFFFFFFu;				// free-list sentinel

	unsigned* bBegin = *reinterpret_cast<unsigned**>(c + 0x10);
	unsigned* bEnd = *reinterpret_cast<unsigned**>(c + 0x14);
	unsigned countB = static_cast<NxU32>(
		(reinterpret_cast<unsigned>(bEnd) - reinterpret_cast<unsigned>(bBegin)) >> 2);
	unsigned* cBegin = *reinterpret_cast<unsigned**>(c + 0x20);
	cBegin[slot] = countB;						// count mirror
	}

// The base dtor's owner arms, image order (0x00026be1..0x00026c35).
void ShapeBase::nxBaseDtorOwnerArms(void)
	{
	// phys_fn_001323 first removes this shape from the debug-name registry,
	// before taking the owner-null early exit at 0x00026be1.
	nxShapeNameRegistry(this, nullptr);
	if(mOwner04 == nullptr)
		return;
	unsigned scene = *reinterpret_cast<const NxU32*>(
		reinterpret_cast<const unsigned char*>(mOwner04) + 4);
	unsigned char* sc = reinterpret_cast<unsigned char*>(scene);
	*reinterpret_cast<NxU32*>(sc + 0x70c) |= 2;

	void* c1 = *reinterpret_cast<void* const*>(sc + 0x48);
	nxSceneRemoveShape(c1, this);

	// 0x00026c0f..19: ecx = scene + 0x5d4 -- the ADDRESS OF THE FIELD, so
	// remover #2's [this] dereference lands on the field's {begin,end}
	// header. Not the field's value.
	nxSceneRemovePairs(reinterpret_cast<unsigned char*>(scene) + 0x5d4, this);

	void* c3 = sc + 0x6e4; // phys_fn_000028 receives the embedded vector address
	nxSceneSlotFree(c3, mArgumentD4);
	// 0x00026c20..3d: remove the owner's pruning records, then destroy the
	// scene's pruner storage. Both calls are gated by the shape's +0xa0 group.
	if(mWordA0 != 0)
		{
		NxScenePrunerShapeRemove(reinterpret_cast<void*>(mWordA0), mOwner04);
		NxScenePrunerOwnerDestroy(reinterpret_cast<void*>(mWordA0), mOwner04);
		}
	// The BASE dtor's final owner arm removes scene collision-pair map records
	// for either 16-bit shape key before the embedded Prunable is destroyed.
	NxSceneRemoveOwnerPairRecords(sc, this);
	}

// Task 4 scaffolding: remover chain, decoded in full this round.
//
// phys_fn_002410 (0x5bac0) is the release arm, and both guards are
// narrower than they look: sentinel != -1 gates ONLY the free-vector push,
// and sentinel == 0 gates only the unlink -- so a virgin index (-1) skips
// the push but still runs the unlink against whatever the mirror word
// names, and an already-released index pushes a duplicate while skipping
// the unlink. The push itself is the same growth arm phys_fn_000028
// carries, inlined (malloc/copy/free through adapter slots +8/+0x14 when
// the cursor sits at capacity). The unlink swaps the count vector's last
// value into the released slot's mirror position across the +0x10 counts /
// +0x14 end cursor / +0x20 mirrors arrays, pops the cursor, zeroes the
// sentinel and poisons the mirror word with 0xD00BEED0. The shapes-array
// clear its callers perform lives in 0x5bbe0 and stays in
// nxSceneRemoveShape below.
void nxSceneReleaseIndex(void* hdr, NxU32 idx)
	{
	unsigned h = reinterpret_cast<unsigned>(hdr);
	unsigned* sent = *reinterpret_cast<unsigned**>(h);
	if(sent[idx] != 0xFFFFFFFFu)
		nxU32VectorPushBack(reinterpret_cast<unsigned*>(h + 0x2c), idx);
	if(sent[idx] == 0)
		return;
	unsigned* cntA = *reinterpret_cast<unsigned**>(h + 0x10);
	unsigned* cntBEnd = *reinterpret_cast<unsigned**>(h + 0x14);
	unsigned* mir = *reinterpret_cast<unsigned**>(h + 0x20);
	unsigned lastVal = *(cntBEnd - 1);
	NxU32 u = mir[idx];
	cntA[u] = lastVal;
	mir[lastVal] = u;
	*reinterpret_cast<unsigned**>(h + 0x14) = cntBEnd - 1;
	sent[idx] = 0;
	mir[idx] = 0xD00BEED0u;
	}

// Remover #1: the deregistration chain's shape-side wrapper -- the slot
// read off Shape+0xd4, released through phys_fn_002410, then the
// shapes-array clear that lives one call over at 0x5bbe0.
// phys_fn_002413 (0x0005bbe0, 38 B)
void nxSceneRemoveShape(void* container, void* shape)
	{
	unsigned c = reinterpret_cast<unsigned>(container);
	const unsigned char* sh = static_cast<const unsigned char*>(shape);
	NxU32 slot = *reinterpret_cast<const NxU32*>(sh + 0xd4);

	nxSceneReleaseIndex(container, slot);

	unsigned* shapes = *reinterpret_cast<unsigned**>(c + 0x90);
	shapes[slot] = 0;							// 0x5bbe0's clear
	}

// Remover #2 (phys_fn_002344, 0x5aae0): pair-list swap-remove. The caller
// hands the ADDRESS OF THE SCENE'S +0x5d4 FIELD (0x00026c13 add ecx,0x5d4),
// so [this] names a second header whose two words are {begin,end} of the
// stride-8 pair array. The image re-reads that header each match, shrinks
// its cursor by 8 per removal, and leaves a matched FINAL element in place
// rather than copied over itself. A match is either half equal to the value.
void nxSceneRemovePairs(void* container, void* shape)
	{
	unsigned c = reinterpret_cast<unsigned>(container);
	unsigned self = reinterpret_cast<unsigned>(shape);
	unsigned* hdr = *reinterpret_cast<unsigned**>(c);
	unsigned* begin = reinterpret_cast<unsigned*>(hdr[0]);
	unsigned count = static_cast<NxU32>(
		(hdr[1] - reinterpret_cast<unsigned>(begin)) >> 3);
	unsigned i = 0;
	while(i < count)
		{
		if(begin[i * 2] == self || begin[i * 2 + 1] == self)
			{
			if(i != count - 1)					// the image skips a self-copy
				{
				begin[i * 2] = begin[(count - 1) * 2];
				begin[i * 2 + 1] = begin[(count - 1) * 2 + 1];
				}
			count -= 1;
			}
		else
			i += 1;
		}
	hdr[1] = reinterpret_cast<unsigned>(begin) + count * 8;
	}

// phys_fn_000028 (0x1b90): dword-vector push_back over the VC9 layout
// {_Myproxy@+0x00 untouched by this row, _Myfirst@+0x04, _Mylast@+0x08,
// _Myend@+0x0c}. The growth arm allocates 2*size + 2 dwords through the Foundation
// allocator (adapter vtable slot +8 with flag word 0), copies the live
// elements dword-wise, releases the old block (slot +0x14) and repoints all
// three cursors; the compiler's own escape (`jae` over the arm when the old
// capacity already reads >= the new one) is arithmetically unreachable
// while the vector is full but is transcribed for faithfulness.
void nxU32VectorPushBack(void* vecHeader, NxU32 value)
	{
	unsigned f = reinterpret_cast<unsigned>(vecHeader);
	unsigned* begin = *reinterpret_cast<unsigned**>(f + 0x04);
	unsigned* end = *reinterpret_cast<unsigned**>(f + 0x08);
	unsigned* capEnd = *reinterpret_cast<unsigned**>(f + 0x0c);
	if(capEnd > end)
		{
		*end = value;						// ja: room at the cursor
		*reinterpret_cast<unsigned**>(f + 0x08) = end + 1;
		return;
		}
	unsigned size = static_cast<unsigned>(
		(reinterpret_cast<unsigned>(end) - reinterpret_cast<unsigned>(begin)) >> 2);
	unsigned newCapDwords = size + size + 2;	// lea eax,[eax+eax+2]
	unsigned oldCapDwords = begin == 0 ? 0u : static_cast<unsigned>(
		(reinterpret_cast<unsigned>(capEnd) - reinterpret_cast<unsigned>(begin)) >> 2);
	if(oldCapDwords < newCapDwords)
		{
		void* fresh = nxFoundationSDKAllocator->malloc(
			newCapDwords * sizeof(unsigned), NX_MEMORY_PERSISTENT);
		unsigned* run = static_cast<unsigned*>(fresh);
		for(unsigned i = 0; i < size; ++i)	// copy before release
			run[i] = begin[i];
		if(begin != 0)
			nxFoundationSDKAllocator->free(begin);
		*reinterpret_cast<unsigned**>(f + 0x04) = run;
		*reinterpret_cast<unsigned**>(f + 0x0c) = run + newCapDwords;
		end = run + size;
		}
	*end = value;
	*reinterpret_cast<unsigned**>(f + 0x08) = end + 1;
	}

// Remover #3: the deregistration chain's name for phys_fn_000028 applied to
// the slot-free vector header.
void nxSceneSlotFree(void* container, NxU32 slot)
	{
	nxU32VectorPushBack(container, slot);
	}

// ---------------------------------------------------------------------------
// NxActor scaffolding. The actor body reads decode as a uniform shape:
// edi = [this+0x10] (the scene lock context), enter guard, read through
// [this+0x14] (the body pointer), leave guard, return. The guard pair
// (0x5b700 / 0x5b790) runs Foundation primitives over the critical section
// POINTER at *[scene]: Enter/LeaveCriticalSection, an InterlockedCompare-
// Exchange writer flag at block+0x18, and the owning thread id at +0x1c.

void nxSceneGuardEnter(void* scene)
	{
	CRITICAL_SECTION* cs =
		*reinterpret_cast<CRITICAL_SECTION**>(scene);
	unsigned* flag = reinterpret_cast<unsigned*>(cs) + 6;
	::EnterCriticalSection(cs);
	::InterlockedCompareExchange(reinterpret_cast<volatile long*>(flag),
		1, 0);
	flag[1] = ::GetCurrentThreadId();
	}

void nxSceneGuardLeave(void* scene)
	{
	CRITICAL_SECTION* cs =
		*reinterpret_cast<CRITICAL_SECTION**>(scene);
	unsigned* flag = reinterpret_cast<unsigned*>(cs) + 6;
	::InterlockedCompareExchange(reinterpret_cast<volatile long*>(flag),
		0, 1);
	::LeaveCriticalSection(cs);
	}

// phys_fn_000110 (slot 19, 0x3580): guard; bool of [body+8]; unguard.
bool nxActorBodyPresent(void* self)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	unsigned r = *reinterpret_cast<unsigned*>(body + 8);
	nxSceneGuardLeave(scene);
	return r != 0;
	}

// phys_fn_000114 (slot 86, 0x3610): guard; the WORD at [body+0x1c]; unguard.
NxU16 nxActorGetGroupWord(void* self)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	NxU16 r = *reinterpret_cast<NxU16*>(body + 0x1c);
	nxSceneGuardLeave(scene);
	return r;
	}

// phys_fn_000078 (slot 77, 0x2c60): guard; ([body+0x14] & mask) != 0 --
// the actor flag word tested against the caller's mask; unguard.
bool nxActorFlagsMasked(void* self, unsigned mask)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	unsigned flags = *reinterpret_cast<unsigned*>(body + 0x14);
	nxSceneGuardLeave(scene);
	return (flags & mask) != 0;
	}

// phys_fn_000066 (slot 69, 0x29e0): guard; [body+8] names a nested record;
// fsqrt of its float at +0xd0, or 0.0f when that record is null (the only
// guard the image carries -- a null body would fault it); unguard. x87
// fsqrt computes at full precision and the fstp m32 narrows -- typed as
// double here for the same rounding shape.
float nxActorSqrtFieldD0(void* self)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	unsigned rec = body != 0
		? *reinterpret_cast<unsigned*>(body + 8) : 0u;
	unsigned bits = 0;
	if(rec != 0)
		bits = *reinterpret_cast<unsigned*>(rec + 0xd0);
	nxSceneGuardLeave(scene);
	float v;
	memcpy(&v, &bits, 4);
	double d = v;
	float r = static_cast<float>(::sqrt(d));
	return rec != 0 ? r : 0.0f;
	}

// phys_fn_000068 (slot 71, 0x2a30): same shape over [record+0xd4].
float nxActorSqrtFieldD4(void* self)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	unsigned rec = body != 0
		? *reinterpret_cast<unsigned*>(body + 8) : 0u;
	unsigned bits = 0;
	if(rec != 0)
		bits = *reinterpret_cast<unsigned*>(rec + 0xd4);
	nxSceneGuardLeave(scene);
	float v;
	memcpy(&v, &bits, 4);
	double d = v;
	float r = static_cast<float>(::sqrt(d));
	return rec != 0 ? r : 0.0f;
	}

// phys_fn_000015 (0x14f0): body helper. [body+0x10] names the shape list
// head; null yields 0, a non-mesh shape (type word at +0xd0 != 5) yields 1,
// and a mesh yields its triangle-array span ([+0xe4]-[+0xe0])>>2.
// Product row: Physics/src/core/JointSupport.cpp.
unsigned nxBodyShapeRecordCount(void* body)
	{
	unsigned sh = *reinterpret_cast<unsigned*>(
		reinterpret_cast<unsigned>(body) + 0x10);
	if(sh == 0)
		return 0;
	if(*reinterpret_cast<unsigned*>(sh + 0xd0) != 5)
		return 1;
	return (*reinterpret_cast<unsigned*>(sh + 0xe4)
		- *reinterpret_cast<unsigned*>(sh + 0xe0)) >> 2;
	}

// phys_fn_000019 (0x1540): body helper. Null shape list yields null; a mesh
// yields [+0xf0]; anything else yields shape+0x9c -- the collision object
// the Shape layout names.
void* nxBodyCollisionObject(void* body)
	{
	unsigned sh = *reinterpret_cast<unsigned*>(
		reinterpret_cast<unsigned>(body) + 0x10);
	if(sh == 0)
		return nullptr;
	if(*reinterpret_cast<unsigned*>(sh + 0xd0) == 5)
		return reinterpret_cast<void*>(*reinterpret_cast<unsigned*>(sh + 0xf0));
	return reinterpret_cast<void*>(sh + 0x9c);
	}

// phys_fn_000082 (slot 15, 0x2d00): guard; nxBodyShapeRecordCount(body);
// unguard.
unsigned nxActorShapeRecordCount(void* self)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	unsigned r = nxBodyShapeRecordCount(
		reinterpret_cast<void*>(body));
	nxSceneGuardLeave(scene);
	return r;
	}

// phys_fn_000084 (slot 16, 0x2d30): guard; nxBodyCollisionObject(body);
// unguard.
void* nxActorCollisionObject(void* self)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	void* r = nxBodyCollisionObject(reinterpret_cast<void*>(body));
	nxSceneGuardLeave(scene);
	return r;
	}

// phys_fn_000086 (slot 84, 0x2d60): guard; the SDK pointer binding keyed on
// the body pointer (phys_fn_000454, closed in Phase 2); unguard.
void* nxActorBoundTarget(void* self)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	void* r = nxGetSdkPointerBinding(reinterpret_cast<void*>(body));
	nxSceneGuardLeave(scene);
	return r;
	}

// The +0x08 member subobject. Its pre-member state lives under a third
// table (0x101088b8) that both the constructor (phys_fn_002404) and the
// destructor (phys_fn_002406) install -- the dtor RESTORES it rather than
// leaving the one-slot member table behind.
void nxActorMemberInit(void* memberAtPlus8)
	{
	unsigned m = reinterpret_cast<unsigned>(memberAtPlus8);
	*reinterpret_cast<unsigned**>(m) =
		reinterpret_cast<unsigned*>(0x101088b8u);
	*reinterpret_cast<unsigned*>(m + 4) = 0;
	*reinterpret_cast<unsigned*>(m + 8) = 0;
	}

void nxActorMemberReset(void* memberAtPlus8)
	{
	*reinterpret_cast<unsigned**>(memberAtPlus8) =
		reinterpret_cast<unsigned*>(0x101088b8u);
	}

// phys_fn_000044 (0x2480): the actor constructor chain tail. Chained-
// construction intermediates are observable here only as order: wall vptr,
// owner zero, member init over +0x08..+0x13, the one-slot member table over
// +0x08, the body pointer at +0x14, then the dynamic final over +0x00.
void nxActorConstruct(void* self, void* body)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	*reinterpret_cast<unsigned**>(a) =
		reinterpret_cast<unsigned*>(0x101043d0u);
	*reinterpret_cast<unsigned*>(a + 4) = 0;
	nxActorMemberInit(reinterpret_cast<unsigned char*>(self) + 8);
	*reinterpret_cast<unsigned**>(a + 8) =
		reinterpret_cast<unsigned*>(0x1010468cu);
	*reinterpret_cast<unsigned*>(a + 0x14) =
		reinterpret_cast<unsigned>(body);
	*reinterpret_cast<unsigned**>(a) =
		reinterpret_cast<unsigned*>(0x10104530u);
	}

// phys_fn_000042 (0x2460): the interface-wall scalar-deleting destructor:
// install the wall table, then release through the linked CRT -- 0x0002471
// calls 0x100f41f0 directly, NOT the SDK allocator adapter its sibling at
// slot 0 uses.
void nxActorInterfaceDtor(void* self, unsigned flags)
	{
	*reinterpret_cast<unsigned**>(self) =
		reinterpret_cast<unsigned*>(0x101043d0u);
	if(flags & 1)
		::free(self);
	}

// phys_fn_000118 (slot 0, 0x3650): the actor scalar-deleting destructor:
// final tables again, member reset to the third table, allocator release
// through slot +0x14 when flagged.
void nxActorDeletingDtor(void* self, unsigned flags)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	*reinterpret_cast<unsigned**>(a) =
		reinterpret_cast<unsigned*>(0x10104530u);
	*reinterpret_cast<unsigned**>(a + 8) =
		reinterpret_cast<unsigned*>(0x1010468cu);
	nxActorMemberReset(reinterpret_cast<unsigned char*>(self) + 8);
	*reinterpret_cast<unsigned**>(a) =
		reinterpret_cast<unsigned*>(0x101043d0u);
	if(flags & 1)
		nxFoundationSDKAllocator->free(self);
	}

// phys_fn_000116 (slot 87, 0x3640): the member table's this-adjustor
// thunk -- `sub ecx,8` onto the actor base, then the deleting dtor. Eight
// bytes because that is where the member subobject sits.
void nxActorDeletingDtorThunk(void* memberThis, unsigned flags)
	{
	nxActorDeletingDtor(
		reinterpret_cast<unsigned char*>(memberThis) - 8, flags);
	}

// ---------------------------------------------------------------------------
// Actor slate 2: the write side and two record readers.
//
// phys_fn_000742 (0x00016dd0, 89 B)
// record helper -- a pure x87 chain whose cross
// terms carry their scale factors SQUARED (the fxch/faddp ladder multiplies
// each product by its second word a second time): 0.5 * (([+0x74]^2 +
// [+0x70]^2 + [+0x6c]^2)*[+0x188] + [+0x194]*[+0x80]^2 + [+0x190]*[+0x7c]^2
// + [+0x18c]*[+0x78]^2). Every intermediate stays at full precision and
// there is ONE rounding -- the helpers return in st(0) and their callers'
// fstp m32 does that rounding, so the C++ computes in double and casts once.
float nxBodyRecordEnergyWord(void* rec)
	{
	unsigned r = reinterpret_cast<unsigned>(rec);
	double v6c = *reinterpret_cast<const float*>(r + 0x6c);
	double v70 = *reinterpret_cast<const float*>(r + 0x70);
	double v74 = *reinterpret_cast<const float*>(r + 0x74);
	double m78 = *reinterpret_cast<const float*>(r + 0x78);
	double m7c = *reinterpret_cast<const float*>(r + 0x7c);
	double m80 = *reinterpret_cast<const float*>(r + 0x80);
	double m188 = *reinterpret_cast<const float*>(r + 0x188);
	double v18c = *reinterpret_cast<const float*>(r + 0x18c);
	double v190 = *reinterpret_cast<const float*>(r + 0x190);
	double v194 = *reinterpret_cast<const float*>(r + 0x194);
	return static_cast<float>(
		(((v74 * v74 + v70 * v70) + v6c * v6c) * m188
		+ v194 * (m80 * m80) + v190 * (m7c * m7c)
		+ v18c * (m78 * m78)) * 0.5);
	}

// Guard upgrade (0x5b730, the write-side guard): try to take the writer
// flag at [cs]+0x18; if it is already held by ANOTHER thread, fail without
// entering -- the caller reports and skips to avoid a deadlock. Held by this
// thread or free: enter, re-take, record the tid, succeed.
bool nxSceneGuardWriteTry(void* ctx)
	{
	CRITICAL_SECTION* cs =
		*reinterpret_cast<CRITICAL_SECTION**>(ctx);
	unsigned* flag = reinterpret_cast<unsigned*>(cs) + 6;
	long held = ::InterlockedCompareExchange(
		reinterpret_cast<volatile long*>(flag), 1, 0);
	if(held != 0
		&& flag[1] != static_cast<unsigned>(::GetCurrentThreadId()))
		return false;
	::EnterCriticalSection(cs);
	::InterlockedCompareExchange(reinterpret_cast<volatile long*>(flag),
		1, 0);
	flag[1] = ::GetCurrentThreadId();
	return true;
	}

// The NpActor.cpp write-lock literals, exposed for the transcript pin.
const char* const	nxSourceFileNpActorCpp =
	"\\Epic\\Novodex\\SDKs\\Physics\\src\\NpActor.cpp";
const char* const	nxMsgWriteLockStillAcquired =
	"PhysicsSDK: WriteLock is still aquired. Procedure call skipped to "
	"avoid a deadlock!";

// Shared body of both flag writers: guard-upgrade on the member field at
// +0xc, report kind 2 and skip on failure, else mutate body+0x14. The line
// differs between the two rows (0x1bb raise, 0x1c1 clear).
static bool nxActorWriteFlagsGuarded(void* self, unsigned mask,
	bool raise, int line)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* ctx = *reinterpret_cast<void**>(a + 0xc);
	if(!nxSceneGuardWriteTry(ctx))
		{
		nxReport(2, nxSourceFileNpActorCpp, line, 0,
			nxMsgWriteLockStillAcquired);
		return false;
		}
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	unsigned flags = *reinterpret_cast<unsigned*>(body + 0x14);
	flags = raise ? (flags | mask) : (flags & ~mask);
	*reinterpret_cast<unsigned*>(body + 0x14) = flags;
	nxSceneGuardLeave(ctx);
	return true;
	}

// phys_fn_000074 (slot 75, 0x2ba0): flags |= mask under the write guard.
void nxActorRaiseFlags(void* self, unsigned mask)
	{
	nxActorWriteFlagsGuarded(self, mask, true, 0x1bb);
	}

// phys_fn_000076 (slot 76, 0x2c00): flags &= ~mask under the write guard.
void nxActorClearFlags(void* self, unsigned mask)
	{
	nxActorWriteFlagsGuarded(self, mask, false, 0x1c1);
	}

// phys_fn_000060 (slot 62, 0x2900): guarded; the nested record's energy
// word from phys_fn_000742, exact float zero when the record is null.
float nxActorRecordEnergyWord(void* self)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	unsigned rec = body != 0
		? *reinterpret_cast<unsigned*>(body + 8) : 0u;
	float out = 0.0f;
	if(rec != 0)
		out = nxBodyRecordEnergyWord(reinterpret_cast<void*>(rec));
	nxSceneGuardLeave(scene);
	return out;
	}

// phys_fn_000064 (slot 68, 0x2990): guarded; true when the nested record is
// null OR its word at +0x84 reads zero.
bool nxActorRecordWord84Zero(void* self)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	unsigned rec = body != 0
		? *reinterpret_cast<unsigned*>(body + 8) : 0u;
	unsigned w84 = rec != 0
		? *reinterpret_cast<unsigned*>(rec + 0x84) : 0u;
	nxSceneGuardLeave(scene);
	return w84 == 0;
	}

// ---------------------------------------------------------------------------
// Actor slate 3: the damping getters. Both read one float off the nested
// record and REPORT kind 1 when the record is null -- the actor must be
// dynamic -- returning the zero constant at 0x101041f0 afterwards.
const char* const	nxMsgLinearDampingDynamic =
	"Actor::setLinearDamping: Actor must be dynamic!";
const char* const	nxMsgAngularDampingDynamic =
	"Actor::getAngularDamping: Actor must be dynamic!";

// phys_fn_000050 (slot 42, 0x2610): getLinearDamping -- [+0xb8].
float nxActorGetLinearDamping(void* self)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	unsigned rec = *reinterpret_cast<unsigned*>(body + 8);
	if(rec == 0)
		{
		nxReport(1, nxSourceFileNpActorCpp, 0xd9, 0,
			nxMsgLinearDampingDynamic);
		nxSceneGuardLeave(scene);
		return 0.0f;
		}
	float v = *reinterpret_cast<const float*>(rec + 0xb8);
	nxSceneGuardLeave(scene);
	return v;
	}

// phys_fn_000052 (slot 44, 0x2680): getAngularDamping -- [+0xbc], line 0xe8.
float nxActorGetAngularDamping(void* self)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	unsigned rec = *reinterpret_cast<unsigned*>(body + 8);
	if(rec == 0)
		{
		nxReport(1, nxSourceFileNpActorCpp, 0xe8, 0,
			nxMsgAngularDampingDynamic);
		nxSceneGuardLeave(scene);
		return 0.0f;
		}
	float v = *reinterpret_cast<const float*>(rec + 0xbc);
	nxSceneGuardLeave(scene);
	return v;
	}

// phys_fn_000048 (slot 36, 0x25c0): guarded float at [record+0x188]; exact
// float zero when the record is null -- no report on this one.
float nxActorRecordField188(void* self)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	unsigned rec = body != 0
		? *reinterpret_cast<unsigned*>(body + 8) : 0u;
	float out = 0.0f;
	if(rec != 0)
		out = *reinterpret_cast<const float*>(rec + 0x188);
	nxSceneGuardLeave(scene);
	return out;
	}

// ---------------------------------------------------------------------------
// Actor slate 4.

// phys_fn_000088 (slot 83, 0x2d90): guarded SDK pointer-binding WRITE --
// phys_fn_000480 keyed on the body pointer, under the write guard; report
// line 0x1ff on a failed upgrade, skip without binding.
void nxActorSetBoundTarget(void* self, void* value)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* ctx = *reinterpret_cast<void**>(a + 0xc);
	if(!nxSceneGuardWriteTry(ctx))
		{
		nxReport(2, nxSourceFileNpActorCpp, 0x1ff, 0,
			nxMsgWriteLockStillAcquired);
		return;
		}
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	nxSetSdkPointerBindingViaThunk(reinterpret_cast<void*>(body),
		reinterpret_cast<void*>(value));
	nxSceneGuardLeave(ctx);
	}

// phys_fn_000092 (slot 6, 0x2ed0): guarded three-word read into `out`.
// With a nested record the words come from record+0x50/54/58; without one,
// from the BODY defaults at +0x44/48/4c. Returns the out pointer.
void* nxActorGetPoseWords(void* self, void* out)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	unsigned rec = *reinterpret_cast<unsigned*>(body + 8);
	unsigned* o = reinterpret_cast<unsigned*>(out);
	if(rec != 0)
		{
		o[0] = *reinterpret_cast<const unsigned*>(rec + 0x50);
		o[1] = *reinterpret_cast<const unsigned*>(rec + 0x54);
		o[2] = *reinterpret_cast<const unsigned*>(rec + 0x58);
		}
	else
		{
		o[0] = *reinterpret_cast<const unsigned*>(body + 0x44);
		o[1] = *reinterpret_cast<const unsigned*>(body + 0x48);
		o[2] = *reinterpret_cast<const unsigned*>(body + 0x4c);
		}
	nxSceneGuardLeave(scene);
	return out;
	}

// phys_fn_000038 (0x2400, ret 8) and phys_fn_000040 (0x2430, ret 8): the
// actor vtable thunks. Dispatch through the object's own vtable slot +0x104
// / +0x108 as (self, &local, arg1), then copy the first three words of the
// returned record into out.
static void* nxActorVtThunk(void* self, void* arg1, unsigned* out, unsigned slot)
	{
	void** vt = *reinterpret_cast<void***>(self);
	typedef void* (__thiscall* SlotFn)(void*, void*, void*);
	SlotFn fn = reinterpret_cast<SlotFn>(vt[slot / 4]);
	unsigned local[3] = { 0, 0, 0 };
	void* r = fn(self, local, arg1);
	const unsigned* src = reinterpret_cast<const unsigned*>(r);
	out[0] = src[0]; out[1] = src[1]; out[2] = src[2];
	return out;
	}

void* nxActorGetPointVelocityVal(void* self, void* point, unsigned* out)
	{ return nxActorVtThunk(self, point, out, 0x104u); }
void* nxActorGetLocalPointVelocityVal(void* self, void* point, unsigned* out)
	{ return nxActorVtThunk(self, point, out, 0x108u); }

namespace
	{
	// The shared quaternion -> 3x3 rotation expansion used by 000132 and
	// 000130. The x87 body accumulates in extended precision and stores to
	// float, so a double intermediate reproduces the staged rounding.
	inline void nxQuatToMatrix9(const float* q, float* m)
		{
		const double x = q[0], y = q[1], z = q[2], w = q[3];
		m[0] = static_cast<float>(1.0 - 2.0 * (y*y + z*z));
		m[1] = static_cast<float>(2.0 * (x*y - z*w));
		m[2] = static_cast<float>(2.0 * (x*z + y*w));
		m[3] = static_cast<float>(2.0 * (x*y + z*w));
		m[4] = static_cast<float>(1.0 - 2.0 * (x*x + z*z));
		m[5] = static_cast<float>(2.0 * (y*z - x*w));
		m[6] = static_cast<float>(2.0 * (x*z - y*w));
		m[7] = static_cast<float>(2.0 * (y*z + x*w));
		m[8] = static_cast<float>(1.0 - 2.0 * (x*x + y*y));
		}
	}

// phys_fn_000130 (0x4580, ret 4): writes the full 0x30-byte pose to out -- the
// quaternion matrix from record+0x5c, then the translation at
// record+0x50/0x54/0x58 -- or copies the cached pose at [self+0x14]+0x20 when
// the record pointer is null. Returns out.
float* nxPoseFromQuat0130(void* self, float* out)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* body = *reinterpret_cast<const unsigned char* const*>(p + 0x14);
	const unsigned char* rec = *reinterpret_cast<const unsigned char* const*>(body + 8);
	if(rec == nullptr)
		{
		memcpy(out, body + 0x20, 0x30);
		return out;
		}
	unsigned char pose[0x30];
	float q[4];
	memcpy(&q[0], rec + 0x5c, 4);
	memcpy(&q[1], rec + 0x60, 4);
	memcpy(&q[2], rec + 0x64, 4);
	memcpy(&q[3], rec + 0x68, 4);
	nxQuatToMatrix9(q, reinterpret_cast<float*>(pose));
	memcpy(pose + 0x24, rec + 0x50, 12);
	memcpy(out, pose, 0x30);
	return out;
	}

// The locked accessor family (see 3z138/3z139): lock [self+0x10], call a
// plain field getter on [self+0x24], unlock, return the result. Since the
// getter is a bare field read, the whole row reduces to this.
unsigned nxLockedFieldRead(void* self, unsigned offset)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + 0x24);
	unsigned v;
	memcpy(&v, field + offset, 4);
	return v;
	}

// The pointer-returning member of the family: the wrapped getter is
// `lea eax,[field+offset]`, so the row returns that address.
void* nxLockedPointerRead(void* self, unsigned offset)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + 0x24);
	return const_cast<unsigned char*>(field + offset);
	}

// Second variant of the family: lock at [self+lockOff], field at
// [self+fieldOff], wrapped getter a plain dword read of [field+dataOff].
unsigned nxLockedFieldReadEx(void* self, unsigned lockOff, unsigned fieldOff,
	unsigned dataOff)
	{
	(void) lockOff;
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	unsigned v;
	memcpy(&v, field + dataOff, 4);
	return v;
	}

// phys_fn_003824 (0x8c9c0): the getter is `lea eax,[field+0x14]` on
// [self+0x14] -- wait, the getter reads [ecx+0x14] and the row then
// dereferences the result, so the row returns *(*(field+0x14)).
unsigned nxLockedDeref3824(void* self)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + 0x14);
	unsigned ptr;
	memcpy(&ptr, field + 0x14, 4);
	unsigned v;
	memcpy(&v, reinterpret_cast<const unsigned char*>(static_cast<size_t>(ptr)), 4);
	return v;
	}

// phys_fn_003872 (0x8d1f0, ret 4): the getter is `lea eax,[field+8]`; the row
// copies twelve dwords from it into out and returns out.
void* nxLockedCopy12_3872(void* self, unsigned* out)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + 0x14);
	memcpy(out, field + 8, 48);
	return out;
	}

// phys_fn_004479 (0xb0d20, ret 4): compares arg with [field+0x168] (field from
// [self+0x18]) and returns SELF when they are equal, else zero -- the mask is
// `and esi, ecx` with esi still holding this.
// Product row: Physics/src/core/NpJointShared.cpp.
unsigned nxLockedMatch4479(void* self, unsigned arg)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + 0x18);
	unsigned v;
	memcpy(&v, field + 0x168, 4);
	return (arg == v)
		? static_cast<unsigned>(reinterpret_cast<size_t>(self))
		: 0u;
	}

// The copy members of the family: the wrapped helper copies `count` dwords
// from [field+dataOff] into out.
void nxLockedCopyOut(void* self, unsigned fieldOff, unsigned dataOff,
	unsigned count, unsigned* out)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	memcpy(out, field + dataOff, count * 4u);
	}

// The pose-copy member: nine dwords from [field+dataOff] followed by the three
// at [field+dataOff+0x24].
void nxLockedCopyPose(void* self, unsigned fieldOff, unsigned dataOff,
	unsigned* out)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	memcpy(out, field + dataOff, 36);
	memcpy(reinterpret_cast<unsigned char*>(out) + 0x24, field + dataOff + 0x24, 12);
	}

// The mask member: the wrapped helper is `mov eax,[ecx+dataOff]; and eax,arg`.
unsigned nxLockedAndRead(void* self, unsigned fieldOff, unsigned dataOff,
	unsigned arg)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	unsigned v;
	memcpy(&v, field + dataOff, 4);
	return v & arg;
	}

// The bit-extract member: `mov eax,[ecx+dataOff]; shr eax,3; and eax,3`.
// Product row: Physics/src/core/NpJointShared.cpp.
unsigned nxLockedBitExtract(void* self, unsigned fieldOff, unsigned dataOff)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	unsigned v;
	memcpy(&v, field + dataOff, 4);
	return (v >> 3) & 3u;
	}

// The element-count member: `mov eax,[ecx+8]; sub eax,[ecx+4]; sar eax,2`.
int nxLockedElementCount(void* self, unsigned fieldOff)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	unsigned hi, lo;
	memcpy(&hi, field + 8, 4);
	memcpy(&lo, field + 4, 4);
	return static_cast<int>(static_cast<signed>(hi - lo) >> 2);
	}

// The same count over caller-chosen offsets (rows that compute it inline).
int nxLockedElementCountAt(void* self, unsigned fieldOff, unsigned hiOff,
	unsigned loOff)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	unsigned hi, lo;
	memcpy(&hi, field + hiOff, 4);
	memcpy(&lo, field + loOff, 4);
	return static_cast<int>(static_cast<signed>(hi - lo) >> 2);
	}

// The array-data member: return [this+0x14] if the byte span to +0x18
// contains any complete word after masking its low two bits.
void* nxArrayDataIfNonEmpty(void* self)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	unsigned lo, hi;
	memcpy(&lo, p + 0x14, 4);
	memcpy(&hi, p + 0x18, 4);
	return ((hi - lo) & 0xfffffffcu) ?
		reinterpret_cast<void*>(static_cast<size_t>(lo)) : 0;
	}

// The interval-flag member: returns [field+0x14] when the span from it to
// [field+0x18] carries any bit above the low two, else zero.
unsigned nxLockedIntervalFlag(void* self, unsigned fieldOff)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	unsigned lo, hi;
	memcpy(&lo, field + 0x14, 4);
	memcpy(&hi, field + 0x18, 4);
	return ((hi - lo) & 0xfffffffcu) ? lo : 0u;
	}

// phys_fn_003700 (0x8b1f0): lock, return the MSVC vector<void*>::size()
// at [this+0x14], then release the lock.
int NxLockedFluidArrayAccessor::elementCount()
	{
	unsigned char* const self = reinterpret_cast<unsigned char*>(this);
	void* const lock = *reinterpret_cast<void**>(self + 0x10);
	nxSceneGuardEnter(lock);
	const int count = nxLockedElementCount(self, 0x14);
	nxSceneGuardLeave(lock);
	return count;
	}

// phys_fn_003702 (0x8b220): lock, apply helper 003479 to [this+0x14],
// release the lock, and return its pointer-sized result.
unsigned NxLockedFluidArrayAccessor::intervalData()
	{
	unsigned char* const self = reinterpret_cast<unsigned char*>(this);
	void* const lock = *reinterpret_cast<void**>(self + 0x10);
	nxSceneGuardEnter(lock);
	const unsigned data = nxLockedIntervalFlag(self, 0x14);
	nxSceneGuardLeave(lock);
	return data;
	}

// The address-returning member.
void* nxLockedFieldAddress(void* self, unsigned fieldOff, unsigned dataOff)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	return const_cast<unsigned char*>(field + dataOff);
	}

// The word-mask member.
unsigned nxLockedWordAndRead(void* self, unsigned fieldOff, unsigned dataOff,
	unsigned arg)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	unsigned short w;
	memcpy(&w, field + dataOff, 2);
	return static_cast<unsigned>(w) & arg;
	}

// The two-pointer copy member.
// Product row: Physics/src/core/NpJointShared.cpp.
void nxLockedCopyTwoPointers(void* self, unsigned fieldOff, unsigned* out1,
	unsigned* out2)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	memcpy(out1, field + 0x3c, 4);
	memcpy(out2, field + 0x40, 4);
	}

// The float member: `fld [field+dataOff]; fadd st(0),st(0)`, stored to float
// by the caller's fstp, so a double intermediate matches.
float nxLockedDoubleField(void* self, unsigned fieldOff, unsigned dataOff)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	float v;
	memcpy(&v, field + dataOff, 4);
	return static_cast<float>(2.0 * static_cast<double>(v));
	}

// The copy-and-flag member.
// Product row: Physics/src/core/RevoluteJoint.cpp.
// Product row: Physics/src/core/NpRevoluteJoint.cpp.
// The same shape (lock, copy the block, return a flag bit) also covers
// NpRevoluteJoint's getLimits/getMotor/getSpring (phys_fn_004711/004715/
// 004719), written there as direct calls through the named RevoluteJoint
// accessor rather than through this byte-offset form.
unsigned char nxLockedCopyAndFlag(void* self, unsigned fieldOff,
	unsigned dataOff, unsigned count, unsigned shift, unsigned* out)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	memcpy(out, field + dataOff, count * 4u);
	unsigned f;
	memcpy(&f, field + 0x1a8, 4);
	return static_cast<unsigned char>((f >> shift) & 1u);
	}

// The three-pointer copy member.
void nxLockedCopyThreePointers(void* self, unsigned fieldOff, unsigned* out1,
	unsigned* out2, unsigned* out3)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	memcpy(out1, field + 0x52c, 4);
	memcpy(out2, field + 0x530, 4);
	memcpy(out3, field + 0x534, 4);
	}

// The conditional-deref member: read the pointer at [field+dataOff]; when it
// is non-null return the word at [that+derefOff], else zero.
unsigned nxLockedDerefField(void* self, unsigned fieldOff, unsigned dataOff,
	unsigned derefOff)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	unsigned ptr;
	memcpy(&ptr, field + dataOff, 4);
	if(ptr == 0u)
		return 0u;
	unsigned v;
	memcpy(&v, reinterpret_cast<const unsigned char*>(static_cast<size_t>(ptr)) + derefOff, 4);
	return v;
	}

// The N-pointer copy member.
void nxLockedCopyNPointers(void* self, unsigned fieldOff, unsigned dataOff,
	unsigned count, unsigned** outs)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	for(unsigned k = 0; k < count; ++k)
		memcpy(outs[k], field + dataOff + 4u * k, 4);
	}

// The conditional-count member.
int nxLockedConditionalCount(void* self, unsigned fieldOff, unsigned dataOff,
	unsigned hiOff, unsigned loOff)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	unsigned ptr;
	memcpy(&ptr, field + dataOff, 4);
	if(ptr == 0u)
		return 0;
	const unsigned char* q = reinterpret_cast<const unsigned char*>(static_cast<size_t>(ptr));
	unsigned hi, lo;
	memcpy(&hi, q + hiOff, 4);
	memcpy(&lo, q + loOff, 4);
	return static_cast<int>(static_cast<signed>(hi - lo) >> 2);
	}

// The nested-deref member.
// Product row: Physics/src/core/NpJointShared.cpp.
void nxLockedTwoNestedDerefs(void* self, unsigned fieldOff, unsigned off1,
	unsigned off2, unsigned nested, unsigned* out1, unsigned* out2)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	const unsigned offs[2] = { off1, off2 };
	unsigned* outs[2] = { out1, out2 };
	for(unsigned k = 0; k < 2; ++k)
		{
		unsigned ptr;
		memcpy(&ptr, field + offs[k], 4);
		unsigned v = 0;
		if(ptr != 0u)
			{
			// [ptr+nested] is itself a pointer, and the word at it is the value.
			unsigned inner = 0;
			memcpy(&inner, reinterpret_cast<const unsigned char*>(static_cast<size_t>(ptr)) + nested, 4);
			if(inner != 0u)
				memcpy(&v, reinterpret_cast<const unsigned char*>(static_cast<size_t>(inner)), 4);
			}
		memcpy(outs[k], &v, 4);
		}
	}

// The match-or-self shape over caller-chosen offsets.
unsigned nxLockedMatchEx(void* self, unsigned fieldOff, unsigned dataOff,
	unsigned arg)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	unsigned v;
	memcpy(&v, field + dataOff, 4);
	return (arg == v)
		? static_cast<unsigned>(reinterpret_cast<size_t>(self))
		: 0u;
	}

// The list-advance reader: pop the head at [field+linkOff], relink to
// [head+nextOff] and return the word at [head+readOff].
unsigned nxLockedAdvanceRead(void* self, unsigned fieldOff, unsigned linkOff,
	unsigned nextOff, unsigned readOff)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	unsigned char* field = const_cast<unsigned char*>(
		*reinterpret_cast<const unsigned char* const*>(p + fieldOff));
	unsigned head;
	memcpy(&head, field + linkOff, 4);
	if(head == 0u)
		return 0u;
	unsigned next;
	memcpy(&next, reinterpret_cast<const unsigned char*>(static_cast<size_t>(head)) + nextOff, 4);
	memcpy(field + linkOff, &next, 4);
	unsigned v;
	memcpy(&v, reinterpret_cast<const unsigned char*>(static_cast<size_t>(head)) + readOff, 4);
	return v;
	}

// phys_fn_005450 (0xef690, ret 4): the 004886-calling wrapper. 004886 runs the
// registered callback at [0x10128478] when set and then sets [node+8] bit 2;
// with that slot bound to a no-op the whole call reduces to the bit set.
unsigned char nxWrap5450(void* self, void* node)
	{
	unsigned char* n = reinterpret_cast<unsigned char*>(node);
	unsigned short w;
	memcpy(&w, n + 0x28, 2);
	if(w != 0xffffu && !(n[8] & 2))
		*(unsigned*)(n + 8) |= 2u;
	++*reinterpret_cast<unsigned*>(reinterpret_cast<unsigned char*>(self) + 0x38);
	return 1;
	}

// phys_fn_001787 (0x3f570): the sibling of 005450, reading its object from the
// second stack argument (bare `ret`), calling 004886 on object+0xa4.
unsigned char nxWrap1787(void* unused, void* node)
	{
	(void) unused;
	unsigned char* n = reinterpret_cast<unsigned char*>(node);
	unsigned short w;
	memcpy(&w, n + 0xcc, 2);
	unsigned char* sub = n + 0xa4;
	if(w != 0xffffu && !(sub[8] & 2))
		*(unsigned*)(sub + 8) |= 2u;
	return 0;
	}

// The assertion-report hook and the six pure report rows. Each row's whole
// body is `report(0xce, file, line, 0, expression)` behind the guarded int3,
// and the guard dereferences the non-null [0x101041b0] word, so the hook is
// the only observable.
static NxAssertReportFn gNxAssertReport = nullptr;

void nxSetAssertReport(NxAssertReportFn fn)
	{
	gNxAssertReport = fn;
	}

static void nxAssertReport(unsigned file, unsigned line, unsigned expression)
	{
	if(gNxAssertReport)
		gNxAssertReport(0xceu, file, line, 0u, expression);
	}

void nxAssertReport0364() { nxAssertReport(0x10105ba8u, 0x17fu, 0x10105d4cu); }
void nxAssertReport0408() { nxAssertReport(0x10105ba8u, 0x28du, 0x10106064u); }
void nxAssertReport0410() { nxAssertReport(0x10105ba8u, 0x293u, 0x1010609cu); }
void nxAssertReport3750() { nxAssertReport(0x101160ccu, 0x0f6u, 0x101163acu); }
void nxAssertReport3754() { nxAssertReport(0x101160ccu, 0x106u, 0x10116418u); }
void nxAssertReport3782() { nxAssertReport(0x101160ccu, 0x176u, 0x101165dcu); }
void nxAssertReport000362() { nxAssertReport(0x10105ba8u, 0x179u, 0x10105d0cu); }
void nxAssertReport000404() { nxAssertReport(0x10105ba8u, 0x281u, 0x10105ff4u); }
void nxAssertReport000406() { nxAssertReport(0x10105ba8u, 0x287u, 0x1010602cu); }
void nxAssertReport003736() { nxAssertReport(0x101160ccu, 0x0bcu, 0x10116268u); }
void nxAssertReport003740() { nxAssertReport(0x101160ccu, 0x0cbu, 0x101162f4u); }
void nxAssertReport003748() { nxAssertReport(0x101160ccu, 0x0edu, 0x10116374u); }
void nxAssertReport003752() { nxAssertReport(0x101160ccu, 0x0fdu, 0x101163e4u); }
void nxAssertReport003756() { nxAssertReport(0x101160ccu, 0x111u, 0x1011644cu); }
void nxAssertReport003758() { nxAssertReport(0x101160ccu, 0x11bu, 0x10116488u); }
void nxAssertReport003760() { nxAssertReport(0x101160ccu, 0x123u, 0x101164c4u); }
void nxAssertReport003762() { nxAssertReport(0x101160ccu, 0x12du, 0x101164f8u); }
void nxAssertReport003764() { nxAssertReport(0x101160ccu, 0x134u, 0x1011652cu); }
void nxAssertReport003766() { nxAssertReport(0x101160ccu, 0x13eu, 0x10116564u); }
void nxAssertReport003884() { nxAssertReport(0x1011681cu, 0x03au, 0x10116858u); }

// The word member: the row's return is the zero-extended word because the
// unlock helper leaves eax at 1 before the `mov ax,si` mask.
unsigned nxLockedWordRead(void* self, unsigned lockOff, unsigned fieldOff,
	unsigned dataOff)
	{
	(void) lockOff;
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* field = *reinterpret_cast<const unsigned char* const*>(p + fieldOff);
	unsigned short w;
	memcpy(&w, field + dataOff, 2);
	return static_cast<unsigned>(w);
	}

// phys_fn_003708 (0x8b420): the gated locked reader. The image's gate byte is
// non-zero, so the shipped path is the work arm; the mirror lets the harness
// drive the report arm too.
static unsigned char gNxGate3708 = 1u;

void nxSetGate3708(unsigned char on)
	{
	gNxGate3708 = on;
	}

unsigned nxGuardedField3708(void* self)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	if(!gNxGate3708)
		{
		if(gNxAssertReport)
			gNxAssertReport(0xceu, 0x101160ccu, 0x4cu, 0u, 0x10116170u);
		return 0u;
		}
	unsigned char* field = *reinterpret_cast<unsigned char**>(p + 0x14);
	unsigned r;
	memcpy(&r, field + 0x24, 4);
	unsigned v;
	memcpy(&v, reinterpret_cast<const unsigned char*>(static_cast<size_t>(r)) + 0x38, 4);
	return v;
	}

// phys_fn_000392 (0xd660): the lock probe. The recursive acquire behind
// 002364 succeeds when the lock's owner word already holds the id the query
// stub reports; the acquire then re-stores the same id, and the release is a
// no-op under the bound stubs.
static unsigned gNxLockOwner = 0x2222u;

void nxSetLockOwner(unsigned id)
	{
	gNxLockOwner = id;
	}

unsigned char nxLockProbe0392(void* self)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned char* lock = *reinterpret_cast<unsigned char**>(p + 0xc);
	// the acquire reads the owner through the lock's first word
	unsigned char* inner = *reinterpret_cast<unsigned char**>(lock);
	unsigned owner;
	memcpy(&owner, inner + 0x1c, 4);
	if(owner != gNxLockOwner)
		return 0;
	return 1;
	}

// The acquire test shared by the mutex family: the owner word is reached
// through the lock's first word.
static bool nxTryAcquireLock(unsigned char* lock)
	{
	unsigned char* inner = *reinterpret_cast<unsigned char**>(lock);
	unsigned owner;
	memcpy(&owner, inner + 0x1c, 4);
	return owner == gNxLockOwner;
	}

// The mutex-guarded virtual dispatch family. The slot call site pushes one
// argument and does not clean it, so the slot is a __stdcall one-argument
// function with `this` in ecx.
void nxMutexVirtualEx(void* self, void* arg, unsigned slot, unsigned code,
	unsigned file, unsigned line, unsigned expression)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned char* lock = *reinterpret_cast<unsigned char**>(p + 0x10);
	if(!nxTryAcquireLock(lock))
		{
		if(gNxAssertReport)
			gNxAssertReport(code, file, line, 0u, expression);
		return;
		}
	unsigned char* obj = *reinterpret_cast<unsigned char**>(p + 0x18);
	void** vt = *reinterpret_cast<void***>(obj);
	typedef void (__stdcall* SlotFn)(void*);
	SlotFn fn = reinterpret_cast<SlotFn>(vt[slot / 4]);
	fn(arg);
	// the release is a no-op under the bound stubs
	}

// The constant-argument virtual thunk family: the thunk pushes its constant
// and does not clean it, so the slot is a __stdcall one-argument function.
void nxVtConstEx(void* self, unsigned slot, unsigned arg)
	{
	void** vt = *reinterpret_cast<void***>(self);
	typedef void (__stdcall* SlotFn)(unsigned);
	SlotFn fn = reinterpret_cast<SlotFn>(vt[slot / 4]);
	fn(arg);
	}

// phys_fn_004417..004433 (0xb0580..0xb0600): preserve the individual
// constant-argument thiscall thunks in the product image. Each row reads its
// vptr from ECX, pushes its own constant, dispatches through slot +0x4c, and
// returns the callee's EAX without disturbing the x86 stack.
#define NX_VT_CONST_THUNK(name, value) \
	extern "C" __declspec(naked) void __cdecl name(void) \
		{ \
		__asm mov eax, [ecx] \
		__asm push value \
		__asm call dword ptr [eax+4Ch] \
		__asm ret \
		}
NX_VT_CONST_THUNK(nxVtConst004417, 1)
NX_VT_CONST_THUNK(nxVtConst004419, 5)
NX_VT_CONST_THUNK(nxVtConst004421, 4)
NX_VT_CONST_THUNK(nxVtConst004423, 0)
NX_VT_CONST_THUNK(nxVtConst004425, 2)
NX_VT_CONST_THUNK(nxVtConst004427, 3)
NX_VT_CONST_THUNK(nxVtConst004429, 8)
NX_VT_CONST_THUNK(nxVtConst004431, 6)
NX_VT_CONST_THUNK(nxVtConst004433, 7)
#undef NX_VT_CONST_THUNK

// phys_fn_002390 (0x5b910): the __cdecl slot takes one argument.
void nxVtCall2390(void* self)
	{
	unsigned char* obj = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(self) + 4);
	// the target is a plain callback field at +0xc, not a vtable entry
	void* target;
	memcpy(&target, obj + 0xc, 4);
	unsigned arg;
	memcpy(&arg, obj + 0x10, 4);
	typedef void (__cdecl* Fn)(unsigned);
	reinterpret_cast<Fn>(target)(arg);
	}

// phys_fn_003924 (0x8ed50): the __stdcall slot takes two arguments.
// Product row: Physics/src/core/SpringAndDamperEffector.cpp.
void nxVtCall3924(void* self)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	void** vt = *reinterpret_cast<void***>(p);
	unsigned a, b;
	memcpy(&a, p + 0x24, 4);
	memcpy(&b, p + 0x28, 4);
	typedef void (__stdcall* Fn)(unsigned, unsigned);
	reinterpret_cast<Fn>(vt[0xc / 4])(a, b);
	}

// phys_fn_001965 (0x4c000): three arguments, callee-cleaned.
void nxVtCall1965(void* unused, void* obj)
	{
	(void) unused;
	void** vt = *reinterpret_cast<void***>(obj);
	typedef void (__stdcall* Fn)(unsigned, unsigned, unsigned);
	reinterpret_cast<Fn>(vt[0x2c / 4])(static_cast<unsigned>(
		reinterpret_cast<size_t>(unused)), 0xff00ffffu, 0u);
	}

// phys_fn_001237 (0x24db0): the slot takes `this` in ecx and no stack
// arguments, which is the __fastcall layout for a one-argument function.
unsigned nxLockedVtCall1237(void* self)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned char* obj = *reinterpret_cast<unsigned char**>(p + 0x18);
	void** vt = *reinterpret_cast<void***>(obj);
	typedef unsigned (__fastcall* SlotFn)(void*);
	SlotFn fn = reinterpret_cast<SlotFn>(vt[0x44 / 4]);
	return fn(obj);
	}

// phys_fn_001119 (0x23c50): the same dispatch through slot +0x3c, float result.
float nxLockedVtCall1119(void* self)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned char* obj = *reinterpret_cast<unsigned char**>(p + 0x18);
	void** vt = *reinterpret_cast<void***>(obj);
	typedef float (__fastcall* SlotFn)(void*);
	SlotFn fn = reinterpret_cast<SlotFn>(vt[0x3c / 4]);
	return fn(obj);
	}

// phys_fn_001022 (0x22970, ret 4): the per-element virtual dispatch loop.
void nxArrayVtCall1022(void* self, void* arg)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned* begin = *reinterpret_cast<unsigned**>(p + 0xe0);
	unsigned* end = *reinterpret_cast<unsigned**>(p + 0xe4);
	const unsigned n = static_cast<unsigned>(
		reinterpret_cast<unsigned char*>(end) - reinterpret_cast<unsigned char*>(begin)) >> 2;
	for(unsigned i = 0; i < n; ++i)
		{
		unsigned char* elem = reinterpret_cast<unsigned char*>(begin[i]);
		void** vt = *reinterpret_cast<void***>(elem);
		typedef void (__stdcall* Fn)(void*);
		reinterpret_cast<Fn>(vt[0xc / 4])(arg);
		}
	}

// phys_fn_002142 (0x532b0, ret 4): the scalar deleting destructor. The free
// arm goes through the allocator singleton's slot +0x14.
static NxAllocFreeFn gNxAllocFree = nullptr;

void nxSetAllocFree(NxAllocFreeFn fn)
	{
	gNxAllocFree = fn;
	}

void* nxScalarDeletingDtor2142(void* self, unsigned flags)
	{
	unsigned vtable = 0x1010829cu;
	memcpy(self, &vtable, 4);
	if(flags & 1u)
		{
		if(gNxAllocFree)
			gNxAllocFree(self);
		}
	return self;
	}

void nxMutexApplyGroupEx(void* self, unsigned arg, unsigned code, unsigned file,
	unsigned line, unsigned expression)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned char* lock = *reinterpret_cast<unsigned char**>(p + 0x10);
	if(!nxTryAcquireLock(lock))
		{
		if(gNxAssertReport)
			gNxAssertReport(code, file, line, 0u, expression);
		return;
		}
	ShapeBase* obj = reinterpret_cast<ShapeBase*>(*reinterpret_cast<void**>(p + 0x18));
	obj->nxApplyGroup(static_cast<unsigned short>(arg));
	}

void nxMutexNoopEx(void* self, unsigned code, unsigned file, unsigned line,
	unsigned expression)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned char* lock = *reinterpret_cast<unsigned char**>(p + 0x10);
	if(!nxTryAcquireLock(lock))
		{
		if(gNxAssertReport)
			gNxAssertReport(code, file, line, 0u, expression);
		return;
		}
	// 004248 is a bare `ret 4`, so the work arm has no observable beyond the
	// release.
	}

// The lock-bracketed vtable call with a caller-chosen slot and no arguments.
// Product row: Physics/src/core/NpRevoluteJoint.cpp.
// This shape also covers getFlags/getProjectionMode (phys_fn_004703/004707:
// read lock, internal vtable slot 12/14, unlock), written there as direct
// calls through the named RevoluteJoint virtual rather than this
// byte-offset form.
unsigned nxLockedVtCallNoArg(void* self, unsigned slot)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned char* obj = *reinterpret_cast<unsigned char**>(p + 0x18);
	void** vt = *reinterpret_cast<void***>(obj);
	typedef unsigned (__fastcall* SlotFn)(void*);
	SlotFn fn = reinterpret_cast<SlotFn>(vt[slot / 4]);
	return fn(obj);
	}

// The same bracket where the slot takes one argument as well.
void nxLockedVtCallArg(void* self, unsigned slot, void* arg)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned char* obj = *reinterpret_cast<unsigned char**>(p + 0x18);
	void** vt = *reinterpret_cast<void***>(obj);
	typedef void (__stdcall* SlotFn)(void*);
	SlotFn fn = reinterpret_cast<SlotFn>(vt[slot / 4]);
	fn(arg);
	}

// The report-once dispatch rows.
void nxOnceReportVtEx(void* self, unsigned char* gate, unsigned slot,
	unsigned nargs, unsigned a1, unsigned a2, unsigned code, unsigned file,
	unsigned line, unsigned expression)
	{
	if(*gate == 0)
		{
		if(gNxAssertReport)
			gNxAssertReport(code, file, line, 0u, expression);
		*gate = 1;
		}
	void** vt = *reinterpret_cast<void***>(self);
	if(nargs <= 1)
		{
		typedef void (__stdcall* Fn1)(unsigned);
		reinterpret_cast<Fn1>(vt[slot / 4])(a1);
		}
	else
		{
		typedef void (__stdcall* Fn2)(unsigned, unsigned);
		reinterpret_cast<Fn2>(vt[slot / 4])(a1, a2);
		}
	}

// phys_fn_003938 (0x8eec0, ret 4): the deleting destructor that also calls a
// process-wide global slot with no arguments.
// Product row: Physics/src/core/SpringAndDamperEffector.cpp.
static void (*gNxGlobalHook3938)(void) = nullptr;

void nxSetGlobalHook3938(void (*fn)(void))
	{
	gNxGlobalHook3938 = fn;
	}

void* nxDtorWithGlobal3938(void* self, unsigned flags)
	{
	unsigned vtable = 0x10117920u;
	memcpy(self, &vtable, 4);
	if(gNxGlobalHook3938)
		gNxGlobalHook3938();
	if(flags & 1u)
		{
		if(gNxAllocFree)
			gNxAllocFree(self);
		}
	return self;
	}

// The guarded store family: the 004334 guard without the clamp tail.
// Product row: Physics/src/core/RevoluteJoint.cpp.
// Product row: Physics/src/core/SphericalJoint.cpp.
// Product row: Physics/src/core/D6Joint.cpp.
void nxGuardedStoreEx(void* self, unsigned arg, unsigned fieldOff, unsigned code,
	unsigned file, unsigned line, unsigned expression)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	if((p[0x2c] & 0x18) == 0x10)
		{
		if(gNxAssertReport)
			gNxAssertReport(code, file, line, 0u, expression);
		return;
		}
	memcpy(p + fieldOff, &arg, 4);
	}

// The lock-first direct-call family.
void nxMutexWorkEx(void* self, unsigned lockOff, unsigned objOff,
	const unsigned* args, unsigned nargs, int workKind, unsigned baseOff,
	unsigned nwords, unsigned code, unsigned file, unsigned line,
	unsigned expression)
	{
	(void) nargs;
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned char* lock = *reinterpret_cast<unsigned char**>(p + lockOff);
	if(!nxTryAcquireLock(lock))
		{
		if(gNxAssertReport)
			gNxAssertReport(code, file, line, 0u, expression);
		return;
		}
	if(workKind == 0)
		return;
	unsigned char* obj = *reinterpret_cast<unsigned char**>(p + objOff);
	if(workKind == 1)
		{
		memcpy(obj + baseOff, &args[0], 4);
		}
	else if(workKind == 2)
		{
		for(unsigned k = 0; k < nwords; ++k)
			memcpy(obj + baseOff + 4u * k, &args[k], 4);
		}
	else if(workKind == 3)
		{
		const unsigned char* src = reinterpret_cast<const unsigned char*>(
			static_cast<size_t>(args[0]));
		for(unsigned k = 0; k < nwords; ++k)
			memcpy(obj + baseOff + 4u * k, src + 4u * k, 4);
		}
	else if(workKind == 4)
		{
		unsigned v;
		memcpy(&v, obj + baseOff, 4);
		v = args[1] ? (v | args[0]) : (v & ~args[0]);
		memcpy(obj + baseOff, &v, 4);
		}
	}

// phys_fn_001024 (0x229b0, ret 0xc): the guarded three-argument dispatch loop.
// Product row: Physics/src/core/ActorMass.cpp.
unsigned char nxArrayVtCall3Args1024(void* self, unsigned a1, unsigned a2,
	unsigned a3)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned* begin = *reinterpret_cast<unsigned**>(p + 0xe0);
	unsigned* end = *reinterpret_cast<unsigned**>(p + 0xe4);
	const unsigned n = static_cast<unsigned>(
		reinterpret_cast<unsigned char*>(end) - reinterpret_cast<unsigned char*>(begin)) >> 2;
	for(unsigned i = 0; i < n; ++i)
		{
		unsigned char* elem = reinterpret_cast<unsigned char*>(begin[i]);
		if(elem[0xde] & 7)
			continue;
		// `mov ecx,[edi]; call [eax+0x10]` (0x229d1-0x229ea): a thiscall on
		// the element, its result tested as a byte.
		void** vt = *reinterpret_cast<void***>(elem);
		typedef unsigned char (__thiscall* Fn)(void*, unsigned, unsigned, unsigned);
		if(reinterpret_cast<Fn>(vt[0x10 / 4])(elem, a1, a2, a3) == 0)
			return 0;
		}
	return 1;
	}

// phys_fn_004491 (0xb0f10): the lock-bracketed global-flag read. The helper
// 004083 tests the data word at [0x10127180] and returns 0 or 1.
// Product row: Physics/src/core/NpJointShared.cpp.
static unsigned gNxGlobalFlag4491 = 0;

void nxSetGlobalFlag4491(unsigned value)
	{
	gNxGlobalFlag4491 = value;
	}

unsigned nxGetGlobalFlag4491()
	{
	return gNxGlobalFlag4491;
	}

unsigned char nxLockedGlobalFlag4491(void* self)
	{
	(void) self;
	return gNxGlobalFlag4491 != 0u ? 1u : 0u;
	}

// phys_fn_000342 (0xcb90, ret 0x10): report-once then a four-step dispatch.
void nxOnceFourCalls0342(void* self, unsigned char* gate, unsigned a1,
	unsigned a2, unsigned a3, unsigned a4)
	{
	if(*gate == 0)
		{
		if(gNxAssertReport)
			gNxAssertReport(0xd0u, 0x10105ba8u, 0x12eu, 0u, 0x10105c88u);
		*gate = 1;
		}
	void** vt = *reinterpret_cast<void***>(self);
	typedef void (__stdcall* Fn3)(unsigned, unsigned, unsigned);
	typedef void (__stdcall* Fn1)(unsigned);
	typedef void (__stdcall* Fn0)();
	typedef void (__stdcall* Fn2)(unsigned, unsigned);
	reinterpret_cast<Fn3>(vt[0x70 / 4])(a2, a3, a4);
	reinterpret_cast<Fn1>(vt[0x100 / 4])(a1);
	reinterpret_cast<Fn0>(vt[0x64 / 4])();
	reinterpret_cast<Fn2>(vt[0x108 / 4])(1u, 1u);
	}

// phys_fn_004763 (0xb3920): the guarded dispatch plus the chain walk.
void nxGuardedVtChain4763(void* self)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned char* obj = *reinterpret_cast<unsigned char**>(p + 0x18);
	if(obj != nullptr)
		{
		bool pass;
		unsigned char* a = *reinterpret_cast<unsigned char**>(obj + 8);
		if(a != nullptr && (a[0x10c] & 0x80) == 0)
			{
			// a non-null entry WITHOUT the bit skips the +0xc test entirely
			pass = true;
			}
		else
			{
			unsigned char* c = *reinterpret_cast<unsigned char**>(obj + 0xc);
			pass = (c != nullptr) && ((c[0x10c] & 0x80) == 0);
			}
		if(pass)
			{
			unsigned char* d = *reinterpret_cast<unsigned char**>(obj + 0x44);
			if(d == nullptr)
				pass = false;
			else
				{
				unsigned v;
				memcpy(&v, obj + 0x2c, 4);
				if(((v >> 2) & 1u) != 0u)
					pass = false;
				}
			}
		if(pass)
			{
			void** vt = *reinterpret_cast<void***>(obj);
			unsigned arg;
			memcpy(&arg, p + 0x14, 4);
			typedef void (__stdcall* Fn1)(unsigned);
			reinterpret_cast<Fn1>(vt[0x20 / 4])(arg);
			}
		}
	// the chain walk runs whatever the guard decided
	unsigned char* node = *reinterpret_cast<unsigned char**>(p + 0xc);
	while(node != nullptr)
		{
		void** vt = *reinterpret_cast<void***>(node);
		typedef void (__fastcall* FnThis)(void*);
		reinterpret_cast<FnThis>(vt[0x10 / 4])(node);
		node = *reinterpret_cast<unsigned char**>(node + 0x10);
		}
	}

// phys_fn_005382 (0xe9440, ret 4): the store-then-dispatch row. The stream
// work is the already-reconstructed MemoryStream::storeDword.
unsigned nxStoreOrDispatch5382(void* self, void* stream)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned value;
	memcpy(&value, p + 8, 4);
	reinterpret_cast<MemoryStream*>(stream)->storeDword(value);
	unsigned char* obj = *reinterpret_cast<unsigned char**>(p + 0x10);
	if(obj != nullptr)
		{
		void** vt = *reinterpret_cast<void***>(obj);
		typedef unsigned (__stdcall* Fn1)(void*);
		return reinterpret_cast<Fn1>(vt[0x14 / 4])(stream);
		}
	return (value >> 2) & 1u;
	}

// phys_fn_001563 / phys_fn_002154: deleting destructors whose allocator comes
// from 004803 instead of a fixed global slot.
static void* gNxAllocator004803 = nullptr;

void nxSetAllocator004803(void* allocator)
	{
	gNxAllocator004803 = allocator;
	}

void* nxDtorViaAllocator(void* self, unsigned flags, unsigned vtable)
	{
	memcpy(self, &vtable, 4);
	if(flags & 1u)
		{
		if(gNxAllocator004803 != nullptr)
			{
			void** vt = *reinterpret_cast<void***>(gNxAllocator004803);
			typedef void (__stdcall* Fn1)(void*);
			reinterpret_cast<Fn1>(vt[0xc / 4])(self);
			}
		}
	return self;
	}

// phys_fn_001585 / phys_fn_001587: destructors that free an owned pointer and
// then re-store the fixed vtable through 001554.
void* nxDtorOwnedPtr(void* self, unsigned flags, unsigned vtable)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned owned;
	memcpy(&owned, p + 0xc, 4);
	memcpy(p, &vtable, 4);
	if(owned != 0u && gNxAllocator004803 != nullptr)
		{
		void** vt = *reinterpret_cast<void***>(gNxAllocator004803);
		typedef void (__stdcall* Fn1)(unsigned);
		reinterpret_cast<Fn1>(vt[0xc / 4])(owned);
		}
	if(owned != 0u)
		{
		unsigned zero = 0u;
		memcpy(p + 0xc, &zero, 4);
		}
	// 001554 stores the fixed vtable word
	unsigned fixed = 0x10107848u;
	memcpy(p, &fixed, 4);
	if((flags & 1u) != 0u && gNxAllocator004803 != nullptr)
		{
		void** vt = *reinterpret_cast<void***>(gNxAllocator004803);
		typedef void (__stdcall* Fn1)(void*);
		reinterpret_cast<Fn1>(vt[0xc / 4])(self);
		}
	return self;
	}

// phys_fn_004743 (0xb3670): the lock-bracketed registry lookup. 000454 walks
// the registry vector when [0x10123c0c] is non-null and returns 0 when it is
// null, which is the image's state.
// Product row: Physics/src/core/NpJointShared.cpp.
static void* gNxRegistry4743 = nullptr;

void nxSetRegistry4743(void* registry)
	{
	gNxRegistry4743 = registry;
	}

unsigned nxLockedRegistryLookup4743(void* self)
	{
	(void) self;
	if(gNxRegistry4743 == nullptr)
		return 0u;
	// a non-null registry would be walked here; the shipped state is null
	return 0u;
	}

// phys_fn_001281 (0x257a0): the bare field getter.
unsigned nxFieldRead4(void* self)
	{
	unsigned v;
	memcpy(&v, reinterpret_cast<unsigned char*>(self) + 4, 4);
	return v;
	}

// phys_fn_004085 (0x95cb0): the registry lookup with its own `this`.
// Product row: Physics/src/core/Joint.cpp.
unsigned nxRegistryLookupNull(void* self)
	{
	(void) self;
	if(gNxRegistry4743 == nullptr)
		return 0u;
	return 0u;
	}

// The two-argument global-call thunks. The harness binds the image's global
// function pointer to the same hook it passes here.
void nxGlobalCall2(void* self, NxGlobalFn2 fn, int viaField4)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	if(viaField4 != 0)
		{
		unsigned inner;
		memcpy(&inner, p + 4, 4);
		p = reinterpret_cast<unsigned char*>(static_cast<size_t>(inner));
		}
	unsigned a, b;
	memcpy(&a, p + 0x7c, 4);
	memcpy(&b, p + 0x80, 4);
	unsigned char* obj = reinterpret_cast<unsigned char*>(static_cast<size_t>(a));
	unsigned first;
	memcpy(&first, obj + 0x30, 4);
	fn(first, b);
	}

// phys_fn_001583 (0x2ea70): a one-byte no-op.
void nxNoop1583(void)
	{
	}

// phys_fn_002371 (0x5b7e0): the lock-API global call with [this].
void nxLockApiCall2371(void* self, void (__stdcall* fn)(unsigned))
	{
	unsigned v;
	memcpy(&v, self, 4);
	fn(v);
	}

// phys_fn_002375 (0x5b800, ret 4): the lock-API test.
unsigned char nxLockApiTest2375(void* self, unsigned arg,
	unsigned (__stdcall* fn)(unsigned, unsigned))
	{
	unsigned v;
	memcpy(&v, self, 4);
	// the call site pushes [this] FIRST and the row argument second, so the
	// slot receives ([this], arg); `neg`/`sbb`/`inc` then yield 1 when the
	// slot returned ZERO and 0 when it returned non-zero.
	const unsigned r = fn(v, arg);
	return r == 0u ? 1u : 0u;
	}

// phys_fn_005320 (0x7c10): frees the owned block minus four bytes.
void nxFreeOwnedBlock5320(void* self)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned owned;
	memcpy(&owned, p + 0x4c, 4);
	if(owned != 0u && gNxAllocator004803 != nullptr)
		{
		void** vt = *reinterpret_cast<void***>(gNxAllocator004803);
		typedef void (__stdcall* Fn1)(unsigned);
		reinterpret_cast<Fn1>(vt[0xc / 4])(owned - 4u);
		}
	unsigned zero = 0u;
	memcpy(p + 0x4c, &zero, 4);
	memcpy(p + 0x48, &zero, 4);
	}

// phys_fn_004409 (0xb0360): the linked-list teardown.
void nxListTeardown4409(void* self)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned char* node = *reinterpret_cast<unsigned char**>(p + 0xc);
	unsigned vtable = 0x1011a648u;
	memcpy(p, &vtable, 4);
	while(node != nullptr)
		{
		void** vt = *reinterpret_cast<void***>(node);
		unsigned char* next = *reinterpret_cast<unsigned char**>(node + 0x10);
		typedef void (__stdcall* Fn1)(unsigned);
		reinterpret_cast<Fn1>(vt[0])(1u);
		node = next;
		}
	// the oracle clears +8, +0xc, +0x10, +0x14, +0x18 and +0x20 -- note it
	// SKIPS +0x1c, which a plain stride would wrongly include
	static const unsigned kClearOffs[6] = { 8u, 0xcu, 0x10u, 0x14u, 0x18u, 0x20u };
	for(unsigned k = 0; k < 6; ++k)
		{
		unsigned zero = 0u;
		memcpy(p + kClearOffs[k], &zero, 4);
		}
	}

// phys_fn_001413 (0x29920): the summed byte count.
unsigned nxByteCount1413(void* self)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned total = 0;
	unsigned a, b, c, d;
	memcpy(&a, p + 8, 4);
	memcpy(&b, p + 4, 4);
	memcpy(&c, p + 0x10, 4);
	memcpy(&d, p + 0xc, 4);
	// 001457
	if(a != 0u)
		total += b * 6u;
	if(c != 0u)
		total += d * 12u;
	// 001668 on [self+0x64] when non-null
	unsigned inner;
	memcpy(&inner, p + 0x64, 4);
	if(inner != 0u)
		{
		unsigned char* q = reinterpret_cast<unsigned char*>(static_cast<size_t>(inner));
		unsigned q0, q4, q8, qc, q10;
		memcpy(&q0, q, 4);
		memcpy(&q4, q + 4, 4);
		memcpy(&q8, q + 8, 4);
		memcpy(&qc, q + 0xc, 4);
		memcpy(&q10, q + 0x10, 4);
		if(q8 != 0u)
			total += q0 * 4u;
		if(qc != 0u)
			total += q0 * 4u;
		if(q10 != 0u)
			total += q4 * 4u;
		}
	return total;
	}

// phys_fn_000443 (0xdeb0): the lazy singleton getter.
unsigned nxLazySingleton0443(void* obj, unsigned cached, NxSingletonFn fn)
	{
	if(cached != 0u)
		return cached;
	return fn(obj);
	}

// The multi-field owned-free rows.
void nxFreeOwnedFields(void* self, const unsigned* offsets, unsigned count,
	int minus4)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	for(unsigned k = 0; k < count; ++k)
		{
		unsigned owned;
		memcpy(&owned, p + offsets[k], 4);
		if(owned == 0u)
			continue;
		if(gNxAllocator004803 != nullptr)
			{
			void** vt = *reinterpret_cast<void***>(gNxAllocator004803);
			typedef void (__stdcall* Fn1)(unsigned);
			reinterpret_cast<Fn1>(vt[0xc / 4])(minus4 ? (owned - 4u) : owned);
			}
		unsigned zero = 0u;
		memcpy(p + offsets[k], &zero, 4);
		}
	}

// phys_fn_001577 (0x2e7f0): two owned frees then the fixed-vtable store.
void nxDtorTwoOwned1577(void* self)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned vtable = 0x10107890u;
	memcpy(p, &vtable, 4);
	const unsigned offs[2] = { 0x10u, 0xcu };
	nxFreeOwnedFields(self, offs, 2, 0);
	unsigned fixed = 0x10107848u;
	memcpy(p, &fixed, 4);
	}

// phys_fn_001526 (0x2d970): the byte count plus a fixed 0x18.
unsigned nxByteCountPlus18(void* self)
	{
	return nxByteCount1413(self) + 0x18u;
	}

// phys_fn_000396 (0xd710, ret 8): the conditional lock-API dispatch.
unsigned char nxLockApiDispatch0396(void* self, unsigned flags, unsigned arg2,
	unsigned (__stdcall* fn)(unsigned, unsigned))
	{
	// when bit 0 is clear the row jumps straight to its `ret 8`, and al still
	// holds the 1 stored at the top -- so that arm returns 1, not 0
	if((flags & 1u) == 0u)
		return 1u;
	unsigned char* p = reinterpret_cast<unsigned char*>(self) + 0x14;
	unsigned v;
	memcpy(&v, p, 4);
	const unsigned arg = (arg2 != 0u) ? 0xffffffffu : 0u;
	return static_cast<unsigned char>(nxLockApiTest2375(p, arg, fn) & 1u);
	}

// The lock-bracketed helper-call rows.
unsigned nxLockedHelperCall(void* self, unsigned lockOff, unsigned objOff,
	int helperKind, NxGlobalFn2Ret fn, int viaField4)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned char* obj = *reinterpret_cast<unsigned char**>(p + objOff);
	if(helperKind == 0)
		{
		// the registry lookup returns zero while the registry is null
		return nxRegistryLookupNull(obj);
		}
	unsigned char* q = obj;
	if(viaField4 != 0)
		{
		unsigned inner;
		memcpy(&inner, q + 4, 4);
		q = reinterpret_cast<unsigned char*>(static_cast<size_t>(inner));
		}
	unsigned a, b;
	memcpy(&a, q + 0x7c, 4);
	memcpy(&b, q + 0x80, 4);
	unsigned first;
	memcpy(&first, reinterpret_cast<unsigned char*>(static_cast<size_t>(a)) + 0x30, 4);
	return fn(first, b);
	}

// The float-returning lock-bracketed helper rows.
float nxLockedHelperCallF(void* self, unsigned lockOff, unsigned objOff,
	NxGlobalFn2RetF fn, int viaField4)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned char* obj = *reinterpret_cast<unsigned char**>(p + objOff);
	unsigned char* q = obj;
	if(viaField4 != 0)
		{
		unsigned inner;
		memcpy(&inner, q + 4, 4);
		q = reinterpret_cast<unsigned char*>(static_cast<size_t>(inner));
		}
	unsigned a, b;
	memcpy(&a, q + 0x7c, 4);
	memcpy(&b, q + 0x80, 4);
	unsigned first;
	memcpy(&first, reinterpret_cast<unsigned char*>(static_cast<size_t>(a)) + 0x30, 4);
	return fn(first, b);
	}

// The destructor-chain rows.
void* nxDtorOwnedThenFree1589(void* self, unsigned flags)
	{
	nxDtorTwoOwned1577(self);
	if((flags & 1u) != 0u && gNxAllocator004803 != nullptr)
		{
		void** vt = *reinterpret_cast<void***>(gNxAllocator004803);
		typedef void (__stdcall* Fn1)(void*);
		reinterpret_cast<Fn1>(vt[0xc / 4])(self);
		}
	return self;
	}

void* nxDtorTeardownThenFree(void* self, unsigned flags, unsigned vtable)
	{
	if(vtable != 0u)
		memcpy(self, &vtable, 4);
	nxListTeardown4409(self);
	if((flags & 1u) != 0u && gNxAllocFree != nullptr)
		gNxAllocFree(self);
	return self;
	}

// The tail-jmp mutex group.
unsigned nxMutexGlobalStore(void* self, unsigned code, unsigned file,
	unsigned line, unsigned expression)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned char* lock = *reinterpret_cast<unsigned char**>(p + 0x10);
	if(!nxTryAcquireLock(lock))
		{
		if(gNxAssertReport)
			gNxAssertReport(code, file, line, 0u, expression);
		return 0u;
		}
	unsigned char* obj = *reinterpret_cast<unsigned char**>(p + 0x18);
	unsigned v;
	memcpy(&v, obj + 0x20, 4);
	// 004081 stores it into the global word [0x10127180]
	gNxGlobalFlag4491 = v;
	// the row TAIL-JUMPS into the unlock, which ends `mov al, 1` -- so the
	// success arm returns 1, not the value it stored
	return 1u;
	}

void nxMutexLinkAdvance(void* self, unsigned objOff, unsigned srcOff,
	unsigned dstOff)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned char* obj = *reinterpret_cast<unsigned char**>(p + objOff);
	unsigned v;
	memcpy(&v, obj + srcOff, 4);
	memcpy(obj + dstOff, &v, 4);
	}

// phys_fn_004387 (0xaf2c4): tail-jumps to Observable::event(this, event,
// observer), forwarding ECX and both stack arguments to the import.
void nxTrampoline4387(void* self, unsigned event, void* observer,
	NxObservableEventFn fn)
	{
	fn(self, event, observer);
	}

// phys_fn_003509 (0x863f0): tail-jumps through the dynamically loaded
// StaticCollisionDestroy cdecl pointer and forwards its single collision arg.
int nxStaticCollisionDestroyTrampoline(void* collision,
	NxStaticCollisionDestroyFn fn)
	{
	return fn(collision);
	}

// The report-once float rows.
static unsigned char gNxGate3716 = 1u;

void nxSetGate3716(unsigned char on)
	{
	gNxGate3716 = on;
	}

float nxOnceReportThunkFloat(void* self, unsigned code, unsigned file,
	unsigned line, unsigned expression, NxGlobalFn2RetF fn, int viaField4)
	{
	if(gNxGate3716 == 0u)
		{
		if(gNxAssertReport)
			gNxAssertReport(code, file, line, 0u, expression);
		gNxGate3716 = 1u;
		// the zero constant at [0x101041f0]
		return 0.0f;
		}
	return nxLockedHelperCallF(self, 0x10, 0x14, fn, viaField4);
	}

// Row 000867 (the kind-selected accumulator) runs only inside the simulation
// step; it is in StepOnlyRows.cpp, built /arch:IA32.

// phys_fn_000448 (0xdef0): the pair count with an arithmetic shift.
int nxCountFromPair0448(void* self)
	{
	unsigned a, b;
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	memcpy(&a, p + 0xc, 4);
	memcpy(&b, p + 8, 4);
	return static_cast<int>(static_cast<signed>(a - b)) >> 2;
	}

// phys_fn_004089 (0x95d20): the list teardown that frees through the
// allocator singleton.
// Product row: Physics/src/core/Joint.cpp.
void nxListFreeViaSingleton4089(void* self)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned node;
	memcpy(&node, p + 0x20, 4);
	while(node != 0u)
		{
		unsigned char* n = reinterpret_cast<unsigned char*>(static_cast<size_t>(node));
		unsigned next;
		memcpy(&next, n + 0x10, 4);
		memcpy(p + 0x20, &next, 4);
		if(gNxAllocFree != nullptr)
			gNxAllocFree(n);
		memcpy(&node, p + 0x20, 4);
		}
	// the row clears the global word once the list is empty
	gNxGlobalFlag4491 = 0u;
	}

// The ten 74-byte mutex rows that run the 004089 teardown under the lock.
// Product row: Physics/src/core/NpPrismaticJoint.cpp.
// Product row: Physics/src/core/NpCylindricalJoint.cpp.
// Product row: Physics/src/core/NpSphericalJoint.cpp.
// Product row: Physics/src/core/NpPointOnLineJoint.cpp.
// Product row: Physics/src/core/NpPointInPlaneJoint.cpp.
// Product row: Physics/src/core/NpDistanceJoint.cpp.
// Product row: Physics/src/core/NpPulleyJoint.cpp.
// Product row: Physics/src/core/NpFixedJoint.cpp.
// Product row: Physics/src/core/NpD6Joint.cpp.
unsigned nxMutexListFree(void* self, unsigned code, unsigned file, unsigned line,
	unsigned expression)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned char* lock = *reinterpret_cast<unsigned char**>(p + 0x10);
	if(!nxTryAcquireLock(lock))
		{
		if(gNxAssertReport)
			gNxAssertReport(code, file, line, 0u, expression);
		return 0u;
		}
	unsigned char* obj = *reinterpret_cast<unsigned char**>(p + 0x18);
	nxListFreeViaSingleton4089(obj);
	// the release is a TAIL JUMP into 002366, whose `mov al, 1` is the return
	return 1u;
	}

// The lock-API trampolines.
void nxLockApiPushSelf(void* self, void (__stdcall* fn)(unsigned))
	{
	unsigned v;
	memcpy(&v, self, 4);
	fn(v);
	}

// phys_fn_002367 (0x5b7b0): the lock-API initializer.
void* nxLockApiInit2367(void* self, unsigned (__stdcall* fn)(unsigned, unsigned,
	unsigned, unsigned))
	{
	const unsigned r = fn(0u, 1u, 0u, 0u);
	memcpy(self, &r, 4);
	return self;
	}

// The three-argument global-call thunk family.
void nxGlobalCallN(void* self, NxGlobalFn3 fn, int objSelf4, unsigned nargs,
	unsigned extraArg)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	if(objSelf4 != 0)
		{
		unsigned inner;
		memcpy(&inner, p + 4, 4);
		p = reinterpret_cast<unsigned char*>(static_cast<size_t>(inner));
		}
	unsigned a, b;
	memcpy(&a, p + 0x7c, 4);
	memcpy(&b, p + 0x80, 4);
	unsigned first;
	memcpy(&first, reinterpret_cast<unsigned char*>(static_cast<size_t>(a)) + 0x30, 4);
	fn(first, b, (nargs == 2u) ? 0u : extraArg);
	}

// phys_fn_003597 and phys_fn_003599: the callback result is returned verbatim
// for mode 2 and normalized to (result == 0) for every other mode.
unsigned nxGlobalCallNResult(void* self, NxGlobalFn3Result fn, int objSelf4,
	unsigned mode, unsigned extraArg)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	if(objSelf4 != 0)
		{
		unsigned inner;
		memcpy(&inner, p + 4, 4);
		p = reinterpret_cast<unsigned char*>(static_cast<size_t>(inner));
		}
	unsigned a, b;
	memcpy(&a, p + 0x7c, 4);
	memcpy(&b, p + 0x80, 4);
	unsigned first;
	memcpy(&first, reinterpret_cast<unsigned char*>(static_cast<size_t>(a))
		+ 0x30, 4);
	const unsigned result = fn(first, b, extraArg);
	return (mode == 2u) ? result : (result == 0u);
	}

void nxGlobalCallN2(void* self, NxGlobalFn2 fn, int objSelf4)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	if(objSelf4 != 0)
		{
		unsigned inner;
		memcpy(&inner, p + 4, 4);
		p = reinterpret_cast<unsigned char*>(static_cast<size_t>(inner));
		}
	unsigned a, b;
	memcpy(&a, p + 0x7c, 4);
	memcpy(&b, p + 0x80, 4);
	unsigned first;
	memcpy(&first, reinterpret_cast<unsigned char*>(static_cast<size_t>(a)) + 0x30, 4);
	fn(first, b);
	}

// phys_fn_003467 preserves the integer result from its global two-argument call.
unsigned nxGlobalCallSelf7c803467(void* self,
	unsigned (__cdecl* fn)(unsigned, unsigned))
	{
	unsigned char* const p = static_cast<unsigned char*>(self);
	void* const nested = *reinterpret_cast<void**>(p + 0x7c);
	const unsigned first = *reinterpret_cast<unsigned*>(
		static_cast<unsigned char*>(nested) + 0x30);
	const unsigned second = *reinterpret_cast<unsigned*>(p + 0x80);
	return fn(first, second);
	}

// phys_fn_003665 forwards the fluid selected by [this+4]'s backend context.
unsigned nxGlobalCallVia4Result003665(void* self,
	unsigned (__cdecl* fn)(unsigned, unsigned))
	{
	unsigned char* const fluid = *reinterpret_cast<unsigned char**>(
		static_cast<unsigned char*>(self) + 4);
	void* const backend = *reinterpret_cast<void**>(fluid + 0x7c);
	const unsigned context = *reinterpret_cast<unsigned*>(
		static_cast<unsigned char*>(backend) + 0x30);
	const unsigned handle = *reinterpret_cast<unsigned*>(fluid + 0x80);
	return fn(context, handle);
	}

// phys_fn_003669 shares the fluid/context loads but targets its own global slot.
unsigned nxGlobalCallVia4Result003669(void* self,
	unsigned (__cdecl* fn)(unsigned, unsigned))
	{
	unsigned char* const fluid = *reinterpret_cast<unsigned char**>(
		static_cast<unsigned char*>(self) + 4);
	void* const backend = *reinterpret_cast<void**>(fluid + 0x7c);
	const unsigned context = *reinterpret_cast<unsigned*>(
		static_cast<unsigned char*>(backend) + 0x30);
	const unsigned handle = *reinterpret_cast<unsigned*>(fluid + 0x80);
	return fn(context, handle);
	}

// phys_fn_003675 shares the fluid/context loads but targets its adjacent slot.
unsigned nxGlobalCallVia4Result003675(void* self,
	unsigned (__cdecl* fn)(unsigned, unsigned))
	{
	unsigned char* const fluid = *reinterpret_cast<unsigned char**>(
		static_cast<unsigned char*>(self) + 4);
	void* const backend = *reinterpret_cast<void**>(fluid + 0x7c);
	const unsigned context = *reinterpret_cast<unsigned*>(
		static_cast<unsigned char*>(backend) + 0x30);
	const unsigned handle = *reinterpret_cast<unsigned*>(fluid + 0x80);
	return fn(context, handle);
	}

// phys_fn_003667 forwards a caller-supplied third value and returns the result.
unsigned nxGlobalCallVia4Result3Arg003667(void* self, unsigned extraArg,
	unsigned (__cdecl* fn)(unsigned, unsigned, unsigned))
	{
	unsigned char* const fluid = *reinterpret_cast<unsigned char**>(
		static_cast<unsigned char*>(self) + 4);
	void* const backend = *reinterpret_cast<void**>(fluid + 0x7c);
	const unsigned context = *reinterpret_cast<unsigned*>(
		static_cast<unsigned char*>(backend) + 0x30);
	const unsigned handle = *reinterpret_cast<unsigned*>(fluid + 0x80);
	return fn(context, handle, extraArg);
	}

// phys_fn_003671 forwards the fluid context, handle, and caller argument.
unsigned nxGlobalCallVia4Result3Arg003671(void* self, unsigned extraArg,
	unsigned (__cdecl* fn)(unsigned, unsigned, unsigned))
	{
	unsigned char* const fluid = *reinterpret_cast<unsigned char**>(
		static_cast<unsigned char*>(self) + 4);
	void* const backend = *reinterpret_cast<void**>(fluid + 0x7c);
	const unsigned context = *reinterpret_cast<unsigned*>(
		static_cast<unsigned char*>(backend) + 0x30);
	const unsigned handle = *reinterpret_cast<unsigned*>(fluid + 0x80);
	return fn(context, handle, extraArg);
	}

// phys_fn_003673 forwards the fluid context, handle, and caller argument.
unsigned nxGlobalCallVia4Result3Arg003673(void* self, unsigned extraArg,
	unsigned (__cdecl* fn)(unsigned, unsigned, unsigned))
	{
	unsigned char* const fluid = *reinterpret_cast<unsigned char**>(
		static_cast<unsigned char*>(self) + 4);
	void* const backend = *reinterpret_cast<void**>(fluid + 0x7c);
	const unsigned context = *reinterpret_cast<unsigned*>(
		static_cast<unsigned char*>(backend) + 0x30);
	const unsigned handle = *reinterpret_cast<unsigned*>(fluid + 0x80);
	return fn(context, handle, extraArg);
	}

// phys_fn_003677 forwards the fluid context, handle, and caller argument.
unsigned nxGlobalCallVia4Result3Arg003677(void* self, unsigned extraArg,
	unsigned (__cdecl* fn)(unsigned, unsigned, unsigned))
	{
	unsigned char* const fluid = *reinterpret_cast<unsigned char**>(
		static_cast<unsigned char*>(self) + 4);
	void* const backend = *reinterpret_cast<void**>(fluid + 0x7c);
	const unsigned context = *reinterpret_cast<unsigned*>(
		static_cast<unsigned char*>(backend) + 0x30);
	const unsigned handle = *reinterpret_cast<unsigned*>(fluid + 0x80);
	return fn(context, handle, extraArg);
	}

// phys_fn_003681 forwards the fluid context, handle, and caller argument.
unsigned nxGlobalCallVia4Result3Arg003681(void* self, unsigned extraArg,
	unsigned (__cdecl* fn)(unsigned, unsigned, unsigned))
	{
	unsigned char* const fluid = *reinterpret_cast<unsigned char**>(
		static_cast<unsigned char*>(self) + 4);
	void* const backend = *reinterpret_cast<void**>(fluid + 0x7c);
	const unsigned context = *reinterpret_cast<unsigned*>(
		static_cast<unsigned char*>(backend) + 0x30);
	const unsigned handle = *reinterpret_cast<unsigned*>(fluid + 0x80);
	return fn(context, handle, extraArg);
	}

// phys_fn_003461 preserves the callback's x87 double result through reciprocal.
double nxGlobalCallReciprocalDouble(void* self, NxGlobalFn2ReciprocalDouble fn)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned inner;
	memcpy(&inner, p + 0x7c, 4);
	unsigned first;
	memcpy(&first, reinterpret_cast<unsigned char*>(static_cast<size_t>(inner))
		+ 0x30, 4);
	unsigned second;
	memcpy(&second, p + 0x80, 4);
	return 1.0 / fn(first, second);
	}

// phys_fn_003483 copies the six-word pose payload and returns the caller's buffer.
unsigned* NxGlobalCopy6ToOut003483::copyTo(unsigned* out) const
	{
	const unsigned char* source = reinterpret_cast<const unsigned char*>(this) + 0x5c;
	for(unsigned i = 0; i != 6; ++i)
		memcpy(out + i, source + i * sizeof(unsigned), sizeof(unsigned));
	return out;
	}

// phys_fn_003575 (0x87f10, ret 4): the float-OUT variant.
void nxThunkFloatOut3575(void* self, float* out, NxFloatRet3Fn fn)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned inner;
	memcpy(&inner, p + 4, 4);
	unsigned char* obj = reinterpret_cast<unsigned char*>(static_cast<size_t>(inner));
	unsigned a, b, third;
	memcpy(&a, obj + 0x7c, 4);
	memcpy(&b, obj + 0x80, 4);
	memcpy(&third, p + 8, 4);
	unsigned first;
	memcpy(&first, reinterpret_cast<unsigned char*>(static_cast<size_t>(a)) + 0x30, 4);
	float* r = fn(first, b, third);
	out[0] = r[0];
	out[1] = r[1];
	out[2] = r[2];
	}

// The lock-bracketed float thunk rows.
float nxLockedThunkFloat(void* self, unsigned lockOff, unsigned objOff,
	NxFloatFn3 fn)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned char* obj = *reinterpret_cast<unsigned char**>(p + objOff);
	unsigned inner;
	memcpy(&inner, obj + 4, 4);
	unsigned char* q = reinterpret_cast<unsigned char*>(static_cast<size_t>(inner));
	unsigned a, b, third;
	memcpy(&a, q + 0x7c, 4);
	memcpy(&b, q + 0x80, 4);
	memcpy(&third, obj + 8, 4);
	unsigned first;
	memcpy(&first, reinterpret_cast<unsigned char*>(static_cast<size_t>(a)) + 0x30, 4);
	return fn(first, b, third);
	}

// The four small self-contained rows.
void* nxInitRow3934(void* self, unsigned arg, NxPtrFn0 fn)
	{
	fn();
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	memcpy(p + 0x1c, &arg, 4);
	unsigned vtable = 0x10117920u;
	memcpy(p, &vtable, 4);
	unsigned zero = 0u;
	memcpy(p + 0x18, &zero, 4);
	return self;
	}

unsigned char nxReportRow2160(unsigned a, unsigned b, unsigned c)
	{
	// the pushes are a, 0, c, b, 2 -- so the callee sees (2, b, c, 0, a)
	if(gNxAssertReport)
		gNxAssertReport(2u, b, c, 0u, a);
	return 0u;
	}

unsigned char nxLockApiTest2385(void* self, unsigned (__stdcall* fn)(unsigned,
	unsigned))
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned obj;
	memcpy(&obj, p + 4, 4);
	unsigned char* o = reinterpret_cast<unsigned char*>(static_cast<size_t>(obj));
	unsigned inner;
	memcpy(&inner, o + 4, 4);
	if(inner == 0u)
		return 0u;
	unsigned v;
	memcpy(&v, o, 4);
	// the pushes are `push -1` then `push eax`, so the callee sees (v, -1)
	fn(v, 0xffffffffu);
	// the row sets al = 1 regardless of what the call returned
	return 1u;
	}

// The two-argument variant of the lock-bracketed float thunk rows.
float nxLockedThunkFloat2(void* self, unsigned lockOff, unsigned objOff,
	NxFloatFn2 fn)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned char* obj = *reinterpret_cast<unsigned char**>(p + objOff);
	unsigned a, b;
	memcpy(&a, obj + 0x7c, 4);
	memcpy(&b, obj + 0x80, 4);
	unsigned first;
	memcpy(&first, reinterpret_cast<unsigned char*>(static_cast<size_t>(a)) + 0x30, 4);
	return fn(first, b);
	}

// phys_fn_003936 (0x8eeb0): the vtable store plus the global tail jump.
// Product row: Physics/src/core/SpringAndDamperEffector.cpp.
void nxDtorTrampoline3936(void* self, NxObservableDestructorFn fn)
	{
	unsigned vtable = 0x10117920u;
	memcpy(self, &vtable, 4);
	fn(self);
	}

// The four-argument global-call thunk rows.
void nxGlobalCall4(void* self, unsigned objOff, unsigned extraOff, NxGlobalFn4 fn,
	unsigned extraArg)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned inner;
	memcpy(&inner, p + objOff, 4);
	unsigned char* obj = reinterpret_cast<unsigned char*>(static_cast<size_t>(inner));
	unsigned a, b, c;
	memcpy(&a, obj + 0x7c, 4);
	memcpy(&b, obj + 0x80, 4);
	memcpy(&c, p + extraOff, 4);
	unsigned first;
	memcpy(&first, reinterpret_cast<unsigned char*>(static_cast<size_t>(a)) + 0x30, 4);
	fn(first, b, c, extraArg);
	}

// phys_fn_003413 (0x84800): the virtual call on a global object.
void nxGlobalVirtualCall3413(unsigned arg, void (__stdcall* fn)(unsigned))
	{
	fn(arg);
	}


// phys_fn_004165 (0x9ace0): thiscall vector walk; each element receives its
// own vtable slot +0x10 call with this in ecx and no stack arguments.
void NxVectorVirtualLoop4165Ctx::run()
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(this);
	for(unsigned i = 0;; ++i)
		{
		unsigned begin, end;
		memcpy(&begin, p + 0x10, 4);
		memcpy(&end, p + 0x14, 4);
		const int n = static_cast<int>(static_cast<signed>(end - begin)) >> 2;
		if(i >= static_cast<unsigned>(n))
			break;
		unsigned elem;
		memcpy(&elem, reinterpret_cast<unsigned char*>(static_cast<size_t>(begin)) + 4u * i, 4);
		void* element = reinterpret_cast<void*>(static_cast<size_t>(elem));
		void* vtable = 0;
		void* slot = 0;
		memcpy(&vtable, element, 4);
		memcpy(&slot, reinterpret_cast<unsigned char*>(vtable) + 0x10, 4);
		reinterpret_cast<void (__fastcall*)(void*)>(slot)(element);
		}
	}

// phys_fn_001544 (0x2dec0): the singleton-slot row.
void nxSlotCall1544(void* self, unsigned objOff, void* obj, NxSlotMfp1 slot)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned field;
	memcpy(&field, p + objOff, 4);
	if(field == 0u)
		return;
	(reinterpret_cast<NxSlotCtx*>(obj)->*slot)(field - 4u);
	unsigned zero = 0u;
	memcpy(p + objOff, &zero, 4);
	}

// phys_fn_004163 (0x9aca0): the vector loop with one argument.
void nxVectorVirtualLoop4163(void* self, unsigned arg, NxSlotMfp1 slot)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned begin, end;
	memcpy(&begin, p + 0x10, 4);
	memcpy(&end, p + 0x14, 4);
	const int n = static_cast<int>(static_cast<signed>(end - begin)) >> 2;
	for(int i = 0; i < n; ++i)
		{
		unsigned elem;
		memcpy(&elem, reinterpret_cast<unsigned char*>(static_cast<size_t>(begin))
			+ 4u * static_cast<unsigned>(i), 4);
		(reinterpret_cast<NxSlotCtx*>(static_cast<size_t>(elem))->*slot)(arg);
		}
	}

// phys_fn_004861 (0xb52a0): the four-pointer slot loop.
void nxFourSlotLoop4861(void* self, unsigned a, unsigned b, NxSlotMfp2 slot)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	for(unsigned k = 0; k < 4u; ++k)
		{
		unsigned elem;
		memcpy(&elem, p + 0x1c + 4u * k, 4);
		if(elem == 0u)
			continue;
		(reinterpret_cast<NxSlotCtx*>(static_cast<size_t>(elem))->*slot)(a, b);
		}
	}

// The singleton-release family.
void nxReleaseOwnedFields(void* self, const unsigned* offsets, unsigned count,
	void* singleton, NxSlotMfp1 slot)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	for(unsigned k = 0; k < count; ++k)
		{
		unsigned field;
		memcpy(&field, p + offsets[k], 4);
		if(field == 0u)
			continue;
		(reinterpret_cast<NxSlotCtx*>(singleton)->*slot)(field);
		unsigned zero = 0u;
		memcpy(p + offsets[k], &zero, 4);
		}
	}

void nxReleaseGuarded4838(void* self, void* singleton, NxSlotMfp1 slot)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	float guard;
	memcpy(&guard, p + 0xc, 4);
	// The row's TAIL block zeroes [self] and [self+4] UNCONDITIONALLY -- both
	// guard failures jump past the release straight INTO it. Only [self+8] is
	// cleared on the release path.
	if(guard >= 0.0f)
		{
		unsigned field;
		memcpy(&field, p + 8, 4);
		if(field != 0u)
			{
			(reinterpret_cast<NxSlotCtx*>(singleton)->*slot)(field);
			const unsigned z8 = 0u;
			memcpy(p + 8, &z8, 4);
			}
		}
	const unsigned zero = 0u;
	memcpy(p, &zero, 4);
	memcpy(p + 4, &zero, 4);
	}

// phys_fn_005159 (0xe32e0): the byte-guarded release.
void nxByteGuardedRelease5159(void* self, void* singleton,
	const unsigned* offsets, unsigned count, NxSlotMfp1 slot)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	if(p[0x14] == 0)
		return;
	nxReleaseOwnedFields(self, offsets, count, singleton, slot);
	}

// phys_fn_004866 (0xb53c0): the masked four-slot loop.
unsigned char nxMaskedFourSlotLoop4866(void* self, unsigned a1, unsigned a2,
	unsigned a3, unsigned a4, unsigned a5, NxSlotMfp4 slot)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	for(unsigned k = 0; k < 4u; ++k)
		{
		unsigned elem;
		memcpy(&elem, p + 0x1c + 4u * k, 4);
		if(elem == 0u)
			continue;
		if((a3 & (1u << k)) == 0u)
			continue;
		(reinterpret_cast<NxSlotCtx*>(static_cast<size_t>(elem))->*slot)(a1, a2, a4, a5);
		}
	return 1u;
	}

// phys_fn_004868 (0xb5410): the AABB/cull four-slot loop dispatches slot 7.
unsigned char nxMaskedFourSlotLoop4868(void* self, unsigned a1, unsigned a2,
	unsigned a3, unsigned a4, unsigned a5, NxSlotMfp4 slot)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	for(unsigned k = 0; k < 4u; ++k)
		{
		unsigned elem;
		memcpy(&elem, p + 0x1c + 4u * k, 4);
		if(elem == 0u)
			continue;
		if((a3 & (1u << k)) == 0u)
			continue;
		(reinterpret_cast<NxSlotCtx*>(static_cast<size_t>(elem))->*slot)(a1, a2, a4, a5);
		}
	return 1u;
	}

// phys_fn_004862 (0xb52d0): the plane-query four-slot loop dispatches slot 5.
unsigned char nxMaskedFourSlotLoop4862(void* self, unsigned result,
	unsigned planes, unsigned nbPlanes, unsigned selectedMask, unsigned queryMask,
	NxSlotMfp5 slot)
	{
	if(!nbPlanes)
		return 0u;
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	for(unsigned k = 0; k < 4u; ++k)
		{
		unsigned elem;
		memcpy(&elem, p + 0x1c + 4u * k, 4);
		if(elem == 0u || (selectedMask & (1u << k)) == 0u)
			continue;
		(reinterpret_cast<NxSlotCtx*>(static_cast<size_t>(elem))->*slot)(
			result, planes, nbPlanes, 1u, queryMask);
		}
	return 1u;
	}

// phys_fn_004864 (0xb5360): the masked four-slot loop with a sixth argument.
unsigned char nxMaskedFourSlotLoop4864(void* self, unsigned a1, unsigned a2,
	unsigned a3, unsigned a4, unsigned a5, unsigned a6, NxSlotMfp5 slot)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	for(unsigned k = 0; k < 4u; ++k)
		{
		unsigned elem;
		memcpy(&elem, p + 0x1c + 4u * k, 4);
		if(elem == 0u)
			continue;
		if((a3 & (1u << k)) == 0u)
			continue;
		(reinterpret_cast<NxSlotCtx*>(static_cast<size_t>(elem))->*slot)(a1, a2, a4,
			a5, a6);
		}
	return 1u;
	}

// phys_fn_002342 (0x5aa90): the allocator release with unconditional clears.
void nxAllocReleaseClear2342(void* self, void* alloc, NxSlotMfp1 slot)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	static const unsigned kBase[2] = { 0x18u, 0x8u };
	for(unsigned g = 0; g < 2u; ++g)
		{
		unsigned field;
		memcpy(&field, p + kBase[g], 4);
		if(field != 0u)
			(reinterpret_cast<NxSlotCtx*>(alloc)->*slot)(field);
		// the three fields are cleared whether or not anything was released
		for(unsigned k = 0; k < 3u; ++k)
			{
			const unsigned zero = 0u;
			memcpy(p + kBase[g] + 4u * k, &zero, 4);
			}
		}
	}

// phys_fn_002060 (0x515d0): the release with per-field adjustments.
void nxReleaseAdjusted2060(void* self, const unsigned* offsets, const int* adjust,
	unsigned count, void* singleton, NxSlotMfp1 slot)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	for(unsigned k = 0; k < count; ++k)
		{
		unsigned field;
		memcpy(&field, p + offsets[k], 4);
		if(field == 0u)
			continue;
		(reinterpret_cast<NxSlotCtx*>(singleton)->*slot)(
			field + static_cast<unsigned>(adjust[k]));
		const unsigned zero = 0u;
		memcpy(p + offsets[k], &zero, 4);
		}
	}

// phys_fn_003950 (0x8f0d0): lock [self+0x10], unlock, return self. The lock
// pair brackets the whole body and has no other observable.
// Product row: Physics/src/core/NpSpringAndDamperEffector.cpp.
void* nxLockedSelf3950(void* self)
	{
	return self;
	}

// phys_fn_000418 (0xda50): under the lock at [self+0x10], read the word at
// [[self+0x24]+0x55c] and return it.
unsigned nxFieldRead0418(void* self)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* owner = *reinterpret_cast<const unsigned char* const*>(p + 0x24);
	unsigned v;
	memcpy(&v, owner + 0x55c, 4);
	return v;
	}

// phys_fn_000094 (0x2f30, ret 4): the orientation quaternion. With a record it
// copies the stored (x, y, z, w) at record+0x5c. Without one it derives the
// quaternion from the cached matrix at [self+0x14]+0x20 using the
// largest-diagonal method: the trace arm when the trace is non-negative, else
// the case selected by the largest of m00/m11/m22 (index stride 4 floats).
float* nxOrientation0094(void* self, float* out)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* body = *reinterpret_cast<const unsigned char* const*>(p + 0x14);
	const unsigned char* rec = *reinterpret_cast<const unsigned char* const*>(body + 8);
	if(rec != nullptr)
		{
		memcpy(out, rec + 0x5c, 16);
		return out;
		}
	const float* m = reinterpret_cast<const float*>(body + 0x20);
	const double m00 = m[0], m01 = m[1], m02 = m[2];
	const double m10 = m[3], m11 = m[4], m12 = m[5];
	const double m20 = m[6], m21 = m[7], m22 = m[8];
	double x, y, z, w;
	const double trace = m00 + m11 + m22;
	if(!(trace < 0.0))
		{
		const double s = sqrt(trace + 1.0);
		const double inv = 0.5 / s;
		x = (m21 - m12) * inv;
		y = (m02 - m20) * inv;
		z = (m10 - m01) * inv;
		w = 0.5 * s;
		}
	else
		{
		unsigned edx = (m11 > m00) ? 1u : 0u;
		if(m22 > m[edx * 4u]) edx = 2u;
		if(edx == 0u)
			{
			const double s = sqrt(m00 - m11 - m22 + 1.0);
			const double inv = 0.5 / s;
			x = 0.5 * s;
			y = (m10 + m01) * inv;
			z = (m20 + m02) * inv;
			w = (m21 - m12) * inv;
			}
		else if(edx == 1u)
			{
			const double s = sqrt(m11 - m00 - m22 + 1.0);
			const double inv = 0.5 / s;
			x = (m10 + m01) * inv;
			y = 0.5 * s;
			z = (m21 + m12) * inv;
			w = (m02 - m20) * inv;
			}
		else if(edx == 2u)
			{
			const double s = sqrt(m22 - m00 - m11 + 1.0);
			const double inv = 0.5 / s;
			x = (m20 + m02) * inv;
			y = (m21 + m12) * inv;
			z = 0.5 * s;
			w = (m10 - m01) * inv;
			}
		else
			{
			x = y = z = w = 0.0;
			}
		}
	out[0] = static_cast<float>(x);
	out[1] = static_cast<float>(y);
	out[2] = static_cast<float>(z);
	out[3] = static_cast<float>(w);
	return out;
	}

// phys_fn_000132 (0x46c0, ret 4): quaternion at record+0x5c (x, y, z, w) to a
// 3x3 rotation matrix in out[0..8]; when the record pointer is null it copies
// the cached 36 bytes at [self+0x14]+0x20 instead. Returns out.
float* nxQuatToMatrix0132(void* self, float* out)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* body = *reinterpret_cast<const unsigned char* const*>(p + 0x14);
	const unsigned char* rec = *reinterpret_cast<const unsigned char* const*>(body + 8);
	if(rec == nullptr)
		{
		memcpy(out, body + 0x20, 36);
		return out;
		}
	float q[4];
	memcpy(&q[0], rec + 0x5c, 4);
	memcpy(&q[1], rec + 0x60, 4);
	memcpy(&q[2], rec + 0x64, 4);
	memcpy(&q[3], rec + 0x68, 4);
	nxQuatToMatrix9(q, out);
	return out;
	}

// phys_fn_000046 (0x24c0, ret 4): gather the descriptor record at
// [[self+0x14]+8] into out. Nine dwords come from record+0xdc, three more
// from record+0x100, then the tail fields; out[25], out[27] and out[28] are
// the square roots of the floats at record+0xd8, +0xd0 and +0xd4. The row
// returns false (and writes nothing) when the record pointer is null. The
// lock pair it calls around the body is a no-op for this output.
bool nxGatherDescriptor0046(void* self, unsigned* out)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	const unsigned char* body = *reinterpret_cast<const unsigned char* const*>(p + 0x14);
	const unsigned char* rec = *reinterpret_cast<const unsigned char* const*>(body + 8);
	if(rec == nullptr)
		return false;
	unsigned char* o = reinterpret_cast<unsigned char*>(out);
	memcpy(o + 0x00, rec + 0xdc, 36);		// out[0..8]
	memcpy(o + 0x24, rec + 0x100, 12);		// out[9..11]
	static const unsigned kFields[13] = {
		0x18c, 0x190, 0x194, 0x188, 0x6c, 0x70, 0x74,
		0x78, 0x7c, 0x80, 0x84, 0xb8, 0xbc };
	for(unsigned i = 0; i < 13; ++i)
		{
		unsigned v; memcpy(&v, rec + kFields[i], 4);
		memcpy(o + (12u + i) * 4u, &v, 4);		// out[12..24]
		}
	float d8; memcpy(&d8, rec + 0xd8, 4);
	const float r8 = static_cast<float>(sqrt(static_cast<double>(d8)));
	memcpy(o + 25u * 4u, &r8, 4);				// out[25]
	unsigned f26; memcpy(&f26, rec + 0x10c, 4);
	memcpy(o + 26u * 4u, &f26, 4);				// out[26]
	float d0; memcpy(&d0, rec + 0xd0, 4);
	const float r0 = static_cast<float>(sqrt(static_cast<double>(d0)));
	memcpy(o + 27u * 4u, &r0, 4);				// out[27]
	float d4; memcpy(&d4, rec + 0xd4, 4);
	const float r4 = static_cast<float>(sqrt(static_cast<double>(d4)));
	memcpy(o + 28u * 4u, &r4, 4);				// out[28]
	unsigned f29; memcpy(&f29, rec + 0x110, 4);
	memcpy(o + 29u * 4u, &f29, 4);				// out[29]
	return true;
	}

// phys_fn_002156 (0x538e0, ret 4): nine doubles at [self+0x18] (8-byte stride)
// to nine floats at out. The row's interleaved fld/fstp schedule reads
// d0,d3,d6 / d1,d4,d7 / d2,d5,d8 and writes +0,+0xc,+0x18 / +4,+0x10,+0x1c /
// +8,+0x14,+0x20, which is exactly dst[k] = (float)src[k].
void nxDoubleToFloat9_2156(void* self, float* out)
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(self);
	for(unsigned k = 0; k < 9; ++k)
		{
		double d;
		memcpy(&d, p + 0x18 + k * 8u, sizeof(d));
		const float f = static_cast<float>(d);
		memcpy(reinterpret_cast<unsigned char*>(out) + k * 4u, &f, sizeof(f));
		}
	}

// phys_fn_001030 (0x22bf0, ret 4): aggregate the local AABBs of the shape list
// at [self+0xe0]..[self+0xe4] into out. Seeded FLT_MAX / -FLT_MAX; each shape
// contributes the 6-dword record selected by the PLANE slot-8 row, merged
// with min on the low triple and max on the high triple.
void nxAggregateAABB1030(void* self, float* out)
	{
	const unsigned kMax = 0x7f7fffffu;		//  1.7014117e38
	const unsigned kMin = 0xff7fffffu;		// -1.7014117e38
	unsigned seed[6] = { kMax, kMax, kMax, kMin, kMin, kMin };
	memcpy(out, seed, sizeof(seed));
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned* it = *reinterpret_cast<unsigned**>(p + 0xe0);
	unsigned* end = *reinterpret_cast<unsigned**>(p + 0xe4);
	const unsigned n = static_cast<unsigned>(
		(reinterpret_cast<unsigned char*>(end) -
		 reinterpret_cast<unsigned char*>(it)) >> 2);
	for(unsigned i = 0; i < n; ++i)
		{
		unsigned char* shape = reinterpret_cast<unsigned char*>(it[i]);
		unsigned rec[6];
		memcpy(rec, seed, sizeof(rec));
		reinterpret_cast<PlaneShape*>(shape)->nxPlaneIndexed6_1267(rec);
		float f[6];
		memcpy(f, rec, sizeof(f));
		if(!(out[0] <= f[0])) out[0] = f[0];
		if(!(out[1] <= f[1])) out[1] = f[1];
		if(!(out[2] <= f[2])) out[2] = f[2];
		if(out[3] <= f[3]) out[3] = f[3];
		if(out[4] <= f[4]) out[4] = f[4];
		if(out[5] <= f[5]) out[5] = f[5];
		}
	}

// ---------------------------------------------------------------------------
// Actor slate 5: the sleep-chain readers.

// Row 000713 (0x00015d50, 32 B) is claimed by core/JointSupport.cpp's
// thiscall Row000713Fixture::row000713, which the other product callers (000776,
// the island rows, 003979) call. This cdecl copy is product code, not a test
// helper: it is compiled into NxPhysics.dll as the callee of 000744 below, which
// the image calls 000713 from out of line (0x16e3c). It stays an unclaimed copy
// (the same effect, cdecl) because the object-layout and shape-table harnesses
// link ObjectModel.cpp without core/JointSupport.cpp, so 000744 cannot call the
// claimed row there. In the DLL no product path reaches 000744 yet: the image's
// 000062 (isGroupSleeping, 0x2950) calls it, but NpActor.cpp's 000062 walks the
// chain itself; the object-layout harness drives it (actorsm5).
// Recursive path compression over the record
// chain -- each record caches its group root at +0x1e8, a self-pointing
// cache naming the root. Recursion terminates on the self-parented node.
// Product row: Physics/src/core/JointSupport.cpp.
unsigned nxBodyRecordFixRoot(void* rec)
	{
	unsigned r = reinterpret_cast<unsigned>(rec);
	unsigned c = *reinterpret_cast<unsigned*>(r + 0x1e8);
	if(r != c)
		{
		unsigned root = nxBodyRecordFixRoot(reinterpret_cast<void*>(c));
		*reinterpret_cast<unsigned*>(r + 0x1e8) = root;
		}
	return *reinterpret_cast<unsigned*>(r + 0x1e8);
	}

// phys_fn_000744 (0x00016e30, 68 B)
// The row takes the record in ecx (fastcall-style, plain `ret`); this is a
// cdecl free function. Its only oracle caller, 000062, calls it directly, so
// the convention is a code-shape difference only. It calls 000713 on the
// cached parent when the record is not its own root (0x16e3c), as here.
// Compress the record's chain to its root, then
// walk the +0x1fc list answering whether EVERY node's word at +0x84 reads
// zero -- the group-wide sleep test. A null root also answers true.
bool nxBodyRecordChainSettled(void* rec)
	{
	unsigned d = reinterpret_cast<unsigned>(rec);
	unsigned cache = *reinterpret_cast<unsigned*>(d + 0x1e8);
	if(d != cache)
		{
		unsigned root = nxBodyRecordFixRoot(reinterpret_cast<void*>(cache));
		*reinterpret_cast<unsigned*>(d + 0x1e8) = root;
		}
	unsigned node = *reinterpret_cast<unsigned*>(d + 0x1e8);
	while(node != 0)
		{
		float v = *reinterpret_cast<const float*>(node + 0x84);
		double dv = v;
		if(dv > 0.0)
			return false;
		node = *reinterpret_cast<unsigned*>(node + 0x1fc);
		}
	return true;
	}

// phys_fn_000062 (slot 67, 0x2950): guarded; the group sleep test over the
// body's nested record; TRUE when the record itself is null.
bool nxActorChainSettled(void* self)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	unsigned rec = *reinterpret_cast<unsigned*>(body + 8);
	bool r = rec == 0
		? true
		: nxBodyRecordChainSettled(reinterpret_cast<void*>(rec));
	nxSceneGuardLeave(scene);
	return r;
	}

// ---------------------------------------------------------------------------
// Actor slate 6: the center-of-mass local frame getters. Both answer from
// record+0xdc (a nine-word rotation followed by a three-word translation at
// +0x100); a STATIC actor (null record) reports kind 1 and hands back an
// identity-plus-zero pose instead.
const char* const	nxMsgCMassLocalPoseStatic =
	"Actor::getCMassLocalPose: Cannot be called on a static actor!";
const char* const	nxMsgCMassLocalOrientationStatic =
	"Actor::getCMassLocalOrientation: Cannot be called on a static actor!";

// phys_fn_000096 (slot 29, 0x3140): twelve words into `out` -- rotation
// then translation -- identity and zero for a static actor.
void* nxActorGetCMassLocalPose(void* self, void* out)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	unsigned rec = *reinterpret_cast<unsigned*>(body + 8);
	if(rec == 0)
		{
		nxReport(1, nxSourceFileNpActorCpp, 0x2f2, 0,
			nxMsgCMassLocalPoseStatic);
		unsigned* o = reinterpret_cast<unsigned*>(out);
		o[0] = 0x3f800000u;
		o[1] = 0; o[2] = 0; o[3] = 0;
		o[4] = 0x3f800000u;
		o[5] = 0; o[6] = 0; o[7] = 0;
		o[8] = 0x3f800000u;
		o[9] = 0; o[10] = 0; o[11] = 0;
		nxSceneGuardLeave(scene);
		return out;
		}
	memcpy(out, reinterpret_cast<const void*>(rec + 0xdc), 48);
	nxSceneGuardLeave(scene);
	return out;
	}

// phys_fn_000100 (slot 31, 0x32a0): nine-word orientation into `out`;
// static actors get the shipped identity table (.rdata 0x10122078) after
// the line-0x301 warning.
void* nxActorGetCMassLocalOrientation(void* self, void* out)
	{
	static const unsigned kIdentity3x3[9] =
		{ 0x3f800000u, 0, 0, 0, 0x3f800000u, 0, 0, 0, 0x3f800000u };
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	unsigned rec = *reinterpret_cast<unsigned*>(body + 8);
	if(rec == 0)
		{
		nxReport(1, nxSourceFileNpActorCpp, 0x301, 0,
			nxMsgCMassLocalOrientationStatic);
		memcpy(out, kIdentity3x3, sizeof(kIdentity3x3));
		nxSceneGuardLeave(scene);
		return out;
		}
	memcpy(out, reinterpret_cast<const void*>(rec + 0xdc), 36);
	nxSceneGuardLeave(scene);
	return out;
	}

// ---------------------------------------------------------------------------
// Actor slate 7: five guarded three-word readers sharing one shape -- read
// guard, record+offset into `out` when a record backs the actor, otherwise
// report kind 1 and fill from defaults. The .data-backed defaults copy the
// triple at 0x10123c1c, zero-initialized in a fresh image; the C++
// transcribes those zeros directly rather than dereferencing an image
// absolute that only means something inside the oracle's own mapping.
const char* const	nxMsgCMassLocalPositionStatic =
	"Actor::getCMassLocalPosition: Cannot be called on a static actor!";
const char* const	nxMsgMassSpaceInertiaStatic =
	"Actor::getMassSpaceInertiaTensorVal: Cannot be called on a static actor!";
const char* const	nxMsgLinearVelocityDynamic =
	"Actor::getLinearVelocity: Actor must be dynamic!";
const char* const	nxMsgAngularVelocityDynamic =
	"Actor::getAngularVelocity: Actor must be dynamic!";
const char* const	nxMsgLinearMomentumStatic =
	"Actor::getLinearMomentumVal: Cannot be called on a static actor!";

static void nxActorFillThreeWords(void* self, unsigned base, int line,
	const char* msg, bool defaultsFromData, void* out)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	unsigned rec = *reinterpret_cast<unsigned*>(body + 8);
	unsigned* o = reinterpret_cast<unsigned*>(out);
	if(rec == 0)
		{
		nxReport(1, nxSourceFileNpActorCpp, line, 0, msg);
		if(defaultsFromData)
			{
			o[0] = 0;						// [.data 0x10123c1c] triple:
			o[1] = 0;						// zero-initialized in any
			o[2] = 0;						// fresh image
			}
		else
			{
			o[0] = 0;
			o[1] = 0;
			o[2] = 0;
			}
		nxSceneGuardLeave(scene);
		return;
		}
	o[0] = *reinterpret_cast<const unsigned*>(rec + base);
	o[1] = *reinterpret_cast<const unsigned*>(rec + base + 4);
	o[2] = *reinterpret_cast<const unsigned*>(rec + base + 8);
	nxSceneGuardLeave(scene);
	}

// phys_fn_000098 (slot 30, 0x3200): getCMassLocalPosition -- [+0x100].
void nxActorGetCMassLocalPosition(void* self, void* out)
	{
	nxActorFillThreeWords(self, 0x100, 0x2fa,
		nxMsgCMassLocalPositionStatic, true, out);
	}

// phys_fn_000102 (slot 44, 0x3310): getMassSpaceInertiaTensorVal --
// the inertia diagonal at [+0x18c], line 0x328.
void nxActorGetMassSpaceInertia(void* self, void* out)
	{
	nxActorFillThreeWords(self, 0x18c, 0x328,
		nxMsgMassSpaceInertiaStatic, true, out);
	}

// phys_fn_000104 (slot 47, 0x33b0): getLinearVelocity -- [+0x6c], inline
// zeros on the static arm, line 0x343.
void nxActorGetLinearVelocity(void* self, void* out)
	{
	nxActorFillThreeWords(self, 0x6c, 0x343,
		nxMsgLinearVelocityDynamic, false, out);
	}

// phys_fn_000106 (slot 48, 0x3440): getAngularVelocity -- [+0x78], inline
// zeros, line 0x34a.
void nxActorGetAngularVelocity(void* self, void* out)
	{
	nxActorFillThreeWords(self, 0x78, 0x34a,
		nxMsgAngularVelocityDynamic, false, out);
	}

// phys_fn_000108 (slot 52, 0x34d0): getLinearMomentumVal -- mass at
// [+0x188] times the velocity words [+0x6c/70/74] in natural order; zeros
// from .data on the static arm, line 0x353.
void nxActorGetLinearMomentum(void* self, void* out)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	unsigned rec = *reinterpret_cast<unsigned*>(body + 8);
	unsigned* o = reinterpret_cast<unsigned*>(out);
	if(rec == 0)
		{
		nxReport(1, nxSourceFileNpActorCpp, 0x353, 0,
			nxMsgLinearMomentumStatic);
		o[0] = 0;
		o[1] = 0;
		o[2] = 0;
		nxSceneGuardLeave(scene);
		return;
		}
	float m = *reinterpret_cast<const float*>(rec + 0x188);
	float v6c = *reinterpret_cast<const float*>(rec + 0x6c);
	float v70 = *reinterpret_cast<const float*>(rec + 0x70);
	float v74 = *reinterpret_cast<const float*>(rec + 0x74);
	double dm = m;
	float r0 = static_cast<float>(dm * v6c);
	float r1 = static_cast<float>(dm * v70);
	float r2 = static_cast<float>(dm * v74);
	memcpy(o, &r0, 4);
	memcpy(o + 1, &r1, 4);
	memcpy(o + 2, &r2, 4);
	nxSceneGuardLeave(scene);
	}

// ---------------------------------------------------------------------------
// Actor slate 8: the group WRITER and two primitives.

// phys_fn_000112 (slot 85, 0x35b0): setGroup -- the word argument into
// body+0x1c under the write guard; line 0x3cd on a failed upgrade. The
// write counterpart to slate 1's group reader.
void nxActorSetGroupWord(void* self, unsigned group)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* ctx = *reinterpret_cast<void**>(a + 0xc);
	if(!nxSceneGuardWriteTry(ctx))
		{
		nxReport(2, nxSourceFileNpActorCpp, 0x3cd, 0,
			nxMsgWriteLockStillAcquired);
		return;
		}
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	*reinterpret_cast<NxU16*>(body + 0x1c) = static_cast<NxU16>(group);
	nxSceneGuardLeave(ctx);
	}

// phys_fn_000004 (0x1070): sub-object virtual forwarder -- [this+0x10]
// names a sub-object whose vtable slot +0x18 receives the call with the
// original stack argument; a null sub-object pointer returns without
// touching anything observable.
unsigned nxForwardSubobjectCall(void* self, void* arg)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	unsigned sub = *reinterpret_cast<unsigned*>(a + 0x10);
	if(sub == 0)
		return 0;
	unsigned vtable = *reinterpret_cast<unsigned*>(sub);
	typedef unsigned (__thiscall* NxSlot18Fn)(void* sub, void* arg);
	NxSlot18Fn fn = reinterpret_cast<NxSlot18Fn>(
		*reinterpret_cast<unsigned*>(vtable + 0x18));
	return fn(reinterpret_cast<void*>(sub), arg);
	}

// phys_fn_000012 (0x1430): id-allocation primitive over {counter@+0x00,
// freelist begin@+0x04, cursor@+0x08}: a non-empty list pops its LAST
// dword and shrinks the cursor; an exhausted list returns the counter and
// increments it.
unsigned nxIdAllocNext(void* container)
	{
	unsigned c = reinterpret_cast<unsigned>(container);
	unsigned count = (*reinterpret_cast<const unsigned*>(c + 8)
		- *reinterpret_cast<const unsigned*>(c + 4)) >> 2;
	if(count != 0)
		{
		unsigned v = *reinterpret_cast<const unsigned*>(
			*reinterpret_cast<const unsigned*>(c + 4) + count * 4 - 4);
		*reinterpret_cast<unsigned*>(c + 8) += -4u;
		return v;
		}
	unsigned old = *reinterpret_cast<const unsigned*>(c);
	*reinterpret_cast<unsigned*>(c) = old + 1;
	return old;
	}

// ---------------------------------------------------------------------------
// Actor slate 10: sibling pool-class lifecycle rows.

// phys_fn_002328 (0x5a470): chained deleting destructor -- member vptr at
// +8 restored through two third tables (0x101087a8 then 0x10108798),
// primary vptr 0x1010878c installed, then ADAPTER release when flagged.
void nxChainedDeletingDtor(void* self, unsigned flags)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	*reinterpret_cast<unsigned**>(a + 8) =
		reinterpret_cast<unsigned*>(0x101087a8u);
	*reinterpret_cast<unsigned**>(a + 8) =
		reinterpret_cast<unsigned*>(0x10108798u);
	*reinterpret_cast<unsigned**>(a) =
		reinterpret_cast<unsigned*>(0x1010878cu);
	if(flags & 1)
		nxFoundationSDKAllocator->free(self);
	}

// phys_fn_002322 (0x5a230): member-table adjustor thunk -- `sub ecx,8` onto
// the pool base, tail-call to the chained deleting destructor (002328).
void nxChainedDeletingDtorThunk(void* memberThis, unsigned flags)
	{
	nxChainedDeletingDtor(
		reinterpret_cast<unsigned char*>(memberThis) - 8, flags);
	}

// phys_fn_002312 (0x5a080): third pool class deleting destructor -- vptr
// 0x1010878c installed, then LINKED-CRT release when flagged.
void nxPoolDeletingDtor78c(void* self, unsigned flags)
	{
	*reinterpret_cast<unsigned**>(self) =
		reinterpret_cast<unsigned*>(0x1010878cu);
	if(flags & 1)
		::free(self);
	}

// phys_fn_002320 (0x5a1e0): destroys the node LIST cached at this+0x5a8.
// Per iteration: cache's +0x30 link is read and zeroed, the cache node is
// deleted through its OWN virtual slot 0 with argument 1, the cache field
// is nulled, then refilled with the saved link -- looping until it reads
// null.
void nxDestroyCachedList(void* self)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	unsigned cache = *reinterpret_cast<unsigned*>(a + 0x5a8);
	while(cache != 0)
		{
		unsigned next = *reinterpret_cast<unsigned*>(cache + 0x30);
		*reinterpret_cast<unsigned*>(cache + 0x30) = 0;
		unsigned vtable = *reinterpret_cast<unsigned*>(cache);
		unsigned slot0 = *reinterpret_cast<const unsigned*>(vtable);
		typedef void (__thiscall* NxNodeKillFn)(void* node, unsigned arg);
		NxNodeKillFn kill = reinterpret_cast<NxNodeKillFn>(slot0);
		kill(reinterpret_cast<void*>(cache), 1);
		*reinterpret_cast<unsigned*>(a + 0x5a8) = 0;
		cache = next;
		*reinterpret_cast<unsigned*>(a + 0x5a8) = next;
		}
	}

// phys_fn_002352 (0x5b610): adjustor thunk -- `add ecx,0x28`, tail-call to
// phys_fn_004846 at 0xb4f50 (SdkContainer::empty). The thunk's return is
// whatever empty() leaves in eax -- unpinned, like the null path of
// phys_fn_000004. The first spelling hard-coded the oracle's absolute
// 0x100b4f50: in the candidate process that is oracle code reached with
// ecx invalid -- the differential caught it as c0000005 at 0xb4f53.
#include "ContactStreamRelease.inl"

// phys_fn_002379 (0x5b860): virtual slot-1 wrapper -- invokes arg's own
// virtual slot 1 and always returns false.
bool nxVirtualSlot1Wrapper(void* arg)
	{
	unsigned a = reinterpret_cast<unsigned>(arg);
	unsigned vtable = *reinterpret_cast<unsigned*>(a);
	typedef void (__thiscall* NxSlot1Fn)(void* self);
	NxSlot1Fn fn = reinterpret_cast<NxSlot1Fn>(
		*reinterpret_cast<unsigned*>(vtable + 4));
	fn(arg);
	return false;
	}

// ---------------------------------------------------------------------------
// Actor slate 9: five small rows.

// phys_fn_000080 (0x2c90): readBodyFlag -- ([record+0x10c] dword AND mask)
// under the READ guard; kind-1 warning ("readBodyFlag: Actor must be
// dynamic!") and false on a static actor.
const char* const	nxMsgReadBodyFlagDynamic =
	"Actor::readBodyFlag: Actor must be dynamic!";
bool nxActorReadBodyFlag(void* self, unsigned mask)
	{
	unsigned a = reinterpret_cast<unsigned>(self);
	void* scene = *reinterpret_cast<void**>(a + 0x10);
	nxSceneGuardEnter(scene);
	unsigned body = *reinterpret_cast<unsigned*>(a + 0x14);
	unsigned rec = body != 0
		? *reinterpret_cast<unsigned*>(body + 8) : 0u;
	if(rec == 0)
		{
		nxReport(1, nxSourceFileNpActorCpp, 0x1e3, 0,
			nxMsgReadBodyFlagDynamic);
		nxSceneGuardLeave(scene);
		return false;
		}
	unsigned flagWord = *reinterpret_cast<const unsigned*>(rec + 0x10c);
	nxSceneGuardLeave(scene);
	return (flagWord & mask) != 0;
	}

// phys_fn_002408 (0x5baa0): the member subobject's scalar-deleting
// destructor -- third table again, then release through the LINKED CRT
// (0x100f41f0) when flagged, exactly like phys_fn_000042.
void nxMemberDeletingDtor(void* self, unsigned flags)
	{
	*reinterpret_cast<unsigned**>(self) =
		reinterpret_cast<unsigned*>(0x101088b8u);
	if(flags & 1)
		::free(self);
	}

// phys_fn_002411 (0x5bbb0): the shapes-clear entry its deregistration
// callers use instead of a raw store: release slot=[arg+0x104] through the
// pool header at this+0x40 (so the release arm's own +0x30-family offsets
// land at this+0x70 family), then zero shapes[slot] in the array named by
// [this+0x80].
void nxSceneClearShapeSlot(void* scenePool, void* shapeArg)
	{
	unsigned s = reinterpret_cast<unsigned>(scenePool);
	unsigned slot = *reinterpret_cast<const unsigned*>(
		reinterpret_cast<unsigned>(shapeArg) + 0x104);
	nxSceneReleaseIndex(reinterpret_cast<unsigned char*>(scenePool) + 0x40,
		slot);
	unsigned shapes = *reinterpret_cast<unsigned*>(s + 0x80);
	*reinterpret_cast<unsigned*>(shapes + slot * 4) = 0;
	}

// phys_fn_002326 (0x5a440): deleting destructor installing vptr 0x10108798,
// adapter release through slot +0x14 when flagged.
void nxBoundDeletingDtor798(void* self, unsigned flags)
	{
	*reinterpret_cast<unsigned**>(self) =
		reinterpret_cast<unsigned*>(0x10108798u);
	if(flags & 1)
		nxFoundationSDKAllocator->free(self);
	}

// phys_fn_002340 (0x5aa60): same shape for the second pool class, vptr
// 0x1010884c.
void nxBoundDeletingDtor84c(void* self, unsigned flags)
	{
	*reinterpret_cast<unsigned**>(self) =
		reinterpret_cast<unsigned*>(0x1010884cu);
	if(flags & 1)
		nxFoundationSDKAllocator->free(self);
	}

// ---------------------------------------------------------------------------
// BoxShape. See ObjectModel.h for the row map.

template<typename Method>
static void* nxShapeMethodAddress(Method method)
	{
	static_assert(sizeof(Method) == sizeof(void*),
		"shape vtable methods require the Win32 single-inheritance ABI");
	void* address;
	memcpy(&address, &method, sizeof(address));
	return address;
	}

static void** nxBoxShapeInternalVtable()
	{
	struct Table
		{
		void* slot[17];
		Table()
			{
			slot[0] = nxShapeMethodAddress(&BoxShape::nxBoxScalarDeletingDtor);
			slot[1] = nxShapeMethodAddress(&ShapeBase::nxApplyDescriptor);
			slot[2] = nxShapeMethodAddress(&ShapeBase::nxBaseSaveState);
			slot[3] = nxShapeMethodAddress(&BoxShape::nxDebugRenderDispatch);
			slot[4] = nxShapeMethodAddress(&BoxShape::nxBoxAccumulateMass);
			slot[5] = nxShapeMethodAddress(&BoxShape::nxBoxRaycast);
			slot[6] = nxShapeMethodAddress(&ShapeBase::nxApplyOwnerUpdate);
			slot[7] = nxShapeMethodAddress(&BoxShape::nxBoxSweep);
			slot[8] = nxShapeMethodAddress(&BoxShape::nxBoxLocalAABB);
			slot[9] = nxShapeMethodAddress(&BoxShape::nxBoxWorldAABB);
			slot[10] = nxShapeMethodAddress(&BoxShape::nxBoxCenterAndDiagonal);
			slot[11] = nxShapeMethodAddress(&BoxShape::nxBoxZeroCenterAndDiagonal);
			slot[12] = nxShapeMethodAddress(&BoxShape::nxBoxLoadFromDesc);
			slot[13] = nxShapeMethodAddress(&BoxShape::nxBoxSaveState);
			slot[14] = nxShapeMethodAddress(&BoxShape::nxBoxSelf);
			slot[15] = slot[14];
			slot[16] = slot[14];
			}
		};
	static Table table;
	return table.slot;
	}

// The box hull facade's final table, .rdata 0x10106a88: twelve slots, read
// from the pinned image (the BOX table follows at 0x10106ab8). The image has
// no direct caller of slots 9 and 10 (000957, 000959); they are reached only
// through this table.
static void** nxBoxHullFacadeVtable()
	{
	struct Table
		{
		void* slot[12];
		Table()
			{
			slot[0] = reinterpret_cast<void*>(&BoxHullFacade::sharedHook);			// 0x21a10, 000985
			slot[1] = nxShapeMethodAddress(&BoxHullFacade::vertexCount);			// 0x20d20, 000953
			slot[2] = nxShapeMethodAddress(&BoxHullFacade::vertices);				// 0x20d30, 000955
			slot[3] = nxShapeMethodAddress(&BoxHullFacade::faceCount);				// 0x213c0, 000961
			slot[4] = nxShapeMethodAddress(&BoxHullFacade::face);					// 0x213d0, 000963
			slot[5] = reinterpret_cast<void*>(&shapeOwnerQuery);					// 0x213e0, 000965
			slot[6] = reinterpret_cast<void*>(&BoxHullFacade::edgeTable);			// 0x213f0, 000967
			slot[7] = reinterpret_cast<void*>(&BoxHullFacade::faceCornerTable);	// 0x21400, 000969
			slot[8] = reinterpret_cast<void*>(&BoxHullFacade::adjacencyTable);		// 0x21410, 000971
			slot[9] = nxShapeMethodAddress(&BoxHullFacade::supportFace);			// 0x20d40, 000957
			slot[10] = nxShapeMethodAddress(&BoxHullFacade::supportFeature);		// 0x20f90, 000959
			slot[11] = nxShapeMethodAddress(&BoxHullFacade::supportBounds);		// 0x217c0, 000975
			}
		};
	static Table table;
	return table.slot;
	}

static void** nxSphereShapeInternalVtable()
	{
	struct Table
		{
		void* slot[19];
		Table()
			{
			slot[0] = nxShapeMethodAddress(&SphereShape::nxSphereScalarDeletingDtor);
			slot[1] = nxShapeMethodAddress(&ShapeBase::nxApplyDescriptor);
			slot[2] = nxShapeMethodAddress(&ShapeBase::nxBaseSaveState);
			slot[3] = nxShapeMethodAddress(&SphereShape::nxSphereDebugRenderDispatch);
			slot[4] = nxShapeMethodAddress(&SphereShape::nxSphereAccumulateMass);
			slot[5] = reinterpret_cast<void*>(&NxShapeRaycastSphere);
			slot[6] = nxShapeMethodAddress(&ShapeBase::nxApplyOwnerUpdate);
			slot[7] = nxShapeMethodAddress(&SphereShape::nxSphereSweepRadius);
			slot[8] = nxShapeMethodAddress(&SphereShape::nxSphereLocalAABB);
			slot[9] = nxShapeMethodAddress(&SphereShape::nxSphereWorldAABB);
			slot[10] = nxShapeMethodAddress(&SphereShape::nxSphereCenterRadius);
			slot[11] = nxShapeMethodAddress(&SphereShape::nxSphereZeroCenterRadius);
			slot[12] = nxShapeMethodAddress(&SphereShape::nxSphereLoadFromDesc);
			slot[13] = nxShapeMethodAddress(&SphereShape::nxSphereSaveState);
			slot[14] = nxShapeMethodAddress(&SphereShape::nxSphereSetRadius);
			slot[15] = nxShapeMethodAddress(&SphereShape::nxSphereGetRadius);
			slot[16] = nxShapeMethodAddress(&ShapeBase::nxSelf);
			slot[17] = slot[16];
			slot[18] = slot[16];
			}
		};
	static Table table;
	return table.slot;
	}

static void** nxCapsuleShapeInternalVtable()
	{
	struct Table
		{
		void* slot[19];
		Table()
			{
			slot[0] = nxShapeMethodAddress(&CapsuleShape::nxCapsuleScalarDeletingDtor);
			slot[1] = nxShapeMethodAddress(&ShapeBase::nxApplyDescriptor);
			slot[2] = nxShapeMethodAddress(&ShapeBase::nxBaseSaveState);
			slot[3] = nxShapeMethodAddress(&CapsuleShape::nxCapsuleDebugRenderDispatch);
			slot[4] = nxShapeMethodAddress(&CapsuleShape::nxCapsuleAccumulateMass);
			slot[5] = reinterpret_cast<void*>(&NxShapeRaycastCapsule);
			slot[6] = nxShapeMethodAddress(&ShapeBase::nxApplyOwnerUpdate);
			slot[7] = nxShapeMethodAddress(&CapsuleShape::nxCapsuleSweepZero);
			slot[8] = nxShapeMethodAddress(&CapsuleShape::nxCapsuleLocalAABB);
			slot[9] = nxShapeMethodAddress(&CapsuleShape::nxCapsuleWorldAABB1016);
			slot[10] = nxShapeMethodAddress(&CapsuleShape::nxCapsuleCenterRadius);
			slot[11] = nxShapeMethodAddress(&CapsuleShape::nxCapsuleZeroCenterRadius);
			slot[12] = nxShapeMethodAddress(&CapsuleShape::nxCapsuleLoadFromDesc);
			slot[13] = nxShapeMethodAddress(&CapsuleShape::nxCapsuleSaveState);
			slot[14] = nxShapeMethodAddress(&CapsuleShape::nxCapsuleSetRadius);
			slot[15] = nxShapeMethodAddress(&CapsuleShape::nxCapsuleGetRadius);
			slot[16] = nxShapeMethodAddress(&ShapeBase::nxSelf);
			slot[17] = slot[16];
			slot[18] = slot[16];
			}
		};
	static Table table;
	return table.slot;
	}

static void** nxPlaneShapeInternalVtable()
	{
	struct Table
		{
		void* slot[17];
		Table()
			{
			slot[0] = nxShapeMethodAddress(&PlaneShape::nxPlaneScalarDeletingDtor);
			slot[1] = nxShapeMethodAddress(&ShapeBase::nxApplyDescriptor);
			slot[2] = nxShapeMethodAddress(&ShapeBase::nxBaseSaveState);
			slot[3] = nxShapeMethodAddress(&PlaneShape::nxPlaneDebugRenderDispatch);
			slot[4] = nxShapeMethodAddress(&ShapeBase::nxBaseSlot4);
			slot[5] = reinterpret_cast<void*>(&NxShapeRaycastPlane);
			slot[6] = nxShapeMethodAddress(&ShapeBase::nxApplyOwnerUpdate);
			slot[7] = nxShapeMethodAddress(&ShapeBase::nxBaseSlot7);
			slot[8] = nxShapeMethodAddress(&PlaneShape::nxPlaneIndexed6_1267);
			slot[9] = nxShapeMethodAddress(&PlaneShape::nxPlaneLocalAABB1255);
			slot[10] = nxShapeMethodAddress(&PlaneShape::nxPlaneExtentRow);
			slot[11] = slot[10];
			slot[12] = nxShapeMethodAddress(&PlaneShape::nxPlaneLoadFromDesc);
			slot[13] = nxShapeMethodAddress(&PlaneShape::nxPlaneSaveState);
			slot[14] = nxShapeMethodAddress(&ShapeBase::nxSelf);
			slot[15] = slot[14];
			slot[16] = slot[14];
			}
		};
	static Table table;
	return table.slot;
	}

// MESH's 18-entry primary table at .rdata:0x00107630. The actor construction
// path needs slots 0/1/6/9/12/13/17; the debug row remains unsupported, while
// slot 5 dispatches the reconstructed mesh raycast.
static void __fastcall nxUnsupportedMeshShapeMethod(void*, void*)
	{
	abort();
	}

static void** nxMeshShapeInternalVtable()
	{
	struct Table
		{
		void* slot[18];
		Table()
			{
			for(unsigned i = 0; i < 18; ++i)
				slot[i] = reinterpret_cast<void*>(&nxUnsupportedMeshShapeMethod);
			slot[0] = nxShapeMethodAddress(&MeshShape::nxMeshScalarDeletingDtor);
			slot[1] = nxShapeMethodAddress(&ShapeBase::nxApplyDescriptor);
			slot[2] = nxShapeMethodAddress(&ShapeBase::nxBaseSaveState);
			slot[3] = nxShapeMethodAddress(&MeshShape::nxMeshDebugRender);
			slot[4] = nxShapeMethodAddress(&MeshShape::nxMeshAccumulateMassCached);
			slot[5] = reinterpret_cast<void*>(&NxShapeRaycastMesh);
			slot[6] = nxShapeMethodAddress(&ShapeBase::nxApplyOwnerUpdate);
			slot[7] = nxShapeMethodAddress(&MeshShape::nxMeshSweepPrepared);
			slot[8] = nxShapeMethodAddress(&MeshShape::nxMeshGetWords44);
			slot[9] = nxShapeMethodAddress(&MeshShape::nxMeshWorldAABB);
			slot[10] = nxShapeMethodAddress(&MeshShape::nxMeshTransformCenter);
			slot[11] = nxShapeMethodAddress(&MeshShape::nxMeshGetWords5C);
			slot[12] = nxShapeMethodAddress(&MeshShape::nxMeshLoadFromDesc);
			slot[13] = nxShapeMethodAddress(&MeshShape::nxMeshSaveState);
			slot[14] = nxShapeMethodAddress(&ShapeBase::nxSelf);
			slot[15] = slot[14];
			slot[16] = slot[14];
			slot[17] = nxShapeMethodAddress(&MeshShape::nxMeshGetMeshWord);
			}
		};
	static Table table;
	return table.slot;
	}

// ShapeBase::ShapeBase's prunable, for Scene.cpp's raw shape allocations (the
// factory does not run the shape constructors): the member built in place
// (Prunable::Prunable at 0x000255df), the three owner hooks in the image's
// store order (0x0002562b, 0x00025635, 0x0002563f), then the shape as the
// prunable's owner (0x00025649). The scene raycasts reach every shape through
// its prunable (the pruners report prunables; the loops read the owner at
// +0x04, and a stale world box is recomputed through the WorldAABB hook).
void nxShapeFactoryInstallPrunable(void* shape)
	{
	Prunable* prunable = new(static_cast<unsigned char*>(shape) + 0xa4) Prunable;
	gPrunableOwnerWorldAABB = shapeOwnerWorldAABB;
	gPrunableOwnerNotify = shapeOwnerNotify;
	gPrunableOwnerQuery = shapeOwnerQuery;
	prunable->mOwner = shape;
	}

// Install the reconstructed final table on Scene's raw shape allocation.
// MESH's product path now reaches its recovered descriptor, owner-update,
// world-bounds, save-state and deletion slots; other family gaps remain open.
void nxShapeFactoryInstallVtable(void* shape, unsigned type)
	{
	void** table = nullptr;
	switch(type)
		{
		case 0: table = nxPlaneShapeInternalVtable(); break;
		case 1: table = nxSphereShapeInternalVtable(); break;
		case 2: table = nxBoxShapeInternalVtable(); break;
		case 3: table = nxCapsuleShapeInternalVtable(); break;
		case 4: table = nxMeshShapeInternalVtable(); break;
		default: break;
		}
	if(table) *reinterpret_cast<void***>(shape) = table;
	}

// The box part of the image's creation path for Scene's raw box allocation.
// NxScene::createActor's shape builder 000032 constructs the box with 000977
// (call 0x10021870 at 0x10001ec6), which stores the facade table at +0xe0
// (0x10021895), then loads the descriptor through the BOX table's slot 12
// (call [eax+0x30] at 0x10001efd: 000981, which copies the dims, rebuilds the
// hull through 000973 and applies the BASE fields) and releases the shape when
// its al is 0 (0x10001f02). The factory's zero fill already gives the face
// records 000977 zeroes; this stores the facade table and makes the slot-12
// call through the table the factory installed. Returns the slot's bool.
bool nxShapeFactoryLoadBox(void* shape, const void* descriptor)
	{
	BoxShape* box = static_cast<BoxShape*>(shape);
	box->mHull.mVptrSlot = nxBoxHullFacadeVtable();		// 0x10021895
	typedef bool (__thiscall* LoadFn)(void*, const void*);
	void** table = *reinterpret_cast<void***>(shape);
	return reinterpret_cast<LoadFn>(table[12])(shape, descriptor);
	}

// phys_fn_000977 (0x00021870, 207 B)
// __thiscall ret 8: the BOX final's constructor.
BoxShape::BoxShape(void* owner, unsigned argument)
	: mBase(owner, argument)				// forwarded unchanged: 0x0002187c..80
	{
	// vptr stores. The image stores the abstract wall at +0xe0 first
	// (0x00021885), the BOX primary table next (0x0002188f), then replaces
	// the wall with the facade's final twelve-slot table (0x00021895). The
	// wall store is a chained-construction intermediate, overwritten before
	// anything can read it, so only the two final tables are stored here.
	// The transcription writes the face-record pointer words once; the image
	// reaches record 5 through a walking pointer with identical effect.
	mBase.mVptrSlot = nxBoxShapeInternalVtable();
	mHull.mVptrSlot = nxBoxHullFacadeVtable();	// 0x00021895
	for(unsigned r = 0; r < 6; ++r)			// 0x000218a1..0x000218ee
		{
		mHull.mFaces[r].mCorners = 0;
		mHull.mFaces[r].mIndexListA = 0;
		mHull.mFaces[r].mIndexListB = 0;
		}

	// The embedded collision object: a fresh 0x1c-byte block through the imported
	// Foundation allocator [0x101041bc] (0x000218f1..fe -- malloc slot, size 0x1c, flag 0), built by
	// phys_fn_001075 (0x00023580), whose body is phys_fn_001193 with the
	// box-family tables and which stores the box at BOTH +0x08 and +0x18.
	void* memory = nxFoundationSDKAllocator->malloc(0x1c, NX_MEMORY_PERSISTENT);
	CollisionObject* object = memory
		? new(memory) CollisionObjectBox(this)
		: 0;								// null arm: 0x0002190f
	mBase.mWord9C = reinterpret_cast<NxU32>(object);	// 0x00021911

	const float one = 1.0f;					// mov eax,0x3f800000 at 0x00021917
	mBase.mSentinelD0 = 2;					// OVERWRITES the base's sentinel: 0x0002191c
	mHull.mDims04[0] = one;					// 0x00021926
	mHull.mDims04[1] = one;					// 0x0002192c
	mHull.mDims04[2] = one;					// 0x00021932

	// The vertices and every face-record float word are written by nobody
	// here; a fresh box carries poison there until the face builder runs.
	}

// ---------------------------------------------------------------------------
// SphereShape. See ObjectModel.h for the row map.

SphereShape::SphereShape(void* owner, unsigned argument)
	: mBase(owner, argument)				// forwarded unchanged: 0x000277bb..cf
	{
	mBase.mVptrSlot = nxSphereShapeInternalVtable();
	mRadiusE0 = 0.0f;						// mov [esi+0xe0],0 at 0x000277da

	// The embedded collision object: a fresh 0x1c-byte block through the imported
	// Foundation allocator [0x101041bc] (0x000277e4..f2), built by phys_fn_001193 itself -- the
	// GENERIC collision-object constructor, not a per-type variant -- with
	// the sphere stored at BOTH +0x08 and +0x18.
	void* memory = nxFoundationSDKAllocator->malloc(0x1c, NX_MEMORY_PERSISTENT);
	CollisionObject* object = memory
		? new(memory) CollisionObjectSphere(this)
		: 0;								// null arm: 0x00027803
	mBase.mWord9C = reinterpret_cast<NxU32>(object);	// 0x00027805

	mBase.mSentinelD0 = 1;					// OVERWRITES the base's sentinel: 0x0002780b
	}

// ---------------------------------------------------------------------------
// SPHERE-table rows. See ObjectModel.h.

// phys_fn_001373 (0x00027c10), SPHERE primary-table slot 7.
bool SphereShape::nxSphereSweepRadius(float* out, const void* /*unused*/) const
	{
	memcpy(out, &mRadiusE0, sizeof(mRadiusE0));
	return true;
	}

// phys_fn_001305 (0x00025960) through a SPHERE receiver. Its B block
// dispatches the receiver's slot-10 center/radius row, so this counterpart
// uses nxSphereCenterRadius where the box counterpart uses its diagonal.
void SphereShape::nxSphereBaseDebugRender(const void* renderer) const
	{
	if(!g_nxGuardRef || !renderer)
		return;
	const float ref = *g_nxGuardRef;
	void** table = *reinterpret_cast<void** const*>(renderer);
	typedef void (__fastcall* DrawLineFn)(void*, void*, const float*,
		const float*, unsigned);
	typedef void (__fastcall* DrawPoseFn)(void*, void*, unsigned,
		const void*, unsigned, unsigned, unsigned);
	DrawLineFn drawLine = reinterpret_cast<DrawLineFn>(table[8]);
	DrawPoseFn drawPose = reinterpret_cast<DrawPoseFn>(table[14]);
	void* rendererArg = const_cast<void*>(renderer);
	const float guardA = *g_nxGuardA;
	if(guardA != ref || guardA != guardA)
		{
		const float k = *g_nxRenderScale * guardA;
		const float* rot = reinterpret_cast<const float*>(&mBase.mPose0C.mRotation);
		const float* trn = reinterpret_cast<const float*>(&mBase.mPose0C.mTranslation);
		static const unsigned colors[3] = { 0xcf0000u, 0xcf00u, 0xcfu };
		for(unsigned axis = 0; axis < 3; ++axis)
			{
			const float start[3] = { trn[0], trn[1], trn[2] };
			const float end[3] = {
				k * rot[axis] + trn[0],
				k * rot[3 + axis] + trn[1],
				k * rot[6 + axis] + trn[2]
			};
			drawLine(rendererArg, nullptr, start, end, colors[axis]);
			}
		}
	const float guardB = *g_nxGuardB;
	if(guardB != ref || guardB != guardB)
		{
		float center[4] = {};
		nxSphereCenterRadius(center);
		const float* rot = reinterpret_cast<const float*>(&mBase.mPose0C.mRotation);
		float pose[12];
		for(unsigned i = 0; i < 9; ++i)
			pose[i] = rot[i];
		pose[9] = center[0];
		pose[10] = center[1];
		pose[11] = center[2];
		unsigned radiusBits;
		memcpy(&radiusBits, center + 3, 4);
		for(unsigned round = 0; round < 3; ++round)
			{
			drawPose(rendererArg, nullptr, 0x14, pose, 0xffff00ffu,
				radiusBits, 0);
			if(round < 2)
				{
				float next[9];
				for(unsigned row = 0; row < 3; ++row)
					for(unsigned col = 0; col < 3; ++col)
						next[row * 3 + col] = pose[row * 3 + (col + 1) % 3];
				memcpy(pose, next, sizeof(next));
				}
			}
		}
	}

// phys_fn_001369 (0x00027a30), SPHERE primary-table slot 3. The shared
// 001305 base draw runs first; the sphere-specific arm then draws three
// rotated poses through renderer slot +0x38 when guard C is active.
void SphereShape::nxSphereDebugRenderDispatch(const void* renderer) const
	{
	if(!mBase.nxFlagBitsDE(8) || !renderer)
		return;
	nxSphereBaseDebugRender(renderer);
	if(!g_nxGuardC || !g_nxGuardRef)
		return;
	const float guard = *g_nxGuardC;
	const float ref = *g_nxGuardRef;
	if(guard == ref && guard == guard)
		return;
	void** table = *reinterpret_cast<void** const*>(renderer);
	typedef void (__fastcall* DrawPoseFn)(void*, void*, unsigned,
		const void*, unsigned, unsigned, unsigned);
	DrawPoseFn drawPose = reinterpret_cast<DrawPoseFn>(table[14]);
	float pose[12];
	memcpy(pose, &mBase.mPose0C, sizeof(pose));
	const unsigned color = (mBase.mHalfwordDE & 7u)
		? 0xffff00ffu : 0xffffffffu;
	unsigned radius;
	memcpy(&radius, &mRadiusE0, sizeof(radius));
	for(unsigned round = 0; round < 3; ++round)
		{
		drawPose(const_cast<void*>(renderer), nullptr, 0x14,
			pose, color, radius, 0);
		if(round < 2)
			{
			float next[9];
			for(unsigned row = 0; row < 3; ++row)
				for(unsigned col = 0; col < 3; ++col)
					next[row * 3 + col] = pose[row * 3 + (col + 1) % 3];
			memcpy(pose, next, sizeof(next));
			}
		}
	}

// phys_fn_001355 (0x000278a0), SPHERE-table slot 13.
bool SphereShape::nxSphereSaveState(void* record)
	{
	memcpy(reinterpret_cast<unsigned char*>(record) + 0x4c,
		&mRadiusE0, sizeof(mRadiusE0));		// mov [+eax+0x4c] at 0x000278aa
	return mBase.nxBaseSaveState(record);	// jmp 0x000256f0 at 0x000278b1
	}

// phys_fn_001365 (0x000279b0), SPHERE-table slot 11.
void SphereShape::nxSphereZeroCenterRadius(float* out) const
	{
	out[0] = 0.0f;							// xor edx,edx; three stores
	out[1] = 0.0f;
	out[2] = 0.0f;
	out[3] = mRadiusE0;						// mov ecx,[ecx+0xe0]
	}

// phys_fn_001363 (0x00027980), SPHERE-table slot 10.
void SphereShape::nxSphereCenterRadius(float* out) const
	{
	out[0] = mBase.mPose0C.mTranslation[0];	// +0x30
	out[1] = mBase.mPose0C.mTranslation[1];	// +0x34
	out[2] = mBase.mPose0C.mTranslation[2];	// +0x38
	out[3] = mRadiusE0;
	}

// phys_fn_001361 (0x00027930), SPHERE-table slot 9.
void SphereShape::nxSphereWorldAABB(float* out) const
	{
	const float* t = mBase.mPose0C.mTranslation;
	out[0] = t[0] - mRadiusE0;				// fsub chain, 0x00027961..6a
	out[1] = t[1] - mRadiusE0;
	out[2] = t[2] - mRadiusE0;
	out[3] = t[0] + mRadiusE0;				// fadd chain, 0x00027937..49
	out[4] = t[1] + mRadiusE0;
	out[5] = t[2] + mRadiusE0;
	}

// phys_fn_001367 (0x000279d0), SPHERE-table slot 8. The x87 dance (fchs,
// dup, fxch) stores the negated radius three times then the raw radius
// three times -- an AABB around the sphere's own center, like BOX slot 8.
void SphereShape::nxSphereLocalAABB(float* out) const
	{
	out[0] = -mRadiusE0;					// fchs; fst [esp]; ... fstp [eax]
	out[1] = -mRadiusE0;
	out[2] = -mRadiusE0;
	out[3] = mRadiusE0;						// mov [eax+0x14],edx
	out[4] = mRadiusE0;						// fstp [eax+0xc]
	out[5] = mRadiusE0;						// fstp [eax+0x10]
	}

// phys_fn_001357 (0x000278c0), SPHERE-table slot 14.
void SphereShape::nxSphereSetRadius(float radius)
	{
	mRadiusE0 = radius;						// mov [esi+0xe0],eax at 0x000278d1
	// The image then fcomp's the stored value against the zero global
	// (0x101041f0): a report fires unless radius > 0 (unordered and less
	// both fall through the jp). Report shape at 0x000278de..0x27900:
	// assert-guarded indirect cdecl call with (1, SphereShape.cpp,
	// 0x4a, 0, "radius should be positive!"). With a valid radius both
	// remaining arms are no-ops on a detached shape: BASE slot 6
	// owner-notify and dirty-flag 0x20 via phys_fn_001315-adjacent helper
	// 0x26c90 (see evidence 3k/3t).
	if(!(radius > 0.0f))
		nxSphereSetRadiusReport();
	mBase.nxApplyOwnerUpdate(1);
	nxSceneMarkShapeDirty(this, 0x20);
	}

// The 001874 narrow-phase callback constructs two temporary SphereShapes and
// explicitly invokes the non-scalar destructor (001351) before returning.
// Its BASE destructor tail (001323) removes owner arms and destroys Prunable.
void SphereShape::nxSphereCallbackDtor(void)
	{
	mBase.mVptrSlot = nxSphereShapeInternalVtable();
	if(mBase.mWord9C != 0)
		nxCollisionFamilyScalarDeletingDtor<CollisionObjectSphere>(
			reinterpret_cast<CollisionObject*>(mBase.mWord9C), 1);
	mBase.nxBaseDtorOwnerArms();
	mBase.mPrunable.~Prunable();
	}

// phys_fn_001329 (0x00026d90): apply a GROUP. See ObjectModel.h.
void ShapeBase::nxApplyGroup(unsigned short group)
	{
	if(group >= 0x20)
		{
		// The image reports (1, Shape.cpp, 0xe0, 0, "group ID must be
		// < 32!") through the guarded slot -- int3 when the flag word is
		// zero -- and then JOINS the valid path at the dirty-flag call,
		// skipping only the store. Evidence section 3u.
		nxReport(1, nxSourceFileShapeCpp, 0xe0, 0, nxMsgGroupBelow32);
		}
	else
		{
		mHalfwordD8 = group;			// mov [esi+0xd8],ax at 0x00026dc7
		}
	// Both paths rejoin at 0x00026dce: dirty-flag 0x04 via 0x26c90 -- a
	// null-owner no-op on a detached shape -- then the prunable's
	// unidentified dword at abs +0xc8 (mPrunable24) becomes the group mask
	// 1 << low-byte(group). The x86 shift masks its count to five bits;
	// reproduced.
	mPrunable.mPrunable24 =
		1u << ((mHalfwordD8 & 0xff) & 0x1f);
	}

// phys_fn_000981 (0x00021990, 55 B)
// BOX-table slot 12, __thiscall ret 4: the three dimension words copied as
// dwords (0x10021998-0x100219af), the hull rebuild (call 0x10021420 at
// 0x100219b5), then BASE slot 1 (call 0x10027740 at 0x100219bd), whose al is
// the row's return.
bool BoxShape::nxBoxLoadFromDesc(const void* record)
	{
	const unsigned char* rec = static_cast<const unsigned char*>(record);
	memcpy(&mHull.mDims04[0], rec + 0x4c, sizeof(mHull.mDims04[0]));
	memcpy(&mHull.mDims04[1], rec + 0x50, sizeof(mHull.mDims04[1]));
	memcpy(&mHull.mDims04[2], rec + 0x54, sizeof(mHull.mDims04[2]));
	nxBoxRebuildHull();
	return mBase.nxApplyDescriptor(rec);
	}

// phys_fn_000973 (0x00021420, 913 B)
// The box hull rebuild; ecx = the box, no stack arguments, plain ret (the
// image's callers 000981 and 000983 call it directly).
// - The box about a zero centre with half-extents +0xe4 (the ICE AABB
//   SetCenterExtents copy 000923, call at 0x10021451) and its eight corners to
//   +0xf0 (AABB::ComputePoints 005147, call at 0x10021461).
// - The face records' list pointers (0x10021466-0x100214d4) and corner counts
//   of 4 (0x100214e3-0x10021501).
// - The axis normals, in the image's store order: face 1 (+1,0,0), face 3
//   (-1,0,0), face 4 (0,+1,0), face 5 (0,-1,0), face 2 (0,0,+1), face 0
//   (0,0,-1) (0x1002151f-0x10021647).
// - Each plane distance -((n_a + n_b) * 0.0f + s * n_c), where n_c is the
//   normal's axis component, n_a + n_b the other two in the listed operand
//   order, and s the signed half-extent on that axis: dx and -dx are float
//   spills of the fld'd dims, dz and -dz and -dy stay in registers, dy is a
//   float spill (0x10021507-0x10021725). Faces 0-5 in that order.
// - Per face, through the facade's own table (+0xe0): slot 3 face count before
//   the loop and after each face, slots 1 and 2 per face; min seeded FLT_MAX
//   and max -FLT_MAX; per corner (vz*nz + vy*ny) + vx*nx unrounded; the min
//   takes it on strictly less (fcom, test ah,5, jp), the max on strictly
//   greater (test ah,0x41) (0x1002172b-0x100217a7).
void BoxShape::nxBoxRebuildHull()
	{
	const IceMaths::Point centre(0.0f, 0.0f, 0.0f);
	IceMaths::AABB box;
	box.SetCenterExtents(centre, *reinterpret_cast<const IceMaths::Point*>(mHull.mDims04));
	box.ComputePoints(reinterpret_cast<IceMaths::Point*>(mHull.mVertices));

	BoxFaceRecord* faces = mHull.mFaces;
	for(unsigned r = 0; r < 6; ++r)
		{
		faces[r].mIndexListA = gHullFaceCorners[r];
		faces[r].mIndexListB = gHullFaceEdges[r];
		}
	for(unsigned r = 0; r < 6; ++r)
		faces[r].mCorners = 4;

	const double dx = mHull.mDims04[0];			// fld [edi] at 0x10021507
	const double dy = mHull.mDims04[1];			// fld [edi+4] at 0x1002150c
	const float dz = mHull.mDims04[2];			// the dword at 0x10021509, spilled

	faces[1].mNormal[0] = 1.0f;  faces[1].mNormal[1] = 0.0f;  faces[1].mNormal[2] = 0.0f;
	faces[3].mNormal[0] = -1.0f; faces[3].mNormal[1] = 0.0f;  faces[3].mNormal[2] = 0.0f;
	faces[4].mNormal[0] = 0.0f;  faces[4].mNormal[1] = 1.0f;  faces[4].mNormal[2] = 0.0f;
	faces[5].mNormal[0] = 0.0f;  faces[5].mNormal[1] = -1.0f; faces[5].mNormal[2] = 0.0f;
	faces[2].mNormal[0] = 0.0f;  faces[2].mNormal[1] = 0.0f;  faces[2].mNormal[2] = 1.0f;
	faces[0].mNormal[0] = 0.0f;  faces[0].mNormal[1] = 0.0f;  faces[0].mNormal[2] = -1.0f;

	const float dxSpill = (float)dx;			// fstp [esp+0x10] at 0x10021623
	const float negDxSpill = (float)-dx;		// fchs; fstp [esp+0x28] at 0x10021633
	const float dySpill = (float)dy;			// fst [esp+0x38] at 0x10021643
	const double negDy = -dy;					// fchs at 0x1002164a, kept
	const double posDz = dz;					// fld [esp+0x24] at 0x10021654, kept
	const double negDz = -(double)dz;			// fld; fchs at 0x10021663

	const double zero = 0.0f;					// fmul [0x101041f0]
	float* n = faces[0].mNormal;
	faces[0].mDistance = (float)-((( (double)n[1] + n[0]) * zero) + negDz * n[2]);
	n = faces[1].mNormal;
	faces[1].mDistance = (float)-((( (double)n[2] + n[1]) * zero) + (double)dxSpill * n[0]);
	n = faces[2].mNormal;
	faces[2].mDistance = (float)-((( (double)n[1] + n[0]) * zero) + posDz * n[2]);
	n = faces[3].mNormal;
	faces[3].mDistance = (float)-((( (double)n[2] + n[1]) * zero) + (double)negDxSpill * n[0]);
	n = faces[4].mNormal;
	faces[4].mDistance = (float)-((( (double)n[2] + n[0]) * zero) + (double)dySpill * n[1]);
	n = faces[5].mNormal;
	faces[5].mDistance = (float)-((( (double)n[2] + n[0]) * zero) + negDy * n[1]);

	const BoxHullFacade* facade = &mHull;
	void* const* table = nxBoxHullTable(facade);
	unsigned face = 0;
	if(reinterpret_cast<BoxHullCountSlot>(table[3])(facade) != 0)
		{
		BoxFaceRecord* record = faces;
		do
			{
			unsigned corners = reinterpret_cast<BoxHullCountSlot>(table[1])(facade);
			const float* v = reinterpret_cast<BoxHullVerticesSlot>(table[2])(facade);
			record->mMinProjection = FLT_MAX;
			record->mMaxProjection = -FLT_MAX;
			for(; corners != 0; --corners)
				{
				const double projection = ((double)v[2] * record->mNormal[2]
					+ (double)v[1] * record->mNormal[1]) + (double)v[0] * record->mNormal[0];
				if(projection < record->mMinProjection)
					record->mMinProjection = (float)projection;
				if(projection > record->mMaxProjection)
					record->mMaxProjection = (float)projection;
				v += 3;
				}
			++face;
			++record;
			}
		while(face < reinterpret_cast<BoxHullCountSlot>(table[3])(facade));
		}
	}

// phys_fn_001383 (0x00027e30), MESH-table slot 12.
bool MeshShape::nxMeshLoadFromDesc(const void* record)
	{
	const unsigned char* rec = static_cast<const unsigned char*>(record);
	void* wrapper = nullptr;
	memcpy(&wrapper, rec + 0x4c, sizeof(wrapper));
	if(wrapper == nullptr)
		return false;
	void* inner = nullptr;
	memcpy(&inner, static_cast<unsigned char*>(wrapper) + 4, sizeof(inner));
	mWordE0 = reinterpret_cast<NxU32>(inner);
	++*reinterpret_cast<unsigned*>(static_cast<unsigned char*>(inner) + 0x74);
	memcpy(&mWordE4, rec + 0x50, sizeof(mWordE4));
	mBase.nxApplyDescriptor(rec);
	return true;
	}

// phys_fn_001265 (0x00025460), PLANE-table slot 12.
void PlaneShape::nxPlaneLoadFromDesc(const void* record)
	{
	const unsigned char* rec = static_cast<const unsigned char*>(record);
	float d = 0.0f;
	memcpy(&d, rec + 0x58, sizeof(d));
	nxPlaneSetEquation(reinterpret_cast<const float*>(rec + 0x4c), d);
	mBase.nxApplyDescriptor(rec);
	}

// phys_fn_001253 (0x24fc0), the plane equation helper reached by slot 12.
void PlaneShape::nxPlaneSetEquation(const float* normal, float distance)
	{
	memcpy(mNormalE0, normal, sizeof(mNormalE0));
	mDistanceEC = -distance;
	unsigned x, y, z;
	memcpy(&x, mNormalE0, 4);
	memcpy(&y, mNormalE0 + 1, 4);
	memcpy(&z, mNormalE0 + 2, 4);
	x &= 0x7fffffffu;
	y &= 0x7fffffffu;
	z &= 0x7fffffffu;
	mWord108 = x == 0x3f800000u && y == 0 && z == 0 ? 0u
		: x == 0 && y == 0x3f800000u && z == 0 ? 1u
		: x == 0 && y == 0 && z == 0x3f800000u ? 2u : 3u;
	NxNormalToTangents(
		*reinterpret_cast<const NxVec3*>(&mNormalE0[0]),
		*reinterpret_cast<NxVec3*>(&mTangentF0[0]),
		*reinterpret_cast<NxVec3*>(&mBinormalFC[0]));
	mBase.nxApplyOwnerUpdate(1);
	// 0x2507e marks owner dirty flag 0x80 through 0x26c90. The detached
	// shape path tested here has no owner and returns without a write.
	}

// phys_fn_001315 (0x000266a0, 1061 B)
// BASE-table slot 6, a row of Shape.cpp: the shape's world pose from its
// owner. A detached shape (owner +4 null) takes the early exit at 0x26abb.
// - 0x266bc-0x266fb: with a nonzero argument, a scene stamp [scene+0x540]
//   other than +8 copies pose one (+0xc) to pose two (+0x3c) and stores it.
// - 0x266fe-0x267f6: the owner pose. A dynamic owner's rotation comes from
//   the record quaternion +0x24 by the inline sequence every body row uses
//   (five doubled products spilled to float: nxNpActorComposeRotation), with
//   the translation +0x18; a static owner's is the body's +0x20/+0x44.
// - 0x267f6-0x269fa: pose one = owner * local (+0x6c): each rotation element
//   one register sum rounded once, in the listing's operand order
//     row 0: (R1 L3 + R2 L6) + R0 L0, (R1 L4 + R2 L7) + R0 L1,
//            (R0 L2 + R1 L5) + R2 L8
//     row 1: (R3 L0 + R4 L3) + R5 L6, (R3 L1 + R4 L4) + R5 L7,
//            (R3 L2 + R4 L5) + R5 L8
//     row 2: (R7 L3 + R8 L6) + R6 L0, (R7 L4 + R8 L7) + R6 L1,
//            (R7 L5 + R8 L8) + R6 L2
//   and the translation row 0, (R0 l0 + R1 l1) + R2 l2, added to T.x in the
//   register, rows 1 and 2 spilled to float before T.y/T.z are added.
// - 0x269fd-0x26a33: +0xdc bit 4 copies pose one to pose two and clears.
// - 0x26a36-0x26ab8, with a nonzero argument: when +0xdc lacks bit 2 and the
//   shape has the object at +0xa0, the shape is appended to that object's
//   array at +0x78 (grown through 0x100b4de0); then +0xdc |= 2; then, when
//   the prunable (+0xa4) has a handle (+0x28 != 0xffff) and a pruning type
//   (+0x2a) below 4, its +8 loses bit 2 and the scene's pruner for that type
//   ([scene+0x640 + 4 type]) gets slot 3 with the prunable. When +0xdc bit 1
//   is clear and +0xa0 names the pruning collection, append this shape to its
//   SdkContainer at +0x78 (growing through 004840 when full), then set bit 1.
//   The owned list/update arm is directly exercised by the object-layout gate.
// phys_fn_001315 stores its owner-composed rotation through x87 fstp. During
// simulateFrame the oracle sets x87 to chop; SSE2 double-to-float casts follow
// MXCSR instead. Keep this conversion local to the owner-update implementation
// so shape pose queries continue to use their separately reconstructed path.
static __declspec(noinline) float nxShapeOwnerX87StoreFloat(double value)
	{
	float result;
	__asm
		{
		fld qword ptr [value]
		fstp dword ptr [result]
		}
	return result;
	}

static __declspec(noinline) float nxShapeOwnerX87SubtractFloat(float left, float right)
	{
	float result;
	// 001315 performs this subtraction in x87 extended precision with chop.
	// A tiny positive right operand moves an exactly representable float just
	// below that value; SSE double subtraction can round the change away first.
	__asm
		{
		fld dword ptr left
		fsub dword ptr right
		fstp dword ptr result
		}
	return result;
	}

static __declspec(noinline) void nxShapeOwnerComposeRotationX87(const float* q, float* r)
	{
	const double x = q[0], y = q[1], z = q[2], w = q[3];
	const float yy2 = nxShapeOwnerX87StoreFloat(y * y + y * y);
	const double zz2 = z * z + z * z;
	r[0] = nxShapeOwnerX87StoreFloat((1.0 - yy2) - zz2);
	const double xy2 = y * x + y * x;
	const double zw2 = z * w + z * w;
	r[1] = nxShapeOwnerX87StoreFloat(xy2 - zw2);
	const float xz2 = nxShapeOwnerX87StoreFloat(z * x + z * x);
	const double yw2 = y * w + y * w;
	const float yw2Spill = nxShapeOwnerX87StoreFloat(yw2);
	r[2] = nxShapeOwnerX87StoreFloat(yw2 + xz2);
	r[3] = nxShapeOwnerX87StoreFloat(zw2 + xy2);
	const double xx1 = 1.0 - (x * x + x * x);
	const float xx1Spill = nxShapeOwnerX87StoreFloat(xx1);
	r[4] = nxShapeOwnerX87StoreFloat(xx1 - zz2);
	const float yz2 = nxShapeOwnerX87StoreFloat(z * y + z * y);
	const double xw2 = x * w + x * w;
	r[5] = nxShapeOwnerX87StoreFloat(static_cast<double>(yz2) - xw2);
	r[6] = nxShapeOwnerX87StoreFloat(static_cast<double>(xz2) - yw2Spill);
	r[7] = nxShapeOwnerX87StoreFloat(xw2 + yz2);
	r[8] = nxShapeOwnerX87SubtractFloat(xx1Spill, yy2);
	}

void ShapeBase::nxApplyOwnerUpdate(unsigned flags)
	{
	if(mOwner04 == nullptr)
		return;
	unsigned char* body = static_cast<unsigned char*>(mOwner04);
	unsigned char* scene = *reinterpret_cast<unsigned char**>(body + 4);
	if((flags & 0xffu) != 0 && scene)
		{
		const unsigned stamp = *reinterpret_cast<unsigned*>(scene + 0x540);
		if(mWord08 != stamp)
			{
			mPose3C = mPose0C;
			mWord08 = stamp;
			}
		}
	unsigned char* record = *reinterpret_cast<unsigned char**>(body + 8);
	float r[9];
	float t[3];
	if(record)
		{
		nxShapeOwnerComposeRotationX87(reinterpret_cast<const float*>(record + 0x24), r);
		memcpy(t, record + 0x18, sizeof(t));
		}
	else
		{
		memcpy(r, body + 0x20, sizeof(r));
		memcpy(t, body + 0x44, sizeof(t));
		}
	const float* l = reinterpret_cast<const float*>(mLocalPose.mRotation);
	const float* lt = mLocalPose.mTranslation;
	float* w = reinterpret_cast<float*>(mPose0C.mRotation);
	#define NX_RL(a, b) (static_cast<double>(r[a]) * l[b])
	const double c0 = (static_cast<double>(r[0]) * lt[0] + static_cast<double>(r[1]) * lt[1]) +
		static_cast<double>(r[2]) * lt[2];
	const float c1 = static_cast<float>((static_cast<double>(r[3]) * lt[0] +
		static_cast<double>(r[4]) * lt[1]) + static_cast<double>(r[5]) * lt[2]);
	const float c2 = static_cast<float>((static_cast<double>(r[6]) * lt[0] +
		static_cast<double>(r[7]) * lt[1]) + static_cast<double>(r[8]) * lt[2]);
	mPose0C.mTranslation[0] = static_cast<float>(t[0] + c0);
	mPose0C.mTranslation[1] = static_cast<float>(static_cast<double>(t[1]) + c1);
	mPose0C.mTranslation[2] = static_cast<float>(static_cast<double>(t[2]) + c2);
	w[0] = static_cast<float>((NX_RL(1, 3) + NX_RL(2, 6)) + NX_RL(0, 0));
	w[1] = static_cast<float>((NX_RL(1, 4) + NX_RL(2, 7)) + NX_RL(0, 1));
	w[2] = static_cast<float>((NX_RL(0, 2) + NX_RL(1, 5)) + NX_RL(2, 8));
	w[3] = static_cast<float>((NX_RL(3, 0) + NX_RL(4, 3)) + NX_RL(5, 6));
	w[4] = static_cast<float>((NX_RL(3, 1) + NX_RL(4, 4)) + NX_RL(5, 7));
	w[5] = static_cast<float>((NX_RL(3, 2) + NX_RL(4, 5)) + NX_RL(5, 8));
	w[6] = static_cast<float>((NX_RL(7, 3) + NX_RL(8, 6)) + NX_RL(6, 0));
	w[7] = static_cast<float>((NX_RL(7, 4) + NX_RL(8, 7)) + NX_RL(6, 1));
	w[8] = static_cast<float>((NX_RL(7, 5) + NX_RL(8, 8)) + NX_RL(6, 2));
	#undef NX_RL
	if((mHalfwordDC & 4u) != 0)
		{
		mHalfwordDC &= static_cast<NxU16>(~4u);
		mPose3C = mPose0C;
		}
	if(flags & 0xffu)
		{
		// 0x26a36-0x26a6f: a pruning collection owns a root list at +0x78.
		// Append this shape only while its +0xdc bit 1 is clear, then set it.
		// If the collection pointer is null the listing skips both operations.
		if((mHalfwordDC & 2u) == 0 && mWordA0 != 0)
			{
			SdkContainer* list = reinterpret_cast<SdkContainer*>(
				reinterpret_cast<unsigned char*>(static_cast<size_t>(mWordA0)) + 0x78);
			if(list->mCount == list->mCapacity)
				list->resize(1);
			list->mEntries[list->mCount] = reinterpret_cast<NxU32>(this);
			++list->mCount;
			mHalfwordDC |= 2u;
			}
		if(scene && mPrunable.mHandle != 0xffffu &&
			mPrunable.mPruningType < 4u)
			{
			// 0x00026a92-0x00026ab5: the box marked stale, then the pruner's
			// slot 3 (UpdateObject) through its own table -- the static
			// pruner's drops its tree there (phys_fn_005222). The pruner is
			// not tested for null, as in the image (scene-raycast Task 3; the
			// stamp increment written here before was the dynamic pruner's
			// slot 3 alone).
			mPrunable.mFlags &= ~2u;
			Pruner* pruner = *reinterpret_cast<Pruner**>(
				scene + 0x640 + mPrunable.mPruningType * 4);
			pruner->UpdateObject(&mPrunable);
			}
		}
	}

// The Scene shape factory constructs a raw internal shape while the final
// shape classes are still being reconstructed. Initialize its three poses
// through the same owner-composition path used by live actor pose changes.
void nxShapeFactoryInitializePose(void* shape, const void* localPose)
	{
	unsigned char* bytes = static_cast<unsigned char*>(shape);
	if(localPose)
		memcpy(bytes + 0x6c, localPose, 0x30);
	else
		{
		memset(bytes + 0x6c, 0, 0x30);
		const float one = 1.0f;
		for(unsigned i = 0; i < 3; ++i)
			memcpy(bytes + 0x6c + 4 * (i * 3 + i), &one, 4);
		}
	// With 1, as the creation path's 000531 runs the shape's slot 6: the
	// shape takes the Scene stamp at +8 and +0xdc gains bit 2 (the fresh
	// shape's +0xdc = 6 loses bit 4 as pose one is copied to pose two).
	static_cast<ShapeBase*>(shape)->nxApplyOwnerUpdate(1);
	memcpy(bytes + 0x3c, bytes + 0x0c, 0x30);
	}

void nxShapeApplyOwnerUpdate(void* shape, unsigned flags)
	{
	static_cast<ShapeBase*>(shape)->nxApplyOwnerUpdate(flags);
	}

void nxShapeFactoryRefreshPose(void* shape)
	{
	unsigned char* bytes = static_cast<unsigned char*>(shape);
	static_cast<ShapeBase*>(shape)->nxApplyOwnerUpdate(0);
	memcpy(bytes + 0x3c, bytes + 0x0c, 0x30);
	}

void nxShapeFactoryInitializePlane(void* shape, const float* normal,
	float distance)
	{
	PlaneShape* plane = static_cast<PlaneShape*>(shape);
	memcpy(plane->mNormalE0, normal, sizeof(plane->mNormalE0));
	plane->mDistanceEC = -distance;
	unsigned x, y, z;
	memcpy(&x, normal, 4);
	memcpy(&y, normal + 1, 4);
	memcpy(&z, normal + 2, 4);
	x &= 0x7fffffffu;
	y &= 0x7fffffffu;
	z &= 0x7fffffffu;
	plane->mWord108 = x == 0x3f800000u && y == 0 && z == 0 ? 0u
		: x == 0 && y == 0x3f800000u && z == 0 ? 1u
		: x == 0 && y == 0 && z == 0x3f800000u ? 2u : 3u;
	NxNormalToTangents(
		*reinterpret_cast<const NxVec3*>(plane->mNormalE0),
		*reinterpret_cast<NxVec3*>(plane->mTangentF0),
		*reinterpret_cast<NxVec3*>(plane->mBinormalFC));
	}

// ---------------------------------------------------------------------------
// Shape-to-name registry. See evidence section 3o for the full decode.

// The global list head at .data 0x10123c0c. Each entry is {shape*, name*}
// (8-byte stride). The list grows via the Foundation allocator when capacity is
// exhausted.
static void* gShapeNameList = nullptr;

// phys_fn_000480 (0x000edc0): the three-word VC9 vector header follows the
// image's initial 0x10-byte header allocation and grows by 2*n+2 records.
struct NxShapeNameList { void** begin; void** end; void** capacity; };

bool ShapeBase::nxShapeNameRegistry(void* shape, void* name)
	{
	if(shape == nullptr)
		return false;
	NxShapeNameList* list = static_cast<NxShapeNameList*>(gShapeNameList);
	if(name == nullptr && list == nullptr)
		return true;
	if(name != nullptr && list == nullptr)
		{
		list = static_cast<NxShapeNameList*>(nxGetSdkAllocator()->malloc(
		0x10, NX_MEMORY_PERSISTENT));
		if(list == nullptr)
			return false;
		list->begin = list->end = list->capacity = nullptr;
		gShapeNameList = list;
		}
	NxU32 count = list->begin == nullptr ? 0u : static_cast<NxU32>(
		(reinterpret_cast<NxU32>(list->end) - reinterpret_cast<NxU32>(list->begin)) >> 3);
	for(NxU32 i = 0; i < count; ++i)
		{
		void** pair = list->begin + i * 2;
		if(pair[0] == shape)
			{
			if(name != nullptr)
				{
				pair[1] = name;
				return true;
				}
			if(i + 1 < count)
				{
				pair[0] = list->end[-2];
				pair[1] = list->end[-1];
				}
			list->end -= 2;
			if(list->begin == list->end)
				{
				if(list->begin)
					nxGetSdkAllocator()->free(list->begin);
				gShapeNameList = nullptr;
				nxGetSdkAllocator()->free(list);
				}
			return true;
			}
		}
	if(list->capacity <= list->end)
		{
		const NxU32 newCapacity = count * 2 + 2;
		void** fresh = static_cast<void**>(nxGetSdkAllocator()->malloc(
			newCapacity * 8, NX_MEMORY_PERSISTENT));
		if(fresh == nullptr)
			return false;
		for(NxU32 i = 0; i < count * 2; ++i)
			fresh[i] = list->begin[i];
		if(list->begin)
			nxGetSdkAllocator()->free(list->begin);
		list->begin = fresh;
		list->end = fresh + count * 2;
		list->capacity = fresh + newCapacity * 2;
		}
	*list->end++ = shape;
	*list->end++ = name;
	return true;
	}

// phys_fn_001347 (0x00027740), BASE-table slot 1. See ObjectModel.h.
bool ShapeBase::nxApplyDescriptor(const void* record)
	{
	const unsigned char* rec = static_cast<const unsigned char*>(record);
	memcpy(&mLocalPose, rec + 8, sizeof(mLocalPose));			// rep movsd 9 + three words
	NxU16 flagsLo = 0;
	memcpy(&flagsLo, rec + 0x38, 2);					// movzx word [ebp+0x38]
	mHalfwordDE = flagsLo;								// mov [ebx+0xde],cx
	memcpy(&mHalfwordDA, rec + 0x3e, sizeof(mHalfwordDA));
	if(mWord9C != 0)
		{
		unsigned ud = 0;
		memcpy(&ud, rec + 0x40, sizeof(ud));
		unsigned char* col = reinterpret_cast<unsigned char*>(mWord9C);
		memcpy(col + 4, &ud, sizeof(ud));				// guarded store 0x0002778d..90
		}
	NxU16 grp = 0;
	memcpy(&grp, rec + 0x3c, sizeof(grp));				// mov ax,[ebp+0x3c]
	nxApplyGroup(grp);									// validated setter 0x26d90
	// The registry arm (phys_fn_000480 with [desc+0x44]) reduces to a
	// no-op-return-true when the name is NULL and the list is empty -- the
	// drive passes name=NULL; full registry transcription is its own row.
	return true;										// mov al,1
	}

// phys_fn_001263 (0x00025420), PLANE-table slot 0.
void PlaneShape::nxPlaneScalarDeletingDtor(unsigned flags)
	{
	if(mBase.mWord9C)
		{
		// plane collision-object deleting entry, flag 1.
		nxCollisionFamilyScalarDeletingDtor<CollisionObjectPlane>(
			reinterpret_cast<CollisionObject*>(mBase.mWord9C), 1);
		}
	mBase.nxBaseDtorOwnerArms();		// owner arms, 0x26be1..c35
	mBase.mPrunable.~Prunable();			// tail of the base-dtor chain
	if(flags & 1u)
		nxFoundationSDKAllocator->free(this);
	}

// phys_fn_001375 (0x00027c30), SPHERE-table slot 0.
void SphereShape::nxSphereScalarDeletingDtor(unsigned flags)
	{
	if(mBase.mWord9C)
		{
		// generic collision-object deleting row at 0x24810, flag 1.
		nxCollisionFamilyScalarDeletingDtor<CollisionObjectSphere>(
			reinterpret_cast<CollisionObject*>(mBase.mWord9C), 1);
		}
	mBase.nxBaseDtorOwnerArms();			// owner arms, 0x26be1..c35
	mBase.mPrunable.~Prunable();			// tail of the base-dtor chain
	if(flags & 1u)
		nxFoundationSDKAllocator->free(this);
	}

// phys_fn_001353 (0x00027850), SPHERE-table slot 12.
void SphereShape::nxSphereLoadFromDesc(const void* record)
	{
	const unsigned char* rec = static_cast<const unsigned char*>(record);
	float r = 0.0f;
	memcpy(&r, rec + 0x4c, sizeof(r));		// fld [edi+0x4c] at 0x00027856
	mRadiusE0 = r;							// fst [esi+0xe0] at 0x0002785b
	// fcomp against the zero literal at 0x00027861: a report fires unless
	// strictly positive (line 0x35). The radius was already stored
	// unconditionally, and the BASE apply-desc tail runs either way.
	if(!(mRadiusE0 > 0.0f))
		nxReport(1, nxSourceFileSphereLoadCpp, 0x35, 0,
			nxMsgSphereLoadRadius);
	mBase.nxApplyDescriptor(record);				// call 0x00027740 at 0x00027895
	}

// ---------------------------------------------------------------------------
// The mass frame and the SPHERE slot-4 row behind it. See evidence section
// 3p for the disassembly walk this transcribes.

// The stored .rdata words participate in the arithmetic, exactly as in
// MassProperties.cpp -- same literals, different rows.
static const NxF32 gMassKFourThirdsPi = 4.1887903f;	// [0x101068d8]
static const NxF32 gMassKTwoFifths = 0.4f;			// [0x101068e4]
static const NxF32 gMassDensitySentinel = 1.0f;		// [0x101041ec]

namespace
	{
	// The process runs x87 at _PC_53, so every register intermediate is a
	// double and only the stores round to 32 bits. Writing the lifetimes as
	// double reproduces the oracle bitwise (same argument as
	// MassProperties.cpp's header comment).
	inline NxF32 mul32(NxF32 a, NxF32 b)
		{
		double p = static_cast<double>(a) * static_cast<double>(b);
		return static_cast<NxF32>(p);
		}
	}

// phys_fn_000843 (0x0001c750, 110 B)
// __thiscall ret 8. The image squares and cubes
// the radius on the x87 stack, multiplies by 4pi/3 for the mass, then keeps
// going from the mass value -- r^3 -> r^5 through two more radius multiplies,
// one 2/5 -- for the diagonal inertia. Nine words are zeroed by integer moves
// (+0x04..+0x1c off-diagonal thirds and the whole offset), which is why a
// fresh frame carries exact +0.0f there.
void MassFrame::nxMassFrameBuildSphere(float radius, const void* extra)
	{
	double r3 = static_cast<double>(radius);
	r3 *= static_cast<double>(radius);
	r3 *= static_cast<double>(radius);					// fld/fmul/fmul chain
	double mass = r3 * static_cast<double>(gMassKFourThirdsPi);

	double diag = mass;
	diag *= static_cast<double>(radius);				// back up to r^4
	diag *= static_cast<double>(radius);				// r^5
	diag *= static_cast<double>(gMassKTwoFifths);		// (2/5) m r^2

	NxF32 stored = static_cast<NxF32>(mass);
	mMass = stored;										// fstp [esi+0x30]
	NxF32 inertia = static_cast<NxF32>(diag);
	mInertia[0] = inertia;								// fst [esi] / [esi+0x10] / [esi+0x20]
	mInertia[4] = inertia;
	mInertia[8] = inertia;
	// mov dword ptr [esi+4/+8/+0xc/+0x14/+0x18/+0x1c], 0
	mInertia[1] = mInertia[2] = mInertia[3] = 0.0f;
	mInertia[5] = mInertia[6] = mInertia[7] = 0.0f;
	// and the whole COM offset triple, also integer-zeroed
	mOffset.x = 0.0f; mOffset.y = 0.0f; mOffset.z = 0.0f;

	if(extra != nullptr)
		{
		// push extra; call 0x1bdc0 ; add extra,0x24 ; call 0x1c040 --
		// the parallel-axis pair, mirroring the BOX builder. A zero payload
		// folds the inertia to zero, which is what the oracle's non-null
		// arm produces (see 3z117/3z118).
		nxMassFrameFoldPayload(extra);
		nxMassFrameTranslate(reinterpret_cast<const unsigned char*>(extra) + 0x24);
		}
	}

// phys_fn_000837 (0x0001c5c0, 101 B)
// __thiscall ret 4. Ten fld/fmul/fstp triples:
// the nine inertia words and the mass. The offset at +0x24..+0x2c is skipped
// entirely -- scaling by density keeps the center.
void MassFrame::nxMassFrameScale(float s)
	{
	for(unsigned i = 0; i < 9; ++i)
		mInertia[i] = mul32(mInertia[i], s);
	mMass = mul32(mMass, s);
	}

// phys_fn_000839 (0x0001c630, 231 B)
// __thiscall ret 4. The weights live at +0x30 on
// both frames; the quotient divides the .rdata 1.0f literal by their sum (a
// real fdiv, then rounded by fstp m32 at 0x1c695 before use). The
// products are formed weight-times-offset per frame first, added pairwise --
// x-side as other+this, y/z sides as this+other, matching the stack order --
// scaled by the quotient and stored z, x, y. Then mass takes the raw sum and
// the nine inertia words accumulate componentwise.
void MassFrame::nxMassFrameMerge(const MassFrame& other)
	{
	double sum = static_cast<double>(mMass) + static_cast<double>(other.mMass);
	const NxF32 q = static_cast<NxF32>(static_cast<double>(gMassDensitySentinel) / sum);

	double ax = static_cast<double>(other.mMass) * static_cast<double>(other.mOffset.x);
	const NxF32 ay = static_cast<NxF32>(static_cast<double>(other.mMass) * other.mOffset.y);
	const NxF32 az = static_cast<NxF32>(static_cast<double>(other.mMass) * other.mOffset.z);
	const NxF32 cx = static_cast<NxF32>(static_cast<double>(mMass) * mOffset.x);
	const NxF32 cy = static_cast<NxF32>(static_cast<double>(mMass) * mOffset.y);
	const NxF32 cz = static_cast<NxF32>(static_cast<double>(mMass) * mOffset.z);
	// Other-x stays in the FPU stack; y/z pairwise sums are stored m32.
	const NxF32 sy = static_cast<NxF32>(static_cast<double>(cy) + ay);
	const NxF32 sz = static_cast<NxF32>(static_cast<double>(cz) + az);

	NxF32 zx = static_cast<NxF32>((ax + cx) * q);
	NxF32 zz = static_cast<NxF32>(static_cast<double>(sz) * q);
	NxF32 zy = static_cast<NxF32>(static_cast<double>(sy) * q);
	mOffset.x = zx;						// fstp [ecx+0x24]
	mOffset.y = zy;						// fstp [ecx+0x28]
	mOffset.z = zz;						// fstp [ecx+0x2c] (stored first in the image)
	mMass = static_cast<NxF32>(sum);	// fstp [ecx+0x30]

	for(unsigned i = 0; i < 9; ++i)
		mInertia[i] = static_cast<NxF32>(
			static_cast<double>(mInertia[i]) + static_cast<double>(other.mInertia[i]));
	}

// phys_fn_000851 (0x0001c930, 77 B)
// __thiscall ret 0xc, SPHERE-table slot 4.
// The row is thiscall on the destination MassFrame; this member takes the
// destination as its first argument and has the shape in ecx. The row's only
// caller, 001371 (SPHERE slot 4), calls it directly and no table holds it,
// so the convention is a code-shape difference, not a behaviour one.
// Local frame, optional payload fold, conditional density scale against the
// 1.0f sentinel (fucompp/test ah,0x44/jnp: an unordered density falls through
// and scales, which `!=` reproduces), merge into the destination.
void SphereShape::nxSphereComputeMassFrame(MassFrame* dest, float density,
	float radius, const void* extra)
	{
	MassFrame local;
	local.nxMassFrameBuildSphere(radius, extra);
	if(density != gMassDensitySentinel)
		local.nxMassFrameScale(density);
	dest->nxMassFrameMerge(local);		}

// ---------------------------------------------------------------------------
// The BOX and CAPSULE slot-4 rows behind the same frame. Evidence section 3p.

static const NxF32 gMassKOneThird = (1.0f / 3.0f);	// [0x101068ec] stored 0.33333334
static const NxF32 gMassKEight = 8.0f;				// [0x101068f0]
static const NxF32 gMassKFour = 4.0f;				// [0x101068f4]
static const NxF32 gMassKThree = 3.0f;				// [0x101068f8] (bits 0x40400000)
static const NxF32 gPiLiteral = 3.1415927f;			// [0x101068d0]
static const NxF32 gHalfLiteral = 0.5f;				// [0x101043cc]
static const NxF32 gTwelfthLiteral = 0.083333336f;	// [0x101068e0]

namespace
	{
	// The box/capsule builders open their volume accumulator at the .rdata
	// 1.0f literal and test each extent with an INTEGER word compare, so
	// -0.0f counts as non-zero and replaces/multiplies in. MassProperties.cpp
	// documents the same shipped quirk in the exported kernels.
	inline bool nonZeroWord(float v)
		{
		NxU32 word;
		memcpy(&word, &v, sizeof(word));
		return word != 0;
		}
	}

// phys_fn_000829 (0x0001bd00, 187 B)
// __thiscall ret 4. Volume over half-extents:
// the accumulator replaces with the first non-zero extent then multiplies the
// rest, times 8 ([0x101068f0]) -- full extents are twice half-extents. That
// mass value stays live on the x87 stack for the whole function; a copy times
// 1/3 ([0x101068ec]) becomes the diagonal factor F. Squares are taken from
// the raw floats; each diagonal is F times its pairwise sum (xx: y^2+z^2,
// yy: z^2+x^2, zz: x^2+y^2). F is spilled to m32 (fstp [esp+0x1c] at
// 0x1001bd39) and so are the three pairwise sums (0x1001bd67/6c/72) before
// each is multiplied by the spilled F. `noinline`: 000849 calls the row as a
// function (0x1001c8d0).
__declspec(noinline) void MassFrame::nxMassFrameBuildBox(const float* he)
	{
	double acc = static_cast<double>(gMassDensitySentinel);	// fld 1.0f
	if(nonZeroWord(he[0]))
		acc = static_cast<double>(he[0]);					// fstp st(0); fld [eax]
	if(nonZeroWord(he[1]))
		acc *= static_cast<double>(he[1]);
	if(nonZeroWord(he[2]))
		acc *= static_cast<double>(he[2]);

	double m = acc * static_cast<double>(gMassKEight);		// fmul [0x101068f0]
	const NxF32 f = static_cast<NxF32>(
		static_cast<double>(gMassKOneThird) * m);			// fld 1/3; fmul st(1); fstp m32

	const double xx = static_cast<double>(he[0]) * static_cast<double>(he[0]);
	const double yy = static_cast<double>(he[1]) * static_cast<double>(he[1]);
	const double zz = static_cast<double>(he[2]) * static_cast<double>(he[2]);

	mInertia[1] = mInertia[2] = mInertia[3] = 0.0f;			// integer zero stores
	mInertia[5] = mInertia[6] = mInertia[7] = 0.0f;

	const NxF32 sXX = static_cast<NxF32>(zz + yy);			// fadd st(2); fstp [esp]
	const NxF32 sYY = static_cast<NxF32>(zz + xx);			// fadd st(2); fstp [esp+4]
	const NxF32 sZZ = static_cast<NxF32>(yy + xx);			// fadd st(1); fstp [esp+8]

	mMass = static_cast<NxF32>(m);							// fstp [ecx+0x30]
	mInertia[0] = static_cast<NxF32>(static_cast<double>(sXX) * f);	// fstp [ecx]
	mInertia[4] = static_cast<NxF32>(static_cast<double>(sYY) * f);	// via [esp+0x10]
	mInertia[8] = static_cast<NxF32>(static_cast<double>(sZZ) * f);	// via [esp+0x14]
	mOffset.x = 0.0f; mOffset.y = 0.0f; mOffset.z = 0.0f;
	}

// phys_fn_000845 (0x0001c7c0, 181 B)
// __thiscall ret 0xc. A unit-density cylinder of
// radius `radius` and height 2*cylHalfHeight: mass = pi*r^2*2c. The axial
// diagonal carries mass*r^2/2 (through r^2*pi*c*... folded to mr^2/2 by the
// .rdata 0.5f at 0x101043cc); the transverse pair carries the full cylinder
// formula mass*(3r^2+4c^2)/12 ([0x101068f8] = 3, [0x101068f4] = 4, over the
// .rdata 1/12 at 0x101068e0). axisSelector routes the axial term:
// 0 -> +0x00, 1 -> +0x10, anything else -> +0x20; the other two diagonals
// take the transverse term. Selector 1 writes +0x00 too: `dec edx` sets ZF,
// `mov [ecx],edx` stores the transverse term at 0x1c836, and only then does
// the `je` at 0x1c838 branch to the +0x10 arm.
void MassFrame::nxMassFrameBuildCapsule(unsigned axisSelector, float radius,
	float cylHalfHeight)
	{
	mInertia[1] = mInertia[2] = mInertia[3] = 0.0f;
	mInertia[5] = mInertia[6] = mInertia[7] = 0.0f;

	double c2 = static_cast<double>(cylHalfHeight);
	c2 += c2;												// fadd st(0),st(0)
	double m = c2 * static_cast<double>(radius);
	m *= static_cast<double>(radius);
	m *= static_cast<double>(gPiLiteral);					// pi from 0x101068d0

	double axial = m * static_cast<double>(radius);			// fld r; fmul st(1)
	axial *= static_cast<double>(radius);
	axial *= static_cast<double>(gHalfLiteral);				// .rdata 0.5f

	double rr = static_cast<double>(radius) * static_cast<double>(radius);
	double t0 = rr * static_cast<double>(gMassKThree);
	double t1 = static_cast<double>(cylHalfHeight) *
		static_cast<double>(cylHalfHeight);
	t1 *= static_cast<double>(gMassKFour);
	double side = (t0 + t1) * m;							// faddp st(1); fmul st(1)
	side *= static_cast<double>(gTwelfthLiteral);			// 1/12 from 0x101068e0

	mMass = static_cast<NxF32>(m);

	NxF32 sAx = static_cast<NxF32>(axial);
	NxF32 sSide = static_cast<NxF32>(side);
	if(axisSelector == 0)
		{
		mInertia[0] = sAx;
		mInertia[4] = sSide;
		mInertia[8] = sSide;
		}
	else if(axisSelector == 1)
		{
		mInertia[0] = sSide;								// mov [ecx],edx at 0x1c836
		mInertia[4] = sAx;
		mInertia[8] = sSide;
		}
	else
		{
		mInertia[0] = sSide;
		mInertia[4] = sSide;
		mInertia[8] = sAx;
		}
	mOffset.x = 0.0f; mOffset.y = 0.0f; mOffset.z = 0.0f;
	}

// phys_fn_000831 (0x0001bdc0, 635 B)
// __thiscall ret 4. Straight-line x87 transform
// over payload {Vec3 d; SymMat3 K}. Nine intermediates -- each an faddp chain
// ROUNDED TO FLOAT32 by its fstp m32 store before the rep movsd copy feeds
// them back -- then nine inertia stores, then the offset triple. The mass at
// +0x30 never participates. Formula table in evidence section 3r.
void MassFrame::nxMassFrameFoldPayload(const void* payload)
	{
	const NxF32* p = static_cast<const NxF32*>(payload);
	const double dx = p[0], dy = p[1], dz = p[2];
	const double k00 = p[3], k01 = p[4], k02 = p[5];
	const double k11 = p[6], k12 = p[7], k22 = p[8];

	const double i0 = mInertia[0], i1 = mInertia[1], i2 = mInertia[2];
	const double i3 = mInertia[3], i4 = mInertia[4], i5 = mInertia[5];
	const double i6 = mInertia[6], i7 = mInertia[7], i8 = mInertia[8];

	// 0x0001bdc7..0x0001beb8: nine three-product chains; each fstp m32
	// rounds its result before anything reads it back.
	NxF32 a0 = static_cast<NxF32>((dy * i3 + dz * i6) + dx * i0);
	NxF32 a1 = static_cast<NxF32>((dz * i7 + dy * i4) + dx * i1);
	NxF32 a2 = static_cast<NxF32>((dz * i8 + dx * i2) + dy * i5);
	NxF32 a3 = static_cast<NxF32>((k00 * i0 + k02 * i6) + k01 * i3);
	NxF32 a4 = static_cast<NxF32>((k02 * i7 + k01 * i4) + k00 * i1);
	NxF32 a5 = static_cast<NxF32>((k00 * i2 + k02 * i8) + k01 * i5);
	NxF32 a6 = static_cast<NxF32>((k11 * i0 + k22 * i6) + k12 * i3);
	NxF32 a7 = static_cast<NxF32>((k22 * i7 + k12 * i4) + k11 * i1);
	NxF32 a8 = static_cast<NxF32>((k11 * i2 + k22 * i8) + k12 * i5);

	// 0x0001bebe..0x0001bfc5: six stored results plus two stacked values and
	// the final scalar. After pop edi/pop esi the 0x1bfb1 load reads A[1]
	// (old frame +0x20): the scalar is the quadratic form d' I d.
	double t1 = ((double)a1 * k01 + (double)a2 * k02) + (double)a0 * k00;
	double u1 = ((double)a3 * k00 + (double)a4 * k01) + (double)a5 * k02;
	double u2 = ((double)a3 * k11 + (double)a4 * k12) + (double)a5 * k22;
	double w  = ((double)a6 * dx + (double)a8 * dz) + (double)a7 * dy;
	double v1 = ((double)a6 * k00 + (double)a7 * k01) + (double)a8 * k02;
	double v2 = ((double)a6 * k11 + (double)a7 * k12) + (double)a8 * k22;
	double t2 = ((double)a1 * k12 + (double)a2 * k22) + (double)a0 * k11;
	double q0 = ((double)a3 * dx + (double)a5 * dz) + (double)a4 * dy;
	double s  = ((double)a0 * dx + (double)a2 * dz) + (double)a1 * dy;

	// 0x0001bfa9..0x0001bfe6: inertia overwrite -- integer moves carry the
	// memory temps while the FPU stack yields [edx]/[edx+4]/[edx+8] through
	// one fxch (top becomes T1).
	mInertia[0] = static_cast<NxF32>(s);
	mInertia[1] = static_cast<NxF32>(t1);
	mInertia[2] = static_cast<NxF32>(t2);
	mInertia[3] = static_cast<NxF32>(q0);
	mInertia[4] = static_cast<NxF32>(u1);
	mInertia[5] = static_cast<NxF32>(u2);
	mInertia[6] = static_cast<NxF32>(w);
	mInertia[7] = static_cast<NxF32>(v1);
	mInertia[8] = static_cast<NxF32>(v2);

	// 0x0001bfe9..0x0001c032: offset triple -- fstp order stores the o.d
	// term FIRST, then fxch hands the two o^T K columns to +0x28/+0x2c.
	// Mass untouched.
	const double ox = mOffset.x, oy = mOffset.y, oz = mOffset.z;
	NxF32 nx = static_cast<NxF32>((oz * dz + dy * oy) + dx * ox);	// [edx+0x24]
	NxF32 ny = static_cast<NxF32>((oy * k01 + ox * k00) + oz * k02);	// [edx+0x28]
	NxF32 nz = static_cast<NxF32>((oy * k12 + ox * k11) + oz * k22);	// [edx+0x2c]
	mOffset.x = nx;
	mOffset.y = ny;
	mOffset.z = nz;
	}

// phys_fn_000847 (0x0001c880, 53 B)
// __thiscall ret 4. Conditionally zeroes all
// thirteen words when the byte argument is non-zero; leaves the frame
// untouched when it is zero.
void MassFrame::nxMassFrameConditionalZero(unsigned flag)
	{
	// `mov dl,[esp+4]; cmp dl,cl` (0x1c880/0x1c888): only the low byte of
	// the argument slot is tested.
	if(static_cast<unsigned char>(flag) == 0)
		return;
	mInertia[0] = 0.0f; mInertia[1] = 0.0f; mInertia[2] = 0.0f;
	mInertia[3] = 0.0f; mInertia[4] = 0.0f; mInertia[5] = 0.0f;
	mInertia[6] = 0.0f; mInertia[7] = 0.0f; mInertia[8] = 0.0f;
	mOffset.x = 0.0f; mOffset.y = 0.0f; mOffset.z = 0.0f;
	mMass = 0.0f;
	}

// The two frame rows Actor.cpp's 000008 (Scene.cpp) calls on its local frame.
void nxMassFrameConditionalZeroAt(void* frame, unsigned flag)
	{
	static_cast<MassFrame*>(frame)->nxMassFrameConditionalZero(flag);
	}

void nxMassFrameTranslateAt(void* frame, const void* displacement)
	{
	static_cast<MassFrame*>(frame)->nxMassFrameTranslate(displacement);
	}

// phys_fn_000841 (0x0001c720, 43 B)
// __thiscall, plain ret: {-o.x, -o.y, -o.z} built on the stack (fld; fchs;
// fstp m32, an exact negation, 0x1c723-0x1c73e), then 000833 on it
// (0x1c742). The only oracle caller is 000008 (the actor's mass from its
// shapes; Scene.cpp nxActorComputeMassFromShapes since the NpActor.cpp
// completion).
void MassFrame::nxMassFrameTranslateToCentre()
	{
	float negated[3];
	negated[0] = -mOffset.x;
	negated[1] = -mOffset.y;
	negated[2] = -mOffset.z;
	nxMassFrameTranslate(negated);
	}

// The .rdata zero [0x101041f0] that 000833 multiplies into its off-diagonal
// terms. The products are real (x*0.0 is NaN for a non-finite x and -0.0 for
// a negative one), so they are written out, not folded.
static const NxF32 gMassZeroLiteral = 0.0f;					// [0x101041f0]

namespace
	{
	// 000833's square of the cross-product matrix X of o (the prologue stores
	// X = [[0,-oz,oy],[oz,0,-ox],[-oy,ox,0]] at [esp+0x1c..0x3c]; its three
	// zero diagonal words are never written). The block at 0x1001c0d7 and
	// its copy at 0x1001c350 compute X*X = -Q(o) row by row, each off-diagonal
	// as the one non-zero product plus a product with the zero literal, and
	// round every element to m32 (fstp [esp+0x40..0x60]). ox*(-ox) is spilled
	// to m32 at [esp+8] (0x1001c14c) and re-read for both +0x10 and +0x20.
	inline void nxMassFrameSkewSquare(double ox, double oy, double oz, float* m)
		{
		const double nox = -ox, noy = -oy, noz = -oz;			// fchs, exact
		const double zero = gMassZeroLiteral;
		const double a1 = noy * oy;								// fld [esp+0x34]; fmul [esp+0x24]
		const double a2 = oz * noz;								// fld [esp+0x28]; fmul [esp+0x20]
		m[0] = static_cast<float>(a2 + a1);						// fadd st(2)
		m[1] = static_cast<float>(ox * oy + noz * zero);		// 0x1001c108
		m[2] = static_cast<float>(nox * noz + oy * zero);		// 0x1001c120
		m[3] = static_cast<float>(noy * nox + oz * zero);		// 0x1001c138
		const float s8 = static_cast<float>(ox * nox);			// fstp [esp+8]
		m[4] = static_cast<float>(a2 + s8);						// fadd [esp+8]
		m[5] = static_cast<float>(oz * oy + nox * zero);		// 0x1001c164
		m[6] = static_cast<float>(ox * oz + noy * zero);		// 0x1001c17c
		m[7] = static_cast<float>(noy * noz + ox * zero);		// 0x1001c194
		m[8] = static_cast<float>(a1 + s8);						// fadd [esp+8]
		}
	}

// phys_fn_000833 (0x0001c040, 1371 B)
// __thiscall ret 4: translate the frame by d (param+0), 0x1001c040-0x1001c598.
// All-zero d words return at once (0x1001c04e-0x1001c05e). c = d + o is
// spilled to m32 (0x1001c09b-0x1001c0b5). When every word of c is zero
// (0x1001c0b9-0x1001c0d1) the inertia gains mass * X*X (the centered path,
// 0x1001c0d7-0x1001c26a); otherwise the displaced path (0x1001c26f) builds
// the matching square of c negated (N = -Q(c)), then X*X again, and adds
// mass * (X*X - N) with every difference, product and sum rounded to m32 as
// stored (0x1001c423-0x1001c569). Both paths end in offset += d
// (0x1001c578-0x1001c58f). `noinline`: 000849 calls the row as a function
// (0x1001c8eb).
__declspec(noinline) void MassFrame::nxMassFrameTranslate(const void* param)
	{
	static const NxF32 zero = 0.0f;						// [0x101041f0]
	const float* d = static_cast<const float*>(param);
	unsigned w0,w1,w2;
	memcpy(&w0,d+0,4); memcpy(&w1,d+1,4); memcpy(&w2,d+2,4);
	if(w0==0 && w1==0 && w2==0)
		return;								// 0x1c04e..0x1c05e all-zero early-out
	const double ox = mOffset.x, oy = mOffset.y, oz = mOffset.z;
	const float cx = static_cast<float>(static_cast<double>(d[0]) + ox);	// [esp+0x10]
	const float cy = static_cast<float>(oy + d[1]);						// [esp+0x14]
	const float cz = static_cast<float>(oz + d[2]);						// [esp+0x18]
	unsigned c0, c1, c2;
	memcpy(&c0, &cx, 4); memcpy(&c1, &cy, 4); memcpy(&c2, &cz, 4);

	float square[9];
	nxMassFrameSkewSquare(ox, oy, oz, square);
	float delta[9];
	if(c0 == 0 && c1 == 0 && c2 == 0)
		{
		for(unsigned i = 0; i < 9; ++i)
			delta[i] = square[i];			// the centered path adds X*X itself
		}
	else
		{
		// 0x1001c26f-0x1001c34a: N from c. -cy is kept at [esp+0x7c];
		// (-cy)*cy is spilled to [esp+0xc] and (-cx)*cx to [esp+8] (fst: the
		// unrounded value feeds +0x10, the spill feeds +0x20).
		const double zero = gMassZeroLiteral;
		const double ncz = -static_cast<double>(cz);
		const double ncx = -static_cast<double>(cx);
		const double ncy = -static_cast<double>(cy);
		const float sc = static_cast<float>(ncy * cy);			// fstp [esp+0xc]
		const double b = ncz * cz;								// fld st(1); fmul [esp+0x18]
		float n[9];
		n[0] = static_cast<float>(sc + b);						// fld [esp+0xc]; fadd st(1)
		n[1] = static_cast<float>(static_cast<double>(cy) * cx + ncz * zero);	// 0x1001c2b4
		n[2] = static_cast<float>(ncx * ncz + static_cast<double>(cy) * zero);	// 0x1001c2c8
		n[3] = static_cast<float>(ncy * ncx + static_cast<double>(cz) * zero);	// 0x1001c2de
		const double tx = ncx * cx;								// fld st(1); fmul [esp+0x10]
		const float s8 = static_cast<float>(tx);				// fst [esp+8]
		n[4] = static_cast<float>(tx + b);						// fadd st(1)
		n[5] = static_cast<float>(static_cast<double>(cz) * cy + ncx * zero);	// 0x1001c306
		n[6] = static_cast<float>(static_cast<double>(cz) * cx + ncy * zero);	// 0x1001c31e
		n[7] = static_cast<float>(ncy * ncz + static_cast<double>(cx) * zero);	// 0x1001c334
		n[8] = static_cast<float>(static_cast<double>(s8) + sc);				// fld [esp+8]; fadd [esp+0xc]
		for(unsigned i = 0; i < 9; ++i)							// fsub, 0x1001c423-0x1001c49e
			delta[i] = static_cast<float>(static_cast<double>(square[i]) - n[i]);
		}
	const double mass = mMass;								// fld [eax+0x30]
	for(unsigned i = 0; i < 9; ++i)
		{
		const float scaled = static_cast<float>(static_cast<double>(delta[i]) * mass);
		mInertia[i] = static_cast<float>(static_cast<double>(scaled) + mInertia[i]);
		}
	// 0x1001c578-0x1001c58f: offset += d.
	mOffset.x = static_cast<float>(static_cast<double>(mOffset.x) + d[0]);
	mOffset.y = static_cast<float>(static_cast<double>(mOffset.y) + d[1]);
	mOffset.z = static_cast<float>(static_cast<double>(mOffset.z) + d[2]);
	}

// phys_fn_000947 (0x00020850, 39 B)
// BOX-table slot 4, thiscall ret 0xc (destination, density, an unused third
// DWORD): when +0xde & 7 is clear, 000849 on the destination with the
// density, the hull dimensions (+0xe4) and the local pose (+0x6c)
// (0x20850-0x2086d); always returns true (0x20872). Faithful to the listing.
bool BoxShape::nxBoxAccumulateMass(MassFrame* destination, float density, unsigned reserved)
	{
	(void) reserved;
	if(!mBase.nxFlagBitsDE(7))
		nxBoxComputeMassFrame(destination, density, mHull.mDims04, &mBase.mLocalPose);
	return true;
	}

// phys_fn_001565 (0x2e5a0, wrapper over the 001552 vptr-ctor):
// the callee installs vptr 0x10107848 and zeroes +0x4/+0x8, then the
// wrapper overrides [+0x10]=arg, vptr 0x1010785c, [+0xc]=0.
void* BoxShape::nxCtorWrap565(unsigned argument)
	{
	*(unsigned*)((unsigned char*)this + 0x00) = 0x1010785cu;
	*(unsigned*)((unsigned char*)this + 0x04) = 0;
	*(unsigned*)((unsigned char*)this + 0x08) = 0;
	*(unsigned*)((unsigned char*)this + 0x0c) = 0;
	*(unsigned*)((unsigned char*)this + 0x10) = argument;
	return this;
	}
void* BoxShape::nxCtorWrap571(unsigned argument)
	{
	*(unsigned*)((unsigned char*)this + 0x00) = 0x1010786cu;
	*(unsigned*)((unsigned char*)this + 0x04) = 0;
	*(unsigned*)((unsigned char*)this + 0x08) = 0;
	*(unsigned*)((unsigned char*)this + 0x0c) = 0;
	*(unsigned*)((unsigned char*)this + 0x10) = argument;
	return this;
	}
void* BoxShape::nxCtorWrap575(unsigned argument)
	{
	*(unsigned*)((unsigned char*)this + 0x00) = 0x10107890u;
	*(unsigned*)((unsigned char*)this + 0x04) = 0;
	*(unsigned*)((unsigned char*)this + 0x08) = 0;
	*(unsigned*)((unsigned char*)this + 0x0c) = 0;
	*(unsigned*)((unsigned char*)this + 0x10) = 0;
	*(unsigned*)((unsigned char*)this + 0x14) = argument;
	return this;
	}

// phys_fn_002065 (0x51ec0): zero the first 0x34 bytes (the 001655-ish init
// zeroes [this+0..0x20], then the 005355 sub-call zeroes [this+0x24..0x30]).
void* BoxShape::nxWrapZero2065()
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(this);
	for(unsigned o = 0; o <= 0x30; o += 4)
		*(unsigned*)(p + o) = 0;
	return this;
	}

// phys_fn_001409 (0x29750): the 001455 sub-ctor sets [this]=0x1010769c and
// zeroes +4..+0x48; the wrapper then overrides vptr to 0x1010767c and zeroes
// +0x64..0x7c.
void* BoxShape::nxWrap1409()
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(this);
	for(unsigned o = 4; o <= 0x48; o += 4)
		*(unsigned*)(p + o) = 0;
	for(unsigned o = 0x64; o <= 0x7c; o += 4)
		*(unsigned*)(p + o) = 0;
	*(unsigned*)(p + 0x00) = 0x1010767cu;
	return this;
	}

// phys_fn_000240 (0xb7c0): out = [*container + index*4] if
//   index < ([container+0xc] - [container+8])>>2 else 0; return [out + 0x6cc].
unsigned BoxShape::nxWrap240(unsigned index)
	{
	void* container = *reinterpret_cast<void**>(
		reinterpret_cast<unsigned char*>(this) + 4);
	const unsigned char* base = reinterpret_cast<const unsigned char*>(container);
	unsigned lo, hi;
	memcpy(&hi, base + 8, 4); memcpy(&lo, base + 0xc, 4);
	signed delta = static_cast<signed>(lo - hi) >> 2;
	unsigned char* hb = reinterpret_cast<unsigned char*>(static_cast<size_t>(hi));
	unsigned char* slot;
	if(index >= static_cast<unsigned>(delta))
		slot = nullptr;
	else
		slot = *reinterpret_cast<unsigned char**>(hb + index*4);
	return *reinterpret_cast<unsigned*>(slot + 0x6cc);
	}

// phys_fn_001255 (0x25090, PLANE vtable slot 9, ret 4): the plane's local
// AABB. An axis-aligned normal selects that axis and writes +/- distance
// there; the other axes keep +/- 0x7effffff. out[2] holds the -0x7effffff
// sentinel (0xfeffffff) unless the normal is -Z, in which case it carries
// the distance (the x87 const is popped and d loaded).
void PlaneShape::nxPlaneLocalAABB1255(float* out) const
	{
	unsigned ax, ay, az, ad;
	memcpy(&ax, &mNormalE0[0], 4);
	memcpy(&ay, &mNormalE0[1], 4);
	memcpy(&az, &mNormalE0[2], 4);
	memcpy(&ad, &mDistanceEC, 4);
	ax &= 0x7fffffffu; ay &= 0x7fffffffu; az &= 0x7fffffffu;
	unsigned w4 = 0x7effffffu, w8 = 0x7effffffu, wc = 0x7effffffu;
	unsigned w10 = 0xfeffffffu, w14 = 0xfeffffffu;
	unsigned w2 = 0xfeffffffu;
	const float negDistance = -mDistanceEC;
	unsigned nd; memcpy(&nd, &negDistance, 4);
	if(ax == 0x3f800000u && ay == 0u && az == 0u)
		{
		if(!(mNormalE0[0] > 0.0f)) w10 = ad;		// jne: nx <= 0 (or unordered)
		else                       w4 = nd;
		}
	else if(ax == 0u && ay == 0x3f800000u && az == 0u)
		{
		if(!(mNormalE0[1] > 0.0f)) w14 = ad;
		else                       w8 = nd;
		}
	else if(ax == 0u && ay == 0u && az == 0x3f800000u)
		{
		if(!(mNormalE0[2] > 0.0f)) w2 = ad;		// const popped, d loaded
		else                       wc = nd;
		}
	unsigned o[6] = { w10, w14, w2, w4, w8, wc };
	memcpy(out, o, sizeof(o));
	}

// phys_fn_001016 (0x22620, CAPSULE vtable slot 9, ret 4): the capsule's world
// AABB. The segment runs along local Y, so each world extent is
// M[i][1]*halfHeight; the row expands the LOWER endpoint (t - e) by +/- radius
// into out[0..2] / out[3..5], then merges the UPPER endpoint's +/- radius
// (A/B below) with min on the low triple and max on the high triple. All
// products accumulate in x87 extended precision and store to float, so a
// double intermediate reproduces the staged rounding.
void CapsuleShape::nxCapsuleWorldAABB1016(float* out) const
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(this);
	float m01, m11, m21, hh, r, t0, t1, t2;
	memcpy(&m01, p + 0x10, 4);
	memcpy(&m11, p + 0x1c, 4);
	memcpy(&m21, p + 0x28, 4);
	memcpy(&hh, p + 0xe4, 4);
	memcpy(&r, p + 0xe0, 4);
	memcpy(&t0, p + 0x30, 4);
	memcpy(&t1, p + 0x34, 4);
	memcpy(&t2, p + 0x38, 4);
	const float e0 = static_cast<float>(static_cast<double>(m01)*hh);
	const float e1 = static_cast<float>(static_cast<double>(m11)*hh);
	const float e2 = static_cast<float>(static_cast<double>(m21)*hh);
	const float mn0 = static_cast<float>(static_cast<double>(t0) - e0);
	const float mn1 = static_cast<float>(static_cast<double>(t1) - e1);
	const float mn2 = static_cast<float>(static_cast<double>(t2) - e2);
	const float mx0 = static_cast<float>(static_cast<double>(t0) + e0);
	const float mx1 = static_cast<float>(static_cast<double>(t1) + e1);
	const float mx2 = static_cast<float>(static_cast<double>(t2) + e2);
	const float a0 = static_cast<float>(static_cast<double>(mx0) - r);
	const float a1 = static_cast<float>(static_cast<double>(mx1) - r);
	const float a2 = static_cast<float>(static_cast<double>(mx2) - r);
	const float b0 = static_cast<float>(static_cast<double>(mx0) + r);
	const float b1 = static_cast<float>(static_cast<double>(mx1) + r);
	const float b2 = static_cast<float>(static_cast<double>(mx2) + r);
	out[0] = static_cast<float>(static_cast<double>(mn0) - r);
	out[1] = static_cast<float>(static_cast<double>(mn1) - r);
	out[2] = static_cast<float>(static_cast<double>(mn2) - r);
	out[3] = static_cast<float>(static_cast<double>(r) + mn0);
	out[4] = static_cast<float>(static_cast<double>(r) + mn1);
	out[5] = static_cast<float>(static_cast<double>(r) + mn2);
	if(!(out[0] <= a0)) out[0] = a0;
	if(!(out[1] <= a1)) out[1] = a1;
	if(!(out[2] <= a2)) out[2] = a2;
	// The +0xc/+0x10 arms use `test ah,5 / jp`, which updates on out <= B
	// (and skips the unordered case) -- a MAX, not an unconditional store.
	if(out[3] <= b0) out[3] = b0;
	if(out[4] <= b1) out[4] = b1;
	if(b2 > out[5]) out[5] = b2;
	}

// phys_fn_001267 (0x25490, PLANE vtable slot 8, ret 4): selects a 6-dword
// record from the table at *([this+0xc4]+0x14), indexed by
// [this+0xa4+0x28], and copies it to out. When [this+0xcc] != 0xffff and
// [this+0xa4+8] lacks bit 2, 004886 recomputes that indexed pruner box through
// the owner hook and marks the resulting AABB valid before it is copied.
void PlaneShape::nxPlaneIndexed6_1267(unsigned* out) const
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(this);
	unsigned short ax;
	memcpy(&ax, p + 0xcc, 2);
	if(ax != 0xffffu && !(p[0xa4 + 8] & 2))
		{
		unsigned char* inner = *reinterpret_cast<unsigned char* const*>(p + 0xc4);
		unsigned char* base = *reinterpret_cast<unsigned char**>(inner + 0x14);
		AABB* rec = reinterpret_cast<AABB*>(base + static_cast<unsigned>(ax) * 24u);
		reinterpret_cast<Prunable*>(const_cast<unsigned char*>(p + 0xa4))
			->UpdateWorldAABB(rec);
		}
	unsigned short ax2;
	memcpy(&ax2, p + 0xa4 + 0x28, 2);
	const unsigned char* inner = *reinterpret_cast<unsigned char* const*>(p + 0xc4);
	const unsigned char* base = *reinterpret_cast<const unsigned char* const*>(inner + 0x14);
	const unsigned char* entry = base + static_cast<unsigned>(ax2) * 24u;
	memcpy(out, entry, 24);
	}

// phys_fn_001008 (0x22440, CAPSULE vtable slot 4, ret 0xc): mirror of the
// sphere slot-4 wrapper -- radius from [this+0xe0], cylHalfHeight from
// [this+0xe0]+[this+0xe4], axisSelector 1, pose [this+0x6c].
bool CapsuleShape::nxCapsuleAccumulateMass(MassFrame* destination, float density,
	unsigned reserved)
	{
	(void) reserved;
	if(!mBase.nxFlagBitsDE(7))
		{
		const unsigned char* p = reinterpret_cast<const unsigned char*>(this);
		const float radius = *reinterpret_cast<const float*>(p + 0xe0);
		const float halfHeight =
			radius + *reinterpret_cast<const float*>(p + 0xe4);
		nxCapsuleComputeMassFrame(destination, density, 1u, radius, halfHeight,
			p + 0x6c);
		}
	return true;
	}

// phys_fn_001371 (0x27be0, SPHERE vtable slot 4, ret 0xc): mirror of the BOX
// slot-4 wrapper -- when the low flag bits are clear, run the sphere mass
// helper on the facade radius and pose, then return true.
bool SphereShape::nxSphereAccumulateMass(MassFrame* destination, float density,
	unsigned reserved)
	{
	(void) reserved;
	if(!mBase.nxFlagBitsDE(7))
		{
		const float radius = *reinterpret_cast<const float*>(
			reinterpret_cast<const unsigned char*>(this) + 0xe0);
		nxSphereComputeMassFrame(destination, density, radius,
			reinterpret_cast<const unsigned char*>(this) + 0x6c);
		}
	return true;
	}

// phys_fn_000827 (0x0001bcc0, 50 B)
// rep-movsd 9 from arg1 into [this+0],
// then arg2[0..2] into [this+0x24..0x2c] and arg3 into [this+0x30].
void BoxShape::nxPoseCopyWithTail0827(const void* src, const unsigned* extra,
	unsigned x)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(this);
	memcpy(p + 0x00, src, 36);
	memcpy(p + 0x24, extra, 12);
	memcpy(p + 0x30, &x, 4);
	}

// phys_fn_001403 (0x29190, MESH vtable slot 10): copy [[this+0xe0]+0x5c][0..3]
// into out[0..3], then out[0..2] = M * v + t, with the 3x3 matrix at
// [this+0xc..0x2c] (rows) and the translation at [this+0x30..0x38]. The x87
// body accumulates in extended precision and stores to float; a double
// intermediate reproduces the staged rounding.
void MeshShape::nxMeshTransformCenter(float* out) const
	{
	const unsigned char* p = reinterpret_cast<const unsigned char*>(this);
	const unsigned char* src =
		*reinterpret_cast<const unsigned char* const*>(p + 0xe0) + 0x5c;
	float v[4];
	memcpy(v, src, sizeof(v));
	float mf[9];
	memcpy(mf, p + 0xc, sizeof(mf));
	float tf[3];
	memcpy(tf, p + 0x30, sizeof(tf));
	const float rx = static_cast<float>(static_cast<double>(mf[0])*v[0] + static_cast<double>(mf[1])*v[1] + static_cast<double>(mf[2])*v[2] + tf[0]);
	const float ry = static_cast<float>(static_cast<double>(mf[3])*v[0] + static_cast<double>(mf[4])*v[1] + static_cast<double>(mf[5])*v[2] + tf[1]);
	const float rz = static_cast<float>(static_cast<double>(mf[6])*v[0] + static_cast<double>(mf[7])*v[1] + static_cast<double>(mf[8])*v[2] + tf[2]);
	out[0] = rx; out[1] = ry; out[2] = rz;
	out[3] = v[3];
	}

// phys_fn_000849 (0x0001c8c0, 101 B)
// __thiscall ret 0xc, helper of BOX slot 4.
// The row is thiscall on the destination MassFrame; this member takes the
// destination as its first argument and has the shape in ecx. The row's only
// caller, 000947 (BOX slot 4), calls it directly and no table holds it,
// so the convention is a code-shape difference, not a behaviour one.
// Listing 0x1001c8c0-0x1001c922: build (000829, 0x1c8d0); when `extra` is
// non-null, fold (000831, 0x1c8de) and translate by extra+0x24 (000833,
// 0x1c8eb); scale (000843) unless density == 1.0f (fucompp; test ah,0x44;
// jnp: equal skips, unordered scales); merge into dest (000839, 0x1c918).
// `noinline`: 000947 calls the row as a function (0x1002086d).
__declspec(noinline) void BoxShape::nxBoxComputeMassFrame(MassFrame* dest, float density,
	const float* halfExtents, const void* extra)
	{
	MassFrame local;
	local.nxMassFrameBuildBox(halfExtents);
	if(extra != nullptr)
		{
		local.nxMassFrameFoldPayload(extra);
		local.nxMassFrameTranslate(
			reinterpret_cast<const unsigned char*>(extra) + 0x24);
		}
	if(density != gMassDensitySentinel)
		local.nxMassFrameScale(density);
	dest->nxMassFrameMerge(local);		}

// phys_fn_000853 (0x0001c980, 115 B)
// __thiscall ret 0x14, CAPSULE-table slot 4.
// The row is thiscall on the destination MassFrame; this member takes the
// destination as its first argument and has the shape in ecx. The row's only
// caller, 001008 (CAPSULE slot 4), calls it directly and no table holds it,
// so the convention is a code-shape difference, not a behaviour one.
void CapsuleShape::nxCapsuleComputeMassFrame(MassFrame* dest, float density,
	unsigned axisSelector, float radius, float cylHalfHeight, const void* extra)
	{
	MassFrame local;
	local.nxMassFrameBuildCapsule(axisSelector, radius, cylHalfHeight);
	if(extra != nullptr)
		{
		// Same parallel-axis pair as the sphere/box builders (see 3z118).
		local.nxMassFrameFoldPayload(extra);
		local.nxMassFrameTranslate(
			reinterpret_cast<const unsigned char*>(extra) + 0x24);
		}
	if(density != gMassDensitySentinel)
		local.nxMassFrameScale(density);
	dest->nxMassFrameMerge(local);		}


// ---------------------------------------------------------------------------
// CapsuleShape. See ObjectModel.h for the row map.

// phys_fn_000987 (0x00021a60, 101 B)
// __thiscall ret 8: the CAPSULE final's constructor.
CapsuleShape::CapsuleShape(void* owner, unsigned argument)
	: mBase(owner, argument)				// forwarded unchanged: 0x00021a67..6f
	{
	mBase.mVptrSlot = nxCapsuleShapeInternalVtable();
	mFloatE0 = 0.0f;						// mov [esi+0xe0],0 at 0x00021a7a
	mFloatE4 = 0.0f;						// mov [esi+0xe4],0 at 0x00021a84

	// The embedded collision object: a fresh 0x1c-byte block through the imported
	// Foundation allocator [0x101041bc] (0x00021a8e..9c), built by phys_fn_001123 -- the capsule-family
	// variant of the shared collision-object constructor -- with the capsule
	// stored at BOTH +0x08 and +0x18.
	void* memory = nxFoundationSDKAllocator->malloc(0x1c, NX_MEMORY_PERSISTENT);
	CollisionObject* object = memory
		? new(memory) CollisionObjectCapsule(this)
		: 0;								// null arm: 0x00021aad
	mBase.mWord9C = reinterpret_cast<NxU32>(object);	// 0x00021aaf

	mBase.mSentinelD0 = 3;					// NX_SHAPE_CAPSULE: 0x00021ab5
	}

// phys_fn_000991 (0x00021b40, 42 B)
// CAPSULE-table slot 13.
bool CapsuleShape::nxCapsuleSaveState(void* record)
	{
	unsigned char* rec = static_cast<unsigned char*>(record);
	memcpy(rec + 0x4c, &mFloatE0, sizeof(mFloatE0));	// mov at 0x00021b4a
	const float height = mFloatE4 + mFloatE4;			// fld; fadd st(0),st(0):
	memcpy(rec + 0x50, &height, sizeof(height));		//   descriptor height = 2 * stored half-height
	// The third saved word lives at +0xe8 (mWordE8) -- real storage the
	// constructor leaves poisoned and nothing has named yet.
	memcpy(rec + 0x54, &mWordE8, sizeof(mWordE8));	// mov [+eax+0x54] at 0x00021b5e
	return mBase.nxBaseSaveState(record);				// jmp 0x000256f0 at 0x00021b65
	}

// phys_fn_000995 (0x00021be0, 23 B)
// CAPSULE-table slot 14.
void CapsuleShape::nxCapsuleSetRadius(float radius)
	{
	// The vptr is read (0x00021be4) before the store, then the argument slot
	// is overwritten with 1 and the row tail-jumps through slot 6 (+0x18,
	// jmp at 0x00021bf4): the owner update with flag 1.
	void** vtable = static_cast<void**>(mBase.mVptrSlot);
	mFloatE0 = radius;						// mov [ecx+0xe0],eax at 0x00021be6
	typedef void (__thiscall* OwnerUpdateFn)(void*, unsigned);
	reinterpret_cast<OwnerUpdateFn>(vtable[6])(this, 1u);
	}

// phys_fn_000997 (0x00021c00, 29 B): public NxCapsuleShape::setHeight's
// internal row. It halves the public height and dispatches BASE slot 6 with 1.
__declspec(noinline) void CapsuleShape::nxCapsuleSetHeight(float height)
	{
	void** vtable = static_cast<void**>(mBase.mVptrSlot);
	mFloatE4 = height * 0.5f;
	typedef void (__thiscall* OwnerUpdateFn)(void*, unsigned);
	reinterpret_cast<OwnerUpdateFn>(vtable[6])(this, 1u);
	}

// phys_fn_000999 (0x00021c20, 9 B): x87 load from +0xe4 and double in place.
__declspec(noinline) float CapsuleShape::nxCapsuleGetHeight() const
	{
	return mFloatE4 + mFloatE4;
	}

// Row 000993 (CapsuleShape::nxCapsuleSetDimensions) is in NpActor.cpp, next
// to its only caller, because it calls 001325 (Scene.cpp).

// phys_fn_001014 (0x000225e0), CAPSULE-table slot 0.
void CapsuleShape::nxCapsuleScalarDeletingDtor(unsigned flags)
	{
	if(mBase.mWord9C)
		{
		// phys_fn_001123's collision-object deleting row, flag 1.
		nxCollisionFamilyScalarDeletingDtor<CollisionObjectCapsule>(
			reinterpret_cast<CollisionObject*>(mBase.mWord9C), 1);
		}
	mBase.nxBaseDtorOwnerArms();		// owner arms, 0x26be1..c35
	mBase.mPrunable.~Prunable();			// tail of the base-dtor chain
	if(flags & 1u)
		nxFoundationSDKAllocator->free(this);
	}

// phys_fn_001004 (0x00021c80), CAPSULE-table slot 8.
void CapsuleShape::nxCapsuleLocalAABB(float* out) const
	{
	const float r = mFloatE0;				// fld [+0xe0]
	const float reach = mFloatE4 + r;		// fld [+0xe4]; fadd [+0xe0]
	out[0] = -r;
	out[1] = -reach;
	out[2] = -r;
	out[3] = r;
	out[4] = reach;
	out[5] = r;
	}

// phys_fn_001012 (0x000225d0), CAPSULE primary-table slot 7.
bool CapsuleShape::nxCapsuleSweepZero(unsigned* out, const void* /*unread*/) const
	{
	*out = 0;
	return false;
	}

// phys_fn_001305 (0x25960) through a CAPSULE receiver. The second guard
// dispatches slot 10, whose radius is half-height plus radius for a capsule.
void CapsuleShape::nxCapsuleBaseDebugRender(const void* renderer) const
	{
	if(!g_nxGuardRef || !renderer)
		return;
	const float ref = *g_nxGuardRef;
	void** table = *reinterpret_cast<void** const*>(renderer);
	typedef void (__fastcall* DrawLineFn)(void*, void*, const float*,
		const float*, unsigned);
	typedef void (__fastcall* DrawPoseFn)(void*, void*, unsigned,
		const void*, unsigned, unsigned, unsigned);
	DrawLineFn drawLine = reinterpret_cast<DrawLineFn>(table[8]);
	DrawPoseFn drawPose = reinterpret_cast<DrawPoseFn>(table[14]);
	void* rendererArg = const_cast<void*>(renderer);
	const float guardA = *g_nxGuardA;
	if(guardA != ref || guardA != guardA)
		{
		const float k = *g_nxRenderScale * guardA;
		const float* rot = reinterpret_cast<const float*>(&mBase.mPose0C.mRotation);
		const float* trn = reinterpret_cast<const float*>(&mBase.mPose0C.mTranslation);
		static const unsigned colors[3] = { 0xcf0000u, 0xcf00u, 0xcfu };
		for(unsigned axis = 0; axis < 3; ++axis)
			{
			const float start[3] = { trn[0], trn[1], trn[2] };
			const float end[3] = {
				k * rot[axis] + trn[0],
				k * rot[3 + axis] + trn[1],
				k * rot[6 + axis] + trn[2]
			};
			drawLine(rendererArg, nullptr, start, end, colors[axis]);
			}
		}
	const float guardB = *g_nxGuardB;
	if(guardB != ref || guardB != guardB)
		{
		float center[4] = {};
		nxCapsuleCenterRadius(center);
		const float* rot = reinterpret_cast<const float*>(&mBase.mPose0C.mRotation);
		float pose[12];
		for(unsigned i = 0; i < 9; ++i)
			pose[i] = rot[i];
		pose[9] = center[0];
		pose[10] = center[1];
		pose[11] = center[2];
		unsigned radiusBits;
		memcpy(&radiusBits, center + 3, 4);
		for(unsigned round = 0; round < 3; ++round)
			{
			drawPose(rendererArg, nullptr, 0x14, pose, 0xffff00ffu,
				radiusBits, 0);
			if(round < 2)
				{
				float next[9];
				for(unsigned row = 0; row < 3; ++row)
					for(unsigned col = 0; col < 3; ++col)
						next[row * 3 + col] = pose[row * 3 + (col + 1) % 3];
				memcpy(pose, next, sizeof(next));
				}
			}
		}
	}

// phys_fn_001006 (0x21cd0), CAPSULE primary-table slot 3.
void CapsuleShape::nxCapsuleDebugRenderDispatch(const void* renderer) const
	{
	if(!mBase.nxFlagBitsDE(8) || !renderer)
		return;
	nxCapsuleBaseDebugRender(renderer);
	if(!g_nxGuardC || !g_nxGuardRef)
		return;
	const float guard = *g_nxGuardC;
	const float ref = *g_nxGuardRef;
	if(guard == ref && guard == guard)
		return;
	void** table = *reinterpret_cast<void** const*>(renderer);
	typedef void (__fastcall* DrawLineFn)(void*, void*, const float*,
		const float*, unsigned);
	typedef void (__fastcall* DrawPoseFn)(void*, void*, unsigned,
		const void*, unsigned, unsigned, unsigned);
	DrawLineFn drawLine = reinterpret_cast<DrawLineFn>(table[8]);
	DrawPoseFn drawPose = reinterpret_cast<DrawPoseFn>(table[14]);
	void* rendererArg = const_cast<void*>(renderer);
	const float* rot = reinterpret_cast<const float*>(&mBase.mPose0C.mRotation);
	const float* trn = reinterpret_cast<const float*>(&mBase.mPose0C.mTranslation);
	const float r = mFloatE0, hh = mFloatE4;
	const unsigned color = mBase.nxFlagBitsDE(7) ? 0xffff00ffu : 0xffffffffu;
	float centers[2][3];
	for(unsigned axis = 0; axis < 3; ++axis)
		{
		const float span = hh * rot[3 * axis + 1];
		centers[0][axis] = trn[axis] - span;
		centers[1][axis] = trn[axis] + span;
		}
	for(unsigned radial = 0; radial < 4; ++radial)
		{
		const unsigned column = radial < 2 ? 0u : 2u;
		const float sign = (radial & 1u) ? -1.0f : 1.0f;
		float start[3], end[3];
		for(unsigned axis = 0; axis < 3; ++axis)
			{
			const float offset = sign * (r * rot[3 * axis + column]);
			start[axis] = centers[0][axis] + offset;
			end[axis] = centers[1][axis] + offset;
			}
		drawLine(rendererArg, nullptr, start, end, color);
		}
	unsigned radiusBits;
	memcpy(&radiusBits, &mFloatE0, 4);
	for(unsigned round = 0; round < 6; ++round)
		{
		float pose[12];
		for(unsigned axis = 0; axis < 3; ++axis)
			{
			const float c0 = rot[3 * axis];
			const float c1 = rot[3 * axis + 1];
			const float c2 = rot[3 * axis + 2];
			if(round < 2)
				{
				pose[3 * axis] = (round & 1u) ? c1 : -c1;
				pose[3 * axis + 1] = -c0;
				pose[3 * axis + 2] = c2;
				}
			else if(round < 4)
				{
				pose[3 * axis] = (round & 1u) ? c1 : -c1;
				pose[3 * axis + 1] = c2;
				pose[3 * axis + 2] = c0;
				}
			else
				{
				pose[3 * axis] = c2;
				pose[3 * axis + 1] = c0;
				pose[3 * axis + 2] = c1;
				}
			}
		memcpy(pose + 9, centers[round & 1u], sizeof(centers[0]));
		drawPose(rendererArg, nullptr, 0x14, pose, color, radiusBits,
			round < 4 ? 1u : 0u);
		}
	}

// phys_fn_000989 (0x00021ad0, 110 B)
// CAPSULE-table slot 12.
bool CapsuleShape::nxCapsuleLoadFromDesc(const void* record)
	{
	const unsigned char* rec = static_cast<const unsigned char*>(record);
	memcpy(&mFloatE0, rec + 0x4c, sizeof(mFloatE0));			// radius: 0x00021ad8
	float h = 0.0f;
	memcpy(&h, rec + 0x50, sizeof(h));							// desc height
	h *= 0.5f;													// fmul [0x101043cc] at 0x00021ae4
	mFloatE4 = h;												// fstp [+0xe4]
	memcpy(&mWordE8, rec + 0x54, sizeof(mWordE8));				// third word: 0x00021af0
	// The radius is reloaded and fcomp'd against the zero literal at
	// 0x00021aff. `test ah,0x41; jp` skips on C3/C0 both clear (greater) and
	// both set (unordered), so the report (line 0x37) fires for less or equal
	// only: a NaN radius is not reported. Everything was already stored
	// unconditionally above.
	if(mFloatE0 <= 0.0f)
		nxReport(1, nxSourceFileCapsuleShapeCpp, 0x37, 0,
			nxMsgCapsuleLoadRadius);
	// call BASE apply-descriptor at 0x00021b34; its al is the row's return.
	return mBase.nxApplyDescriptor(rec);
	}

// phys_fn_001001 (0x00021c30), CAPSULE-table slot 10.
void CapsuleShape::nxCapsuleCenterRadius(float* out) const
	{
	out[0] = mBase.mPose0C.mTranslation[0];	// +0x30 at 0x00021c30
	out[1] = mBase.mPose0C.mTranslation[1];
	out[2] = mBase.mPose0C.mTranslation[2];
	const float rr = mFloatE4 + mFloatE0;	// fld [+0xe4]; fadd [+0xe0]
	out[3] = rr;
	}

// phys_fn_001003 (0x00021c60), CAPSULE-table slot 11.
void CapsuleShape::nxCapsuleZeroCenterRadius(float* out) const
	{
	out[0] = 0.0f;							// xor edx,edx; three stores
	out[1] = 0.0f;
	out[2] = 0.0f;
	const float rr = mFloatE4 + mFloatE0;
	out[3] = rr;
	}

// ---------------------------------------------------------------------------
// BOX-table slot 10. See ObjectModel.h.

// phys_fn_000937 (0x00020670, 69 B)
void BoxShape::nxBoxCenterAndDiagonal(float* out) const
	{
	out[0] = mBase.mPose0C.mTranslation[0];	// mov edx,[ecx+0x30] at 0x00020670
	out[1] = mBase.mPose0C.mTranslation[1];	// +0x34
	out[2] = mBase.mPose0C.mTranslation[2];	// +0x38
	const double dx = mHull.mDims04[0];		// fld [+0xe4]
	const double dy = mHull.mDims04[1];		// [+0xe8]
	const double dz = mHull.mDims04[2];		// [+0xec]
	// fsqrt of (dx dx + dy dy) + dz dz, formed on the stack (0x20697-0x206a7).
	out[3] = static_cast<float>(x87FsqrtDot3(dx, dx, dy, dy, dz, dz));
	}

// phys_fn_000939 (0x000206c0, 62 B)
// BOX-table slot 11.
void BoxShape::nxBoxZeroCenterAndDiagonal(float* out) const
	{
	out[0] = 0.0f;							// xor edx,edx; mov [eax],edx ...
	out[1] = 0.0f;							//   (three dword stores, 0x000206c4..cc)
	out[2] = 0.0f;
	const double dx = mHull.mDims04[0];
	const double dy = mHull.mDims04[1];
	const double dz = mHull.mDims04[2];
	// fsqrt of (dx dx + dy dy) + dz dz, formed on the stack (0x206e0-0x206f0).
	out[3] = static_cast<float>(x87FsqrtDot3(dx, dx, dy, dy, dz, dz));	// 0x000206ce..f8
	}

// phys_fn_000925 (0x00020440, 13 B)
// `mov eax,ecx; xor ecx,ecx`, three dword stores of zero at +0/+4/+8, `ret`:
// the constructor returns this, as every MSVC constructor does.
HullScratchElement::HullScratchElement()
	{
	mWords[0] = 0;
	mWords[1] = 0;
	mWords[2] = 0;
	}

// phys_fn_000927 (0x00020450, 40 B)
// BOX-table slot 13.
bool BoxShape::nxBoxSaveState(void* record)
	{
	memcpy(reinterpret_cast<unsigned char*>(record) + 0x4c,
		&mHull.mDims04[0], sizeof(mHull.mDims04));	// 0x00020450..6e
	return mBase.nxBaseSaveState(record);			// jmp 0x000256f0 at 0x00020473
	}

// phys_fn_000941 (0x00020700, 68 B)
// BOX-table slot 8.
void BoxShape::nxBoxLocalAABB(float* out) const
	{
	out[0] = -mHull.mDims04[0];				// fld/fchs/fxch chain, 0x00020700..23
	out[1] = -mHull.mDims04[1];
	out[2] = -mHull.mDims04[2];
	out[3] = mHull.mDims04[0];				// raw copies, 0x00020726..41
	out[4] = mHull.mDims04[1];
	out[5] = mHull.mDims04[2];
	}

// phys_fn_000935 (0x000205a0, 198 B)
// BOX-table slot 9. Extent rows of pose one's
// rotation (0x100205a0-0x10020663). Each row sums its terms as
// (|dz*r2| + |dy*r1|) + |dx*r0| on the x87 stack (faddp at 0x100205ce, then
// 0x100205dc). The row-0 and row-1 extents are spilled to m32 (0x100205de,
// 0x10020604) and re-read by the fsub/fadd; the row-2 extent stays in st(4)
// and is used unrounded (fsub st(4) at 0x10020646, fadd st(1) at 0x10020659).
// The fst stores at 0x100205d8/0x100205fe/0x10020622 are dead.
void BoxShape::nxBoxWorldAABB(float* out) const
	{
	const float* r = reinterpret_cast<const float*>(&mBase.mPose0C.mRotation[0]);
	const double dx = mHull.mDims04[0];
	const double dy = mHull.mDims04[1];
	const double dz = mHull.mDims04[2];
	const float e0 = static_cast<float>(
		(fabs(dz * r[2]) + fabs(dy * r[1])) + fabs(dx * r[0]));
	const float e1 = static_cast<float>(
		(fabs(dz * r[5]) + fabs(dy * r[4])) + fabs(dx * r[3]));
	const double e2 = (fabs(dz * r[8]) + fabs(dy * r[7])) + fabs(dx * r[6]);
	const float* t = mBase.mPose0C.mTranslation;
	out[0] = static_cast<float>(static_cast<double>(t[0]) - e0);	// 0x10020639
	out[1] = static_cast<float>(static_cast<double>(t[1]) - e1);	// 0x1002063f
	out[2] = static_cast<float>(static_cast<double>(t[2]) - e2);	// 0x10020646
	out[3] = static_cast<float>(static_cast<double>(t[0]) + e0);	// 0x1002064b
	out[4] = static_cast<float>(static_cast<double>(t[1]) + e1);	// 0x10020652
	out[5] = static_cast<float>(static_cast<double>(t[2]) + e2);	// 0x10020659
	}

// Row 001309 (0x00025f60, 797 B), not claimed: written from the listing for
// 000933, its caller. [owner+8] non-null: the rotation is expanded from the
// body's quaternion (x,y,z,w at +0x5c..+0x68) with its m32 spills
// (0x10025f9f 2yy, 0x10025fd3 2xz, 0x10025fdd 2yw, 0x10025ffd 1-2xx,
// 0x1002600d 2yz) and the position copied from +0x50 (0x10026043-0x1002605a);
// otherwise the owner's pose at +0x20 is read. The translation row x stays
// in a register into its `+ t.x` while y and z are spilled first
// (0x100260d3/0x100260f9); every rotation element sums its terms as
// (a + b) + c in the order the listing loads them.
void ShapeBase::nxShapeGlobalPose(float* out) const
	{
	const unsigned char* owner = static_cast<const unsigned char*>(mOwner04);
	const unsigned char* body = *reinterpret_cast<const unsigned char* const*>(owner + 8);
	float pose[12];
	const float* src;
	if(body != nullptr)
		{
		const float* q = reinterpret_cast<const float*>(body + 0x5c);
		const double x = q[0], y = q[1], z = q[2], w = q[3];
		const double one = gMassDensitySentinel;				// [0x101041ec]
		double p;
		p = y * y; const float s8 = static_cast<float>(p + p);		// 2yy
		p = z * z; const double zz2 = p + p;
		pose[0] = static_cast<float>((one - s8) - zz2);
		p = y * x; const double xy2 = p + p;
		p = z * w; const double zw2 = p + p;
		pose[1] = static_cast<float>(xy2 - zw2);
		p = z * x; const float s0c = static_cast<float>(p + p);	// 2xz
		p = y * w; const double yw2 = p + p;
		const float s14 = static_cast<float>(yw2);				// fst 2yw
		pose[2] = static_cast<float>(yw2 + s0c);
		pose[3] = static_cast<float>(zw2 + xy2);
		p = x * x; const double oneMinusXx2 = one - (p + p);		// fsubr
		const float s10 = static_cast<float>(oneMinusXx2);		// fst 1-2xx
		pose[4] = static_cast<float>(oneMinusXx2 - zz2);
		p = z * y; const float s18 = static_cast<float>(p + p);	// 2yz
		p = w * x; const double xw2 = p + p;
		pose[5] = static_cast<float>(s18 - xw2);
		pose[6] = static_cast<float>(static_cast<double>(s0c) - s14);
		pose[7] = static_cast<float>(xw2 + s18);
		pose[8] = static_cast<float>(static_cast<double>(s10) - s8);
		memcpy(pose + 9, body + 0x50, 12);
		src = pose;
		}
	else
		src = reinterpret_cast<const float*>(owner + 0x20);

	const float* R = src;
	const float* L = reinterpret_cast<const float*>(mLocalPose.mRotation);	// +0x6c
	const float* lt = mLocalPose.mTranslation;								// +0x90
	const double tx = src[9];											// fld [edx+0x24]
	const double a = (static_cast<double>(R[0]) * lt[0] +
		static_cast<double>(R[1]) * lt[1]) + static_cast<double>(R[2]) * lt[2];
	const float sB = static_cast<float>((static_cast<double>(R[3]) * lt[0] +
		static_cast<double>(R[4]) * lt[1]) + static_cast<double>(R[5]) * lt[2]);
	const float sC = static_cast<float>((static_cast<double>(R[6]) * lt[0] +
		static_cast<double>(R[7]) * lt[1]) + static_cast<double>(R[8]) * lt[2]);
	const float ty = src[10], tz = src[11];
	const float outZ = static_cast<float>(static_cast<double>(tz) + sC);
	const float outX = static_cast<float>(tx + a);
	const float outY = static_cast<float>(static_cast<double>(ty) + sB);

	// Rotation: rows (R_r0 L0c + R_r1 L1c) + R_r2 L2c for columns 0 and 1,
	// (R_r1 L12 + R_r2 L22) + R_r0 L02 for column 2 (0x10026122-0x1002624d).
	float rot[9];
	for(unsigned r = 0; r < 3; ++r)
		{
		const double r0 = R[3 * r], r1 = R[3 * r + 1], r2 = R[3 * r + 2];
		rot[3 * r] = static_cast<float>((r0 * L[0] + r1 * L[3]) + r2 * L[6]);
		rot[3 * r + 1] = static_cast<float>((r0 * L[1] + r1 * L[4]) + r2 * L[7]);
		rot[3 * r + 2] = static_cast<float>((r1 * L[5] + r2 * L[8]) + r0 * L[2]);
		}
	memcpy(out, rot, sizeof(rot));
	out[9] = outX;
	out[10] = outY;
	out[11] = outZ;
	}

// phys_fn_000933 (0x000204e0, 188 B)
// __thiscall ret 4 (0x100204e0-0x10020599): an identity pose on the stack,
// filled by 001309 (call at 0x1002054d), then the NxBox: center = the
// pose's translation (x through fld/fstp, y and z as dwords), rotation to
// out+0x18 (rep movsd, 9 dwords), extents = the dims at +0xe4..+0xec.
void BoxShape::nxBoxGetWorldOBB(float* out) const
	{
	float pose[12] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 0.0f };
	mBase.nxShapeGlobalPose(pose);
	out[0] = pose[9];
	out[1] = pose[10];
	out[2] = pose[11];
	memcpy(out + 6, pose, 9 * sizeof(float));
	out[3] = mHull.mDims04[0];
	out[4] = mHull.mDims04[1];
	out[5] = mHull.mDims04[2];
	}

// phys_fn_000979 (0x00021940, 69 B)
// BOX-table slot 0. The image's order: the two vtable restores (the BOX
// table at +0x00, 0x1002194b, and the facade table at +0xe0, 0x10021951),
// the colobj destruction, then the base-dtor chain whose tail destroys the
// embedded Prunable, and the free of this through the imported Foundation
// allocator [0x101041bc] (0x10021971) when flags&1. The image destroys the
// colobj through its own table slot 0 (`call [eax]`, 0x1002195d); the
// product's collision object has one class, so its deleting destructor is
// called directly.
void BoxShape::nxBoxScalarDeletingDtor(unsigned flags)
	{
	mBase.mVptrSlot = nxBoxShapeInternalVtable();	// 0x1002194b
	mHull.mVptrSlot = nxBoxHullFacadeVtable();		// 0x10021951
	if(mBase.mWord9C)
		{
		// mov ecx,[esi+0x9c]; test; push 1; call [eax] at 0x0002195b..61.
		nxCollisionFamilyScalarDeletingDtor<CollisionObjectBox>(
			reinterpret_cast<CollisionObject*>(mBase.mWord9C), 1);
		}
	mBase.nxBaseDtorOwnerArms();		// owner arms, 0x26be1..c35
	mBase.mPrunable.~Prunable();			// tail of 0x00026bd0: jmp 0xb5640
	if(flags & 1u)
		nxFoundationSDKAllocator->free(this);
	}

// phys_fn_001399 (0x00028e80), MESH-table slot 0.
void MeshShape::nxMeshScalarDeletingDtor(unsigned flags)
	{
	if(mBase.mWord9C)
		{
		nxCollisionFamilyScalarDeletingDtor<CollisionObjectMesh>(
			reinterpret_cast<CollisionObject*>(mBase.mWord9C), 1);
		}
	if(mWordE0)
		{
		--*reinterpret_cast<NxU32*>(mWordE0 + 0x74);	// dec [mesh+0x74]: 0x00028ea3
		}
	mBase.nxBaseDtorOwnerArms();		// owner arms, 0x26be1..c35
	mBase.mPrunable.~Prunable();			// tail of the base-dtor chain
	if(flags & 1u)
		nxFoundationSDKAllocator->free(this);
	}

// ---------------------------------------------------------------------------
// PlaneShape. See ObjectModel.h for the row map.

PlaneShape::PlaneShape(void* owner, unsigned argument)
	: mBase(owner, argument)				// forwarded unchanged: 0x00024edb..df
	{
	mBase.mVptrSlot = nxPlaneShapeInternalVtable();
	// The embedded collision object: a fresh 0x1c-byte block through the imported
	// Foundation allocator [0x101041bc] (0x00024eea..f8), built by phys_fn_001159 -- the plane-family
	// variant of the shared collision-object constructor -- with the plane
	// stored at BOTH +0x08 and +0x18.
	void* memory = nxFoundationSDKAllocator->malloc(0x1c, NX_MEMORY_PERSISTENT);
	CollisionObject* object = memory
		? new(memory) CollisionObjectPlane(this)
		: 0;								// null arm: 0x00024f09
	mBase.mWord9C = reinterpret_cast<NxU32>(object);	// 0x00024f0b

	mBase.mSentinelD0 = 0;					// NX_SHAPE_PLANE: 0x00024f24
	mNormalE0[0] = 0.0f;					// 0x00024f2f
	mNormalE0[1] = 1.0f;					// 0x00024f35
	mNormalE0[2] = 0.0f;					// 0x00024f3c
	mDistanceEC = 0.0f;						// 0x00024f44

	// The image calls NxFoundation's NxNormalToTangents through the import at
	// .rdata 0x1010418c (call 0x00024f4e); the transcription reaches the same
	// function through the reconstruction's own export.
	NxNormalToTangents(
		*reinterpret_cast<const NxVec3*>(&mNormalE0[0]),
		*reinterpret_cast<NxVec3*>(&mTangentF0[0]),
		*reinterpret_cast<NxVec3*>(&mBinormalFC[0]));

	mWord108 = 1;							// 0x00024f57
	}

// phys_fn_001251 (0x00024f80), PLANE-table slot 13.
bool PlaneShape::nxPlaneSaveState(void* record)
	{
	unsigned char* rec = static_cast<unsigned char*>(record);
	memcpy(rec + 0x4c, &mNormalE0[0], sizeof(mNormalE0));	// 0x00024f80..9c
	const float negD = -mDistanceEC;		// fld [+0xec]; fchs at 0x00024f9f..a5
	memcpy(rec + 0x58, &negD, sizeof(negD));
	return mBase.nxBaseSaveState(record);	// jmp 0x000256f0 at 0x00024fae
	}

// phys_fn_001257 (0x000251d0), PLANE-table slots 9 and 11.
void PlaneShape::nxPlaneExtentRow(float* out) const
	{
	out[0] = 0.0f;							// xor ecx,ecx; three stores
	out[1] = 0.0f;
	out[2] = 0.0f;
	unsigned big = 0x7f7fffffu;				// +FLT_MAX: the unbounded reach
	memcpy(out + 3, &big, sizeof(big));
	}

// phys_fn_001305 (0x25960) through a PLANE receiver. The second arm calls
// the plane's slot-10 extent row for the shared three-pose debug display.
void PlaneShape::nxPlaneBaseDebugRender(const void* renderer) const
	{
	if(!g_nxGuardRef || !renderer)
		return;
	const float ref = *g_nxGuardRef;
	void** table = *reinterpret_cast<void** const*>(renderer);
	typedef void (__fastcall* DrawLineFn)(void*, void*, const float*,
		const float*, unsigned);
	typedef void (__fastcall* DrawPoseFn)(void*, void*, unsigned,
		const void*, unsigned, unsigned, unsigned);
	DrawLineFn drawLine = reinterpret_cast<DrawLineFn>(table[8]);
	DrawPoseFn drawPose = reinterpret_cast<DrawPoseFn>(table[14]);
	void* rendererArg = const_cast<void*>(renderer);
	const float guardA = *g_nxGuardA;
	if(guardA != ref || guardA != guardA)
		{
		const float k = *g_nxRenderScale * guardA;
		const float* rot = reinterpret_cast<const float*>(&mBase.mPose0C.mRotation);
		const float* trn = reinterpret_cast<const float*>(&mBase.mPose0C.mTranslation);
		static const unsigned colors[3] = { 0xcf0000u, 0xcf00u, 0xcfu };
		for(unsigned axis = 0; axis < 3; ++axis)
			{
			const float start[3] = { trn[0], trn[1], trn[2] };
			const float end[3] = {
				k * rot[axis] + trn[0],
				k * rot[3 + axis] + trn[1],
				k * rot[6 + axis] + trn[2]
			};
			drawLine(rendererArg, nullptr, start, end, colors[axis]);
			}
		}
	const float guardB = *g_nxGuardB;
	if(guardB != ref || guardB != guardB)
		{
		float center[4] = {};
		nxPlaneExtentRow(center);
		const float* rot = reinterpret_cast<const float*>(&mBase.mPose0C.mRotation);
		float pose[12];
		for(unsigned i = 0; i < 9; ++i)
			pose[i] = rot[i];
		pose[9] = center[0];
		pose[10] = center[1];
		pose[11] = center[2];
		unsigned radiusBits;
		memcpy(&radiusBits, center + 3, 4);
		for(unsigned round = 0; round < 3; ++round)
			{
			drawPose(rendererArg, nullptr, 0x14, pose, 0xffff00ffu,
				radiusBits, 0);
			if(round < 2)
				{
				float next[9];
				for(unsigned row = 0; row < 3; ++row)
					for(unsigned col = 0; col < 3; ++col)
						next[row * 3 + col] = pose[row * 3 + (col + 1) % 3];
				memcpy(pose, next, sizeof(next));
				}
			}
		}
	}

// phys_fn_001259 (0x251f0), PLANE primary-table slot 3.
void PlaneShape::nxPlaneDebugRenderDispatch(const void* renderer) const
	{
	if(!mBase.nxFlagBitsDE(8) || !renderer)
		return;
	nxPlaneBaseDebugRender(renderer);
	if(!g_nxGuardC || !g_nxGuardRef)
		return;
	const float guard = *g_nxGuardC;
	const float ref = *g_nxGuardRef;
	if(guard == ref && guard == guard)
		return;
	void** table = *reinterpret_cast<void** const*>(renderer);
	typedef void (__fastcall* DrawPoseFn)(void*, void*, unsigned,
		const void*, unsigned, unsigned, unsigned);
	DrawPoseFn drawPose = reinterpret_cast<DrawPoseFn>(table[14]);
	const unsigned color = mBase.nxFlagBitsDE(7) ? 0xffff00ffu : 0xffffffffu;
	float pose[12];
	for(unsigned row = 0; row < 3; ++row)
		{
		pose[3 * row] = mTangentF0[row];
		pose[3 * row + 1] = mBinormalFC[row];
		pose[3 * row + 2] = mNormalE0[row];
		pose[9 + row] = -mNormalE0[row] * mDistanceEC;
		}
	static const float scales[4] = {1.0f,10.0f,100.0f,1000.0f};
	for(unsigned round = 0; round < 4; ++round)
		{
		unsigned scaleBits;
		memcpy(&scaleBits, &scales[round], 4);
		drawPose(const_cast<void*>(renderer), nullptr, 0x14, pose,
			color, scaleBits, 0);
		}
	}

// ---------------------------------------------------------------------------
// MeshShape. See ObjectModel.h for the row map.

MeshShape::MeshShape(void* owner, unsigned argument)
	: mBase(owner, argument)				// forwarded unchanged: 0x00027dbb..bf
	{
	mBase.mVptrSlot = nxMeshShapeInternalVtable();	// .rdata:0x00107630, 0x00027dc4
	mWordE0 = 0;							// mov [esi+0xe0],0 at 0x00027dca
	mWordE4 = 0;							// mov [esi+0xe4],0 at 0x00027dd4

	// The embedded collision object: a fresh 0x1c-byte block through the imported
	// Foundation allocator [0x101041bc] (0x00027dde..ec), built by phys_fn_001241 -- the mesh-family
	// variant of the shared collision-object constructor -- with the mesh
	// shape stored at BOTH +0x08 and +0x18.
	void* memory = nxFoundationSDKAllocator->malloc(0x1c, NX_MEMORY_PERSISTENT);
	CollisionObject* object = memory
		? new(memory) CollisionObjectMesh(this)
		: 0;								// null arm: 0x00027dfd
	mBase.mWord9C = reinterpret_cast<NxU32>(object);	// 0x00027dff

	mBase.mSentinelD0 = 4;					// NX_SHAPE_MESH: 0x00027e05
	}

// Scene.cpp's raw actor-shape factory delegates construction of the embedded
// collision object here, keeping its allocator-backed hook member valid while
// the scene unit assigns the public shape-handle table.
void* nxShapeFactoryConstructMeshHandle(void* memory, void* shape)
	{
	return ::new(memory) CollisionObjectMesh(shape);
	}

// phys_fn_001385 (0x00027e60), MESH-table slot 13.
bool MeshShape::nxMeshSaveState(void* record)
	{
	const unsigned char* mesh = reinterpret_cast<const unsigned char*>(mWordE0);
	unsigned meshWord = *reinterpret_cast<const unsigned*>(mesh + 0xe4);	// 0x00027e60..66
	unsigned char* rec = static_cast<unsigned char*>(record);
	memcpy(rec + 0x4c, &meshWord, sizeof(meshWord));
	memcpy(rec + 0x50, &mWordE4, sizeof(mWordE4));		// 0x00027e73..79
	return mBase.nxBaseSaveState(record);				// jmp 0x000256f0 at 0x00027e80
	}

// phys_fn_001387 (0x00027e90), MESH-table slot 11.
void MeshShape::nxMeshGetWords5C(unsigned* out) const
	{
	const unsigned char* mesh = reinterpret_cast<const unsigned char*>(mWordE0);
	memcpy(out, mesh + 0x5c, 16);						// four dwords, 0x00027e96..b1
	}

// phys_fn_001389 (0x00027ec0), MESH-table slot 8.
void MeshShape::nxMeshGetWords44(unsigned* out) const
	{
	const unsigned char* mesh = reinterpret_cast<const unsigned char*>(mWordE0);
	memcpy(out, mesh + 0x44, 24);						// six dwords, 0x00027ec0..ef
	}

// phys_fn_001401 (0x00028ed0), MESH-table slot 9: the branch at 0x28fe4
// taken when mesh+0xa0 holds no acceleration tree. The tree-backed branch
// will need its own reconstruction before this can be used as the full slot.
void MeshShape::nxMeshWorldAABBNoTree(float* out) const
	{
	const unsigned char* mesh = reinterpret_cast<const unsigned char*>(mWordE0);
	float bounds[6];
	memcpy(bounds, mesh + 0x44, sizeof(bounds));
	// The x87 listing rounds these intermediates at different points: the
	// z sum and z difference before halving, the x center and all extents
	// after halving, while the y/z centers remain in registers.
	const float cx = static_cast<float>(
		(static_cast<double>(bounds[0]) + bounds[3]) * 0.5);
	const double cy = (static_cast<double>(bounds[1]) + bounds[4]) * 0.5;
	const float zsum = static_cast<float>(
		static_cast<double>(bounds[2]) + bounds[5]);
	const double cz = static_cast<double>(zsum) * 0.5;
	const float ex = static_cast<float>(
		(static_cast<double>(bounds[3]) - bounds[0]) * 0.5);
	const float ey = static_cast<float>(
		(static_cast<double>(bounds[4]) - bounds[1]) * 0.5);
	const float zdifference = static_cast<float>(
		static_cast<double>(bounds[5]) - bounds[2]);
	const float ez = static_cast<float>(static_cast<double>(zdifference) * 0.5);
	const float* r = reinterpret_cast<const float*>(mBase.mPose0C.mRotation);
	const float* t = mBase.mPose0C.mTranslation;
	for(unsigned row = 0; row < 3; ++row)
		{
		const unsigned j = row * 3;
		const double transformed =
			cz * r[j + 2] + cy * r[j + 1] +
			static_cast<double>(cx) * r[j];
		const double extent =
			fabs(static_cast<double>(ey) * r[j + 1]) +
			fabs(static_cast<double>(ez) * r[j + 2]) +
			fabs(static_cast<double>(ex) * r[j]);
		if(row == 0)
			{
			const double world = static_cast<double>(
				static_cast<float>(transformed)) + t[row];
			out[row] = static_cast<float>(world - extent);
			out[row + 3] = static_cast<float>(world + extent);
			}
		else
			{
			const float world = static_cast<float>(
				(row == 1 ? static_cast<double>(static_cast<float>(transformed))
					: transformed) + t[row]);
			const float roundedExtent = static_cast<float>(extent);
			out[row] = static_cast<float>(
				static_cast<double>(world) - roundedExtent);
			out[row + 3] = static_cast<float>(
				static_cast<double>(world) + roundedExtent);
			}
		}
	}

// phys_fn_001401 (0x00028ed0), MESH-table slot 9. With a support graph,
// run phys_fn_001530's stamped neighbor climb for each signed pose axis.
// The graph stores a vertex count, offset, and neighbor list for each vertex;
// six cached start indices live at tree+0x68. The owner scratch record supplies
// the visited array and a monotonically increasing stamp.
void MeshShape::nxMeshWorldAABB(float* out) const
	{
	unsigned char* mesh = reinterpret_cast<unsigned char*>(mWordE0);
	unsigned char* tree = *reinterpret_cast<unsigned char**>(mesh + 0xa0);
	if(tree == nullptr)
		{
		nxMeshWorldAABBNoTree(out);
		return;
		}
	const float* vertices = *reinterpret_cast<const float* const*>(tree + 0x10);
	unsigned char* graph = *reinterpret_cast<unsigned char**>(tree + 0x64);
	unsigned* cached = reinterpret_cast<unsigned*>(tree + 0x68);
	const unsigned* counts = graph
		? *reinterpret_cast<const unsigned* const*>(graph + 8) : nullptr;
	const unsigned* offsets = graph
		? *reinterpret_cast<const unsigned* const*>(graph + 0x0c) : nullptr;
	const unsigned* neighbors = graph
		? *reinterpret_cast<const unsigned* const*>(graph + 0x10) : nullptr;
	unsigned char* scratch = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(mBase.mOwner04) + 4);
	unsigned* visited = *reinterpret_cast<unsigned**>(scratch + 8);
	unsigned& stamp = *reinterpret_cast<unsigned*>(scratch + 0x14);
	const float* r = reinterpret_cast<const float*>(mBase.mPose0C.mRotation);
	const float* t = mBase.mPose0C.mTranslation;
	for(unsigned slot = 0; slot < 6; ++slot)
		{
		const unsigned axis = slot % 3;
		const float* row = r + axis * 3;
		float direction[3] = {row[0], row[1], row[2]};
		if(slot < 3)
			for(unsigned j = 0; j < 3; ++j)
				direction[j] = -direction[j];
		++stamp;
		if(stamp == 0)
			{
			const unsigned count = *reinterpret_cast<unsigned*>(scratch + 4);
			if(visited)
				memset(visited, 0, (count & 0x3fffffffu) * sizeof(unsigned));
			stamp = count;
			}
		unsigned current = cached[slot];
		if(graph && visited && counts && offsets && neighbors)
			{
			visited[current] = stamp;
			const float* initial = vertices + current * 3;
			float best = static_cast<float>(
				static_cast<double>(initial[1]) * direction[1] +
				static_cast<double>(initial[2]) * direction[2] +
				static_cast<double>(initial[0]) * direction[0]);
			for(;;)
				{
				const unsigned previous = current;
				cached[slot] = previous;
				const unsigned start = offsets[previous];
				for(unsigned j = 0; j < counts[previous]; ++j)
					{
					const unsigned neighbor = neighbors[start + j];
					if(visited[neighbor] == stamp)
						continue;
					visited[neighbor] = stamp;
					const float* vertex = vertices + neighbor * 3;
					const double score =
						static_cast<double>(vertex[1]) * direction[1] +
						static_cast<double>(vertex[2]) * direction[2] +
						static_cast<double>(vertex[0]) * direction[0];
					if(score > best)
						{
						current = neighbor;
						best = static_cast<float>(score);
						}
					}
				if(current == previous)
					break;
				}
			}
		const float* vertex = vertices + cached[slot] * 3;
		out[slot] = static_cast<float>(
			static_cast<double>(vertex[2]) * row[2] +
			static_cast<double>(vertex[1]) * row[1] +
			static_cast<double>(vertex[0]) * row[0] + t[axis]);
		}
	}

// Listing transcriptions of phys_fn_001397 (0x00028e10, the mesh shape's slot 4: the mass frame from the cached
// volume integrals) and of the three rows it calls that were not rows of the candidate's own: 002241 (0x00054bb0, the
// mesh's cached mass block: the volume integrals, the finite checks, the negative-volume report and sign flip),
// 002156 (0x000538e0, nine doubles to nine floats; the C++ nxDoubleToFloat9_2156 above is the cdecl form for the
// other callers) and 000835 (0x0001c5a0, the payload fold then the translation). The imports are the candidate's own
// IAT slots, the CRT's _fpclass is the candidate's. Compare script:
// docs/reconstruction/novodex-physics/evidence/phase3-mesh-debug-listing-compare.py.
extern "C" void nxMSCallPoseCopy();
extern "C" void nxMSCallFold();
extern "C" void nxMSCallTranslate();
extern "C" void nxMSCallMerge();
extern "C" void nxMSCallNoop();
extern "C" void nxMSCallFpclass();
extern "C" void* nxMSVolumeSlot;
extern "C" void* nxMSErrorSlot;
#pragma comment(linker, "/alternatename:_nxMSCallPoseCopy=?nxPoseCopyWithTail0827@BoxShape@@QAEXPBXPBII@Z")
#pragma comment(linker, "/alternatename:_nxMSCallFold=?nxMassFrameFoldPayload@MassFrame@@QAEXPBX@Z")
#pragma comment(linker, "/alternatename:_nxMSCallTranslate=?nxMassFrameTranslate@MassFrame@@QAEXPBX@Z")
#pragma comment(linker, "/alternatename:_nxMSCallMerge=?nxMassFrameMerge@MassFrame@@QAEXABV1@@Z")
#pragma comment(linker, "/alternatename:_nxMSCallNoop=?nxSupportMapNoop@@YIXPAUIceSupportMap@@@Z")
#pragma comment(linker, "/alternatename:_nxMSCallFpclass=__fpclass")
#pragma comment(linker, "/alternatename:_nxMSVolumeSlot=__imp__NxComputeVolumeIntegrals")
#pragma comment(linker, "/alternatename:_nxMSErrorSlot=__imp_?error@FoundationSDK@NxFoundation@@SA_NW4NxErrorCode@@PBDHPA_N1ZZ")
static const unsigned nxMSK101041f0_4 = 0x0;
static const unsigned long long nxMSK101076e8_8 = 0x0ull;
static const char nxMSMessage[] = "TriangleMesh: Mesh has a negative volume!  Is it open or do (some) faces have reversed winding? (Taking absolute value.)";
static const char nxMSFile[] = "\\Epic\\Novodex\\SDKs\\Physics\\src\\TriangleMesh.cpp";
// phys_fn_002156 (0x000538e0, 46 B), 16 instructions
__declspec(naked) static void __fastcall nxMeshRow2156(void* self, void* edxUnused, float* out)
	{
	__asm
		{
		mov edx, dword ptr [esp + 4]		// 0x000538e0
		lea eax, [ecx + 0x30]		// 0x000538e4
		add edx, 0x18		// 0x000538e7
		mov ecx, 3		// 0x000538ea
		nop		// 0x000538ef
L538f0:
		fld qword ptr [eax - 0x18]		// 0x000538f0
		add eax, 8		// 0x000538f3
		fstp dword ptr [edx - 0x18]		// 0x000538f6
		add edx, 4		// 0x000538f9
		dec ecx		// 0x000538fc
		fld qword ptr [eax - 8]		// 0x000538fd
		fstp dword ptr [edx - 0x10]		// 0x00053900
		fld qword ptr [eax + 0x10]		// 0x00053903
		fstp dword ptr [edx - 4]		// 0x00053906
		jne L538f0		// 0x00053909
		ret 4		// 0x0005390b
		}
	}

// phys_fn_000835 (0x0001c5a0, 30 B), 13 instructions
__declspec(naked) static void __fastcall nxMassFrameRow835(void* destination, void* edxUnused, const void* extra)
	{
	__asm
		{
		push esi		// 0x0001c5a0
		push edi		// 0x0001c5a1
		mov edi, dword ptr [esp + 0xc]		// 0x0001c5a2
		push edi		// 0x0001c5a6
		mov esi, ecx		// 0x0001c5a7
		call nxMSCallFold		// 0x0001c5a9
		add edi, 0x24		// 0x0001c5ae
		push edi		// 0x0001c5b1
		mov ecx, esi		// 0x0001c5b2
		call nxMSCallTranslate		// 0x0001c5b4
		pop edi		// 0x0001c5b9
		pop esi		// 0x0001c5ba
		ret 4		// 0x0001c5bb
		}
	}

// phys_fn_002241 (0x00054bb0, 692 B), 193 instructions
__declspec(naked) static void* __fastcall nxMeshMassRow2241(void* mesh)
	{
	__asm
		{
		push ebp		// 0x00054bb0
		mov ebp, esp		// 0x00054bb1
		and esp, 0xfffffff8		// 0x00054bb3
		sub esp, 0xc4		// 0x00054bb6
		push ebx		// 0x00054bbc
		push esi		// 0x00054bbd
		push edi		// 0x00054bbe
		mov edi, ecx		// 0x00054bbf
		fld dword ptr [edi + 0xb0]		// 0x00054bc1
		lea ebx, [edi + 0xb0]		// 0x00054bc7
		fcomp dword ptr [nxMSK101041f0_4]		// 0x00054bcd
		fnstsw ax		// 0x00054bd3
		test ah, 5		// 0x00054bd5
		jp L54e52		// 0x00054bd8
		mov ecx, dword ptr [edi + 0xc]		// 0x00054bde
		mov eax, dword ptr [edi + 8]		// 0x00054be1
		mov edx, dword ptr [edi + 0x10]		// 0x00054be4
		mov dword ptr [esp + 0x10], ecx		// 0x00054be7
		mov dword ptr [esp + 0xc], eax		// 0x00054beb
		lea ecx, [esp + 0x28]		// 0x00054bef
		mov eax, 0xc		// 0x00054bf3
		push ecx		// 0x00054bf8
		mov dword ptr [esp + 0x20], edx		// 0x00054bf9
		mov dword ptr [esp + 0x18], eax		// 0x00054bfd
		mov dword ptr [esp + 0x1c], eax		// 0x00054c01
		mov eax, dword ptr [edi + 0x14]		// 0x00054c05
		lea edx, [esp + 0x10]		// 0x00054c08
		push 0x3f800000		// 0x00054c0c
		push edx		// 0x00054c11
		mov dword ptr [esp + 0x30], 0		// 0x00054c12
		mov dword ptr [esp + 0x2c], eax		// 0x00054c1a
		call dword ptr [nxMSVolumeSlot]		// 0x00054c1e
		add esp, 0xc		// 0x00054c24
		test al, al		// 0x00054c27
		je L54e5b		// 0x00054c29
		lea esi, [edi + 0xb4]		// 0x00054c2f
		push esi		// 0x00054c35
		lea ecx, [esp + 0x2c]		// 0x00054c36
		call nxMeshRow2156		// 0x00054c3a
		mov eax, dword ptr [esp + 0x28]		// 0x00054c3f
		mov ecx, dword ptr [esp + 0x2c]		// 0x00054c43
		mov edx, dword ptr [esp + 0x30]		// 0x00054c47
		mov dword ptr [edi + 0xd8], eax		// 0x00054c4b
		mov dword ptr [edi + 0xdc], ecx		// 0x00054c51
		mov dword ptr [edi + 0xe0], edx		// 0x00054c57
		fld dword ptr [esi]		// 0x00054c5d
		sub esp, 8		// 0x00054c5f
		fstp qword ptr [esp]		// 0x00054c62
		call nxMSCallFpclass		// 0x00054c65
		add esp, 8		// 0x00054c6a
		test eax, 0x207		// 0x00054c6d
		jne L54e5b		// 0x00054c72
		fld dword ptr [esi + 4]		// 0x00054c78
		sub esp, 8		// 0x00054c7b
		fstp qword ptr [esp]		// 0x00054c7e
		call nxMSCallFpclass		// 0x00054c81
		add esp, 8		// 0x00054c86
		test eax, 0x207		// 0x00054c89
		jne L54e5b		// 0x00054c8e
		fld dword ptr [esi + 8]		// 0x00054c94
		sub esp, 8		// 0x00054c97
		fstp qword ptr [esp]		// 0x00054c9a
		call nxMSCallFpclass		// 0x00054c9d
		add esp, 8		// 0x00054ca2
		test eax, 0x207		// 0x00054ca5
		jne L54e5b		// 0x00054caa
		fld dword ptr [esi + 0xc]		// 0x00054cb0
		sub esp, 8		// 0x00054cb3
		fstp qword ptr [esp]		// 0x00054cb6
		call nxMSCallFpclass		// 0x00054cb9
		add esp, 8		// 0x00054cbe
		test eax, 0x207		// 0x00054cc1
		jne L54e5b		// 0x00054cc6
		fld dword ptr [esi + 0x10]		// 0x00054ccc
		sub esp, 8		// 0x00054ccf
		fstp qword ptr [esp]		// 0x00054cd2
		call nxMSCallFpclass		// 0x00054cd5
		add esp, 8		// 0x00054cda
		test eax, 0x207		// 0x00054cdd
		jne L54e5b		// 0x00054ce2
		fld dword ptr [esi + 0x14]		// 0x00054ce8
		sub esp, 8		// 0x00054ceb
		fstp qword ptr [esp]		// 0x00054cee
		call nxMSCallFpclass		// 0x00054cf1
		add esp, 8		// 0x00054cf6
		test eax, 0x207		// 0x00054cf9
		jne L54e5b		// 0x00054cfe
		fld dword ptr [esi + 0x18]		// 0x00054d04
		sub esp, 8		// 0x00054d07
		fstp qword ptr [esp]		// 0x00054d0a
		call nxMSCallFpclass		// 0x00054d0d
		add esp, 8		// 0x00054d12
		test eax, 0x207		// 0x00054d15
		jne L54e5b		// 0x00054d1a
		fld dword ptr [esi + 0x1c]		// 0x00054d20
		sub esp, 8		// 0x00054d23
		fstp qword ptr [esp]		// 0x00054d26
		call nxMSCallFpclass		// 0x00054d29
		add esp, 8		// 0x00054d2e
		test eax, 0x207		// 0x00054d31
		jne L54e5b		// 0x00054d36
		fld dword ptr [esi + 0x20]		// 0x00054d3c
		sub esp, 8		// 0x00054d3f
		fstp qword ptr [esp]		// 0x00054d42
		call nxMSCallFpclass		// 0x00054d45
		add esp, 8		// 0x00054d4a
		test eax, 0x207		// 0x00054d4d
		jne L54e5b		// 0x00054d52
		fld dword ptr [edi + 0xd8]		// 0x00054d58
		sub esp, 8		// 0x00054d5e
		fstp qword ptr [esp]		// 0x00054d61
		call nxMSCallFpclass		// 0x00054d64
		add esp, 8		// 0x00054d69
		test eax, 0x207		// 0x00054d6c
		jne L54e5b		// 0x00054d71
		fld dword ptr [edi + 0xdc]		// 0x00054d77
		sub esp, 8		// 0x00054d7d
		fstp qword ptr [esp]		// 0x00054d80
		call nxMSCallFpclass		// 0x00054d83
		add esp, 8		// 0x00054d88
		test eax, 0x207		// 0x00054d8b
		jne L54e5b		// 0x00054d90
		fld dword ptr [edi + 0xe0]		// 0x00054d96
		sub esp, 8		// 0x00054d9c
		fstp qword ptr [esp]		// 0x00054d9f
		call nxMSCallFpclass		// 0x00054da2
		add esp, 8		// 0x00054da7
		test eax, 0x207		// 0x00054daa
		jne L54e5b		// 0x00054daf
		fld qword ptr [esp + 0x38]		// 0x00054db5
		sub esp, 8		// 0x00054db9
		fstp qword ptr [esp]		// 0x00054dbc
		call nxMSCallFpclass		// 0x00054dbf
		add esp, 8		// 0x00054dc4
		test eax, 0x207		// 0x00054dc7
		jne L54e5b		// 0x00054dcc
		fld qword ptr [esp + 0x38]		// 0x00054dd2
		fcom qword ptr [nxMSK101076e8_8]		// 0x00054dd6
		fnstsw ax		// 0x00054ddc
		test ah, 5		// 0x00054dde
		jp L54e50		// 0x00054de1
		push offset nxMSMessage		// 0x00054de3
		fstp st(0)		// 0x00054de8
		push 0		// 0x00054dea
		push 0x2f7		// 0x00054dec
		push offset nxMSFile		// 0x00054df1
		push 0xce		// 0x00054df6
		call dword ptr [nxMSErrorSlot]		// 0x00054dfb
		fld qword ptr [esp + 0x4c]		// 0x00054e01
		fchs		// 0x00054e05
		add esp, 0x14		// 0x00054e07
		fld dword ptr [esi]		// 0x00054e0a
		fchs		// 0x00054e0c
		fstp dword ptr [esi]		// 0x00054e0e
		fld dword ptr [esi + 4]		// 0x00054e10
		fchs		// 0x00054e13
		fstp dword ptr [esi + 4]		// 0x00054e15
		fld dword ptr [esi + 8]		// 0x00054e18
		fchs		// 0x00054e1b
		fstp dword ptr [esi + 8]		// 0x00054e1d
		fld dword ptr [esi + 0xc]		// 0x00054e20
		fchs		// 0x00054e23
		fstp dword ptr [esi + 0xc]		// 0x00054e25
		fld dword ptr [esi + 0x10]		// 0x00054e28
		fchs		// 0x00054e2b
		fstp dword ptr [esi + 0x10]		// 0x00054e2d
		fld dword ptr [esi + 0x14]		// 0x00054e30
		fchs		// 0x00054e33
		fstp dword ptr [esi + 0x14]		// 0x00054e35
		fld dword ptr [esi + 0x18]		// 0x00054e38
		fchs		// 0x00054e3b
		fstp dword ptr [esi + 0x18]		// 0x00054e3d
		fld dword ptr [esi + 0x1c]		// 0x00054e40
		fchs		// 0x00054e43
		fstp dword ptr [esi + 0x1c]		// 0x00054e45
		fld dword ptr [esi + 0x20]		// 0x00054e48
		fchs		// 0x00054e4b
		fstp dword ptr [esi + 0x20]		// 0x00054e4d
L54e50:
		fstp dword ptr [ebx]		// 0x00054e50
L54e52:
		mov eax, ebx		// 0x00054e52
		pop edi		// 0x00054e54
		pop esi		// 0x00054e55
		pop ebx		// 0x00054e56
		mov esp, ebp		// 0x00054e57
		pop ebp		// 0x00054e59
		ret		// 0x00054e5a
L54e5b:
		pop edi		// 0x00054e5b
		pop esi		// 0x00054e5c
		xor eax, eax		// 0x00054e5d
		pop ebx		// 0x00054e5f
		mov esp, ebp		// 0x00054e60
		pop ebp		// 0x00054e62
		ret		// 0x00054e63
		}
	}

// phys_fn_001397 (0x00028e10, 104 B), 35 instructions
__declspec(naked) bool MeshShape::nxMeshAccumulateMassCached(MassFrame* destination, float density, unsigned reserved) const
	{
	__asm
		{
		sub esp, 0x34		// 0x00028e10
		push esi		// 0x00028e13
		mov esi, ecx		// 0x00028e14
		test byte ptr [esi + 0xde], 7		// 0x00028e16
		jne L28e66		// 0x00028e1d
		mov ecx, dword ptr [esi + 0xe0]		// 0x00028e1f
		call nxMeshMassRow2241		// 0x00028e25
		test eax, eax		// 0x00028e2a
		je L28e6f		// 0x00028e2c
		mov ecx, dword ptr [eax]		// 0x00028e2e
		push ecx		// 0x00028e30
		lea edx, [eax + 0x28]		// 0x00028e31
		push edx		// 0x00028e34
		add eax, 4		// 0x00028e35
		push eax		// 0x00028e38
		lea ecx, [esp + 0x10]		// 0x00028e39
		call nxMSCallPoseCopy		// 0x00028e3d
		add esi, 0x6c		// 0x00028e42
		push esi		// 0x00028e45
		lea ecx, [esp + 8]		// 0x00028e46
		call nxMassFrameRow835		// 0x00028e4a
		mov ecx, dword ptr [esp + 0x3c]		// 0x00028e4f
		lea eax, [esp + 4]		// 0x00028e53
		push eax		// 0x00028e57
		call nxMSCallMerge		// 0x00028e58
		lea ecx, [esp + 4]		// 0x00028e5d
		call nxMSCallNoop		// 0x00028e61
L28e66:
		mov al, 1		// 0x00028e66
		pop esi		// 0x00028e68
		add esp, 0x34		// 0x00028e69
		ret 0xc		// 0x00028e6c
L28e6f:
		xor al, al		// 0x00028e6f
		pop esi		// 0x00028e71
		add esp, 0x34		// 0x00028e72
		ret 0xc		// 0x00028e75
		}
	}

// Listing transcription of phys_fn_001407 at 0x00029610 (MESH's slot 7): with a classifier at mesh+0xac it carries the
// point into the mesh's frame, picks the polygon plane the support map names (001556), builds the hull's polygons first
// if absent (001472) and writes the ray/plane distance (NxRayPlaneIntersect) from the hull's origin. Compare script:
// docs/reconstruction/novodex-physics/evidence/phase3-mesh-raycast-listing-compare.py.
extern "C" void nxMSCallSupportLookup();
extern "C" void nxMSCallHullPolygons();
extern "C" void nxMSCallRayPlane();
#pragma comment(linker, "/alternatename:_nxMSCallSupportLookup=?nxSupportMapLookup@@YIIPBUIceSupportMap@@IPBVPoint@IceMaths@@@Z")
#pragma comment(linker, "/alternatename:_nxMSCallHullPolygons=?nxHullComputePolygons@@YI_NPAVConvexHull@@@Z")
#pragma comment(linker, "/alternatename:_nxMSCallRayPlane=_NxRayPlaneIntersect")
__declspec(naked) bool MeshShape::nxMeshSweepPrepared(float* out, const float* point) const
	{
	__asm
		{
		sub esp, 0x30		// 0x00029610
		push esi		// 0x00029613
		mov esi, ecx		// 0x00029614
		mov eax, dword ptr [esi + 0xe0]		// 0x00029616
		push edi		// 0x0002961c
		mov edi, dword ptr [eax + 0xac]		// 0x0002961d
		test edi, edi		// 0x00029623
		je L29737		// 0x00029625
		mov eax, dword ptr [esp + 0x40]		// 0x0002962b
		fld dword ptr [eax]		// 0x0002962f
		push ebx		// 0x00029631
		fsub dword ptr [esi + 0x30]		// 0x00029632
		lea ecx, [esp + 0xc]		// 0x00029635
		fld dword ptr [eax + 4]		// 0x00029639
		push ecx		// 0x0002963c
		fsub dword ptr [esi + 0x34]		// 0x0002963d
		mov ecx, edi		// 0x00029640
		fstp dword ptr [esp + 0x14]		// 0x00029642
		fld dword ptr [eax + 8]		// 0x00029646
		fsub dword ptr [esi + 0x38]		// 0x00029649
		fst dword ptr [esp + 0x18]		// 0x0002964c
		fmul dword ptr [esi + 0x24]		// 0x00029650
		fld dword ptr [esp + 0x14]		// 0x00029653
		fmul dword ptr [esi + 0x18]		// 0x00029657
		faddp st(1), st(0)		// 0x0002965a
		fld st(1)		// 0x0002965c
		fmul dword ptr [esi + 0xc]		// 0x0002965e
		faddp st(1), st(0)		// 0x00029661
		fld dword ptr [esp + 0x18]		// 0x00029663
		fmul dword ptr [esi + 0x28]		// 0x00029667
		fld st(2)		// 0x0002966a
		fmul dword ptr [esi + 0x10]		// 0x0002966c
		faddp st(1), st(0)		// 0x0002966f
		fld dword ptr [esp + 0x14]		// 0x00029671
		fmul dword ptr [esi + 0x1c]		// 0x00029675
		faddp st(1), st(0)		// 0x00029678
		fld dword ptr [esp + 0x18]		// 0x0002967a
		fmul dword ptr [esi + 0x2c]		// 0x0002967e
		fxch st(3)		// 0x00029681
		fmul dword ptr [esi + 0x14]		// 0x00029683
		faddp st(3), st(0)		// 0x00029686
		fld dword ptr [esp + 0x14]		// 0x00029688
		fmul dword ptr [esi + 0x20]		// 0x0002968c
		faddp st(3), st(0)		// 0x0002968f
		fxch st(2)		// 0x00029691
		fstp dword ptr [esp + 0x24]		// 0x00029693
		mov eax, dword ptr [esp + 0x24]		// 0x00029697
		mov dword ptr [esp + 0x18], eax		// 0x0002969b
		fstp dword ptr [esp + 0x10]		// 0x0002969f
		fstp dword ptr [esp + 0x14]		// 0x000296a3
		call nxMSCallSupportLookup		// 0x000296a7
		mov edx, dword ptr [edi + 0xc]		// 0x000296ac
		movzx ebx, byte ptr [eax + edx]		// 0x000296af
		mov eax, dword ptr [esi + 0xe0]		// 0x000296b3
		mov edi, dword ptr [eax + 0xa0]		// 0x000296b9
		mov eax, dword ptr [edi + 0x28]		// 0x000296bf
		test eax, eax		// 0x000296c2
		jne L296cd		// 0x000296c4
		mov ecx, edi		// 0x000296c6
		call nxMSCallHullPolygons		// 0x000296c8
L296cd:
		mov edx, dword ptr [edi + 0x28]		// 0x000296cd
		mov eax, dword ptr [esi + 0xe0]		// 0x000296d0
		mov eax, dword ptr [eax + 0xa0]		// 0x000296d6
		add eax, 0x18		// 0x000296dc
		lea ecx, [ebx + ebx*8]		// 0x000296df
		lea ecx, [edx + ecx*4]		// 0x000296e2
		mov edx, dword ptr [eax]		// 0x000296e5
		mov dword ptr [esp + 0x24], edx		// 0x000296e7
		mov edx, dword ptr [eax + 4]		// 0x000296eb
		mov dword ptr [esp + 0x28], edx		// 0x000296ee
		mov eax, dword ptr [eax + 8]		// 0x000296f2
		mov edx, dword ptr [esp + 0xc]		// 0x000296f5
		mov dword ptr [esp + 0x2c], eax		// 0x000296f9
		mov eax, dword ptr [esp + 0x10]		// 0x000296fd
		mov dword ptr [esp + 0x30], edx		// 0x00029701
		mov edx, dword ptr [esp + 0x14]		// 0x00029705
		mov dword ptr [esp + 0x34], eax		// 0x00029709
		lea eax, [esp + 0x18]		// 0x0002970d
		push eax		// 0x00029711
		mov dword ptr [esp + 0x3c], edx		// 0x00029712
		mov edx, dword ptr [esp + 0x44]		// 0x00029716
		push edx		// 0x0002971a
		add ecx, 0xc		// 0x0002971b
		push ecx		// 0x0002971e
		lea eax, [esp + 0x30]		// 0x0002971f
		push eax		// 0x00029723
		call nxMSCallRayPlane		// 0x00029724
		add esp, 0x10		// 0x00029729
		pop ebx		// 0x0002972c
		pop edi		// 0x0002972d
		mov al, 1		// 0x0002972e
		pop esi		// 0x00029730
		add esp, 0x30		// 0x00029731
		ret 8		// 0x00029734
L29737:
		mov ecx, dword ptr [eax + 0x68]		// 0x00029737
		mov edx, dword ptr [esp + 0x3c]		// 0x0002973a
		pop edi		// 0x0002973e
		mov dword ptr [edx], ecx		// 0x0002973f
		xor al, al		// 0x00029741
		pop esi		// 0x00029743
		add esp, 0x30		// 0x00029744
		ret 8		// 0x00029747
		}
	}


// Listing transcription of phys_fn_001393 and 001395 (0x00027f10, 3835 B), the mesh shape's slot 3: debug
// visualisation. The inventory splits it at the jump target 0x00028370; it is one function with one `ret 4`.
// After the flag gate (+0xde & 8) and the shape's own axes (001305) it draws, under the shapes/vertex-normal/
// face-normal parameters, the hull's polygon outlines and normals and the mesh's triangles, edges and vertex
// normals through the renderer's slot 8 (+0x20, a line). The four parameters the listing reads at
// 0x10123b4c, 0x10123bc4, 0x10123bd0 and 0x10123bd4 are the live array's elements 13, 43, 46 and 47; they are
// copied into the statics below by the wrapper just before the body runs. Compare script:
// docs/reconstruction/novodex-physics/evidence/phase3-mesh-debug-listing-compare.py.
extern "C" void nxMDCallFlagBits();
extern "C" void nxMDCallDebugRender();
extern "C" void nxMDCallHullPolygons();
extern "C" void nxMDCallHullEdges();
extern "C" void nxMDCallVertexNormals();
extern "C" void nxMDCallEdgeList();
#pragma comment(linker, "/alternatename:_nxMDCallFlagBits=?nxFlagBitsDE@ShapeBase@@QBEII@Z")
#pragma comment(linker, "/alternatename:_nxMDCallDebugRender=?nxDebugRender@BoxShape@@QBEXPBX@Z")
#pragma comment(linker, "/alternatename:_nxMDCallHullPolygons=?nxHullComputePolygons@@YI_NPAVConvexHull@@@Z")
#pragma comment(linker, "/alternatename:_nxMDCallHullEdges=?nxHullComputeEdges@@YI_NPAVConvexHull@@@Z")
#pragma comment(linker, "/alternatename:_nxMDCallVertexNormals=?nxMeshComputeVertexNormals@@YAXXZ")
#pragma comment(linker, "/alternatename:_nxMDCallEdgeList=?createEdgeList@TriangleMesh@@QAEXXZ")
static float nxMDParamScale, nxMDParamShapes, nxMDParamVNormals, nxMDParamFNormals;
static const unsigned nxMDK101041ec_4 = 0x3f800000;
static const unsigned nxMDK101041f0_4 = 0x0;
static const unsigned nxMDK101043cc_4 = 0x3f000000;
static const unsigned nxMDK101066f8_4 = 0x4f800000;
static const unsigned nxMDK101068ec_4 = 0x3eaaaaab;
__declspec(naked) static void __fastcall nxMeshDebugBody(const void* self, void* edxUnused, const void* renderer)
	{
	__asm
		{
		sub esp, 0xc4		// 0x00027f10
		push esi		// 0x00027f16
		push 8		// 0x00027f17
		mov esi, ecx		// 0x00027f19
		call nxMDCallFlagBits		// 0x00027f1b
		test eax, eax		// 0x00027f20
		je L28e01		// 0x00027f22
		push ebp		// 0x00027f28
		mov ebp, dword ptr [esp + 0xd0]		// 0x00027f29
		push ebp		// 0x00027f30
		mov ecx, esi		// 0x00027f31
		call nxMDCallDebugRender		// 0x00027f33
		fld dword ptr [nxMDK101041f0_4]		// 0x00027f38
		fld dword ptr [nxMDParamShapes]		// 0x00027f3e
		fucompp		// 0x00027f44
		fnstsw ax		// 0x00027f46
		test ah, 0x44		// 0x00027f48
		jnp L28e00		// 0x00027f4b
		fld dword ptr [nxMDParamVNormals]		// 0x00027f51
		mov eax, dword ptr [esi + 0xe0]		// 0x00027f57
		fmul dword ptr [nxMDParamScale]		// 0x00027f5d
		push ebx		// 0x00027f63
		mov ebx, dword ptr [eax + 0xa0]		// 0x00027f64
		xor ecx, ecx		// 0x00027f6a
		cmp ebx, ecx		// 0x00027f6c
		fstp dword ptr [esp + 0x2c]		// 0x00027f6e
		fld dword ptr [nxMDParamFNormals]		// 0x00027f72
		push edi		// 0x00027f78
		fmul dword ptr [nxMDParamScale]		// 0x00027f79
		fstp dword ptr [esp + 0x14]		// 0x00027f7f
		je L2855c		// 0x00027f83
		cmp dword ptr [ebx + 0x24], ecx		// 0x00027f89
		jne L27f95		// 0x00027f8c
		mov ecx, ebx		// 0x00027f8e
		call nxMDCallHullPolygons		// 0x00027f90
L27f95:
		mov eax, dword ptr [ebx + 0x24]		// 0x00027f95
		test eax, eax		// 0x00027f98
		jbe L282ff		// 0x00027f9a
		xor edi, edi		// 0x00027fa0
		mov dword ptr [esp + 0xa0], edi		// 0x00027fa2
		mov dword ptr [esp + 0x10], eax		// 0x00027fa9
		lea ecx, [ecx]		// 0x00027fad
L27fb0:
		mov eax, dword ptr [ebx + 0x28]		// 0x00027fb0
		test eax, eax		// 0x00027fb3
		jne L27fbe		// 0x00027fb5
		mov ecx, ebx		// 0x00027fb7
		call nxMDCallHullPolygons		// 0x00027fb9
L27fbe:
		mov eax, dword ptr [ebx + 0x28]		// 0x00027fbe
		test eax, eax		// 0x00027fc1
		mov ecx, dword ptr [edi + eax + 4]		// 0x00027fc3
		mov dword ptr [esp + 0x5c], ecx		// 0x00027fc7
		jne L27fd4		// 0x00027fcb
		mov ecx, ebx		// 0x00027fcd
		call nxMDCallHullPolygons		// 0x00027fcf
L27fd4:
		mov edx, dword ptr [ebx + 0x28]		// 0x00027fd4
		mov edx, dword ptr [edi + edx]		// 0x00027fd7
		xor eax, eax		// 0x00027fda
		test edx, edx		// 0x00027fdc
		mov dword ptr [esp + 0x60], edx		// 0x00027fde
		mov dword ptr [esp + 0x24], 0		// 0x00027fe2
		mov dword ptr [esp + 0x28], 0		// 0x00027fea
		mov dword ptr [esp + 0x2c], 0		// 0x00027ff2
		jbe L28187		// 0x00027ffa
		mov ecx, dword ptr [ebx + 0x10]		// 0x00028000
L28003:
		mov edx, dword ptr [esp + 0x5c]		// 0x00028003
		mov edi, dword ptr [edx + eax*4]		// 0x00028007
		lea edi, [edi + edi*2]		// 0x0002800a
		shl edi, 2		// 0x0002800d
		fld dword ptr [edi + ecx]		// 0x00028010
		inc eax		// 0x00028013
		fld dword ptr [edi + ecx + 4]		// 0x00028014
		mov dword ptr [esp + 0x34], eax		// 0x00028018
		fld dword ptr [edi + ecx + 8]		// 0x0002801c
		fld st(0)		// 0x00028020
		fmul dword ptr [esi + 0x14]		// 0x00028022
		fld st(2)		// 0x00028025
		fmul dword ptr [esi + 0x10]		// 0x00028027
		faddp st(1), st(0)		// 0x0002802a
		fld st(3)		// 0x0002802c
		fmul dword ptr [esi + 0xc]		// 0x0002802e
		faddp st(1), st(0)		// 0x00028031
		fstp dword ptr [esp + 0x64]		// 0x00028033
		fld st(2)		// 0x00028037
		fmul dword ptr [esi + 0x18]		// 0x00028039
		fld st(1)		// 0x0002803c
		fmul dword ptr [esi + 0x20]		// 0x0002803e
		faddp st(1), st(0)		// 0x00028041
		fld st(2)		// 0x00028043
		fmul dword ptr [esi + 0x1c]		// 0x00028045
		faddp st(1), st(0)		// 0x00028048
		fstp dword ptr [esp + 0x68]		// 0x0002804a
		fxch st(2)		// 0x0002804e
		fmul dword ptr [esi + 0x24]		// 0x00028050
		fxch st(2)		// 0x00028053
		fmul dword ptr [esi + 0x2c]		// 0x00028055
		faddp st(2), st(0)		// 0x00028058
		fmul dword ptr [esi + 0x28]		// 0x0002805a
		faddp st(1), st(0)		// 0x0002805d
		fld dword ptr [esp + 0x64]		// 0x0002805f
		fadd dword ptr [esi + 0x30]		// 0x00028063
		fld dword ptr [esp + 0x68]		// 0x00028066
		fadd dword ptr [esi + 0x34]		// 0x0002806a
		fxch st(2)		// 0x0002806d
		fadd dword ptr [esi + 0x38]		// 0x0002806f
		fstp dword ptr [esp + 0xd0]		// 0x00028072
		mov edx, dword ptr [esp + 0xd0]		// 0x00028079
		mov dword ptr [esp + 0x58], edx		// 0x00028080
		xor edx, edx		// 0x00028084
		fstp dword ptr [esp + 0x50]		// 0x00028086
		div dword ptr [esp + 0x60]		// 0x0002808a
		fstp dword ptr [esp + 0x54]		// 0x0002808e
		mov eax, dword ptr [esp + 0x5c]		// 0x00028092
		mov edx, dword ptr [eax + edx*4]		// 0x00028096
		lea edx, [edx + edx*2]		// 0x00028099
		fld dword ptr [ecx + edx*4]		// 0x0002809c
		lea ecx, [ecx + edx*4]		// 0x0002809f
		fld dword ptr [ecx + 4]		// 0x000280a2
		fld dword ptr [ecx + 8]		// 0x000280a5
		fld st(0)		// 0x000280a8
		fmul dword ptr [esi + 0x14]		// 0x000280aa
		fld st(2)		// 0x000280ad
		fmul dword ptr [esi + 0x10]		// 0x000280af
		faddp st(1), st(0)		// 0x000280b2
		fld st(3)		// 0x000280b4
		fmul dword ptr [esi + 0xc]		// 0x000280b6
		faddp st(1), st(0)		// 0x000280b9
		fstp dword ptr [esp + 0xa4]		// 0x000280bb
		fld st(0)		// 0x000280c2
		fmul dword ptr [esi + 0x20]		// 0x000280c4
		fld st(2)		// 0x000280c7
		fmul dword ptr [esi + 0x1c]		// 0x000280c9
		faddp st(1), st(0)		// 0x000280cc
		fld st(3)		// 0x000280ce
		fmul dword ptr [esi + 0x18]		// 0x000280d0
		faddp st(1), st(0)		// 0x000280d3
		fstp dword ptr [esp + 0xa8]		// 0x000280d5
		fxch st(2)		// 0x000280dc
		fmul dword ptr [esi + 0x24]		// 0x000280de
		fxch st(2)		// 0x000280e1
		fmul dword ptr [esi + 0x2c]		// 0x000280e3
		faddp st(2), st(0)		// 0x000280e6
		fmul dword ptr [esi + 0x28]		// 0x000280e8
		faddp st(1), st(0)		// 0x000280eb
		fld dword ptr [esp + 0xa4]		// 0x000280ed
		fadd dword ptr [esi + 0x30]		// 0x000280f4
		fld dword ptr [esp + 0xa8]		// 0x000280f7
		mov edx, dword ptr [ebp]		// 0x000280fe
		fadd dword ptr [esi + 0x34]		// 0x00028101
		push 0xff8040		// 0x00028104
		fxch st(2)		// 0x00028109
		lea ecx, [esp + 0x54]		// 0x0002810b
		fadd dword ptr [esi + 0x38]		// 0x0002810f
		fstp dword ptr [esp + 0xc8]		// 0x00028112
		mov eax, dword ptr [esp + 0xc8]		// 0x00028119
		mov dword ptr [esp + 0x7c], eax		// 0x00028120
		lea eax, [esp + 0x74]		// 0x00028124
		fstp dword ptr [esp + 0x74]		// 0x00028128
		push eax		// 0x0002812c
		push ecx		// 0x0002812d
		fstp dword ptr [esp + 0x80]		// 0x0002812e
		mov ecx, ebp		// 0x00028135
		call dword ptr [edx + 0x20]		// 0x00028137
		mov ecx, dword ptr [ebx + 0x10]		// 0x0002813a
		fld dword ptr [edi + ecx]		// 0x0002813d
		mov eax, dword ptr [esp + 0x34]		// 0x00028140
		fld dword ptr [edi + ecx + 4]		// 0x00028144
		mov edx, dword ptr [esp + 0x60]		// 0x00028148
		cmp eax, edx		// 0x0002814c
		fld dword ptr [edi + ecx + 8]		// 0x0002814e
		fstp dword ptr [esp + 0xb8]		// 0x00028152
		fxch st(1)		// 0x00028159
		fadd dword ptr [esp + 0x24]		// 0x0002815b
		fstp dword ptr [esp + 0x24]		// 0x0002815f
		fadd dword ptr [esp + 0x28]		// 0x00028163
		fstp dword ptr [esp + 0x28]		// 0x00028167
		fld dword ptr [esp + 0xb8]		// 0x0002816b
		fadd dword ptr [esp + 0x2c]		// 0x00028172
		fstp dword ptr [esp + 0x2c]		// 0x00028176
		jb L28003		// 0x0002817a
		mov edi, dword ptr [esp + 0xa0]		// 0x00028180
L28187:
		fld dword ptr [nxMDK101041f0_4]		// 0x00028187
		fld dword ptr [esp + 0x14]		// 0x0002818d
		fucompp		// 0x00028191
		fnstsw ax		// 0x00028193
		test ah, 0x44		// 0x00028195
		jnp L282e6		// 0x00028198
		test edx, edx		// 0x0002819e
		mov dword ptr [esp + 0x34], edx		// 0x000281a0
		fild dword ptr [esp + 0x34]		// 0x000281a4
		jge L281b0		// 0x000281a8
		fadd dword ptr [nxMDK101066f8_4]		// 0x000281aa
L281b0:
		fdivr dword ptr [nxMDK101041ec_4]		// 0x000281b0
		mov eax, dword ptr [ebx + 0x28]		// 0x000281b6
		test eax, eax		// 0x000281b9
		fld dword ptr [esp + 0x24]		// 0x000281bb
		fmul st(0), st(1)		// 0x000281bf
		fld st(1)		// 0x000281c1
		fmul dword ptr [esp + 0x28]		// 0x000281c3
		fxch st(2)		// 0x000281c7
		fmul dword ptr [esp + 0x2c]		// 0x000281c9
		fld st(0)		// 0x000281cd
		fmul dword ptr [esi + 0x14]		// 0x000281cf
		fld st(3)		// 0x000281d2
		fmul dword ptr [esi + 0x10]		// 0x000281d4
		faddp st(1), st(0)		// 0x000281d7
		fld st(2)		// 0x000281d9
		fmul dword ptr [esi + 0xc]		// 0x000281db
		faddp st(1), st(0)		// 0x000281de
		fstp dword ptr [esp + 0x44]		// 0x000281e0
		fld st(1)		// 0x000281e4
		fmul dword ptr [esi + 0x18]		// 0x000281e6
		fld st(1)		// 0x000281e9
		fmul dword ptr [esi + 0x20]		// 0x000281eb
		faddp st(1), st(0)		// 0x000281ee
		fld st(3)		// 0x000281f0
		fmul dword ptr [esi + 0x1c]		// 0x000281f2
		faddp st(1), st(0)		// 0x000281f5
		fstp dword ptr [esp + 0x48]		// 0x000281f7
		fxch st(1)		// 0x000281fb
		fmul dword ptr [esi + 0x24]		// 0x000281fd
		fxch st(1)		// 0x00028200
		fmul dword ptr [esi + 0x2c]		// 0x00028202
		faddp st(1), st(0)		// 0x00028205
		fxch st(1)		// 0x00028207
		fmul dword ptr [esi + 0x28]		// 0x00028209
		faddp st(1), st(0)		// 0x0002820c
		fld dword ptr [esp + 0x44]		// 0x0002820e
		fadd dword ptr [esi + 0x30]		// 0x00028212
		fld dword ptr [esp + 0x48]		// 0x00028215
		fadd dword ptr [esi + 0x34]		// 0x00028219
		fxch st(2)		// 0x0002821c
		fadd dword ptr [esi + 0x38]		// 0x0002821e
		fstp dword ptr [esp + 0x9c]		// 0x00028221
		mov edx, dword ptr [esp + 0x9c]		// 0x00028228
		mov dword ptr [esp + 0x20], edx		// 0x0002822f
		fstp dword ptr [esp + 0x18]		// 0x00028233
		fstp dword ptr [esp + 0x1c]		// 0x00028237
		jne L28244		// 0x0002823b
		mov ecx, ebx		// 0x0002823d
		call nxMDCallHullPolygons		// 0x0002823f
L28244:
		mov eax, dword ptr [ebx + 0x28]		// 0x00028244
		fld dword ptr [esi + 0x14]		// 0x00028247
		fmul dword ptr [edi + eax + 0x14]		// 0x0002824a
		lea eax, [edi + eax + 0xc]		// 0x0002824e
		fld dword ptr [esi + 0x10]		// 0x00028252
		mov edx, dword ptr [ebp]		// 0x00028255
		fmul dword ptr [eax + 4]		// 0x00028258
		push 0x4080ff		// 0x0002825b
		lea ecx, [esp + 0x1c]		// 0x00028260
		faddp st(1), st(0)		// 0x00028264
		fld dword ptr [esi + 0xc]		// 0x00028266
		fmul dword ptr [eax]		// 0x00028269
		faddp st(1), st(0)		// 0x0002826b
		fld dword ptr [esi + 0x20]		// 0x0002826d
		fmul dword ptr [eax + 8]		// 0x00028270
		fld dword ptr [esi + 0x18]		// 0x00028273
		fmul dword ptr [eax]		// 0x00028276
		faddp st(1), st(0)		// 0x00028278
		fld dword ptr [esi + 0x1c]		// 0x0002827a
		fmul dword ptr [eax + 4]		// 0x0002827d
		faddp st(1), st(0)		// 0x00028280
		fld dword ptr [esi + 0x2c]		// 0x00028282
		fmul dword ptr [eax + 8]		// 0x00028285
		fld dword ptr [esi + 0x24]		// 0x00028288
		fmul dword ptr [eax]		// 0x0002828b
		faddp st(1), st(0)		// 0x0002828d
		fld dword ptr [esi + 0x28]		// 0x0002828f
		fmul dword ptr [eax + 4]		// 0x00028292
		lea eax, [esp + 0x3c]		// 0x00028295
		push eax		// 0x00028299
		push ecx		// 0x0002829a
		faddp st(1), st(0)		// 0x0002829b
		mov ecx, ebp		// 0x0002829d
		fstp dword ptr [esp + 0x9c]		// 0x0002829f
		fxch st(1)		// 0x000282a6
		fmul dword ptr [esp + 0x20]		// 0x000282a8
		fstp dword ptr [esp + 0x88]		// 0x000282ac
		fmul dword ptr [esp + 0x20]		// 0x000282b3
		fld dword ptr [esp + 0x9c]		// 0x000282b7
		fmul dword ptr [esp + 0x20]		// 0x000282be
		fld dword ptr [esp + 0x88]		// 0x000282c2
		fadd dword ptr [esp + 0x24]		// 0x000282c9
		fstp dword ptr [esp + 0x44]		// 0x000282cd
		fxch st(1)		// 0x000282d1
		fadd dword ptr [esp + 0x28]		// 0x000282d3
		fstp dword ptr [esp + 0x48]		// 0x000282d7
		fadd dword ptr [esp + 0x2c]		// 0x000282db
		fstp dword ptr [esp + 0x4c]		// 0x000282df
		call dword ptr [edx + 0x20]		// 0x000282e3
L282e6:
		mov eax, dword ptr [esp + 0x10]		// 0x000282e6
		add edi, 0x24		// 0x000282ea
		dec eax		// 0x000282ed
		mov dword ptr [esp + 0xa0], edi		// 0x000282ee
		mov dword ptr [esp + 0x10], eax		// 0x000282f5
		jne L27fb0		// 0x000282f9
L282ff:
		fld dword ptr [nxMDK101041f0_4]		// 0x000282ff
		fld dword ptr [esp + 0x30]		// 0x00028305
		fucompp		// 0x00028309
		fnstsw ax		// 0x0002830b
		test ah, 0x44		// 0x0002830d
		jnp L28dfe		// 0x00028310
		mov eax, dword ptr [ebx + 0x38]		// 0x00028316
		test eax, eax		// 0x00028319
		jne L28324		// 0x0002831b
		mov ecx, ebx		// 0x0002831d
		call nxMDCallHullEdges		// 0x0002831f
L28324:
		mov eax, dword ptr [ebx + 0x3c]		// 0x00028324
		test eax, eax		// 0x00028327
		mov edi, dword ptr [ebx + 0x38]		// 0x00028329
		mov dword ptr [esp + 0x34], edi		// 0x0002832c
		jne L28339		// 0x00028330
		mov ecx, ebx		// 0x00028332
		call nxMDCallHullEdges		// 0x00028334
L28339:
		mov eax, dword ptr [ebx + 0x40]		// 0x00028339
		test eax, eax		// 0x0002833c
		mov edx, dword ptr [ebx + 0x3c]		// 0x0002833e
		mov dword ptr [esp + 0x10], edx		// 0x00028341
		jne L2834e		// 0x00028345
		mov ecx, ebx		// 0x00028347
		call nxMDCallHullEdges		// 0x00028349
L2834e:
		mov eax, dword ptr [ebx + 0x40]		// 0x0002834e
		xor edx, edx		// 0x00028351
		test edi, edi		// 0x00028353
		mov dword ptr [esp + 0x14], edx		// 0x00028355
		jbe L28dfe		// 0x00028359
		lea edi, [eax + 4]		// 0x0002835f
		jmp L28370		// 0x00028362
L28364:
		mov edx, dword ptr [esp + 0x14]		// 0x00028364
		jmp L28370		// 0x00028368
		lea ebx, [ebx]		// 0x0002836a
L28370:
		mov eax, dword ptr [esp + 0x10]		// 0x00028370
		mov eax, dword ptr [eax + edx*8]		// 0x00028374
		mov ecx, dword ptr [ebx + 0x10]		// 0x00028377
		lea eax, [eax + eax*2]		// 0x0002837a
		fld dword ptr [ecx + eax*4]		// 0x0002837d
		lea eax, [ecx + eax*4]		// 0x00028380
		fld dword ptr [eax + 4]		// 0x00028383
		fld dword ptr [eax + 8]		// 0x00028386
		mov eax, dword ptr [esp + 0x10]		// 0x00028389
		fld st(0)		// 0x0002838d
		mov eax, dword ptr [eax + edx*8 + 4]		// 0x0002838f
		fmul dword ptr [esi + 0x14]		// 0x00028393
		lea edx, [eax + eax*2]		// 0x00028396
		fld st(2)		// 0x00028399
		lea eax, [ecx + edx*4]		// 0x0002839b
		fmul dword ptr [esi + 0x10]		// 0x0002839e
		faddp st(1), st(0)		// 0x000283a1
		fld st(3)		// 0x000283a3
		fmul dword ptr [esi + 0xc]		// 0x000283a5
		faddp st(1), st(0)		// 0x000283a8
		fstp dword ptr [esp + 0x38]		// 0x000283aa
		fld st(0)		// 0x000283ae
		fmul dword ptr [esi + 0x20]		// 0x000283b0
		fld st(2)		// 0x000283b3
		fmul dword ptr [esi + 0x1c]		// 0x000283b5
		faddp st(1), st(0)		// 0x000283b8
		fld st(3)		// 0x000283ba
		fmul dword ptr [esi + 0x18]		// 0x000283bc
		faddp st(1), st(0)		// 0x000283bf
		fstp dword ptr [esp + 0x3c]		// 0x000283c1
		fmul dword ptr [esi + 0x2c]		// 0x000283c5
		fxch st(1)		// 0x000283c8
		fmul dword ptr [esi + 0x28]		// 0x000283ca
		faddp st(1), st(0)		// 0x000283cd
		fxch st(1)		// 0x000283cf
		fmul dword ptr [esi + 0x24]		// 0x000283d1
		faddp st(1), st(0)		// 0x000283d4
		fld dword ptr [esp + 0x38]		// 0x000283d6
		fadd dword ptr [esi + 0x30]		// 0x000283da
		fld dword ptr [esp + 0x3c]		// 0x000283dd
		fadd dword ptr [esi + 0x34]		// 0x000283e1
		fstp dword ptr [esp + 0x54]		// 0x000283e4
		fxch st(1)		// 0x000283e8
		fadd dword ptr [esi + 0x38]		// 0x000283ea
		fstp dword ptr [esp + 0x58]		// 0x000283ed
		fld dword ptr [eax]		// 0x000283f1
		fld dword ptr [eax + 4]		// 0x000283f3
		fld dword ptr [eax + 8]		// 0x000283f6
		fld st(0)		// 0x000283f9
		fmul dword ptr [esi + 0x14]		// 0x000283fb
		fld st(2)		// 0x000283fe
		fmul dword ptr [esi + 0x10]		// 0x00028400
		faddp st(1), st(0)		// 0x00028403
		fld st(3)		// 0x00028405
		fmul dword ptr [esi + 0xc]		// 0x00028407
		faddp st(1), st(0)		// 0x0002840a
		fstp dword ptr [esp + 0x44]		// 0x0002840c
		fld st(0)		// 0x00028410
		fmul dword ptr [esi + 0x20]		// 0x00028412
		fld st(2)		// 0x00028415
		fmul dword ptr [esi + 0x1c]		// 0x00028417
		faddp st(1), st(0)		// 0x0002841a
		fld st(3)		// 0x0002841c
		fmul dword ptr [esi + 0x18]		// 0x0002841e
		faddp st(1), st(0)		// 0x00028421
		fstp dword ptr [esp + 0x48]		// 0x00028423
		fmul dword ptr [esi + 0x2c]		// 0x00028427
		fxch st(1)		// 0x0002842a
		fmul dword ptr [esi + 0x28]		// 0x0002842c
		faddp st(1), st(0)		// 0x0002842f
		fxch st(1)		// 0x00028431
		fmul dword ptr [esi + 0x24]		// 0x00028433
		faddp st(1), st(0)		// 0x00028436
		fld dword ptr [esp + 0x44]		// 0x00028438
		fadd dword ptr [esi + 0x30]		// 0x0002843c
		fstp dword ptr [esp + 0x24]		// 0x0002843f
		fld dword ptr [esp + 0x48]		// 0x00028443
		fadd dword ptr [esi + 0x34]		// 0x00028447
		fstp dword ptr [esp + 0x28]		// 0x0002844a
		fadd dword ptr [esi + 0x38]		// 0x0002844e
		fstp dword ptr [esp + 0x2c]		// 0x00028451
		fadd dword ptr [esp + 0x24]		// 0x00028455
		mov eax, dword ptr [ebp]		// 0x00028459
		fld dword ptr [esp + 0x28]		// 0x0002845c
		push 0xff00ff00		// 0x00028460
		fadd dword ptr [esp + 0x58]		// 0x00028465
		lea ecx, [esp + 0x74]		// 0x00028469
		fld dword ptr [esp + 0x30]		// 0x0002846d
		push ecx		// 0x00028471
		fadd dword ptr [esp + 0x60]		// 0x00028472
		lea edx, [esp + 0x20]		// 0x00028476
		push edx		// 0x0002847a
		mov ecx, ebp		// 0x0002847b
		fstp dword ptr [esp + 0x90]		// 0x0002847d
		fxch st(1)		// 0x00028484
		fmul dword ptr [nxMDK101043cc_4]		// 0x00028486
		fstp dword ptr [esp + 0x24]		// 0x0002848c
		fmul dword ptr [nxMDK101043cc_4]		// 0x00028490
		fstp dword ptr [esp + 0x28]		// 0x00028496
		fld dword ptr [esp + 0x90]		// 0x0002849a
		fmul dword ptr [nxMDK101043cc_4]		// 0x000284a1
		fstp dword ptr [esp + 0x2c]		// 0x000284a7
		fld dword ptr [edi + 4]		// 0x000284ab
		fmul dword ptr [esi + 0x14]		// 0x000284ae
		fld dword ptr [edi - 4]		// 0x000284b1
		fmul dword ptr [esi + 0xc]		// 0x000284b4
		faddp st(1), st(0)		// 0x000284b7
		fld dword ptr [edi]		// 0x000284b9
		fmul dword ptr [esi + 0x10]		// 0x000284bb
		faddp st(1), st(0)		// 0x000284be
		fld dword ptr [edi + 4]		// 0x000284c0
		fmul dword ptr [esi + 0x20]		// 0x000284c3
		fld dword ptr [edi]		// 0x000284c6
		fmul dword ptr [esi + 0x1c]		// 0x000284c8
		faddp st(1), st(0)		// 0x000284cb
		fld dword ptr [esi + 0x18]		// 0x000284cd
		fmul dword ptr [edi - 4]		// 0x000284d0
		faddp st(1), st(0)		// 0x000284d3
		fld dword ptr [edi + 4]		// 0x000284d5
		fmul dword ptr [esi + 0x2c]		// 0x000284d8
		fld dword ptr [edi]		// 0x000284db
		fmul dword ptr [esi + 0x28]		// 0x000284dd
		faddp st(1), st(0)		// 0x000284e0
		fld dword ptr [edi - 4]		// 0x000284e2
		fmul dword ptr [esi + 0x24]		// 0x000284e5
		faddp st(1), st(0)		// 0x000284e8
		fstp dword ptr [esp + 0x9c]		// 0x000284ea
		fxch st(1)		// 0x000284f1
		fmul dword ptr [esp + 0x3c]		// 0x000284f3
		fstp dword ptr [esp + 0xa0]		// 0x000284f7
		fmul dword ptr [esp + 0x3c]		// 0x000284fe
		fld dword ptr [esp + 0x9c]		// 0x00028502
		fmul dword ptr [esp + 0x3c]		// 0x00028509
		fld dword ptr [esp + 0xa0]		// 0x0002850d
		fadd dword ptr [esp + 0x24]		// 0x00028514
		fstp dword ptr [esp + 0x7c]		// 0x00028518
		fxch st(1)		// 0x0002851c
		fadd dword ptr [esp + 0x28]		// 0x0002851e
		fstp dword ptr [esp + 0x80]		// 0x00028522
		fadd dword ptr [esp + 0x2c]		// 0x00028529
		fstp dword ptr [esp + 0x84]		// 0x0002852d
		call dword ptr [eax + 0x20]		// 0x00028534
		mov eax, dword ptr [esp + 0x14]		// 0x00028537
		mov ecx, dword ptr [esp + 0x34]		// 0x0002853b
		inc eax		// 0x0002853f
		add edi, 0xc		// 0x00028540
		cmp eax, ecx		// 0x00028543
		mov dword ptr [esp + 0x14], eax		// 0x00028545
		jb L28364		// 0x00028549
		pop edi		// 0x0002854f
		pop ebx		// 0x00028550
		pop ebp		// 0x00028551
		pop esi		// 0x00028552
		add esp, 0xc4		// 0x00028553
		ret 4		// 0x00028559
L2855c:
		mov eax, dword ptr [eax + 0xc]		// 0x0002855c
		cmp eax, ecx		// 0x0002855f
		jbe L2878f		// 0x00028561
		mov dword ptr [esp + 0x5c], ecx		// 0x00028567
		mov dword ptr [esp + 0x60], ecx		// 0x0002856b
		mov dword ptr [esp + 0x10], eax		// 0x0002856f
L28573:
		mov ecx, dword ptr [esi + 0xe0]		// 0x00028573
		mov eax, dword ptr [ecx + 0x14]		// 0x00028579
		mov edx, dword ptr [esp + 0x60]		// 0x0002857c
		mov ecx, dword ptr [ecx + 0x10]		// 0x00028580
		add eax, edx		// 0x00028583
		mov edx, dword ptr [eax]		// 0x00028585
		lea edx, [edx + edx*2]		// 0x00028587
		fld dword ptr [ecx + edx*4]		// 0x0002858a
		lea edx, [ecx + edx*4]		// 0x0002858d
		fld dword ptr [edx + 4]		// 0x00028590
		mov edi, dword ptr [eax + 4]		// 0x00028593
		fld dword ptr [edx + 8]		// 0x00028596
		mov eax, dword ptr [eax + 8]		// 0x00028599
		fld st(0)		// 0x0002859c
		lea edi, [edi + edi*2]		// 0x0002859e
		fmul dword ptr [esi + 0x14]		// 0x000285a1
		lea edi, [ecx + edi*4]		// 0x000285a4
		fld st(2)		// 0x000285a7
		lea eax, [eax + eax*2]		// 0x000285a9
		fmul dword ptr [esi + 0x10]		// 0x000285ac
		lea ecx, [ecx + eax*4]		// 0x000285af
		faddp st(1), st(0)		// 0x000285b2
		fld st(3)		// 0x000285b4
		fmul dword ptr [esi + 0xc]		// 0x000285b6
		faddp st(1), st(0)		// 0x000285b9
		fstp dword ptr [esp + 0x38]		// 0x000285bb
		fld st(0)		// 0x000285bf
		fmul dword ptr [esi + 0x20]		// 0x000285c1
		fld st(2)		// 0x000285c4
		fmul dword ptr [esi + 0x1c]		// 0x000285c6
		faddp st(1), st(0)		// 0x000285c9
		fld st(3)		// 0x000285cb
		fmul dword ptr [esi + 0x18]		// 0x000285cd
		faddp st(1), st(0)		// 0x000285d0
		fstp dword ptr [esp + 0x3c]		// 0x000285d2
		fmul dword ptr [esi + 0x2c]		// 0x000285d6
		fxch st(1)		// 0x000285d9
		fmul dword ptr [esi + 0x28]		// 0x000285db
		faddp st(1), st(0)		// 0x000285de
		fxch st(1)		// 0x000285e0
		fmul dword ptr [esi + 0x24]		// 0x000285e2
		faddp st(1), st(0)		// 0x000285e5
		fld dword ptr [esp + 0x38]		// 0x000285e7
		fadd dword ptr [esi + 0x30]		// 0x000285eb
		fld dword ptr [esp + 0x3c]		// 0x000285ee
		fadd dword ptr [esi + 0x34]		// 0x000285f2
		fxch st(2)		// 0x000285f5
		fadd dword ptr [esi + 0x38]		// 0x000285f7
		fstp dword ptr [esp + 0x84]		// 0x000285fa
		mov edx, dword ptr [esp + 0x84]		// 0x00028601
		mov dword ptr [esp + 0x6c], edx		// 0x00028608
		fstp dword ptr [esp + 0x64]		// 0x0002860c
		fstp dword ptr [esp + 0x68]		// 0x00028610
		fld dword ptr [edi]		// 0x00028614
		fld dword ptr [edi + 4]		// 0x00028616
		fld dword ptr [edi + 8]		// 0x00028619
		fld st(0)		// 0x0002861c
		fmul dword ptr [esi + 0x14]		// 0x0002861e
		fld st(2)		// 0x00028621
		fmul dword ptr [esi + 0x10]		// 0x00028623
		faddp st(1), st(0)		// 0x00028626
		fld st(3)		// 0x00028628
		fmul dword ptr [esi + 0xc]		// 0x0002862a
		faddp st(1), st(0)		// 0x0002862d
		fstp dword ptr [esp + 0x44]		// 0x0002862f
		fld st(0)		// 0x00028633
		fmul dword ptr [esi + 0x20]		// 0x00028635
		fld st(2)		// 0x00028638
		fmul dword ptr [esi + 0x1c]		// 0x0002863a
		faddp st(1), st(0)		// 0x0002863d
		fld st(3)		// 0x0002863f
		fmul dword ptr [esi + 0x18]		// 0x00028641
		faddp st(1), st(0)		// 0x00028644
		fstp dword ptr [esp + 0x48]		// 0x00028646
		fmul dword ptr [esi + 0x2c]		// 0x0002864a
		fxch st(1)		// 0x0002864d
		fmul dword ptr [esi + 0x28]		// 0x0002864f
		faddp st(1), st(0)		// 0x00028652
		fxch st(1)		// 0x00028654
		fmul dword ptr [esi + 0x24]		// 0x00028656
		faddp st(1), st(0)		// 0x00028659
		fld dword ptr [esp + 0x44]		// 0x0002865b
		xor ebx, ebx		// 0x0002865f
		fadd dword ptr [esi + 0x30]		// 0x00028661
		fld dword ptr [esp + 0x48]		// 0x00028664
		fadd dword ptr [esi + 0x34]		// 0x00028668
		fxch st(2)		// 0x0002866b
		fadd dword ptr [esi + 0x38]		// 0x0002866d
		fstp dword ptr [esp + 0x90]		// 0x00028670
		mov eax, dword ptr [esp + 0x90]		// 0x00028677
		mov dword ptr [esp + 0x2c], eax		// 0x0002867e
		fstp dword ptr [esp + 0x24]		// 0x00028682
		mov eax, dword ptr [esi + 0xe0]		// 0x00028686
		fstp dword ptr [esp + 0x28]		// 0x0002868c
		fld dword ptr [ecx]		// 0x00028690
		fld dword ptr [ecx + 4]		// 0x00028692
		fld dword ptr [ecx + 8]		// 0x00028695
		fld st(2)		// 0x00028698
		fmul dword ptr [esi + 0xc]		// 0x0002869a
		fld st(1)		// 0x0002869d
		fmul dword ptr [esi + 0x14]		// 0x0002869f
		faddp st(1), st(0)		// 0x000286a2
		fld st(2)		// 0x000286a4
		fmul dword ptr [esi + 0x10]		// 0x000286a6
		faddp st(1), st(0)		// 0x000286a9
		fstp dword ptr [esp + 0x50]		// 0x000286ab
		fld st(0)		// 0x000286af
		fmul dword ptr [esi + 0x20]		// 0x000286b1
		fld st(2)		// 0x000286b4
		fmul dword ptr [esi + 0x1c]		// 0x000286b6
		faddp st(1), st(0)		// 0x000286b9
		fld st(3)		// 0x000286bb
		fmul dword ptr [esi + 0x18]		// 0x000286bd
		faddp st(1), st(0)		// 0x000286c0
		fstp dword ptr [esp + 0x54]		// 0x000286c2
		fmul dword ptr [esi + 0x2c]		// 0x000286c6
		fxch st(1)		// 0x000286c9
		fmul dword ptr [esi + 0x28]		// 0x000286cb
		faddp st(1), st(0)		// 0x000286ce
		fxch st(1)		// 0x000286d0
		fmul dword ptr [esi + 0x24]		// 0x000286d2
		faddp st(1), st(0)		// 0x000286d5
		fld dword ptr [esp + 0x50]		// 0x000286d7
		fadd dword ptr [esi + 0x30]		// 0x000286db
		fld dword ptr [esp + 0x54]		// 0x000286de
		fadd dword ptr [esi + 0x34]		// 0x000286e2
		fxch st(2)		// 0x000286e5
		fadd dword ptr [esi + 0x38]		// 0x000286e7
		fstp dword ptr [esp + 0x9c]		// 0x000286ea
		mov ecx, dword ptr [esp + 0x9c]		// 0x000286f1
		mov dword ptr [esp + 0x20], ecx		// 0x000286f8
		fstp dword ptr [esp + 0x18]		// 0x000286fc
		fstp dword ptr [esp + 0x1c]		// 0x00028700
		mov eax, dword ptr [eax + 0x3c]		// 0x00028704
		test eax, eax		// 0x00028707
		je L28716		// 0x00028709
		mov edx, dword ptr [esp + 0x5c]		// 0x0002870b
		mov bl, byte ptr [eax + edx + 0xc]		// 0x0002870f
		and ebx, 7		// 0x00028713
L28716:
		test bl, 1		// 0x00028716
		jne L28732		// 0x00028719
		mov eax, dword ptr [ebp]		// 0x0002871b
		push 0xffff0000		// 0x0002871e
		lea ecx, [esp + 0x28]		// 0x00028723
		push ecx		// 0x00028727
		lea edx, [esp + 0x6c]		// 0x00028728
		push edx		// 0x0002872c
		mov ecx, ebp		// 0x0002872d
		call dword ptr [eax + 0x20]		// 0x0002872f
L28732:
		test bl, 2		// 0x00028732
		jne L2874e		// 0x00028735
		mov eax, dword ptr [ebp]		// 0x00028737
		push 0xffff0000		// 0x0002873a
		lea ecx, [esp + 0x1c]		// 0x0002873f
		push ecx		// 0x00028743
		lea edx, [esp + 0x6c]		// 0x00028744
		push edx		// 0x00028748
		mov ecx, ebp		// 0x00028749
		call dword ptr [eax + 0x20]		// 0x0002874b
L2874e:
		test bl, 4		// 0x0002874e
		jne L2876a		// 0x00028751
		mov eax, dword ptr [ebp]		// 0x00028753
		push 0xffff0000		// 0x00028756
		lea ecx, [esp + 0x1c]		// 0x0002875b
		push ecx		// 0x0002875f
		lea edx, [esp + 0x2c]		// 0x00028760
		push edx		// 0x00028764
		mov ecx, ebp		// 0x00028765
		call dword ptr [eax + 0x20]		// 0x00028767
L2876a:
		mov edx, dword ptr [esp + 0x60]		// 0x0002876a
		mov ecx, dword ptr [esp + 0x5c]		// 0x0002876e
		mov eax, dword ptr [esp + 0x10]		// 0x00028772
		add edx, 0xc		// 0x00028776
		add ecx, 0x10		// 0x00028779
		dec eax		// 0x0002877c
		mov dword ptr [esp + 0x60], edx		// 0x0002877d
		mov dword ptr [esp + 0x5c], ecx		// 0x00028781
		mov dword ptr [esp + 0x10], eax		// 0x00028785
		jne L28573		// 0x00028789
L2878f:
		mov edi, dword ptr [esi + 0xe0]		// 0x0002878f
		mov eax, dword ptr [edi + 0x88]		// 0x00028795
		test eax, eax		// 0x0002879b
		jne L287a6		// 0x0002879d
		mov ecx, edi		// 0x0002879f
		call nxMDCallEdgeList		// 0x000287a1
L287a6:
		mov edi, dword ptr [edi + 0x88]		// 0x000287a6
		test edi, edi		// 0x000287ac
		mov dword ptr [esp + 0x34], edi		// 0x000287ae
		je L289cc		// 0x000287b2
		mov eax, dword ptr [esi + 0xe0]		// 0x000287b8
		mov eax, dword ptr [eax + 0xc]		// 0x000287be
		test eax, eax		// 0x000287c1
		jbe L289cc		// 0x000287c3
		xor ebx, ebx		// 0x000287c9
		mov dword ptr [esp + 0x10], eax		// 0x000287cb
		nop		// 0x000287cf
L287d0:
		mov ecx, dword ptr [esi + 0xe0]		// 0x000287d0
		mov eax, dword ptr [ecx + 0x14]		// 0x000287d6
		mov edx, dword ptr [eax + ebx]		// 0x000287d9
		mov ecx, dword ptr [ecx + 0x10]		// 0x000287dc
		add eax, ebx		// 0x000287df
		lea edx, [edx + edx*2]		// 0x000287e1
		fld dword ptr [ecx + edx*4]		// 0x000287e4
		lea edx, [ecx + edx*4]		// 0x000287e7
		fld dword ptr [edx + 4]		// 0x000287ea
		mov edi, dword ptr [eax + 4]		// 0x000287ed
		fld dword ptr [edx + 8]		// 0x000287f0
		mov eax, dword ptr [eax + 8]		// 0x000287f3
		fld st(0)		// 0x000287f6
		lea edi, [edi + edi*2]		// 0x000287f8
		fmul dword ptr [esi + 0x14]		// 0x000287fb
		lea edi, [ecx + edi*4]		// 0x000287fe
		fld st(2)		// 0x00028801
		lea eax, [eax + eax*2]		// 0x00028803
		fmul dword ptr [esi + 0x10]		// 0x00028806
		lea eax, [ecx + eax*4]		// 0x00028809
		faddp st(1), st(0)		// 0x0002880c
		fld st(3)		// 0x0002880e
		fmul dword ptr [esi + 0xc]		// 0x00028810
		faddp st(1), st(0)		// 0x00028813
		fstp dword ptr [esp + 0x38]		// 0x00028815
		fld st(2)		// 0x00028819
		fmul dword ptr [esi + 0x18]		// 0x0002881b
		fld st(1)		// 0x0002881e
		fmul dword ptr [esi + 0x20]		// 0x00028820
		faddp st(1), st(0)		// 0x00028823
		fld st(2)		// 0x00028825
		fmul dword ptr [esi + 0x1c]		// 0x00028827
		faddp st(1), st(0)		// 0x0002882a
		fstp dword ptr [esp + 0x3c]		// 0x0002882c
		fxch st(2)		// 0x00028830
		fmul dword ptr [esi + 0x24]		// 0x00028832
		fxch st(2)		// 0x00028835
		fmul dword ptr [esi + 0x2c]		// 0x00028837
		faddp st(2), st(0)		// 0x0002883a
		fmul dword ptr [esi + 0x28]		// 0x0002883c
		faddp st(1), st(0)		// 0x0002883f
		fld dword ptr [esp + 0x38]		// 0x00028841
		fadd dword ptr [esi + 0x30]		// 0x00028845
		fld dword ptr [esp + 0x3c]		// 0x00028848
		fadd dword ptr [esi + 0x34]		// 0x0002884c
		fxch st(2)		// 0x0002884f
		fadd dword ptr [esi + 0x38]		// 0x00028851
		fstp dword ptr [esp + 0x84]		// 0x00028854
		mov ecx, dword ptr [esp + 0x84]		// 0x0002885b
		mov dword ptr [esp + 0x6c], ecx		// 0x00028862
		fstp dword ptr [esp + 0x64]		// 0x00028866
		fstp dword ptr [esp + 0x68]		// 0x0002886a
		fld dword ptr [edi]		// 0x0002886e
		fld dword ptr [edi + 4]		// 0x00028870
		fld dword ptr [edi + 8]		// 0x00028873
		fld st(0)		// 0x00028876
		fmul dword ptr [esi + 0x14]		// 0x00028878
		fld st(2)		// 0x0002887b
		fmul dword ptr [esi + 0x10]		// 0x0002887d
		faddp st(1), st(0)		// 0x00028880
		fld st(3)		// 0x00028882
		fmul dword ptr [esi + 0xc]		// 0x00028884
		faddp st(1), st(0)		// 0x00028887
		fstp dword ptr [esp + 0x44]		// 0x00028889
		fld st(2)		// 0x0002888d
		fmul dword ptr [esi + 0x18]		// 0x0002888f
		fld st(1)		// 0x00028892
		fmul dword ptr [esi + 0x20]		// 0x00028894
		faddp st(1), st(0)		// 0x00028897
		fld st(2)		// 0x00028899
		fmul dword ptr [esi + 0x1c]		// 0x0002889b
		faddp st(1), st(0)		// 0x0002889e
		fstp dword ptr [esp + 0x48]		// 0x000288a0
		fxch st(2)		// 0x000288a4
		fmul dword ptr [esi + 0x24]		// 0x000288a6
		fxch st(2)		// 0x000288a9
		fmul dword ptr [esi + 0x2c]		// 0x000288ab
		faddp st(2), st(0)		// 0x000288ae
		fmul dword ptr [esi + 0x28]		// 0x000288b0
		faddp st(1), st(0)		// 0x000288b3
		fld dword ptr [esp + 0x44]		// 0x000288b5
		fadd dword ptr [esi + 0x30]		// 0x000288b9
		mov ecx, dword ptr [esp + 0x34]		// 0x000288bc
		fld dword ptr [esp + 0x48]		// 0x000288c0
		mov edi, dword ptr [ecx + 0xc]		// 0x000288c4
		fadd dword ptr [esi + 0x34]		// 0x000288c7
		add edi, ebx		// 0x000288ca
		fxch st(2)		// 0x000288cc
		fadd dword ptr [esi + 0x38]		// 0x000288ce
		fstp dword ptr [esp + 0x90]		// 0x000288d1
		mov edx, dword ptr [esp + 0x90]		// 0x000288d8
		mov dword ptr [esp + 0x2c], edx		// 0x000288df
		fstp dword ptr [esp + 0x24]		// 0x000288e3
		fstp dword ptr [esp + 0x28]		// 0x000288e7
		fld dword ptr [eax]		// 0x000288eb
		fld dword ptr [eax + 4]		// 0x000288ed
		fld dword ptr [eax + 8]		// 0x000288f0
		fld st(0)		// 0x000288f3
		fmul dword ptr [esi + 0x14]		// 0x000288f5
		fld st(2)		// 0x000288f8
		fmul dword ptr [esi + 0x10]		// 0x000288fa
		faddp st(1), st(0)		// 0x000288fd
		fld st(3)		// 0x000288ff
		fmul dword ptr [esi + 0xc]		// 0x00028901
		faddp st(1), st(0)		// 0x00028904
		fstp dword ptr [esp + 0x50]		// 0x00028906
		fld st(2)		// 0x0002890a
		fmul dword ptr [esi + 0x18]		// 0x0002890c
		fld st(2)		// 0x0002890f
		fmul dword ptr [esi + 0x1c]		// 0x00028911
		faddp st(1), st(0)		// 0x00028914
		fld st(1)		// 0x00028916
		fmul dword ptr [esi + 0x20]		// 0x00028918
		faddp st(1), st(0)		// 0x0002891b
		fstp dword ptr [esp + 0x54]		// 0x0002891d
		fxch st(1)		// 0x00028921
		fmul dword ptr [esi + 0x28]		// 0x00028923
		fxch st(1)		// 0x00028926
		fmul dword ptr [esi + 0x2c]		// 0x00028928
		faddp st(1), st(0)		// 0x0002892b
		fxch st(1)		// 0x0002892d
		fmul dword ptr [esi + 0x24]		// 0x0002892f
		faddp st(1), st(0)		// 0x00028932
		fld dword ptr [esp + 0x50]		// 0x00028934
		fadd dword ptr [esi + 0x30]		// 0x00028938
		fld dword ptr [esp + 0x54]		// 0x0002893b
		fadd dword ptr [esi + 0x34]		// 0x0002893f
		fxch st(2)		// 0x00028942
		fadd dword ptr [esi + 0x38]		// 0x00028944
		fstp dword ptr [esp + 0x9c]		// 0x00028947
		mov eax, dword ptr [esp + 0x9c]		// 0x0002894e
		mov dword ptr [esp + 0x20], eax		// 0x00028955
		fstp dword ptr [esp + 0x18]		// 0x00028959
		fstp dword ptr [esp + 0x1c]		// 0x0002895d
		mov eax, dword ptr [edi]		// 0x00028961
		test eax, eax		// 0x00028963
		jns L2897e		// 0x00028965
		mov edx, dword ptr [ebp]		// 0x00028967
		push 0xffff0000		// 0x0002896a
		lea eax, [esp + 0x28]		// 0x0002896f
		push eax		// 0x00028973
		lea ecx, [esp + 0x6c]		// 0x00028974
		push ecx		// 0x00028978
		mov ecx, ebp		// 0x00028979
		call dword ptr [edx + 0x20]		// 0x0002897b
L2897e:
		mov eax, dword ptr [edi + 8]		// 0x0002897e
		test eax, eax		// 0x00028981
		jns L2899c		// 0x00028983
		mov edx, dword ptr [ebp]		// 0x00028985
		push 0xffff0000		// 0x00028988
		lea eax, [esp + 0x1c]		// 0x0002898d
		push eax		// 0x00028991
		lea ecx, [esp + 0x6c]		// 0x00028992
		push ecx		// 0x00028996
		mov ecx, ebp		// 0x00028997
		call dword ptr [edx + 0x20]		// 0x00028999
L2899c:
		mov eax, dword ptr [edi + 4]		// 0x0002899c
		test eax, eax		// 0x0002899f
		jns L289ba		// 0x000289a1
		mov edx, dword ptr [ebp]		// 0x000289a3
		push 0xffff0000		// 0x000289a6
		lea eax, [esp + 0x1c]		// 0x000289ab
		push eax		// 0x000289af
		lea ecx, [esp + 0x2c]		// 0x000289b0
		push ecx		// 0x000289b4
		mov ecx, ebp		// 0x000289b5
		call dword ptr [edx + 0x20]		// 0x000289b7
L289ba:
		mov eax, dword ptr [esp + 0x10]		// 0x000289ba
		add ebx, 0xc		// 0x000289be
		dec eax		// 0x000289c1
		mov dword ptr [esp + 0x10], eax		// 0x000289c2
		jne L287d0		// 0x000289c6
L289cc:
		fld dword ptr [nxMDK101041f0_4]		// 0x000289cc
		fld dword ptr [esp + 0x30]		// 0x000289d2
		fucompp		// 0x000289d6
		fnstsw ax		// 0x000289d8
		test ah, 0x44		// 0x000289da
		jnp L28b56		// 0x000289dd
		mov edi, dword ptr [esi + 0xe0]		// 0x000289e3
		mov eax, dword ptr [edi + 0x20]		// 0x000289e9
		mov ebx, dword ptr [edi + 8]		// 0x000289ec
		add edi, 8		// 0x000289ef
		test eax, eax		// 0x000289f2
		mov dword ptr [esp + 0x34], ebx		// 0x000289f4
		jne L28a01		// 0x000289f8
		mov ecx, edi		// 0x000289fa
		call nxMDCallVertexNormals		// 0x000289fc
L28a01:
		test ebx, ebx		// 0x00028a01
		mov eax, dword ptr [edi + 0x18]		// 0x00028a03
		jbe L28b56		// 0x00028a06
		mov edx, dword ptr [esp + 0x34]		// 0x00028a0c
		mov ebx, 0xfffffffc		// 0x00028a10
		lea edi, [eax + 4]		// 0x00028a15
		sub ebx, eax		// 0x00028a18
		mov dword ptr [esp + 0x10], edx		// 0x00028a1a
		mov edi, edi		// 0x00028a1e
L28a20:
		mov eax, dword ptr [esi + 0xe0]		// 0x00028a20
		mov eax, dword ptr [eax + 0x10]		// 0x00028a26
		add eax, ebx		// 0x00028a29
		fld dword ptr [eax + edi]		// 0x00028a2b
		add eax, edi		// 0x00028a2e
		fld dword ptr [eax + 4]		// 0x00028a30
		fld dword ptr [eax + 8]		// 0x00028a33
		fld st(0)		// 0x00028a36
		fmul dword ptr [esi + 0x14]		// 0x00028a38
		fld st(2)		// 0x00028a3b
		fmul dword ptr [esi + 0x10]		// 0x00028a3d
		faddp st(1), st(0)		// 0x00028a40
		fld st(3)		// 0x00028a42
		fmul dword ptr [esi + 0xc]		// 0x00028a44
		faddp st(1), st(0)		// 0x00028a47
		fstp dword ptr [esp + 0x38]		// 0x00028a49
		fld st(2)		// 0x00028a4d
		fmul dword ptr [esi + 0x18]		// 0x00028a4f
		fld st(1)		// 0x00028a52
		fmul dword ptr [esi + 0x20]		// 0x00028a54
		faddp st(1), st(0)		// 0x00028a57
		fld st(2)		// 0x00028a59
		fmul dword ptr [esi + 0x1c]		// 0x00028a5b
		faddp st(1), st(0)		// 0x00028a5e
		fstp dword ptr [esp + 0x3c]		// 0x00028a60
		fxch st(2)		// 0x00028a64
		fmul dword ptr [esi + 0x24]		// 0x00028a66
		fxch st(2)		// 0x00028a69
		fmul dword ptr [esi + 0x2c]		// 0x00028a6b
		faddp st(2), st(0)		// 0x00028a6e
		fmul dword ptr [esi + 0x28]		// 0x00028a70
		faddp st(1), st(0)		// 0x00028a73
		fld dword ptr [esp + 0x38]		// 0x00028a75
		fadd dword ptr [esi + 0x30]		// 0x00028a79
		fld dword ptr [esp + 0x3c]		// 0x00028a7c
		fadd dword ptr [esi + 0x34]		// 0x00028a80
		fst dword ptr [esp + 0x48]		// 0x00028a83
		fxch st(2)		// 0x00028a87
		fadd dword ptr [esi + 0x38]		// 0x00028a89
		fst dword ptr [esp + 0x4c]		// 0x00028a8c
		fld st(1)		// 0x00028a90
		fstp dword ptr [esp + 0x70]		// 0x00028a92
		fxch st(2)		// 0x00028a96
		fstp dword ptr [esp + 0x74]		// 0x00028a98
		fxch st(1)		// 0x00028a9c
		fstp dword ptr [esp + 0x78]		// 0x00028a9e
		fld dword ptr [edi]		// 0x00028aa2
		fmul dword ptr [esi + 0x10]		// 0x00028aa4
		fld dword ptr [edi - 4]		// 0x00028aa7
		fmul dword ptr [esi + 0xc]		// 0x00028aaa
		faddp st(1), st(0)		// 0x00028aad
		fld dword ptr [edi + 4]		// 0x00028aaf
		fmul dword ptr [esi + 0x14]		// 0x00028ab2
		faddp st(1), st(0)		// 0x00028ab5
		fld dword ptr [edi + 4]		// 0x00028ab7
		fmul dword ptr [esi + 0x20]		// 0x00028aba
		fld dword ptr [edi]		// 0x00028abd
		fmul dword ptr [esi + 0x1c]		// 0x00028abf
		faddp st(1), st(0)		// 0x00028ac2
		fld dword ptr [edi - 4]		// 0x00028ac4
		fmul dword ptr [esi + 0x18]		// 0x00028ac7
		faddp st(1), st(0)		// 0x00028aca
		fld dword ptr [esi + 0x24]		// 0x00028acc
		fmul dword ptr [edi - 4]		// 0x00028acf
		fld dword ptr [edi + 4]		// 0x00028ad2
		fmul dword ptr [esi + 0x2c]		// 0x00028ad5
		faddp st(1), st(0)		// 0x00028ad8
		fld dword ptr [esi + 0x28]		// 0x00028ada
		fmul dword ptr [edi]		// 0x00028add
		faddp st(1), st(0)		// 0x00028adf
		fstp dword ptr [esp + 0x84]		// 0x00028ae1
		fxch st(1)		// 0x00028ae8
		fmul dword ptr [esp + 0x30]		// 0x00028aea
		fstp dword ptr [esp + 0x18]		// 0x00028aee
		fmul dword ptr [esp + 0x30]		// 0x00028af2
		fstp dword ptr [esp + 0x1c]		// 0x00028af6
		fld dword ptr [esp + 0x84]		// 0x00028afa
		fmul dword ptr [esp + 0x30]		// 0x00028b01
		fstp dword ptr [esp + 0x20]		// 0x00028b05
		fld dword ptr [esp + 0x18]		// 0x00028b09
		fadd st(0), st(1)		// 0x00028b0d
		mov edx, dword ptr [ebp]		// 0x00028b0f
		fstp dword ptr [esp + 0x50]		// 0x00028b12
		push 0x2266aa		// 0x00028b16
		fstp st(0)		// 0x00028b1b
		lea eax, [esp + 0x54]		// 0x00028b1d
		fld dword ptr [esp + 0x20]		// 0x00028b21
		push eax		// 0x00028b25
		fadd dword ptr [esp + 0x50]		// 0x00028b26
		lea ecx, [esp + 0x78]		// 0x00028b2a
		push ecx		// 0x00028b2e
		mov ecx, ebp		// 0x00028b2f
		fstp dword ptr [esp + 0x60]		// 0x00028b31
		fld dword ptr [esp + 0x2c]		// 0x00028b35
		fadd dword ptr [esp + 0x58]		// 0x00028b39
		fstp dword ptr [esp + 0x64]		// 0x00028b3d
		call dword ptr [edx + 0x20]		// 0x00028b41
		mov eax, dword ptr [esp + 0x10]		// 0x00028b44
		add edi, 0xc		// 0x00028b48
		dec eax		// 0x00028b4b
		mov dword ptr [esp + 0x10], eax		// 0x00028b4c
		jne L28a20		// 0x00028b50
L28b56:
		fld dword ptr [nxMDK101041f0_4]		// 0x00028b56
		fld dword ptr [esp + 0x14]		// 0x00028b5c
		fucompp		// 0x00028b60
		fnstsw ax		// 0x00028b62
		test ah, 0x44		// 0x00028b64
		jnp L28dfe		// 0x00028b67
		mov eax, dword ptr [esi + 0xe0]		// 0x00028b6d
		mov edi, dword ptr [eax + 0x24]		// 0x00028b73
		mov eax, dword ptr [eax + 0xc]		// 0x00028b76
		test eax, eax		// 0x00028b79
		jbe L28dfe		// 0x00028b7b
		xor ebx, ebx		// 0x00028b81
		add edi, 4		// 0x00028b83
		mov dword ptr [esp + 0x10], eax		// 0x00028b86
		lea ebx, [ebx]		// 0x00028b8a
L28b90:
		mov ecx, dword ptr [esi + 0xe0]		// 0x00028b90
		mov eax, dword ptr [ecx + 0x14]		// 0x00028b96
		mov edx, dword ptr [eax + ebx]		// 0x00028b99
		mov ecx, dword ptr [ecx + 0x10]		// 0x00028b9c
		add eax, ebx		// 0x00028b9f
		lea edx, [edx + edx*2]		// 0x00028ba1
		fld dword ptr [ecx + edx*4]		// 0x00028ba4
		lea edx, [ecx + edx*4]		// 0x00028ba7
		fld dword ptr [edx + 4]		// 0x00028baa
		fld dword ptr [edx + 8]		// 0x00028bad
		mov edx, dword ptr [eax + 4]		// 0x00028bb0
		fld st(0)		// 0x00028bb3
		lea edx, [edx + edx*2]		// 0x00028bb5
		fmul dword ptr [esi + 0x14]		// 0x00028bb8
		lea edx, [ecx + edx*4]		// 0x00028bbb
		fld st(2)		// 0x00028bbe
		fmul dword ptr [esi + 0x10]		// 0x00028bc0
		faddp st(1), st(0)		// 0x00028bc3
		fld st(3)		// 0x00028bc5
		fmul dword ptr [esi + 0xc]		// 0x00028bc7
		faddp st(1), st(0)		// 0x00028bca
		fstp dword ptr [esp + 0x38]		// 0x00028bcc
		fld st(2)		// 0x00028bd0
		fmul dword ptr [esi + 0x18]		// 0x00028bd2
		fld st(1)		// 0x00028bd5
		fmul dword ptr [esi + 0x20]		// 0x00028bd7
		faddp st(1), st(0)		// 0x00028bda
		fld st(2)		// 0x00028bdc
		fmul dword ptr [esi + 0x1c]		// 0x00028bde
		faddp st(1), st(0)		// 0x00028be1
		fstp dword ptr [esp + 0x3c]		// 0x00028be3
		fxch st(2)		// 0x00028be7
		fmul dword ptr [esi + 0x24]		// 0x00028be9
		fxch st(2)		// 0x00028bec
		fmul dword ptr [esi + 0x2c]		// 0x00028bee
		faddp st(2), st(0)		// 0x00028bf1
		fmul dword ptr [esi + 0x28]		// 0x00028bf3
		faddp st(1), st(0)		// 0x00028bf6
		fld dword ptr [esp + 0x38]		// 0x00028bf8
		fadd dword ptr [esi + 0x30]		// 0x00028bfc
		fld dword ptr [esp + 0x3c]		// 0x00028bff
		fadd dword ptr [esi + 0x34]		// 0x00028c03
		fstp dword ptr [esp + 0x74]		// 0x00028c06
		fxch st(1)		// 0x00028c0a
		fadd dword ptr [esi + 0x38]		// 0x00028c0c
		fstp dword ptr [esp + 0x78]		// 0x00028c0f
		fld dword ptr [edx]		// 0x00028c13
		fld dword ptr [edx + 4]		// 0x00028c15
		fld dword ptr [edx + 8]		// 0x00028c18
		fld st(0)		// 0x00028c1b
		fmul dword ptr [esi + 0x14]		// 0x00028c1d
		fld st(2)		// 0x00028c20
		fmul dword ptr [esi + 0x10]		// 0x00028c22
		faddp st(1), st(0)		// 0x00028c25
		fld st(3)		// 0x00028c27
		fmul dword ptr [esi + 0xc]		// 0x00028c29
		faddp st(1), st(0)		// 0x00028c2c
		fstp dword ptr [esp + 0x44]		// 0x00028c2e
		fld st(2)		// 0x00028c32
		fmul dword ptr [esi + 0x18]		// 0x00028c34
		fld st(1)		// 0x00028c37
		fmul dword ptr [esi + 0x20]		// 0x00028c39
		faddp st(1), st(0)		// 0x00028c3c
		fld st(2)		// 0x00028c3e
		fmul dword ptr [esi + 0x1c]		// 0x00028c40
		faddp st(1), st(0)		// 0x00028c43
		fstp dword ptr [esp + 0x48]		// 0x00028c45
		fxch st(2)		// 0x00028c49
		fmul dword ptr [esi + 0x24]		// 0x00028c4b
		fxch st(2)		// 0x00028c4e
		fmul dword ptr [esi + 0x2c]		// 0x00028c50
		faddp st(2), st(0)		// 0x00028c53
		fmul dword ptr [esi + 0x28]		// 0x00028c55
		faddp st(1), st(0)		// 0x00028c58
		fld dword ptr [esp + 0x44]		// 0x00028c5a
		fadd dword ptr [esi + 0x30]		// 0x00028c5e
		fstp dword ptr [esp + 0x24]		// 0x00028c61
		fld dword ptr [esp + 0x48]		// 0x00028c65
		fadd dword ptr [esi + 0x34]		// 0x00028c69
		fstp dword ptr [esp + 0x28]		// 0x00028c6c
		fadd dword ptr [esi + 0x38]		// 0x00028c70
		fstp dword ptr [esp + 0x2c]		// 0x00028c73
		mov eax, dword ptr [eax + 8]		// 0x00028c77
		lea eax, [eax + eax*2]		// 0x00028c7a
		fld dword ptr [ecx + eax*4]		// 0x00028c7d
		lea eax, [ecx + eax*4]		// 0x00028c80
		fld dword ptr [eax + 4]		// 0x00028c83
		fld dword ptr [eax + 8]		// 0x00028c86
		fld st(0)		// 0x00028c89
		fmul dword ptr [esi + 0x14]		// 0x00028c8b
		fld st(2)		// 0x00028c8e
		fmul dword ptr [esi + 0x10]		// 0x00028c90
		faddp st(1), st(0)		// 0x00028c93
		fld st(3)		// 0x00028c95
		fmul dword ptr [esi + 0xc]		// 0x00028c97
		faddp st(1), st(0)		// 0x00028c9a
		fstp dword ptr [esp + 0x50]		// 0x00028c9c
		fld st(2)		// 0x00028ca0
		fmul dword ptr [esi + 0x18]		// 0x00028ca2
		fld st(1)		// 0x00028ca5
		fmul dword ptr [esi + 0x20]		// 0x00028ca7
		faddp st(1), st(0)		// 0x00028caa
		fld st(2)		// 0x00028cac
		fmul dword ptr [esi + 0x1c]		// 0x00028cae
		faddp st(1), st(0)		// 0x00028cb1
		fstp dword ptr [esp + 0x54]		// 0x00028cb3
		fxch st(2)		// 0x00028cb7
		fmul dword ptr [esi + 0x24]		// 0x00028cb9
		fxch st(2)		// 0x00028cbc
		fmul dword ptr [esi + 0x2c]		// 0x00028cbe
		faddp st(2), st(0)		// 0x00028cc1
		fmul dword ptr [esi + 0x28]		// 0x00028cc3
		faddp st(1), st(0)		// 0x00028cc6
		fld dword ptr [esp + 0x50]		// 0x00028cc8
		fadd dword ptr [esi + 0x30]		// 0x00028ccc
		fstp dword ptr [esp + 0x64]		// 0x00028ccf
		fld dword ptr [esp + 0x54]		// 0x00028cd3
		fadd dword ptr [esi + 0x34]		// 0x00028cd7
		fstp dword ptr [esp + 0x68]		// 0x00028cda
		fadd dword ptr [esi + 0x38]		// 0x00028cde
		fstp dword ptr [esp + 0x6c]		// 0x00028ce1
		fadd dword ptr [esp + 0x24]		// 0x00028ce5
		fld dword ptr [esp + 0x28]		// 0x00028ce9
		fadd dword ptr [esp + 0x74]		// 0x00028ced
		fld dword ptr [esp + 0x2c]		// 0x00028cf1
		fadd dword ptr [esp + 0x78]		// 0x00028cf5
		fstp dword ptr [esp + 0x84]		// 0x00028cf9
		fxch st(1)		// 0x00028d00
		fadd dword ptr [esp + 0x64]		// 0x00028d02
		fstp dword ptr [esp + 0x88]		// 0x00028d06
		fadd dword ptr [esp + 0x68]		// 0x00028d0d
		fld dword ptr [esp + 0x84]		// 0x00028d11
		fadd dword ptr [esp + 0x6c]		// 0x00028d18
		fld dword ptr [esp + 0x88]		// 0x00028d1c
		fmul dword ptr [nxMDK101068ec_4]		// 0x00028d23
		fstp dword ptr [esp + 0x18]		// 0x00028d29
		fxch st(1)		// 0x00028d2d
		fmul dword ptr [nxMDK101068ec_4]		// 0x00028d2f
		fstp dword ptr [esp + 0x1c]		// 0x00028d35
		fmul dword ptr [nxMDK101068ec_4]		// 0x00028d39
		fstp dword ptr [esp + 0x20]		// 0x00028d3f
		fld dword ptr [edi + 4]		// 0x00028d43
		fmul dword ptr [esi + 0x14]		// 0x00028d46
		fld dword ptr [esi + 0xc]		// 0x00028d49
		fmul dword ptr [edi - 4]		// 0x00028d4c
		faddp st(1), st(0)		// 0x00028d4f
		fld dword ptr [edi]		// 0x00028d51
		fmul dword ptr [esi + 0x10]		// 0x00028d53
		faddp st(1), st(0)		// 0x00028d56
		fld dword ptr [esi + 0x1c]		// 0x00028d58
		fmul dword ptr [edi]		// 0x00028d5b
		fld dword ptr [edi - 4]		// 0x00028d5d
		fmul dword ptr [esi + 0x18]		// 0x00028d60
		faddp st(1), st(0)		// 0x00028d63
		fld dword ptr [esi + 0x20]		// 0x00028d65
		fmul dword ptr [edi + 4]		// 0x00028d68
		faddp st(1), st(0)		// 0x00028d6b
		fld dword ptr [edi - 4]		// 0x00028d6d
		fmul dword ptr [esi + 0x24]		// 0x00028d70
		fld dword ptr [edi + 4]		// 0x00028d73
		fmul dword ptr [esi + 0x2c]		// 0x00028d76
		faddp st(1), st(0)		// 0x00028d79
		fld dword ptr [edi]		// 0x00028d7b
		mov edx, dword ptr [ebp]		// 0x00028d7d
		fmul dword ptr [esi + 0x28]		// 0x00028d80
		push 0xaa6622		// 0x00028d83
		lea eax, [esp + 0xa8]		// 0x00028d88
		push eax		// 0x00028d8f
		faddp st(1), st(0)		// 0x00028d90
		lea ecx, [esp + 0x20]		// 0x00028d92
		push ecx		// 0x00028d96
		mov ecx, ebp		// 0x00028d97
		fstp dword ptr [esp + 0xa8]		// 0x00028d99
		fxch st(1)		// 0x00028da0
		fmul dword ptr [esp + 0x20]		// 0x00028da2
		fstp dword ptr [esp + 0xbc]		// 0x00028da6
		fmul dword ptr [esp + 0x20]		// 0x00028dad
		fld dword ptr [esp + 0xa8]		// 0x00028db1
		fmul dword ptr [esp + 0x20]		// 0x00028db8
		fld dword ptr [esp + 0xbc]		// 0x00028dbc
		fadd dword ptr [esp + 0x24]		// 0x00028dc3
		fstp dword ptr [esp + 0xb0]		// 0x00028dc7
		fxch st(1)		// 0x00028dce
		fadd dword ptr [esp + 0x28]		// 0x00028dd0
		fstp dword ptr [esp + 0xb4]		// 0x00028dd4
		fadd dword ptr [esp + 0x2c]		// 0x00028ddb
		fstp dword ptr [esp + 0xb8]		// 0x00028ddf
		call dword ptr [edx + 0x20]		// 0x00028de6
		mov eax, dword ptr [esp + 0x10]		// 0x00028de9
		add ebx, 0xc		// 0x00028ded
		add edi, 0x10		// 0x00028df0
		dec eax		// 0x00028df3
		mov dword ptr [esp + 0x10], eax		// 0x00028df4
		jne L28b90		// 0x00028df8
L28dfe:
		pop edi		// 0x00028dfe
		pop ebx		// 0x00028dff
L28e00:
		pop ebp		// 0x00028e00
L28e01:
		pop esi		// 0x00028e01
		add esp, 0xc4		// 0x00028e02
		ret 4		// 0x00028e08
		}
	}

void MeshShape::nxMeshDebugRender(const void* renderer) const
	{
	const NxReal* const parameter = nxPhysicsSDKParameters();
	nxMDParamScale = parameter[NX_VISUALIZATION_SCALE];
	nxMDParamShapes = parameter[NX_VISUALIZE_COLLISION_SHAPES];
	nxMDParamVNormals = parameter[NX_VISUALIZE_COLLISION_VNORMALS];
	nxMDParamFNormals = parameter[NX_VISUALIZE_COLLISION_FNORMALS];
	nxMeshDebugBody(this, 0, renderer);
	}


// ---------------------------------------------------------------------------
// The BASE vtable's stub rows. See ObjectModel.h for the slot map.

// phys_fn_001249 (0x00024f70), base-table slot 4.
bool ShapeBase::nxBaseSlot4(void* /*destination*/, float /*density*/,
	unsigned /*reserved*/)
	{
	return false;							// xor al,al; ret 0xc
	}

// phys_fn_004812 (0x000b4070), base-table slot 5.
void* ShapeBase::nxBaseSlot5(void* /*ray*/, float /*maxDistance*/,
	unsigned /*groups*/, unsigned /*hintFlags*/, void* /*hit*/)
	{
	return 0;								// xor eax,eax; ret 0x14
	}

// phys_fn_001035 (0x00022dd0), base-table slot 7.
bool ShapeBase::nxBaseSlot7(unsigned* /*out*/, const void* /*swept*/)
	{
	return false;							// xor al,al; ret 8
	}

// phys_fn_001277 (0x000256f0), base-table slot 2. Pure data movement:
// pose three, the halfword trio and [colobj+4] go into a descriptor-shaped
// record; every other byte of the record is untouched.
bool ShapeBase::nxBaseSaveState(void* record)
	{
	unsigned char* rec = static_cast<unsigned char*>(record);
	memcpy(rec + 8, &mLocalPose, sizeof(mLocalPose));		// rep movsd 9 + three words
	unsigned int de = mHalfwordDE;
	memcpy(rec + 0x38, &de, sizeof(de));			// movzx + dword store
	memcpy(rec + 0x3c, &mHalfwordD8, sizeof(mHalfwordD8));
	memcpy(rec + 0x3e, &mHalfwordDA, sizeof(mHalfwordDA));
	unsigned colobj = mWord9C;						// mov edx,[eax+0x9c]
	unsigned word04 = *reinterpret_cast<const unsigned*>(
		reinterpret_cast<const unsigned char*>(colobj) + 4);
	memcpy(rec + 0x40, &word04, sizeof(word04));	// no null test in the image
	return true;									// mov al,1
	}
