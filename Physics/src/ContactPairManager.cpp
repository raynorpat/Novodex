/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The contact-pair manager rows of the SceneRaycast.cpp..CapsuleShape.cpp gap
// (scene-raycast block Task 4): 000750..000921 less 000867 (ObjectModel.cpp),
// 000869/000907 (visualisation) and 000873 (ContactGeneration.cpp). See
// ContactPairManager.h for the objects.
//
// Every row here runs from the simulation step (NxScene::simulate's worker
// 002400 -> 000659 -> 000655 and its callees), under the step's control word
// 0x0f7f, except the release paths of 000881/000887/000889/000903/000913/
// 000915; so this translation unit is on the /arch:IA32 list (CMakeLists.txt)
// and follows the x87 conventions: register lifetimes are `double`, spills are
// NxReal, and each expression keeps the listing's grouping and operand order.
// Raw copies the listing makes with integer moves are made with integer moves
// here (cpmCopyWords), so a float is never loaded onto the FPU to be copied.
//
// The allocator is the oracle's for every row: `[0x101041bc]`, i.e. the
// imported nxFoundationSDKAllocator (slot 8 malloc(size, type 0), slot 0x14
// free). No row here calls 004803.
//
// Open callees (REPRODUCTION HOLES, bottom of the "callees" block below): the
// narrow-phase pair dispatcher 002348, the stream sub-object's reset 002354
// and constructor 002356, and the pair-keyed hash 004153/004155/004157 with
// its SDK instance at .data 0x10123c28. None of them exists in the candidate.

#include "ContactPairManager.h"

#include "Containers.h"
#include "NxMaterial.h"
#include "NxUserContactReport.h"
#include "NxActor.h"
#include "NxUtilities.h"
#include "ObjectModel.h"
#include "ContactGeneration.h"
#include "NarrowPhase.h"
#include "PhysicsSDK.h"
#include "Scene.h"
#include "X87Sqrt.h"
#include "core/JointSupport.h"

#include <float.h>
#include <math.h>
#include <new>
#include <stddef.h>
#include <string.h>

static_assert(sizeof(NxFrictionPatch) == 0x78, "000895 allocates 0x78-byte patches");
static_assert(sizeof(NxActorPair) == 0xec, "the pair runs from node +0x14 to node +0x100");
static_assert(sizeof(NxPairNode) == 0x108, "000911 allocates 0x108-byte nodes");
static_assert(sizeof(CpmFrictionParams) == 0x24, "000879's parameter block");
static_assert(offsetof(CpmFrictionParams, enabled) == 0x20, "000861 writes the enable byte at +0x20");
static_assert(sizeof(CpmBufferedContact) == 0x2c, "000917 appends 0x2c-byte records");
static_assert(sizeof(CpmPairHashEntry) == 8, "000913 steps the hash entries by 8");
static_assert(offsetof(CpmPairHash, entries) == 0x14, "000913 reads the entries at +0x14");
static_assert(offsetof(NxMaterial, restitution) == 0x10, "material +0x10");
static_assert(offsetof(NxMaterial, dirOfAnisotropy) == 0x1c, "material +0x1c");
static_assert(offsetof(NxMaterial, flags) == 0x38, "material +0x38");
static_assert(offsetof(NxMaterial, frictionCombineMode) == 0x3c, "material +0x3c");
static_assert(offsetof(NxMaterial, restitutionCombineMode) == 0x40, "material +0x40");
static_assert(offsetof(NxMaterial, programData) == 0x44, "material +0x44");

// ---------------------------------------------------------------------------
// Shared helpers (not rows)
// ---------------------------------------------------------------------------

template <class T> static NX_INLINE T& cpmAt(void* p, NxU32 offset)
	{
	return *reinterpret_cast<T*>(static_cast<NxU8*>(p) + offset);
	}

template <class T> static NX_INLINE const T& cpmAt(const void* p, NxU32 offset)
	{
	return *reinterpret_cast<const T*>(static_cast<const NxU8*>(p) + offset);
	}

// `mov r,[src+k]; mov [dst+k],r` runs: a copy that never touches the FPU.
static NX_INLINE void cpmCopyWords(void* dst, const void* src, NxU32 words)
	{
	NxU32* d = static_cast<NxU32*>(dst);
	const NxU32* s = static_cast<const NxU32*>(src);
	for(NxU32 i = 0; i < words; i++)
		d[i] = s[i];
	}

static NX_INLINE NxU32 cpmBits(NxReal value)
	{
	NxU32 word;
	memcpy(&word, &value, 4);
	return word;
	}

// A stored 32-bit word used as a pointer (the stream's objects, the report
// records, the hash values).
static NX_INLINE void* cpmPointer(NxU32 word)
	{
	return reinterpret_cast<void*>((size_t)word);
	}

static NX_INLINE NxReal cpmFloat(NxU32 word)
	{
	NxReal value;
	memcpy(&value, &word, 4);
	return value;
	}

// The body record fields these rows read: the row-major world rotation at
// +0x134..+0x154, the world position at +0x158, the solver body at +0x204.
static NX_INLINE const NxReal* cpmBodyRotation(const NxU8* body)
	{
	return reinterpret_cast<const NxReal*>(body + 0x134);
	}

static NX_INLINE const NxReal* cpmBodyPosition(const NxU8* body)
	{
	return reinterpret_cast<const NxReal*>(body + 0x158);
	}

// `mov eax,[body]; test; je; mov eax,[eax+0x204]` (e.g. 0x1001dd49..0x1001dd7e).
static NX_INLINE JointSupportBody* cpmBodySupport(const NxU8* body)
	{
	return body ? *reinterpret_cast<JointSupportBody* const*>(body + 0x204) : 0;
	}

// The inlined PhysicsSDK::getMaterial every material row repeats (e.g.
// 0x1001ca50..0x1001ca8f): the count is (last - first) / 0x48 (signed, by the
// 0x38e38e39 reciprocal), the index the low 16 bits, compared unsigned; an
// index out of range reads material 0.
static NX_INLINE const NxMaterial* cpmMaterial(NxU32 index)
	{
	const NxU8* sdk = reinterpret_cast<const NxU8*>(PhysicsSDK::instance);
	const NxMaterial* first = *reinterpret_cast<const NxMaterial* const*>(sdk + 0x28);
	const NxMaterial* last = *reinterpret_cast<const NxMaterial* const*>(sdk + 0x2c);
	const NxI32 count = (NxI32)(last - first);
	const NxU32 i = index & 0xffff;
	return i < (NxU32)count ? first + i : first;
	}

// The combine-mode pick: `cmp modeA, modeB; jl` keeps the larger (signed).
static NX_INLINE NxI32 cpmMaxMode(NxI32 modeA, NxI32 modeB)
	{
	return modeA < modeB ? modeB : modeA;
	}

// NxCombineMode on the FPU (000855's body, inlined in 000857 and 000859):
// 0 average, 1 min, 2 multiply, anything else max. The compares are
// `fcom; test ah,5; jp`: a NaN keeps b for min and a for max.
static NX_INLINE double cpmCombine(double a, double b, NxI32 mode)
	{
	switch(mode)
		{
		case 0:		return (a + b) * 0.5;
		case 1:		return a < b ? a : b;
		case 2:		return a * b;
		default:	return a < b ? b : a;
		}
	}

// The body-local transform 000865 inlines twice (0x1001d0ee..0x1001d177): the
// transpose of the body rotation applied to (p - position). dy is stored,
// dz is stored with `fst` and its unrounded value feeds y only.
static NX_INLINE void cpmToBodyLocal0865(const NxU8* body, const NxReal* p, NxReal* out)
	{
	const NxReal* m = cpmBodyRotation(body);
	const NxReal* t = cpmBodyPosition(body);
	const double dx = (double)p[0] - t[0];
	const NxReal dy = (NxReal)((double)p[1] - t[1]);
	const double dz = (double)p[2] - t[2];
	const NxReal dzf = (NxReal)dz;
	const double y = (dz * m[7] + dx * m[1]) + (double)dy * m[4];
	const double z = ((double)dzf * m[8] + dx * m[2]) + (double)dy * m[5];
	const double x = ((double)dzf * m[6] + (double)dy * m[3]) + dx * m[0];
	out[0] = (NxReal)x;
	out[1] = (NxReal)y;
	out[2] = (NxReal)z;
	}

// 000891's copy of the same transform (0x1001ed4b..0x1001edd5): here dz is
// stored with `fstp`, so every product reads the stored value, and the
// products are formed in a different order.
static NX_INLINE void cpmToBodyLocal0891(const NxU8* body, const NxReal* p, NxReal* out)
	{
	const NxReal* m = cpmBodyRotation(body);
	const NxReal* t = cpmBodyPosition(body);
	const double dx = (double)p[0] - t[0];
	const NxReal dy = (NxReal)((double)p[1] - t[1]);
	const NxReal dz = (NxReal)((double)p[2] - t[2]);
	const double y = (dx * m[1] + (double)dz * m[7]) + (double)dy * m[4];
	const double z = (dx * m[2] + (double)dz * m[8]) + (double)dy * m[5];
	const double x = ((double)dz * m[6] + (double)dy * m[3]) + dx * m[0];
	out[0] = (NxReal)x;
	out[1] = (NxReal)y;
	out[2] = (NxReal)z;
	}

// A world direction into a body's frame, 000895's form (0x1001f026..0x1001f08b).
static NX_INLINE void cpmDirectionToBody0895(const NxU8* body, const NxReal* n, NxReal* out)
	{
	const NxReal* m = cpmBodyRotation(body);
	out[0] = (NxReal)(((double)m[3] * n[1] + (double)m[6] * n[2]) + (double)m[0] * n[0]);
	out[1] = (NxReal)(((double)m[1] * n[0] + (double)m[4] * n[1]) + (double)m[7] * n[2]);
	out[2] = (NxReal)(((double)m[2] * n[0] + (double)m[5] * n[1]) + (double)m[8] * n[2]);
	}

// The record-kind update every constraint record site here makes after the
// row vector (000879 0x1001e246..0x1001e2c3, 000883 0x1001e86e..0x1001e8d2,
// 000897 0x1001f827..0x1001f894): kind into bits 0-4, bit 9 = (kind is 0 or
// 2), bit 10 = (kind is 2, 3 or 5), bits 5-8 and 11-18 cleared. The kind is
// a constant at every site but the listing tests it, so the tests are kept.
static NX_INLINE void cpmRecordKind(JointSupportRecord* record, NxU32 keep, NxU32 kind)
	{
	const NxU32 f1 = (record->mFlags & keep) | kind;
	record->mFlags = f1;
	const NxU32 k1 = f1 & 0x1f;
	const NxU32 bit9 = (k1 == 0 || k1 == 2) ? 1u : 0u;
	const NxU32 f2 = (((bit9 << 9) ^ f1) & 0x200) ^ f1;
	record->mFlags = f2;
	const NxU32 k2 = f2 & 0x1f;
	const NxU32 bit10 = (k2 == 3 || k2 == 2 || k2 == 5) ? 1u : 0u;
	record->mFlags = ((bit10 & 1) << 10) | (f2 & 0xfff8021f);
	}

// The tail after each record's fields (000879 0x1001e2c9..0x1001e2fc, 000897
// 0x1001f8af..0x1001f8e5): 004391 into +0x40 (the first output is a dead
// frame slot), +0x3c = +0x40, then +0x40 scaled by NX_PENALTY_FORCE (kind 0
// or 2) or 0.7f (0x10106940; kind 1 or 3).
static NX_INLINE void cpmSolveRecord(JointSupportRecord* record, const NxReal* parameter)
	{
	NxReal unused;
	record->row004391(unused, record->mUnknown040);
	record->mUnknown03c = record->mUnknown040;
	const NxU32 kind = record->mFlags & 0x1f;
	if(kind == 0 || kind == 2)
		record->mUnknown040 = (NxReal)((double)parameter[NX_PENALTY_FORCE] * record->mUnknown040);
	else if(kind == 1 || kind == 3)
		record->mUnknown040 = (NxReal)((double)record->mUnknown040 * 0.7f);
	}

// Semantic model of phys_fn_004403. The velocity error accumulates separately
// in +0x4c. Its body impulse is reduced by the scaled penetration bias before
// being accumulated in +0x44; the listing stores that change as a float and
// clamps the accumulated impulse at zero.
static void cpmSolveContactRecord0403(NxReal, NxI32, JointSupportRecord* record)
	{
	NxReal applied;
	static const NxReal zero = 0.0f;
	__asm
		{
		mov edx, record
		mov ecx, dword ptr [edx + 10h]
		test ecx, ecx
		jz cpm_solve_no_body0
		fld dword ptr [ecx + 18h]
		fmul dword ptr [edx + 20h]
		fld dword ptr [ecx + 14h]
		fmul dword ptr [edx + 1ch]
		faddp st(1), st(0)
		fld dword ptr [ecx + 8]
		fmul dword ptr [edx + 8]
		faddp st(1), st(0)
		fld dword ptr [ecx + 4]
		fmul dword ptr [edx + 4]
		faddp st(1), st(0)
		fld dword ptr [ecx]
		fmul dword ptr [edx]
		faddp st(1), st(0)
		fld dword ptr [ecx + 10h]
		fmul dword ptr [edx + 18h]
		faddp st(1), st(0)
		jmp cpm_solve_body1
	cpm_solve_no_body0:
		fldz
	cpm_solve_body1:
		mov eax, dword ptr [edx + 14h]
		test eax, eax
		jz cpm_solve_relative_ready
		fld dword ptr [eax + 18h]
		fmul dword ptr [edx + 2ch]
		fld dword ptr [eax + 14h]
		fmul dword ptr [edx + 28h]
		faddp st(1), st(0)
		fld dword ptr [eax + 8]
		fmul dword ptr [edx + 8]
		faddp st(1), st(0)
		fld dword ptr [eax + 4]
		fmul dword ptr [edx + 4]
		faddp st(1), st(0)
		fld dword ptr [eax + 10h]
		fmul dword ptr [edx + 24h]
		faddp st(1), st(0)
		fld dword ptr [eax]
		fmul dword ptr [edx]
		faddp st(1), st(0)
		fsubp st(1), st(0)
	cpm_solve_relative_ready:
		fsubr dword ptr [edx + 38h]
		fmul dword ptr [edx + 3ch]
		fld st(0)
		fadd dword ptr [edx + 4ch]
		fstp dword ptr [edx + 4ch]
		fld dword ptr [edx + 40h]
		fmul dword ptr [edx + 34h]
		fsubr st(0), st(1)
		fstp dword ptr [applied]
		fstp st(0)
		fld dword ptr [applied]
		fadd dword ptr [edx + 44h]
		fcom dword ptr [zero]
		fnstsw ax
		test ah, 5
		jp cpm_solve_store_impulse
		fstp st(0)
		fld dword ptr [edx + 44h]
		mov dword ptr [edx + 44h], 0
		fchs
		fstp dword ptr [applied]
		jmp cpm_solve_impulse_ready
	cpm_solve_store_impulse:
		fstp dword ptr [edx + 44h]
	cpm_solve_impulse_ready:
		fldz
		fld dword ptr [applied]
		fucompp
		fnstsw ax
		test ah, 44h
		jnp cpm_solve_no_apply
	cpm_solve_no_apply:
		}
	if(applied == 0.0f)
		return;

	const NxVec3& n = record->mUnknown000;
	for(unsigned side = 0; side < 2; ++side)
		{
		JointSupportBody* body = record->mBody[side];
		if(!body || body->mUnknown00c <= 0.0f)
			continue;
		const NxReal sign = side == 0 ? applied : -applied;
		NxReal scaledImpulseY, scaledImpulseZ;
		__asm
			{
			mov edx, record
			mov ecx, body
			fld sign
			fmul dword ptr [edx + 4]
			fstp dword ptr [scaledImpulseY]
			fld sign
			fmul dword ptr [edx + 8]
			fstp dword ptr [scaledImpulseZ]
			fld sign
			fmul dword ptr [edx]
			fld dword ptr [ecx + 0ch]
			fld st(0)
			fmul st(0), st(2)
			fld dword ptr [scaledImpulseY]
			fmul st(0), st(2)
			fstp dword ptr [scaledImpulseY]
			fld dword ptr [scaledImpulseZ]
			fmul st(0), st(2)
			fstp dword ptr [scaledImpulseZ]
			fadd dword ptr [ecx]
			fstp dword ptr [ecx]
			fld dword ptr [scaledImpulseY]
			fadd dword ptr [ecx + 4]
			fstp dword ptr [ecx + 4]
			fld dword ptr [scaledImpulseZ]
			fadd dword ptr [ecx + 8]
			fstp dword ptr [ecx + 8]
			fstp st(0)
			fstp st(0)
			}
		const NxVec3& arm = side == 0 ? record->mUnknown018 : record->mUnknown024;
		const NxReal angularInput[3] = {
			(NxReal)((double)arm.x * sign),
			(NxReal)((double)arm.y * sign),
			(NxReal)((double)arm.z * sign) };
		NxReal angularDelta[3];
		for(unsigned row = 0; row < 3; ++row)
			angularDelta[row] = (NxReal)(((double)body->mUnknown020[row * 3] * angularInput[0]
				+ (double)body->mUnknown020[row * 3 + 1] * angularInput[1])
				+ (double)body->mUnknown020[row * 3 + 2] * angularInput[2]);
		body->mUnknown010.x = (NxReal)((double)body->mUnknown010.x + angularDelta[0]);
		body->mUnknown010.y = (NxReal)((double)body->mUnknown010.y + angularDelta[1]);
		body->mUnknown010.z = (NxReal)((double)body->mUnknown010.z + angularDelta[2]);
		}
	}

// phys_fn_004176's record pass for the live contact kind. The schedule and
// body-view writeback remain at the exact 000611 island boundary.
void __cdecl cpmSolveSceneContactRecords(NxSceneInternal* scene, NxU32 maxIterations)
	{
	const NxReal dt = scene->at<NxReal>(0x548);
	const NxU32 count = scene->at<NxU32>(0x5bc);
	JointSupportRecord* records = scene->at<JointSupportRecord*>(0x5b8);
	for(NxI32 pass = (NxI32)maxIterations; pass > 0; --pass)
		for(NxU32 i = 0; i < count; ++i)
			{
			JointSupportRecord* record = records + i;
			if((record->mFlags & 0x1f) != 0)
				continue;
			const JointSupportBody* a = record->mBody[0];
			const JointSupportBody* b = record->mBody[1];
			if((a && a->mUnknown05c >= (NxU32)pass)
				|| (b && b->mUnknown05c >= (NxU32)pass))
				cpmSolveContactRecord0403(dt, pass, record);
			}
	for(NxU32 i = 0; i < scene->at<NxU32>(0x5b0); ++i)
		{
		JointSupportBody* body = scene->at<JointSupportBody*>(0x5ac) + i;
		body->mUnknown044 = body->mUnknown000;
		body->mUnknown050 = body->mUnknown010;
		}
	for(NxU32 i = 0; i < count; ++i)
		{
		JointSupportRecord* record = records + i;
		if((record->mFlags & 0x1f) != 0)
			continue;
		if(record->mUnknown034 <= 0.0f)
			record->mUnknown034 = 0.0f;
		cpmSolveContactRecord0403(dt, -1, record);
		}
	}

// The Scene's 0x50-byte record array (+0x5b8 records, +0x5bc count, +0x5c0
// capacity), grown by 000598 when full: the inline sequence at 0x1001e175,
// 0x1001e314, 0x1001e735 and 0x1001f76d.
static NX_INLINE JointSupportRecord* cpmTakeRecord(NxSceneInternal* scene)
	{
	if(scene->at<NxU32>(0x5bc) == scene->at<NxU32>(0x5c0))
		scene->growJointRecords();
	const NxU32 index = scene->at<NxU32>(0x5bc);
	scene->at<NxU32>(0x5bc) = index + 1;
	return scene->at<JointSupportRecord*>(0x5b8) + index;
	}

// ---------------------------------------------------------------------------
// Open callees: REPRODUCTION HOLES. Each is named for the row it stands in
// for and does nothing (or finds nothing); none carries a stable-ID line.
// ---------------------------------------------------------------------------

// Row 002356 (0x0005b680, 22 B): constructs the stream sub-object at pair
// +0x10 (its SdkContainer at +0x28 through 004836, then 002354). thiscall.
static __declspec(noinline) void cpmOpen002356(void* streamObject)
	{
	new (static_cast<NxU8*>(streamObject) + 0x28) SdkContainer();
	NxContactSinkResetState(reinterpret_cast<NxU32*>(streamObject));
	}

// Row 002354 (0x0005b620, 86 B): resets the stream sub-object (zeroes
// +0x00..+0x33, reserves the pair-count word). thiscall.
static __declspec(noinline) void cpmOpen002354(void* streamObject)
	{
	NxContactSinkResetState(reinterpret_cast<NxU32*>(streamObject));
	}

// Row 002348 (0x0005ab80, 719 B): the narrow-phase pair dispatcher,
// thiscall on the shape-pair function table at .data 0x10123c18
// (PhysicsSDK.cpp gShapePairFunctionTable), (shape0, shape1, pair, scene).
static __declspec(noinline) void cpmOpen002348(NxU8* shape0, NxU8* shape1, NxActorPair* pair, NxSceneInternal* scene)
	{
	(void)scene;
	NxDispatchShapePair(NxGetCollisionDispatchMatrix(),
		reinterpret_cast<const NxCollisionShape*>(shape0),
		reinterpret_cast<const NxCollisionShape*>(shape1),
		pair, 0);
	}

static NxU32 cpmPairHash(const CpmPairHash* hash, NxU32 key0, NxU32 key1)
	{
	if(key1 < key0)
		{
		const NxU32 swap = key0;
		key0 = key1;
		key1 = swap;
		}
	NxU32 value = ((key1 & 0xffffu) << 16) | (key0 & 0xffffu);
	value += ~(value << 15);
	value = (static_cast<NxU32>(static_cast<NxI32>(value) >> 10) ^ value) * 9;
	value ^= static_cast<NxU32>(static_cast<NxI32>(value) >> 6);
	value += ~(value << 11);
	return (static_cast<NxU32>(static_cast<NxI32>(value) >> 16) ^ value)
		& *reinterpret_cast<const NxU32*>(reinterpret_cast<const NxU8*>(hash) + 4);
	}

static void cpmPairHashEnsure(CpmPairHash* hash)
	{
	if(hash->entries || hash->count)
		return;
	*reinterpret_cast<NxU32*>(reinterpret_cast<NxU8*>(hash) + 0x18) = 0xffffffffu;
	}

static void cpmPairHashGrow(CpmPairHash* hash)
	{
	NxU8* bytes = reinterpret_cast<NxU8*>(hash);
	const NxU32 oldCapacity = *reinterpret_cast<NxU32*>(bytes + 4) + 1;
	const NxU32 newCapacity = oldCapacity > 1 ? oldCapacity * 2 : 8;
	NxI32* buckets = static_cast<NxI32*>(nxFoundationSDKAllocator->malloc(
		newCapacity * sizeof(NxI32), NX_MEMORY_PERSISTENT));
	NxI32* links = static_cast<NxI32*>(nxFoundationSDKAllocator->malloc(
		newCapacity * sizeof(NxI32), NX_MEMORY_PERSISTENT));
	CpmPairHashEntry* entries = static_cast<CpmPairHashEntry*>(nxFoundationSDKAllocator->malloc(
		newCapacity * sizeof(CpmPairHashEntry), NX_MEMORY_PERSISTENT));
	if(!buckets || !links || !entries)
		{
		if(buckets) nxFoundationSDKAllocator->free(buckets);
		if(links) nxFoundationSDKAllocator->free(links);
		if(entries) nxFoundationSDKAllocator->free(entries);
		return;
		}
	for(NxU32 i = 0; i < newCapacity; ++i)
		buckets[i] = links[i] = -1;
	const NxU32 count = hash->count;
	*reinterpret_cast<NxU32*>(bytes + 4) = newCapacity - 1;
	for(NxU32 i = 0; i < count; ++i)
		{
		entries[i] = hash->entries[i];
		const NxU32 bucket = cpmPairHash(hash, entries[i].key0, entries[i].key1)
			& (newCapacity - 1);
		links[i] = buckets[bucket];
		buckets[bucket] = static_cast<NxI32>(i);
		}
	NxI32* oldBuckets = *reinterpret_cast<NxI32**>(bytes + 8);
	NxI32* oldLinks = *reinterpret_cast<NxI32**>(bytes + 0x0c);
	CpmPairHashEntry* oldEntries = hash->entries;
	*reinterpret_cast<NxI32**>(bytes + 8) = buckets;
	*reinterpret_cast<NxI32**>(bytes + 0x0c) = links;
	hash->entries = entries;
	if(oldBuckets) nxFoundationSDKAllocator->free(oldBuckets);
	if(oldLinks) nxFoundationSDKAllocator->free(oldLinks);
	if(oldEntries) nxFoundationSDKAllocator->free(oldEntries);
	}

// Row 004153 (0x0009a570, 156 B): find (key0, key1). thiscall on the
// hash; returns the entry or null.
static __declspec(noinline) CpmPairHashEntry* cpmOpen004153(CpmPairHash* hash, NxU32 key0, NxU32 key1)
	{
	if(key1 < key0)
		{
		const NxU32 swap = key0;
		key0 = key1;
		key1 = swap;
		}
	return static_cast<CpmPairHashEntry*>(NxFindCollisionPairRecord(hash,
		static_cast<NxU16>(key0), static_cast<NxU16>(key1)));
	}

// Row 004155 (0x0009a610, 772 B): insert (key0, key1) -> value; returns
// the entry. thiscall on the hash.
static __declspec(noinline) CpmPairHashEntry* cpmOpen004155(CpmPairHash* hash, NxU32 key0, NxU32 key1, void* value)
	{
	if(key1 < key0)
		{
		const NxU32 swap = key0;
		key0 = key1;
		key1 = swap;
		}
	cpmPairHashEnsure(hash);
	if(CpmPairHashEntry* found = cpmOpen004153(hash, key0, key1))
		{
		found->value = reinterpret_cast<NxU32>(value);
		return found;
		}
	if(!*reinterpret_cast<NxU32*>(reinterpret_cast<NxU8*>(hash) + 8)
		|| hash->count >= *reinterpret_cast<NxU32*>(reinterpret_cast<NxU8*>(hash) + 4) + 1)
		cpmPairHashGrow(hash);
	NxU8* bytes = reinterpret_cast<NxU8*>(hash);
	NxI32* buckets = *reinterpret_cast<NxI32**>(bytes + 8);
	NxI32* links = *reinterpret_cast<NxI32**>(bytes + 0x0c);
	if(!buckets || !links || !hash->entries)
		return 0;
	const NxU32 index = hash->count;
	CpmPairHashEntry* entry = hash->entries + index;
	entry->key0 = static_cast<NxU16>(key0);
	entry->key1 = static_cast<NxU16>(key1);
	entry->value = reinterpret_cast<NxU32>(value);
	const NxU32 bucket = cpmPairHash(hash, key0, key1);
	links[index] = buckets[bucket];
	buckets[bucket] = static_cast<NxI32>(index);
	++hash->count;
	return entry;
	}

// Row 004157 (0x0009a920, 476 B): erase (key0, key1). thiscall on the
// hash.
static __declspec(noinline) void cpmOpen004157(CpmPairHash* hash, NxU32 key0, NxU32 key1)
	{
	NxRemoveCollisionPairRecord(hash, static_cast<NxU16>(key0), static_cast<NxU16>(key1));
	}

// .data 0x10123c28: the SDK's actor-group pair-flags hash (NpPhysicsSDK.cpp's
// setActorGroupPairFlags is blocked on the same object). The oracle passes its
// address to 004153 (`mov ecx,0x10123c28`, 0x1001ffa4 and 0x10020156); the
// candidate has no such object.
static CpmPairHash* const kCpmActorGroupPairFlags = 0;

// ---------------------------------------------------------------------------
// The rows
// ---------------------------------------------------------------------------

// phys_fn_000750 (0x00016fb0, 45 B)
// phys_fn_000752 (0x00016fe0, 36 B)
// Whether a joint between two bodies disables their contacts. thiscall on
// body `this`, `ret 4`. Walks this body's joint list (+0x1d8, linked through
// joint +0x34) for a joint whose +0x08 or +0x0c is `other`, then (000752,
// entered by the `jmp` at 0x10016fdb) other's list for one naming `this`.
// Found: !(joint +0x2c bit 8) (`shr eax,8; not al; and eax,1`); none: 0.
__declspec(noinline) NxU32 CpmJointedBody::row000750(const CpmJointedBody* other) const
	{
	const NxU8* joint = cpmAt<const NxU8*>(this, 0x1d8);
	for(; joint; joint = cpmAt<const NxU8*>(joint, 0x34))
		{
		if(cpmAt<const void*>(joint, 8) == other || cpmAt<const void*>(joint, 0xc) == other)
			return (~(cpmAt<NxU32>(joint, 0x2c) >> 8)) & 1;
		}
	joint = cpmAt<const NxU8*>(other, 0x1d8);
	for(; joint; joint = cpmAt<const NxU8*>(joint, 0x34))
		{
		if(cpmAt<const void*>(joint, 8) == this || cpmAt<const void*>(joint, 0xc) == this)
			return (~(cpmAt<NxU32>(joint, 0x2c) >> 8)) & 1;
		}
	return 0;
	}

// phys_fn_000855 (0x0001ca00, 75 B)
// NxCombineMode on two floats: 0 (a + b) * 0.5f ([0x101043cc]), 1 min, 2
// a * b, else max; the result is left in st(0). stdcall, `ret 0xc`.
__declspec(noinline) double __stdcall cpmCombine0855(NxReal a, NxReal b, NxU32 mode)
	{
	return cpmCombine(a, b, (NxI32)mode);
	}

// phys_fn_000857 (0x0001ca50, 209 B)
// The restitution of a material pair: material A is the low 16 bits of the
// ids, B the high; A's and B's restitution combined with the larger of their
// restitution combine modes into out[0]; out[1] = (A.flags | B.flags) & 4.
// stdcall, `ret 8`.
__declspec(noinline) void __stdcall cpmRestitution0857(NxU32 materialIds, CpmRestitution* out)
	{
	const NxMaterial* a = cpmMaterial(materialIds & 0xffff);
	const NxMaterial* b = cpmMaterial(materialIds >> 16);
	const NxI32 mode = cpmMaxMode(a->restitutionCombineMode, b->restitutionCombineMode);
	out->restitution = (NxReal)cpmCombine(a->restitution, b->restitution, mode);
	out->spring = (b->flags | a->flags) & 4;
	}

// phys_fn_000859 (0x0001cb30, 607 B)
// The anisotropic friction of `material` (the anisotropic one) against
// `other`. stdcall, `ret 0x14`. `other` is made isotropic when it is itself
// anisotropic (the averages at 0x1001cb3b; its static value is spilled to a
// float, its dynamic one stays in st(0)); the world anisotropy direction is
// `shape`'s rotation (+0x0c..+0x2c) times material.dirOfAnisotropy; the four
// coefficients are combined with the larger friction combine mode; the two
// dynamic ones are scaled by NX_DYN_FRICT_SCALING and clamped to [0, 1], the
// two static ones scaled by NX_STA_FRICT_SCALING, raised to their dynamic
// counterpart and multiplied by the normal force.
__declspec(noinline) void __stdcall cpmAnisotropicFriction0859(const NxU8* shape,
	const NxMaterial* material, const NxMaterial* other, NxReal normalForce, CpmFrictionParams* out)
	{
	const NxReal* parameter = nxPhysicsSDKParameters();
	double otherDynamic;
	NxReal otherStatic;
	if(other->flags & NX_MF_ANISOTROPIC)
		{
		otherDynamic = ((double)other->dynamicFrictionV + other->dynamicFriction) * 0.5;
		otherStatic = (NxReal)(((double)other->staticFrictionV + other->staticFriction) * 0.5);
		}
	else
		{
		otherStatic = other->staticFriction;
		otherDynamic = other->dynamicFriction;
		}

	const NxReal* m = reinterpret_cast<const NxReal*>(shape + 0xc);
	const NxVec3& d = material->dirOfAnisotropy;
	const double y = ((double)m[5] * d.z + (double)m[3] * d.x) + (double)m[4] * d.y;
	const double z = ((double)m[8] * d.z + (double)m[6] * d.x) + (double)m[7] * d.y;
	const double x = ((double)m[2] * d.z + (double)m[1] * d.y) + (double)d.x * m[0];
	out->anisotropyDirection.x = (NxReal)x;
	out->anisotropyDirection.y = (NxReal)y;
	out->anisotropyDirection.z = (NxReal)z;

	const NxI32 mode = cpmMaxMode(material->frictionCombineMode, other->frictionCombineMode);
	const double dynamic = cpmCombine(material->dynamicFriction, otherDynamic, mode);
	out->dynamicFriction = (NxReal)((double)parameter[NX_DYN_FRICT_SCALING] * dynamic);
	const double dynamicV = cpmCombine(material->dynamicFrictionV, otherDynamic, mode);
	out->dynamicFrictionV = (NxReal)((double)parameter[NX_DYN_FRICT_SCALING] * dynamicV);
	if(out->dynamicFriction > 1.0f)
		out->dynamicFriction = 1.0f;
	if(out->dynamicFriction < 0.0f)
		out->dynamicFriction = 0.0f;
	if(out->dynamicFrictionV > 1.0f)
		out->dynamicFrictionV = 1.0f;
	if(out->dynamicFrictionV < 0.0f)
		out->dynamicFrictionV = 0.0f;

	NxReal staticF = (NxReal)((double)parameter[NX_STA_FRICT_SCALING]
		* cpmCombine(material->staticFriction, otherStatic, mode));
	double staticV = cpmCombine(material->staticFrictionV, otherStatic, mode)
		* (double)parameter[NX_STA_FRICT_SCALING];
	if(staticF < out->dynamicFriction)
		staticF = out->dynamicFriction;
	if(staticV < out->dynamicFrictionV)
		staticV = out->dynamicFrictionV;
	out->staticFriction = (NxReal)((double)staticF * normalForce);
	out->staticFrictionV = (NxReal)(staticV * normalForce);
	}

// phys_fn_000861 (0x0001cd90, 791 B)
// The friction parameters and tangents of one patch. stdcall, `ret 0x1c`.
// Materials A (low 16 bits of the ids) and B (high); out->spring = (A.flags |
// B.flags) & 4; a normal force that is not > 0 disables the patch.
// Isotropic (neither material has NX_MF_ANISOTROPIC): 000855 with the larger
// friction combine mode, scaled and clamped as 000859 does (the > 1 test is on
// the unrounded product, 0x1001ce5b `fst` then `fcomp`), static raised to
// dynamic and times the normal force into both static words, the dynamic
// value copied into dynamicV; enabled when either result is > 0; tangents from
// NxNormalToTangents ([0x1010418c]).
// Anisotropic: 000859 on the anisotropic side (the choice is on the whole
// flags words and, when both are non-zero, on |dynamic - dynamicV|); t1 =
// dir x n normalized when |dir x n| > 0.01f ([0x1010691c]) and t2 = n x t1,
// else NxNormalToTangents and the two coefficient pairs averaged.
__declspec(noinline) void __stdcall cpmFrictionParams0861(NxU8* const* shapes, NxU32 materialIds,
	const NxVec3* normal, NxReal normalForce, CpmFrictionParams* out, NxVec3* tangent0, NxVec3* tangent1)
	{
	const NxReal* parameter = nxPhysicsSDKParameters();
	const NxMaterial* a = cpmMaterial(materialIds & 0xffff);
	const NxMaterial* b = cpmMaterial(materialIds >> 16);
	out->spring = (a->flags | b->flags) & 4;
	if(!(normalForce > 0.0f))
		{
		out->enabled = 0;
		return;
		}

	if(!(a->flags & NX_MF_ANISOTROPIC) && !(b->flags & NX_MF_ANISOTROPIC))
		{
		NxI32 mode = cpmMaxMode(a->frictionCombineMode, b->frictionCombineMode);
		const double dynamic = cpmCombine0855(a->dynamicFriction, b->dynamicFriction, (NxU32)mode)
			* (double)parameter[NX_DYN_FRICT_SCALING];
		out->dynamicFriction = (NxReal)dynamic;
		if(dynamic > 1.0)
			out->dynamicFriction = 1.0f;
		if(out->dynamicFriction < 0.0f)
			out->dynamicFriction = 0.0f;
		mode = cpmMaxMode(a->frictionCombineMode, b->frictionCombineMode);
		double staticF = cpmCombine0855(a->staticFriction, b->staticFriction, (NxU32)mode)
			* (double)parameter[NX_STA_FRICT_SCALING];
		if(staticF < out->dynamicFriction)
			staticF = out->dynamicFriction;
		staticF = staticF * normalForce;
		cpmCopyWords(&out->dynamicFrictionV, &out->dynamicFriction, 1);
		out->staticFriction = (NxReal)staticF;
		out->staticFrictionV = (NxReal)staticF;
		out->enabled = (staticF > 0.0 || out->dynamicFriction > 0.0f) ? 1 : 0;
		NxNormalToTangents(*normal, *tangent0, *tangent1);
		return;
		}

	if(b->flags == 0)
		cpmAnisotropicFriction0859(shapes[0], a, b, normalForce, out);
	else if(a->flags == 0)
		cpmAnisotropicFriction0859(shapes[1], b, a, normalForce, out);
	else if(fabs((double)b->dynamicFriction - b->dynamicFrictionV)
		< fabs((double)a->dynamicFriction - a->dynamicFrictionV))
		cpmAnisotropicFriction0859(shapes[0], a, b, normalForce, out);
	else
		cpmAnisotropicFriction0859(shapes[1], b, a, normalForce, out);
	out->enabled = 1;

	// dir x n: x and y spilled to floats, z kept (0x1001cf81..0x1001cfc2).
	const NxVec3& d = out->anisotropyDirection;
	const NxReal cx = (NxReal)((double)d.y * normal->z - (double)d.z * normal->y);
	const NxReal cy = (NxReal)((double)d.z * normal->x - (double)d.x * normal->z);
	const double cz = (double)d.x * normal->y - (double)d.y * normal->x;
	tangent0->y = cy;
	tangent0->x = cx;
	tangent0->z = (NxReal)cz;
	const double length = x87FsqrtDot3(cz, cz, cy, cy, cx, cx);
	if(length > (double)0.01f)
		{
		const double inverse = 1.0 / length;
		const NxReal t0x = (NxReal)((double)cx * inverse);
		tangent0->x = t0x;
		const NxReal t0y = (NxReal)((double)cy * inverse);
		tangent0->y = t0y;
		const double t0z = cz * inverse;
		tangent0->z = (NxReal)t0z;
		const NxReal t1x = (NxReal)(t0z * normal->y - (double)t0y * normal->z);
		const double t1y = (double)t0x * normal->z - t0z * normal->x;
		const double t1z = (double)t0y * normal->x - (double)t0x * normal->y;
		tangent1->x = t1x;
		tangent1->y = (NxReal)t1y;
		tangent1->z = (NxReal)t1z;
		return;
		}
	NxNormalToTangents(*normal, *tangent0, *tangent1);
	const NxReal staticAverage = (NxReal)(((double)out->staticFriction + out->staticFrictionV) * 0.5);
	out->staticFrictionV = staticAverage;
	out->staticFriction = staticAverage;
	const NxReal dynamicAverage = (NxReal)(((double)out->dynamicFriction + out->dynamicFrictionV) * 0.5);
	out->dynamicFrictionV = dynamicAverage;
	out->dynamicFriction = dynamicAverage;
	}

// phys_fn_000863 (0x0001d0b0, 8 B)
// `add ecx,0x10; jmp 0x1005b620`: 002354 on the pair's stream sub-object.
__declspec(noinline) void __fastcall cpmActorPairResetStream0863(NxActorPair* pair)
	{
	cpmOpen002354(pair->mBytes + 0x10);
	}

// phys_fn_000865 (0x0001d0c0, 409 B)
// patch +0x74 = count; for each of the first count points, the point in body
// 0's frame into anchor i's first half (+0x04 + 0x18 i) and in body 1's into
// its second (+0x10 + 0x18 i), copied as is for a missing body; +0x75 = 0.
// The loop bound is reloaded from +0x74 (0x1001d23b).
__declspec(noinline) void NxActorPair::row000865(NxFrictionPatch* patch, const NxReal* const* points,
	NxU8 count)
	{
	patch->at<NxU8>(0x74) = count;
	if(count)
		{
		NxU32 i = 0;
		do
			{
			NxReal* anchor = &patch->at<NxReal>(0x04 + 0x18 * i);
			const NxU8* body0 = at<const NxU8*>(8);
			if(body0)
				cpmToBodyLocal0865(body0, points[i], anchor);
			else
				cpmCopyWords(anchor, points[i], 3);
			const NxU8* body1 = at<const NxU8*>(0xc);
			if(body1)
				cpmToBodyLocal0865(body1, points[i], anchor + 3);
			else
				cpmCopyWords(anchor + 3, points[i], 3);
			i++;
			}
		while(i < patch->at<NxU8>(0x74));
		}
	patch->at<NxU8>(0x75) = 0;
	}

// phys_fn_000871 (0x0001d590, 122 B)
// (flags of material [patch+0x68] | flags of material [patch+0x6a]) & 4
// (NX_MF_SPRING_CONTACT). cdecl.
__declspec(noinline) NxU32 __cdecl cpmPatchSpring0871(const NxFrictionPatch* patch)
	{
	const NxMaterial* a = cpmMaterial(patch->at<NxU16>(0x68));
	const NxMaterial* b = cpmMaterial(patch->at<NxU16>(0x6a));
	return (b->flags | a->flags) & 4;
	}

// The stream appends 000875 makes through the pair's SdkContainer (+0x38),
// growing it with 004840 first: `count == capacity` before a single word,
// `count + 3 > capacity` before three.
static NX_INLINE void cpmStreamAppend(SdkContainer* stream, NxU32 word)
	{
	if(stream->mCount == stream->mCapacity)
		stream->resize(1);
	stream->mEntries[stream->mCount] = word;
	stream->mCount++;
	}

static NX_INLINE void cpmStreamAppend3(SdkContainer* stream, const void* words)
	{
	if(stream->mCount + 3 > stream->mCapacity)
		stream->resize(3);
	cpmCopyWords(stream->mEntries + stream->mCount, words, 3);
	stream->mCount += 3;
	}

// phys_fn_000875 (0x0001d8e0, 915 B)
// The contact-stream emitter for 32-bit features; the pair is the sink.
// thiscall, `ret 0x24`. As 000873, plus: the pair header's flag word gains 4
// when either shape's +0xde has bit 0x20; with that flag set, a contact whose
// full feature ids do not both fit in 16 bits gets bit 31 in its separation
// word and both ids as two words, otherwise one word fid1 << 16 | fid0.
// Unlike the candidate's 000873, every append grows the stream through 004840
// (SdkContainer::resize) at the listing's eight sites.
__declspec(noinline) void NxActorPair::row000875(const NxU8* object1, const NxU8* object0,
	NxU32 separationBits, const NxReal* point, const NxReal* normal, NxU16 featureId0,
	NxU16 featureId1, NxU32 feature0, NxU32 feature1)
	{
	const NxU8* shape1 = cpmAt<const NxU8*>(object1, 8);
	const NxU8* shape0 = cpmAt<const NxU8*>(object0, 8);
	NxReal negated[3];
	if(cpmAt<const void*>(cpmAt<const NxU8*>(shape1, 4), 8) != at<const void*>(8))
		{
		const NxU8* swapShape = shape1;
		shape1 = shape0;
		shape0 = swapShape;
		const NxU32 swapFeature = feature0;
		feature0 = feature1;
		feature1 = swapFeature;
		const NxU16 swapId = featureId0;
		featureId0 = featureId1;
		featureId1 = swapId;
		negated[0] = (NxReal)(-(double)normal[0]);
		negated[1] = (NxReal)(-(double)normal[1]);
		negated[2] = (NxReal)(-(double)normal[2]);
		normal = negated;
		}

	SdkContainer* stream = reinterpret_cast<SdkContainer*>(mBytes + 0x38);
	if(at<const void*>(0x20) != cpmAt<const void*>(shape1, 0x9c)
		|| at<const void*>(0x24) != cpmAt<const void*>(shape0, 0x9c))
		{
		const NxU32 valid = (featureId0 != 0xffff && featureId1 != 0xffff) ? 1u : 0u;
		const NxU32 wide = ((shape1[0xde] & 0x20) || (shape0[0xde] & 0x20)) ? 4u : 0u;
		const NxU32 flags = wide | valid;
		at<NxU32>(0x34) = flags;
		const NxU32 packed = flags << 16;
		at<const void*>(0x20) = cpmAt<const void*>(shape1, 0x9c);
		at<const void*>(0x24) = cpmAt<const void*>(shape0, 0x9c);
		cpmStreamAppend(stream, cpmAt<NxU32>(shape1, 0x9c));
		cpmStreamAppend(stream, cpmAt<NxU32>(shape0, 0x9c));
		const NxU8* holder = cpmAt<const NxU8*>(cpmAt<const NxU8*>(shape1, 4), 8);
		const NxU32 material = holder
			? cpmAt<NxU32>(holder, 0x240)
			: cpmAt<NxU32>(cpmAt<const NxU8*>(cpmAt<const NxU8*>(shape0, 4), 8), 0x240);
		const NxU32 normalCountIndex = stream->mCount;
		cpmStreamAppend(stream, (material << 24) | packed);
		at<NxU32>(0x18) = normalCountIndex;
		stream->mEntries[at<NxU32>(0x14)]++;
		at<NxU32>(0x30) = 0;
		at<NxU32>(0x2c) = 0;
		at<NxU32>(0x28) = 0;
		}

	const NxU32* normalWords = reinterpret_cast<const NxU32*>(normal);
	if(at<NxU32>(0x28) != normalWords[0] || at<NxU32>(0x2c) != normalWords[1]
		|| at<NxU32>(0x30) != normalWords[2])
		{
		cpmCopyWords(mBytes + 0x28, normalWords, 3);
		cpmStreamAppend3(stream, normalWords);
		const NxU32 pointCountIndex = stream->mCount;
		cpmStreamAppend(stream, 0);
		at<NxU32>(0x1c) = pointCountIndex;
		stream->mEntries[at<NxU32>(0x18)]++;
		}

	const NxU32 wideFeature = (feature0 > 0xffff || feature1 > 0xffff) ? 0x80000000u : 0u;
	at<NxU32>(0x10)++;
	cpmStreamAppend3(stream, point);
	cpmStreamAppend(stream, (separationBits & 0x7fffffff) | wideFeature);
	stream->mEntries[at<NxU32>(0x1c)]++;
	if(at<NxU32>(0x34) & 1)
		cpmStreamAppend(stream, ((NxU32)featureId1 << 16) | (NxU32)featureId0);
	if(at<NxU32>(0x34) & 4)
		{
		if(wideFeature)
			{
			cpmStreamAppend(stream, feature0);
			cpmStreamAppend(stream, feature1);
			}
		else
			cpmStreamAppend(stream, (feature1 << 16) | feature0);
		}
	}

// The patch's first three slots: the folded empty bodies 004248 (`ret 4`),
// 001583 (`ret`) and 005242 (`ret 8`). Not claimed here.
void NxFrictionPatch::slot0(NxU32)
	{
	}

void NxFrictionPatch::slot1()
	{
	}

void NxFrictionPatch::slot2(NxU32, NxU32)
	{
	}

// Slot 3 is phys_fn_000867 (another sub-area's row, ObjectModel.cpp); see the
// header.
void NxFrictionPatch::accumulate(NxReal value, void* record, NxReal divisor)
	{
	nxAccumulateByKind0867(this, value, record, divisor);
	}

// phys_fn_000877 (0x0001dc80, 189 B)
// thiscall, `ret 4`; `mov eax,ecx` first, so it returns this.
__declspec(noinline) NxFrictionPatch& NxFrictionPatch::operator=(const NxFrictionPatch& other)
	{
	cpmCopyWords(mData, other.mData, 0x1c);
	mData[0x70] = other.mData[0x70];
	mData[0x71] = other.mData[0x71];
	return *this;
	}

// phys_fn_000879 (0x0001dd40, 1983 B)
// The friction rows. thiscall, `ret 0xc`. For each patch (the embedded one,
// then the vector) whose normal force (+0x64) is not 0.0f: 000861 with
// forceScale * normalForce; if enabled, for each anchor: both anchors in the
// world, the tangential drift e1 (along t1, stored) and e2 (along t2, kept),
// the point W1 - t1 e1 - t2 e2 as body 0's lever, the error (r0 + p0) -
// (r1 + p1) along t1 and t2 times errorScale, and two 0x50-byte records (t1
// with staticFriction/dynamicFriction, t2 with the V pair), each of kind 4
// through 004391. Then the patch's +0x58..+0x64 are zeroed.
__declspec(noinline) void NxActorPair::row000879(NxSceneInternal* scene, NxReal forceScale,
	NxReal errorScale)
	{
	const NxReal* parameter = nxPhysicsSDKParameters();
	JointSupportBody* const support0 = cpmBodySupport(at<const NxU8*>(8));
	JointSupportBody* const support1 = cpmBodySupport(at<const NxU8*>(0xc));
	for(NxU32 i = 0; i < at<NxU32>(0x48); i++)
		{
		NxFrictionPatch* patch = i == 0
			? reinterpret_cast<NxFrictionPatch*>(mBytes + 0x4c)
			: at<NxFrictionPatch**>(0xc4)[i - 1];
		if(patch->at<NxReal>(0x64) != 0.0f)
			{
			CpmFrictionParams params;
			NxVec3 t1;
			NxVec3 t2;
			cpmFrictionParams0861(&patch->at<NxU8*>(0x6c), patch->at<NxU32>(0x68),
				&patch->at<NxVec3>(0x4c), (NxReal)((double)forceScale * patch->at<NxReal>(0x64)),
				&params, &t1, &t2);
			if(params.enabled && patch->at<NxU8>(0x74))
				{
				NxU32 j = 0;
				do
					{
					const NxReal* local0 = &patch->at<NxReal>(0x04 + 0x18 * j);
					const NxReal* local1 = &patch->at<NxReal>(0x10 + 0x18 * j);
					const NxU8* body0 = at<const NxU8*>(8);
					NxReal w0[3];
					if(body0)
						{
						const NxReal* m = cpmBodyRotation(body0);
						const NxReal* p = cpmBodyPosition(body0);
						const double ax = ((double)m[2] * local0[2] + (double)m[1] * local0[1])
							+ (double)m[0] * local0[0];
						const NxReal ay = (NxReal)(((double)m[5] * local0[2] + (double)m[4] * local0[1])
							+ (double)m[3] * local0[0]);
						const NxReal az = (NxReal)(((double)m[8] * local0[2] + (double)m[7] * local0[1])
							+ (double)m[6] * local0[0]);
						w0[2] = (NxReal)((double)az + p[2]);
						w0[0] = (NxReal)(ax + p[0]);
						w0[1] = (NxReal)((double)ay + p[1]);
						}
					else
						cpmCopyWords(w0, local0, 3);

					const NxU8* body1 = at<const NxU8*>(0xc);
					NxReal r1[3];
					double w1x;
					NxReal w1y;
					NxReal w1z;
					if(body1)
						{
						const NxReal* m = cpmBodyRotation(body1);
						const NxReal* p = cpmBodyPosition(body1);
						r1[0] = (NxReal)(((double)m[2] * local1[2] + (double)m[1] * local1[1])
							+ (double)m[0] * local1[0]);
						r1[1] = (NxReal)(((double)m[5] * local1[2] + (double)m[4] * local1[1])
							+ (double)m[3] * local1[0]);
						r1[2] = (NxReal)(((double)m[8] * local1[2] + (double)m[7] * local1[1])
							+ (double)m[6] * local1[0]);
						w1x = (double)r1[0] + p[0];
						w1y = (NxReal)((double)r1[1] + p[1]);
						w1z = (NxReal)((double)r1[2] + p[2]);
						}
					else
						{
						cpmCopyWords(r1, local1, 3);
						w1x = r1[0];
						w1y = r1[1];
						w1z = r1[2];
						}

					// The drift along the two tangents (0x1001dfb4..0x1001e0a3).
					const double dx = w1x - w0[0];
					const double dy = (double)w1y - w0[1];
					const double dz = (double)w1z - w0[2];
					const NxReal e1 = (NxReal)((dx * t1.x + (double)t1.z * dz) + (double)t1.y * dy);
					const double e2 = (dx * t2.x + (double)t2.z * dz) + (double)t2.y * dy;
					const NxReal q2x = (NxReal)((double)t2.x * e2);
					const NxReal q2y = (NxReal)((double)t2.y * e2);
					const NxReal q2z = (NxReal)((double)t2.z * e2);
					const double q1x = (double)t1.x * e1;
					const NxReal q1y = (NxReal)((double)t1.y * e1);
					const NxReal q1z = (NxReal)((double)t1.z * e1);
					const NxReal px = (NxReal)(w1x - q1x);
					const double py = (double)w1y - q1y;
					const double pz = (double)w1z - q1z;
					const NxReal pointX = (NxReal)((double)px - q2x);
					const NxReal pointY = (NxReal)(py - q2y);
					const double pointZ = pz - q2z;
					NxReal r0[3];
					r0[0] = pointX;
					r0[1] = pointY;
					r0[2] = (NxReal)pointZ;
					if(body0)
						{
						const NxReal* p = cpmBodyPosition(body0);
						r0[0] = (NxReal)((double)pointX - p[0]);
						r0[1] = (NxReal)((double)pointY - p[1]);
						r0[2] = (NxReal)(pointZ - p[2]);
						}

					// The error along the tangents (0x1001e0d3..0x1001e171).
					double ex = (double)r0[0] - r1[0];
					double ey = (double)r0[1] - r1[1];
					double ez = (double)r0[2] - r1[2];
					if(body0)
						{
						const NxReal* p = cpmBodyPosition(body0);
						ex = ex + p[0];
						ey = ey + p[1];
						ez = ez + p[2];
						}
					if(body1)
						{
						const NxReal* p = cpmBodyPosition(body1);
						ex = ex - p[0];
						ey = ey - p[1];
						ez = ez - p[2];
						}
					const NxReal f1 = (NxReal)(((ex * t1.x + ez * t1.z) + ey * t1.y) * errorScale);
					const NxReal f2 = (NxReal)(((ex * t2.x + ez * t2.z) + ey * t2.y) * errorScale);

					for(NxU32 k = 0; k < 2; k++)
						{
						const NxVec3& t = k == 0 ? t1 : t2;
						JointSupportRecord* record = cpmTakeRecord(scene);
						record->mBody[1] = support1;
						record->mBody[0] = support0;
						cpmCopyWords(&record->mUnknown000, &t, 3);
						const double ay0 = (double)r0[2] * t.x - (double)t.z * r0[0];
						const double az0 = (double)t.y * r0[0] - (double)r0[1] * t.x;
						const double ax0 = (double)r0[1] * t.z - (double)r0[2] * t.y;
						record->mUnknown018.x = (NxReal)ax0;
						record->mUnknown018.y = (NxReal)ay0;
						record->mUnknown018.z = (NxReal)az0;
						const double ay1 = (double)r1[2] * t.x - (double)t.z * r1[0];
						const double az1 = (double)t.y * r1[0] - (double)r1[1] * t.x;
						const double ax1 = (double)t.z * r1[1] - (double)t.y * r1[2];
						record->mUnknown024.x = (NxReal)ax1;
						record->mUnknown024.y = (NxReal)ay1;
						record->mUnknown024.z = (NxReal)az1;
						cpmRecordKind(record, 0xffffffe4, 4);
						record->mUnknown048 = k == 0 ? params.staticFriction : params.staticFrictionV;
						record->mUnknown038 = 0.0f;
						record->mUnknown044 = 0;
						record->mUnknown04c = 0;
						record->mUnknown034 = k == 0 ? f1 : f2;
						record->mUnknown030 = patch;
						cpmSolveRecord(record, parameter);
						record->mUnknown04c = cpmBits(k == 0 ? params.dynamicFriction : params.dynamicFrictionV);
						}
					j++;
					}
				while(j < patch->at<NxU8>(0x74));
				}
			}
		patch->at<NxU32>(0x64) = 0;
		patch->at<NxU32>(0x60) = 0;
		patch->at<NxU32>(0x5c) = 0;
		patch->at<NxU32>(0x58) = 0;
		}
	}

// phys_fn_000881 (0x0001e500, 213 B)
// sumNormalForce = sum of patch normal (+0x4c) * normal force (+0x64),
// sumFrictionForce = sum of the patches' +0x58 vectors, over the embedded
// patch (pair +0x98/+0xb0/+0xa4) and the first count - 1 vector patches.
// thiscall, `ret 8`. Nothing is written when the count is 0.
__declspec(noinline) void NxActorPair::row000881(NxVec3* sumNormalForce, NxVec3* sumFrictionForce)
	{
	if(at<NxU32>(0x48) == 0)
		return;
	const NxReal force = at<NxReal>(0xb0);
	sumNormalForce->x = (NxReal)((double)force * at<NxReal>(0x98));
	sumNormalForce->y = (NxReal)((double)force * at<NxReal>(0x9c));
	sumNormalForce->z = (NxReal)((double)force * at<NxReal>(0xa0));
	cpmCopyWords(sumFrictionForce, mBytes + 0xa4, 3);
	for(NxU32 i = 0; i < at<NxU32>(0x48) - 1; i++)
		{
		const NxFrictionPatch* patch = at<NxFrictionPatch**>(0xc4)[i];
		const double patchForce = patch->at<NxReal>(0x64);
		const double x = patchForce * patch->at<NxReal>(0x4c);
		const NxReal y = (NxReal)(patchForce * patch->at<NxReal>(0x50));
		const NxReal z = (NxReal)(patchForce * patch->at<NxReal>(0x54));
		sumNormalForce->x = (NxReal)(x + sumNormalForce->x);
		sumNormalForce->y = (NxReal)((double)y + sumNormalForce->y);
		sumNormalForce->z = (NxReal)((double)z + sumNormalForce->z);
		sumFrictionForce->x = (NxReal)((double)sumFrictionForce->x + patch->at<NxReal>(0x58));
		sumFrictionForce->y = (NxReal)((double)patch->at<NxReal>(0x5c) + sumFrictionForce->y);
		sumFrictionForce->z = (NxReal)((double)patch->at<NxReal>(0x60) + sumFrictionForce->z);
		}
	}

// phys_fn_000883 (0x0001e5e0, 200 B)
// phys_fn_000885 (0x0001e6b0, 593 B)
// The spring contact (a swept capsule: the patch shape of type 3 with +0xe8
// bit 0, NX_SWEPT_SHAPE; the oracle dereferences a null shape when neither
// is one). thiscall, `ret 0x14`; the third and fifth arguments are not read.
// The separation is the point's offset along the capsule's axis (rotation
// column 1) less its +0xe4 and NX_MIN_SEPARATION_FOR_PENALTY; one record of
// kind 0 with +0x34 = (separation + spring.targetValue) * Scene+0x54c and
// 004393(1 / (h (k h + d)), k h / (k h + d)), h = Scene+0x548, k/d the
// spring and damper of the capsule material's programData (NxSpringDesc).
__declspec(noinline) void NxActorPair::row000883(NxSceneInternal* scene, NxFrictionPatch* patch,
	NxU32 separationBits, const NxReal* point, const NxReal* normal)
	{
	(void)separationBits;
	(void)normal;
	const NxReal* parameter = nxPhysicsSDKParameters();
	const NxU8* body0 = at<const NxU8*>(8);
	JointSupportBody* const support0 = cpmBodySupport(body0);
	JointSupportBody* const support1 = cpmBodySupport(at<const NxU8*>(0xc));
	NxReal r0[3];
	NxReal r1[3];
	cpmCopyWords(r0, point, 3);
	cpmCopyWords(r1, r0, 3);
	if(body0)
		{
		const NxReal* p = cpmBodyPosition(body0);
		r0[0] = (NxReal)((double)r0[0] - p[0]);
		r0[1] = (NxReal)((double)r0[1] - p[1]);
		r0[2] = (NxReal)((double)r0[2] - p[2]);
		}
	const NxU8* body1 = at<const NxU8*>(0xc);
	if(body1)
		{
		const NxReal* p = cpmBodyPosition(body1);
		r1[0] = (NxReal)((double)r1[0] - p[0]);
		r1[1] = (NxReal)((double)r1[1] - p[1]);
		r1[2] = (NxReal)((double)r1[2] - p[2]);
		}

	// 000885 from here (0x1001e6b0): the shape search and the record.
	const NxU8* capsule = 0;
	for(NxU32 k = 0; k < 2; k++)
		{
		const NxU8* shape = patch->at<const NxU8*>(0x6c + 4 * k);
		if(cpmAt<NxU32>(shape, 0xd0) == 3 && (cpmAt<NxU8>(shape, 0xe8) & 1))
			{
			capsule = shape;
			break;
			}
		}
	const NxReal* position = reinterpret_cast<const NxReal*>(capsule + 0x30);
	const double dx = (double)point[0] - position[0];
	const double dy = (double)point[1] - position[1];
	const double dz = (double)point[2] - position[2];
	NxReal axis[3];
	cpmCopyWords(&axis[0], capsule + 0x10, 1);
	cpmCopyWords(&axis[1], capsule + 0x1c, 1);
	cpmCopyWords(&axis[2], capsule + 0x28, 1);
	const NxReal separation = (NxReal)(((((double)axis[2] * dz + (double)axis[1] * dy)
		+ (double)axis[0] * dx) - cpmAt<NxReal>(capsule, 0xe4)) - parameter[NX_MIN_SEPARATION_FOR_PENALTY]);

	JointSupportRecord* record = cpmTakeRecord(scene);
	const NxMaterial* material = cpmMaterial(cpmAt<NxU16>(capsule, 0xda));
	const NxReal* spring = static_cast<const NxReal*>(material->programData);
	const double target = ((double)separation + spring[2]) * scene->at<NxReal>(0x54c);
	const double step = scene->at<NxReal>(0x548);
	const double stiffness = (double)spring[0] * step;
	record->mBody[0] = support0;
	const NxReal stiffnessDamping = (NxReal)(stiffness + spring[1]);
	cpmCopyWords(&record->mUnknown000, axis, 3);
	const NxReal ratio = (NxReal)(stiffness / stiffnessDamping);
	record->mBody[1] = support1;
	const NxReal inverse = (NxReal)(1.0 / (step * stiffnessDamping));
	record->mUnknown018.x = (NxReal)((double)axis[2] * r0[1] - (double)axis[1] * r0[2]);
	record->mUnknown018.y = (NxReal)((double)r0[2] * axis[0] - (double)axis[2] * r0[0]);
	record->mUnknown018.z = (NxReal)((double)axis[1] * r0[0] - (double)r0[1] * axis[0]);
	record->mUnknown024.x = (NxReal)((double)axis[2] * r1[1] - (double)axis[1] * r1[2]);
	record->mUnknown024.y = (NxReal)((double)r1[2] * axis[0] - (double)axis[2] * r1[0]);
	record->mUnknown024.z = (NxReal)((double)axis[1] * r1[0] - (double)r1[1] * axis[0]);
	cpmRecordKind(record, 0xffffffe0, 0);
	record->mUnknown034 = (NxReal)target;
	record->mUnknown030 = patch;
	record->mUnknown038 = 0.0f;
	record->mUnknown044 = 0;
	record->mUnknown04c = 0;
	record->mUnknown048 = FLT_MAX;
	record->row004393(inverse, ratio);
	}

// phys_fn_000887 (0x0001e910, 41 B)
// phys_fn_000889 (0x0001e940, 99 B)
// The pair's release. fastcall. 002354 on the stream sub-object; every vector
// patch freed (the count re-read each pass, 0x1001e957), the patch count
// zeroed, the vector freed and zeroed; then the tail jump to 002352 (the
// SdkContainer::empty thunk) on the stream sub-object.
__declspec(noinline) void __fastcall cpmActorPairRelease0887(NxActorPair* pair)
	{
	cpmOpen002354(pair->mBytes + 0x10);
	for(NxU32 i = 0; i < (NxU32)(pair->at<NxFrictionPatch**>(0xc8) - pair->at<NxFrictionPatch**>(0xc4)); i++)
		nxFoundationSDKAllocator->free(pair->at<NxFrictionPatch**>(0xc4)[i]);
	pair->at<NxU32>(0x48) = 0;
	if(pair->at<void*>(0xc4))
		nxFoundationSDKAllocator->free(pair->at<void*>(0xc4));
	pair->at<void*>(0xc4) = 0;
	pair->at<void*>(0xc8) = 0;
	pair->at<void*>(0xcc) = 0;
	nxContainerAddThunk(pair->mBytes + 0x10);
	}

// phys_fn_000891 (0x0001e9b0, 1509 B)
// Refresh or drop each patch. thiscall, `ret 4`. Pair +0xd4 = 0; for each
// patch: both bodies' world normals and anchors from the stored local ones
// (the second anchor only when the count is not 0); the patch is dropped
// (its normal force added to +0xd4; a vector patch freed and replaced by the
// vector's last, the embedded one overwritten by the last through 000877)
// when it needs re-anchoring but is a spring patch, when the pair was not
// refreshed last frame (+0xd8 + 1 != Scene+0x540), when the two world normals
// are not > 0.996f apart ([0x1010693c]), or when an anchor pair has drifted
// apart along the normal by |d| not < -NX_MIN_SEPARATION_FOR_PENALTY (the
// second only when the count is not 1). A kept patch that needs re-anchoring
// is re-anchored at body 0's world anchors (000865's transform, 000891's
// operand order), and its world normal becomes body 0's. Last, +0xd8 =
// Scene+0x540.
__declspec(noinline) void NxActorPair::row000891(NxSceneInternal* scene)
	{
	const NxReal* parameter = nxPhysicsSDKParameters();
	at<NxU32>(0xd4) = 0;
	// The frame slots (+0x24..+0x6c) live across patches: a patch with no
	// anchors leaves the previous patch's second anchors in place and the
	// count != 1 test below still reads them, as the listing does. Before the
	// first patch the oracle's slots hold whatever the stack held; here, 0.
	NxReal normal[2][3] = {};
	NxReal anchor0[2][3] = {};
	NxReal anchor1[2][3] = {};
	NxU32 i = 0;
	if(at<NxU32>(0x48) != 0)
		{
		do
			{
			NxFrictionPatch* patch = i == 0
				? reinterpret_cast<NxFrictionPatch*>(mBytes + 0x4c)
				: at<NxFrictionPatch**>(0xc4)[i - 1];
			const NxU32 spring = cpmPatchSpring0871(patch);
			const NxU8 count = patch->at<NxU8>(0x74);
			for(NxU32 k = 0; k < 2; k++)
				{
				const NxU8* body = at<const NxU8*>(8 + 4 * k);
				const NxReal* n = &patch->at<NxReal>(0x34 + 0xc * k);
				const NxReal* a0 = &patch->at<NxReal>(0x04 + 0xc * k);
				const NxReal* a1 = &patch->at<NxReal>(0x1c + 0xc * k);
				if(body)
					{
					const NxReal* m = cpmBodyRotation(body);
					const NxReal* p = cpmBodyPosition(body);
					normal[k][0] = (NxReal)(((double)m[2] * n[2] + (double)m[1] * n[1]) + (double)n[0] * m[0]);
					normal[k][1] = (NxReal)(((double)m[5] * n[2] + (double)m[3] * n[0]) + (double)m[4] * n[1]);
					normal[k][2] = (NxReal)(((double)m[8] * n[2] + (double)m[6] * n[0]) + (double)m[7] * n[1]);
					{
					const double x = ((double)m[1] * a0[1] + (double)m[2] * a0[2]) + (double)a0[0] * m[0];
					const NxReal y = (NxReal)(((double)m[4] * a0[1] + (double)m[3] * a0[0]) + (double)m[5] * a0[2]);
					const NxReal z = (NxReal)(((double)m[7] * a0[1] + (double)m[6] * a0[0]) + (double)m[8] * a0[2]);
					const NxReal wz = (NxReal)((double)z + p[2]);
					anchor0[k][0] = (NxReal)(x + p[0]);
					anchor0[k][1] = (NxReal)((double)y + p[1]);
					anchor0[k][2] = wz;
					}
					if(count)
						{
						const double x = ((double)m[1] * a1[1] + (double)m[2] * a1[2]) + (double)a1[0] * m[0];
						const NxReal y = (NxReal)(((double)m[4] * a1[1] + (double)m[3] * a1[0]) + (double)m[5] * a1[2]);
						const NxReal z = (NxReal)(((double)m[7] * a1[1] + (double)m[6] * a1[0]) + (double)m[8] * a1[2]);
						const NxReal wz = (NxReal)((double)z + p[2]);
						anchor1[k][0] = (NxReal)(x + p[0]);
						anchor1[k][1] = (NxReal)((double)y + p[1]);
						anchor1[k][2] = wz;
						}
					}
				else
					{
					cpmCopyWords(normal[k], n, 3);
					cpmCopyWords(anchor0[k], a0, 3);
					if(count)
						cpmCopyWords(anchor1[k], a1, 3);
					}
				}

			const NxU8 reanchor = patch->at<NxU8>(0x75);
			bool drop = true;
			if(!(reanchor && spring) && at<NxU32>(0xd8) + 1 == scene->at<NxU32>(0x540)
				&& ((double)normal[1][0] * normal[0][0] + (double)normal[1][1] * normal[0][1])
					+ (double)normal[1][2] * normal[0][2] > (double)0.996f)
				{
				const double dx = (double)anchor0[0][0] - anchor0[1][0];
				const double dy = (double)anchor0[0][1] - anchor0[1][1];
				const double dz = (double)anchor0[0][2] - anchor0[1][2];
				const NxReal limit = (NxReal)(-(double)parameter[NX_MIN_SEPARATION_FOR_PENALTY]);
				if(fabs((dx * normal[0][0] + dy * normal[0][1]) + dz * normal[0][2]) < limit)
					{
					drop = false;
					if(count != 1)
						{
						const double ex = (double)anchor1[0][0] - anchor1[1][0];
						const double ey = (double)anchor1[0][1] - anchor1[1][1];
						const double ez = (double)anchor1[0][2] - anchor1[1][2];
						if(!(fabs((ex * normal[0][0] + ey * normal[0][1]) + ez * normal[0][2]) < limit))
							drop = true;
						}
					}
				}

			if(!drop)
				{
				if(spring == 0 && reanchor != 0)
					{
					patch->at<NxU8>(0x75) = 0;
					if(count)
						{
						NxU32 j = 0;
						do
							{
							const NxReal* world = j == 0 ? anchor0[0] : anchor1[0];
							NxReal* local = &patch->at<NxReal>(0x04 + 0x18 * j);
							const NxU8* body0 = at<const NxU8*>(8);
							if(body0)
								cpmToBodyLocal0891(body0, world, local);
							else
								cpmCopyWords(local, world, 3);
							const NxU8* body1 = at<const NxU8*>(0xc);
							if(body1)
								cpmToBodyLocal0891(body1, world, local + 3);
							else
								cpmCopyWords(local + 3, world, 3);
							j++;
							}
						while(j < patch->at<NxU8>(0x74));
						}
					}
				cpmCopyWords(&patch->at<NxReal>(0x4c), normal[0], 3);
				i++;
				}
			else
				{
				at<NxReal>(0xd4) = (NxReal)((double)patch->at<NxReal>(0x64) + at<NxReal>(0xd4));
				if(i != 0)
					{
					nxFoundationSDKAllocator->free(patch);
					NxFrictionPatch** begin = at<NxFrictionPatch**>(0xc4);
					NxFrictionPatch** end = at<NxFrictionPatch**>(0xc8);
					const NxU32 index = i - 1;
					if(index != (NxU32)(end - begin) - 1)
						begin[index] = end[-1];
					at<NxFrictionPatch**>(0xc8) = end - 1;
					}
				else if(at<NxU32>(0x48) > 1)
					{
					*reinterpret_cast<NxFrictionPatch*>(mBytes + 0x4c) = *at<NxFrictionPatch**>(0xc8)[-1];
					nxFoundationSDKAllocator->free(at<NxFrictionPatch**>(0xc8)[-1]);
					at<NxFrictionPatch**>(0xc8) = at<NxFrictionPatch**>(0xc8) - 1;
					}
				at<NxU32>(0x48)--;
				}
			}
		while(i != at<NxU32>(0x48));
		}
	at<NxU32>(0xd8) = scene->at<NxU32>(0x540);
	}

// phys_fn_000893 (0x0001efa0, 109 B)
// The constructor. thiscall, `ret 8`, returns this. 002356 on the stream
// sub-object; the embedded patch's vptr (0x10106944; only the vptr, the
// patch's own fields are left); the patch vector, the patch count, +0xd4 and
// +0xd8 zeroed; +0xe8/+0xe9 = 0xff; +0/+4 each shape's +0x04, +8/+0xc their
// +0x08.
__declspec(noinline) NxActorPair* NxActorPair::row000893(const NxU8* shape0, const NxU8* shape1)
	{
	cpmOpen002356(mBytes + 0x10);
	new (mBytes + 0x4c) NxFrictionPatch;
	at<void*>(0xc4) = 0;
	at<void*>(0xc8) = 0;
	at<void*>(0xcc) = 0;
	at<NxU8>(0xe8) = 0xff;
	at<NxU8>(0xe9) = 0xff;
	at<NxU32>(0x48) = 0;
	at<const void*>(0) = cpmAt<const void*>(shape0, 4);
	at<const void*>(4) = cpmAt<const void*>(shape1, 4);
	at<const void*>(8) = cpmAt<const void*>(at<const NxU8*>(0), 8);
	at<NxU32>(0xd4) = 0;
	at<NxU32>(0xd8) = 0;
	at<const void*>(0xc) = cpmAt<const void*>(at<const NxU8*>(4), 8);
	return this;
	}

// phys_fn_000895 (0x0001f010, 779 B)
// The patch for (shape0, shape1, material ids, world normal). thiscall,
// `ret 0x10`. The normal in body 0's frame; the first patch with the same
// ids and shapes whose body-0 normal (+0x34) has dot >= 0.996f with it is
// returned. Otherwise the embedded patch when there is none yet, else a new
// 0x78-byte patch (nxFoundationSDKAllocator, vptr only) pushed onto the
// vector (grown to 2 size + 2, copied, the old block freed). The new patch
// gets the count incremented, both body-local normals, the world normal, the
// ids and shapes, count/re-anchor bytes 0, normal force 0 for a spring patch
// (000871) or pair +0xd4 otherwise, and +0x58..+0x60 zeroed.
__declspec(noinline) NxFrictionPatch* NxActorPair::row000895(const NxU8* shape0, const NxU8* shape1,
	const NxReal* normal, NxU32 materialIds)
	{
	NxReal local0[3];
	const NxU8* body0 = at<const NxU8*>(8);
	if(body0)
		cpmDirectionToBody0895(body0, normal, local0);
	else
		cpmCopyWords(local0, normal, 3);

	const NxU32 existing = at<NxU32>(0x48);
	for(NxU32 i = 0; i < at<NxU32>(0x48); i++)
		{
		NxFrictionPatch* candidate = i == 0
			? reinterpret_cast<NxFrictionPatch*>(mBytes + 0x4c)
			: at<NxFrictionPatch**>(0xc4)[i - 1];
		if(candidate->at<NxU32>(0x68) == materialIds && candidate->at<const NxU8*>(0x6c) == shape0
			&& candidate->at<const NxU8*>(0x70) == shape1
			&& ((double)local0[2] * candidate->at<NxReal>(0x3c) + (double)local0[1] * candidate->at<NxReal>(0x38))
				+ (double)local0[0] * candidate->at<NxReal>(0x34) >= (double)0.996f)
			return candidate;
		}

	NxFrictionPatch* patch;
	if(existing == 0)
		patch = reinterpret_cast<NxFrictionPatch*>(mBytes + 0x4c);
	else
		{
		void* block = nxFoundationSDKAllocator->malloc(0x78, NX_MEMORY_PERSISTENT);
		patch = block ? new (block) NxFrictionPatch : 0;
		NxFrictionPatch** begin = at<NxFrictionPatch**>(0xc4);
		NxFrictionPatch** end = at<NxFrictionPatch**>(0xc8);
		NxFrictionPatch** capacity = at<NxFrictionPatch**>(0xcc);
		if(!(capacity > end))
			{
			const NxU32 wanted = (NxU32)(end - begin) * 2 + 2;
			const NxU32 have = begin ? (NxU32)(capacity - begin) : 0;
			if(have < wanted)
				{
				const NxU32 bytes = wanted * 4;
				NxFrictionPatch** grown = static_cast<NxFrictionPatch**>(
					nxFoundationSDKAllocator->malloc(bytes, NX_MEMORY_PERSISTENT));
				NxFrictionPatch** to = grown;
				for(NxFrictionPatch** from = at<NxFrictionPatch**>(0xc4); from != at<NxFrictionPatch**>(0xc8); from++)
					*to++ = *from;
				if(at<void*>(0xc4))
					nxFoundationSDKAllocator->free(at<void*>(0xc4));
				const NxU32 size = (NxU32)(at<NxFrictionPatch**>(0xc8) - at<NxFrictionPatch**>(0xc4));
				at<NxFrictionPatch**>(0xc4) = grown;
				at<NxU8*>(0xcc) = reinterpret_cast<NxU8*>(grown) + bytes;
				at<NxFrictionPatch**>(0xc8) = grown + size;
				}
			}
		at<NxFrictionPatch**>(0xc8) = at<NxFrictionPatch**>(0xc8) + 1;
		at<NxFrictionPatch**>(0xc8)[-1] = patch;
		}

	at<NxU32>(0x48) = at<NxU32>(0x48) + 1;
	cpmCopyWords(&patch->at<NxReal>(0x34), local0, 3);
	const NxU8* body1 = at<const NxU8*>(0xc);
	if(body1)
		cpmDirectionToBody0895(body1, normal, &patch->at<NxReal>(0x40));
	else
		cpmCopyWords(&patch->at<NxReal>(0x40), normal, 3);
	cpmCopyWords(&patch->at<NxReal>(0x4c), normal, 3);
	patch->at<NxU8>(0x74) = 0;
	patch->at<NxU32>(0x68) = materialIds;
	patch->at<const NxU8*>(0x6c) = shape0;
	patch->at<const NxU8*>(0x70) = shape1;
	patch->at<NxU8>(0x75) = 0;
	patch->at<NxReal>(0x64) = cpmPatchSpring0871(patch) ? 0.0f : at<NxReal>(0xd4);
	patch->at<NxU32>(0x60) = 0;
	patch->at<NxU32>(0x5c) = 0;
	patch->at<NxU32>(0x58) = 0;
	return patch;
	}

// The anchor selection 000897 runs per contact while collecting
// (0x1001f950..0x1001fa4c): the first point, then the second with their
// squared distance, then whichever of the two a farther point replaces.
struct CpmAnchors
	{
	const NxReal*	point[2];	//!< frame +0x30/+0x34, handed to 000865
	NxReal			distance2;	//!< frame +0x38
	};

// phys_fn_000897 (0x0001f320, 554 B)
// phys_fn_000899 (0x0001f550, 1356 B)
// ActorPair::generateConstraints. thiscall, `ret 0xc`. Nothing when either
// object's +0x14 has bit 1. 000891, then the contact stream (+0x40): per pair
// header (objects, then the word material << 24 | flags << 16 | normal
// count), skipping headers with flag 2; a pair with a missing body whose
// other body's +0x240 exceeds the header's material gets flag 2 set in the
// stream and its contacts skipped. Otherwise the pair's restitution (000857
// on (shape B's material << 16 | shape A's)); per normal block a new-normal
// mark when it is not >= 0.996f along the previous; per contact the 32-bit
// feature word (flag 1) as per-triangle material ids with their own
// restitution, the anchors of the previous patch stored (000865) and the
// next patch found (000895) on a material change or new normal, then one
// record: 000883 for a spring material, else a kind-0 record with
// +0x34 = (-|separation| - NX_MIN_SEPARATION_FOR_PENALTY) * penaltyScale,
// 004391, and the restitution bounce (-(004389 * restitution) into +0x38
// when 004389 < NX_BOUNCE_TRESHOLD); then the anchor selection. After the
// stream, the last patch's anchors and 000879.
__declspec(noinline) void NxActorPair::row000897(NxSceneInternal* scene, NxReal forceScale,
	NxReal penaltyScale)
	{
	const NxReal* parameter = nxPhysicsSDKParameters();
	JointSupportBody* const support0 = cpmBodySupport(at<const NxU8*>(8));
	JointSupportBody* const support1 = cpmBodySupport(at<const NxU8*>(0xc));
	if((cpmAt<NxU8>(at<const NxU8*>(0), 0x14) & 2) || (cpmAt<NxU8>(at<const NxU8*>(4), 0x14) & 2))
		return;

	NxU8 storeAnchors = 0;		// frame +0x12
	NxU8 collectAnchors = 0;	// frame +0x11
	NxU8 newNormal = 0;			// frame +0x13
	NxFrictionPatch* patch = 0;	// frame +0x1c
	NxU32 anchorCount = 0;		// frame +0x18
	NxU32 materialKey = 0;		// frame +0x3c
	CpmAnchors anchors;
	anchors.point[0] = 0;
	anchors.point[1] = 0;
	anchors.distance2 = 0.0f;
	row000891(scene);

	const NxU32* s = at<const NxU32*>(0x40);
	NxU32 pairCount = 0;
	if(s)
		pairCount = *s++;
	while(pairCount != 0)
		{
		const NxU8* object1 = static_cast<const NxU8*>(cpmPointer(s[0]));
		const NxU8* object0 = static_cast<const NxU8*>(cpmPointer(s[1]));
		const NxU32 header = s[2];
		s += 3;
		NxU16 normalCount = (NxU16)header;
		const NxU16 high = (NxU16)(header >> 16);
		const NxU8 flags = (NxU8)high;
		pairCount--;
		if(high & 2)
			continue;

		const NxU8* body0 = at<const NxU8*>(8);
		if(!body0 || !at<const NxU8*>(0xc))
			{
			const NxU32 material = (NxU32)high >> 8;
			const NxU8* body = body0 ? body0 : at<const NxU8*>(0xc);
			if(cpmAt<NxU32>(body, 0x240) > material)
				{
				const_cast<NxU32*>(s)[-1] |= 0x20000;
				while(normalCount--)
					{
					NxU32 contacts = s[3];
					s += 4;
					while(contacts--)
						{
						const NxU32 wide = s[3] & 0x80000000;
						s += 4;
						if(flags & 1)
							s += 1;
						if(flags & 4)
							s += wide ? 2 : 1;
						}
					}
				continue;
				}
			}

		const NxU8* shapeA = cpmAt<const NxU8*>(object1, 8);
		const NxU8* shapeB = cpmAt<const NxU8*>(object0, 8);
		NxU32 patchMaterials = ((NxU32)cpmAt<NxU16>(shapeB, 0xda) << 16) | cpmAt<NxU16>(shapeA, 0xda);
		CpmRestitution pairRestitution;
		CpmRestitution featureRestitution;
		cpmRestitution0857(patchMaterials, &pairRestitution);
		const CpmRestitution* restitution = &pairRestitution;
		const NxReal* previousNormal = 0;
		while(normalCount--)
			{
			const NxReal* normal = reinterpret_cast<const NxReal*>(s);
			const NxU32 contactCount = s[3];
			s += 4;
			if(!previousNormal
				|| ((double)normal[2] * previousNormal[2] + (double)normal[1] * previousNormal[1])
					+ (double)previousNormal[0] * normal[0] < (double)0.996f)
				{
				newNormal = 1;
				materialKey = 0xffffffff;
				}
			NxU32 remaining = contactCount;
			while(remaining--)
				{
				const NxReal* point = reinterpret_cast<const NxReal*>(s);
				const NxU32 separationWord = s[3];
				s += 4;
				const NxU32 separationBits = separationWord | 0x80000000;
				NxU32 feature = 0xffffffff;
				if(flags & 1)
					feature = *s++;
				if(flags & 4)
					s += (separationWord & 0x80000000) ? 2 : 1;

				bool changed = false;
				if(feature != 0xffffffff)
					{
					patchMaterials = feature;
					if(feature != materialKey)
						{
						materialKey = feature;
						cpmRestitution0857(feature, &featureRestitution);
						restitution = &featureRestitution;
						changed = true;
						}
					}
				else if(materialKey != 0xffffffff)
					{
					restitution = &pairRestitution;
					materialKey = 0xffffffff;
					changed = true;
					}
				if(changed || newNormal)
					{
					if(storeAnchors)
						row000865(patch, anchors.point, (NxU8)anchorCount);
					else if(collectAnchors && anchorCount == 2)
						row000865(patch, anchors.point, 2);
					patch = row000895(shapeA, shapeB, normal, patchMaterials);
					const NxU8 count = patch->at<NxU8>(0x74);
					storeAnchors = count == 0 ? 1 : 0;
					collectAnchors = (count == 0 || (count < 2 && contactCount >= 2)) ? 1 : 0;
					anchorCount = 0;
					newNormal = 0;
					}

				if(restitution->spring)
					row000883(scene, patch, separationBits, point, normal);
				else
					{
					NxReal r0[3];
					NxReal r1[3];
					cpmCopyWords(r0, point, 3);
					cpmCopyWords(r1, point, 3);
					const NxU8* b0 = at<const NxU8*>(8);
					if(b0)
						{
						const NxReal* p = cpmBodyPosition(b0);
						r0[0] = (NxReal)((double)r0[0] - p[0]);
						r0[1] = (NxReal)((double)r0[1] - p[1]);
						r0[2] = (NxReal)((double)r0[2] - p[2]);
						}
					const NxU8* b1 = at<const NxU8*>(0xc);
					if(b1)
						{
						const NxReal* p = cpmBodyPosition(b1);
						r1[0] = (NxReal)((double)r1[0] - p[0]);
						r1[1] = (NxReal)((double)r1[1] - p[1]);
						r1[2] = (NxReal)((double)r1[2] - p[2]);
						}
					const NxReal penalty = (NxReal)(((double)cpmFloat(separationBits)
						- parameter[NX_MIN_SEPARATION_FOR_PENALTY]) * penaltyScale);
					JointSupportRecord* record = cpmTakeRecord(scene);
					record->mBody[0] = support0;
					record->mBody[1] = support1;
					cpmCopyWords(&record->mUnknown000, normal, 3);
					{
					const double y = (double)r0[2] * normal[0] - (double)r0[0] * normal[2];
					const double z = (double)r0[0] * normal[1] - (double)r0[1] * normal[0];
					const double x = (double)r0[1] * normal[2] - (double)r0[2] * normal[1];
					record->mUnknown018.x = (NxReal)x;
					record->mUnknown018.y = (NxReal)y;
					record->mUnknown018.z = (NxReal)z;
					}
					{
					const double y = (double)r1[2] * normal[0] - (double)r1[0] * normal[2];
					const double z = (double)r1[0] * normal[1] - (double)r1[1] * normal[0];
					const double x = (double)r1[1] * normal[2] - (double)r1[2] * normal[1];
					record->mUnknown024.x = (NxReal)x;
					record->mUnknown024.y = (NxReal)y;
					record->mUnknown024.z = (NxReal)z;
					}
					cpmRecordKind(record, 0xffffffe0, 0);
					record->mUnknown038 = 0.0f;
					record->mUnknown044 = 0;
					record->mUnknown04c = 0;
					record->mUnknown030 = patch;
					record->mUnknown034 = penalty;
					record->mUnknown048 = FLT_MAX;
					cpmSolveRecord(record, parameter);
					if(restitution->restitution > 0.0f)
						{
						const double velocity = record->row004389();
						if(velocity < (double)parameter[NX_BOUNCE_TRESHOLD])
							{
							record->mUnknown034 = 0.0f;
							record->mUnknown038 = (NxReal)(-(velocity * restitution->restitution));
							}
						}
					}

				if(collectAnchors)
					{
					switch(anchorCount)
						{
						case 0:
							anchors.point[0] = point;
							anchorCount = 1;
							break;
						case 1:
							{
							const NxReal* a = anchors.point[0];
							anchors.point[1] = point;
							anchorCount = 2;
							const double dx = (double)a[0] - point[0];
							const double dy = (double)a[1] - point[1];
							const double dz = (double)a[2] - point[2];
							anchors.distance2 = (NxReal)((dy * dy + dz * dz) + dx * dx);
							break;
							}
						default:
							{
							const NxReal* a = anchors.point[0];
							const double dx = (double)point[0] - a[0];
							const double dy = (double)point[1] - a[1];
							const double dz = (double)point[2] - a[2];
							const double d = (dz * dz + dy * dy) + dx * dx;
							if(d > anchors.distance2)
								{
								anchors.distance2 = (NxReal)d;
								anchors.point[1] = point;
								break;
								}
							const NxReal* b = anchors.point[1];
							const double ex = (double)point[0] - b[0];
							const double ey = (double)point[1] - b[1];
							const double ez = (double)point[2] - b[2];
							const double e = (ez * ez + ey * ey) + ex * ex;
							if(e > anchors.distance2)
								{
								anchors.distance2 = (NxReal)e;
								anchors.point[0] = point;
								}
							break;
							}
						}
					}
				}
			previousNormal = normal;
			}
		}

	if(storeAnchors)
		row000865(patch, anchors.point, (NxU8)anchorCount);
	else if(collectAnchors && anchorCount == 2)
		row000865(patch, anchors.point, 2);
	row000879(scene, forceScale, penaltyScale);
	}

// phys_fn_000901 (0x0001faa0, 140 B)
// The node constructor. thiscall, `ret 0xc`, returns this. The pair (000893)
// on the two elements' +0x10; +0x104 = -1, +0x10 = the list, +0x100/+0/+4 =
// 0; linked at the tail when both elements' +0x08 are set, else at the head.
__declspec(noinline) NxPairNode* NxPairNode::row000901(NxU8* element0, NxU8* element1, NxPairList* list)
	{
	pair()->row000893(cpmAt<const NxU8*>(element0, 0x10), cpmAt<const NxU8*>(element1, 0x10));
	at<NxU32>(0x104) = 0xffffffff;
	at<NxPairList*>(0x10) = list;
	at<NxU32>(0x100) = 0;
	at<NxU32>(0) = 0;
	at<NxU32>(4) = 0;
	if(cpmAt<void*>(element0, 8) && cpmAt<void*>(element1, 8))
		{
		at<NxPairNode*>(0xc) = list->tail;
		if(list->tail)
			list->tail->at<NxPairNode*>(8) = this;
		if(!list->head)
			list->head = this;
		list->tail = this;
		at<NxPairNode*>(8) = 0;
		return this;
		}
	if(!list->tail)
		list->tail = this;
	at<NxPairNode*>(8) = list->head;
	if(list->head)
		list->head->at<NxPairNode*>(0xc) = this;
	list->head = this;
	at<NxPairNode*>(0xc) = 0;
	return this;
	}

// phys_fn_000903 (0x0001fb30, 120 B)
// Unlink the node (+0x08 next, +0x0c prev, the list's head/tail), clear its
// links, then the tail jump to 000887 on its pair. fastcall.
__declspec(noinline) void __fastcall cpmPairNodeUnlink0903(NxPairNode* node)
	{
	NxPairNode* const prev = node->at<NxPairNode*>(0xc);
	if(!prev)
		{
		NxPairList* list = node->at<NxPairList*>(0x10);
		if(list->tail == node)
			list->tail = 0;
		node->at<NxPairList*>(0x10)->head = node->at<NxPairNode*>(8);
		if(node->at<NxPairNode*>(8))
			node->at<NxPairNode*>(8)->at<NxPairNode*>(0xc) = 0;
		}
	else if(!node->at<NxPairNode*>(8))
		{
		NxPairList* list = node->at<NxPairList*>(0x10);
		if(list->tail == node)
			list->tail = prev;
		node->at<NxPairNode*>(0xc)->at<NxPairNode*>(8) = 0;
		}
	else
		{
		prev->at<NxPairNode*>(8) = node->at<NxPairNode*>(8);
		node->at<NxPairNode*>(8)->at<NxPairNode*>(0xc) = node->at<NxPairNode*>(0xc);
		}
	node->at<NxPairNode*>(0xc) = 0;
	node->at<NxPairNode*>(8) = 0;
	cpmActorPairRelease0887(node->pair());
	}

// phys_fn_000905 (0x0001fbb0, 491 B)
// The per-step refresh of one node. thiscall, `ret 4`. Nothing for a pair a
// joint disables (000750), a pair without a dynamic body (+0x10c bit 7
// clear), or one with an object whose +0x14 bit 0 is set; nothing either when
// the Scene hash (+0x2c, 004153 on the two shape ids +0xd4) holds a flag
// entry (bit 0). With reports on (Scene+0x6b4) the pair's report record gets
// the frame (a new 0x14-byte record inserted through 004155 when there is
// none). When either side's `+0x10->+0x08` changed: 000863 and 002348 (the
// narrow-phase re-registration), and the node's cache refreshed. Last, the
// record's bit 31 = (the pair's contact count != 0) and its node/objects.
__declspec(noinline) void NxPairNode::row000905(NxSceneInternal* scene)
	{
	const NxU32 frame = scene->at<NxU32>(0x540);
	const NxU32 reporting = scene->at<NxU32>(0x6b4);
	NxU8* shape1 = cpmAt<NxU8*>(at<NxU8*>(0x18), 0x10);
	NxU8* shape0 = cpmAt<NxU8*>(at<NxU8*>(0x14), 0x10);
	const NxU8* owner0 = cpmAt<const NxU8*>(shape0, 4);
	const NxU8* owner1 = cpmAt<const NxU8*>(shape1, 4);
	const CpmJointedBody* body0 = cpmAt<const CpmJointedBody*>(owner0, 8);
	const CpmJointedBody* body1 = cpmAt<const CpmJointedBody*>(owner1, 8);
	NxU8 dynamic0 = 0;
	if(body0)
		{
		if(body1 && body0->row000750(body1))
			return;
		dynamic0 = (cpmAt<NxU8>(body0, 0x10c) & 0x80) ? 0 : 1;
		}
	const NxU8 dynamic1 = (body1 && !(cpmAt<NxU8>(body1, 0x10c) & 0x80)) ? 1 : 0;
	if(!dynamic0 && !dynamic1)
		return;
	if((cpmAt<NxU8>(owner0, 0x14) & 1) || (cpmAt<NxU8>(owner1, 0x14) & 1))
		return;

	CpmPairHash* hash = reinterpret_cast<CpmPairHash*>(scene->bytes() + 0x2c);
	CpmPairHashEntry* entry = cpmOpen004153(hash, cpmAt<NxU32>(shape0, 0xd4), cpmAt<NxU32>(shape1, 0xd4));
	if(entry && (entry->value & 1))
		return;
	CpmPairHashEntry* recordEntry = 0;
	if(reporting)
		{
		if(entry)
			{
			static_cast<NxU32*>(cpmPointer(entry->value))[1] = frame;
			recordEntry = entry;
			}
		else
			{
			NxU32* record = static_cast<NxU32*>(nxFoundationSDKAllocator->malloc(0x14, NX_MEMORY_PERSISTENT));
			record[1] = frame;
			record[0] = 0;
			recordEntry = cpmOpen004155(hash, cpmAt<NxU32>(shape0, 0xd4), cpmAt<NxU32>(shape1, 0xd4), record);
			}
		}

	if(cpmAt<void*>(cpmAt<NxU8*>(at<NxU8*>(0x14), 0x10), 8) != at<void*>(0)
		|| cpmAt<void*>(cpmAt<NxU8*>(at<NxU8*>(0x18), 0x10), 8) != at<void*>(4))
		{
		cpmActorPairResetStream0863(pair());
		cpmOpen002348(cpmAt<NxU8*>(at<NxU8*>(0x14), 0x10), cpmAt<NxU8*>(at<NxU8*>(0x18), 0x10), pair(), scene);
		at<void*>(0) = cpmAt<void*>(cpmAt<NxU8*>(at<NxU8*>(0x14), 0x10), 8);
		at<void*>(4) = cpmAt<void*>(cpmAt<NxU8*>(at<NxU8*>(0x18), 0x10), 8);
		}

	if(reporting && recordEntry)
		{
		NxU32* record = static_cast<NxU32*>(cpmPointer(recordEntry->value));
		NxU32 state = record[0];
		state = at<NxU32>(0x24) ? (state | 0x80000000) : (state & 0x7fffffff);
		record[0] = state;
		record[2] = (NxU32)(size_t)this;
		record[3] = at<NxU32>(0x14);
		record[4] = at<NxU32>(0x18);
		}
	}

// phys_fn_000909 (0x0001fdb0, 99 B)
// For each node whose first or second object's `+0x08` body has a non-zero
// +0x4c, 000905. thiscall on the list, `ret 4`.
__declspec(noinline) void NxPairList::row000909(NxSceneInternal* scene)
	{
	for(NxPairNode* node = head; node; node = node->at<NxPairNode*>(8))
		{
		const NxU8* body0 = cpmAt<const NxU8*>(node->at<NxU8*>(0x14), 8);
		if(body0 && cpmAt<NxU32>(body0, 0x4c) != 0)
			{
			node->row000905(scene);
			continue;
			}
		const NxU8* body1 = cpmAt<const NxU8*>(node->at<NxU8*>(0x18), 8);
		if(body1 && cpmAt<NxU32>(body1, 0x4c) != 0)
			node->row000905(scene);
		}
	}

// phys_fn_000911 (0x0001fe20, 143 B)
// A node for two broadphase elements, ordered by their shapes' +0xd4 id
// (unsigned); null when both collision groups (+0xd8, 0xffff = none) are set
// and the group mask ([0x10123a98]) disables the pair, or when the 0x108-byte
// allocation (nxFoundationSDKAllocator) fails. thiscall on the list, `ret 8`.
__declspec(noinline) NxPairNode* NxPairList::row000911(NxU8* element0, NxU8* element1)
	{
	NxU8* first = element0;
	NxU8* second = element1;
	if(cpmAt<NxU32>(cpmAt<NxU8*>(element0, 0x10), 0xd4) > cpmAt<NxU32>(cpmAt<NxU8*>(element1, 0x10), 0xd4))
		{
		first = element1;
		second = element0;
		}
	const NxU32 group0 = cpmAt<NxU16>(cpmAt<NxU8*>(first, 0x10), 0xd8);
	const NxU32 group1 = cpmAt<NxU16>(cpmAt<NxU8*>(second, 0x10), 0xd8);
	if(group0 != 0xffff && group1 != 0xffff
		&& !(nxPhysicsSDKGroupCollisionMasks()[group0] & (1u << (group1 & 31))))
		return 0;
	void* block = nxFoundationSDKAllocator->malloc(0x108, NX_MEMORY_PERSISTENT);
	if(!block)
		return 0;
	return static_cast<NxPairNode*>(block)->row000901(first, second, this);
	}

// The event logic 000913 and 000917 share (0x1001fed0..0x1001ffb9 and
// 0x10020080..0x1002016b): a stale record (frame != Scene+0x540) loses its
// node and bit 31, and is released when its bit 29 is clear; otherwise
// START|TOUCH (0xa) when it starts touching (bit 30 set), TOUCH (8) while it
// touches, END (4) when it stops (bit 30 cleared), masked by (state | the
// actor-group pair flags from 004153 on [0x10123c28]).
static NX_INLINE NxU32 cpmReportEvents(NxSceneInternal* scene, NxU32* record, NxU8& release)
	{
	const bool stale = record[1] != scene->at<NxU32>(0x540);
	release = 0;
	if(stale)
		{
		record[2] = 0;
		record[0] = record[0] & 0x7fffffff;
		}
	NxU32 state = record[0];
	NxU32 events = 0;
	if(!(state & 0x20000000) && stale)
		release = 1;
	else
		{
		bool lookup = true;
		if((NxI32)state < 0)
			{
			if(state & 0x40000000)
				events = 8;
			else
				{
				events = 0xa;
				record[0] = state | 0x40000000;
				}
			}
		else if(state & 0x40000000)
			{
			events = 4;
			record[0] = state & 0xbfffffff;
			}
		else
			lookup = false;
		if(lookup)
			{
			if(kCpmActorGroupPairFlags)
				{
				const CpmPairHashEntry* flags = cpmOpen004153(kCpmActorGroupPairFlags,
					cpmAt<NxU16>(cpmPointer(record[3]), 0x1c),
					cpmAt<NxU16>(cpmPointer(record[4]), 0x1c));
				if(flags)
					return (record[0] | flags->value) & events;
				}
			}
		}
	return record[0] & events;
	}

// phys_fn_000913 (0x0001feb0, 359 B)
// Fire the contact reports now. cdecl. For each hash entry holding a record
// (bit 0 clear): the events (cpmReportEvents); when any, NxContactPair
// {the objects' +0x00, the pair's stream (+0x40), 000881's sums} to the
// report's slot 0 (onContactNotify); a released record is freed
// (nxFoundationSDKAllocator) and erased (004157). With no node the sums are
// not written, as in the oracle.
__declspec(noinline) void __cdecl cpmFireContactReports0913(NxSceneInternal* scene,
	NxUserContactReport* report, CpmPairHash* hash)
	{
	NxU32 count = hash->count;
	CpmPairHashEntry* entry = hash->entries;
	if(!count)
		return;
	do
		{
		if(!(entry->value & 1))
			{
			NxU32* record = static_cast<NxU32*>(cpmPointer(entry->value));
			NxU8 release;
			const NxU32 events = cpmReportEvents(scene, record, release);
			if(events)
				{
				NxContactPair pair;
				pair.actors[0] = cpmAt<NxActor*>(cpmPointer(record[3]), 0);
				pair.actors[1] = cpmAt<NxActor*>(cpmPointer(record[4]), 0);
				NxPairNode* node = static_cast<NxPairNode*>(cpmPointer(record[2]));
				if(node)
					{
					pair.stream = node->at<NxConstContactStream>(0x54);
					node->pair()->row000881(&pair.sumNormalForce, &pair.sumFrictionForce);
					}
				else
					pair.stream = 0;
				report->onContactNotify(pair, events);
				}
			if(release)
				{
				nxFoundationSDKAllocator->free(record);
				cpmOpen004157(hash, entry->key0, entry->key1);
				}
			}
		entry++;
		}
	while(--count);
	}

// phys_fn_000915 (0x00020020, 33 B)
// Delete a node: 000903, then nxFoundationSDKAllocator->free. stdcall,
// `ret 4`; a null node is ignored.
__declspec(noinline) void __stdcall cpmDeletePairNode0915(NxPairNode* node)
	{
	if(node)
		{
		cpmPairNodeUnlink0903(node);
		nxFoundationSDKAllocator->free(node);
		}
	}

// phys_fn_000917 (0x00020050, 39 B)
// phys_fn_000919 (0x00020080, 477 B)
// phys_fn_000921 (0x00020260, 339 B)
// Buffer the contact reports. cdecl. As 000913, but each report is a 0x2c-byte
// record {objects' +0x00, stream, sums, the record, the events} appended to
// the Scene's vector (+0x60c begin, +0x610 end, +0x614 capacity), grown to
// 2 size + 2 records through nxFoundationSDKAllocator (copied, the old block
// freed). With no node the sums are left unwritten, as in the oracle.
__declspec(noinline) void __cdecl cpmBufferContactReports0917(NxSceneInternal* scene, CpmPairHash* hash)
	{
	NxU32 count = hash->count;
	CpmPairHashEntry* entry = hash->entries;
	if(!count)
		return;
	do
		{
		if(!(entry->value & 1))
			{
			NxU32* record = static_cast<NxU32*>(cpmPointer(entry->value));
			NxU8 release;
			const NxU32 events = cpmReportEvents(scene, record, release);
			if(events)
				{
				CpmBufferedContact contact;
				contact.actors[0] = cpmAt<void*>(cpmPointer(record[3]), 0);
				contact.actors[1] = cpmAt<void*>(cpmPointer(record[4]), 0);
				NxPairNode* node = static_cast<NxPairNode*>(cpmPointer(record[2]));
				if(node)
					{
					contact.stream = node->at<void*>(0x54);
					node->pair()->row000881(reinterpret_cast<NxVec3*>(contact.sumNormalForce),
						reinterpret_cast<NxVec3*>(contact.sumFrictionForce));
					}
				else
					contact.stream = 0;
				contact.record = record;
				contact.events = events;

				CpmBufferedContact* begin = scene->at<CpmBufferedContact*>(0x60c);
				CpmBufferedContact* end = scene->at<CpmBufferedContact*>(0x610);
				CpmBufferedContact* capacity = scene->at<CpmBufferedContact*>(0x614);
				if(!(capacity > end))
					{
					const NxU32 wanted = (NxU32)(end - begin) * 2 + 2;
					const NxU32 have = begin ? (NxU32)(capacity - begin) : 0;
					if(have < wanted)
						{
						const NxU32 bytes = wanted * 0x2c;
						CpmBufferedContact* grown = static_cast<CpmBufferedContact*>(
							nxFoundationSDKAllocator->malloc(bytes, NX_MEMORY_PERSISTENT));
						CpmBufferedContact* to = grown;
						for(CpmBufferedContact* from = scene->at<CpmBufferedContact*>(0x60c);
							from != scene->at<CpmBufferedContact*>(0x610); from++, to++)
							cpmCopyWords(to, from, 0x2c / 4);
						if(scene->at<void*>(0x60c))
							nxFoundationSDKAllocator->free(scene->at<void*>(0x60c));
						const NxU32 size = (NxU32)(scene->at<CpmBufferedContact*>(0x610)
							- scene->at<CpmBufferedContact*>(0x60c));
						scene->at<CpmBufferedContact*>(0x60c) = grown;
						scene->at<NxU8*>(0x614) = reinterpret_cast<NxU8*>(grown) + bytes;
						scene->at<CpmBufferedContact*>(0x610) = grown + size;
						}
					}
				cpmCopyWords(scene->at<CpmBufferedContact*>(0x610), &contact, 0x2c / 4);
				scene->at<CpmBufferedContact*>(0x610) = scene->at<CpmBufferedContact*>(0x610) + 1;
				}
			if(release)
				{
				nxFoundationSDKAllocator->free(record);
				cpmOpen004157(hash, entry->key0, entry->key1);
				}
			}
		entry++;
		}
	while(--count);
	}

// phys_fn_000589 (0x00010d50): replace a shape-pair flag record in the Scene
// hash. Ignore-pair is stored as the tagged flag word itself; report flags use
// a 0x14-byte state record so 000905/000917 can track contact transitions.
void cpmSetShapePairFlags(NxSceneInternal* scene, const NxU8* shape0,
	const NxU8* shape1, NxU32 flags)
	{
	CpmPairHash* hash = reinterpret_cast<CpmPairHash*>(scene->bytes() + 0x2c);
	const NxU32 id0 = cpmAt<NxU32>(shape0, 0xd4);
	const NxU32 id1 = cpmAt<NxU32>(shape1, 0xd4);
	CpmPairHashEntry* old = cpmOpen004153(hash, id0, id1);
	if(old)
		{
		if(!(old->value & 1) && old->value)
			nxFoundationSDKAllocator->free(cpmPointer(old->value));
		cpmOpen004157(hash, id0, id1);
		}
	if(!flags)
		return;

	NxU32 value = (flags & 0x1fffffffu) | 0x20000000u;
	if(!(flags & 1))
		{
		NxU32* record = static_cast<NxU32*>(
			nxFoundationSDKAllocator->malloc(0x14, NX_MEMORY_PERSISTENT));
		if(!record)
			return;
		record[0] = value;
		record[1] = scene->at<NxU32>(0x540);
		record[2] = record[3] = record[4] = 0;
		value = reinterpret_cast<NxU32>(record);
		}
	cpmOpen004155(hash, id0, id1, reinterpret_cast<void*>(value));
	}

// The fetch-results path delivers 000917's per-step buffer. Keep the event and
// stream values exactly as captured by the simulation worker and reset the
// vector only after the callbacks have returned.
void cpmDeliverBufferedContactReports(NxSceneInternal* scene, NxUserContactReport* report)
	{
	CpmBufferedContact* begin = scene->at<CpmBufferedContact*>(0x60c);
	CpmBufferedContact* end = scene->at<CpmBufferedContact*>(0x610);
	if(report && begin)
		for(CpmBufferedContact* item = begin; item && item != end; ++item)
			{
			NxContactPair pair;
			pair.actors[0] = static_cast<NxActor*>(item->actors[0]);
			pair.actors[1] = static_cast<NxActor*>(item->actors[1]);
			pair.stream = static_cast<NxConstContactStream>(item->stream);
			memcpy(&pair.sumNormalForce, item->sumNormalForce, sizeof(pair.sumNormalForce));
			memcpy(&pair.sumFrictionForce, item->sumFrictionForce, sizeof(pair.sumFrictionForce));
			report->onContactNotify(pair, item->events);
			}
	scene->at<CpmBufferedContact*>(0x610) = begin;
	}
