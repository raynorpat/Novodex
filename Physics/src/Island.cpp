/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The body record's island and sleep-group rows of the gap unit
// SceneRaycast.cpp..CapsuleShape.cpp (scene-raycast Task 4, island
// sub-area): 000708 000714 000716 000718 000720 000724 000728 000730 000762
// 000764. Declarations and the callers in Physics/src/include/BodyStep.h.
//
// The record has two union-find forests. The joint island: parent +0x1bc
// (000712 finds the root), rank +0x1c0, counts +0x1c4 / +0x1c8, member list
// next +0x1d0 / tail +0x1d4, joint lists +0x1d8 (link Joint+0x34) and +0x1dc
// (link Joint+0x38), island object +0x1e0, flag +0x1e4 bit 1 (island dirty).
// The sleep group: parent +0x1e8 (000713), rank +0x1ec, counts +0x1f0 /
// +0x1f4, float +0x1f8, member list next +0x1fc / tail +0x200, contact-pair
// list +0x208 (link pair+0x100); +0x25c counts the body's contact pairs.
//
// Every find in these rows is the listing's inlined first test: `mov ecx,
// [rec+parent]; cmp rec,ecx; je; call find; mov [rec+parent],eax`, i.e.
// the find row runs on the parent and its result is stored back; the root
// is then re-read from the record. The rows run inside the simulation step
// (control word 0x0f7f); the x87 parts here are compares, one float add and
// one float subtraction.

#include "BodyStep.h"
#include "ContactPairManager.h"
#include "core/JointSupport.h"
#include "core/Joint.h"
#include "ObjectModel.h"
#include "PhysicsInternal.h"

static NX_INLINE NxU8* islandBytes(void* record)
	{
	return static_cast<NxU8*>(record);
	}

static NX_INLINE NxU32& islandWord(void* record, NxU32 offset)
	{
	return *reinterpret_cast<NxU32*>(islandBytes(record) + offset);
	}

static NX_INLINE void*& islandPointer(void* record, NxU32 offset)
	{
	return *reinterpret_cast<void**>(islandBytes(record) + offset);
	}

static NX_INLINE NxReal& islandReal(void* record, NxU32 offset)
	{
	return *reinterpret_cast<NxReal*>(islandBytes(record) + offset);
	}

// .rdata 0x101053d4: the wake floor, 0.39999998f (0x3ecccccc); the raise
// stores the bit pattern (`mov [rec+0x4c],0x3ecccccc`), as 000760 does.
static const NxReal gIslandWakeFloor = 0.39999998f;

// .rdata 0x101041f0: 0.0f.
static const NxReal gIslandZero = 0.0f;

// The wake raise the island rows inline (000714 0x15d85-0x15da0 and
// 0x15e88-0x15ea3, 000762 0x177d1-0x177ec and 0x17856-0x17871): unless
// +0x114 bit 8 is set, `fld [rec+0x4c]; fcomp [0x101053d4]; test ah,5; jp`
// raises +0x4c to 0.4f only when it is ordered below the floor.
static NX_INLINE void islandRaiseWake(void* record)
	{
	if(!(islandWord(record, 0x114) & 0x100))
		{
		if(islandReal(record, 0x4c) < gIslandWakeFloor)
			islandWord(record, 0x4c) = 0x3ecccccc;
		}
	}

// The joint-island find as the listing inlines it around 000712.
static NX_INLINE void* islandCompress(void* record)
	{
	void* parent = islandPointer(record, 0x1bc);
	if(record != parent)
		islandPointer(record, 0x1bc) = static_cast<Row000712Fixture*>(parent)->row000712();
	return islandPointer(record, 0x1bc);
	}

// The sleep-group find as the listing inlines it around 000713 (the claimed
// 000713 is core/JointSupport.cpp's thiscall Row000713Fixture::row000713).
static NX_INLINE void* islandGroupCompress(void* record)
	{
	void* parent = islandPointer(record, 0x1e8);
	if(record != parent)
		islandPointer(record, 0x1e8) = static_cast<Row000713Fixture*>(parent)->row000713();
	return islandPointer(record, 0x1e8);
	}

// The island-object release 000762 (twice) and 000764 inline: 004167 on the
// object, then the Foundation allocator's free (`mov eax,[0x101041bc]; mov
// ecx,[eax]; call [edx+0x14]`), then +0x1e0 = 0.
static NX_INLINE void islandReleaseObject(void* root)
	{
	void* island = islandPointer(root, 0x1e0);
	if(island)
		{
		reinterpret_cast<Row004167Fixture*>(island)->row004167();
		nxFoundationSDKAllocator->free(island);
		islandPointer(root, 0x1e0) = 0;
		}
	}

// phys_fn_000708 (0x00015c20, 130 B)
// 000613's copy-back of the step's JointSupportBody (+0x204, see
// core/JointSupport.h): its +0x00 and +0x10 vectors to +0x34 and +0x40, then
// +0x1e4 |= 0x20 (saved velocities valid), then its +0x44 and +0x50 vectors
// to +0x1a0 and +0x1ac. Dword moves; the listing re-reads +0x204 before each
// group (0x15c20, 0x15c37, 0x15c59, 0x15c7d).
void Row000708Fixture::row000708()
	{
	const JointSupportBody* support = static_cast<const JointSupportBody*>(islandPointer(this, 0x204));
	const NxU32* from = reinterpret_cast<const NxU32*>(&support->mUnknown000);
	islandWord(this, 0x34) = from[0];
	islandWord(this, 0x38) = from[1];
	islandWord(this, 0x3c) = from[2];
	support = static_cast<const JointSupportBody*>(islandPointer(this, 0x204));
	from = reinterpret_cast<const NxU32*>(&support->mUnknown010);
	islandWord(this, 0x40) = from[0];
	islandWord(this, 0x44) = from[1];
	islandWord(this, 0x48) = from[2];
	islandWord(this, 0x1e4) |= 0x20;
	support = static_cast<const JointSupportBody*>(islandPointer(this, 0x204));
	from = reinterpret_cast<const NxU32*>(&support->mUnknown044);
	islandWord(this, 0x1a0) = from[0];
	islandWord(this, 0x1a4) = from[1];
	islandWord(this, 0x1a8) = from[2];
	support = static_cast<const JointSupportBody*>(islandPointer(this, 0x204));
	from = reinterpret_cast<const NxU32*>(&support->mUnknown050);
	islandWord(this, 0x1ac) = from[0];
	islandWord(this, 0x1b0) = from[1];
	islandWord(this, 0x1b4) = from[2];
	}

// phys_fn_000714 (0x00015d70, 328 B)
// G = the cached sleep-group parent +0x1e8 (read as it is, no find). When
// +0x1b8 differs from G's +0x1f4, the wake raise; then +0x1b8 = G+0x1f4.
// The velocities are the saved +0x1a0/+0x1ac when +0x1e4 bit 5 is set,
// else the live +0x34/+0x40. Each squared length is (x x + y y) + z z on
// the stack (0x15dce-0x15ddc, 0x15e20-0x15e2e); the linear one stays in a
// register, the angular one is spilled to a float (0x15e30). Both must be
// ordered below their thresholds (+0xd0, +0xd4; `test ah,5; jp`), else the
// wake raise (reloading +0x114). Below both: a +0x4c equal to 0 (`fucompp;
// test ah,0x44; jnp`) is left alone; otherwise +0x4c -= dt is stored
// (`fst`) and the unrounded difference, ordered below 0, stores 0.
// Finally +0x114 bit 8 is cleared.
void Row000714Fixture::row000714(NxReal dt)
	{
	void* group = islandPointer(this, 0x1e8);
	if(islandWord(this, 0x1b8) != islandWord(group, 0x1f4))
		islandRaiseWake(this);
	const NxU32 saved = islandWord(this, 0x1e4) & 0x20;
	islandWord(this, 0x1b8) = islandWord(group, 0x1f4);
	double linear;
	const NxReal* angular;
	if(saved)
		{
		const NxReal* v = &islandReal(this, 0x1a0);
		linear = ((double)v[0] * v[0] + (double)v[1] * v[1]) + (double)v[2] * v[2];
		angular = &islandReal(this, 0x1ac);
		}
	else
		{
		const NxReal* v = &islandReal(this, 0x34);
		linear = ((double)v[0] * v[0] + (double)v[1] * v[1]) + (double)v[2] * v[2];
		angular = &islandReal(this, 0x40);
		}
	const NxReal angular2 = (NxReal)(((double)angular[0] * angular[0] + (double)angular[1] * angular[1])
		+ (double)angular[2] * angular[2]);
	if(linear < islandReal(this, 0xd0) && angular2 < islandReal(this, 0xd4))
		{
		if(!(islandReal(this, 0x4c) == gIslandZero))
			{
			const double wake = (double)islandReal(this, 0x4c) - dt;
			islandReal(this, 0x4c) = (NxReal)wake;
			if(wake < gIslandZero)
				islandWord(this, 0x4c) = 0;
			}
		}
	else
		{
		islandRaiseWake(this);
		}
	islandWord(this, 0x114) &= ~0x100u;
	}

// phys_fn_000716 (0x00015ec0, 277 B)
// Finds `other`'s root, then this record's (000712 on each parent). Equal
// roots: nothing. A root whose rank (+0x1c0) is strictly greater (unsigned,
// `jbe`) absorbs the other: the absorbed root's parent becomes the
// survivor's parent (the survivor itself), the member lists are spliced
// (survivor tail's +0x1d0 = absorbed; survivor tail = absorbed tail), +0x1c4
// and +0x1c8 are summed into the survivor, which gets +0x1e4 bit 1 while the
// absorbed root loses it. Otherwise `other`'s root survives and its rank is
// incremented unconditionally (0x15f85; the listing does not test for a
// tie).
__declspec(noinline) void Row000716Fixture::row000716(void* other)
	{
	islandCompress(other);
	void* otherRoot = islandPointer(other, 0x1bc);
	islandCompress(this);
	void* root = islandPointer(this, 0x1bc);
	if(otherRoot == root)
		return;
	if(islandWord(root, 0x1c0) > islandWord(otherRoot, 0x1c0))
		{
		islandPointer(otherRoot, 0x1bc) = islandPointer(root, 0x1bc);
		islandPointer(islandPointer(root, 0x1d4), 0x1d0) = otherRoot;
		islandPointer(root, 0x1d4) = islandPointer(otherRoot, 0x1d4);
		islandWord(root, 0x1c4) += islandWord(otherRoot, 0x1c4);
		islandWord(root, 0x1c8) += islandWord(otherRoot, 0x1c8);
		islandWord(root, 0x1e4) |= 2;
		islandWord(otherRoot, 0x1e4) &= ~2u;
		return;
		}
	islandPointer(root, 0x1bc) = islandPointer(otherRoot, 0x1bc);
	++islandWord(otherRoot, 0x1c0);
	islandPointer(islandPointer(otherRoot, 0x1d4), 0x1d0) = root;
	islandPointer(otherRoot, 0x1d4) = islandPointer(root, 0x1d4);
	islandWord(otherRoot, 0x1c4) += islandWord(root, 0x1c4);
	islandWord(otherRoot, 0x1c8) += islandWord(root, 0x1c8);
	islandWord(otherRoot, 0x1e4) |= 2;
	islandWord(root, 0x1e4) &= ~2u;
	}

// phys_fn_000718 (0x00015fe0, 285 B)
// 000716's shape on the sleep-group fields (000713 on each parent; rank
// +0x1ec; list next +0x1fc / tail +0x200), summing the words +0x1f0 and
// +0x1f4 and the float +0x1f8 (`fld absorbed; fadd survivor; fstp`), with
// no +0x1e4 flag. The else arm again increments `other`'s root's rank
// unconditionally (0x160a1).
__declspec(noinline) void Row000718Fixture::row000718(void* other)
	{
	islandGroupCompress(other);
	void* otherRoot = islandPointer(other, 0x1e8);
	islandGroupCompress(this);
	void* root = islandPointer(this, 0x1e8);
	if(otherRoot == root)
		return;
	if(islandWord(root, 0x1ec) > islandWord(otherRoot, 0x1ec))
		{
		islandPointer(otherRoot, 0x1e8) = islandPointer(root, 0x1e8);
		islandPointer(islandPointer(root, 0x200), 0x1fc) = otherRoot;
		islandPointer(root, 0x200) = islandPointer(otherRoot, 0x200);
		islandWord(root, 0x1f0) += islandWord(otherRoot, 0x1f0);
		islandWord(root, 0x1f4) += islandWord(otherRoot, 0x1f4);
		islandReal(root, 0x1f8) = (NxReal)((double)islandReal(otherRoot, 0x1f8) + islandReal(root, 0x1f8));
		return;
		}
	islandPointer(root, 0x1e8) = islandPointer(otherRoot, 0x1e8);
	++islandWord(otherRoot, 0x1ec);
	islandPointer(islandPointer(otherRoot, 0x200), 0x1fc) = root;
	islandPointer(otherRoot, 0x200) = islandPointer(root, 0x200);
	islandWord(otherRoot, 0x1f0) += islandWord(root, 0x1f0);
	islandWord(otherRoot, 0x1f4) += islandWord(root, 0x1f4);
	islandReal(otherRoot, 0x1f8) = (NxReal)((double)islandReal(root, 0x1f8) + islandReal(otherRoot, 0x1f8));
	}

// phys_fn_000720 (0x00016100, 46 B)
// Root via 000712; when its +0x1e4 byte has bit 1, 004172 on it (cdecl,
// `push edx; call; pop ecx`).
void Row000720Fixture::row000720()
	{
	void* root = islandCompress(this);
	if(*(islandBytes(root) + 0x1e4) & 2)
		nxBodyIslandRebuild004172Open(root);
	}

// phys_fn_000724 (0x000161b0, 175 B)
// ++this+0x25c and, with `other`, ++other+0x25c. This record's sleep-group
// root (000713 on the parent) gets ++root+0x1f0. With `other`: its root is
// found too, the pair is pushed (pair+0x100 = old head) onto the +0x208 list
// of `other` when other+0x11c is below this+0x11c (unsigned, `jb`), else of
// this record, and then 000718(other). Without: onto this record's list.
void Row000724Fixture::row000724(void* other, void* pair)
	{
	++islandWord(this, 0x25c);
	if(other)
		++islandWord(other, 0x25c);
	void* root = islandGroupCompress(this);
	++islandWord(root, 0x1f0);
	if(other)
		{
		islandGroupCompress(other);
		void* owner = islandWord(other, 0x11c) < islandWord(this, 0x11c) ? other : static_cast<void*>(this);
		islandPointer(pair, 0x100) = islandPointer(owner, 0x208);
		islandPointer(owner, 0x208) = pair;
		reinterpret_cast<Row000718Fixture*>(this)->row000718(other);
		return;
		}
	islandPointer(pair, 0x100) = islandPointer(this, 0x208);
	islandPointer(this, 0x208) = pair;
	}

// phys_fn_000728 (0x000167c0, 108 B)
// Every joint on the +0x1d8 list (link Joint+0x34): +0x160 = -1, +0x164 = 0,
// then the Joint base's slot-6 body 004133 called directly (`call
// 0x10099ab0`, not through the table) with dt. Then, with scene = the actor
// body's (+0x19c) +4 word, read before the list test (0x167fe), 000897 on
// every contact pair of the +0x208 list (link pair+0x100) with ecx = pair +
// 0x14 and (scene, dt, invDt).
__declspec(noinline) void Row000728Fixture::row000728(NxReal dt, NxReal invDt)
	{
	for(Joint* joint = static_cast<Joint*>(islandPointer(this, 0x1d8)); joint;
		joint = static_cast<Joint*>(islandPointer(joint, 0x34)))
		{
		islandWord(joint, 0x160) = 0xffffffff;
		islandWord(joint, 0x164) = 0;
		joint->Joint::row_slot6(dt);
		}
	void* pair = islandPointer(this, 0x208);
	void* scene = islandPointer(islandPointer(this, 0x19c), 4);
	for(; pair; pair = islandPointer(pair, 0x100))
		static_cast<NxPairNode*>(pair)->pair()->row000897(static_cast<NxSceneInternal*>(scene), dt, invDt);
	}

// phys_fn_000730 (0x00016830, 42 B)
// `test esi,esi` on this first: a null record does nothing. Otherwise 000728
// on it and on every record after it on the sleep-group chain (+0x1fc).
void Row000730Fixture::row000730(NxReal dt, NxReal invDt)
	{
	for(void* body = this; body; body = islandPointer(body, 0x1fc))
		static_cast<Row000728Fixture*>(body)->row000728(dt, invDt);
	}

// phys_fn_000762 (0x000177c0, 357 B)
// With joint+8 set, the wake raise on this record (esi, not the joint's
// body: the caller 000635 passes that body as this), and b1 = joint+0xc;
// else b1 = null. This record's root (000712 on the parent) releases its
// island object and gets ++root+0x1c4. With b1: the wake raise on this
// record again (0x17856 reads esi), b1's root (000712 on its parent)
// releases its island object; the joint goes on the +0x1d8 list (link
// Joint+0x34) of the record with the smaller +0x11c (b1 when b1's is below,
// unsigned `jae`) and on the other's +0x1dc list (link Joint+0x38; the
// listing tests that record for null, 0x178d7), then 000716(b1). Without b1:
// onto this record's +0x1d8 list, and root+0x1e4 |= 2.
void Row000762Fixture::row000762(void* joint)
	{
	void* other;
	if(islandPointer(joint, 0x08))
		{
		islandRaiseWake(this);
		other = islandPointer(joint, 0x0c);
		}
	else
		{
		other = 0;
		}
	islandCompress(this);
	void* root = islandPointer(this, 0x1bc);
	islandReleaseObject(root);
	++islandWord(root, 0x1c4);
	if(!other)
		{
		islandPointer(joint, 0x34) = islandPointer(this, 0x1d8);
		islandPointer(this, 0x1d8) = joint;
		islandWord(root, 0x1e4) |= 2;
		return;
		}
	islandRaiseWake(this);
	islandCompress(other);
	islandReleaseObject(islandPointer(other, 0x1bc));
	void* first;
	void* second;
	if(islandWord(other, 0x11c) < islandWord(this, 0x11c))
		{
		first = other;
		second = this;
		}
	else
		{
		second = other;
		first = this;
		}
	islandPointer(joint, 0x34) = islandPointer(first, 0x1d8);
	islandPointer(first, 0x1d8) = joint;
	if(second)
		{
		islandPointer(joint, 0x38) = islandPointer(second, 0x1dc);
		islandPointer(second, 0x1dc) = joint;
		}
	reinterpret_cast<Row000716Fixture*>(this)->row000716(other);
	}

// phys_fn_000764 (0x00017930, 100 B)
// Root via 000712; when its +0x1e4 byte has bit 1: its island object is
// released (004167, allocator free, +0x1e0 = 0) and 004172 runs on it
// (cdecl, `add esp,4`). Then a tail jump to 000722 on this record.
void Row000764Fixture::row000764()
	{
	void* root = islandCompress(this);
	if(*(islandBytes(root) + 0x1e4) & 2)
		{
		islandReleaseObject(root);
		nxBodyIslandRebuild004172Open(root);
		}
	reinterpret_cast<Row000722Fixture*>(this)->row000722();
	}

// ---------------------------------------------------------------------------
// OPEN callee stand-in, no stable-ID line: this is not the row. 004172
// (0x0009b0d0, 69 B, owner gap Joint.cpp..D6Joint.cpp) has no candidate
// anywhere; NX_ASSERT(0) is a silent no-op in Release, like the deferred
// Row004167Fixture::row004167. Nothing in the candidate calls 000720 or
// 000764 yet, so it never runs. Replace it with the owner's function when
// that lands.
__declspec(noinline) void __cdecl nxBodyIslandRebuild004172Open(void* /*root*/)
	{
	NX_ASSERT(0);
	}
