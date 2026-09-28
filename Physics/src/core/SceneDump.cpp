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
#include "PhysicsSDK.h"
#include "NpActor.h"
#include "NxJoint.h"
#include "NxPrismaticJointDesc.h"
#include "NxRevoluteJointDesc.h"
#include "NxCylindricalJointDesc.h"
#include "NxSphericalJointDesc.h"
#include "NxPointOnLineJointDesc.h"
#include "NxPointInPlaneJointDesc.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cfloat>
#include <cmath>
#include <ctime>
#include <new>

// The scene core-dump writer (effector-and-coredump Task 3a: the rows
// through the joint blocks and the entry; Task 3b adds the asset writer, the
// actors, shapes, meshes and effectors). The contract is
// units/effector-coredump-contract.md "## Core dump".
//
// Output goes through the static CRT exactly where the oracle calls it:
// fopen (phys_fn_005671, `_fsopen(name, "wb", _SH_DENYNO)`), fprintf
// (005716) for every line, sprintf (005739) for names and tokens, strstr
// (005732) in the float token, fclose (005673); strftime (005794) and the
// time rows (005766, 005775, 005802) for the header date. No fputs, no fwrite.
// NxPhysics.dll links legacy_stdio_float_rounding.obj, so "%f"/"%.9f" round
// as the oracle's CRT does.
//
// Floating point: the rows here do no x87 arithmetic of their own -- they
// compare floats (fucompp), widen them for sprintf (fld dword; fstp qword)
// and round three setting kinds with fistp at the live control word -- but
// the asset writer Task 3b adds converts the actor pose to a quaternion with
// fsqrt, so this translation unit is on the /arch:IA32 list with the joint
// files. It is also built /EHs-c-: the joint block keeps descriptors with
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

// phys_fn_003985 (0x0008fc50, 92 B)
// Compiler-generated: NxJointDesc::setToDefault (NxJointDesc.h), slot 1 of
// 0x10117a44 and of the revolute, spherical, cylindrical, prismatic,
// point-on-line and point-in-plane descriptor tables writeJoint installs
// (0x101182a4, 0x101182c0, 0x10118280, 0x101182b4, 0x10118298, 0x1011828c).
// Emitted from the header as 003981 is.

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

// The per-scene asset writer phys_fn_004051 and the rows below it (the
// actors, shapes, meshes and effectors) are Task 3b's. Until then the entry
// point is not reachable (NpPhysicsSDK::coreDump, phys_fn_000267, is still
// the candidate stub that returns false; Task 4 wires it), and this
// placeholder asserts.
bool SceneDump::writeAsset(NxSceneInternal* scene, FILE* file, bool binary, NxU32 index, SceneDumpNames* names)
	{
	(void)scene; (void)file; (void)binary; (void)index; (void)names;
	NX_ASSERT(0);
	return false;
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
