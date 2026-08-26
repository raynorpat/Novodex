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

// The scene guard pair runs real Foundation-side primitives -- the image
// reaches them through kernel32 import slots (0x10104010/14/2c/44).
#define NOMINMAX
#include <windows.h>

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

// Task 4 scaffolding: the scene shape-array insert. Write order follows the
// image -- registrar's shape store first (0x5c5a4), then the notify helper's
// free-list sentinel (0x5c093) and count mirror (0x5c0a8). Slot must be
// within every pre-sized vector's count: growth is not modelled.
void nxSceneInsertShape(void* container, void* shape, NxU32 slot)
	{
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

	void* c3 = *reinterpret_cast<void* const*>(sc + 0x6e4);
	nxSceneSlotFree(c3, mArgumentD4);
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
// _Myend@+0x0c}. The growth arm allocates 2*size + 2 dwords through the SDK
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
		void* fresh = nxGetSdkAllocator()->malloc(
			newCapDwords * sizeof(unsigned), NX_MEMORY_PERSISTENT);
		unsigned* run = static_cast<unsigned*>(fresh);
		for(unsigned i = 0; i < size; ++i)	// copy before release
			run[i] = begin[i];
		if(begin != 0)
			nxGetSdkAllocator()->free(begin);
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
		nxGetSdkAllocator()->free(self);
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
// phys_fn_000742 (0x16dd0): record helper -- a pure x87 chain whose cross
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

// phys_fn_000730-equivalent guard upgrade (0x5b730): try to take the writer
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
		nxReport(1, nxSourceFileSphereShapeCpp, 0x4a, 0,
			nxMsgSetRadiusPositive);
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

// phys_fn_001315 (0x000266a0), BASE-table slot 6. Detached-shape path:
// owner == null takes the early exit at 0x00026abb which is a pure no-op
// (pop ebp / add esp,0x84 / ret 4). The owned path needs scene
// infrastructure from Task 4 and is not reachable for detached shapes.
// phys_fn_000981 (0x00021990), BOX-table slot 12.
void BoxShape::nxBoxLoadFromDesc(const void* record)
	{
	const unsigned char* rec = static_cast<const unsigned char*>(record);
	memcpy(&mHull.mDims04[0], rec + 0x4c, sizeof(mHull.mDims04[0]));
	memcpy(&mHull.mDims04[1], rec + 0x50, sizeof(mHull.mDims04[1]));
	memcpy(&mHull.mDims04[2], rec + 0x54, sizeof(mHull.mDims04[2]));
	// helper 0x21420 recomputes hull data -- Task 3 deep chain
	mBase.nxApplyDescriptor(rec);
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
	memcpy(&mNormalE0, rec + 0x4c, sizeof(mNormalE0));
	float d = 0.0f;
	memcpy(&d, rec + 0x58, sizeof(d));
	mDistanceEC = -d;
	mBase.nxApplyDescriptor(rec);
	}

void ShapeBase::nxApplyOwnerUpdate(unsigned flags)
	{
	if(mOwner04 == nullptr)
		return;
	(void) flags;
	}

// ---------------------------------------------------------------------------
// Shape-to-name registry. See evidence section 3o for the full decode.

// The global list head at .data 0x10123c0c. Each entry is {shape*, name*}
// (8-byte stride). The list grows via the SDK allocator when capacity is
// exhausted.
static void* gShapeNameList = nullptr;

// phys_fn_000480 (0x000edc0): associate or dissociate a shape with a name.
static const unsigned NX_REG_CAP = 32;
static void* sRegShapes[NX_REG_CAP] = {};
static void* sRegNames[NX_REG_CAP] = {};
static unsigned sRegCount = 0;

bool ShapeBase::nxShapeNameRegistry(void* shape, void* name)
	{
	if(shape == nullptr)
		return false;
	for(unsigned i = 0; i < sRegCount; ++i)
		{
		if(sRegShapes[i] == shape)
			{
			if(name != nullptr)
				{
				sRegNames[i] = name;
				return true;
				}
			if(i < sRegCount - 1)
				{
				sRegShapes[i] = sRegShapes[sRegCount - 1];
				sRegNames[i] = sRegNames[sRegCount - 1];
				}
			--sRegCount;
			return true;
			}
		}
	if(name != nullptr && sRegCount < NX_REG_CAP)
		{
		sRegShapes[sRegCount] = shape;
		sRegNames[sRegCount] = name;
		++sRegCount;
		return true;
		}
	return name == nullptr;
	}

// phys_fn_001347 (0x00027740), BASE-table slot 1. See ObjectModel.h.
bool ShapeBase::nxApplyDescriptor(const void* record)
	{
	const unsigned char* rec = static_cast<const unsigned char*>(record);
	memcpy(&mPose6C, rec + 8, sizeof(mPose6C));			// rep movsd 9 + three words
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
		// destroyed through its own vtable: 0x00025433..37
		}
	mBase.nxBaseDtorOwnerArms();		// owner arms, 0x26be1..c35
	mBase.mPrunable.~Prunable();			// tail of the base-dtor chain
	(void) flags;							// self-free arm not modeled
	}

// phys_fn_001375 (0x00027c30), SPHERE-table slot 0.
void SphereShape::nxSphereScalarDeletingDtor(unsigned flags)
	{
	if(mBase.mWord9C)
		{
		// destroyed through its own vtable by the image: 0x00027c43..47
		}
	mBase.nxBaseDtorOwnerArms();			// owner arms, 0x26be1..c35
	mBase.mPrunable.~Prunable();			// tail of the base-dtor chain
	(void) flags;							// self-free arm not modeled
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

// phys_fn_000843 (0x0001c750), __thiscall ret 8. The image squares and cubes
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
		// the parallel-axis pair. Neither helper is transcribed yet; this
		// arm is documented, not reproduced, and every drive passes null.
		(void) extra;
		}
	}

// phys_fn_000837 (0x0001c5c0), __thiscall ret 4. Ten fld/fmul/fstp triples:
// the nine inertia words and the mass. The offset at +0x24..+0x2c is skipped
// entirely -- scaling by density keeps the center.
void MassFrame::nxMassFrameScale(float s)
	{
	for(unsigned i = 0; i < 9; ++i)
		mInertia[i] = mul32(mInertia[i], s);
	mMass = mul32(mMass, s);
	}

// phys_fn_000839 (0x0001c630), __thiscall ret 4. The weights live at +0x30 on
// both frames; the quotient divides the .rdata 1.0f literal by their sum (a
// real fdiv, kept single-precision-source but full x87 precision). The
// products are formed weight-times-offset per frame first, added pairwise --
// x-side as other+this, y/z sides as this+other, matching the stack order --
// scaled by the quotient and stored z, x, y. Then mass takes the raw sum and
// the nine inertia words accumulate componentwise.
void MassFrame::nxMassFrameMerge(const MassFrame& other)
	{
	double sum = static_cast<double>(mMass) + static_cast<double>(other.mMass);
	double q = static_cast<double>(gMassDensitySentinel) / sum;

	double ax = static_cast<double>(other.mMass) * static_cast<double>(other.mOffset.x);
	double ay = static_cast<double>(other.mMass) * static_cast<double>(other.mOffset.y);
	double az = static_cast<double>(other.mMass) * static_cast<double>(other.mOffset.z);
	double cx = static_cast<double>(mMass) * static_cast<double>(mOffset.x);
	double cy = static_cast<double>(mMass) * static_cast<double>(mOffset.y);
	double cz = static_cast<double>(mMass) * static_cast<double>(mOffset.z);

	NxF32 zx = static_cast<NxF32>((ax + cx) * q);
	NxF32 zz = static_cast<NxF32>((cz + az) * q);
	NxF32 zy = static_cast<NxF32>((cy + ay) * q);
	mOffset.x = zx;						// fstp [ecx+0x24]
	mOffset.y = zy;						// fstp [ecx+0x28]
	mOffset.z = zz;						// fstp [ecx+0x2c] (stored first in the image)
	mMass = static_cast<NxF32>(sum);	// fstp [ecx+0x30]

	for(unsigned i = 0; i < 9; ++i)
		mInertia[i] = static_cast<NxF32>(
			static_cast<double>(mInertia[i]) + static_cast<double>(other.mInertia[i]));
	}

// phys_fn_000851 (0x0001c930), __thiscall ret 0xc, SPHERE-table slot 4.
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

// phys_fn_000829 (0x0001bd00), __thiscall ret 4. Volume over half-extents:
// the accumulator replaces with the first non-zero extent then multiplies the
// rest, times 8 ([0x101068f0]) -- full extents are twice half-extents. That
// mass value stays live on the x87 stack for the whole function; a copy times
// 1/3 ([0x101068ec]) becomes the diagonal factor F. Squares are taken from
// the raw floats; each diagonal is F times its pairwise sum (xx: y^2+z^2,
// yy: z^2+x^2, zz: x^2+y^2), rounded once at the store.
void MassFrame::nxMassFrameBuildBox(const float* he)
	{
	double acc = static_cast<double>(gMassDensitySentinel);	// fld 1.0f
	if(nonZeroWord(he[0]))
		acc = static_cast<double>(he[0]);					// fstp st(0); fld [eax]
	if(nonZeroWord(he[1]))
		acc *= static_cast<double>(he[1]);
	if(nonZeroWord(he[2]))
		acc *= static_cast<double>(he[2]);

	double m = acc * static_cast<double>(gMassKEight);		// fmul [0x101068f0]
	double f = m * static_cast<double>(gMassKOneThird);		// fld 1/3; fmul st(1)

	double xx = static_cast<double>(he[0]) * static_cast<double>(he[0]);
	double yy = static_cast<double>(he[1]) * static_cast<double>(he[1]);
	double zz = static_cast<double>(he[2]) * static_cast<double>(he[2]);

	mInertia[1] = mInertia[2] = mInertia[3] = 0.0f;			// integer zero stores
	mInertia[5] = mInertia[6] = mInertia[7] = 0.0f;

	double iXX = (zz + yy) * f;								// fadd st(2) chain
	double iYY = (zz + xx) * f;
	double iZZ = (yy + xx) * f;

	mMass = static_cast<NxF32>(m);							// fstp [ecx+0x30]
	mInertia[0] = static_cast<NxF32>(iXX);					// fstp [ecx]
	mInertia[4] = static_cast<NxF32>(iYY);					// via [esp+0x10]
	mInertia[8] = static_cast<NxF32>(iZZ);					// via [esp+0x14]
	mOffset.x = 0.0f; mOffset.y = 0.0f; mOffset.z = 0.0f;
	}

// phys_fn_000845 (0x0001c7c0), __thiscall ret 0xc. A unit-density cylinder of
// radius `radius` and height 2*cylHalfHeight: mass = pi*r^2*2c. The axial
// diagonal carries mass*r^2/2 (through r^2*pi*c*... folded to mr^2/2 by the
// .rdata 0.5f at 0x101043cc); the transverse pair carries the full cylinder
// formula mass*(3r^2+4c^2)/12 ([0x101068f8] = 3, [0x101068f4] = 4, over the
// .rdata 1/12 at 0x101068e0). axisSelector routes the axial term:
// 0 -> +0x00, 1 -> +0x10 (and that path never writes +0x00 -- an image hole
// this transcription reproduces), anything else -> +0x20.
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
		mInertia[4] = sAx;
		mInertia[8] = sSide;
		// +0x00 stays as the caller left it: the image's selector==1 path
		// never stores it.
		}
	else
		{
		mInertia[0] = sSide;
		mInertia[4] = sSide;
		mInertia[8] = sAx;
		}
	mOffset.x = 0.0f; mOffset.y = 0.0f; mOffset.z = 0.0f;
	}

// phys_fn_000831 (0x0001bdc0), __thiscall ret 4. Straight-line x87 transform
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

// phys_fn_000847 (0x0001c880), __thiscall ret 4. Conditionally zeroes all
// thirteen words when the byte argument is non-zero; leaves the frame
// untouched when it is zero.
void MassFrame::nxMassFrameConditionalZero(unsigned flag)
	{
	if(flag == 0)
		return;
	mInertia[0] = 0.0f; mInertia[1] = 0.0f; mInertia[2] = 0.0f;
	mInertia[3] = 0.0f; mInertia[4] = 0.0f; mInertia[5] = 0.0f;
	mInertia[6] = 0.0f; mInertia[7] = 0.0f; mInertia[8] = 0.0f;
	mOffset.x = 0.0f; mOffset.y = 0.0f; mOffset.z = 0.0f;
	mMass = 0.0f;
	}

// phys_fn_000849 (0x0001c8c0), __thiscall ret 0xc, BOX-table slot 4.
void BoxShape::nxBoxComputeMassFrame(MassFrame* dest, float density,
	const float* halfExtents, const void* extra)
	{
	MassFrame local;
	local.nxMassFrameBuildBox(halfExtents);
	if(extra != nullptr)
		{
		// push extra; call 0x1bdc0 ; extra += 0x24 ; call 0x1c040 -- the
		// parallel-axis pair, not yet transcribed; drives pass null.
		(void) extra;
		}
	if(density != gMassDensitySentinel)
		local.nxMassFrameScale(density);
	dest->nxMassFrameMerge(local);		}

// phys_fn_000853 (0x0001c980), __thiscall ret 0x14, CAPSULE-table slot 4.
void CapsuleShape::nxCapsuleComputeMassFrame(MassFrame* dest, float density,
	unsigned axisSelector, float radius, float cylHalfHeight, const void* extra)
	{
	MassFrame local;
	local.nxMassFrameBuildCapsule(axisSelector, radius, cylHalfHeight);
	if(extra != nullptr)
		{
		(void) extra;						// parallel-axis pair, see above
		}
	if(density != gMassDensitySentinel)
		local.nxMassFrameScale(density);
	dest->nxMassFrameMerge(local);		}


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

// phys_fn_000991 (0x00021b40), CAPSULE-table slot 13.
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

// phys_fn_000995 (0x00021be0), CAPSULE-table slot 14.
void CapsuleShape::nxCapsuleSetRadius(float radius)
	{
	mFloatE0 = radius;						// mov [ecx+0xe0],eax at 0x00021be6
	// Tail-jumps through BASE slot 6 (owner-notify) with the flag forced to
	// 1: a null-owner no-op on a detached shape.
	}

// phys_fn_001014 (0x000225e0), CAPSULE-table slot 0.
void CapsuleShape::nxCapsuleScalarDeletingDtor(unsigned flags)
	{
	if(mBase.mWord9C)
		{
		// destroyed through its own vtable: 0x000225f3..f7
		}
	mBase.nxBaseDtorOwnerArms();		// owner arms, 0x26be1..c35
	mBase.mPrunable.~Prunable();			// tail of the base-dtor chain
	(void) flags;							// self-free arm not modeled
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

// phys_fn_000989 (0x00021ad0), CAPSULE-table slot 12.
void CapsuleShape::nxCapsuleLoadFromDesc(const void* record)
	{
	const unsigned char* rec = static_cast<const unsigned char*>(record);
	memcpy(&mFloatE0, rec + 0x4c, sizeof(mFloatE0));			// radius: 0x00021ad8
	float h = 0.0f;
	memcpy(&h, rec + 0x50, sizeof(h));							// desc height
	h *= 0.5f;													// fmul [0x101043cc] at 0x00021ae4
	mFloatE4 = h;												// fstp [+0xe4]
	memcpy(&mWordE8, rec + 0x54, sizeof(mWordE8));				// third word: 0x00021af0
	// The radius is reloaded and fcomp'd against the zero literal at
	// 0x00021aff: a report fires unless strictly positive (line 0x37).
	// Everything was already stored unconditionally above.
	if(!(mFloatE0 > 0.0f))
		nxReport(1, nxSourceFileCapsuleShapeCpp, 0x37, 0,
			nxMsgCapsuleLoadRadius);
	mBase.nxApplyDescriptor(rec);										// call BASE slot 1 at 0x00021b34
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
	mBase.nxBaseDtorOwnerArms();		// owner arms, 0x26be1..c35
	mBase.mPrunable.~Prunable();			// tail of 0x00026bd0: jmp 0xb5640
	(void) flags;							// flags&1 self-free arm: operator
	}										// delete territory, not modeled

// phys_fn_001399 (0x00028e80), MESH-table slot 0.
void MeshShape::nxMeshScalarDeletingDtor(unsigned flags)
	{
	if(mBase.mWord9C)
		{
		// destroyed through its own vtable: 0x00028e93..97
		}
	if(mWordE0)
		{
		--*reinterpret_cast<NxU32*>(mWordE0 + 0x74);	// dec [mesh+0x74]: 0x00028ea3
		}
	mBase.nxBaseDtorOwnerArms();		// owner arms, 0x26be1..c35
	mBase.mPrunable.~Prunable();			// tail of the base-dtor chain
	(void) flags;							// self-free arm not modeled
	}

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

// phys_fn_001257 (0x000251d0), PLANE-table slots 9 and 11.
void PlaneShape::nxPlaneExtentRow(float* out) const
	{
	out[0] = 0.0f;							// xor ecx,ecx; three stores
	out[1] = 0.0f;
	out[2] = 0.0f;
	unsigned big = 0x7f7fffffu;				// +FLT_MAX: the unbounded reach
	memcpy(out + 3, &big, sizeof(big));
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
