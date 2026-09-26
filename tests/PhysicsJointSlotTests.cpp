// The internal-slot joint differential (joint-open-items Task 6, open items 4
// and 5).
//
// No simulation step exists in the candidate, so the joint rows the step and
// the debug renderer reach -- every family's solver slot (internal slot 6),
// the limit-plane slot (7), the impulse slot (0), the projection slot (8) and
// the debug-visualization slot (4) -- had been checked only against the
// listing. This harness reaches them directly: it creates one joint of each
// family through the public API, takes the internal joint the public object
// keeps at +0x18, and calls the internal virtuals through that object's own
// vtable BY SLOT INDEX. Nothing here names an oracle address, so the same
// code drives both DLLs of the staged pair, and the transcript is compared
// word for word by run_differential.ps1.
//
// Harness setup that stands in for the simulation step. Each item is state
// the step builds before it calls the joint slots, and is injected identically
// on both sides:
//
//   * body record +0x204. The step's phys_fn_000611 points it at the body's
//     0x60-byte element of the Scene's +0x5ac array. Here each dynamic body
//     gets a harness-owned 0x60-byte block filled from the body record's own
//     words exactly as 000611 fills it (+0x00..+0x08 <- +0x34..+0x3c, +0x0c
//     <- +0xc0, +0x10..+0x18 <- +0x40..+0x48, +0x1c <- the body record,
//     +0x20..+0x40 <- the nine dwords at +0x164, +0x5c <- +0x110; +0x44..+0x58
//     zero). The words are read from each DLL's own body record, so each side
//     gets the content its own step would build. +0x204 is put back to 0
//     before the actors are released.
//   * the Scene's record count +0x5bc is set to 0 before each emulated step,
//     as phys_fn_000613 clears it after each island, and the joint's record
//     window (+0x160 = -1, +0x164 = 0) is reset as phys_fn_000728 does before
//     it calls phys_fn_004133. The records the slots write are then records
//     0..count-1 of the array at +0x5b8.
//   * an emulated step calls slot 1, then slot 7, then slot 6, which is the
//     order phys_fn_004133 uses (slot 7, then slot 6 unless flag bit 2).
//     004133 itself is only called directly by the step's 000728, never
//     through a table (every family overrides slot 6), so it stays unreached.
//
// Every pointer-valued word the transcript would print (the harness blocks,
// the body records, the joint, the Scene) is printed as a name, never as an
// address, because addresses differ between the two processes.
//
// Everything is run under the default control word, and the step and
// projection calls again under the in-step word 0x0f7f (PC 64, RC chop), set
// with fldcw around each call and restored after. A difference that appears
// only under 0x0f7f is open item 5 (PC64 narrowing at the naked x87 helpers).
//
// D6's solver slot writes D6JointDump.txt to the working directory and never
// closes it. The harness makes the pair directory the working directory, so
// each side writes its own file there, and the two files are compared after
// the run (evidence/joint-open-items.md, Task 6). They are not printed here:
// the candidate's CRT is the shared UCRT, which flushes the stream only at
// process exit, so the file is empty for as long as this process runs.
//
// The last section drives the Foundation export NxFindRotationMatrix, which
// the projection slots call (004356, 004298, 004207), over both of its arms
// and every choice of helper axis in the parallel arm.

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
#include "NxJoint.h"
#include "NxJointDesc.h"
#include "NxDebugRenderable.h"
#include "NxRevoluteJointDesc.h"
#include "NxPrismaticJointDesc.h"
#include "NxCylindricalJointDesc.h"
#include "NxSphericalJointDesc.h"
#include "NxPointOnLineJointDesc.h"
#include "NxPointInPlaneJointDesc.h"
#include "NxDistanceJointDesc.h"
#include "NxPulleyJointDesc.h"
#include "NxFixedJointDesc.h"
#include "NxBitField.h"
#include "NxD6JointDesc.h"

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32, NxUserAllocator*, NxUserOutputStream*);
typedef void (NX_CALL_CONV *JointDescSetGlobalAnchorFn)(NxJointDesc&, const NxVec3&);
typedef void (NX_CALL_CONV *JointDescSetGlobalAxisFn)(NxJointDesc&, const NxVec3&);

static JointDescSetGlobalAnchorFn nxSetGlobalAnchor = 0;
static JointDescSetGlobalAxisFn nxSetGlobalAxis = 0;

// The internal slots, called by index through the object's own table.
typedef void (__thiscall *NxSlotVoid)(void* self);
typedef void (__thiscall *NxSlotU32)(void* self, NxU32 arg);
typedef void (__thiscall *NxSlotBreak)(void* self, const void* record, NxReal value);
typedef void (__thiscall *NxSlotRender)(void* self, NxDebugRenderable* renderable);
typedef void (__thiscall *NxSlotReal)(void* self, NxReal arg);
typedef void (__thiscall *NxSlotPointer)(void* self, void* body);

static void* nxSlot(void* object, unsigned index)
	{
	return (*reinterpret_cast<void***>(object))[index];
	}

static const unsigned short kControlDefault = 0x027f;	// PC 53, RC near
static const unsigned short kControlStep = 0x0f7f;		// PC 64, RC chop

static unsigned short nxGetControl()
	{
	unsigned short value;
	__asm { fnstcw value }
	return value;
	}

static void nxSetControl(unsigned short word)
	{
	unsigned short value = word;
	__asm { fldcw value }
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

// ---------------------------------------------------------------------------
// Pointer names. A word equal to one of these is printed as its name.

struct NxSymbol
	{
	const void* pointer;
	const char* name;
	};

static NxSymbol nxSymbols[16];
static unsigned nxSymbolCount = 0;

static void nxSymbolsReset()
	{
	nxSymbolCount = 0;
	}

static void nxSymbolAdd(const void* pointer, const char* name)
	{
	if(pointer && nxSymbolCount < 16)
		{
		nxSymbols[nxSymbolCount].pointer = pointer;
		nxSymbols[nxSymbolCount].name = name;
		nxSymbolCount++;
		}
	}

// Prints one word, with a separator unless it is the first.
static void nxPrintWord(NxU32 word, bool first)
	{
	if(!first)
		printf(".");
	for(unsigned i = 0; i < nxSymbolCount; i++)
		if(word == reinterpret_cast<NxU32>(nxSymbols[i].pointer))
			{
			printf("%s", nxSymbols[i].name);
			return;
			}
	printf("%08x", static_cast<unsigned>(word));
	}

static void nxPrintVec(const char* tag, const NxVec3& v)
	{
	printf(" %s=%08x.%08x.%08x", tag, nxU(v.x), nxU(v.y), nxU(v.z));
	}

// ---------------------------------------------------------------------------
// The case label every line starts with.

static char nxLabel[96];

// ---------------------------------------------------------------------------
// A recording NxDebugRenderable: every call is printed as raw words.

class NxRecordingRenderable : public NxDebugRenderable
	{
	public:
	NxRecordingRenderable() : mCalls(0) {}

	unsigned mCalls;

	virtual NxU32 getNbPoints() const { return 0; }
	virtual const NxDebugPoint* getPoints() const { return 0; }
	virtual NxU32 getNbLines() const { return 0; }
	virtual const NxDebugLine* getLines() const { return 0; }
	virtual NxU32 getNbTriangles() const { return 0; }
	virtual const NxDebugTriangle* getTriangles() const { return 0; }
	virtual void clear() { head("clear"); printf("\n"); }
	virtual void addPoint(const NxVec3& p, NxU32 color)
		{
		head("point");
		nxPrintVec("p", p);
		printf(" color=%08x\n", static_cast<unsigned>(color));
		}
	virtual void addLine(const NxVec3& p0, const NxVec3& p1, NxU32 color)
		{
		head("line");
		nxPrintVec("p0", p0);
		nxPrintVec("p1", p1);
		printf(" color=%08x\n", static_cast<unsigned>(color));
		}
	virtual void addTriangle(const NxVec3& p0, const NxVec3& p1, const NxVec3& p2, NxU32 color)
		{
		head("triangle");
		nxPrintVec("p0", p0);
		nxPrintVec("p1", p1);
		nxPrintVec("p2", p2);
		printf(" color=%08x\n", static_cast<unsigned>(color));
		}
	virtual void addOBB(const NxBox& box, NxU32 color, bool renderFrame)
		{
		head("obb");
		nxPrintVec("c", box.center);
		nxPrintVec("e", box.extents);
		printf(" color=%08x frame=%u\n", static_cast<unsigned>(color), renderFrame ? 1u : 0u);
		}
	virtual void addAABB(const NxBounds3& bounds, NxU32 color, bool renderFrame)
		{
		head("aabb");
		nxPrintVec("min", bounds.getMin());
		nxPrintVec("max", bounds.getMax());
		printf(" color=%08x frame=%u\n", static_cast<unsigned>(color), renderFrame ? 1u : 0u);
		}
	virtual void addArrow(const NxVec3& position, const NxVec3& direction, NxReal length, NxReal scale, NxU32 color)
		{
		head("arrow");
		nxPrintVec("p", position);
		nxPrintVec("d", direction);
		printf(" length=%08x scale=%08x color=%08x\n", nxU(length), nxU(scale), static_cast<unsigned>(color));
		}
	virtual void addBasis(const NxVec3& position, const NxMat33& columns, const NxVec3& lengths, NxReal scale, NxU32 colors[3])
		{
		head("basis");
		nxPrintVec("p", position);
		NxReal m[9];
		columns.getRowMajor(m);
		printf(" m=");
		for(unsigned i = 0; i < 9; i++)
			printf("%s%08x", i ? "." : "", nxU(m[i]));
		nxPrintVec("lengths", lengths);
		printf(" scale=%08x colors=", nxU(scale));
		if(colors)
			printf("%08x.%08x.%08x\n", static_cast<unsigned>(colors[0]), static_cast<unsigned>(colors[1]),
				static_cast<unsigned>(colors[2]));
		else
			printf("null\n");
		}
	virtual void addCircle(NxU32 nbSegments, const NxMat34& matrix, NxU32 color, NxF32 radius, bool semicircle)
		{
		head("circle");
		NxReal m[9];
		matrix.M.getRowMajor(m);
		printf(" segments=%u m=", static_cast<unsigned>(nbSegments));
		for(unsigned i = 0; i < 9; i++)
			printf("%s%08x", i ? "." : "", nxU(m[i]));
		nxPrintVec("t", matrix.t);
		printf(" color=%08x radius=%08x semi=%u\n", static_cast<unsigned>(color), nxU(radius), semicircle ? 1u : 0u);
		}

	private:
	void head(const char* kind)
		{
		printf("%s vis n=%u %s", nxLabel, mCalls, kind);
		mCalls++;
		}
	};

// ---------------------------------------------------------------------------
// Snapshots and differences.

enum { NX_BODY_RECORD_SIZE = 0x260, NX_SUPPORT_SIZE = 0x60, NX_MAX_JOINT_SIZE = 0x270 };

struct NxSnapshot
	{
	unsigned char joint[NX_MAX_JOINT_SIZE];
	unsigned char body[2][NX_BODY_RECORD_SIZE];
	unsigned char support[2][NX_SUPPORT_SIZE];
	};

struct NxSlotCase
	{
	const char* family;
	unsigned config;
	unsigned char* internal;	// the internal joint (public +0x18)
	unsigned jointSize;
	unsigned char* body[2];		// Joint +0x08 / +0x0c, null for the world
	unsigned char* support[2];	// the injected +0x204 blocks
	unsigned char* scene;		// Joint +0x30
	};

static void nxTake(const NxSlotCase& c, NxSnapshot& s)
	{
	memcpy(s.joint, c.internal, c.jointSize);
	for(unsigned i = 0; i < 2; i++)
		{
		if(c.body[i])
			memcpy(s.body[i], c.body[i], NX_BODY_RECORD_SIZE);
		if(c.support[i])
			memcpy(s.support[i], c.support[i], NX_SUPPORT_SIZE);
		}
	}

// Prints the words of `now` that differ from `before`, eight per line.
static void nxPrintChanged(const char* what, const unsigned char* before, const unsigned char* now, unsigned size)
	{
	unsigned changed = 0;
	for(unsigned off = 0; off < size; off += 4)
		{
		const NxU32 a = nxWordAt(before, off);
		const NxU32 b = nxWordAt(now, off);
		if(a == b)
			continue;
		if(changed % 8 == 0)
			printf("%s%s %s changed", changed ? "\n" : "", nxLabel, what);
		printf(" %03x=", off);
		nxPrintWord(a, true);
		printf(">");
		nxPrintWord(b, true);
		changed++;
		}
	if(changed)
		printf("\n");
	else
		printf("%s %s changed none\n", nxLabel, what);
	}

static void nxPrintDifferences(const NxSlotCase& c, const NxSnapshot& before)
	{
	nxPrintChanged("joint", before.joint, c.internal, c.jointSize);
	for(unsigned i = 0; i < 2; i++)
		{
		if(c.body[i])
			nxPrintChanged(i ? "body1" : "body0", before.body[i], c.body[i], NX_BODY_RECORD_SIZE);
		if(c.support[i])
			nxPrintChanged(i ? "support1" : "support0", before.support[i], c.support[i], NX_SUPPORT_SIZE);
		}
	}

static void nxPrintWords(const char* what, const unsigned char* base, unsigned size)
	{
	for(unsigned off = 0; off < size; off += 0x20)
		{
		printf("%s %s off=%03x words=", nxLabel, what, off);
		for(unsigned w = off; w < off + 0x20 && w < size; w += 4)
			nxPrintWord(nxWordAt(base, w), w == off);
		printf("\n");
		}
	}

// The records the last emulated step took: count, capacity, then each 0x50
// bytes as 20 words.
static void nxPrintRecords(const NxSlotCase& c)
	{
	const NxU32 count = nxWordAt(c.scene, 0x5bc);
	const NxU32 capacity = nxWordAt(c.scene, 0x5c0);
	const unsigned char* records = *reinterpret_cast<unsigned char* const*>(c.scene + 0x5b8);
	printf("%s records count=%u capacity=%u window=%08x.%08x\n", nxLabel, static_cast<unsigned>(count),
		static_cast<unsigned>(capacity), static_cast<unsigned>(nxWordAt(c.internal, 0x160)),
		static_cast<unsigned>(nxWordAt(c.internal, 0x164)));
	for(NxU32 r = 0; r < count && r < 32; r++)
		{
		printf("%s record=%u words=", nxLabel, static_cast<unsigned>(r));
		for(unsigned w = 0; w < 0x50; w += 4)
			nxPrintWord(nxWordAt(records + r * 0x50, w), w == 0);
		printf("\n");
		}
	}

// ---------------------------------------------------------------------------
// The calls.

static void nxLabelSet(const NxSlotCase& c, const char* call, unsigned short control)
	{
	sprintf(nxLabel, "slots family=%s config=%u call=%s cw=%04x", c.family, c.config, call,
		static_cast<unsigned>(control));
	}

static void nxCallVisualization(const NxSlotCase& c)
	{
	nxLabelSet(c, "vis", kControlDefault);
	NxSnapshot before;
	nxTake(c, before);
	NxRecordingRenderable renderable;
	reinterpret_cast<NxSlotRender>(nxSlot(c.internal, 4))(c.internal, &renderable);
	printf("%s calls=%u\n", nxLabel, renderable.mCalls);
	nxPrintDifferences(c, before);
	}

// One emulated step for this joint: the record window and the Scene count
// reset as 000728/000613 do, then slots 1, 7 and 6, under `control`.
static void nxCallStep(const NxSlotCase& c, NxReal dt, unsigned short control)
	{
	nxLabelSet(c, "step", control);
	*reinterpret_cast<NxU32*>(c.scene + 0x5bc) = 0;
	*reinterpret_cast<NxU32*>(c.internal + 0x160) = 0xffffffffu;
	*reinterpret_cast<NxU32*>(c.internal + 0x164) = 0;
	NxSnapshot before;
	nxTake(c, before);
	const unsigned short saved = nxGetControl();
	nxSetControl(control);
	reinterpret_cast<NxSlotVoid>(nxSlot(c.internal, 1))(c.internal);
	reinterpret_cast<NxSlotReal>(nxSlot(c.internal, 7))(c.internal, dt);
	reinterpret_cast<NxSlotReal>(nxSlot(c.internal, 6))(c.internal, dt);
	nxSetControl(saved);
	nxPrintRecords(c);
	nxPrintDifferences(c, before);
	}

// The impulse slot (0); its argument is never read by any family.
static void nxCallImpulse(const NxSlotCase& c, unsigned short control)
	{
	nxLabelSet(c, "impulse", control);
	NxSnapshot before;
	nxTake(c, before);
	const unsigned short saved = nxGetControl();
	nxSetControl(control);
	reinterpret_cast<NxSlotU32>(nxSlot(c.internal, 0))(c.internal, 0);
	nxSetControl(saved);
	nxPrintDifferences(c, before);
	}

// The projection slot (8), once for each body record the joint holds.
static void nxCallProjection(const NxSlotCase& c, unsigned short control)
	{
	for(unsigned i = 0; i < 2; i++)
		{
		if(!c.body[i])
			continue;
		nxLabelSet(c, i ? "project1" : "project0", control);
		NxSnapshot before;
		nxTake(c, before);
		const unsigned short saved = nxGetControl();
		nxSetControl(control);
		reinterpret_cast<NxSlotPointer>(nxSlot(c.internal, 8))(c.internal, c.body[i]);
		nxSetControl(saved);
		nxPrintDifferences(c, before);
		}
	}

// The break test (slot 2, phys_fn_004111) on the first record of the last
// step: marks the joint broken and posts a break event through 000571.
static void nxCallBreak(const NxSlotCase& c, NxJoint& joint)
	{
	nxLabelSet(c, "break", kControlDefault);
	const NxU32 count = nxWordAt(c.scene, 0x5bc);
	if(!count)
		{
		printf("%s skipped=no_record\n", nxLabel);
		return;
		}
	const unsigned char* records = *reinterpret_cast<unsigned char* const*>(c.scene + 0x5b8);
	NxSnapshot before;
	nxTake(c, before);
	reinterpret_cast<NxSlotBreak>(nxSlot(c.internal, 2))(c.internal, records, 2.5f);
	printf("%s state=%u record0_flags=%08x\n", nxLabel, static_cast<unsigned>(joint.getState()),
		static_cast<unsigned>(nxWordAt(records, 0x0c)));
	nxPrintDifferences(c, before);
	}

// ---------------------------------------------------------------------------
// The fixture: two fresh dynamic actors per case, posed and moving.

static NxActor* nxCreateBody(NxScene& scene, const NxMat33& rotation, const NxVec3& position)
	{
	NxBoxShapeDesc box;
	box.dimensions = NxVec3(1.0f, 0.5f, 0.75f);
	NxBodyDesc body;
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

static unsigned char nxSupportBlocks[2][NX_SUPPORT_SIZE];

// Fills a support block from a body record as phys_fn_000611 does.
static void nxFillSupport(unsigned char* support, unsigned char* body)
	{
	memset(support, 0, NX_SUPPORT_SIZE);
	memcpy(support + 0x00, body + 0x34, 12);
	memcpy(support + 0x0c, body + 0xc0, 4);
	memcpy(support + 0x10, body + 0x40, 12);
	memcpy(support + 0x1c, &body, 4);
	memcpy(support + 0x20, body + 0x164, 36);
	memcpy(support + 0x5c, body + 0x110, 4);
	}

static unsigned nxJointSize(NxJointType type)
	{
	switch(type)
		{
		case NX_JOINT_PRISMATIC:	return 0x17c;
		case NX_JOINT_REVOLUTE:		return 0x204;
		case NX_JOINT_SPHERICAL:	return 0x23c;
		case NX_JOINT_DISTANCE:		return 0x184;
		case NX_JOINT_PULLEY:		return 0x1e0;
		case NX_JOINT_FIXED:		return 0x188;
		case NX_JOINT_D6:			return 0x270;
		default:					return 0x16c;
		}
	}

// One family case. `worldBody0` leaves actor 0 null (the world), which the
// pulley case uses so that 004228 never reads its uninitialised body-0 lever.
static void nxSlotCase(NxScene& scene, NxJointDesc& desc, const char* family, unsigned config,
	bool worldBody0, bool breakTest)
	{
	NxActor* a = worldBody0 ? 0 : nxCreateBody(scene, nxQuatMatrix(0.1f, 0.2f, -0.1f, 0.95f), NxVec3(0.0f, 1.0f, 0.0f));
	NxActor* b = nxCreateBody(scene, nxQuatMatrix(-0.2f, 0.1f, 0.3f, 0.9f), NxVec3(3.0f, 0.5f, -1.0f));
	printf("slots family=%s config=%u actors=%s,%s\n", family, config,
		worldBody0 ? "world" : (a ? "created" : "null"), b ? "created" : "null");
	if((!worldBody0 && !a) || !b)
		return;

	desc.actor[0] = a;
	desc.actor[1] = b;
	nxSetGlobalAnchor(desc, NxVec3(1.5f, 0.75f, -0.5f));
	nxSetGlobalAxis(desc, NxVec3(0.36f, 0.48f, 0.8f));
	NxJoint* joint = scene.createJoint(desc);
	printf("slots family=%s config=%u created=%s\n", family, config, joint ? "yes" : "no");
	if(!joint)
		{
		if(a)
			scene.releaseActor(*a);
		scene.releaseActor(*b);
		return;
		}

	// A limit point on body 1 behind one of two limit planes, so the
	// limit-plane slot (7, 004135) takes a record.
	joint->setLimitPoint(NxVec3(2.0f, 0.5f, -0.5f), true);
	const bool plane0 = joint->addLimitPlane(NxVec3(0.0f, 1.0f, 0.0f), NxVec3(0.0f, -3.0f, 0.0f));
	const bool plane1 = joint->addLimitPlane(NxVec3(1.0f, 0.0f, 0.0f), NxVec3(2.5f, 0.0f, 0.0f));
	printf("slots family=%s config=%u limit_planes=%u.%u\n", family, config, plane0 ? 1u : 0u, plane1 ? 1u : 0u);

	// The joint drifts: body 1 moves and turns after the joint was made, and
	// both bodies move, so the solver has an error and velocities to use.
	b->setGlobalPosition(NxVec3(3.25f, 0.375f, -0.875f));
	b->setGlobalOrientation(nxQuatMatrix(-0.25f, 0.125f, 0.3f, 0.875f));
	b->setLinearVelocity(NxVec3(0.5f, -0.25f, 0.125f));
	b->setAngularVelocity(NxVec3(-0.75f, 0.375f, 1.25f));
	if(a)
		{
		a->setLinearVelocity(NxVec3(-0.125f, 0.625f, 0.25f));
		a->setAngularVelocity(NxVec3(0.5f, 0.25f, -0.375f));
		}

	NxSlotCase c;
	c.family = family;
	c.config = config;
	c.internal = *reinterpret_cast<unsigned char**>(reinterpret_cast<unsigned char*>(joint) + 0x18);
	c.jointSize = nxJointSize(joint->getType());
	c.scene = *reinterpret_cast<unsigned char**>(c.internal + 0x30);
	for(unsigned i = 0; i < 2; i++)
		{
		c.body[i] = *reinterpret_cast<unsigned char**>(c.internal + 0x08 + 4 * i);
		c.support[i] = 0;
		if(c.body[i])
			{
			c.support[i] = nxSupportBlocks[i];
			nxFillSupport(c.support[i], c.body[i]);
			*reinterpret_cast<unsigned char**>(c.body[i] + 0x204) = c.support[i];
			}
		}

	nxSymbolsReset();
	nxSymbolAdd(c.internal, "JOINT");
	nxSymbolAdd(joint, "NPJOINT");
	nxSymbolAdd(c.scene, "SCENE");
	nxSymbolAdd(c.body[0], "BODY0");
	nxSymbolAdd(c.body[1], "BODY1");
	nxSymbolAdd(c.support[0], "SUPPORT0");
	nxSymbolAdd(c.support[1], "SUPPORT1");

	printf("slots family=%s config=%u type=%u bodies=%s,%s size=%x\n", family, config,
		static_cast<unsigned>(joint->getType()), c.body[0] ? "BODY0" : "world", c.body[1] ? "BODY1" : "world",
		c.jointSize);
	sprintf(nxLabel, "slots family=%s config=%u call=setup", family, config);
	for(unsigned i = 0; i < 2; i++)
		if(c.support[i])
			nxPrintWords(i ? "support1" : "support0", c.support[i], NX_SUPPORT_SIZE);

	const NxReal dt = 1.0f / 60.0f;
	nxCallVisualization(c);
	nxCallStep(c, dt, kControlDefault);
	nxCallImpulse(c, kControlDefault);
	nxCallProjection(c, kControlDefault);
	nxCallStep(c, dt, kControlStep);
	nxCallImpulse(c, kControlStep);
	nxCallProjection(c, kControlStep);
	nxCallVisualization(c);
	if(breakTest)
		nxCallBreak(c, *joint);

	for(unsigned i = 0; i < 2; i++)
		if(c.body[i])
			*reinterpret_cast<unsigned char**>(c.body[i] + 0x204) = 0;
	nxSymbolsReset();
	scene.releaseJoint(*joint);
	if(a)
		scene.releaseActor(*a);
	scene.releaseActor(*b);
	printf("slots family=%s config=%u released=yes\n", family, config);
	}

// ---------------------------------------------------------------------------
// The family configurations.

static void nxRevoluteCases(NxScene& scene)
	{
	NxRevoluteJointDesc limited;
	limited.limit.low.value = -0.25f;
	limited.limit.low.restitution = 0.5f;
	limited.limit.high.value = 0.125f;
	limited.limit.high.restitution = 0.25f;
	limited.spring = NxSpringDesc(8.0f, 0.5f, 0.0625f);
	limited.flags = NX_RJF_LIMIT_ENABLED | NX_RJF_SPRING_ENABLED;
	limited.projectionMode = NX_JPM_POINT_MINDIST;
	limited.projectionDistance = 0.05f;
	limited.projectionAngle = 0.03125f;
	nxSlotCase(scene, limited, "revolute", 0, false, true);

	NxRevoluteJointDesc motor;
	motor.motor = NxMotorDesc(2.5f, 40.0f, 1);
	motor.flags = NX_RJF_MOTOR_ENABLED;
	motor.projectionMode = NX_JPM_POINT_MINDIST;
	motor.projectionDistance = 0.5f;
	motor.projectionAngle = 1.0f;
	nxSlotCase(scene, motor, "revolute", 1, false, false);
	}

static void nxSphericalCases(NxScene& scene)
	{
	NxSphericalJointDesc all;
	all.twistLimit.low.value = -0.5f;
	all.twistLimit.low.restitution = 0.25f;
	all.twistLimit.high.value = 0.375f;
	all.twistLimit.high.restitution = 0.125f;
	all.swingLimit.value = 0.25f;
	all.swingLimit.restitution = 0.5f;
	all.twistSpring = NxSpringDesc(6.0f, 0.25f, 0.125f);
	all.swingSpring = NxSpringDesc(4.0f, 0.75f, 0.0625f);
	all.jointSpring = NxSpringDesc(12.0f, 1.5f, 0.0f);
	all.swingAxis = NxVec3(0.0f, 0.6f, 0.8f);
	all.flags = NX_SJF_TWIST_LIMIT_ENABLED | NX_SJF_SWING_LIMIT_ENABLED | NX_SJF_TWIST_SPRING_ENABLED
		| NX_SJF_SWING_SPRING_ENABLED | NX_SJF_JOINT_SPRING_ENABLED;
	all.projectionMode = NX_JPM_POINT_MINDIST;
	all.projectionDistance = 0.05f;
	nxSlotCase(scene, all, "spherical", 0, false, false);

	NxSphericalJointDesc plain;
	plain.projectionMode = NX_JPM_POINT_MINDIST;
	plain.projectionDistance = 2.0f;
	nxSlotCase(scene, plain, "spherical", 1, false, false);
	}

static void nxD6Cases(NxScene& scene)
	{
	NxD6JointDesc first;
	first.xMotion = NX_D6JOINT_MOTION_LOCKED;
	first.yMotion = NX_D6JOINT_MOTION_LIMITED;
	first.zMotion = NX_D6JOINT_MOTION_FREE;
	first.twistMotion = NX_D6JOINT_MOTION_LIMITED;
	first.swing1Motion = NX_D6JOINT_MOTION_LIMITED;
	first.swing2Motion = NX_D6JOINT_MOTION_LOCKED;
	first.linearLimit.value = 0.25f;
	first.linearLimit.restitution = 0.25f;
	first.twistLimit.low.value = -0.25f;
	first.twistLimit.high.value = 0.375f;
	first.swing1Limit.value = 0.3125f;
	first.swing2Limit.value = 0.1875f;
	first.xDrive.driveType = NX_D6JOINT_DRIVE_POSITION;
	first.xDrive.spring = 3.0f;
	first.xDrive.damping = 0.5f;
	first.xDrive.forceLimit = 50.0f;
	first.twistDrive.driveType = NX_D6JOINT_DRIVE_VELOCITY;
	first.twistDrive.spring = 2.0f;
	first.twistDrive.damping = 0.25f;
	first.twistDrive.forceLimit = 20.0f;
	first.useSpherical = false;
	first.drivePosition = NxVec3(0.125f, -0.25f, 0.0625f);
	first.driveOrientation.x = 0.0f;
	first.driveOrientation.y = 0.6f;
	first.driveOrientation.z = 0.0f;
	first.driveOrientation.w = 0.8f;
	first.driveLinearVelocity = NxVec3(0.5f, 0.0f, -0.25f);
	first.driveAngularVelocity = NxVec3(0.0f, 0.75f, 0.125f);
	first.projectionDistance = 0.0625f;
	first.projectionAngle = 0.03125f;
	first.projectionMode = NX_JPM_POINT_MINDIST;
	nxSlotCase(scene, first, "d6", 0, false, false);

	NxD6JointDesc second;
	second.xMotion = NX_D6JOINT_MOTION_LIMITED;
	second.yMotion = NX_D6JOINT_MOTION_LIMITED;
	second.zMotion = NX_D6JOINT_MOTION_LIMITED;
	second.twistMotion = NX_D6JOINT_MOTION_LOCKED;
	second.swing1Motion = NX_D6JOINT_MOTION_LOCKED;
	second.swing2Motion = NX_D6JOINT_MOTION_LIMITED;
	second.linearLimit.value = 0.5f;
	second.twistLimit.low.value = -0.5f;
	second.twistLimit.high.value = 0.5f;
	second.swing1Limit.value = 0.25f;
	second.swing2Limit.value = 0.4375f;
	second.swingDrive.driveType = NX_D6JOINT_DRIVE_POSITION;
	second.swingDrive.spring = 5.0f;
	second.swingDrive.damping = 1.0f;
	second.swingDrive.forceLimit = 30.0f;
	second.sphericalDrive.driveType = NX_D6JOINT_DRIVE_VELOCITY;
	second.sphericalDrive.spring = 1.5f;
	second.sphericalDrive.damping = 0.75f;
	second.sphericalDrive.forceLimit = 10.0f;
	second.useSpherical = true;
	second.drivePosition = NxVec3(0.0f, 0.0f, 0.0f);
	second.driveOrientation.x = 0.0f;
	second.driveOrientation.y = 0.0f;
	second.driveOrientation.z = 0.0f;
	second.driveOrientation.w = 1.0f;
	second.driveLinearVelocity = NxVec3(0.0f, 0.0f, 0.0f);
	second.driveAngularVelocity = NxVec3(0.25f, 0.0f, 0.0f);
	second.projectionDistance = 0.5f;
	second.projectionAngle = 0.25f;
	second.projectionMode = NX_JPM_POINT_MINDIST;
	nxSlotCase(scene, second, "d6", 1, false, false);
	}

static void nxAllSlotCases(NxScene& scene)
	{
	nxRevoluteCases(scene);

	NxPrismaticJointDesc prismatic;
	nxSlotCase(scene, prismatic, "prismatic", 0, false, false);
	NxCylindricalJointDesc cylindrical;
	nxSlotCase(scene, cylindrical, "cylindrical", 0, false, false);

	nxSphericalCases(scene);

	NxPointOnLineJointDesc pointOnLine;
	nxSlotCase(scene, pointOnLine, "point_on_line", 0, false, false);
	NxPointInPlaneJointDesc pointInPlane;
	nxSlotCase(scene, pointInPlane, "point_in_plane", 0, false, false);

	NxDistanceJointDesc distance;
	distance.maxDistance = 2.0f;
	distance.minDistance = 1.0f;
	distance.spring = NxSpringDesc(10.0f, 0.5f, 1.5f);
	distance.flags = NX_DJF_MAX_DISTANCE_ENABLED | NX_DJF_MIN_DISTANCE_ENABLED | NX_DJF_SPRING_ENABLED;
	nxSlotCase(scene, distance, "distance", 0, false, false);
	NxDistanceJointDesc rod;
	rod.maxDistance = 0.5f;
	rod.minDistance = 0.5f;
	rod.flags = NX_DJF_MAX_DISTANCE_ENABLED | NX_DJF_MIN_DISTANCE_ENABLED;
	nxSlotCase(scene, rod, "distance", 1, false, false);

	// Body 0 is the world, so 004228 writes every lever it reads.
	NxPulleyJointDesc pulley;
	pulley.pulley[0] = NxVec3(0.0f, 5.0f, 0.0f);
	pulley.pulley[1] = NxVec3(3.0f, 5.0f, -1.0f);
	pulley.distance = 6.0f;
	pulley.stiffness = 0.75f;
	pulley.ratio = 1.5f;
	pulley.flags = NX_PJF_IS_RIGID;
	nxSlotCase(scene, pulley, "pulley", 0, true, false);

	NxFixedJointDesc fixed;
	nxSlotCase(scene, fixed, "fixed", 0, false, false);

	nxD6Cases(scene);
	}

// ---------------------------------------------------------------------------
// NxFindRotationMatrix, the Foundation export the projection slots call.

typedef void (NX_CALL_CONV *FindRotationMatrixFn)(const NxVec3&, const NxVec3&, NxMat33&);

static NxVec3 nxUnit(NxReal x, NxReal y, NxReal z)
	{
	const NxReal inv = 1.0f / sqrtf(x * x + y * y + z * z);
	return NxVec3(x * inv, y * inv, z * inv);
	}

static void nxRotationCase(FindRotationMatrixFn find, unsigned index, const NxVec3& from, const NxVec3& to)
	{
	NxMat33 m;
	m.id();
	find(from, to, m);
	NxReal rows[9];
	m.getRowMajor(rows);
	printf("rotation case=%u", index);
	nxPrintVec("from", from);
	nxPrintVec("to", to);
	printf(" m=");
	for(unsigned i = 0; i < 9; i++)
		printf("%s%08x", i ? "." : "", nxU(rows[i]));
	printf("\n");
	}

static void nxRotationCases()
	{
	HMODULE foundation = GetModuleHandleW(L"NxFoundation.dll");
	FindRotationMatrixFn find = foundation ?
		reinterpret_cast<FindRotationMatrixFn>(GetProcAddress(foundation, "NxFindRotationMatrix")) : 0;
	printf("export=NxFindRotationMatrix present=%s\n", find ? "yes" : "no");
	if(!find)
		return;
	// The common arm: general pairs, a right angle, and two axes 0.002 rad
	// apart (|e| just below 1 - 1e-6).
	nxRotationCase(find, 0, NxVec3(1.0f, 0.0f, 0.0f), NxVec3(0.0f, 1.0f, 0.0f));
	nxRotationCase(find, 1, nxUnit(0.36f, 0.48f, 0.8f), nxUnit(-0.2f, 0.9f, 0.3f));
	nxRotationCase(find, 2, nxUnit(0.1f, -0.7f, 0.2f), nxUnit(0.6f, 0.1f, -0.75f));
	nxRotationCase(find, 3, nxUnit(0.36f, 0.48f, 0.8f), nxUnit(0.36f, 0.482f, 0.8f));
	nxRotationCase(find, 4, nxUnit(-0.9f, 0.3f, 0.1f), nxUnit(0.5f, -0.5f, -0.7f));
	// The parallel arm (|e| > 1 - 1e-6): each helper axis, the same and
	// opposite directions, and a pair 1e-4 rad apart.
	nxRotationCase(find, 10, nxUnit(0.1f, 0.7f, 0.7f), nxUnit(0.1f, 0.7f, 0.7f));
	nxRotationCase(find, 11, nxUnit(0.3f, 0.9f, 0.1f), nxUnit(0.3f, 0.9f, 0.1f));
	nxRotationCase(find, 12, nxUnit(0.8f, 0.1f, 0.59f), nxUnit(0.8f, 0.1f, 0.59f));
	nxRotationCase(find, 13, nxUnit(0.8f, 0.59f, 0.1f), nxUnit(0.8f, 0.59f, 0.1f));
	nxRotationCase(find, 14, nxUnit(0.1f, 0.7f, 0.7f), nxUnit(-0.1f, -0.7f, -0.7f));
	nxRotationCase(find, 15, nxUnit(-0.3f, 0.9f, -0.1f), nxUnit(0.3f, -0.9f, 0.1f));
	nxRotationCase(find, 16, nxUnit(0.36f, 0.48f, 0.8f), nxUnit(0.36f, 0.4801f, 0.8f));
	nxRotationCase(find, 17, nxUnit(-0.8f, 0.1f, -0.59f), nxUnit(-0.8f, 0.1001f, -0.59f));
	}

// ---------------------------------------------------------------------------

int wmain(int argc, wchar_t** argv)
	{
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsJointSlotTests", pairDirectory, &physics);
	if(status)
		return status;
	// Unbuffered, so a fault leaves the lines before it.
	setvbuf(stdout, 0, _IONBF, 0);

	CreatePhysicsSDKFn createSDK =
		reinterpret_cast<CreatePhysicsSDKFn>(GetProcAddress(physics, "NxCreatePhysicsSDK"));
	nxSetGlobalAnchor = reinterpret_cast<JointDescSetGlobalAnchorFn>(
		GetProcAddress(physics, "NxJointDesc_SetGlobalAnchor"));
	nxSetGlobalAxis = reinterpret_cast<JointDescSetGlobalAxisFn>(
		GetProcAddress(physics, "NxJointDesc_SetGlobalAxis"));
	printf("export=NxCreatePhysicsSDK present=%s\n", createSDK ? "yes" : "no");
	if(!createSDK || !nxSetGlobalAnchor || !nxSetGlobalAxis)
		{
		FreeLibrary(physics);
		return nxFail("a required export is missing");
		}

	// D6's dump file goes to the working directory: make it the pair's own.
	wchar_t dumpPath[MAX_PATH];
	swprintf(dumpPath, MAX_PATH, L"%s\\D6JointDump.txt", pairDirectory);
	DeleteFileW(dumpPath);
	SetCurrentDirectoryW(pairDirectory);
	printf("control default=%04x step=%04x current=%04x\n", kControlDefault, kControlStep,
		static_cast<unsigned>(nxGetControl()));

	static NxPageGuardedAllocator guardedAllocator;
	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, &guardedAllocator, 0);
	printf("sdk=%s\n", sdk ? "created" : "null");
	if(!sdk)
		{
		FreeLibrary(physics);
		return nxFail("SDK creation failed");
		}

	// The visualization parameters the slot-4 rows read, and a record of the
	// solver parameters the slot 6/7 rows scale by.
	const NxParameter visual[] = { NX_VISUALIZATION_SCALE, NX_VISUALIZE_JOINT_LOCAL_AXES,
		NX_VISUALIZE_JOINT_WORLD_AXES, NX_VISUALIZE_JOINT_LIMITS };
	const NxReal visualValue[] = { 1.5f, 1.0f, 0.75f, 2.0f };
	for(unsigned i = 0; i < 4; i++)
		{
		const bool set = sdk->setParameter(visual[i], visualValue[i]);
		printf("parameter=%u set=%s value=%08x\n", static_cast<unsigned>(visual[i]), set ? "yes" : "no",
			nxU(sdk->getParameter(visual[i])));
		}
	printf("parameter=%u value=%08x\n", static_cast<unsigned>(NX_PENALTY_FORCE), nxU(sdk->getParameter(NX_PENALTY_FORCE)));
	printf("parameter=%u value=%08x\n", static_cast<unsigned>(NX_MIN_SEPARATION_FOR_PENALTY),
		nxU(sdk->getParameter(NX_MIN_SEPARATION_FOR_PENALTY)));

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

	nxAllSlotCases(*scene);
	nxRotationCases();

	sdk->releaseScene(*scene);
	printf("scene=released\n");
	sdk->release();
	printf("sdk=released\n");
	printf("control after=%04x\n", static_cast<unsigned>(nxGetControl()));

	return nxReportPairIdentity(pairDirectory);
	}
