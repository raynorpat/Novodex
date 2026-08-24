/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "ObjectModel.h"

#include <math.h>
#include <string.h>

// phys_fn_002404 (0x0005ba70) is the shared member constructor; the oracle's
// collision-object ctor calls it at 0x000247d7 and then overwrites the vptr
// with the container's final table. The transcription constructs the member
// directly and lets C++ install this container's vptr for +0x00, which lands
// in the same post-construction state without replaying the intermediate
// base-vtable stores.
CollisionObject::CollisionObject(void* argument)
	{
	mWord04 = 0;							// 0x000247cb
	mArgument08 = argument;					// 0x000247e9
	mArgument18 = argument;					// 0x000247e6
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

static const NxU32 gEdgeTable[12] =
	{ 0, 1, 1, 2, 2, 3, 3, 0, 7, 6, 6, 5 };			// 0x10122180..
static const NxU32 gFaceCornerTable[12] =
	{ 131073, 0, 131073, 2, 131073, 4, 131073, 6, 131073, 8, 131073, 10 };	// 0x101221e0..
static const NxU32 gAdjacencyTable[12] =
	{ 0, 5, 0, 1, 0, 4, 0, 3, 2, 4, 1, 2 };			// 0x10122240..

const NxU32* BoxHullFacade::vertices() const
	{
	return mVertices;							// lea eax,[ecx+0x10]
	}

const BoxFaceRecord* BoxHullFacade::face(unsigned index) const
	{
	// lea eax,[eax+eax*8]; lea eax,[ecx+eax*4+0x70] -- index*36 + (this+0x70).
	return &mFaces[index];
	}

const NxU32* BoxHullFacade::edgeTable()
	{
	return gEdgeTable;
	}

const NxU32* BoxHullFacade::faceCornerTable()
	{
	return gFaceCornerTable;
	}

const NxU32* BoxHullFacade::adjacencyTable()
	{
	return gAdjacencyTable;
	}

// phys_fn_000975 (0x000217c0). The three per-corner values are column dots
// plus translation: A = col0.v + tx, B = col1.v + ty, C = col2.v + tz; the
// projection combines them as A*dx + (C*dz + B*dy) -- that exact association,
// because the x87 stack built C first and folded B before A. Bounds updates
// reproduce fcom/fnstsw exactly: the minimum replaces on strictly less, the
// maximum on strictly greater, and a NaN projection replaces neither (the
// unordered case sets C0, which both masks include). Intermediates are kept
// in double so a spilled value cannot truncate what the x87 stack held at
// 64-bit. The row reads neither of its two unread stack arguments.
void BoxHullFacade::supportBounds(const float* direction, float* outMin,
	float* outMax, const float* pose) const
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

// phys_fn_000985 (0x00021a10). The image's initializer array at .rdata
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

// phys_fn_000965 (0x000213e0): `xor eax,eax; ret` -- the same folded stub the
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
	mPose6C = identity;						// third instruction run, +0x6c
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
	// shape pushed. Driven with Task 4's actor classes; not replayed here.
	}

// ---------------------------------------------------------------------------
// BoxShape. See ObjectModel.h for the row map.

BoxShape::BoxShape(void* owner, unsigned argument)
	: mBase(owner, argument)				// forwarded unchanged: 0x0002187c..80
	{
	// vptr stores. The image stores the abstract wall at +0xe0 first
	// (0x00021885), the BOX primary table next (0x0002188f), then replaces
	// the wall with the facade's final twelve-slot table (0x00021895). The
	// wall store is a chained-construction intermediate; C++ installs both
	// final tables and it is never observable after the constructor returns.
	// The transcription writes the face-record pointer words once; the image
	// reaches record 5 through a walking pointer with identical effect.
	for(unsigned r = 0; r < 6; ++r)			// 0x000218a1..0x000218ee
		{
		mHull.mFaces[r].mCorners = 0;
		mHull.mFaces[r].mIndexListA = 0;
		mHull.mFaces[r].mIndexListB = 0;
		}

	// The embedded collision object: a fresh 0x1c-byte block through the SDK
	// allocator (0x000218f1..fe -- malloc slot, size 0x1c, flag 0), built by
	// phys_fn_001075 (0x00023580), whose body is phys_fn_001193 with the
	// box-family tables and which stores the box at BOTH +0x08 and +0x18.
	void* memory = nxGetSdkAllocator()->malloc(0x1c, NX_MEMORY_PERSISTENT);
	CollisionObject* object = memory
		? new(memory) CollisionObject(this)
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
	mRadiusE0 = 0.0f;						// mov [esi+0xe0],0 at 0x000277da

	// The embedded collision object: a fresh 0x1c-byte block through the SDK
	// allocator (0x00027de4..f2), built by phys_fn_001193 itself -- the
	// GENERIC collision-object constructor, not a per-type variant -- with
	// the sphere stored at BOTH +0x08 and +0x18.
	void* memory = nxGetSdkAllocator()->malloc(0x1c, NX_MEMORY_PERSISTENT);
	CollisionObject* object = memory
		? new(memory) CollisionObject(this)
		: 0;								// null arm: 0x00027803
	mBase.mWord9C = reinterpret_cast<NxU32>(object);	// 0x00027805

	mBase.mSentinelD0 = 1;					// OVERWRITES the base's sentinel: 0x0002780b
	}

// ---------------------------------------------------------------------------
// SPHERE-table rows. See ObjectModel.h.

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

// ---------------------------------------------------------------------------
// CapsuleShape. See ObjectModel.h for the row map.

CapsuleShape::CapsuleShape(void* owner, unsigned argument)
	: mBase(owner, argument)				// forwarded unchanged: 0x00021a67..6f
	{
	mFloatE0 = 0.0f;						// mov [esi+0xe0],0 at 0x00021a7a
	mFloatE4 = 0.0f;						// mov [esi+0xe4],0 at 0x00021a84

	// The embedded collision object: a fresh 0x1c-byte block through the SDK
	// allocator (0x00021a8e..9c), built by phys_fn_001123 -- the capsule-family
	// variant of the shared collision-object constructor -- with the capsule
	// stored at BOTH +0x08 and +0x18.
	void* memory = nxGetSdkAllocator()->malloc(0x1c, NX_MEMORY_PERSISTENT);
	CollisionObject* object = memory
		? new(memory) CollisionObject(this)
		: 0;								// null arm: 0x00021aad
	mBase.mWord9C = reinterpret_cast<NxU32>(object);	// 0x00021aaf

	mBase.mSentinelD0 = 3;					// NX_SHAPE_CAPSULE: 0x00021ab5
	}

// ---------------------------------------------------------------------------
// BOX-table slot 10. See ObjectModel.h.

void BoxShape::nxBoxCenterAndDiagonal(float* out) const
	{
	out[0] = mBase.mPose0C.mTranslation[0];	// mov edx,[ecx+0x30] at 0x00020670
	out[1] = mBase.mPose0C.mTranslation[1];	// +0x34
	out[2] = mBase.mPose0C.mTranslation[2];	// +0x38
	const double dx = mHull.mDims04[0];		// fld [+0xe4]
	const double dy = mHull.mDims04[1];		// [+0xe8]
	const double dz = mHull.mDims04[2];		// [+0xec]
	out[3] = static_cast<float>(sqrt(dx * dx + dy * dy + dz * dz));
	}

// phys_fn_000939 (0x000206c0), BOX-table slot 11.
void BoxShape::nxBoxZeroCenterAndDiagonal(float* out) const
	{
	out[0] = 0.0f;							// xor edx,edx; mov [eax],edx ...
	out[1] = 0.0f;							//   (three dword stores, 0x000206c4..cc)
	out[2] = 0.0f;
	const double dx = mHull.mDims04[0];
	const double dy = mHull.mDims04[1];
	const double dz = mHull.mDims04[2];
	out[3] = static_cast<float>(sqrt(dx * dx + dy * dy + dz * dz));	// 0x000206ce..f8
	}

// phys_fn_000927 (0x00020450), BOX-table slot 13.
bool BoxShape::nxBoxSaveState(void* record)
	{
	memcpy(reinterpret_cast<unsigned char*>(record) + 0x4c,
		&mHull.mDims04[0], sizeof(mHull.mDims04));	// 0x00020450..6e
	return mBase.nxBaseSaveState(record);			// jmp 0x000256f0 at 0x00020473
	}

// phys_fn_000941 (0x00020700), BOX-table slot 8.
void BoxShape::nxBoxLocalAABB(float* out) const
	{
	out[0] = -mHull.mDims04[0];				// fld/fchs/fxch chain, 0x00020700..23
	out[1] = -mHull.mDims04[1];
	out[2] = -mHull.mDims04[2];
	out[3] = mHull.mDims04[0];				// raw copies, 0x00020726..41
	out[4] = mHull.mDims04[1];
	out[5] = mHull.mDims04[2];
	}

// phys_fn_000935 (0x000205a0), BOX-table slot 9. Extent rows of pose one's
// rotation: axis 0 uses r0/r2/r1, axis 1 r3/r5/r4, axis 2 r6/r7/r8 -- the
// same |row . dims| pattern as every OBB bounds helper.
void BoxShape::nxBoxWorldAABB(float* out) const
	{
	const float* r = reinterpret_cast<const float*>(&mBase.mPose0C.mRotation[0]);
	const float dx = mHull.mDims04[0];
	const float dy = mHull.mDims04[1];
	const float dz = mHull.mDims04[2];
	const float e0 = fabsf(dx * r[0]) + fabsf(dy * r[2]) + fabsf(dz * r[1]);
	const float e1 = fabsf(dx * r[3]) + fabsf(dy * r[5]) + fabsf(dz * r[4]);
	const float e2 = fabsf(dx * r[6]) + fabsf(dy * r[8]) + fabsf(dz * r[7]);
	const float* t = mBase.mPose0C.mTranslation;
	out[0] = t[0] - e0;						// fsub [esp+4], 0x00020639
	out[1] = t[1] - e1;
	out[2] = t[2] - e2;
	out[3] = t[0] + e0;						// fadd, 0x0002064b..5b
	out[4] = t[1] + e1;
	out[5] = t[2] + e2;
	}

// phys_fn_000979 (0x00021940), BOX-table slot 0. The image's order: vtable
// restore words (masked on both sides), the colobj destruction, then the
// base-dtor chain whose tail destroys the embedded Prunable. The colobj's
// own deleting row is not transcribed yet -- its writes land in an external
// block, so nothing inside the shape buffer depends on it.
void BoxShape::nxBoxScalarDeletingDtor(unsigned flags)
	{
	if(mBase.mWord9C)
		{
		// mov ecx,[esi+0x9c]; test; push 1; call [eax] at 0x0002195b..61 --
		// destroyed through its own vtable by the image.
		}
	mBase.mPrunable.~Prunable();			// tail of 0x00026bd0: jmp 0xb5640
	(void) flags;							// flags&1 self-free arm: operator
	}										// delete territory, not modeled

// ---------------------------------------------------------------------------
// PlaneShape. See ObjectModel.h for the row map.

#include "NxUtilities.h"

PlaneShape::PlaneShape(void* owner, unsigned argument)
	: mBase(owner, argument)				// forwarded unchanged: 0x00024edb..df
	{
	// The embedded collision object: a fresh 0x1c-byte block through the SDK
	// allocator (0x00024eea..f8), built by phys_fn_001159 -- the plane-family
	// variant of the shared collision-object constructor -- with the plane
	// stored at BOTH +0x08 and +0x18.
	void* memory = nxGetSdkAllocator()->malloc(0x1c, NX_MEMORY_PERSISTENT);
	CollisionObject* object = memory
		? new(memory) CollisionObject(this)
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

// ---------------------------------------------------------------------------
// MeshShape. See ObjectModel.h for the row map.

MeshShape::MeshShape(void* owner, unsigned argument)
	: mBase(owner, argument)				// forwarded unchanged: 0x00027dbb..bf
	{
	mWordE0 = 0;							// mov [esi+0xe0],0 at 0x00027dca
	mWordE4 = 0;							// mov [esi+0xe4],0 at 0x00027dd4

	// The embedded collision object: a fresh 0x1c-byte block through the SDK
	// allocator (0x00027dde..ec), built by phys_fn_001241 -- the mesh-family
	// variant of the shared collision-object constructor -- with the mesh
	// shape stored at BOTH +0x08 and +0x18.
	void* memory = nxGetSdkAllocator()->malloc(0x1c, NX_MEMORY_PERSISTENT);
	CollisionObject* object = memory
		? new(memory) CollisionObject(this)
		: 0;								// null arm: 0x00027dfd
	mBase.mWord9C = reinterpret_cast<NxU32>(object);	// 0x00027dff

	mBase.mSentinelD0 = 4;					// NX_SHAPE_MESH: 0x00027e05
	}

// ---------------------------------------------------------------------------
// The BASE vtable's stub rows. See ObjectModel.h for the slot map.

// phys_fn_001249 (0x00024f70), base-table slot 4.
bool ShapeBase::nxBaseSlot4(void* /*argument1*/, void* /*argument2*/)
	{
	return false;							// xor al,al; ret 0xc
	}

// phys_fn_004812 (0x000b4070), base-table slot 5.
void* ShapeBase::nxBaseSlot5(void* /*argument1*/, void* /*argument2*/,
	void* /*argument3*/, void* /*argument4*/)
	{
	return 0;								// xor eax,eax; ret 0x14
	}

// phys_fn_001035 (0x00022dd0), base-table slot 7.
bool ShapeBase::nxBaseSlot7(void* /*argument1*/)
	{
	return false;							// xor al,al; ret 8
	}

// phys_fn_001277 (0x000256f0), base-table slot 2. Pure data movement:
// pose three, the halfword trio and [colobj+4] go into a descriptor-shaped
// record; every other byte of the record is untouched.
bool ShapeBase::nxBaseSaveState(void* record)
	{
	unsigned char* rec = static_cast<unsigned char*>(record);
	memcpy(rec + 8, &mPose6C, sizeof(mPose6C));		// rep movsd 9 + three words
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
