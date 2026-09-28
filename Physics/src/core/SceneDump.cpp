/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/SceneDump.h"
#include "core/Joint.h"
#include "core/PrismaticJoint.h"
#include "core/RevoluteJoint.h"
#include "core/CylindricalJoint.h"
#include "core/SphericalJoint.h"
#include "core/PointOnLineJoint.h"
#include "core/PointInPlaneJoint.h"
#include "core/SpringAndDamperEffector.h"
#include "PhysicsSDK.h"
#include "NpActor.h"
#include "Scene.h"
#include "TriangleMesh.h"
#include "NarrowPhase.h"
#include "ContactGeneration.h"
#include "X87Sqrt.h"
#include "NxJoint.h"
#include "NxPrismaticJointDesc.h"
#include "NxRevoluteJointDesc.h"
#include "NxCylindricalJointDesc.h"
#include "NxSphericalJointDesc.h"
#include "NxPointOnLineJointDesc.h"
#include "NxPointInPlaneJointDesc.h"
#include "NxActorDesc.h"
#include "NxBodyDesc.h"
#include "NxScene.h"
#include "NxPlaneShapeDesc.h"
#include "NxSphereShapeDesc.h"
#include "NxBoxShapeDesc.h"
#include "NxCapsuleShapeDesc.h"
#include "NxTriangleMeshShapeDesc.h"
#include "NxTriangleMeshShape.h"
#include "NxSimpleTriangleMesh.h"
#include "NxTriangleMeshDesc.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cfloat>
#include <cmath>
#include <ctime>
#include <new>

// The scene core-dump writer (effector-and-coredump Task 3a: the rows
// through the joint blocks and the entry; Task 3b: the asset writer, the
// actors, shapes, meshes and effectors). The contract is
// units/effector-coredump-contract.md "## Core dump". The readers the asset
// writer calls are written with their own units: the actor body's shape
// readers 000015/000017 in core/JointSupport.cpp, the Scene's 000509/000523
// in Scene.cpp (000525 deferred there), the shape type 001283 in
// ContactGeneration.cpp; 001472, the convex mesh's polygon builder, is a
// deferred stub below.
//
// Output goes through the static CRT exactly where the oracle calls it:
// fopen (phys_fn_005671, `_fsopen(name, "wb", _SH_DENYNO)`), fprintf
// (005716) for every line, sprintf (005739) for names and tokens, strstr
// (005732) in the float token, fclose (005673); strftime (005794) and the
// time rows (005766, 005775, 005802) for the header date. No fputs, no fwrite.
// NxPhysics.dll links legacy_stdio_float_rounding.obj, so "%f"/"%.9f" round
// as the oracle's CRT does.
//
// Floating point: the 3a rows do no x87 arithmetic of their own -- they
// compare floats (fucompp), widen them for sprintf (fld dword; fstp qword)
// and round three setting kinds with fistp at the live control word -- but
// the asset and shape writers convert poses to quaternions with fsqrt
// (sceneDumpQuat below), so this translation unit is on the /arch:IA32 list
// with the joint files, and follows their convention: a value the listing
// keeps on the x87 stack is a `double`, one it stores is an `NxReal`. It is also built /EHs-c-: the joint block keeps descriptors with
// virtual destructors on its stack, and the oracle's frame for it has no
// unwind state.

// The descriptor offsets the listing reads (the frame offsets of 004037's
// descriptor, which starts at esp+0x14, and 004007's reads through `desc`).
static_assert(offsetof(NxJointDesc, localNormal) == 0x10, "localNormal at desc+0x10");
static_assert(offsetof(NxJointDesc, localAxis) == 0x28, "localAxis at desc+0x28");
static_assert(offsetof(NxJointDesc, localAnchor) == 0x40, "localAnchor at desc+0x40");
static_assert(offsetof(NxJointDesc, maxForce) == 0x58, "maxForce at desc+0x58");
static_assert(offsetof(NxJointDesc, maxTorque) == 0x5c, "maxTorque at desc+0x5c");
static_assert(offsetof(NxJointDesc, jointFlags) == 0x68, "jointFlags at desc+0x68");
static_assert(offsetof(NxRevoluteJointDesc, limit) == 0x6c, "limit at desc+0x6c");
static_assert(offsetof(NxRevoluteJointDesc, motor) == 0x84, "motor at desc+0x84");
static_assert(offsetof(NxRevoluteJointDesc, spring) == 0x90, "spring at desc+0x90");
static_assert(offsetof(NxRevoluteJointDesc, flags) == 0xa4, "flags at desc+0xa4");
static_assert(offsetof(NxSphericalJointDesc, projectionDistance) == 0x78, "projectionDistance at desc+0x78");
static_assert(offsetof(NxSphericalJointDesc, twistLimit) == 0x7c, "twistLimit at desc+0x7c");
static_assert(offsetof(NxSphericalJointDesc, swingLimit) == 0x94, "swingLimit at desc+0x94");
static_assert(offsetof(NxSphericalJointDesc, twistSpring) == 0xa0, "twistSpring at desc+0xa0");
static_assert(offsetof(NxSphericalJointDesc, swingSpring) == 0xac, "swingSpring at desc+0xac");
static_assert(offsetof(NxSphericalJointDesc, jointSpring) == 0xb8, "jointSpring at desc+0xb8");
static_assert(offsetof(NxSphericalJointDesc, flags) == 0xc4, "flags at desc+0xc4");
static_assert(offsetof(NxSphericalJointDesc, projectionMode) == 0xc8, "projectionMode at desc+0xc8");
static_assert(sizeof(NxMaterial) == 0x48, "the material block strides by 0x48 (0x9526c)");
static_assert(offsetof(NxMaterial, speedOfMotion) == 0x34, "speedOfMotion at +0x34");
static_assert(offsetof(NxMaterial, flags) == 0x38, "flags at +0x38");
// The actor and body descriptors 004055 builds (actor at esp+0x58, body at
// esp+0xa8 of 004051's frame) and the words it reads back.
static_assert(offsetof(NxActorDescBase, globalPose) == 0x00, "globalPose at +0x00");
static_assert(offsetof(NxActorDescBase, density) == 0x34, "density at +0x34 (esp+0x8c)");
static_assert(offsetof(NxActorDescBase, flags) == 0x38, "flags at +0x38 (esp+0x90)");
static_assert(offsetof(NxBodyDesc, massSpaceInertia) == 0x30, "massSpaceInertia at +0x30");
static_assert(offsetof(NxBodyDesc, mass) == 0x3c, "mass at +0x3c");
static_assert(offsetof(NxBodyDesc, linearVelocity) == 0x40, "linearVelocity at +0x40");
static_assert(offsetof(NxBodyDesc, angularVelocity) == 0x4c, "angularVelocity at +0x4c");
static_assert(offsetof(NxBodyDesc, wakeUpCounter) == 0x58, "wakeUpCounter at +0x58");
static_assert(offsetof(NxBodyDesc, maxAngularVelocity) == 0x64, "maxAngularVelocity at +0x64");
static_assert(offsetof(NxBodyDesc, flags) == 0x68, "flags at +0x68 (esp+0x110)");
static_assert(offsetof(NxBodyDesc, solverIterationCount) == 0x74, "solverIterationCount at +0x74 (esp+0x11c)");
// The shape descriptors 004048 builds. NX_SHAPE_DESC_LIST (Nxp.h) puts
// NxShapeDesc::next at +0x48, so every family's own fields start at +0x4c.
static_assert(offsetof(NxShapeDesc, localPose) == 0x08, "localPose at +0x08");
static_assert(offsetof(NxShapeDesc, shapeFlags) == 0x38, "shapeFlags at +0x38");
static_assert(offsetof(NxShapeDesc, group) == 0x3c, "group at +0x3c");
static_assert(offsetof(NxShapeDesc, materialIndex) == 0x3e, "materialIndex at +0x3e");
static_assert(offsetof(NxPlaneShapeDesc, normal) == 0x4c, "normal at +0x4c");
static_assert(offsetof(NxPlaneShapeDesc, d) == 0x58, "d at +0x58");
static_assert(offsetof(NxSphereShapeDesc, radius) == 0x4c, "radius at +0x4c");
static_assert(offsetof(NxBoxShapeDesc, dimensions) == 0x4c, "dimensions at +0x4c");
static_assert(offsetof(NxCapsuleShapeDesc, radius) == 0x4c, "radius at +0x4c");
static_assert(offsetof(NxCapsuleShapeDesc, height) == 0x50, "height at +0x50");
static_assert(offsetof(NxCapsuleShapeDesc, flags) == 0x54, "flags at +0x54");
static_assert(offsetof(NxTriangleMeshShapeDesc, meshData) == 0x4c, "meshData at +0x4c");
static_assert(offsetof(NxTriangleMeshShapeDesc, meshFlags) == 0x50, "meshFlags at +0x50");
static_assert(offsetof(NxTriangleMeshDesc, flags) == 0x18, "flags at +0x18");
static_assert(sizeof(NxMat33) == 0x24, "a row-major 3x3 of floats");
static_assert(sizeof(NxPairFlag) == 0x0c, "pair records are 0x0c bytes");

// .data 0x10126678: the mesh-name buffer phys_fn_003991 returns.
static char gSceneDumpMeshName[0x200];

// .data 0x10126878: the "%s__%I64x" name buffer (phys_fn_003994, 004004,
// 004006, 004062).
static char gSceneDumpName[0x100];

// .data 0x10126978 / 0x10126d78: the float-token ring phys_fn_003995 fills,
// 16 buffers of 64 bytes, and its index. One fprintf can therefore hold at
// most 16 tokens.
static char gSceneDumpTokens[16][0x40];
static int gSceneDumpTokenIndex = 0;

// .data 0x10126d80: the joint-name buffer phys_fn_004004 returns.
static char gSceneDumpJointName[0x200];

// .data 0x10126f80: the limit/spring/motor text buffer of phys_fn_004009,
// 004011 and 004013.
static char gSceneDumpText[0x100];

// The pointers in names are pushed as 64-bit values sign-extended from 32
// bits (`cdq` before the two pushes, e.g. 0x90e1a, 0x951e3).
static NX_INLINE __int64 sceneDumpKey(const void* pointer)
	{
	return (__int64)(int)(size_t)pointer;
	}

static NX_INLINE NxU32 sceneDumpBits(NxReal value)
	{
	NxU32 bits;
	memcpy(&bits, &value, 4);
	return bits;
	}

// `fld dword; fistp qword` at the live control word, the low word kept: the
// material index, solver count and group kinds of phys_fn_003997/003999
// (0x90135, 0x90432, 0x90455; 0x9080c, 0x90ae6, 0x90b0b). Not a C cast,
// which would truncate.
static int sceneDumpRound(NxReal value)
	{
#if defined(_MSC_VER) && defined(_M_IX86)
	__int64 result;
	__asm
		{
		fld		value
		fistp	result
		}
	return (int)result;
#else
	return (int)(__int64)nearbyint((double)value);
#endif
	}

// phys_fn_003981 (0x0008fb00, 254 B)
// Compiler-generated: NxJointDesc::isValid (Physics/include/NxJointDesc.h),
// slot 2 of NxJointDesc's own table 0x10117a44 [004025 003985 003981]. MSVC
// emits it from the public header wherever a joint descriptor is constructed;
// here that is writeJoint (phys_fn_004037) below. No hand-written body.

// phys_fn_003983 (0x0008fc00, 74 B)
// Compiler-generated: NxCapsuleShapeDesc::setToDefault
// (Physics/include/NxCapsuleShapeDesc.h), slot 1 of the capsule descriptor
// table 0x10118274 [004031 003983 004019] writeShape (phys_fn_004048)
// installs. No hand-written body.

// phys_fn_003985 (0x0008fc50, 92 B)
// Compiler-generated: NxJointDesc::setToDefault (NxJointDesc.h), slot 1 of
// 0x10117a44 and of the revolute, spherical, cylindrical, prismatic,
// point-on-line and point-in-plane descriptor tables writeJoint installs
// (0x101182a4, 0x101182c0, 0x10118280, 0x101182b4, 0x10118298, 0x1011828c).
// Emitted from the header as 003981 is.

// phys_fn_003987 (0x0008fcb0, 68 B)
// Compiler-generated: NxSphereShapeDesc::setToDefault (NxSphereShapeDesc.h),
// slot 1 of the sphere descriptor table 0x101182cc [004031 003987 004029].

// phys_fn_003989 (0x0008fd00, 71 B)
// Compiler-generated: NxTriangleMeshShapeDesc::setToDefault
// (NxTriangleMeshShapeDesc.h), slot 1 of the mesh descriptor table
// 0x101182d8 [004031 003989 004033] writeShape and writeMesh install.

// phys_fn_003991 (0x0008fd50, 144 B)
// The search compares the table's pointers in order and names a hit by its
// position plus one. A new mesh is appended and counted first, then named by
// the new count; a count that reaches 0x8000 is held at 0x7fff, so the
// 0x8000-pointer block is never overrun and every later mesh overwrites the
// last slot.
const char* SceneDumpNames::meshName(const void* mesh, bool* isNew)
	{
	for(NxU32 i = 0; i < mMeshCount; i++)
		{
		if(mMeshes[i] == mesh)
			{
			*isNew = false;
			sprintf(gSceneDumpMeshName, "tmesh%d", i + 1);
			return gSceneDumpMeshName;
			}
		}
	*isNew = true;
	mMeshes[mMeshCount] = mesh;
	mMeshCount++;
	sprintf(gSceneDumpMeshName, "tmesh%d", mMeshCount);
	if(mMeshCount == 0x8000)
		mMeshCount = 0x7fff;
	return gSceneDumpMeshName;
	}

// phys_fn_003992 (0x0008fde0, 77 B)
// _tzset, _time32, _localtime32, then the two strftime calls with 0x80 as
// the size (0x8fe07, 0x8fe1c).
void sceneDumpDateTime(char* date, char* clock)
	{
	__time32_t now;
	_tzset();
	_time32(&now);
	struct tm* local = _localtime32(&now);
	strftime(date, 0x80, "%A, %B %d, %Y", local);
	strftime(clock, 0x80, "%I:%M %p", local);
	}

// phys_fn_003994 (0x0008fe30, 32 B)
// Its one caller, phys_fn_004006, hands it the pointer in eax (the listing
// sign-extends eax on entry, `cdq` at 0x8fe30); here it is a parameter.
const char* sceneDumpPointerName(const char* prefix, const void* pointer)
	{
	sprintf(gSceneDumpName, "%s__%I64x", prefix, sceneDumpKey(pointer));
	return gSceneDumpName;
	}

// phys_fn_003995 (0x0008fe50, 323 B)
// FLT_MAX (.rdata 0x10106858), 0 (0x101041f0), 1 (0x101041ec) and -1
// (0x1010687c) are literals in both modes. Text: "%.9f", then, when the text
// holds a '.', trailing '0's are cut and a '.' left last is cut too. Binary:
// "%.4f$%x" with the float's raw bits after the '$'.
const char* sceneDumpToken(NxReal value, bool binary)
	{
	char* token = gSceneDumpTokens[gSceneDumpTokenIndex];
	gSceneDumpTokenIndex++;
	if(gSceneDumpTokenIndex == 16)
		gSceneDumpTokenIndex = 0;
	if(value == FLT_MAX)
		{
		strcpy(token, "fltmax");
		return token;
		}
	if(value == 0.0f)
		{
		strcpy(token, "0");
		return token;
		}
	if(value == 1.0f)
		{
		strcpy(token, "1");
		return token;
		}
	if(value == -1.0f)
		{
		strcpy(token, "-1");
		return token;
		}
	if(binary)
		{
		sprintf(token, "%.4f$%x", (double)value, sceneDumpBits(value));
		return token;
		}
	sprintf(token, "%.9f", (double)value);
	if(strstr(token, "."))
		{
		char* last = token + strlen(token) - 1;
		while(*last == '0')
			last--;
		if(*last == '.')
			{
			*last = 0;
			return token;
			}
		last[1] = 0;
		}
	return token;
	}

// phys_fn_003997 (0x0008ffa0, 1757 B)
// mEmit is set first. After the first store, a value equal to the stored one
// (all four floats, `==` as fucompp: a NaN never matches) clears mEmit and,
// when a change is pending, writes a `PsDefaultSettings <kind>(...)` line of
// the new values and clears mPending; a different value sets mPending. The
// value is stored and mFirst cleared in every case (0x90653). Kind 2 writes
// only a density above 0. Kinds 16 and 17 (force, torque) have their arms;
// the asset writer never stores them.
bool SceneDumpSetting::store(NxReal v0, NxReal v1, NxReal v2, NxReal v3)
	{
	mEmit = true;
	if(!mFirst)
		{
		if(v0 == mValue[0] && v1 == mValue[1] && v2 == mValue[2] && v3 == mValue[3])
			{
			mEmit = false;
			if(mPending)
				{
				mPending = false;
				switch(mKind)
					{
					case NX_DUMP_POSITION:
						fprintf(mFile, "PsDefaultSettings position(%s,%s,%s)\r\n", sceneDumpToken(v0, mBinary),
							sceneDumpToken(v1, mBinary), sceneDumpToken(v2, mBinary));
						break;
					case NX_DUMP_ORIENTATION:
						fprintf(mFile, "PsDefaultSettings orientation(%s,%s,%s,%s)\r\n", sceneDumpToken(v0, mBinary),
							sceneDumpToken(v1, mBinary), sceneDumpToken(v2, mBinary), sceneDumpToken(v3, mBinary));
						break;
					case NX_DUMP_DENSITY:
						if(v0 > 0.0f)
							fprintf(mFile, "PsDefaultSettings density(%s)\r\n", sceneDumpToken(v0, mBinary));
						break;
					case NX_DUMP_SIDES:
						fprintf(mFile, "PsDefaultSettings sides(%s,%s,%s)\r\n", sceneDumpToken(v0, mBinary),
							sceneDumpToken(v1, mBinary), sceneDumpToken(v2, mBinary));
						break;
					case NX_DUMP_LOCALPOSITION:
						fprintf(mFile, "PsDefaultSettings localposition(%s,%s,%s)\r\n", sceneDumpToken(v0, mBinary),
							sceneDumpToken(v1, mBinary), sceneDumpToken(v2, mBinary));
						break;
					case NX_DUMP_LOCALORIENTATION:
						fprintf(mFile, "PsDefaultSettings localorientation(%s,%s,%s,%s)\r\n", sceneDumpToken(v0, mBinary),
							sceneDumpToken(v1, mBinary), sceneDumpToken(v2, mBinary), sceneDumpToken(v3, mBinary));
						break;
					case NX_DUMP_PLANE:
						fprintf(mFile, "PsDefaultSettings plane(%s,%s,%s,%s)\r\n", sceneDumpToken(v0, mBinary),
							sceneDumpToken(v1, mBinary), sceneDumpToken(v2, mBinary), sceneDumpToken(v3, mBinary));
						break;
					case NX_DUMP_HEIGHT:
						fprintf(mFile, "PsDefaultSettings height(%s)\r\n", sceneDumpToken(v0, mBinary));
						break;
					case NX_DUMP_RADIUS:
						fprintf(mFile, "PsDefaultSettings radius(%s)\r\n", sceneDumpToken(v0, mBinary));
						break;
					case NX_DUMP_MATERIAL:
						fprintf(mFile, "PsDefaultSettings material(mat%d)\r\n", sceneDumpRound(v0) + 1);
						break;
					case NX_DUMP_COM:
						fprintf(mFile, "PsDefaultSettings com(%s,%s,%s)\r\n", sceneDumpToken(v0, mBinary),
							sceneDumpToken(v1, mBinary), sceneDumpToken(v2, mBinary));
						break;
					case NX_DUMP_COMROT:
						fprintf(mFile, "PsDefaultSettings comrot(%s,%s,%s,%s)\r\n", sceneDumpToken(v0, mBinary),
							sceneDumpToken(v1, mBinary), sceneDumpToken(v2, mBinary), sceneDumpToken(v3, mBinary));
						break;
					case NX_DUMP_INERTIA:
						fprintf(mFile, "PsDefaultSettings inertia(%s,%s,%s)\r\n", sceneDumpToken(v0, mBinary),
							sceneDumpToken(v1, mBinary), sceneDumpToken(v2, mBinary));
						break;
					case NX_DUMP_MASS:
						fprintf(mFile, "PsDefaultSettings mass(%s)\r\n", sceneDumpToken(v0, mBinary));
						break;
					case NX_DUMP_VELOCITY:
						fprintf(mFile, "PsDefaultSettings velocity(%s,%s,%s)\r\n", sceneDumpToken(v0, mBinary),
							sceneDumpToken(v1, mBinary), sceneDumpToken(v2, mBinary));
						break;
					case NX_DUMP_ANGULARVELOCITY:
						fprintf(mFile, "PsDefaultSettings angularvelocity(%s,%s,%s)\r\n", sceneDumpToken(v0, mBinary),
							sceneDumpToken(v1, mBinary), sceneDumpToken(v2, mBinary));
						break;
					case NX_DUMP_FORCE:
						fprintf(mFile, "PsDefaultSettings force(%s,%s,%s)\r\n", sceneDumpToken(v0, mBinary),
							sceneDumpToken(v1, mBinary), sceneDumpToken(v2, mBinary));
						break;
					case NX_DUMP_TORQUE:
						fprintf(mFile, "PsDefaultSettings torque(%s,%s,%s)\r\n", sceneDumpToken(v0, mBinary),
							sceneDumpToken(v1, mBinary), sceneDumpToken(v2, mBinary));
						break;
					case NX_DUMP_WAKEUPCOUNTER:
						fprintf(mFile, "PsDefaultSettings wakeupcounter(%s)\r\n", sceneDumpToken(v0, mBinary));
						break;
					case NX_DUMP_LINEARDAMPING:
						fprintf(mFile, "PsDefaultSettings lineardamping(%s)\r\n", sceneDumpToken(v0, mBinary));
						break;
					case NX_DUMP_ANGULARDAMPING:
						fprintf(mFile, "PsDefaultSettings angulardamping(%s)\r\n", sceneDumpToken(v0, mBinary));
						break;
					case NX_DUMP_MAXANGULARVELOCITY:
						fprintf(mFile, "PsDefaultSettings maxangularvelocity(%s)\r\n", sceneDumpToken(v0, mBinary));
						break;
					case NX_DUMP_SOLVERCOUNT:
						fprintf(mFile, "PsDefaultSettings solvercount(%d)\r\n", sceneDumpRound(v0));
						break;
					case NX_DUMP_GROUP:
						fprintf(mFile, "PsDefaultSettings group(%d)\r\n", sceneDumpRound(v0));
						break;
					}
				}
			}
		else
			{
			mPending = true;
			}
		}
	mValue[0] = v0;
	mValue[1] = v1;
	mValue[2] = v2;
	mValue[3] = v3;
	mFirst = false;
	return mEmit;
	}

// phys_fn_003999 (0x000906e0, 1637 B)
// The stored value inline, `<kind>(...) `, when mEmit is set; kind 2 only
// for a density above 0.
bool SceneDumpSetting::print()
	{
	if(mEmit)
		{
		switch(mKind)
			{
			case NX_DUMP_POSITION:
				fprintf(mFile, "position(%s,%s,%s) ", sceneDumpToken(mValue[0], mBinary),
					sceneDumpToken(mValue[1], mBinary), sceneDumpToken(mValue[2], mBinary));
				break;
			case NX_DUMP_ORIENTATION:
				fprintf(mFile, "orientation(%s,%s,%s,%s) ", sceneDumpToken(mValue[0], mBinary),
					sceneDumpToken(mValue[1], mBinary), sceneDumpToken(mValue[2], mBinary),
					sceneDumpToken(mValue[3], mBinary));
				break;
			case NX_DUMP_DENSITY:
				if(mValue[0] > 0.0f)
					fprintf(mFile, "density(%s) ", sceneDumpToken(mValue[0], mBinary));
				break;
			case NX_DUMP_SIDES:
				fprintf(mFile, "sides(%s,%s,%s) ", sceneDumpToken(mValue[0], mBinary),
					sceneDumpToken(mValue[1], mBinary), sceneDumpToken(mValue[2], mBinary));
				break;
			case NX_DUMP_LOCALPOSITION:
				fprintf(mFile, "localposition(%s,%s,%s) ", sceneDumpToken(mValue[0], mBinary),
					sceneDumpToken(mValue[1], mBinary), sceneDumpToken(mValue[2], mBinary));
				break;
			case NX_DUMP_LOCALORIENTATION:
				fprintf(mFile, "localorientation(%s,%s,%s,%s) ", sceneDumpToken(mValue[0], mBinary),
					sceneDumpToken(mValue[1], mBinary), sceneDumpToken(mValue[2], mBinary),
					sceneDumpToken(mValue[3], mBinary));
				break;
			case NX_DUMP_PLANE:
				fprintf(mFile, "plane(%s,%s,%s,%s) ", sceneDumpToken(mValue[0], mBinary),
					sceneDumpToken(mValue[1], mBinary), sceneDumpToken(mValue[2], mBinary),
					sceneDumpToken(mValue[3], mBinary));
				break;
			case NX_DUMP_HEIGHT:
				fprintf(mFile, "height(%s) ", sceneDumpToken(mValue[0], mBinary));
				break;
			case NX_DUMP_RADIUS:
				fprintf(mFile, "radius(%s) ", sceneDumpToken(mValue[0], mBinary));
				break;
			case NX_DUMP_MATERIAL:
				fprintf(mFile, "material(mat%d) ", sceneDumpRound(mValue[0]) + 1);
				break;
			case NX_DUMP_COM:
				fprintf(mFile, "com(%s,%s,%s) ", sceneDumpToken(mValue[0], mBinary),
					sceneDumpToken(mValue[1], mBinary), sceneDumpToken(mValue[2], mBinary));
				break;
			case NX_DUMP_COMROT:
				fprintf(mFile, "comrot(%s,%s,%s,%s) ", sceneDumpToken(mValue[0], mBinary),
					sceneDumpToken(mValue[1], mBinary), sceneDumpToken(mValue[2], mBinary),
					sceneDumpToken(mValue[3], mBinary));
				break;
			case NX_DUMP_INERTIA:
				fprintf(mFile, "inertia(%s,%s,%s) ", sceneDumpToken(mValue[0], mBinary),
					sceneDumpToken(mValue[1], mBinary), sceneDumpToken(mValue[2], mBinary));
				break;
			case NX_DUMP_MASS:
				fprintf(mFile, "mass(%s) ", sceneDumpToken(mValue[0], mBinary));
				break;
			case NX_DUMP_VELOCITY:
				fprintf(mFile, "velocity(%s,%s,%s) ", sceneDumpToken(mValue[0], mBinary),
					sceneDumpToken(mValue[1], mBinary), sceneDumpToken(mValue[2], mBinary));
				break;
			case NX_DUMP_ANGULARVELOCITY:
				fprintf(mFile, "angularvelocity(%s,%s,%s) ", sceneDumpToken(mValue[0], mBinary),
					sceneDumpToken(mValue[1], mBinary), sceneDumpToken(mValue[2], mBinary));
				break;
			case NX_DUMP_FORCE:
				fprintf(mFile, "force(%s,%s,%s) ", sceneDumpToken(mValue[0], mBinary),
					sceneDumpToken(mValue[1], mBinary), sceneDumpToken(mValue[2], mBinary));
				break;
			case NX_DUMP_TORQUE:
				fprintf(mFile, "torque(%s,%s,%s) ", sceneDumpToken(mValue[0], mBinary),
					sceneDumpToken(mValue[1], mBinary), sceneDumpToken(mValue[2], mBinary));
				break;
			case NX_DUMP_WAKEUPCOUNTER:
				fprintf(mFile, "wakeupcounter(%s) ", sceneDumpToken(mValue[0], mBinary));
				break;
			case NX_DUMP_LINEARDAMPING:
				fprintf(mFile, "lineardamping(%s) ", sceneDumpToken(mValue[0], mBinary));
				break;
			case NX_DUMP_ANGULARDAMPING:
				fprintf(mFile, "angulardamping(%s) ", sceneDumpToken(mValue[0], mBinary));
				break;
			case NX_DUMP_MAXANGULARVELOCITY:
				fprintf(mFile, "maxangularvelocity(%s) ", sceneDumpToken(mValue[0], mBinary));
				break;
			case NX_DUMP_SOLVERCOUNT:
				fprintf(mFile, "solvercount(%d) ", sceneDumpRound(mValue[0]));
				break;
			case NX_DUMP_GROUP:
				fprintf(mFile, "group(%d) ", sceneDumpRound(mValue[0]));
				break;
			}
		}
	return mEmit;
	}

// phys_fn_004002 (0x00090db0, 92 B)
// Space, '"', tab, ',', '(', ')', '=', '[', ']', '{', '}', '#', tested in
// that order.
bool SceneDump::hasDelimiter(const char* name)
	{
	for(char c = *name; c; c = *++name)
		{
		if(c == ' ' || c == '"' || c == '\t' || c == ',' || c == '(' || c == ')' || c == '='
			|| c == '[' || c == ']' || c == '{' || c == '}' || c == '#')
			return true;
		}
	return false;
	}

// phys_fn_004004 (0x00090e10, 196 B)
// `$__<joint>`; a named joint is `<name>___<joint>` instead, quoted with '"'
// when the name holds a delimiter. The name comes from Joint::getName
// (phys_fn_004085), keyed on the internal joint.
const char* SceneDump::jointName(Joint* joint)
	{
	const __int64 key = sceneDumpKey(joint);
	gSceneDumpJointName[0] = 0;
	sprintf(gSceneDumpName, "%s__%I64x", "$", key);
	strcpy(gSceneDumpJointName, gSceneDumpName);
	const char* name = joint->getName();
	if(name)
		{
		if(hasDelimiter(name))
			{
			sprintf(gSceneDumpName, "%s__%I64x", "_", key);
			sprintf(gSceneDumpJointName, "%c%s%s%c", '"', name, gSceneDumpName, '"');
			}
		else
			{
			sprintf(gSceneDumpName, "%s__%I64x", "_", key);
			sprintf(gSceneDumpJointName, "%s%s", name, gSceneDumpName);
			}
		}
	return gSceneDumpJointName;
	}

// phys_fn_004006 (0x00090ee0, 208 B)
// `@world` for a null actor body; else `$__<body>`, or `<name>___<body>`
// (quoted as in 004004) when the actor has a name. The name is NpActor slot
// 84, getName (phys_fn_000086), called directly on the body's +0 NxActor.
void SceneDump::actorName(char* out, JointActorBody* body)
	{
	out[0] = 0;
	if(!body)
		{
		strcpy(out, "@world");
		return;
		}
	sprintf(gSceneDumpName, "%s__%I64x", "$", sceneDumpKey(body));
	strcpy(out, gSceneDumpName);
	const char* name = static_cast<NpActorVtable*>(body->mActor)->NpActorVtable::getName();
	if(name)
		{
		if(hasDelimiter(name))
			sprintf(out, "%c%s%s%c", '"', name, sceneDumpPointerName("_", body), '"');
		else
			sprintf(out, "%s%s", name, sceneDumpPointerName("_", body));
		}
	}

// phys_fn_004007 (0x00090fb0, 630 B)
// The two anchors (localAnchor[i], desc +0x40/+0x4c), the two axis frames
// (xaxis = localAxis[i], +0x28/+0x34; yaxis = localNormal[i], +0x10/+0x1c),
// then the joint's defaults: bodycollide by jointFlags bit 0 (+0x68);
// `breakable(false) ` when maxForce and maxTorque (+0x58/+0x5c) are both
// FLT_MAX -- compared as words with 0x7f7fffff (0x911a4-0x911b3) -- else
// both values; the name when the joint has one.
void SceneDump::writeJointFrames(const NxJointDesc& desc, FILE* file, Joint* joint, bool binary)
	{
	fprintf(file, "PsJointOffset frame(primary)   body(primary)   offset(%s,%s,%s)\r\n",
		sceneDumpToken(desc.localAnchor[0].x, binary), sceneDumpToken(desc.localAnchor[0].y, binary),
		sceneDumpToken(desc.localAnchor[0].z, binary));
	fprintf(file, "PsJointOffset frame(secondary) body(secondary) offset(%s,%s,%s)\r\n",
		sceneDumpToken(desc.localAnchor[1].x, binary), sceneDumpToken(desc.localAnchor[1].y, binary),
		sceneDumpToken(desc.localAnchor[1].z, binary));
	fprintf(file, "PsJointAxes   frame(primary)   body(primary)   xaxis(%s,%s,%s) yaxis(%s,%s,%s)\r\n",
		sceneDumpToken(desc.localAxis[0].x, binary), sceneDumpToken(desc.localAxis[0].y, binary),
		sceneDumpToken(desc.localAxis[0].z, binary), sceneDumpToken(desc.localNormal[0].x, binary),
		sceneDumpToken(desc.localNormal[0].y, binary), sceneDumpToken(desc.localNormal[0].z, binary));
	fprintf(file, "PsJointAxes   frame(secondary) body(secondary) xaxis(%s,%s,%s) yaxis(%s,%s,%s)\r\n",
		sceneDumpToken(desc.localAxis[1].x, binary), sceneDumpToken(desc.localAxis[1].y, binary),
		sceneDumpToken(desc.localAxis[1].z, binary), sceneDumpToken(desc.localNormal[1].x, binary),
		sceneDumpToken(desc.localNormal[1].y, binary), sceneDumpToken(desc.localNormal[1].z, binary));
	fprintf(file, "PsDefaultSettings ");
	fprintf(file, (desc.jointFlags & NX_JF_COLLISION_ENABLED) ? "bodycollide(true) " : "bodycollide(false) ");
	if(sceneDumpBits(desc.maxForce) == 0x7f7fffff && sceneDumpBits(desc.maxTorque) == 0x7f7fffff)
		fprintf(file, "breakable(false) ");
	else
		fprintf(file, "breakable(%s,%s) ", sceneDumpToken(desc.maxForce, binary),
			sceneDumpToken(desc.maxTorque, binary));
	const char* name = joint->getName();
	if(name)
		fprintf(file, "name(%c%s%c) ", '"', name, '"');
	fprintf(file, "\r\n");
	}

// phys_fn_004009 (0x00091230, 121 B)
// low.value, low.restitution, low.hardness, then the same of high.
const char* SceneDump::limitPairText(const NxJointLimitPairDesc& limit, bool binary)
	{
	sprintf(gSceneDumpText, "%s,%s,%s,  %s,%s,%s", sceneDumpToken(limit.low.value, binary),
		sceneDumpToken(limit.low.restitution, binary), sceneDumpToken(limit.low.hardness, binary),
		sceneDumpToken(limit.high.value, binary), sceneDumpToken(limit.high.restitution, binary),
		sceneDumpToken(limit.high.hardness, binary));
	return gSceneDumpText;
	}

// phys_fn_004011 (0x000912b0, 79 B)
// The first three floats of an NxJointLimitDesc (value, restitution,
// hardness) or an NxSpringDesc (spring, damper, targetValue): the joint block
// calls this one row for both (0x9272b, 0x923dc, 0x927a2, ...).
const char* SceneDump::tripleText(const NxReal* values, bool binary)
	{
	sprintf(gSceneDumpText, "%s,%s,%s", sceneDumpToken(values[0], binary), sceneDumpToken(values[1], binary),
		sceneDumpToken(values[2], binary));
	return gSceneDumpText;
	}

// phys_fn_004013 (0x00091300, 83 B)
// velTarget, maxForce, then freeSpin as `true`/`false` (0x10117a7c,
// 0x10117a74).
const char* SceneDump::motorText(const NxMotorDesc& motor, bool binary)
	{
	sprintf(gSceneDumpText, "%s,%s,%s", sceneDumpToken(motor.velTarget, binary),
		sceneDumpToken(motor.maxForce, binary), motor.freeSpin ? "true" : "false");
	return gSceneDumpText;
	}

// phys_fn_004015 (0x00091360, 135 B)
// The two owners come from 004068 (0 when a body is missing), each named by
// 004006 into a 512-byte frame buffer; the joint by 004004. The `binary`
// argument is not read.
void SceneDump::writeJointLine(FILE* file, Joint* joint, bool binary)
	{
	(void)binary;
	void* owner0 = 0;
	void* owner1 = 0;
	char name0[0x200];
	char name1[0x200];
	joint->getBodyOwners(owner0, owner1);
	actorName(name0, static_cast<JointActorBody*>(owner0));
	actorName(name1, static_cast<JointActorBody*>(owner1));
	fprintf(file, "PsJoint %s %s %s\r\n", jointName(joint), name0, name1);
	}

// phys_fn_004017 (0x000913f0, 105 B)
// NX_TRIGGER_ON_ENTER, _ON_LEAVE and _ON_STAY (bits 0-2 of the low byte).
void SceneDump::writeTriggerFlags(FILE* file, NxU32 flags)
	{
	if(flags & NX_TRIGGER_ENABLE)
		{
		fprintf(file, "triggerevent(");
		if(flags & NX_TRIGGER_ON_ENTER)
			fprintf(file, "enter,");
		if(flags & NX_TRIGGER_ON_LEAVE)
			fprintf(file, "leave,");
		if(flags & NX_TRIGGER_ON_STAY)
			fprintf(file, "stay");
		fprintf(file, ") ");
		}
	}

// phys_fn_004019 (0x00091460, 452 B)
// Compiler-generated: NxCapsuleShapeDesc::isValid (NxCapsuleShapeDesc.h),
// slot 2 of 0x10118274.

// phys_fn_004021 (0x00091630, 522 B)
// Compiler-generated: NxRevoluteJointDesc::isValid (NxRevoluteJointDesc.h),
// slot 2 of the revolute descriptor table 0x101182a4 writeJoint installs.

// phys_fn_004023 (0x00091840, 254 B)
// Compiler-generated: the isValid shared by the cylindrical, prismatic,
// point-on-line and point-in-plane descriptors (each returns
// NxJointDesc::isValid()), slot 2 of their tables 0x10118280, 0x101182b4,
// 0x10118298 and 0x1011828c.

// phys_fn_004025 (0x00091940, 31 B)
// Compiler-generated: the scalar deleting destructor of NxJointDesc and of
// every joint descriptor above (their destructors are the empty inline
// ~NxJointDesc): restores 0x10117a44 and frees when bit 0 of the flags is
// set. Slot 0 of every table named above.

// phys_fn_004027 (0x00091960, 730 B)
// Compiler-generated: NxSphericalJointDesc::isValid (NxSphericalJointDesc.h),
// slot 2 of the spherical descriptor table 0x101182c0.

// phys_fn_004029 (0x00091c40, 404 B)
// Compiler-generated: NxSphereShapeDesc::isValid (NxSphereShapeDesc.h), slot
// 2 of 0x101182cc.

// phys_fn_004031 (0x00091de0, 31 B)
// Compiler-generated: the scalar deleting destructor of NxShapeDesc's
// families (their destructors are the empty inline ~NxShapeDesc), slot 0 of
// the capsule, sphere and mesh descriptor tables above.

// phys_fn_004033 (0x00091e00, 367 B)
// Compiler-generated: NxTriangleMeshShapeDesc::isValid
// (NxTriangleMeshShapeDesc.h), slot 2 of 0x101182d8.

// .rdata 0x10107a08: the weld distance phys_fn_004035 compares against.
static const NxReal gSceneDumpWeldDistance = 0.00001f;

// phys_fn_004035 (0x00091f70, 238 B)
// Each vertex is compared with every earlier one (not only the written ones):
// `fld; fsub; fabs; fcomp [1e-5f]; test ah,5; jp` leaves the axis test
// unless the distance is ordered below the weld distance. A vertex that
// matches vertex j maps to j; one that matches none maps to itself and is
// written. The remap is allocated even for no vertices.
NxU32* SceneDumpNames::writeVertices(NxU32 count, const void* points, NxU32 stride, FILE* file, bool binary)
	{
	NxU32* remap = static_cast<NxU32*>(::operator new(count * 4));
	const NxU8* point = static_cast<const NxU8*>(points);
	for(NxU32 i = 0; i < count; i++, point += stride)
		{
		const NxReal* p = reinterpret_cast<const NxReal*>(point);
		const NxU8* other = static_cast<const NxU8*>(points);
		NxU32 j = 0;
		for(; j < i; j++, other += stride)
			{
			const NxReal* o = reinterpret_cast<const NxReal*>(other);
			if(fabs((double)p[0] - o[0]) < gSceneDumpWeldDistance && fabs((double)p[1] - o[1]) < gSceneDumpWeldDistance
				&& fabs((double)p[2] - o[2]) < gSceneDumpWeldDistance)
				{
				remap[i] = j;
				break;
				}
			}
		if(j == i)
			{
			remap[i] = i;
			fprintf(file, "PsVert %s %s %s\r\n", sceneDumpToken(p[0], binary), sceneDumpToken(p[1], binary),
				sceneDumpToken(p[2], binary));
			}
		}
	return remap;
	}

// phys_fn_004037 (0x00092060, 409 B)
// With its continuations phys_fn_004039 (0x00092200, 665 B), phys_fn_004041
// (0x000924a0, 445 B) and phys_fn_004043 (0x00092660, 1231 B), which carry
// the revolute, cylindrical and spherical arms and the shared tail. The
// owners are read (004068) and not used. Types 0-5 (the jump table
// 0x10092b30) write `PsJointBegin`, the family banner, the descriptor
// (built on the stack by the public header's constructor, then filled by the
// internal joint's saveToDesc, internal slot 10 `[vt+0x28]`, on the joint
// 004072 returns), the frame lines (004007) and the limit line; every type,
// 6-9 included (`ja` to the tail), then writes one `PsJointLimitPlane` per
// limit plane and `PsJointEnd`.
void SceneDump::writeJoint(FILE* file, Joint* joint, bool binary)
	{
	void* owner0 = 0;
	void* owner1 = 0;
	joint->getBodyOwners(owner0, owner1);
	switch(joint->getType())
		{
		case NX_JOINT_PRISMATIC:
			{
			fprintf(file, "PsJointBegin %s\r\n", jointName(joint));
			fprintf(file, "### Prismatic Joint\r\n");
			PrismaticJoint* internal = static_cast<PrismaticJoint*>(joint->is(NX_JOINT_PRISMATIC));
			NxPrismaticJointDesc desc;
			internal->saveToDesc(desc);
			writeJointFrames(desc, file, joint, binary);
			fprintf(file, "PsJointLimit ");
			fprintf(file, "swing1(locked) swing2(locked) twist(locked) linear(free,locked,locked)\r\n");
			break;
			}
		case NX_JOINT_REVOLUTE:
			{
			fprintf(file, "PsJointBegin %s\r\n", jointName(joint));
			fprintf(file, "### Revolute Joint\r\n");
			RevoluteJoint* internal = static_cast<RevoluteJoint*>(joint->is(NX_JOINT_REVOLUTE));
			NxRevoluteJointDesc desc;
			internal->saveToDesc(desc);
			writeJointFrames(desc, file, joint, binary);
			fprintf(file, "PsJointLimit ");
			if(desc.flags & NX_RJF_LIMIT_ENABLED)
				fprintf(file, "twist(%s) ", limitPairText(desc.limit, binary));
			else
				fprintf(file, "twist(free) ");
			if(desc.flags & NX_RJF_MOTOR_ENABLED)
				fprintf(file, "motor(%s) ", motorText(desc.motor, binary));
			else
				fprintf(file, "motor(false) ");
			if(desc.flags & NX_RJF_SPRING_ENABLED)
				{
				fprintf(file, "twistspring(%s) ", tripleText(&desc.spring.spring, binary));
				fprintf(file, "swing1(locked) swing2(locked) linear(locked,locked,locked)\r\n");
				}
			else
				{
				fprintf(file, "twistspring(none) ");
				fprintf(file, "swing1(locked) swing2(locked) linear(locked,locked,locked)\r\n");
				}
			break;
			}
		case NX_JOINT_CYLINDRICAL:
			{
			fprintf(file, "PsJointBegin %s\r\n", jointName(joint));
			fprintf(file, "### Cylindrical Joint\r\n");
			CylindricalJoint* internal = static_cast<CylindricalJoint*>(joint->is(NX_JOINT_CYLINDRICAL));
			NxCylindricalJointDesc desc;
			internal->saveToDesc(desc);
			writeJointFrames(desc, file, joint, binary);
			fprintf(file, "PsJointLimit ");
			fprintf(file, "twist(free) swing1(locked) swing2(locked) linear(free,locked,locked)\r\n");
			break;
			}
		case NX_JOINT_SPHERICAL:
			{
			fprintf(file, "PsJointBegin %s\r\n", jointName(joint));
			fprintf(file, "### Spherical Joint\r\n");
			SphericalJoint* internal = static_cast<SphericalJoint*>(joint->is(NX_JOINT_SPHERICAL));
			NxSphericalJointDesc desc;
			internal->saveToDesc(desc);
			writeJointFrames(desc, file, joint, binary);
			fprintf(file, "PsJointLimit ");
			if(desc.flags & NX_SJF_SWING_LIMIT_ENABLED)
				{
				const char* swing = tripleText(&desc.swingLimit.value, binary);
				fprintf(file, "swing1(%s) swing2(%s) ", swing, swing);
				}
			else
				{
				fprintf(file, "swing1(free) swing2(free) ");
				}
			if(desc.flags & NX_SJF_TWIST_LIMIT_ENABLED)
				fprintf(file, "twist(%s) ", limitPairText(desc.twistLimit, binary));
			else
				fprintf(file, "twist(free) ");
			if(desc.flags & NX_SJF_TWIST_SPRING_ENABLED)
				fprintf(file, "twistspring(%s) ", tripleText(&desc.twistSpring.spring, binary));
			else
				fprintf(file, "twistspring(none) ");
			if(desc.flags & NX_SJF_SWING_SPRING_ENABLED)
				fprintf(file, "swingspring(%s) ", tripleText(&desc.swingSpring.spring, binary));
			else
				fprintf(file, "swingspring(none) ");
			if(desc.flags & NX_SJF_JOINT_SPRING_ENABLED)
				fprintf(file, "jointspring(%s) ", tripleText(&desc.jointSpring.spring, binary));
			else
				fprintf(file, "jointspring(none) ");
			if(desc.projectionMode != NX_JPM_NONE)
				{
				fprintf(file, "projection(%s) ", sceneDumpToken(desc.projectionDistance, binary));
				fprintf(file, "linear(locked,locked,locked)\r\n");
				}
			else
				{
				fprintf(file, "projection(none) ");
				fprintf(file, "linear(locked,locked,locked)\r\n");
				}
			break;
			}
		case NX_JOINT_POINT_ON_LINE:
			{
			fprintf(file, "PsJointBegin %s\r\n", jointName(joint));
			fprintf(file, "### Point On Line Joint\r\n");
			PointOnLineJoint* internal = static_cast<PointOnLineJoint*>(joint->is(NX_JOINT_POINT_ON_LINE));
			NxPointOnLineJointDesc desc;
			internal->saveToDesc(desc);
			writeJointFrames(desc, file, joint, binary);
			fprintf(file, "PsJointLimit ");
			fprintf(file, "swing1(free) swing2(free) twist(free) linear(free,locked,locked)\r\n");
			break;
			}
		case NX_JOINT_POINT_IN_PLANE:
			{
			fprintf(file, "PsJointBegin %s\r\n", jointName(joint));
			fprintf(file, "### Point In Plane Joint\r\n");
			PointInPlaneJoint* internal = static_cast<PointInPlaneJoint*>(joint->is(NX_JOINT_POINT_IN_PLANE));
			NxPointInPlaneJointDesc desc;
			internal->saveToDesc(desc);
			writeJointFrames(desc, file, joint, binary);
			fprintf(file, "PsJointLimit ");
			fprintf(file, "swing1(free) swing2(free) twist(free) linear(free,free,locked)\r\n");
			break;
			}
		default:
			break;
		}
	NxVec3 normal;
	NxReal d;
	joint->resetLimitPlaneIterator();
	while(joint->hasMoreLimitPlanes())
		{
		joint->getNextLimitPlane(normal, d);
		fprintf(file, "PsJointLimitPlane %s %s %s %s\r\n", sceneDumpToken(normal.x, binary),
			sceneDumpToken(normal.y, binary), sceneDumpToken(normal.z, binary), sceneDumpToken(d, binary));
		}
	fprintf(file, "PsJointEnd\r\n");
	}

// The three spellings of NxMat33::toQuat (Foundation/include/NxMat33.h) the
// asset and shape writers inline. The header's trace and its diagonal arms
// are one expression; the listing forms one pair of the diagonal first,
// spills it to a float (`fst dword`) and reuses the spill in the arm whose
// diagonal is the third term, so which pair it formed decides both the
// trace's association and which arm sees a rounded pair:
//   84_WIDE   (004055's actor pose and centre-of-mass pose, 0x94399 and
//             0x946e4) pair m8 + m4, trace (m8 + m4) + m0 on the stack;
//   08_WIDE   (004048's plane, sphere, box and capsule arms) pair m0 + m8,
//             trace (m0 + m8) + m4 on the stack;
//   08_STORED (004048's mesh arm, 0x93e6e) pair m0 + m8 stored first
//             (`fstp`), trace m4 + the stored float.
// The root stays on the stack: `w = 0.5 * s` and `r = 0.5 / s` are formed
// from it unrounded, and so is every product with r. The listing's fourth
// switch arm (an index above 2) cannot be reached and is not written.
// q is x, y, z, w.
enum SceneDumpQuatForm
	{
	SCENE_DUMP_QUAT_84_WIDE,
	SCENE_DUMP_QUAT_08_WIDE,
	SCENE_DUMP_QUAT_08_STORED
	};

static void sceneDumpQuat(const NxMat33& matrix, SceneDumpQuatForm form, NxReal* q)
	{
	const NxReal* m = reinterpret_cast<const NxReal*>(&matrix);
	const NxU32 a = form == SCENE_DUMP_QUAT_84_WIDE ? 8 : 0;
	const NxU32 b = form == SCENE_DUMP_QUAT_84_WIDE ? 4 : 8;
	const NxU32 c = form == SCENE_DUMP_QUAT_84_WIDE ? 0 : 4;
	const double wide = (double)m[a] + m[b];
	const NxReal pair = (NxReal)wide;
	const double trace = form == SCENE_DUMP_QUAT_08_STORED ? (double)m[c] + pair : wide + m[c];
	if(trace >= 0.0)
		{
		const double s = form == SCENE_DUMP_QUAT_08_STORED ? x87FsqrtSum3(m[c], pair, 1.0)
			: x87FsqrtSum4(m[a], m[b], m[c], 1.0);
		q[3] = (NxReal)(0.5f * s);
		const double r = 0.5f / s;
		q[0] = (NxReal)(((double)m[7] - m[5]) * r);
		q[1] = (NxReal)(((double)m[2] - m[6]) * r);
		q[2] = (NxReal)(((double)m[3] - m[1]) * r);
		return;
		}
	NxU32 k = 0;
	if(m[4] > m[0])
		k = 1;
	if(m[8] > m[k * 4])
		k = 2;
	// The arm of the third term takes the spilled pair; the other two form
	// their own pair (`fld; fadd; fsubr`), in the listing's operand order.
	double s;
	if(k * 4 == c)
		s = x87FsqrtSum3(m[c], -pair, 1.0);
	else if(k * 4 == a)
		s = x87FsqrtDiag(m[a], m[b], m[c]);
	else
		s = x87FsqrtDiag(m[b], m[a], m[c]);
	const double r = 0.5f / s;
	if(k == 0)
		{
		q[0] = (NxReal)(0.5f * s);
		q[1] = (NxReal)(((double)m[3] + m[1]) * r);
		q[2] = (NxReal)(((double)m[6] + m[2]) * r);
		q[3] = (NxReal)(((double)m[7] - m[5]) * r);
		}
	else if(k == 1)
		{
		q[1] = (NxReal)(0.5f * s);
		q[2] = (NxReal)(((double)m[7] + m[5]) * r);
		q[0] = (NxReal)(((double)m[3] + m[1]) * r);
		q[3] = (NxReal)(((double)m[2] - m[6]) * r);
		}
	else
		{
		q[2] = (NxReal)(0.5f * s);
		q[0] = (NxReal)(((double)m[6] + m[2]) * r);
		q[1] = (NxReal)(((double)m[7] + m[5]) * r);
		q[3] = (NxReal)(((double)m[3] - m[1]) * r);
		}
	}

// The internal shape's table as the shape writers call it: slot 13
// (`[vt+0x34]`), the family's saveToDesc (the candidate's
// nxPlaneSaveState/nxSphereSaveState/nxBoxSaveState/nxCapsuleSaveState in
// ObjectModel.cpp, installed by nxShapeFactoryInstallVtable). A call view
// only: nothing derives from it, and the earlier slots are named by index.
class SceneDumpShape
	{
	public:
	virtual void slot0() = 0;
	virtual void slot1() = 0;
	virtual void slot2() = 0;
	virtual void slot3() = 0;
	virtual void slot4() = 0;
	virtual void slot5() = 0;
	virtual void slot6() = 0;
	virtual void slot7() = 0;
	virtual void slot8() = 0;
	virtual void slot9() = 0;
	virtual void slot10() = 0;
	virtual void slot11() = 0;
	virtual void slot12() = 0;
	virtual bool saveToDesc(NxShapeDesc& desc) = 0;
	};

// The internal TriangleMesh's table (.rdata 0x10108608, nineteen slots) as
// the mesh arm and writeMesh call it: two leading slots, then the public
// NxTriangleMesh interface in its header order, so slot 3 is saveToDesc
// (phys_fn_002166), slot 10 hasPMap (002170) and slot 13 getPMapDensity
// (002176). A call view only; the candidate's TriangleMesh (TriangleMesh.h)
// carries its table as an opaque word and cannot be instanced as a shape (see
// writeMesh).
class SceneDumpTriangleMesh
	{
	public:
	virtual void slot0() = 0;
	virtual void slot1() = 0;
	virtual bool loadFromDesc(const NxTriangleMeshDesc& desc) = 0;
	virtual bool saveToDesc(NxTriangleMeshDesc& desc) const = 0;
	virtual void getSubmeshCount() const = 0;
	virtual void getCount() const = 0;
	virtual void getFormat() const = 0;
	virtual void getBase() const = 0;
	virtual void getStride() const = 0;
	virtual void loadPMap() = 0;
	virtual bool hasPMap() const = 0;
	virtual NxU32 getPMapSize() const = 0;
	virtual void getPMapData() const = 0;
	virtual NxU32 getPMapDensity() const = 0;
	};

// The convex mesh at TriangleMesh+0xa0 as writeMesh reads it: the hull's
// vertices (+0x0c count, +0x10 points, 12 bytes each) and its polygons
// (+0x24 count, +0x28 array of 0x24-byte records whose +0x00 is the vertex
// count and +0x04 the vertex indices). Read view only.
struct SceneDumpConvexPolygon
	{
	NxU32			mVertexCount;		//!< +0x00
	const NxU32*	mIndices;			//!< +0x04
	NxU8			mUnknown08[0x24 - 0x08];
	};

struct SceneDumpConvexMesh
	{
	//! phys_fn_001472 (0x0002b6f0). Builds the polygons (deferred below).
	void buildPolygons();

	NxU8					mUnknown00[0x0c];
	NxU32					mVertexCount;		//!< +0x0c
	const void*				mVertices;			//!< +0x10
	NxU8					mUnknown14[0x24 - 0x14];
	NxU32					mPolygonCount;		//!< +0x24
	SceneDumpConvexPolygon*	mPolygons;			//!< +0x28
	};

static_assert(sizeof(SceneDumpConvexPolygon) == 0x24, "convex polygons are 0x24 bytes");
static_assert(offsetof(SceneDumpConvexMesh, mVertexCount) == 0x0c, "hull vertex count at +0x0c");
static_assert(offsetof(SceneDumpConvexMesh, mPolygonCount) == 0x24, "polygon count at +0x24");
static_assert(offsetof(SceneDumpConvexMesh, mPolygons) == 0x28, "polygons at +0x28");

// phys_fn_001472 (0x0002b6f0, 664 B)
// (deferred: Phase 2, gap:ConvexHull.cpp..IceAdjacencies.cpp; the convex
// mesh's polygon builder, which writeMesh calls when the hull has no polygons
// yet. The candidate builds no mesh shape, so nothing reaches it.)
void SceneDumpConvexMesh::buildPolygons()
	{
	NX_ASSERT(0);
	}

// The listing's `setne; dec; and` cast (the internal Shape's inline isX):
// the shape when its type word matches, else null.
static NX_INLINE SceneDumpShape* sceneDumpShapeIs(NxCollisionShape* shape, NxU32 type)
	{
	return shape->type == type ? reinterpret_cast<SceneDumpShape*>(shape) : 0;
	}

// phys_fn_004046 (0x00092b50, 1079 B)
// A mesh met before is only named. A new one writes its begin line (for a
// triangle mesh with the pmap density, the shape's gouraud flag -- the shape
// saved again into a fresh descriptor -- the mesh's winding and, for an
// axis 0-2, the height field), then, when it has vertices, its vertices
// (004035) and its faces: a convex mesh's hull polygons (built by 001472 when
// missing; a null hull writes nothing and frees a null remap), or the
// triangles, sixteen to a `PsTri` line. The remap is freed through `free`
// (005668) and the end line written. The height-field axis and extent are the
// mesh's own words (+0x7c, +0x80), read on entry.
//
// Not reachable in the candidate: nxShapeFactoryInstallVtable installs tables
// for shape types 0-3 only, so no shape of type 4 exists, and the candidate's
// TriangleMesh has no C++ table for the slot calls above.
const char* SceneDump::writeMesh(FILE* file, TriangleMesh* mesh, SceneDumpShape* shape, bool binary, bool isConvex,
	NxU32 pmapDensity, SceneDumpNames* names)
	{
	const NxU32 axis = mesh->mHeightFieldVerticalAxis;
	const NxReal extent = mesh->mHeightFieldVerticalExtent;
	InternalTriangleMesh* internal = &mesh->mInternal;
	bool isNew;
	const char* name = names->meshName(internal, &isNew);
	if(!isNew)
		return name;
	if(isConvex)
		{
		fprintf(file, "PsConvexBegin %s\r\n", name);
		}
	else
		{
		fprintf(file, "PsTriangleMeshBegin %s ", name);
		if(pmapDensity)
			fprintf(file, "@pmap(%d) ", pmapDensity);
		NxTriangleMeshShapeDesc shapeDesc;
		shape->saveToDesc(shapeDesc);
		fprintf(file, "gouraud(%s) ", (shapeDesc.meshFlags & NX_MESH_SMOOTH_SPHERE_COLLISIONS) ? "true" : "false");
		NxTriangleMeshDesc meshDesc;
		reinterpret_cast<SceneDumpTriangleMesh*>(mesh)->saveToDesc(meshDesc);
		fprintf(file, "winding(%s) ", (meshDesc.flags & NX_MF_FLIPNORMALS) ? "ccw" : "cw");
		switch(axis)
			{
			case NX_X:
				fprintf(file, "@heightfield(%s, %f) ", "xaxis", (double)extent);
				break;
			case NX_Y:
				fprintf(file, "@heightfield(%s, %f) ", "yaxis", (double)extent);
				break;
			case NX_Z:
				fprintf(file, "@heightfield(%s, %f) ", "zaxis", (double)extent);
				break;
			}
		fprintf(file, "\r\n");
		}
	if(internal->mVertexCount)
		{
		NxU32* remap = 0;
		if(isConvex)
			{
			SceneDumpConvexMesh* convex = static_cast<SceneDumpConvexMesh*>(mesh->mConvexMesh);
			if(convex)
				{
				remap = names->writeVertices(convex->mVertexCount, convex->mVertices, 0xc, file, binary);
				if(!convex->mPolygonCount)
					convex->buildPolygons();
				const int nbPolygons = (int)convex->mPolygonCount;
				for(int i = 0; i < nbPolygons; i++)
					{
					if(!convex->mPolygons)
						convex->buildPolygons();
					const SceneDumpConvexPolygon& polygon = convex->mPolygons[i];
					fprintf(file, "PsFace %d ", polygon.mVertexCount);
					for(NxU32 j = 0; j < polygon.mVertexCount; j++)
						fprintf(file, "%d ", remap[polygon.mIndices[j]]);
					fprintf(file, "\r\n");
					}
				}
			}
		else
			{
			remap = names->writeVertices(internal->mVertexCount, internal->mVertices, 0xc, file, binary);
			const NxU32 nbTriangles = internal->mTriangleCount;
			if(nbTriangles)
				{
				const NxU32* triangle = static_cast<const NxU32*>(internal->mTriangles);
				for(int i = 0; i < (int)nbTriangles; i++, triangle += 3)
					{
					const NxU32 v0 = remap[triangle[0]];
					const NxU32 v1 = remap[triangle[1]];
					const NxU32 v2 = remap[triangle[2]];
					if(i & 0xf)
						{
						fprintf(file, "%d %d %d ", v0, v1, v2);
						}
					else
						{
						if(i)
							fprintf(file, "\r\n");
						fprintf(file, "PsTri %d %d %d ", v0, v1, v2);
						}
					}
				fprintf(file, "\r\n");
				}
			}
		free(remap);
		}
	if(isConvex)
		fprintf(file, "PsConvexEnd\r\n");
	else
		fprintf(file, "PsTriangleMeshEnd\r\n");
	return name;
	}

// phys_fn_004048 (0x00092f90, 4481 B)
// Switch on 001283 (the jump table 0x10094114, five entries; `ja` for a type
// above 4 writes nothing). Each arm builds its family's descriptor on the
// stack (the public header's constructor), fills it through the shape's slot
// 13 on the shape cast by type, then stores the setting records and prints
// them inline:
//   plane    stores localposition, localorientation, plane, material, group;
//            prints `PsPlane  `, plane, localposition, localorientation,
//            group, material -- group before material here only;
//   sphere   stores localposition, localorientation, radius, material, group;
//            prints `PsSphere `, radius, then the four in stored order;
//   box      stores sides (the dimensions doubled, `fadd st0,st0`) first,
//            then localposition, localorientation, material, group; prints
//            `PsBox ` and the five in stored order;
//   capsule  stores localposition, localorientation, radius, height,
//            material, group; prints `PsCapsule `, height, radius,
//            localposition, localorientation, material, group;
//   mesh     writes the mesh (004046) first, then stores localposition,
//            localorientation, material, group and prints `PsConvex <name> `
//            or `PsTriangleMesh <name> ` and the four.
// Material and group are the descriptor's 16-bit words widened (`movzx;
// fild`). Every arm ends with the trigger flags (004017) of the
// descriptor's shapeFlags -- except the capsule's, which passes the
// capsule's own flags word (desc +0x54, `mov ecx,[esp+0x7c]` at 0x93d02).
// `binary` is read only by the mesh arm, for 004046.
void SceneDump::writeShape(FILE* file, NxCollisionShape* shape, bool binary, SceneDumpNames* names)
	{
	NxReal q[4];
	switch(NxShapeGetType(shape, 0))
		{
		case NX_SHAPE_PLANE:
			{
			NxPlaneShapeDesc desc;
			sceneDumpShapeIs(shape, NX_SHAPE_PLANE)->saveToDesc(desc);
			sceneDumpQuat(desc.localPose.M, SCENE_DUMP_QUAT_08_WIDE, q);
			mSettings[NX_DUMP_LOCALPOSITION].store(desc.localPose.t.x, desc.localPose.t.y, desc.localPose.t.z, 0.0f);
			mSettings[NX_DUMP_LOCALORIENTATION].store(q[0], q[1], q[2], q[3]);
			mSettings[NX_DUMP_PLANE].store(desc.normal.x, desc.normal.y, desc.normal.z, desc.d);
			mSettings[NX_DUMP_MATERIAL].store((NxReal)(int)desc.materialIndex, 0.0f, 0.0f, 0.0f);
			mSettings[NX_DUMP_GROUP].store((NxReal)(int)desc.group, 0.0f, 0.0f, 0.0f);
			fprintf(file, "PsPlane  ");
			mSettings[NX_DUMP_PLANE].print();
			mSettings[NX_DUMP_LOCALPOSITION].print();
			mSettings[NX_DUMP_LOCALORIENTATION].print();
			mSettings[NX_DUMP_GROUP].print();
			mSettings[NX_DUMP_MATERIAL].print();
			writeTriggerFlags(file, desc.shapeFlags);
			break;
			}
		case NX_SHAPE_SPHERE:
			{
			NxSphereShapeDesc desc;
			sceneDumpShapeIs(shape, NX_SHAPE_SPHERE)->saveToDesc(desc);
			sceneDumpQuat(desc.localPose.M, SCENE_DUMP_QUAT_08_WIDE, q);
			mSettings[NX_DUMP_LOCALPOSITION].store(desc.localPose.t.x, desc.localPose.t.y, desc.localPose.t.z, 0.0f);
			mSettings[NX_DUMP_LOCALORIENTATION].store(q[0], q[1], q[2], q[3]);
			mSettings[NX_DUMP_RADIUS].store(desc.radius, 0.0f, 0.0f, 0.0f);
			mSettings[NX_DUMP_MATERIAL].store((NxReal)(int)desc.materialIndex, 0.0f, 0.0f, 0.0f);
			mSettings[NX_DUMP_GROUP].store((NxReal)(int)desc.group, 0.0f, 0.0f, 0.0f);
			fprintf(file, "PsSphere ");
			mSettings[NX_DUMP_RADIUS].print();
			mSettings[NX_DUMP_LOCALPOSITION].print();
			mSettings[NX_DUMP_LOCALORIENTATION].print();
			mSettings[NX_DUMP_MATERIAL].print();
			mSettings[NX_DUMP_GROUP].print();
			writeTriggerFlags(file, desc.shapeFlags);
			break;
			}
		case NX_SHAPE_BOX:
			{
			NxBoxShapeDesc desc;
			sceneDumpShapeIs(shape, NX_SHAPE_BOX)->saveToDesc(desc);
			const NxReal sideX = (NxReal)((double)desc.dimensions.x + desc.dimensions.x);
			const NxReal sideY = (NxReal)((double)desc.dimensions.y + desc.dimensions.y);
			const NxReal sideZ = (NxReal)((double)desc.dimensions.z + desc.dimensions.z);
			mSettings[NX_DUMP_SIDES].store(sideX, sideY, sideZ, 0.0f);
			sceneDumpQuat(desc.localPose.M, SCENE_DUMP_QUAT_08_WIDE, q);
			mSettings[NX_DUMP_LOCALPOSITION].store(desc.localPose.t.x, desc.localPose.t.y, desc.localPose.t.z, 0.0f);
			mSettings[NX_DUMP_LOCALORIENTATION].store(q[0], q[1], q[2], q[3]);
			mSettings[NX_DUMP_MATERIAL].store((NxReal)(int)desc.materialIndex, 0.0f, 0.0f, 0.0f);
			mSettings[NX_DUMP_GROUP].store((NxReal)(int)desc.group, 0.0f, 0.0f, 0.0f);
			fprintf(file, "PsBox ");
			mSettings[NX_DUMP_SIDES].print();
			mSettings[NX_DUMP_LOCALPOSITION].print();
			mSettings[NX_DUMP_LOCALORIENTATION].print();
			mSettings[NX_DUMP_MATERIAL].print();
			mSettings[NX_DUMP_GROUP].print();
			writeTriggerFlags(file, desc.shapeFlags);
			break;
			}
		case NX_SHAPE_CAPSULE:
			{
			NxCapsuleShapeDesc desc;
			sceneDumpShapeIs(shape, NX_SHAPE_CAPSULE)->saveToDesc(desc);
			sceneDumpQuat(desc.localPose.M, SCENE_DUMP_QUAT_08_WIDE, q);
			mSettings[NX_DUMP_LOCALPOSITION].store(desc.localPose.t.x, desc.localPose.t.y, desc.localPose.t.z, 0.0f);
			mSettings[NX_DUMP_LOCALORIENTATION].store(q[0], q[1], q[2], q[3]);
			mSettings[NX_DUMP_RADIUS].store(desc.radius, 0.0f, 0.0f, 0.0f);
			mSettings[NX_DUMP_HEIGHT].store(desc.height, 0.0f, 0.0f, 0.0f);
			mSettings[NX_DUMP_MATERIAL].store((NxReal)(int)desc.materialIndex, 0.0f, 0.0f, 0.0f);
			mSettings[NX_DUMP_GROUP].store((NxReal)(int)desc.group, 0.0f, 0.0f, 0.0f);
			fprintf(file, "PsCapsule ");
			mSettings[NX_DUMP_HEIGHT].print();
			mSettings[NX_DUMP_RADIUS].print();
			mSettings[NX_DUMP_LOCALPOSITION].print();
			mSettings[NX_DUMP_LOCALORIENTATION].print();
			mSettings[NX_DUMP_MATERIAL].print();
			mSettings[NX_DUMP_GROUP].print();
			writeTriggerFlags(file, desc.flags);
			break;
			}
		case NX_SHAPE_MESH:
			{
			NxTriangleMeshShapeDesc desc;
			SceneDumpShape* meshShape = sceneDumpShapeIs(shape, NX_SHAPE_MESH);
			meshShape->saveToDesc(desc);
			// The public mesh's +4 is the internal TriangleMesh.
			TriangleMesh* mesh = *reinterpret_cast<TriangleMesh**>(reinterpret_cast<NxU8*>(desc.meshData) + 4);
			const bool isConvex = mesh->mConvexMesh != 0;
			SceneDumpTriangleMesh* meshTable = reinterpret_cast<SceneDumpTriangleMesh*>(mesh);
			meshTable->hasPMap();
			NxTriangleMeshDesc meshDesc;
			meshTable->saveToDesc(meshDesc);
			const char* name = writeMesh(file, mesh, meshShape, binary, isConvex, meshTable->getPMapDensity(), names);
			sceneDumpQuat(desc.localPose.M, SCENE_DUMP_QUAT_08_STORED, q);
			mSettings[NX_DUMP_LOCALPOSITION].store(desc.localPose.t.x, desc.localPose.t.y, desc.localPose.t.z, 0.0f);
			mSettings[NX_DUMP_LOCALORIENTATION].store(q[0], q[1], q[2], q[3]);
			mSettings[NX_DUMP_MATERIAL].store((NxReal)(int)desc.materialIndex, 0.0f, 0.0f, 0.0f);
			mSettings[NX_DUMP_GROUP].store((NxReal)(int)desc.group, 0.0f, 0.0f, 0.0f);
			if(isConvex)
				fprintf(file, "PsConvex %s ", name);
			else
				fprintf(file, "PsTriangleMesh %s ", name);
			mSettings[NX_DUMP_LOCALPOSITION].print();
			mSettings[NX_DUMP_LOCALORIENTATION].print();
			mSettings[NX_DUMP_MATERIAL].print();
			mSettings[NX_DUMP_GROUP].print();
			writeTriggerFlags(file, desc.shapeFlags);
			break;
			}
		default:
			break;
		}
	}

// phys_fn_004051 (0x00094130, 409 B)
// With its continuations phys_fn_004053 (0x000942d0, 42 B, the settings
// initialisation), phys_fn_004055 (0x00094300, 1977 B, one actor through its
// shapes), phys_fn_004057 (0x00094ac0, 733 B, the multi-shape loop and the
// actor line), phys_fn_004059 (0x00094da0, 378 B, the pair flags) and the
// effectors' phys_fn_004061 (0x00094f20, 464 B). In order:
//   * the asset banner and the timing header -- the scene's +0x544 (last
//     frame) equal to 0.0 means `initial configuration`; otherwise +0x53c,
//     +0x544 and +0x52c as text tokens (binary false), +0x530 and +0x534;
//   * PsGravity (000509);
//   * a joint block (004037) per joint of the scene iterator (000563/567);
//   * the 24 setting records reset (kind, first, file, binary, pending set;
//     values left as they are);
//   * per actor of the Scene's array (+0x55c..+0x560) -- the count is the
//     `sar` quotient and the loop counts it down -- through its body (NxActor
//     +0x14): the name (004006), the pose (NxActorDescBase, whose empty
//     constructor leaves only globalPose's identity), position, orientation
//     and density; when saveBodyToDesc (NxActor slot 81, `[vt+0x144]`) returns
//     true, the NxBodyDesc records 10, 11, 12, 13, 22 (the solver count as an
//     unsigned widened to float), 14, 15, 18, 19, 20, 21, in that order; then
//     the shapes: exactly one continues on the actor's line (004048); any
//     other count, 0 included, takes the next shape-group number, saves the
//     records, writes the group's block, restores them and starts the line
//     with `PsShape Shape%d `. The line: name, `awake(false) ` when the body
//     has no record or its +0x4c wake counter is the word 0 (`test edx,edx`,
//     so -0.0 is awake), records 0-2, then for a static actor
//     `static(true) `, for a dynamic one records 10-22 as stored and
//     `kinematic(true) ` (flags bit 7), `locked(true) ` (bits 1-6 all set) or
//     `locked(...) ` of bits 1-6 when some are set; `collision(false) ` for
//     NX_AF_DISABLE_COLLISION; the line end. The saved actor and body
//     descriptors are both read through the body's NxActor (+0), not through
//     the array's pointer;
//   * a PsJoint line (004015) per joint, the iterator reset again;
//   * the pair flags when 000523 counts any: a 0xc-byte record each through
//     operator new, filled by 000525; an actor pair (bit 31) whose actors
//     both have a body is written as it is; a shape pair whose shapes both
//     have an internal shape (+8) is written by the shapes' owners (001281),
//     once per unordered pair of owners (two parallel operator-new arrays,
//     freed through `free`); the records are released through operator
//     delete (005700) when the allocation returned one;
//   * per effector of the +0x5a4 list, as a spring-and-damper effector: its
//     spring (003974), damper (003975) and ends (003964, which reads both
//     records' +0x19c before any null test: an effector with a world end
//     crashes the oracle there); written when either end's body has a
//     record, with `%f` of the promoted floats and `\n` line ends, the
//     anchors only for a non-null end;
//   * PsAssetEnd and the closing banner. Returns true.
bool SceneDump::writeAsset(NxSceneInternal* scene, FILE* file, bool binary, NxU32 index, SceneDumpNames* names)
	{
	(void)index;
	if(!file)
		return false;
	fprintf(file, "###########################################################\r\n");
	fprintf(file, "#### Asset Information\r\n");
	fprintf(file, "###########################################################\r\n");
	if(scene->at<NxReal>(0x544) == 0.0f)
		{
		fprintf(file, "## Scene in initial configuration.\r\n");
		}
	else
		{
		fprintf(file, "## Total Elapsed Time: %s\r\n", sceneDumpToken(scene->at<NxReal>(0x53c), false));
		fprintf(file, "## Elapsed Time Last Frame: %s\r\n", sceneDumpToken(scene->at<NxReal>(0x544), false));
		fprintf(file, "## MaxTimeStep: %s\r\n", sceneDumpToken(scene->at<NxReal>(0x52c), false));
		fprintf(file, "## MaxIter: %d\r\n", scene->at<NxU32>(0x530));
		if(scene->at<NxU32>(0x534) == 0)
			fprintf(file, "## TimeStep = FIXED\r\n");
		else
			fprintf(file, "## TimeStep = VARIABLE\r\n");
		}
	fprintf(file, "###########################################################\r\n");
	NxVec3 gravity;
	scene->getGravity(gravity);
	fprintf(file, "PsGravity %s %s %s\r\n", sceneDumpToken(gravity.x, binary), sceneDumpToken(gravity.y, binary),
		sceneDumpToken(gravity.z, binary));
	scene->resetJointIterator();
	for(Joint* joint = scene->getNextJoint(); joint; joint = scene->getNextJoint())
		writeJoint(file, joint, binary);

	NxActor** actors = scene->at<NxActor**>(0x55c);
	int nbActors = (int)(scene->at<NxActor**>(0x560) - actors);
	for(int kind = 0; kind < NX_DUMP_SETTING_COUNT; kind++)
		{
		SceneDumpSetting& setting = mSettings[kind];
		setting.mKind = kind;
		setting.mFirst = true;
		setting.mFile = file;
		setting.mBinary = binary;
		setting.mPending = true;
		}
	for(; nbActors != 0; nbActors--)
		{
		NxActor* actor = *actors++;
		JointActorBody* body = *reinterpret_cast<JointActorBody**>(reinterpret_cast<NpActorObject*>(actor)->at(0x14));
		char name[0x200];
		actorName(name, body);
		NxReal q[4];
		NxActorDescBase actorDesc;
		body->mActor->saveToDesc(actorDesc);
		sceneDumpQuat(actorDesc.globalPose.M, SCENE_DUMP_QUAT_84_WIDE, q);
		mSettings[NX_DUMP_POSITION].store(actorDesc.globalPose.t.x, actorDesc.globalPose.t.y, actorDesc.globalPose.t.z,
			0.0f);
		mSettings[NX_DUMP_ORIENTATION].store(q[0], q[1], q[2], q[3]);
		mSettings[NX_DUMP_DENSITY].store(actorDesc.density, 0.0f, 0.0f, 0.0f);
		NxBodyDesc bodyDesc;
		const bool dynamic = body->mActor->saveBodyToDesc(bodyDesc);
		if(dynamic)
			{
			sceneDumpQuat(bodyDesc.massLocalPose.M, SCENE_DUMP_QUAT_84_WIDE, q);
			mSettings[NX_DUMP_COM].store(bodyDesc.massLocalPose.t.x, bodyDesc.massLocalPose.t.y,
				bodyDesc.massLocalPose.t.z, 0.0f);
			mSettings[NX_DUMP_COMROT].store(q[0], q[1], q[2], q[3]);
			mSettings[NX_DUMP_INERTIA].store(bodyDesc.massSpaceInertia.x, bodyDesc.massSpaceInertia.y,
				bodyDesc.massSpaceInertia.z, 0.0f);
			mSettings[NX_DUMP_MASS].store(bodyDesc.mass, 0.0f, 0.0f, 0.0f);
			mSettings[NX_DUMP_SOLVERCOUNT].store((NxReal)bodyDesc.solverIterationCount, 0.0f, 0.0f, 0.0f);
			mSettings[NX_DUMP_VELOCITY].store(bodyDesc.linearVelocity.x, bodyDesc.linearVelocity.y,
				bodyDesc.linearVelocity.z, 0.0f);
			mSettings[NX_DUMP_ANGULARVELOCITY].store(bodyDesc.angularVelocity.x, bodyDesc.angularVelocity.y,
				bodyDesc.angularVelocity.z, 0.0f);
			mSettings[NX_DUMP_WAKEUPCOUNTER].store(bodyDesc.wakeUpCounter, 0.0f, 0.0f, 0.0f);
			mSettings[NX_DUMP_LINEARDAMPING].store(bodyDesc.linearDamping, 0.0f, 0.0f, 0.0f);
			mSettings[NX_DUMP_ANGULARDAMPING].store(bodyDesc.angularDamping, 0.0f, 0.0f, 0.0f);
			mSettings[NX_DUMP_MAXANGULARVELOCITY].store(bodyDesc.maxAngularVelocity, 0.0f, 0.0f, 0.0f);
			}
		const NxU32 nbShapes = body->getNbShapes();
		void** shapes = body->getShapes();
		if(nbShapes == 1)
			{
			writeShape(file, static_cast<NxCollisionShape*>(shapes[0]), binary, names);
			}
		else
			{
			const NxU32 group = ++names->mShapeCount;
			memcpy(mSaved, mSettings, sizeof(mSettings));
			fprintf(file, "PsShapeBegin Shape%d\r\n", group);
			for(NxU32 i = 0; i < nbShapes; i++)
				{
				writeShape(file, static_cast<NxCollisionShape*>(shapes[i]), binary, names);
				fprintf(file, "\r\n");
				}
			fprintf(file, "PsShapeEnd\r\n");
			memcpy(mSettings, mSaved, sizeof(mSettings));
			fprintf(file, "PsShape Shape%d ", group);
			}
		fprintf(file, "name(%s) ", name);
		JointBodyRecord* record = body->mBody;
		if(!record || sceneDumpBits(record->mWakeUpCounter) == 0)
			fprintf(file, "awake(false) ");
		mSettings[NX_DUMP_POSITION].print();
		mSettings[NX_DUMP_ORIENTATION].print();
		mSettings[NX_DUMP_DENSITY].print();
		if(!dynamic)
			{
			fprintf(file, "static(true) ");
			}
		else
			{
			mSettings[NX_DUMP_COM].print();
			mSettings[NX_DUMP_COMROT].print();
			mSettings[NX_DUMP_INERTIA].print();
			mSettings[NX_DUMP_MASS].print();
			mSettings[NX_DUMP_SOLVERCOUNT].print();
			mSettings[NX_DUMP_VELOCITY].print();
			mSettings[NX_DUMP_ANGULARVELOCITY].print();
			mSettings[NX_DUMP_WAKEUPCOUNTER].print();
			mSettings[NX_DUMP_LINEARDAMPING].print();
			mSettings[NX_DUMP_ANGULARDAMPING].print();
			mSettings[NX_DUMP_MAXANGULARVELOCITY].print();
			const NxU32 flags = bodyDesc.flags;
			const NxU32 frozen = NX_BF_FROZEN_POS_X | NX_BF_FROZEN_POS_Y | NX_BF_FROZEN_POS_Z | NX_BF_FROZEN_ROT_X
				| NX_BF_FROZEN_ROT_Y | NX_BF_FROZEN_ROT_Z;
			if(flags & NX_BF_KINEMATIC)
				{
				fprintf(file, "kinematic(true) ");
				}
			else if(flags & frozen)
				{
				if((flags & frozen) == frozen)
					fprintf(file, "locked(true) ");
				else
					fprintf(file, "locked(%s,%s,%s,%s,%s,%s) ", (flags & NX_BF_FROZEN_POS_X) ? "true" : "false",
						(flags & NX_BF_FROZEN_POS_Y) ? "true" : "false", (flags & NX_BF_FROZEN_POS_Z) ? "true" : "false",
						(flags & NX_BF_FROZEN_ROT_X) ? "true" : "false", (flags & NX_BF_FROZEN_ROT_Y) ? "true" : "false",
						(flags & NX_BF_FROZEN_ROT_Z) ? "true" : "false");
				}
			}
		if(actorDesc.flags & NX_AF_DISABLE_COLLISION)
			fprintf(file, "collision(false) ");
		fprintf(file, "\r\n");
		}

	scene->resetJointIterator();
	for(Joint* joint = scene->getNextJoint(); joint; joint = scene->getNextJoint())
		writeJointLine(file, joint, binary);

	const NxU32 nbPairs = scene->getNbPairs();
	if(nbPairs)
		{
		NxPairFlag* pairs = static_cast<NxPairFlag*>(::operator new(nbPairs * sizeof(NxPairFlag)));
		if(scene->getPairFlagArray(pairs, nbPairs))
			{
			const void** firsts = static_cast<const void**>(::operator new(nbPairs * 4));
			const void** seconds = static_cast<const void**>(::operator new(nbPairs * 4));
			int nbShapePairs = 0;
			for(NxU32 i = 0; i < nbPairs; i++)
				{
				const NxPairFlag& pair = pairs[i];
				if(pair.isActorPair())
					{
					JointActorBody* body0 = *reinterpret_cast<JointActorBody**>(static_cast<NxU8*>(pair.objects[0]) + 0x14);
					JointActorBody* body1 = *reinterpret_cast<JointActorBody**>(static_cast<NxU8*>(pair.objects[1]) + 0x14);
					if(body0 && body1)
						{
						char name0[0x200];
						char name1[0x200];
						actorName(name0, body0);
						actorName(name1, body1);
						fprintf(file, "PsActorPair %s %s false\r\n", name0, name1);
						}
					}
				else
					{
					const NxCollisionShape* shape0
						= *reinterpret_cast<NxCollisionShape**>(static_cast<NxU8*>(pair.objects[0]) + 8);
					const NxCollisionShape* shape1
						= *reinterpret_cast<NxCollisionShape**>(static_cast<NxU8*>(pair.objects[1]) + 8);
					if(shape0 && shape1)
						{
						const void* owner0 = NxShapeOwner(shape0, 0);
						const void* owner1 = NxShapeOwner(shape1, 0);
						int k = 0;
						for(; k < nbShapePairs; k++)
							{
							if((firsts[k] == owner0 && seconds[k] == owner1) || (firsts[k] == owner1 && seconds[k] == owner0))
								break;
							}
						if(k == nbShapePairs)
							{
							firsts[nbShapePairs] = owner0;
							seconds[nbShapePairs] = owner1;
							nbShapePairs++;
							char name0[0x200];
							char name1[0x200];
							actorName(name0, static_cast<JointActorBody*>(const_cast<void*>(owner0)));
							actorName(name1, static_cast<JointActorBody*>(const_cast<void*>(owner1)));
							fprintf(file, "PsActorPair %s %s false\r\n", name0, name1);
							}
						}
					}
				}
			free(firsts);
			free(seconds);
			}
		if(pairs)
			::operator delete(pairs);
		}

	for(Effector* effector = scene->at<Effector*>(0x5a4); effector; effector = effector->mNext)
		{
		SpringAndDamperEffector* spring = static_cast<SpringAndDamperEffector*>(effector);
		NxReal distCompressSaturate, distRelaxed, distStretchSaturate, maxCompressForce, maxStretchForce;
		spring->getLinearSpring(distCompressSaturate, distRelaxed, distStretchSaturate, maxCompressForce, maxStretchForce);
		NxReal velCompressSaturate, velStretchSaturate, damperMaxCompressForce, damperMaxStretchForce;
		spring->getLinearDamper(velCompressSaturate, velStretchSaturate, damperMaxCompressForce, damperMaxStretchForce);
		void* owner1;
		void* owner2;
		NxVec3 pos1;
		NxVec3 pos2;
		spring->getBodies(&owner1, pos1, &owner2, pos2);
		JointActorBody* body1 = static_cast<JointActorBody*>(owner1);
		JointActorBody* body2 = static_cast<JointActorBody*>(owner2);
		if(body1->mBody || body2->mBody)
			{
			char name1[0x200];
			char name2[0x200];
			actorName(name1, body1);
			actorName(name2, body2);
			fprintf(file, "PsDefaultSettings spring_dist_relaxed(%f)\n", (double)distRelaxed);
			fprintf(file, "PsDefaultSettings spring_dist_compress_saturate(%f) spring_dist_stretch_saturate(%f)\n",
				(double)distCompressSaturate, (double)distStretchSaturate);
			fprintf(file, "PsDefaultSettings spring_max_compress_force(%f) spring_max_stretch_force(%f)\n",
				(double)maxCompressForce, (double)maxStretchForce);
			fprintf(file, "PsDefaultSettings spring_vel_compress_saturate(%f) spring_vel_stretch_saturate(%f)\n",
				(double)velCompressSaturate, (double)velStretchSaturate);
			fprintf(file,
				"PsDefaultSettings spring_damper_max_compress_force(%f) spring_damper_max_stretch_force(%f)\n",
				(double)damperMaxCompressForce, (double)damperMaxStretchForce);
			if(body1)
				fprintf(file, "PsDefaultSettings spring_pos1(%f,%f,%f)\n", (double)pos1.x, (double)pos1.y, (double)pos1.z);
			if(body2)
				fprintf(file, "PsDefaultSettings spring_pos2(%f,%f,%f)\n", (double)pos2.x, (double)pos2.y, (double)pos2.z);
			fprintf(file, "PsSpring %s %s\n", name1, name2);
			}
		}
	fprintf(file, "PsAssetEnd\r\n");
	fprintf(file, "##################################################################################\r\n\r\n");
	return true;
	}

// phys_fn_004062 (0x000950f0, 1699 B)
// PhysicsSDK::coreDump, entered from NpPhysicsSDK::coreDump (phys_fn_000267)
// with this = the PhysicsSDK and every scene locked. The frame holds the
// three name counters (SceneDumpNames; the mesh table is the 0x20000-byte
// block allocated first, through operator new, and released through free on
// both paths, 0x9577d), the 512-byte file name, the date and time buffers
// and the settings object the asset writer runs on. A failed open skips
// everything but the free. The file: header, one block per SDK material
// (read from PhysicsSDK::instance, the index taken as an NxMaterialIndex and
// replaced by 0 past the end; no bit-31 test, unlike getMaterial), then per
// scene the asset header, the 13 live SDK parameters, the collision-group
// masks that are not 0xffffffff (with their banner only when there is one),
// and the asset (004051); then the scene list, `PsSetScene 0` and the user
// addendum. Returns false on both paths (`xor al, al`, 0x95787).
bool PhysicsSDK::coreDump(const char* fname, bool binary, const char* addendum)
	{
	SceneDumpNames names;
	names.mShapeCount = 0;
	names.mMeshCount = 0;
	names.mMeshes = static_cast<const void**>(::operator new(0x20000));
	char fileName[0x200];
	sprintf(fileName, "%s.psc", fname);
	FILE* file = fopen(fileName, "wb");
	if(file)
		{
		char date[0x200];
		char clock[0x200];
		SceneDump dump;
		const NxU32 nbScenes = mScenes.size();
		fprintf(file, "################################################################\r\n");
		fprintf(file, "### Core Dump from Novodex Physics SDK\r\n");
		sceneDumpDateTime(date, clock);
		fprintf(file, "### Core Dump Generated on %s at %s\r\n", date, clock);
		if(nbScenes == 1)
			fprintf(file, "### Contains one Asset.\r\n");
		else
			fprintf(file, "### Contains %d assets.\r\n", nbScenes);
		fprintf(file, "PsReset\r\n");
		fprintf(file, "PsVersion 1.4\r\n");
		sprintf(gSceneDumpName, "%s__%I64x", "PhysicsSDK", sceneDumpKey(this));
		fprintf(file, "PsNameSpace %s\r\n\r\n", gSceneDumpName);

		const int nbMaterials = mMaterials.size();
		for(int i = 0; i < nbMaterials; i++)
			{
			NxMaterial* materials = instance->mMaterials.begin();
			const NxMaterialIndex index = (NxMaterialIndex)i;
			const NxMaterial* material = index < instance->mMaterials.size() ? materials + index : materials;
			fprintf(file, "\r\n### Begin Material Definition ####\r\n");
			fprintf(file, "PsMatBegin mat%d\r\n", i + 1);
			fprintf(file, "PsMatDynamicFriction %s\r\n", sceneDumpToken(material->dynamicFriction, binary));
			fprintf(file, "PsMatStaticFriction %s\r\n", sceneDumpToken(material->staticFriction, binary));
			fprintf(file, "PsMatSpinFriction %s\r\n", sceneDumpToken(material->spinFriction, binary));
			fprintf(file, "PsMatRollFriction %s\r\n", sceneDumpToken(material->rollFriction, binary));
			fprintf(file, "PsMatRestitution %s\r\n", sceneDumpToken(material->restitution, binary));
			fprintf(file, "PsMatDynamicFrictionV %s\r\n", sceneDumpToken(material->dynamicFrictionV, binary));
			fprintf(file, "PsMatStaticFrictionV %s\r\n", sceneDumpToken(material->staticFrictionV, binary));
			fprintf(file, "PsMatDirOfAnisotropy %s %s %s\r\n", sceneDumpToken(material->dirOfAnisotropy.x, binary),
				sceneDumpToken(material->dirOfAnisotropy.y, binary), sceneDumpToken(material->dirOfAnisotropy.z, binary));
			fprintf(file, "PsMatDirOfMotion %s %s %s\r\n", sceneDumpToken(material->dirOfMotion.x, binary),
				sceneDumpToken(material->dirOfMotion.y, binary), sceneDumpToken(material->dirOfMotion.z, binary));
			fprintf(file, "PsMatSpeedOfMotion %s\r\n", sceneDumpToken(material->speedOfMotion, binary));
			fprintf(file, (material->flags & NX_MF_ANISOTROPIC) ? "PsMatAnisotropic true\r\n" : "PsMatAnisotropic false\r\n");
			fprintf(file, (material->flags & NX_MF_MOVING_SURFACE) ? "PsMatMovingSurface true\r\n"
				: "PsMatMovingSurface false\r\n");
			fprintf(file, "PsMatEnd\r\n");
			fprintf(file, "### End Material Definition ####\r\n\r\n");
			}

		Scene** scenes = mScenes.begin();
		for(NxU32 i = 0; i < nbScenes; i++)
			{
			Scene* scene = scenes[i];
			sprintf(gSceneDumpName, "%s__%I64x", "Asset", sceneDumpKey(scene));
			fprintf(file, "PsAssetBegin %s\r\n", gSceneDumpName);
			fprintf(file, "################################################################\r\n");
			fprintf(file, "### Novodex Specific Physics Simulation Constants\r\n");
			fprintf(file, "################################################################\r\n");
			const NxReal* parameters = nxPhysicsSDKParameters();
			fprintf(file, "PsSetConstant %s %s\r\n", "NX_PENALTY_FORCE", sceneDumpToken(parameters[NX_PENALTY_FORCE], binary));
			fprintf(file, "PsSetConstant %s %s\r\n", "NX_MIN_SEPARATION_FOR_PENALTY",
				sceneDumpToken(parameters[NX_MIN_SEPARATION_FOR_PENALTY], binary));
			fprintf(file, "PsSetConstant %s %s\r\n", "NX_DEFAULT_SLEEP_LIN_VEL_SQUARED",
				sceneDumpToken(parameters[NX_DEFAULT_SLEEP_LIN_VEL_SQUARED], binary));
			fprintf(file, "PsSetConstant %s %s\r\n", "NX_DEFAULT_SLEEP_ANG_VEL_SQUARED",
				sceneDumpToken(parameters[NX_DEFAULT_SLEEP_ANG_VEL_SQUARED], binary));
			fprintf(file, "PsSetConstant %s %s\r\n", "NX_BOUNCE_TRESHOLD", sceneDumpToken(parameters[NX_BOUNCE_TRESHOLD], binary));
			fprintf(file, "PsSetConstant %s %s\r\n", "NX_DYN_FRICT_SCALING",
				sceneDumpToken(parameters[NX_DYN_FRICT_SCALING], binary));
			fprintf(file, "PsSetConstant %s %s\r\n", "NX_STA_FRICT_SCALING",
				sceneDumpToken(parameters[NX_STA_FRICT_SCALING], binary));
			fprintf(file, "PsSetConstant %s %s\r\n", "NX_MAX_ANGULAR_VELOCITY",
				sceneDumpToken(parameters[NX_MAX_ANGULAR_VELOCITY], binary));
			fprintf(file, "PsSetConstant %s %s\r\n", "NX_MESH_MESH_LEVEL", sceneDumpToken(parameters[NX_MESH_MESH_LEVEL], binary));
			fprintf(file, "PsSetConstant %s %s\r\n", "NX_ENABLE_MESH_DEBUG",
				sceneDumpToken(parameters[NX_ENABLE_MESH_DEBUG], binary));
			fprintf(file, "PsSetConstant %s %s\r\n", "NX_COLL_INFINITY", sceneDumpToken(parameters[NX_COLL_INFINITY], binary));
			fprintf(file, "PsSetConstant %s %s\r\n", "NX_CONTINUOUS_CD", sceneDumpToken(parameters[NX_CONTINUOUS_CD], binary));
			fprintf(file, "PsSetConstant %s %s\r\n", "NX_MESH_HINT_SPEED", sceneDumpToken(parameters[NX_MESH_HINT_SPEED], binary));
			fprintf(file, "################################################################\r\n\r\n");

			const NxU32* masks = nxPhysicsSDKGroupCollisionMasks();
			bool first = true;
			for(int group = 0; group < 32; group++)
				{
				if(masks[group] == 0xffffffff)
					continue;
				if(first)
					{
					fprintf(file, "################################################################\r\n");
					fprintf(file, "### Collision Group Flags\r\n");
					fprintf(file, "################################################################\r\n");
					first = false;
					}
				fprintf(file, "PsGroupCollisionFlag %d %08X\r\n", group, masks[group]);
				}
			if(!first)
				fprintf(file, "################################################################\r\n\r\n");

			dump.writeAsset(reinterpret_cast<NxSceneInternal*>(scene), file, binary, i, &names);
			}

		fprintf(file, "\r\n");
		for(NxU32 i = 0; i < nbScenes; i++)
			{
			Scene* scene = scenes[i];
			fprintf(file, "PsSetScene %d\r\n", i);
			sprintf(gSceneDumpName, "%s__%I64x", "Asset", sceneDumpKey(scene));
			fprintf(file, "PsAsset %s position(0,0,0) orientation(0,0,0,1)\r\n", gSceneDumpName);
			fprintf(file, "\r\n");
			}
		fprintf(file, "PsSetScene 0\r\n");
		if(addendum)
			{
			fprintf(file, "### Begin : User supplied addendum script.\r\n");
			fprintf(file, "%s\r\n", addendum);
			fprintf(file, "### End   : User supplied addendum script.\r\n");
			}
		fclose(file);
		}
	free(names.mMeshes);
	return false;
	}
