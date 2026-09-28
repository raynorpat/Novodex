/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The dynamic body record's constructor and destructor rows, written from the
// Capstone listing (oracle/capstone/manifest.json, authoritative) in the
// scene-raycast block's Task 4, sub-area body-creation. See
// include/BodyCreation.h and units/scene-raycast-contract.md
// (## Task 4 results: body-creation).
//
// Call graph, as in the image:
//   000026 (Scene.cpp nxActorComputeMass, not a row of this block)
//     -> 000797 DynamicBody::construct
//          -> 000801 DynamicBodyBase::construct -> 002421 (aux registration)
//          -> ??0Observable (import), vtable
//          -> 000760, 000722 (core/JointSupport.cpp Row000722Fixture, as on main)
//          -> 000793+000795 DynamicBody::loadFromBodyDesc
//               -> 000768, 000785, 000748
//   000030 (Scene.cpp releaseActor, not a row of this block)
//     -> 000776 DynamicBody::destruct -> 000028, 000713, 000722, 000760,
//          ??1Observable (import) -> 000799 DynamicBodyBase::destruct -> 002411
//
// Every row runs at API time under 0x027f (see the header), so this unit keeps
// the default architecture: register lifetimes are `double`, spills `NxReal`.
// Each row is `noinline`: the image calls every one as its own function (000801
// from 0x1b607, 000793 from 0x1b744, 000748 from 0x1b5a3; 000799 is 000776's
// tail jump), which the compiler would otherwise fold into its caller. The unit is built /EHs-c- (CMakeLists.txt):
// the image's rows are frameless, where /EHsc gave 000776 an unwind frame
// around the Observable's destructor.

#include "BodyCreation.h"
#include "PhysicsInternal.h"
#include "PhysicsSDK.h"
#include "NxBodyDesc.h"
#include "NpActorDynamicMath.h"
#include "core/JointSupport.h"
#include "FoundationSDK.h"

#include <float.h>
#include <new>

class NxSceneInternal;

// The emulations of the rows these call that are not rows of this block
// (Scene.cpp): 002421 and 002411, the Scene auxiliary manager's record
// registration keyed on the manager (the record's +0x120), and 000028's
// push of a recycled record id onto the Scene's +0x6fc vector.
void nxSceneAuxRegisterRecord(void* aux, void* record);
void nxSceneAuxUnregisterRecord(void* aux, void* record);
void nxSceneRecycleRecordId(NxSceneInternal* scene, unsigned id);
// 000713 (ObjectModel.cpp): the sleep-group root at +0x1e8, path-compressed.
unsigned nxBodyRecordFixRoot(void* rec);

namespace
	{

// The record's +0 object. The image constructs the Observable through the
// import ??0Observable@NxFoundation@@QAE@XZ ([0x10104190], 0x1b60e), then
// installs the body's own table, .rdata 0x10106890, whose one slot is the
// imported Observable::event (0x100af2c4 `jmp [0x10104198]`): a class that
// derives from the Observable and overrides nothing has exactly that table.
class DynamicBodyObservable : public NxFoundation::Observable
	{
	public:
	__forceinline DynamicBodyObservable() {}
	__forceinline ~DynamicBodyObservable() {}
	};

NX_INLINE NxU32& bodyWord(void* p, unsigned offset)
	{
	return *reinterpret_cast<NxU32*>(static_cast<NxU8*>(p) + offset);
	}

NX_INLINE NxReal& bodyReal(void* p, unsigned offset)
	{
	return *reinterpret_cast<NxReal*>(static_cast<NxU8*>(p) + offset);
	}

NX_INLINE void*& bodyPointer(void* p, unsigned offset)
	{
	return *reinterpret_cast<void**>(static_cast<NxU8*>(p) + offset);
	}

// The SDK parameter the image reads straight from its .data table (0x10123b18
// + 4 * parameter): 0x10123b20 NX_DEFAULT_SLEEP_LIN_VEL_SQUARED, 0x10123b24
// NX_DEFAULT_SLEEP_ANG_VEL_SQUARED, 0x10123b34 NX_MAX_ANGULAR_VELOCITY. The
// reconstruction's table is PhysicsSDK.cpp's file-scope gParameter, read here
// through getParameter, which returns the stored float unchanged.
NX_INLINE NxReal bodySdkParameter(NxParameter parameter)
	{
	const PhysicsSDK* const sdk = PhysicsSDK::instance;
	return sdk ? sdk->getParameter(parameter) : 0.0f;
	}

// The dirty mark 000793/000795 inline at every field (e.g. 0x1a373-0x1a455):
// the Scene auxiliary manager is the record's +0x120 word, its record table at
// +0x40 = {flags, +0x10 first, +0x14 last, +0x18 end, +0x20 index}. A record
// whose flags word is zero is queued (its index is the queue's size), growing
// the queue to 2 * size + 2 entries when it is full, then the mask is ORed in.
// Two recorded differences: the growth goes through nxGetSdkAllocator() where
// the image uses the Foundation allocator [0x101041bc] (0x1a3d9, 0x1a419): the
// queue is the manager's, allocated and freed in Scene.cpp through the SDK
// allocator, so it moves as one lifetime (Task 2's deferred list); and the
// `id < 256` test is not in the image: Scene.cpp's emulation of the manager
// (002421) keeps fixed 256-entry tables, and the test keeps a 257th record from
// writing past them. Below 256 records it never fails.
NX_INLINE void bodyMarkDirty(void* record, NxU32 mask)
	{
	NxU8* table = static_cast<NxU8*>(bodyPointer(record, 0x120)) + 0x40;
	const NxU32 id = bodyWord(record, 0x11c);
	if(id >= 256)
		return;
	NxU32* flags = *reinterpret_cast<NxU32**>(table);
	if(flags[id] == 0)
		{
		NxU32*& first = *reinterpret_cast<NxU32**>(table + 0x10);
		NxU32*& last = *reinterpret_cast<NxU32**>(table + 0x14);
		NxU32*& end = *reinterpret_cast<NxU32**>(table + 0x18);
		(*reinterpret_cast<NxU32**>(table + 0x20))[id] = NxU32(last - first);
		if(end <= last)
			{
			const NxU32 wanted = NxU32(last - first) * 2 + 2;
			const NxU32 capacity = first ? NxU32(end - first) : 0;
			if(capacity < wanted)
				{
				NxU32* grown = static_cast<NxU32*>(
					nxGetSdkAllocator()->malloc(wanted * sizeof(NxU32), NX_MEMORY_PERSISTENT));
				NxU32* to = grown;
				for(NxU32* from = first; from != last; ++from, ++to)
					*to = *from;
				const NxU32 count = NxU32(last - first);
				if(first)
					nxGetSdkAllocator()->free(first);
				last = grown + count;
				end = grown + wanted;
				first = grown;
				}
			}
		*last++ = id;
		}
	flags[id] |= mask;
	}

	}

// phys_fn_000801 (0x0001b7a0, 741 B)
// Thiscall on record + 0x18, `ret 0xc` (aux, pose, id). The listing's order:
// the identity 3x3 at +0xc4 (record +0xdc), the id at +0x104 and the manager
// at +0x108, the position (pose +0x24) at +0 and +0x38, the quaternion of the
// pose 3x3 at +0xc (0x1b82e-0x1b987: nxNpActorBodyQuaternionFromMatrix, the
// conversion 000768 and 000756 share, every root an X87Sqrt.h fsqrt) copied to
// +0x44, the zeroed +0x70..+0x100 block with the identity stored again, and last
// the manager's registration 002421 (0x1ba79), keyed on the base.
__declspec(noinline) DynamicBodyBase* DynamicBodyBase::construct(void* aux, const NxReal* pose, NxU32 id)
	{
	static const NxU32 one = 0x3f800000;
	bodyWord(this, 0xf0) = 0;
	bodyWord(this, 0xec) = 0;
	bodyWord(this, 0xe8) = 0;
	bodyWord(this, 0xc8) = 0;
	bodyWord(this, 0xcc) = 0;
	bodyWord(this, 0xd0) = 0;
	bodyWord(this, 0xd8) = 0;
	bodyWord(this, 0xdc) = 0;
	bodyWord(this, 0xe0) = 0;
	bodyWord(this, 0xc4) = one;
	bodyWord(this, 0xd4) = one;
	bodyWord(this, 0xe4) = one;
	bodyWord(this, 0x104) = id;
	bodyPointer(this, 0x108) = aux;
	const NxU32* poseWords = reinterpret_cast<const NxU32*>(pose);
	bodyWord(this, 0x00) = poseWords[9];
	bodyWord(this, 0x04) = poseWords[10];
	bodyWord(this, 0x08) = poseWords[11];
	bodyWord(this, 0x38) = poseWords[9];
	bodyWord(this, 0x3c) = poseWords[10];
	bodyWord(this, 0x40) = poseWords[11];
	nxNpActorBodyQuaternionFromMatrix(pose, &bodyReal(this, 0x0c));
	bodyWord(this, 0x44) = bodyWord(this, 0x0c);
	bodyWord(this, 0x48) = bodyWord(this, 0x10);
	bodyWord(this, 0x4c) = bodyWord(this, 0x14);
	bodyWord(this, 0x50) = bodyWord(this, 0x18);
	bodyWord(this, 0x78) = 0;
	bodyWord(this, 0x74) = 0;
	bodyWord(this, 0x70) = 0;
	bodyWord(this, 0x84) = 0;
	bodyWord(this, 0x80) = 0;
	bodyWord(this, 0x7c) = 0;
	bodyWord(this, 0x90) = 0;
	bodyWord(this, 0x8c) = 0;
	bodyWord(this, 0x88) = 0;
	bodyWord(this, 0x9c) = 0;
	bodyWord(this, 0x98) = 0;
	bodyWord(this, 0x94) = 0;
	bodyWord(this, 0x100) = 0;
	bodyWord(this, 0xfc) = 0;
	bodyWord(this, 0xf4) = 0;
	bodyWord(this, 0xf8) = 0;
	bodyWord(this, 0xa0) = 0;
	bodyWord(this, 0xa4) = 0;
	bodyWord(this, 0xa8) = 0;
	bodyWord(this, 0xb8) = 0;
	bodyWord(this, 0xbc) = 0;
	bodyWord(this, 0xc0) = 0;
	bodyWord(this, 0xb4) = 0;
	bodyWord(this, 0xb0) = 0;
	bodyWord(this, 0xac) = 0;
	bodyWord(this, 0xc4) = one;
	bodyWord(this, 0xc8) = 0;
	bodyWord(this, 0xcc) = 0;
	bodyWord(this, 0xd0) = 0;
	bodyWord(this, 0xd4) = one;
	bodyWord(this, 0xd8) = 0;
	bodyWord(this, 0xdc) = 0;
	bodyWord(this, 0xe0) = 0;
	bodyWord(this, 0xe4) = one;
	bodyWord(this, 0xf0) = 0;
	bodyWord(this, 0xec) = 0;
	bodyWord(this, 0xe8) = 0;
	nxSceneAuxRegisterRecord(bodyPointer(this, 0x108),
		static_cast<NxU8*>(static_cast<void*>(this)) - 0x18);
	return this;
	}

// phys_fn_000799 (0x0001b760, 51 B)
// Thiscall on record + 0x18, plain `ret` (000776 tail-jumps here at 0x185e0):
// the manager's unregistration 002411 ([+0x108], this), then the 0x20-byte
// kinematic block at +0x100 (record +0x118, 000785's) freed through the
// Foundation allocator [0x101041bc] (0x1b779-0x1b784), where 000785 allocates
// it, and nulled.
__declspec(noinline) void DynamicBodyBase::destruct()
	{
	nxSceneAuxUnregisterRecord(bodyPointer(this, 0x108),
		static_cast<NxU8*>(static_cast<void*>(this)) - 0x18);
	void* block = bodyPointer(this, 0x100);
	if(block)
		{
		nxFoundationSDKAllocator->free(block);
		bodyPointer(this, 0x100) = 0;
		}
	}

// phys_fn_000797 (0x0001b5c0, 402 B)
// Thiscall on the record, `ret 0xc` (owner, pose, desc); returns the record.
// The record id is inlined (0x1b5c4-0x1b5f5): the last entry of the Scene's
// recycled-id vector (+0x6fc first, +0x700 last) when it is not empty, else
// the counter at +0x6f8, post-incremented. Then 000801 on +0x18 with the
// Scene's auxiliary manager (+0x48), the Observable and the body's table, and
// the stores in the listing's order (0x1b61a-0x1b6f1): +0x134 identity with
// +0x158 zero, +0x20c identity with +0x230 zero, the empty bounds (+0x244
// FLT_MAX, +0x250 -FLT_MAX), the owner, +0x198 and +0x1a0..+0x1b4 zero,
// +0x1b8 = 1; 000760 and 000722 (0x1b6fb, 0x1b702); +0x1e4, +0x1e0 and +0x204
// zero; +0x134 copied to +0x20c (9 dwords) and +0x158 to +0x230; 000793.
__declspec(noinline) DynamicBody* DynamicBody::construct(void* owner, const NxReal* pose, const NxBodyDesc* desc)
	{
	NxU8* scene = static_cast<NxU8*>(bodyPointer(owner, 4));
	NxU8* ids = scene + 0x6f8;
	NxU32 id;
	const NxU32 recycled = NxU32(*reinterpret_cast<NxU32**>(ids + 8) - *reinterpret_cast<NxU32**>(ids + 4));
	if(recycled != 0)
		{
		id = (*reinterpret_cast<NxU32**>(ids + 4))[recycled - 1];
		*reinterpret_cast<NxU32**>(ids + 8) -= 1;
		}
	else
		id = (*reinterpret_cast<NxU32*>(ids))++;
	NxU8* record = static_cast<NxU8*>(static_cast<void*>(this));
	reinterpret_cast<DynamicBodyBase*>(record + 0x18)->construct(
		bodyPointer(scene, 0x48), pose, id);
	::new (record) DynamicBodyObservable;

	static const NxU32 one = 0x3f800000;
	bodyWord(this, 0x160) = 0;
	bodyWord(this, 0x15c) = 0;
	bodyWord(this, 0x158) = 0;
	bodyWord(this, 0x138) = 0;
	bodyWord(this, 0x13c) = 0;
	bodyWord(this, 0x140) = 0;
	bodyWord(this, 0x148) = 0;
	bodyWord(this, 0x14c) = 0;
	bodyWord(this, 0x150) = 0;
	bodyWord(this, 0x134) = one;
	bodyWord(this, 0x144) = one;
	bodyWord(this, 0x154) = one;
	bodyWord(this, 0x238) = 0;
	bodyWord(this, 0x234) = 0;
	bodyWord(this, 0x230) = 0;
	bodyWord(this, 0x20c) = one;
	bodyWord(this, 0x21c) = one;
	bodyWord(this, 0x22c) = one;
	bodyWord(this, 0x210) = 0;
	bodyWord(this, 0x214) = 0;
	bodyWord(this, 0x218) = 0;
	bodyWord(this, 0x220) = 0;
	bodyWord(this, 0x224) = 0;
	bodyWord(this, 0x228) = 0;
	bodyWord(this, 0x244) = 0x7f7fffff;
	bodyWord(this, 0x248) = 0x7f7fffff;
	bodyWord(this, 0x24c) = 0x7f7fffff;
	bodyWord(this, 0x250) = 0xff7fffff;
	bodyWord(this, 0x254) = 0xff7fffff;
	bodyWord(this, 0x258) = 0xff7fffff;
	bodyPointer(this, 0x19c) = owner;
	bodyWord(this, 0x198) = 0;
	bodyWord(this, 0x1a8) = 0;
	bodyWord(this, 0x1a4) = 0;
	bodyWord(this, 0x1a0) = 0;
	bodyWord(this, 0x1b4) = 0;
	bodyWord(this, 0x1b0) = 0;
	bodyWord(this, 0x1ac) = 0;
	bodyWord(this, 0x1b8) = 1;
	reinterpret_cast<Row000760Fixture*>(this)->row000760();
	reinterpret_cast<Row000722Fixture*>(this)->row000722();
	bodyWord(this, 0x1e4) = 0;
	bodyWord(this, 0x1e0) = 0;
	bodyWord(this, 0x204) = 0;
	for(unsigned i = 0; i < 9; ++i)
		bodyWord(this, 0x20c + 4 * i) = bodyWord(this, 0x134 + 4 * i);
	bodyWord(this, 0x230) = bodyWord(this, 0x158);
	bodyWord(this, 0x234) = bodyWord(this, 0x15c);
	bodyWord(this, 0x238) = bodyWord(this, 0x160);
	loadFromBodyDesc(desc);
	return this;
	}

// phys_fn_000793 (0x0001a350, 1613 B)
// phys_fn_000795 (0x0001a9a0, 3090 B)
// One function: the splitter cut it at 0x1a9a0 (0x1a99b `jmp 0x1a9a0` and
// 0x1a84d `je 0x1aa06` enter the tail, which has no callers of its own and
// carries the `ret 4` at 0x1b5af). Thiscall on the record. Every field is
// followed by the inlined dirty mark (bodyMarkDirty) with its own mask:
//   +0x188 = mass, +0xc0 = 1.0 / mass unconditionally (fdiv, 0x1a370)  0x10000
//   +0xb8 = linearDamping                                                0x800
//   +0xbc = angularDamping                                               0x1000
//   +0x100 = massLocalPose.t, ++(+0x198)                                 0x200
//   +0xdc = massLocalPose.M (9 dwords, rep movsd), ++(+0x198)            0x400
//   inertia: any nonzero word of massSpaceInertia (integer tests
//     0x1a83a-0x1a84d) stores it at +0x18c and the three inverses spilled to
//     float, kept at +0xc4 only when _fpclass (0xf4140, the CRT's) of each
//     spill has none of 0x207 (NaN or infinity), else zeros (0x1a853-0x1a910);
//     all zero stores (1, 1, 1) and inverses 1.0 through the same test on the
//     double 1.0 (0x1aa06-0x1aaa6)                                       0x20000
//   +0x6c = linearVelocity, copied to +0x34                              4
//   +0x78 = angularVelocity, copied to +0x40                             8
//   +0x84 = +0x4c = wakeUpCounter                                        0x10
//   +0x110 = solverIterationCount                                        0x40000
//   +0xd8 = maxAngularVelocity > 0 ? v * v : g * g, g the SDK's
//     NX_MAX_ANGULAR_VELOCITY (0x10123b34)                               0x8000
//   +0xd0 = sleepLinearVelocity > 0 ? v * v : the SDK's
//     NX_DEFAULT_SLEEP_LIN_VEL_SQUARED (0x10123b20)                      0x2000
//   +0xd4 = sleepAngularVelocity > 0 ? v * v : the SDK's
//     NX_DEFAULT_SLEEP_ANG_VEL_SQUARED (0x10123b24)                      0x4000
// then 000768 (0x1b497), 000785((flags >> 7) & 1) (0x1b4a8), +0x10c = flags
// (mask 0x80000) and 000748 (0x1b5a3). Each `> 0` is `fcomp 0.0; test ah,
// 0x41; jne default`: zero, negative and NaN take the default. The squares
// are a register product rounded once at the store.
__declspec(noinline) void DynamicBody::loadFromBodyDesc(const NxBodyDesc* desc)
	{
		{
		const double mass = desc->mass;
		bodyReal(this, 0x188) = NxReal(mass);
		bodyReal(this, 0xc0) = NxReal(1.0 / mass);
		}
	bodyMarkDirty(this, 0x10000);
	bodyReal(this, 0xb8) = desc->linearDamping;
	bodyMarkDirty(this, 0x800);
	bodyReal(this, 0xbc) = desc->angularDamping;
	bodyMarkDirty(this, 0x1000);
	const NxU32* descWords = reinterpret_cast<const NxU32*>(desc);
	bodyWord(this, 0x100) = descWords[9];
	bodyWord(this, 0x104) = descWords[10];
	bodyWord(this, 0x108) = descWords[11];
	bodyMarkDirty(this, 0x200);
	++bodyWord(this, 0x198);
	for(unsigned i = 0; i < 9; ++i)
		bodyWord(this, 0xdc + 4 * i) = descWords[i];
	bodyMarkDirty(this, 0x400);
	++bodyWord(this, 0x198);

	if(descWords[12] != 0 || descWords[13] != 0 || descWords[14] != 0)
		{
		bodyWord(this, 0x18c) = descWords[12];
		bodyWord(this, 0x190) = descWords[13];
		bodyWord(this, 0x194) = descWords[14];
		const NxReal inverseX = NxReal(1.0 / double(desc->massSpaceInertia.x));
		const NxReal inverseY = NxReal(1.0 / double(desc->massSpaceInertia.y));
		const NxReal inverseZ = NxReal(1.0 / double(desc->massSpaceInertia.z));
		if(!(_fpclass(inverseX) & 0x207) && !(_fpclass(inverseY) & 0x207) &&
			!(_fpclass(inverseZ) & 0x207))
			{
			bodyReal(this, 0xc4) = inverseX;
			bodyReal(this, 0xc8) = inverseY;
			bodyReal(this, 0xcc) = inverseZ;
			}
		else
			{
			bodyReal(this, 0xc4) = 0.0f;
			bodyReal(this, 0xc8) = 0.0f;
			bodyReal(this, 0xcc) = 0.0f;
			}
		}
	else
		{
		bodyReal(this, 0x18c) = 1.0f;
		bodyReal(this, 0x190) = 1.0f;
		bodyReal(this, 0x194) = 1.0f;
		if(!(_fpclass(1.0) & 0x207) && !(_fpclass(1.0) & 0x207) &&
			!(_fpclass(1.0) & 0x207))
			{
			bodyReal(this, 0xc4) = 1.0f;
			bodyReal(this, 0xc8) = 1.0f;
			bodyReal(this, 0xcc) = 1.0f;
			}
		else
			{
			bodyReal(this, 0xc4) = 0.0f;
			bodyReal(this, 0xc8) = 0.0f;
			bodyReal(this, 0xcc) = 0.0f;
			}
		}
	bodyMarkDirty(this, 0x20000);

	bodyWord(this, 0x6c) = descWords[16];
	bodyWord(this, 0x70) = descWords[17];
	bodyWord(this, 0x74) = descWords[18];
	bodyWord(this, 0x34) = descWords[16];
	bodyWord(this, 0x38) = bodyWord(this, 0x70);
	bodyWord(this, 0x3c) = bodyWord(this, 0x74);
	bodyMarkDirty(this, 4);
	bodyWord(this, 0x78) = descWords[19];
	bodyWord(this, 0x7c) = descWords[20];
	bodyWord(this, 0x80) = descWords[21];
	bodyWord(this, 0x40) = descWords[19];
	bodyWord(this, 0x44) = bodyWord(this, 0x7c);
	bodyWord(this, 0x48) = bodyWord(this, 0x80);
	bodyMarkDirty(this, 8);
	bodyReal(this, 0x84) = desc->wakeUpCounter;
	bodyReal(this, 0x4c) = desc->wakeUpCounter;
	bodyMarkDirty(this, 0x10);
	bodyWord(this, 0x110) = desc->solverIterationCount;
	bodyMarkDirty(this, 0x40000);

	if(desc->maxAngularVelocity > 0.0f)
		{
		const double v = desc->maxAngularVelocity;
		bodyReal(this, 0xd8) = NxReal(v * v);
		}
	else
		{
		const double g = bodySdkParameter(NX_MAX_ANGULAR_VELOCITY);
		bodyReal(this, 0xd8) = NxReal(g * g);
		}
	bodyMarkDirty(this, 0x8000);
	if(desc->sleepLinearVelocity > 0.0f)
		{
		const double v = desc->sleepLinearVelocity;
		bodyReal(this, 0xd0) = NxReal(v * v);
		}
	else
		bodyReal(this, 0xd0) = bodySdkParameter(NX_DEFAULT_SLEEP_LIN_VEL_SQUARED);
	bodyMarkDirty(this, 0x2000);
	if(desc->sleepAngularVelocity > 0.0f)
		{
		const double v = desc->sleepAngularVelocity;
		bodyReal(this, 0xd4) = NxReal(v * v);
		}
	else
		bodyReal(this, 0xd4) = bodySdkParameter(NX_DEFAULT_SLEEP_ANG_VEL_SQUARED);
	bodyMarkDirty(this, 0x4000);

	NxU8* record = static_cast<NxU8*>(static_cast<void*>(this));
	nxNpActorUpdateMassFrame(record);
	setKinematic((desc->flags >> 7) & 1);		// 0x1b49c-0x1b4a8
	bodyWord(this, 0x10c) = desc->flags;
	bodyMarkDirty(this, 0x80000);
	markIslandDirty();
	}

// phys_fn_000748 (0x00016f80, 47 B)
// The record in ecx, plain `ret`: the root refreshed through 000712 on the
// parent when the record is not its own root, reloaded, and its +0x1e4 ORed
// with 2 when its island object (+0x1e0) exists.
__declspec(noinline) void DynamicBody::markIslandDirty()
	{
	void* parent = bodyPointer(this, 0x1bc);
	if(this != parent)
		bodyPointer(this, 0x1bc) = static_cast<Row000712Fixture*>(parent)->row000712();
	void* root = bodyPointer(this, 0x1bc);
	if(bodyPointer(root, 0x1e0))
		bodyWord(root, 0x1e4) |= 2;
	}

// phys_fn_000776 (0x00018570, 117 B)
// Thiscall on the record, plain `ret` through its tail jump to 000799. The
// listing: the body table stored at +0 (0x1857f); the record id (+0x11c)
// pushed back onto the Scene's recycled-id vector by 000028 (0x1858f; the
// Scene is the owner's +4); the sleep-group root refreshed through 000713 on
// the parent when the record is not its own root (0x1859e); 000722 on every
// member of the root's +0x1fc chain, the next link read before each call; 000760
// and 000722 on the record; ??1Observable (import [0x10104194], 0x185d6);
// 000799 on +0x18. The table store is the class destructor's here, just before
// the Observable's destructor: the word already holds that table (installed by
// 000797 and never changed), so the moved store writes the same value.
__declspec(noinline) void DynamicBody::destruct()
	{
	NxU8* scene = static_cast<NxU8*>(bodyPointer(bodyPointer(this, 0x19c), 4));
	nxSceneRecycleRecordId(reinterpret_cast<NxSceneInternal*>(scene), bodyWord(this, 0x11c));
	void* parent = bodyPointer(this, 0x1e8);
	if(this != parent)
		bodyWord(this, 0x1e8) = nxBodyRecordFixRoot(parent);
	for(void* member = bodyPointer(this, 0x1e8); member; )
		{
		void* next = bodyPointer(member, 0x1fc);
		static_cast<Row000722Fixture*>(member)->row000722();
		member = next;
		}
	reinterpret_cast<Row000760Fixture*>(this)->row000760();
	reinterpret_cast<Row000722Fixture*>(this)->row000722();
	reinterpret_cast<DynamicBodyObservable*>(this)->~DynamicBodyObservable();
	reinterpret_cast<DynamicBodyBase*>(static_cast<NxU8*>(static_cast<void*>(this)) + 0x18)->destruct();
	}

// ---------------------------------------------------------------------------
// The setters (scene-raycast Task 4, sub-area setters). Each row runs at API
// time: the NxActor force, kinematic, body-flag and CMass setters (NpActor.cpp
// helpers forward to these members) and actor creation (000795 calls 000785),
// under 0x027f. The image's in-step callers of 000782 (003601, and 003979
// through 000791) are not reproduced. The dirty marks are bodyMarkDirty, the
// image's inline sequence (with the recorded allocator and `id < 256`
// differences above).

namespace
	{

// .rdata 0x101053d4 = 0x3ecccccc, one ulp below 0.4f, and the immediate stored.
static const NxReal gBodyWakeFloor = 0.39999998f;

// The wake block 000782 (0x1936f-0x19488) and 000784 (0x1950a-0x1961b) end
// with: +0x114 bit 8 clear and an ordered +0x84 below the floor (`fcomp
// [0x101053d4]; test ah,5; jp`) raise +0x84 and +0x4c to 0x3ecccccc, then
// mark 0x10.
NX_INLINE void bodyWake(void* record)
	{
	if(!(bodyWord(record, 0x114) & 0x100) && bodyReal(record, 0x84) < gBodyWakeFloor)
		{
		bodyWord(record, 0x84) = 0x3ecccccc;
		bodyWord(record, 0x4c) = 0x3ecccccc;
		bodyMarkDirty(record, 0x10);
		}
	}

// A row of the world inverse inertia tensor (+0x164, row-major) times the
// torque, as every arm of 000782 sums it: (I[r][2] tz + I[r][1] ty) + I[r][0] tx
// (e.g. 0x1888b-0x100188a7).
NX_INLINE double bodyInverseTensorRow(void* record, NxU32 row, const NxVec3* torque)
	{
	const NxU32 base = 0x164 + row * 12;
	return (static_cast<double>(bodyReal(record, base + 8)) * torque->z +
		static_cast<double>(bodyReal(record, base + 4)) * torque->y) +
		static_cast<double>(bodyReal(record, base)) * torque->x;
	}

// The linear arms of modes 0 and 3 (0x18759-0x1879b, 0x18ecf-0x18f11): the
// inverse mass in a register times the force; the x product stays in the
// register and is added to +target, the y and z products are spilled to
// float first.
NX_INLINE void bodyAddScaledForceSpillYZ(void* record, NxU32 target, const NxVec3* force)
	{
	const double inverseMass = bodyReal(record, 0xc0);
	const double x = inverseMass * force->x;
	const NxReal y = static_cast<NxReal>(inverseMass * force->y);
	const NxReal z = static_cast<NxReal>(inverseMass * force->z);
	bodyReal(record, target) = static_cast<NxReal>(x + bodyReal(record, target));
	bodyReal(record, target + 4) = static_cast<NxReal>(static_cast<double>(y) + bodyReal(record, target + 4));
	bodyReal(record, target + 8) = static_cast<NxReal>(static_cast<double>(z) + bodyReal(record, target + 8));
	}

// The angular arms of modes 0 and 3 (0x1888b-0x1890d, 0x1900b-0x1908d): x and
// y stay in registers, z is spilled to float.
NX_INLINE void bodyAddTorqueSpillZ(void* record, NxU32 target, const NxVec3* torque)
	{
	const double x = bodyInverseTensorRow(record, 0, torque);
	const double y = bodyInverseTensorRow(record, 1, torque);
	const NxReal z = static_cast<NxReal>(bodyInverseTensorRow(record, 2, torque));
	bodyReal(record, target) = static_cast<NxReal>(x + bodyReal(record, target));
	bodyReal(record, target + 4) = static_cast<NxReal>(y + bodyReal(record, target + 4));
	bodyReal(record, target + 8) = static_cast<NxReal>(static_cast<double>(z) + bodyReal(record, target + 8));
	}

// The velocity arms of modes 1 and 2 store the new velocity at `target` and
// its copy at `copy` (+0x6c -> +0x34, +0x78 -> +0x40): the x word from the
// register, y and z reloaded from the stores (e.g. 0x18a3c-0x18a55).
NX_INLINE void bodyStoreVelocity(void* record, NxU32 target, NxU32 copy,
	double x, double y, double z)
	{
	const NxReal storedX = static_cast<NxReal>(x);
	bodyReal(record, target) = storedX;
	bodyReal(record, target + 4) = static_cast<NxReal>(y);
	bodyReal(record, target + 8) = static_cast<NxReal>(z);
	bodyReal(record, copy) = storedX;
	bodyWord(record, copy + 4) = bodyWord(record, target + 4);
	bodyWord(record, copy + 8) = bodyWord(record, target + 8);
	}

// The accumulation of an unscaled vector (modes 2 and 4): each component is
// the float input plus the float field, summed in a register.
NX_INLINE double bodySum(NxReal a, NxReal b)
	{
	return static_cast<double>(a) + b;
	}

	}

// phys_fn_000782 (0x00018730, 3428 B)
// Thiscall on the record, `ret 0x10` (force, torque, mode, wake). `mode` above
// 4 (`cmp eax,4; ja 0x1936f`) skips every arm but not the wake; otherwise the
// jump table 0x10019494 (0x1874d, 0x189fd, 0x18c87, 0x18ec3, 0x19138):
//   0 NX_FORCE: force * inverse mass (+0xc0) into +0x88, mark 0x20; inverse
//     tensor * torque into +0x94, mark 0x40;
//   1 NX_IMPULSE: the same products (all three linear ones spilled; the
//     angular x kept in the register, y and z spilled) into the velocities
//     +0x6c/+0x78 and their copies +0x34/+0x40, marks 4 and 8;
//   2 NX_VELOCITY_CHANGE: the vectors unscaled into the same fields, marks 4, 8;
//   3 NX_SMOOTH_IMPULSE: as mode 0 into +0xa0/+0xac, marks 0x80 and 0x100;
//   4 NX_SMOOTH_VELOCITY_CHANGE: unscaled into +0xa0/+0xac, marks 0x80, 0x100.
// A null vector skips its half. Then, when the low byte of `wake` is set
// (0x1936f), the wake block.
__declspec(noinline) void DynamicBody::addForce(const NxVec3* force, const NxVec3* torque, NxU32 mode, bool wake)
	{
	switch(mode)
		{
		case 0:
			if(force)
				{
				bodyAddScaledForceSpillYZ(this, 0x88, force);
				bodyMarkDirty(this, 0x20);
				}
			if(torque)
				{
				bodyAddTorqueSpillZ(this, 0x94, torque);
				bodyMarkDirty(this, 0x40);
				}
			break;
		case 1:
			if(force)
				{
				// 0x18a09-0x18a55: every product spilled before the sums.
				const double inverseMass = bodyReal(this, 0xc0);
				const NxReal x = static_cast<NxReal>(inverseMass * force->x);
				const NxReal y = static_cast<NxReal>(inverseMass * force->y);
				const NxReal z = static_cast<NxReal>(inverseMass * force->z);
				bodyStoreVelocity(this, 0x6c, 0x34, bodySum(x, bodyReal(this, 0x6c)),
					bodySum(y, bodyReal(this, 0x70)), bodySum(z, bodyReal(this, 0x74)));
				bodyMarkDirty(this, 4);
				}
			if(torque)
				{
				// 0x18b44-0x18bd9: x stays in the register, y and z spilled.
				const double x = bodyInverseTensorRow(this, 0, torque);
				const NxReal y = static_cast<NxReal>(bodyInverseTensorRow(this, 1, torque));
				const NxReal z = static_cast<NxReal>(bodyInverseTensorRow(this, 2, torque));
				bodyStoreVelocity(this, 0x78, 0x40, x + bodyReal(this, 0x78),
					bodySum(y, bodyReal(this, 0x7c)), bodySum(z, bodyReal(this, 0x80)));
				bodyMarkDirty(this, 8);
				}
			break;
		case 2:
			if(force)
				{
				bodyStoreVelocity(this, 0x6c, 0x34, bodySum(force->x, bodyReal(this, 0x6c)),
					bodySum(bodyReal(this, 0x70), force->y), bodySum(bodyReal(this, 0x74), force->z));
				bodyMarkDirty(this, 4);
				}
			if(torque)
				{
				bodyStoreVelocity(this, 0x78, 0x40, bodySum(bodyReal(this, 0x78), torque->x),
					bodySum(bodyReal(this, 0x7c), torque->y), bodySum(bodyReal(this, 0x80), torque->z));
				bodyMarkDirty(this, 8);
				}
			break;
		case 3:
			if(force)
				{
				bodyAddScaledForceSpillYZ(this, 0xa0, force);
				bodyMarkDirty(this, 0x80);
				}
			if(torque)
				{
				bodyAddTorqueSpillZ(this, 0xac, torque);
				bodyMarkDirty(this, 0x100);
				}
			break;
		case 4:
			if(force)
				{
				bodyReal(this, 0xa0) = static_cast<NxReal>(bodySum(force->x, bodyReal(this, 0xa0)));
				bodyReal(this, 0xa4) = static_cast<NxReal>(bodySum(force->y, bodyReal(this, 0xa4)));
				bodyReal(this, 0xa8) = static_cast<NxReal>(bodySum(force->z, bodyReal(this, 0xa8)));
				bodyMarkDirty(this, 0x80);
				}
			if(torque)
				{
				bodyReal(this, 0xac) = static_cast<NxReal>(bodySum(torque->x, bodyReal(this, 0xac)));
				bodyReal(this, 0xb0) = static_cast<NxReal>(bodySum(torque->y, bodyReal(this, 0xb0)));
				bodyReal(this, 0xb4) = static_cast<NxReal>(bodySum(torque->z, bodyReal(this, 0xb4)));
				bodyMarkDirty(this, 0x100);
				}
			break;
		default:
			break;
		}
	if(wake)
		bodyWake(this);
	}

// phys_fn_000784 (0x000194b0, 368 B)
// Thiscall on the record, `ret 8` (position, orientation). The kinematic block
// at +0x118 (000785 allocates it) holds the target position at +0 and the
// quaternion at +0x10; +0xc ORs 1 for a position (0x194d5) and 2 for an
// orientation (0x19506), each written only when its pointer is non-null (the
// block pointer is not tested). Then the wake block (0x1950a-0x1961b), always.
__declspec(noinline) void DynamicBody::setKinematicTarget(const NxVec3* position, const NxQuat* orientation)
	{
	if(position)
		{
		const NxU32* words = reinterpret_cast<const NxU32*>(position);
		void* block = bodyPointer(this, 0x118);
		bodyWord(block, 0x0) = words[0];
		bodyWord(block, 0x4) = words[1];
		bodyWord(block, 0x8) = words[2];
		bodyWord(bodyPointer(this, 0x118), 0xc) |= 1;
		}
	if(orientation)
		{
		const NxU32* words = reinterpret_cast<const NxU32*>(orientation);
		void* block = bodyPointer(this, 0x118);
		bodyWord(block, 0x10) = words[0];
		bodyWord(block, 0x14) = words[1];
		bodyWord(block, 0x18) = words[2];
		bodyWord(block, 0x1c) = words[3];
		bodyWord(bodyPointer(this, 0x118), 0xc) |= 2;
		}
	bodyWake(this);
	}

// phys_fn_000785 (0x00019620, 1325 B)
// phys_fn_000787 (0x00019b50, 428 B)
// Thiscall on the record, `ret 4`; the argument is tested as a dword
// (`test eax,eax`, 0x1962c). 000787 is the tail of the leave path (the
// splitter cut it at the copy loop's `jmp 0x19b50`); it has no callers of its
// own. Each path first tests the kinematic bit (+0x10c bit 7, `test al,al;
// js/jns`) and returns when there is nothing to do, then runs the island step
// 000748 inlines (0x19643-0x1966f, 0x19992-0x199be: the +0x1bc root through
// 000712 on the parent, and the root's +0x1e4 |= 2 when it has an island
// object).
//   Enter: +0xc0 = 0, mark 0x10000; +0xc4..+0xcc = 0, mark 0x20000; +0x10c |=
//   0x80, mark 0x80000; the 0x20-byte block at +0x118 from the Foundation
//   allocator [0x101041bc] when it is null (0x1995d), and its +0xc = 0.
//   Leave: +0x10c &= ~0x80, mark 0x80000; +0xc0 = 1.0f / +0x188 (0x19abb, no
//   test), mark 0x10000; the three inverses 1.0f / +0x18c, +0x190, +0x194
//   formed before any store, z spilled (0x19bd4), stored x, z, y (000787), mark
//   0x20000; the block freed through [0x101041bc] (0x19cda) and nulled.
__declspec(noinline) void DynamicBody::setKinematic(NxU32 enable)
	{
	if(enable)
		{
		if(bodyWord(this, 0x10c) & 0x80)
			return;
		}
	else if(!(bodyWord(this, 0x10c) & 0x80))
		return;
	void* parent = bodyPointer(this, 0x1bc);
	if(this != parent)
		bodyPointer(this, 0x1bc) = static_cast<Row000712Fixture*>(parent)->row000712();
	void* root = bodyPointer(this, 0x1bc);
	if(bodyPointer(root, 0x1e0))
		bodyWord(root, 0x1e4) |= 2;
	if(enable)
		{
		bodyWord(this, 0xc0) = 0;
		bodyMarkDirty(this, 0x10000);
		bodyWord(this, 0xc4) = 0;
		bodyWord(this, 0xc8) = 0;
		bodyWord(this, 0xcc) = 0;
		bodyMarkDirty(this, 0x20000);
		bodyWord(this, 0x10c) |= 0x80;
		bodyMarkDirty(this, 0x80000);
		if(!bodyPointer(this, 0x118))
			bodyPointer(this, 0x118) = nxFoundationSDKAllocator->malloc(0x20, NX_MEMORY_PERSISTENT);
		bodyWord(bodyPointer(this, 0x118), 0xc) = 0;
		return;
		}
	bodyWord(this, 0x10c) &= ~0x80u;
	bodyMarkDirty(this, 0x80000);
	bodyReal(this, 0xc0) = static_cast<NxReal>(1.0 / bodyReal(this, 0x188));
	bodyMarkDirty(this, 0x10000);
	const double x = 1.0 / bodyReal(this, 0x18c);
	const double y = 1.0 / bodyReal(this, 0x190);
	const NxReal z = static_cast<NxReal>(1.0 / bodyReal(this, 0x194));
	bodyReal(this, 0xc4) = static_cast<NxReal>(x);
	bodyReal(this, 0xcc) = z;
	bodyReal(this, 0xc8) = static_cast<NxReal>(y);
	bodyMarkDirty(this, 0x20000);
	void* block = bodyPointer(this, 0x118);
	if(block)
		{
		nxFoundationSDKAllocator->free(block);
		bodyPointer(this, 0x118) = 0;
		}
	}

namespace
	{

// 000789's matrix-to-quaternion conversion (0x19fe1-0x1a1ce), the instruction
// sequence of the pose setters' (NpActorDynamicMath.h
// nxNpActorSetterQuaternionFromMatrix, 000196/000200) with every root an
// inline fsqrt, here through the X87Sqrt.h helpers in the listing's operand
// order: trace arm `fld m22; fadd m11; fadd m00; fadd 1.0f` (0x19fe1-0x1a010),
// z arm `fld m11; fadd m00; fsubr m22; fadd 1.0f` (0x1a093-0x1a0a5), y arm
// `fld m22; fadd m00; fsubr m11; fadd 1.0f` (0x1a0ec-0x1a0fe), x arm `fld m00;
// fsub float(m22 + m11); fadd 1.0f` (0x1a148-0x1a156). (m22 + m11) is spilled
// to float (0x19ff3) and the x arm reuses it; the z arm spills s and forms
// 0.5 / float(s) (0x1a0a7-0x1a0b7); the y and x arms spill the reciprocal
// 0.5 / s (0x1a114, 0x1a16c). The arm is chosen by strict greater-than
// (`fcomp; test ah,0x41`), a negative or unordered trace taking the arms.
NX_INLINE void bodyPoseQuaternionFromMatrix(const NxReal* m, NxReal* q)
	{
	const double zy = static_cast<double>(m[8]) + m[4];
	const NxReal zySpill = static_cast<NxReal>(zy);
	const double trace = zy + m[0];
	if(trace >= 0.0)
		{
		const double s = x87FsqrtSum4(m[8], m[4], m[0], 1.0f);
		q[3] = static_cast<NxReal>(0.5 * s);
		const double r = 0.5 / s;
		q[0] = static_cast<NxReal>((static_cast<double>(m[7]) - m[5]) * r);
		q[1] = static_cast<NxReal>((static_cast<double>(m[2]) - m[6]) * r);
		q[2] = static_cast<NxReal>((static_cast<double>(m[3]) - m[1]) * r);
		return;
		}
	NxU32 axis = m[4] > m[0] ? 1u : 0u;
	if(m[8] > m[axis * 4])
		axis = 2;
	if(axis == 2)
		{
		const double s = x87FsqrtDiag(m[8], m[4], m[0]);
		const NxReal sSpill = static_cast<NxReal>(s);
		q[2] = static_cast<NxReal>(s * 0.5);
		const double r = 0.5 / static_cast<double>(sSpill);
		q[0] = static_cast<NxReal>((static_cast<double>(m[6]) + m[2]) * r);
		q[1] = static_cast<NxReal>((static_cast<double>(m[7]) + m[5]) * r);
		q[3] = static_cast<NxReal>((static_cast<double>(m[3]) - m[1]) * r);
		}
	else if(axis == 1)
		{
		const double s = x87FsqrtDiag(m[4], m[8], m[0]);
		q[1] = static_cast<NxReal>(0.5 * s);
		const NxReal r = static_cast<NxReal>(0.5 / s);
		q[2] = static_cast<NxReal>((static_cast<double>(m[7]) + m[5]) * r);
		q[0] = static_cast<NxReal>((static_cast<double>(m[3]) + m[1]) * r);
		q[3] = static_cast<NxReal>((static_cast<double>(m[2]) - m[6]) * r);
		}
	else
		{
		const double s = x87FsqrtSum3(m[0], -zySpill, 1.0f);
		q[0] = static_cast<NxReal>(0.5 * s);
		const NxReal r = static_cast<NxReal>(0.5 / s);
		q[1] = static_cast<NxReal>((static_cast<double>(m[3]) + m[1]) * r);
		q[2] = static_cast<NxReal>((static_cast<double>(m[6]) + m[2]) * r);
		q[3] = static_cast<NxReal>((static_cast<double>(m[7]) - m[5]) * r);
		}
	}

	}

// phys_fn_000789 (0x00019d00, 1461 B)
// Thiscall on the record, no arguments, plain `ret`. The listing's order:
// 000746 (+0xc4, +0x134, +0x164) first (0x19d1e, the out-of-line
// nxNpActorWorldTensorRDRt); M = C F^T into a float local (C the world mass
// frame +0x134, F the local one +0xdc), each element summed in the listing's
// order (0x19d23-0x19e5a); the displacement M p (p = +0x100) with the x row
// kept in the register and the y and z rows spilled to float (0x19e5e-0x19ec8);
// t = +0x158 - M p to +0x50 and +0x18 (0x19ecc-0x19f15), mark 1; the
// quaternion of M to +0x5c and +0x24 (0x19fe1-0x1a1ce), mark 2 (the first mark's
// flag OR is interleaved with the trace sum at 0x19fee; it is the same store).
__declspec(noinline) void DynamicBody::setPoseFromCMass()
	{
	const NxReal* c = &bodyReal(this, 0x134);
	nxNpActorWorldTensorRDRt(&bodyReal(this, 0xc4), c, &bodyReal(this, 0x164));
	const NxReal* f = &bodyReal(this, 0xdc);
	#define NX_CF(a, b) (static_cast<double>(c[a]) * f[b])
	NxReal m[9];
	m[0] = static_cast<NxReal>((NX_CF(1, 1) + NX_CF(2, 2)) + NX_CF(0, 0));
	m[1] = static_cast<NxReal>((NX_CF(1, 4) + NX_CF(0, 3)) + NX_CF(2, 5));
	m[2] = static_cast<NxReal>((NX_CF(0, 6) + NX_CF(2, 8)) + NX_CF(1, 7));
	m[3] = static_cast<NxReal>((NX_CF(3, 0) + NX_CF(4, 1)) + NX_CF(5, 2));
	m[4] = static_cast<NxReal>((NX_CF(5, 5) + NX_CF(4, 4)) + NX_CF(3, 3));
	m[5] = static_cast<NxReal>((NX_CF(5, 8) + NX_CF(4, 7)) + NX_CF(3, 6));
	m[6] = static_cast<NxReal>((NX_CF(6, 0) + NX_CF(7, 1)) + NX_CF(8, 2));
	m[7] = static_cast<NxReal>((NX_CF(8, 5) + NX_CF(7, 4)) + NX_CF(6, 3));
	m[8] = static_cast<NxReal>((NX_CF(8, 8) + NX_CF(7, 7)) + NX_CF(6, 6));
	#undef NX_CF
	const NxReal* p = &bodyReal(this, 0x100);
	#define NX_MP(a, b) (static_cast<double>(m[a]) * p[b])
	const double dx = (NX_MP(2, 2) + NX_MP(1, 1)) + NX_MP(0, 0);
	const NxReal dy = static_cast<NxReal>((NX_MP(5, 2) + NX_MP(4, 1)) + NX_MP(3, 0));
	const NxReal dz = static_cast<NxReal>((NX_MP(8, 2) + NX_MP(7, 1)) + NX_MP(6, 0));
	#undef NX_MP
	const NxReal tx = static_cast<NxReal>(static_cast<double>(bodyReal(this, 0x158)) - dx);
	const NxReal ty = static_cast<NxReal>(static_cast<double>(bodyReal(this, 0x15c)) - dy);
	const NxReal tz = static_cast<NxReal>(static_cast<double>(bodyReal(this, 0x160)) - dz);
	bodyReal(this, 0x50) = tx;
	bodyReal(this, 0x54) = ty;
	bodyReal(this, 0x58) = tz;
	bodyReal(this, 0x18) = tx;
	bodyWord(this, 0x1c) = bodyWord(this, 0x54);
	bodyWord(this, 0x20) = bodyWord(this, 0x58);
	bodyMarkDirty(this, 1);
	NxReal q[4];
	bodyPoseQuaternionFromMatrix(m, q);
	bodyReal(this, 0x64) = q[2];
	bodyReal(this, 0x68) = q[3];
	bodyReal(this, 0x5c) = q[0];
	bodyReal(this, 0x60) = q[1];
	bodyReal(this, 0x24) = q[0];
	bodyWord(this, 0x28) = bodyWord(this, 0x60);
	bodyWord(this, 0x2c) = bodyWord(this, 0x64);
	bodyWord(this, 0x30) = bodyWord(this, 0x68);
	bodyMarkDirty(this, 2);
	}
