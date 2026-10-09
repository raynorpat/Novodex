// The spring-and-damper effector differential (effector-and-coredump Task 2).
//
// A staged-pair target: the same code drives the oracle pair and the
// candidate pair, and run_differential.ps1 compares the two transcripts. It
// reaches the effector through the public API -- NxScene's
// createSpringAndDamperEffector, releaseEffector, getNbEffectors and the
// effector iterator, every NxSpringAndDamperEffector method -- plus the
// internal effector's own table, called BY SLOT INDEX through the object the
// public wrapper keeps at +0x14 (the internal-slot method of
// NxPhysicsJointSlotTests). Nothing here names an oracle address.
//
// What is printed, per step:
//   * the SDK allocations and frees the step made (sizes, in order), from the
//     page-guarded allocator's history: the effector is 0x68 bytes and its
//     wrapper 0x18, both through the Foundation's allocator; the observer
//     arrays the Foundation's Observable grows are visible too;
//   * the internal effector's words (+0x14..+0x64) and its Observable part,
//     and each body record's Observable part (its table, observer count and
//     observers), with pointers printed as names: the effector, the wrapper,
//     the two records, the actor bodies, the Scene; a pointer into either
//     DLL's image is printed as the module's name;
//   * every getter's result as hex words.
//
// Cases: create over two dynamic actors; getters; setters; setBodies with the
// ends swapped; the effector's slots 2 and 3 called through its own table
// (each record's chain root +0x1e8 and its island words +0x1bc..+0x200 --
// the island 000760 builds and the copy 000722 makes of it, whose +0x1f8 is
// the island's wake counter -- are printed first; then the root's +0x1f8
// is set to 0 on both sides, so the slot's only callee that would apply a
// force, 000791, is not reached: its dependency 000782 is not written);
// release; a release/create cycle; an actor released while an
// effector holds its record (the record's 0x100 notify nulls the effector's
// pointer), then the effector released; and the scene released with a live
// effector (000575 after the actor loop).
//
// No effector here has a world end: the core dump's reader 003964 faults on
// one in the oracle, and Task 4 dumps scenes built the same way.

#include "PhysicsPairLoader.h"
#include "NxPageGuardedAllocator.h"

#include <math.h>
#include <string.h>

#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxActor.h"
#include "NxActorDesc.h"
#include "NxBodyDesc.h"
#include "NxBoxShapeDesc.h"
#include "NxSpringAndDamperEffector.h"
#include "NxSpringAndDamperEffectorDesc.h"

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32, NxUserAllocator*, NxUserOutputStream*);

typedef void (__thiscall *NxSlotVoid)(void* self);
typedef void (__thiscall *NxSlotApply)(void* self, void* body1, void* body2);
typedef void* (__thiscall *NxSlotSelf)(void* self);

static NxPageGuardedAllocator gAllocator;

static void* nxSlot(void* object, unsigned index)
	{
	return (*reinterpret_cast<void***>(object))[index];
	}

static NxU32 nxU(NxReal value)
	{
	NxU32 bits;
	memcpy(&bits, &value, 4);
	return bits;
	}

static NxU32 nxWordAt(const void* base, unsigned offset)
	{
	NxU32 word;
	memcpy(&word, static_cast<const unsigned char*>(base) + offset, 4);
	return word;
	}

static void* nxPointerAt(const void* base, unsigned offset)
	{
	return reinterpret_cast<void*>(nxWordAt(base, offset));
	}

// ---------------------------------------------------------------------------
// Pointer names.

struct NxSymbol
	{
	const void* pointer;
	char name[24];
	};

static NxSymbol gSymbols[32];
static unsigned gSymbolCount = 0;
static const unsigned char* gImageBase[2];
static unsigned gImageSize[2];
static const char* const gImageName[2] = { "physics", "foundation" };

static void nxSymbolAdd(const void* pointer, const char* name)
	{
	if(!pointer)
		return;
	for(unsigned i = 0; i < gSymbolCount; i++)
		if(gSymbols[i].pointer == pointer)
			{
			strncpy(gSymbols[i].name, name, sizeof(gSymbols[i].name) - 1);
			return;
			}
	if(gSymbolCount < 32)
		{
		gSymbols[gSymbolCount].pointer = pointer;
		strncpy(gSymbols[gSymbolCount].name, name, sizeof(gSymbols[gSymbolCount].name) - 1);
		gSymbols[gSymbolCount].name[sizeof(gSymbols[gSymbolCount].name) - 1] = 0;
		gSymbolCount++;
		}
	}

static void nxImageRange(HMODULE module, unsigned index)
	{
	const unsigned char* base = reinterpret_cast<const unsigned char*>(module);
	const IMAGE_DOS_HEADER* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
	const IMAGE_NT_HEADERS* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
	gImageBase[index] = base;
	gImageSize[index] = nt->OptionalHeader.SizeOfImage;
	}

static void nxPrintWord(NxU32 word, bool first)
	{
	if(!first)
		printf(".");
	for(unsigned i = 0; i < gSymbolCount; i++)
		if(word == reinterpret_cast<NxU32>(gSymbols[i].pointer))
			{
			printf("%s", gSymbols[i].name);
			return;
			}
	for(unsigned m = 0; m < 2; m++)
		if(gImageBase[m] && word >= reinterpret_cast<NxU32>(gImageBase[m])
			&& word < reinterpret_cast<NxU32>(gImageBase[m]) + gImageSize[m])
			{
			printf("%s", gImageName[m]);
			return;
			}
	printf("%08x", static_cast<unsigned>(word));
	}

// ---------------------------------------------------------------------------
// Allocation windows.

struct NxWindow
	{
	unsigned allocations;
	unsigned frees;
	};

static NxWindow nxWindowOpen()
	{
	NxWindow w = { gAllocator.allocations(), gAllocator.frees() };
	return w;
	}

static void nxWindowPrint(const char* label, const NxWindow& w)
	{
	const unsigned allocations = gAllocator.allocations() - w.allocations;
	const unsigned frees = gAllocator.frees() - w.frees;
	printf("effector %s allocs=%u sizes=", label, allocations);
	for(unsigned i = 0; i < allocations && i < NxPageGuardedAllocator::HISTORY; i++)
		printf("%s%x", i ? "," : "", gAllocator.allocSizeFromEnd(allocations - 1 - i));
	printf("%s frees=%u sizes=", allocations ? "" : "none", frees);
	for(unsigned i = 0; i < frees && i < NxPageGuardedAllocator::HISTORY; i++)
		printf("%s%x", i ? "," : "", gAllocator.freedSizeFromEnd(frees - 1 - i));
	printf("%s\n", frees ? "" : "none");
	}

// ---------------------------------------------------------------------------
// Object dumps.

// An NxFoundation::Observable at `p`: its table, then the observer array
// {first, last, memEnd} as a count, a capacity and the observers by name.
static void nxPrintObservable(const char* label, const char* what, const void* p)
	{
	const unsigned char* first = static_cast<const unsigned char*>(nxPointerAt(p, 4));
	const unsigned char* last = static_cast<const unsigned char*>(nxPointerAt(p, 8));
	const unsigned char* end = static_cast<const unsigned char*>(nxPointerAt(p, 0xc));
	printf("effector %s %s vt=", label, what);
	nxPrintWord(nxWordAt(p, 0), true);
	printf(" observers=%u capacity=%u list=", static_cast<unsigned>((last - first) / 4),
		static_cast<unsigned>((end - first) / 4));
	if(first == last)
		printf("none");
	for(const unsigned char* it = first; it < last; it += 4)
		nxPrintWord(nxWordAt(it, 0), it == first);
	printf("\n");
	}

static void nxPrintInternal(const char* label, const void* internal)
	{
	nxPrintObservable(label, "internal", internal);
	printf("effector %s internal words=", label);
	for(unsigned off = 0x18; off < 0x68; off += 4)
		nxPrintWord(nxWordAt(internal, off), off == 0x18);
	printf("\n");
	}

static void nxPrintRecord(const char* label, const char* name, const void* record)
	{
	if(!record)
		{
		printf("effector %s %s none\n", label, name);
		return;
		}
	nxPrintObservable(label, name, record);
	}

// ---------------------------------------------------------------------------
// The fixture.

static NxActor* nxCreateBody(NxScene& scene, const NxMat33& rotation, const NxVec3& position,
	const NxVec3& linear, const NxVec3& angular)
	{
	NxBoxShapeDesc box;
	box.dimensions = NxVec3(1.0f, 0.5f, 0.75f);
	NxBodyDesc body;
	body.linearVelocity = linear;
	body.angularVelocity = angular;
	NxActorDesc desc;
	desc.body = &body;
	desc.density = 2.0f;
	desc.shapes.pushBack(&box);
	desc.globalPose.M = rotation;
	desc.globalPose.t = position;
	return scene.createActor(desc);
	}

// Row-major matrix of the unit quaternion (x, y, z, w) / |q|.
static NxMat33 nxQuatMatrix(NxReal x, NxReal y, NxReal z, NxReal w)
	{
	const NxReal inv = 1.0f / sqrtf(x * x + y * y + z * z + w * w);
	x *= inv; y *= inv; z *= inv; w *= inv;
	return NxMat33(
		NxVec3(1.0f - 2.0f * (y * y + z * z), 2.0f * (x * y - z * w), 2.0f * (x * z + y * w)),
		NxVec3(2.0f * (x * y + z * w), 1.0f - 2.0f * (x * x + z * z), 2.0f * (y * z - x * w)),
		NxVec3(2.0f * (x * z - y * w), 2.0f * (y * z + x * w), 1.0f - 2.0f * (x * x + y * y)));
	}

static unsigned char* nxActorBody(NxActor* actor)
	{
	return actor ? static_cast<unsigned char*>(nxPointerAt(actor, 0x14)) : 0;
	}

static unsigned char* nxActorRecord(NxActor* actor)
	{
	unsigned char* body = nxActorBody(actor);
	return body ? static_cast<unsigned char*>(nxPointerAt(body, 8)) : 0;
	}

static void nxNameActor(NxActor* actor, const char* suffix)
	{
	char name[24];
	sprintf(name, "body_%s", suffix);
	nxSymbolAdd(nxActorBody(actor), name);
	sprintf(name, "rec_%s", suffix);
	nxSymbolAdd(nxActorRecord(actor), name);
	sprintf(name, "actor_%s", suffix);
	nxSymbolAdd(actor, name);
	}

static void nxPrintCount(const char* label, NxScene& scene)
	{
	printf("effector %s count=%u iterator=", label, static_cast<unsigned>(scene.getNbEffectors()));
	scene.resetEffectorIterator();
	unsigned n = 0;
	for(NxEffector* e = scene.getNextEffector(); e && n < 8; e = scene.getNextEffector(), n++)
		nxPrintWord(reinterpret_cast<NxU32>(e), n == 0);
	printf("%s\n", n ? "" : "none");
	}

static void nxPrintGetters(const char* label, NxSpringAndDamperEffector* e)
	{
	NxReal s[5] = { -1, -1, -1, -1, -1 };
	NxReal d[4] = { -1, -1, -1, -1 };
	e->getLinearSpring(s[0], s[1], s[2], s[3], s[4]);
	e->getLinearDamper(d[0], d[1], d[2], d[3]);
	printf("effector %s spring=%08x.%08x.%08x.%08x.%08x damper=%08x.%08x.%08x.%08x\n", label,
		nxU(s[0]), nxU(s[1]), nxU(s[2]), nxU(s[3]), nxU(s[4]), nxU(d[0]), nxU(d[1]), nxU(d[2]), nxU(d[3]));
	}

// isSpringAndDamperEffector (slot 0) and the two folded slots 6 and 7 of
// the wrapper's primary table, each expected to return the wrapper.
static void nxPrintIs(const char* label, NxSpringAndDamperEffector* e)
	{
	printf("effector %s is=", label);
	nxPrintWord(reinterpret_cast<NxU32>(e->isSpringAndDamperEffector()), true);
	printf(" slot6=");
	nxPrintWord(reinterpret_cast<NxU32>(reinterpret_cast<NxSlotSelf>(nxSlot(e, 6))(e)), true);
	printf(" slot7=");
	nxPrintWord(reinterpret_cast<NxU32>(reinterpret_cast<NxSlotSelf>(nxSlot(e, 7))(e)), true);
	printf(" userData_set=");
	e->userData = reinterpret_cast<void*>(0x5eed0001);
	printf("%08x\n", static_cast<unsigned>(nxWordAt(e, 4)));
	}

static void nxFillDesc(NxSpringAndDamperEffectorDesc& desc, NxActor* a, NxActor* b)
	{
	desc.body1 = a;
	desc.body2 = b;
	desc.pos1 = NxVec3(0.25f, 1.5f, -0.3f);
	desc.pos2 = NxVec3(2.6f, 0.2f, -0.85f);
	desc.springDistCompressSaturate = 0.5f;
	desc.springDistRelaxed = 1.25f;
	desc.springDistStretchSaturate = 3.0f;
	desc.springMaxCompressForce = 40.0f;
	desc.springMaxStretchForce = 55.0f;
	desc.damperVelCompressSaturate = -2.5f;
	desc.damperVelStretchSaturate = 1.75f;
	desc.damperMaxCompressForce = 7.0f;
	desc.damperMaxStretchForce = 9.5f;
	}

// Creates one effector over (a, b) and prints what the create did.
static NxSpringAndDamperEffector* nxCreate(const char* label, NxScene& scene, NxActor* a, NxActor* b)
	{
	NxSpringAndDamperEffectorDesc desc;
	nxFillDesc(desc, a, b);
	const NxWindow w = nxWindowOpen();
	NxSpringAndDamperEffector* e = scene.createSpringAndDamperEffector(desc);
	nxWindowPrint(label, w);
	printf("effector %s created=%s\n", label, e ? "yes" : "no");
	if(!e)
		return 0;
	void* internal = nxPointerAt(e, 0x14);
	nxSymbolAdd(e, "np");
	nxSymbolAdd(internal, "eff");
	printf("effector %s np words=", label);
	for(unsigned off = 0; off < 0x18; off += 4)
		nxPrintWord(off == 4 ? 0 : nxWordAt(e, off), off == 0);
	printf("\n");
	nxPrintInternal(label, internal);
	return e;
	}

// The hook base is the wrapper's secondary subobject (+8). Invoke its virtual
// deleting-destructor slot on a byte-for-byte scratch copy of a live wrapper,
// with deletion disabled. The destructor writes both vptrs but does not read
// the internal pointer, so the copy isolates the thunk's this-adjustment and
// leaves the scene-owned wrapper available for normal release.
static void nxProbeHookAdjustor(NxSpringAndDamperEffector* e)
	{
	unsigned char copy[0x18];
	memcpy(copy, e, sizeof(copy));
	void** hookTable = *reinterpret_cast<void***>(copy + 8);
	void* primaryBefore = reinterpret_cast<void*>(nxWordAt(copy, 0));
	void* hookBefore = reinterpret_cast<void*>(nxWordAt(copy, 8));
	void* readLinkBefore = reinterpret_cast<void*>(nxWordAt(copy, 0x10));
	void* internalBefore = reinterpret_cast<void*>(nxWordAt(copy, 0x14));
	typedef void* (__thiscall *DeletingDestructor)(void*, NxU32);
	reinterpret_cast<DeletingDestructor>(hookTable[0])(copy + 8, 0);
	printf("effector hook_dtor primary_same=%s hook_changed=%s read_link_same=%s internal_same=%s\n",
		nxPointerAt(copy, 0) == primaryBefore ? "yes" : "no",
		nxPointerAt(copy, 8) != hookBefore ? "yes" : "no",
		nxPointerAt(copy, 0x10) == readLinkBefore ? "yes" : "no",
		nxPointerAt(copy, 0x14) == internalBefore ? "yes" : "no");
	}

static void nxRelease(const char* label, NxScene& scene, NxSpringAndDamperEffector* e)
	{
	const NxWindow w = nxWindowOpen();
	scene.releaseEffector(*e);
	nxWindowPrint(label, w);
	}

// Slot 2 (tick: slot 3 on the two records) and slot 3 itself with each
// record alone and with neither, through the internal effector's own table.
static void nxSlotCalls(const char* label, void* internal, unsigned char* recA, unsigned char* recB)
	{
	unsigned char* records[2] = { recA, recB };
	for(unsigned i = 0; i < 2; i++)
		{
		printf("effector %s rec%u root=", label, i);
		nxPrintWord(nxWordAt(records[i], 0x1e8), true);
		printf("\n");
		printf("effector %s rec%u island=", label, i);
		for(unsigned off = 0x1bc; off <= 0x200; off += 4)
			nxPrintWord(nxWordAt(records[i], off), off == 0x1bc);
		printf(" wake=%08x\n", static_cast<unsigned>(nxWordAt(records[i], 0x4c)));
		}
	for(unsigned i = 0; i < 2; i++)
		if(nxPointerAt(records[i], 0x1e8) != records[i])
			{
			printf("effector %s slots skipped=root\n", label);
			return;
			}
	for(unsigned i = 0; i < 2; i++)
		*reinterpret_cast<NxU32*>(records[i] + 0x1f8) = 0;

	static unsigned char before[3][0x260];
	memcpy(before[0], internal, 0x68);
	memcpy(before[1], recA, 0x260);
	memcpy(before[2], recB, 0x260);
	const NxWindow w = nxWindowOpen();
	reinterpret_cast<NxSlotVoid>(nxSlot(internal, 2))(internal);
	reinterpret_cast<NxSlotApply>(nxSlot(internal, 3))(internal, recA, 0);
	reinterpret_cast<NxSlotApply>(nxSlot(internal, 3))(internal, 0, recB);
	reinterpret_cast<NxSlotApply>(nxSlot(internal, 3))(internal, 0, 0);
	nxWindowPrint(label, w);
	const unsigned char* now[3] = { static_cast<unsigned char*>(internal), recA, recB };
	const unsigned sizes[3] = { 0x68, 0x260, 0x260 };
	const char* names[3] = { "internal", "rec0", "rec1" };
	for(unsigned k = 0; k < 3; k++)
		{
		unsigned changed = 0;
		printf("effector %s slots %s changed", label, names[k]);
		for(unsigned off = 0; off < sizes[k]; off += 4)
			if(nxWordAt(before[k], off) != nxWordAt(now[k], off))
				{
				printf(" %03x=%08x>%08x", off, static_cast<unsigned>(nxWordAt(before[k], off)),
					static_cast<unsigned>(nxWordAt(now[k], off)));
				changed++;
				}
		printf("%s\n", changed ? "" : " none");
		}
	}

static void nxEffectorCases(NxPhysicsSDK& sdk, NxScene& scene)
	{
	nxSymbolAdd(nxPointerAt(&scene, 0x24), "scene");
	nxSymbolAdd(nxPointerAt(&scene, 0x0c), "wlink");
	nxSymbolAdd(nxPointerAt(&scene, 0x10), "rlink");
	nxPrintCount("start", scene);

	NxActor* a = nxCreateBody(scene, nxQuatMatrix(0.1f, 0.2f, -0.1f, 0.95f), NxVec3(0.0f, 1.0f, 0.0f),
		NxVec3(0.5f, -0.25f, 1.0f), NxVec3(0.3f, 0.7f, -0.2f));
	NxActor* b = nxCreateBody(scene, nxQuatMatrix(-0.2f, 0.1f, 0.3f, 0.9f), NxVec3(3.0f, 0.5f, -1.0f),
		NxVec3(-0.75f, 0.1f, 0.4f), NxVec3(-0.5f, 0.2f, 0.9f));
	printf("effector actors=%s,%s\n", a ? "created" : "null", b ? "created" : "null");
	if(!a || !b)
		return;
	nxNameActor(a, "a");
	nxNameActor(b, "b");
	nxPrintRecord("fixture", "rec_a", nxActorRecord(a));
	nxPrintRecord("fixture", "rec_b", nxActorRecord(b));
	printf("effector fixture rec_a owner=");
	nxPrintWord(nxWordAt(nxActorRecord(a), 0x19c), true);
	printf("\n");

	// Create, read back, set, read back, swap the ends.
	NxSpringAndDamperEffector* e = nxCreate("create", scene, a, b);
	if(!e)
		return;
	nxProbeHookAdjustor(e);
	void* internal = nxPointerAt(e, 0x14);
	nxPrintRecord("create", "rec_a", nxActorRecord(a));
	nxPrintRecord("create", "rec_b", nxActorRecord(b));
	nxPrintCount("create", scene);
	nxPrintIs("create", e);
	nxPrintGetters("create", e);

	e->setLinearSpring(0.75f, 2.0f, 4.5f, 12.5f, 30.0f);
	e->setLinearDamper(-1.5f, 3.25f, 0.5f, 6.0f);
	nxPrintGetters("set", e);
	nxPrintInternal("set", internal);

	{
	const NxWindow w = nxWindowOpen();
	e->setBodies(b, NxVec3(-0.4f, 2.25f, 0.6f), a, NxVec3(1.1f, -0.35f, 0.05f));
	nxWindowPrint("swap", w);
	}
	nxPrintInternal("swap", internal);
	nxPrintRecord("swap", "rec_a", nxActorRecord(a));
	nxPrintRecord("swap", "rec_b", nxActorRecord(b));

	{
	const NxWindow w = nxWindowOpen();
	e->setBodies(a, NxVec3(0.25f, 1.5f, -0.3f), b, NxVec3(2.6f, 0.2f, -0.85f));
	nxWindowPrint("restore", w);
	}
	nxPrintInternal("restore", internal);

	nxSlotCalls("slots", internal, nxActorRecord(a), nxActorRecord(b));

	nxRelease("release", scene, e);
	nxPrintCount("release", scene);
	nxPrintRecord("release", "rec_a", nxActorRecord(a));
	nxPrintRecord("release", "rec_b", nxActorRecord(b));

	// A release/create cycle: two effectors, the first released, a third.
	NxSpringAndDamperEffector* e1 = nxCreate("cycle1", scene, a, b);
	NxSpringAndDamperEffector* e2 = nxCreate("cycle2", scene, b, a);
	if(!e1 || !e2)
		return;
	nxSymbolAdd(e1, "np1");
	nxSymbolAdd(nxPointerAt(e1, 0x14), "eff1");
	nxSymbolAdd(e2, "np2");
	nxSymbolAdd(nxPointerAt(e2, 0x14), "eff2");
	nxPrintCount("cycle2", scene);
	nxPrintRecord("cycle2", "rec_a", nxActorRecord(a));
	nxPrintRecord("cycle2", "rec_b", nxActorRecord(b));
	nxRelease("cycle_release1", scene, e1);
	nxPrintCount("cycle_release1", scene);
	NxSpringAndDamperEffector* e3 = nxCreate("cycle3", scene, a, b);
	if(!e3)
		return;
	nxSymbolAdd(e3, "np3");
	nxSymbolAdd(nxPointerAt(e3, 0x14), "eff3");
	nxPrintCount("cycle3", scene);
	nxPrintRecord("cycle3", "rec_a", nxActorRecord(a));
	nxPrintRecord("cycle3", "rec_b", nxActorRecord(b));

	// Actor a released while e2 and e3 hold its record: the record's 0x100
	// notify nulls their pointers to it; then e3 is released (it stops
	// observing rec_b only).
	{
	const NxWindow w = nxWindowOpen();
	scene.releaseActor(*a);
	nxWindowPrint("actor_release", w);
	}
	nxPrintInternal("actor_release e2", nxPointerAt(e2, 0x14));
	nxPrintInternal("actor_release e3", nxPointerAt(e3, 0x14));
	nxPrintRecord("actor_release", "rec_b", nxActorRecord(b));
	nxPrintCount("actor_release", scene);
	nxRelease("after_actor_release", scene, e3);
	nxPrintCount("after_actor_release", scene);
	nxPrintRecord("after_actor_release", "rec_b", nxActorRecord(b));

	// Two fresh actors and one more effector, left alive with e2 for the
	// scene release.
	NxActor* c = nxCreateBody(scene, nxQuatMatrix(0.3f, -0.1f, 0.2f, 0.85f), NxVec3(-2.0f, 0.5f, 1.5f),
		NxVec3(0.1f, 0.2f, 0.3f), NxVec3(0.0f, 0.4f, 0.0f));
	NxActor* d = nxCreateBody(scene, nxQuatMatrix(0.0f, 0.0f, 0.0f, 1.0f), NxVec3(1.0f, 3.0f, 2.0f),
		NxVec3(0.0f, 0.0f, 0.0f), NxVec3(0.0f, 0.0f, 0.0f));
	printf("effector actors2=%s,%s\n", c ? "created" : "null", d ? "created" : "null");
	if(!c || !d)
		return;
	nxNameActor(c, "c");
	nxNameActor(d, "d");
	NxSpringAndDamperEffector* e4 = nxCreate("live", scene, c, d);
	if(!e4)
		return;
	nxSymbolAdd(e4, "np4");
	nxSymbolAdd(nxPointerAt(e4, 0x14), "eff4");
	nxPrintCount("live", scene);

	const NxWindow w = nxWindowOpen();
	sdk.releaseScene(scene);
	nxWindowPrint("scene_release", w);
	}

int wmain(int argc, wchar_t** argv)
	{
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsEffectorTests", pairDirectory, &physics);
	if(status)
		return status;
	// Unbuffered, so a fault leaves the lines before it.
	setvbuf(stdout, 0, _IONBF, 0);

	CreatePhysicsSDKFn createSDK =
		reinterpret_cast<CreatePhysicsSDKFn>(GetProcAddress(physics, "NxCreatePhysicsSDK"));
	printf("export=NxCreatePhysicsSDK present=%s\n", createSDK ? "yes" : "no");
	if(!createSDK)
		{
		FreeLibrary(physics);
		return nxFail("a required export is missing");
		}
	nxImageRange(physics, 0);
	HMODULE foundation = GetModuleHandleW(L"NxFoundation.dll");
	if(foundation)
		nxImageRange(foundation, 1);

	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, &gAllocator, 0);
	printf("sdk=%s\n", sdk ? "created" : "null");
	if(!sdk)
		{
		FreeLibrary(physics);
		return nxFail("SDK creation failed");
		}

	NxSceneDesc sceneDesc;
	sceneDesc.setToDefault();
	sceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	NxScene* scene = sdk->createScene(sceneDesc);
	printf("scene=%s\n", scene ? "created" : "null");
	if(!scene)
		{
		sdk->release();
		FreeLibrary(physics);
		return nxFail("scene creation failed");
		}

	nxEffectorCases(*sdk, *scene);
	printf("scene=released\n");
	sdk->release();
	printf("sdk=released\n");

	status = nxReportPairIdentity(pairDirectory);
	if(status)
		return status;
	FreeLibrary(physics);
	return 0;
	}
