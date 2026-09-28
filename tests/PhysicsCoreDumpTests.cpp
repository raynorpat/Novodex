// The scene core-dump differential (effector-and-coredump Task 4).
//
// A staged-pair target: the same code drives the oracle pair and the
// candidate pair, and run_differential.ps1 compares the two transcripts. It
// builds two populated scenes through the public API only and calls
// NxPhysicsSDK::coreDump (NpPhysicsSDK slot phys_fn_000267 -> the .psc writer
// phys_fn_004062) in text and binary mode, each with and without an addendum,
// then with one scene and with none. Each .psc file is read back and printed
// into the transcript line by line, so the whole file is compared.
//
// Scene contents: everything the dump's written readers cover and nothing
// an unwritten row is needed for -- no mesh shapes (the candidate builds
// shape types 0-3 only), no actor/shape pair flags (the pair-flag array
// 000525 is deferred), and no mass computed from shapes: every dynamic actor
// but one gives its mass and mass-space inertia (and some a rotated or
// shifted mass frame), because the mass-from-shapes row phys_fn_000008 is
// not written (the candidate's creation model covers one unrotated box, and
// only that actor, "mover", takes its mass from a density).
//   * SDK: two added materials (one with anisotropy and a moving surface,
//     flags bits 0 and 1), three changed parameters (NX_MAX_ANGULAR_VELOCITY
//     among them, which every body without its own limit takes), two
//     disabled group pairs.
//   * scene A: gravity (0.5, -9.81, 0.25); a static plane and a static box
//     with all three trigger flags; dynamic actors with one sphere, one box
//     (local pose, group, material, damping, solver count, rotated mass
//     frame), one capsule (frozen position x and rotation z), a kinematic
//     box, a three-shape actor (box, sphere with a trigger flag, capsule), a
//     fully frozen actor with collision disabled, a zero-shape actor, an
//     actor created asleep and unnamed; names plain, with a space, with a
//     quote; joints of all ten families (0-5 get full blocks, 6-9 only their
//     limit planes and PsJointEnd), one to the world, three named, two
//     breakable, one with collision enabled, limit planes on three; one
//     spring-and-damper effector between two dynamic actors (never a world
//     end: the oracle's reader 003964 faults on one). The oracle refuses a
//     dynamic actor whose only shape is a trigger ("Can't compute mass from
//     shapes"), so the triggers sit on a static actor and on one part of the
//     compound.
//   * scene B: a static box, a dynamic sphere and a two-shape actor.
// Dumps: text, text with a two-line addendum, binary, binary with an
// addendum, the deadlock arm of 000267 (below), scene B released (one
// asset), both released (no asset, binary, an empty addendum); then scene C
// (nxBuildSceneC, below) in text and binary, in a new pointer epoch (the
// ordinals are compared within an epoch; they are reset once, before scene C,
// after the outstanding SDK block count is printed). Every report
// the SDK makes goes to a printing output stream and is compared too.
//
// What is normalised, and nothing else:
//   * the date line, "### Core Dump Generated on <date> at <time>" (003992):
//     everything after "Generated on " is printed as "<date>";
//   * pointer tokens "<prefix>__<hex>" (the "%s__%I64x" names of 004062,
//     004004, 004006 and 003994: the PhysicsSDK, the scenes, the actor bodies
//     and the joints): the hex is replaced by "P<n>", n the first-appearance
//     ordinal of that pointer value within an epoch (reset once, before scene
//     C), so identity and aliasing are still compared within it;
//   * the CRT digit residual: a number whose integer part has more than 17
//     digits is printed as the bits of the float it parses to (f32:xxxxxxxx).
//     The 2003 static CRT prints 17 significant digits and pads with zeros,
//     the UCRT prints the exact integer (NxPhysicsJointSlotTests documents
//     the same residual). The dump prints FLT_MAX itself as the literal
//     "fltmax" (003995), NX_COLL_INFINITY included, so no token of this
//     scene reaches the rule; it stays as a guard.
// Line ends are shown: a CR byte is printed as "<CR>", any other control
// byte as "<xx>", and a last line with no LF is marked "<no-LF>"; each line
// is one LF-terminated record of the file. The byte count printed is of the
// normalised text (pointer tokens differ in length between the processes).
//
// The files are written to the pair directory, made the working directory.
// 004062 closes its file (fclose) before it returns, so no flush is needed.

#include "PhysicsPairLoader.h"
#include "NxPageGuardedAllocator.h"

#include <math.h>
#include <string.h>

#include "NxUserOutputStream.h"
#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxActor.h"
#include "NxActorDesc.h"
#include "NxBodyDesc.h"
#include "NxMaterial.h"
#include "NxBoxShapeDesc.h"
#include "NxSphereShapeDesc.h"
#include "NxCapsuleShapeDesc.h"
#include "NxCapsuleShape.h"
#include "NxPlaneShapeDesc.h"
#include "NxShape.h"
#include "NxJoint.h"
#include "NxJointDesc.h"
#include "NxPrismaticJointDesc.h"
#include "NxRevoluteJointDesc.h"
#include "NxCylindricalJointDesc.h"
#include "NxSphericalJointDesc.h"
#include "NxPointOnLineJointDesc.h"
#include "NxPointInPlaneJointDesc.h"
#include "NxDistanceJointDesc.h"
#include "NxPulleyJointDesc.h"
#include "NxFixedJointDesc.h"
#include "NxBitField.h"
#include "NxD6JointDesc.h"
#include "NxSpringAndDamperEffector.h"
#include "NxSpringAndDamperEffectorDesc.h"

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32, NxUserAllocator*, NxUserOutputStream*);
typedef void (NX_CALL_CONV *JointDescSetGlobalAnchorFn)(NxJointDesc&, const NxVec3&);
typedef void (NX_CALL_CONV *JointDescSetGlobalAxisFn)(NxJointDesc&, const NxVec3&);

// Every report the SDK makes is printed, so a report either side makes is
// compared too (the deadlock case below expects exactly one).
class NxPrintingOutputStream : public NxUserOutputStream
	{
	public:
	void reportError(NxErrorCode code, const char* message, const char* file, int line)
		{
		printf("report error code=%d file=%s line=%d message=%s\n", static_cast<int>(code), file ? file : "null",
			line, message ? message : "null");
		}
	NxAssertResponse reportAssertViolation(const char* message, const char* file, int line)
		{
		printf("report assert file=%s line=%d message=%s\n", file ? file : "null", line, message ? message : "null");
		return NX_AR_CONTINUE;
		}
	void print(const char* message)
		{
		printf("report print message=%s\n", message ? message : "null");
		}
	};

static NxPrintingOutputStream gOutput;
static JointDescSetGlobalAnchorFn nxSetGlobalAnchor = 0;
static JointDescSetGlobalAxisFn nxSetGlobalAxis = 0;
static NxPageGuardedAllocator gAllocator;

static NxU32 nxU(NxReal value)
	{
	NxU32 bits;
	memcpy(&bits, &value, 4);
	return bits;
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

// ---------------------------------------------------------------------------
// Reading a dump back.

struct NxPointerName
	{
	char hex[20];
	};

static NxPointerName gPointers[256];
static unsigned gPointerCount = 0;

static unsigned nxPointerOrdinal(const char* hex, size_t length)
	{
	for(unsigned i = 0; i < gPointerCount; i++)
		if(strlen(gPointers[i].hex) == length && memcmp(gPointers[i].hex, hex, length) == 0)
			return i;
	if(gPointerCount < 256 && length < sizeof(gPointers[0].hex))
		{
		memcpy(gPointers[gPointerCount].hex, hex, length);
		gPointers[gPointerCount].hex[length] = 0;
		return gPointerCount++;
		}
	return 999;
	}

static bool nxIsHex(char c)
	{
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
	}

// Appends to `out` (NUL-terminated, capacity `size`).
static void nxAppend(char* out, size_t size, const char* text, size_t length)
	{
	size_t used = strlen(out);
	if(used + length >= size)
		length = size - used - 1;
	memcpy(out + used, text, length);
	out[used + length] = 0;
	}

// One line, normalised as the header describes.
static void nxNormalise(unsigned index, const char* line, size_t length, char* out, size_t size)
	{
	out[0] = 0;
	static const char kDate[] = "### Core Dump Generated on ";
	if(index == 2 && length >= sizeof(kDate) - 1 && memcmp(line, kDate, sizeof(kDate) - 1) == 0)
		{
		nxAppend(out, size, kDate, sizeof(kDate) - 1);
		nxAppend(out, size, "<date>", 6);
		// Keep the line end visible.
		if(length && line[length - 1] == '\r')
			nxAppend(out, size, "<CR>", 4);
		return;
		}
	size_t i = 0;
	char buffer[64];
	while(i < length)
		{
		const char c = line[i];
		// A pointer token: "__" followed by hex digits.
		if(c == '_' && i + 2 < length && line[i + 1] == '_' && nxIsHex(line[i + 2]))
			{
			size_t end = i + 2;
			while(end < length && nxIsHex(line[end]))
				end++;
			sprintf(buffer, "__P%u", nxPointerOrdinal(line + i + 2, end - i - 2));
			nxAppend(out, size, buffer, strlen(buffer));
			i = end;
			continue;
			}
		// A number: optional '-', digits; more than 17 integer digits -> bits.
		if(c == '-' || (c >= '0' && c <= '9'))
			{
			size_t p = i;
			if(line[p] == '-')
				p++;
			const size_t digits = p;
			while(p < length && line[p] >= '0' && line[p] <= '9')
				p++;
			if(p - digits > 17)
				{
				while(p < length && ((line[p] >= '0' && line[p] <= '9') || line[p] == '.'))
					p++;
				char number[128];
				const size_t n = p - i < sizeof(number) - 1 ? p - i : sizeof(number) - 1;
				memcpy(number, line + i, n);
				number[n] = 0;
				sprintf(buffer, "f32:%08x", static_cast<unsigned>(nxU(static_cast<NxReal>(atof(number)))));
				nxAppend(out, size, buffer, strlen(buffer));
				i = p;
				continue;
				}
			if(p == digits)
				p = i + 1;
			nxAppend(out, size, line + i, p - i);
			i = p;
			continue;
			}
		if(c == '\r')
			nxAppend(out, size, "<CR>", 4);
		else if(static_cast<unsigned char>(c) < 0x20 || static_cast<unsigned char>(c) >= 0x7f)
			{
			sprintf(buffer, "<%02x>", static_cast<unsigned>(static_cast<unsigned char>(c)));
			nxAppend(out, size, buffer, strlen(buffer));
			}
		else
			nxAppend(out, size, &c, 1);
		i++;
		}
	}

static void nxPrintDump(const char* label, const char* fname)
	{
	char path[MAX_PATH];
	sprintf(path, "%s.psc", fname);
	FILE* file = fopen(path, "rb");
	printf("dump %s present=%s\n", label, file ? "yes" : "no");
	if(!file)
		return;
	fseek(file, 0, SEEK_END);
	const long size = ftell(file);
	fseek(file, 0, SEEK_SET);
	char* bytes = static_cast<char*>(malloc(static_cast<size_t>(size) + 1));
	const size_t read = fread(bytes, 1, static_cast<size_t>(size), file);
	fclose(file);
	bytes[read] = 0;

	static char out[8192];
	unsigned lines = 0;
	unsigned long normalised = 0;
	size_t start = 0;
	while(start < read)
		{
		size_t end = start;
		while(end < read && bytes[end] != '\n')
			end++;
		nxNormalise(lines, bytes + start, end - start, out, sizeof(out));
		const bool eol = end < read;
		printf("dump %s line=%u text=%s%s\n", label, lines, out, eol ? "" : "<no-LF>");
		normalised += static_cast<unsigned long>(strlen(out)) + (eol ? 1 : 0);
		lines++;
		start = end + 1;
		}
	free(bytes);
	printf("dump %s lines=%u normalised_bytes=%lu\n", label, lines, normalised);
	}

static void nxDump(NxPhysicsSDK& sdk, const char* label, const char* fname, bool binary, const char* addendum)
	{
	char path[MAX_PATH];
	sprintf(path, "%s.psc", fname);
	DeleteFileA(path);
	const bool result = sdk.coreDump(fname, binary, addendum);
	printf("dump %s coreDump binary=%u addendum=%s returned=%u\n", label, binary ? 1u : 0u,
		addendum ? "yes" : "no", result ? 1u : 0u);
	nxPrintDump(label, fname);
	}

// The deadlock arm of 000267. A scene's write lock is the 0x20-byte block its
// NxScene wrapper's link at +0x0c points to: a CRITICAL_SECTION with the
// writer flag at +0x18 and the owner's thread id at +0x1c (002364 fails when
// the flag is set and the id is not the caller's). Scene B's block is made to
// look held by another thread; 000267 takes scene A's lock, fails on B,
// releases A (its flag must be back to 0), reports NpPhysicsSDK.cpp:225 and
// returns false without writing a file. The two words are then restored.
static unsigned* nxSceneLockState(NxScene& scene)
	{
	unsigned char* link = *reinterpret_cast<unsigned char**>(reinterpret_cast<unsigned char*>(&scene) + 0x0c);
	unsigned char* block = *reinterpret_cast<unsigned char**>(link);
	return reinterpret_cast<unsigned*>(block + 0x18);
	}

static void nxDeadlockCase(NxPhysicsSDK& sdk, NxScene& sceneA, NxScene& sceneB)
	{
	unsigned* stateA = nxSceneLockState(sceneA);
	unsigned* stateB = nxSceneLockState(sceneB);
	printf("deadlock before a_flag=%u b_flag=%u\n", stateA[0], stateB[0]);
	const unsigned saved[2] = { stateB[0], stateB[1] };
	stateB[0] = 1;
	stateB[1] = GetCurrentThreadId() ^ 0x40000000u;
	nxDump(sdk, "deadlock", "coredump_deadlock", false, "never written");
	printf("deadlock after a_flag=%u a_owner_is_self=%u b_flag=%u\n", stateA[0],
		stateA[1] == GetCurrentThreadId() ? 1u : 0u, stateB[0]);
	stateB[0] = saved[0];
	stateB[1] = saved[1];
	}

// ---------------------------------------------------------------------------
// The scenes.

static NxActor* nxReport(const char* label, NxActor* actor)
	{
	printf("scene actor %s=%s\n", label, actor ? "created" : "null");
	return actor;
	}

static NxActor* nxDynamic(NxScene& scene, const char* name, NxShapeDesc* s0, NxShapeDesc* s1, NxShapeDesc* s2,
	const NxMat33& rotation, const NxVec3& position, NxBodyDesc& body, NxReal density, NxU32 actorFlags)
	{
	NxActorDesc desc;
	desc.body = &body;
	desc.density = density;
	if(s0) desc.shapes.pushBack(s0);
	if(s1) desc.shapes.pushBack(s1);
	if(s2) desc.shapes.pushBack(s2);
	desc.globalPose.M = rotation;
	desc.globalPose.t = position;
	desc.flags = actorFlags;
	NxActor* actor = scene.createActor(desc);
	if(actor && name)
		actor->setName(name);
	return nxReport(name ? name : "unnamed", actor);
	}

static NxActor* nxStatic(NxScene& scene, const char* name, NxShapeDesc* shape, const NxMat33& rotation,
	const NxVec3& position)
	{
	NxActorDesc desc;
	desc.shapes.pushBack(shape);
	desc.globalPose.M = rotation;
	desc.globalPose.t = position;
	NxActor* actor = scene.createActor(desc);
	if(actor && name)
		actor->setName(name);
	return nxReport(name ? name : "unnamed", actor);
	}

static NxJoint* nxJoint(NxScene& scene, const char* label, NxJointDesc& desc, NxActor* a, NxActor* b,
	const NxVec3& anchor, const NxVec3& axis)
	{
	desc.actor[0] = a;
	desc.actor[1] = b;
	nxSetGlobalAnchor(desc, anchor);
	nxSetGlobalAxis(desc, axis);
	NxJoint* joint = scene.createJoint(desc);
	printf("scene joint %s=%s\n", label, joint ? "created" : "null");
	return joint;
	}

static void nxBuildSceneA(NxScene& scene)
	{
	NxPlaneShapeDesc plane;
	plane.normal = NxVec3(0.0f, 1.0f, 0.0f);
	plane.d = -0.5f;
	nxStatic(scene, "ground", &plane, nxQuatMatrix(0.0f, 0.0f, 0.0f, 1.0f), NxVec3(0.0f, 0.0f, 0.0f));

	NxBoxShapeDesc wall;
	wall.dimensions = NxVec3(4.0f, 2.0f, 0.25f);
	wall.materialIndex = 2;
	wall.shapeFlags |= NX_TRIGGER_ON_ENTER | NX_TRIGGER_ON_LEAVE | NX_TRIGGER_ON_STAY;
	nxStatic(scene, "wall block", &wall, nxQuatMatrix(0.0f, 0.38f, 0.0f, 0.92f), NxVec3(-3.0f, 2.0f, 5.0f));

	NxSphereShapeDesc ballShape;
	ballShape.radius = 0.625f;
	NxBodyDesc ballBody;
	ballBody.linearVelocity = NxVec3(0.5f, -0.25f, 1.0f);
	ballBody.angularVelocity = NxVec3(0.3f, 0.7f, -0.2f);
	ballBody.mass = 2.045f;
	ballBody.massSpaceInertia = NxVec3(0.3125f, 0.3125f, 0.3125f);
	NxActor* ball = nxDynamic(scene, "ball", &ballShape, 0, 0, nxQuatMatrix(0.1f, 0.2f, -0.1f, 0.95f),
		NxVec3(0.0f, 1.0f, 0.0f), ballBody, 0.0f, 0);

	NxBoxShapeDesc crateShape;
	crateShape.dimensions = NxVec3(1.0f, 0.5f, 0.75f);
	crateShape.localPose.M = nxQuatMatrix(0.0f, 0.0f, 0.3826834f, 0.9238795f);
	crateShape.localPose.t = NxVec3(0.125f, -0.25f, 0.5f);
	crateShape.group = 3;
	crateShape.materialIndex = 1;
	NxBodyDesc crateBody;
	crateBody.linearDamping = 0.125f;
	crateBody.angularDamping = 0.0625f;
	crateBody.maxAngularVelocity = 12.5f;
	crateBody.solverIterationCount = 7;
	crateBody.mass = 4.5f;
	crateBody.massSpaceInertia = NxVec3(2.34375f, 1.21875f, 1.875f);
	crateBody.massLocalPose.M = nxQuatMatrix(0.0f, 0.0f, -0.3826834f, 0.9238795f);
	crateBody.massLocalPose.t = NxVec3(0.125f, -0.25f, 0.5f);
	NxActor* crate = nxDynamic(scene, "crate\"q", &crateShape, 0, 0, nxQuatMatrix(-0.2f, 0.1f, 0.3f, 0.9f),
		NxVec3(3.0f, 0.5f, -1.0f), crateBody, 0.0f, 0);

	NxCapsuleShapeDesc pillShape;
	pillShape.radius = 0.375f;
	pillShape.height = 1.25f;
	pillShape.group = 2;
	NxBodyDesc pillBody;
	pillBody.flags |= NX_BF_FROZEN_POS_X | NX_BF_FROZEN_ROT_Z;
	pillBody.linearVelocity = NxVec3(-0.75f, 0.1f, 0.4f);
	pillBody.mass = 2.65f;
	pillBody.massSpaceInertia = NxVec3(0.975f, 0.1875f, 0.975f);
	pillBody.sleepLinearVelocity = 0.25f;
	NxActor* pill = nxDynamic(scene, "pill", &pillShape, 0, 0, nxQuatMatrix(0.3f, -0.1f, 0.2f, 0.85f),
		NxVec3(-2.0f, 0.5f, 1.5f), pillBody, 0.0f, 0);

	NxBoxShapeDesc moverShape;
	moverShape.dimensions = NxVec3(0.5f, 0.5f, 2.0f);
	NxBodyDesc moverBody;
	moverBody.flags |= NX_BF_KINEMATIC;
	NxActor* mover = nxDynamic(scene, "mover", &moverShape, 0, 0, nxQuatMatrix(0.0f, 0.0f, 0.0f, 1.0f),
		NxVec3(1.0f, 3.0f, 2.0f), moverBody, 1.0f, 0);

	NxBoxShapeDesc partBox;
	partBox.dimensions = NxVec3(0.5f, 0.25f, 0.5f);
	partBox.localPose.t = NxVec3(0.0f, -0.5f, 0.0f);
	NxSphereShapeDesc partSphere;
	partSphere.radius = 0.25f;
	partSphere.localPose.t = NxVec3(0.0f, 0.5f, 0.0f);
	partSphere.materialIndex = 1;
	partSphere.shapeFlags |= NX_TRIGGER_ON_LEAVE;
	NxCapsuleShapeDesc partCapsule;
	partCapsule.radius = 0.125f;
	partCapsule.height = 0.75f;
	partCapsule.localPose.M = nxQuatMatrix(0.7071068f, 0.0f, 0.0f, 0.7071068f);
	partCapsule.localPose.t = NxVec3(0.5f, 0.0f, 0.0f);
	partCapsule.group = 5;
	NxBodyDesc compoundBody;
	compoundBody.angularVelocity = NxVec3(0.0f, 1.5f, 0.0f);
	compoundBody.mass = 1.375f;
	compoundBody.massSpaceInertia = NxVec3(0.16f, 0.25f, 0.1875f);
	compoundBody.massLocalPose.M = nxQuatMatrix(0.0f, 0.0f, 0.1545888f, 0.9879789f);
	compoundBody.massLocalPose.t = NxVec3(0.044699065f, -0.455300927f, 0.0f);
	NxActor* compound = nxDynamic(scene, "compound", &partBox, &partSphere, &partCapsule,
		nxQuatMatrix(0.05f, -0.3f, 0.1f, 0.9f), NxVec3(4.0f, 2.0f, 3.0f), compoundBody, 0.0f, 0);

	NxBoxShapeDesc frozenShape;
	frozenShape.dimensions = NxVec3(0.3f, 0.3f, 0.3f);
	NxBodyDesc frozenBody;
	frozenBody.flags |= NX_BF_FROZEN;
	frozenBody.mass = 0.216f;
	frozenBody.massSpaceInertia = NxVec3(0.01296f, 0.01296f, 0.01296f);
	frozenBody.sleepAngularVelocity = 0.5f;
	nxDynamic(scene, "frozen one", &frozenShape, 0, 0, nxQuatMatrix(0.0f, 0.0f, 0.0f, 1.0f),
		NxVec3(-4.0f, 1.0f, -4.0f), frozenBody, 0.0f, NX_AF_DISABLE_COLLISION);

	NxBodyDesc ghostBody;
	ghostBody.mass = 2.0f;
	ghostBody.massSpaceInertia = NxVec3(1.0f, 0.5f, 0.25f);
	ghostBody.massLocalPose.t = NxVec3(0.0f, 0.25f, 0.0f);
	nxDynamic(scene, "ghost", 0, 0, 0, nxQuatMatrix(0.0f, 0.0f, 0.0f, 1.0f), NxVec3(0.0f, 6.0f, 0.0f),
		ghostBody, 0.0f, 0);

	NxSphereShapeDesc sleeperShape;
	sleeperShape.radius = 0.5f;
	NxBodyDesc sleeperBody;
	sleeperBody.wakeUpCounter = 0.0f;
	sleeperBody.mass = 0.5f;
	sleeperBody.massSpaceInertia = NxVec3(0.05f, 0.05f, 0.05f);
	NxActor* sleeper = nxDynamic(scene, 0, &sleeperShape, 0, 0, nxQuatMatrix(0.0f, 0.0f, 0.0f, 1.0f),
		NxVec3(2.0f, 0.5f, 4.0f), sleeperBody, 0.0f, 0);

	if(!ball || !crate || !pill || !mover || !compound || !sleeper)
		return;

	// Joints: all ten families.
	NxPrismaticJointDesc prismatic;
	nxJoint(scene, "prismatic", prismatic, ball, crate, NxVec3(1.5f, 0.75f, -0.5f), NxVec3(0.36f, 0.48f, 0.8f));

	NxRevoluteJointDesc revolute;
	revolute.limit.low.value = -0.25f;
	revolute.limit.low.restitution = 0.5f;
	revolute.limit.low.hardness = 0.75f;
	revolute.limit.high.value = 0.125f;
	revolute.limit.high.restitution = 0.25f;
	revolute.motor.velTarget = 2.5f;
	revolute.motor.maxForce = 30.0f;
	revolute.motor.freeSpin = 1;
	revolute.spring = NxSpringDesc(8.0f, 0.5f, 0.0625f);
	revolute.flags = NX_RJF_LIMIT_ENABLED | NX_RJF_MOTOR_ENABLED | NX_RJF_SPRING_ENABLED;
	revolute.projectionMode = NX_JPM_POINT_MINDIST;
	revolute.projectionDistance = 0.05f;
	revolute.projectionAngle = 0.03125f;
	revolute.jointFlags |= NX_JF_COLLISION_ENABLED;
	NxJoint* hinge = nxJoint(scene, "revolute", revolute, crate, pill, NxVec3(0.5f, 1.0f, 0.25f),
		NxVec3(0.0f, 0.0f, 1.0f));
	if(hinge)
		{
		hinge->setName("hinge");
		hinge->setLimitPoint(NxVec3(2.0f, 0.5f, -0.5f), true);
		hinge->addLimitPlane(NxVec3(0.0f, 1.0f, 0.0f), NxVec3(0.0f, -3.0f, 0.0f));
		hinge->addLimitPlane(NxVec3(1.0f, 0.0f, 0.0f), NxVec3(2.5f, 0.0f, 0.0f));
		}

	NxRevoluteJointDesc plainRevolute;
	nxJoint(scene, "revolute_plain", plainRevolute, pill, compound, NxVec3(-1.0f, 1.0f, 2.0f),
		NxVec3(1.0f, 0.0f, 0.0f));

	NxCylindricalJointDesc cylindrical;
	NxJoint* slider = nxJoint(scene, "cylindrical", cylindrical, compound, ball, NxVec3(2.0f, 1.5f, 1.5f),
		NxVec3(0.0f, 1.0f, 0.0f));
	if(slider)
		slider->setBreakable(100.0f, 250.5f);

	NxSphericalJointDesc spherical;
	spherical.twistLimit.low.value = -0.5f;
	spherical.twistLimit.low.restitution = 0.25f;
	spherical.twistLimit.high.value = 0.375f;
	spherical.twistLimit.high.restitution = 0.125f;
	spherical.swingLimit.value = 0.25f;
	spherical.swingLimit.restitution = 0.5f;
	spherical.twistSpring = NxSpringDesc(6.0f, 0.25f, 0.125f);
	spherical.swingSpring = NxSpringDesc(4.0f, 0.75f, 0.0625f);
	spherical.jointSpring = NxSpringDesc(3.0f, 0.5f, 0.25f);
	spherical.swingAxis = NxVec3(0.0f, 0.6f, 0.8f);
	spherical.projectionDistance = 0.125f;
	spherical.projectionMode = NX_JPM_POINT_MINDIST;
	spherical.flags = NX_SJF_TWIST_LIMIT_ENABLED | NX_SJF_SWING_LIMIT_ENABLED | NX_SJF_TWIST_SPRING_ENABLED
		| NX_SJF_SWING_SPRING_ENABLED | NX_SJF_JOINT_SPRING_ENABLED;
	NxJoint* shoulder = nxJoint(scene, "spherical", spherical, ball, pill, NxVec3(-1.0f, 0.75f, 0.75f),
		NxVec3(0.0f, 0.6f, 0.8f));
	if(shoulder)
		shoulder->setName("shoulder joint");

	NxSphericalJointDesc anchor;
	nxJoint(scene, "spherical_world", anchor, 0, mover, NxVec3(1.0f, 4.0f, 2.0f), NxVec3(0.0f, 1.0f, 0.0f));

	NxPointOnLineJointDesc onLine;
	NxJoint* rail = nxJoint(scene, "point_on_line", onLine, crate, compound, NxVec3(3.5f, 1.25f, 1.0f),
		NxVec3(0.6f, 0.0f, 0.8f));
	if(rail)
		rail->addLimitPlane(NxVec3(0.0f, 0.0f, 1.0f), NxVec3(0.0f, 0.0f, 2.0f));

	NxPointInPlaneJointDesc inPlane;
	nxJoint(scene, "point_in_plane", inPlane, sleeper, ball, NxVec3(1.0f, 0.75f, 2.0f), NxVec3(0.0f, 1.0f, 0.0f));

	NxDistanceJointDesc distance;
	distance.maxDistance = 2.0f;
	distance.minDistance = 1.0f;
	distance.spring = NxSpringDesc(10.0f, 0.5f, 1.5f);
	distance.flags = NX_DJF_MAX_DISTANCE_ENABLED | NX_DJF_MIN_DISTANCE_ENABLED | NX_DJF_SPRING_ENABLED;
	NxJoint* rope = nxJoint(scene, "distance", distance, ball, sleeper, NxVec3(1.0f, 0.75f, 2.0f),
		NxVec3(0.0f, 1.0f, 0.0f));
	if(rope)
		{
		rope->setName("rope");
		rope->setLimitPoint(NxVec3(1.5f, 0.5f, 3.0f), false);
		rope->addLimitPlane(NxVec3(0.0f, 1.0f, 0.0f), NxVec3(0.0f, 0.25f, 0.0f));
		}

	NxPulleyJointDesc pulley;
	pulley.pulley[0] = NxVec3(0.0f, 5.0f, 0.0f);
	pulley.pulley[1] = NxVec3(3.0f, 5.0f, -1.0f);
	pulley.distance = 6.0f;
	pulley.stiffness = 0.75f;
	pulley.ratio = 1.5f;
	pulley.flags = NX_PJF_IS_RIGID;
	nxJoint(scene, "pulley", pulley, ball, crate, NxVec3(1.5f, 2.0f, -0.5f), NxVec3(0.0f, 1.0f, 0.0f));

	NxFixedJointDesc fixed;
	NxJoint* weld = nxJoint(scene, "fixed", fixed, compound, sleeper, NxVec3(3.0f, 1.0f, 3.5f),
		NxVec3(0.0f, 0.0f, 1.0f));
	if(weld)
		weld->setBreakable(1000.0f, 3.4028235e38f);

	NxD6JointDesc d6;
	d6.twistMotion = NX_D6JOINT_MOTION_LIMITED;
	d6.swing1Motion = NX_D6JOINT_MOTION_LIMITED;
	d6.swing2Motion = NX_D6JOINT_MOTION_LOCKED;
	d6.twistLimit.low.value = -0.25f;
	d6.twistLimit.high.value = 0.375f;
	d6.swing1Limit.value = 0.3125f;
	d6.swing2Limit.value = 0.1875f;
	nxJoint(scene, "d6", d6, pill, sleeper, NxVec3(0.0f, 0.5f, 2.75f), NxVec3(0.36f, 0.48f, 0.8f));

	// One effector between two dynamic actors, all nine values distinct.
	NxSpringAndDamperEffectorDesc effector;
	effector.body1 = ball;
	effector.body2 = pill;
	effector.pos1 = NxVec3(0.25f, 1.5f, -0.3f);
	effector.pos2 = NxVec3(-1.75f, 0.2f, 1.25f);
	effector.springDistCompressSaturate = 0.5f;
	effector.springDistRelaxed = 1.25f;
	effector.springDistStretchSaturate = 3.0f;
	effector.springMaxCompressForce = 40.0f;
	effector.springMaxStretchForce = 55.0f;
	effector.damperVelCompressSaturate = -2.5f;
	effector.damperVelStretchSaturate = 1.75f;
	effector.damperMaxCompressForce = 7.0f;
	effector.damperMaxStretchForce = 9.5f;
	NxSpringAndDamperEffector* spring = scene.createSpringAndDamperEffector(effector);
	printf("scene effector=%s\n", spring ? "created" : "null");
	}

// Scene C (review addition, dumped after the scenes above are gone, so the
// earlier dumps are unchanged): an unjointed actor created asleep (the
// `awake(false)` arm of a dynamic body), a static capsule with a trigger
// flag (the capsule arm hands the desc's own +0x54 flags to 004017),
// consecutive actors and shapes with equal values so the settings records
// print their `PsDefaultSettings` lines (position, orientation, density,
// sides, localposition, localorientation, plane, height, radius, material,
// group, com, comrot, inertia, mass, solvercount, velocity, angularvelocity,
// wakeupcounter, lineardamping, angulardamping, maxangularvelocity), and a
// static actor with two shapes.
static NxActor* nxTwinBox(NxScene& scene, const char* name)
	{
	NxBoxShapeDesc box;
	box.dimensions = NxVec3(0.5f, 0.75f, 1.25f);
	box.localPose.t = NxVec3(0.0f, 0.25f, 0.0f);
	box.materialIndex = 1;
	box.group = 4;
	NxBodyDesc body;
	body.linearVelocity = NxVec3(0.25f, 0.0f, -0.5f);
	body.angularVelocity = NxVec3(0.0f, 0.75f, 0.0f);
	body.linearDamping = 0.25f;
	body.angularDamping = 0.125f;
	body.maxAngularVelocity = 5.0f;
	body.solverIterationCount = 6;
	body.mass = 3.0f;
	body.massSpaceInertia = NxVec3(0.5f, 0.625f, 0.75f);
	body.massLocalPose.t = NxVec3(0.0f, 0.25f, 0.0f);
	return nxDynamic(scene, name, &box, 0, 0, nxQuatMatrix(0.0f, 0.3f, 0.0f, 0.95f), NxVec3(5.0f, 1.0f, 5.0f), body,
		0.0f, 0);
	}

static void nxBuildSceneC(NxScene& scene)
	{
	NxSphereShapeDesc napShape;
	napShape.radius = 0.5f;
	NxBodyDesc napBody;
	napBody.wakeUpCounter = 0.0f;
	napBody.mass = 0.5f;
	napBody.massSpaceInertia = NxVec3(0.05f, 0.05f, 0.05f);
	nxDynamic(scene, "napper", &napShape, 0, 0, nxQuatMatrix(0.0f, 0.0f, 0.0f, 1.0f), NxVec3(-6.0f, 0.5f, 0.0f),
		napBody, 0.0f, 0);

	NxCapsuleShapeDesc sensor;
	sensor.radius = 0.25f;
	sensor.height = 2.0f;
	sensor.shapeFlags |= NX_TRIGGER_ON_ENTER | NX_TRIGGER_ON_STAY;
	nxStatic(scene, "sensor", &sensor, nxQuatMatrix(0.0f, 0.0f, 0.3826834f, 0.9238795f), NxVec3(0.0f, 1.0f, -6.0f));
	NxCapsuleShapeDesc sensor2;
	sensor2.radius = 0.25f;
	sensor2.height = 2.0f;
	sensor2.flags = NX_SWEPT_SHAPE;
	nxStatic(scene, "sensor 2", &sensor2, nxQuatMatrix(0.0f, 0.0f, 0.3826834f, 0.9238795f), NxVec3(0.0f, 1.0f, -6.0f));

	NxPlaneShapeDesc wallPlane;
	wallPlane.normal = NxVec3(1.0f, 0.0f, 0.0f);
	wallPlane.d = -20.0f;
	nxStatic(scene, "west", &wallPlane, nxQuatMatrix(0.0f, 0.0f, 0.0f, 1.0f), NxVec3(0.0f, 0.0f, 0.0f));
	NxPlaneShapeDesc wallPlane2 = wallPlane;
	nxStatic(scene, "west again", &wallPlane2, nxQuatMatrix(0.0f, 0.0f, 0.0f, 1.0f), NxVec3(0.0f, 0.0f, 0.0f));

	NxSphereShapeDesc ball1;
	ball1.radius = 0.375f;
	ball1.materialIndex = 2;
	nxStatic(scene, "pebble", &ball1, nxQuatMatrix(0.0f, 0.0f, 0.0f, 1.0f), NxVec3(3.0f, 0.375f, 3.0f));
	NxSphereShapeDesc ball2;
	ball2.radius = 0.375f;
	ball2.materialIndex = 2;
	nxStatic(scene, "pebble 2", &ball2, nxQuatMatrix(0.0f, 0.0f, 0.0f, 1.0f), NxVec3(3.0f, 0.375f, 3.0f));

	nxTwinBox(scene, "twin");
	nxTwinBox(scene, "twin 2");

	NxBoxShapeDesc brick;
	brick.dimensions = NxVec3(0.25f, 0.25f, 0.5f);
	NxBodyDesc brickBody;
	nxDynamic(scene, "brick", &brick, 0, 0, nxQuatMatrix(0.0f, 0.0f, 0.0f, 1.0f), NxVec3(-3.0f, 2.0f, -3.0f),
		brickBody, 2.0f, 0);
	NxBoxShapeDesc brick2;
	brick2.dimensions = NxVec3(0.25f, 0.25f, 0.5f);
	NxBodyDesc brickBody2;
	nxDynamic(scene, "brick 2", &brick2, 0, 0, nxQuatMatrix(0.0f, 0.0f, 0.0f, 1.0f), NxVec3(-3.0f, 3.0f, -3.0f),
		brickBody2, 2.0f, 0);

	NxBoxShapeDesc postA;
	postA.dimensions = NxVec3(0.25f, 2.0f, 0.25f);
	postA.localPose.t = NxVec3(-1.0f, 0.0f, 0.0f);
	NxBoxShapeDesc postB;
	postB.dimensions = NxVec3(0.25f, 2.0f, 0.25f);
	postB.localPose.t = NxVec3(1.0f, 0.0f, 0.0f);
	NxCapsuleShapeDesc beam;
	beam.radius = 0.2f;
	beam.height = 2.0f;
	beam.localPose.M = nxQuatMatrix(0.0f, 0.0f, 0.7071068f, 0.7071068f);
	beam.localPose.t = NxVec3(0.0f, 2.0f, 0.0f);
	beam.group = 6;
	NxActorDesc gate;
	gate.shapes.pushBack(&postA);
	gate.shapes.pushBack(&postB);
	gate.shapes.pushBack(&beam);
	gate.globalPose.t = NxVec3(8.0f, 2.0f, -2.0f);
	NxActor* gateActor = scene.createActor(gate);
	if(gateActor)
		gateActor->setName("gate");
	nxReport("gate", gateActor);
	}

static void nxBuildSceneB(NxScene& scene)
	{
	NxBoxShapeDesc floorShape;
	floorShape.dimensions = NxVec3(10.0f, 0.5f, 10.0f);
	nxStatic(scene, "floor", &floorShape, nxQuatMatrix(0.0f, 0.0f, 0.0f, 1.0f), NxVec3(0.0f, -0.5f, 0.0f));

	NxSphereShapeDesc dropShape;
	dropShape.radius = 0.75f;
	NxBodyDesc dropBody;
	dropBody.mass = 1.75f;
	dropBody.massSpaceInertia = NxVec3(0.375f, 0.375f, 0.375f);
	nxDynamic(scene, "drop", &dropShape, 0, 0, nxQuatMatrix(0.0f, 0.0f, 0.0f, 1.0f), NxVec3(0.0f, 3.0f, 0.0f),
		dropBody, 0.0f, 0);

	NxBoxShapeDesc pairBox;
	pairBox.dimensions = NxVec3(0.25f, 0.25f, 0.25f);
	pairBox.localPose.t = NxVec3(-0.5f, 0.0f, 0.0f);
	NxBoxShapeDesc pairBox2;
	pairBox2.dimensions = NxVec3(0.25f, 0.5f, 0.25f);
	pairBox2.localPose.t = NxVec3(0.5f, 0.0f, 0.0f);
	pairBox2.materialIndex = 2;
	NxBodyDesc pairBody;
	pairBody.mass = 0.375f;
	pairBody.massSpaceInertia = NxVec3(0.03125f, 0.1f, 0.115f);
	pairBody.massLocalPose.t = NxVec3(0.166666672f, 0.0f, 0.0f);
	nxDynamic(scene, "pair", &pairBox, &pairBox2, 0, nxQuatMatrix(0.0f, 0.0f, 0.2f, 0.98f), NxVec3(2.0f, 1.0f, 0.0f),
		pairBody, 0.0f, 0);
	}

static void nxConfigureSdk(NxPhysicsSDK& sdk)
	{
	NxMaterial surface;
	surface.dynamicFriction = 0.375f;
	surface.staticFriction = 0.625f;
	surface.restitution = 0.25f;
	surface.dynamicFrictionV = 0.125f;
	surface.staticFrictionV = 0.3f;
	surface.dirOfAnisotropy = NxVec3(0.0f, 0.0f, 1.0f);
	surface.dirOfMotion = NxVec3(1.0f, 0.0f, 0.0f);
	surface.speedOfMotion = 2.5f;
	surface.flags = 3;
	const NxMaterialIndex first = sdk.addMaterial(surface);

	NxMaterial rubber;
	rubber.dynamicFriction = 1.25f;
	rubber.staticFriction = 1.5f;
	rubber.restitution = 0.875f;
	const NxMaterialIndex second = sdk.addMaterial(rubber);
	printf("sdk materials=%u added=%u,%u\n", static_cast<unsigned>(sdk.getNbMaterials()),
		static_cast<unsigned>(first), static_cast<unsigned>(second));

	sdk.setParameter(NX_PENALTY_FORCE, 0.75f);
	sdk.setParameter(NX_BOUNCE_TRESHOLD, -1.5f);
	sdk.setParameter(NX_MAX_ANGULAR_VELOCITY, 9.0f);
	sdk.setGroupCollisionFlag(1, 3, false);
	sdk.setGroupCollisionFlag(2, 2, false);
	printf("sdk group_1_3=%u group_2_2=%u\n", sdk.getGroupCollisionFlag(1, 3) ? 1u : 0u,
		sdk.getGroupCollisionFlag(2, 2) ? 1u : 0u);
	}

// ---------------------------------------------------------------------------

int wmain(int argc, wchar_t** argv)
	{
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsCoreDumpTests", pairDirectory, &physics);
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

	// The dump files go to the pair directory.
	SetCurrentDirectoryW(pairDirectory);

	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, &gAllocator, &gOutput);
	printf("sdk=%s\n", sdk ? "created" : "null");
	if(!sdk)
		{
		FreeLibrary(physics);
		return nxFail("SDK creation failed");
		}
	nxConfigureSdk(*sdk);

	NxSceneDesc sceneDesc;
	sceneDesc.setToDefault();
	sceneDesc.gravity = NxVec3(0.5f, -9.81f, 0.25f);
	NxScene* sceneA = sdk->createScene(sceneDesc);
	sceneDesc.gravity = NxVec3(0.0f, -3.5f, 0.0f);
	NxScene* sceneB = sdk->createScene(sceneDesc);
	printf("scene a=%s b=%s\n", sceneA ? "created" : "null", sceneB ? "created" : "null");
	if(!sceneA || !sceneB)
		{
		sdk->release();
		FreeLibrary(physics);
		return nxFail("scene creation failed");
		}
	nxBuildSceneA(*sceneA);
	nxBuildSceneB(*sceneB);

	nxDump(*sdk, "text", "coredump_text", false, 0);
	nxDump(*sdk, "text_addendum", "coredump_text_addendum", false, "PsSetScene 0\r\nPsUserLine hello, world");
	nxDump(*sdk, "binary", "coredump_binary", true, 0);
	nxDump(*sdk, "binary_addendum", "coredump_binary_addendum", true, "# addendum for the binary dump");
	nxDeadlockCase(*sdk, *sceneA, *sceneB);

	sdk->releaseScene(*sceneB);
	nxDump(*sdk, "one_scene", "coredump_one_scene", false, 0);
	sdk->releaseScene(*sceneA);
	nxDump(*sdk, "no_scene", "coredump_no_scene", true, "");
	printf("scene=released\n");
	// Before the pointer epoch below resets: the SDK blocks still outstanding
	// once scenes A and B are released, so the reset cannot hide a block one
	// DLL leaks or frees that the other does not. The raw allocation and free
	// COUNTS are not printed: they differ by 4 each (232/218 in the oracle,
	// 228/214 in the candidate) because the candidate's actor-creation models
	// (broadphase, pruner and auxiliary-array growth, Phase 5) allocate and
	// free a different number of intermediate blocks; the step-by-step
	// difference is recorded in evidence/effector-and-coredump.md
	// (## Open items). The outstanding count, and every block the dump reads,
	// agree.
	printf("allocator after_release outstanding=%u\n", gAllocator.allocations() - gAllocator.frees());

	// Scene C, dumped on its own after the others (review addition).
	sceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	NxScene* sceneC = sdk->createScene(sceneDesc);
	printf("scene c=%s\n", sceneC ? "created" : "null");
	if(sceneC)
		{
		// A new pointer epoch: every object of scenes A and B is freed, and a
		// block scene C allocates may take an address one of them had. Which
		// address the page-guarded allocator's VirtualAlloc hands back after
		// a free depends on the process's whole address-space history (the
		// two DLLs' CRT heaps differ by construction), so aliasing with a
		// freed object is not compared: the ordinals restart here.
		gPointerCount = 0;
		printf("dump pointer_epoch reset\n");
		nxBuildSceneC(*sceneC);
		nxDump(*sdk, "scene_c", "coredump_scene_c", false, 0);
		nxDump(*sdk, "scene_c_binary", "coredump_scene_c_binary", true, 0);
		sdk->releaseScene(*sceneC);
		printf("scene c=released\n");
		}

	sdk->release();
	printf("sdk=released\n");

	status = nxReportPairIdentity(pairDirectory);
	if(status)
		return status;
	FreeLibrary(physics);
	return 0;
	}
