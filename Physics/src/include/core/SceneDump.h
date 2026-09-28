#ifndef NX_PHYSICS_CORE_SCENEDUMP
#define NX_PHYSICS_CORE_SCENEDUMP
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "Nxp.h"
#include "NxJointLimitPairDesc.h"
#include "NxMotorDesc.h"

#include <cstdio>
#include <cstddef>

// The scene core-dump (.psc) writer behind NxPhysicsSDK::coreDump: the entry
// PhysicsSDK::coreDump (phys_fn_004062) and the rows it and the per-scene
// asset writer call. The oracle keeps these rows in the unnamed gap between
// NpSpringAndDamperEffector.cpp and Joint.cpp, and no string in them names a
// file (none reports an error), so the unit is named by what it does. See
// units/effector-coredump-contract.md "## Core dump" and "### Task 3a record".

class Joint;
class NxSceneInternal;
class NxJointDesc;
struct JointActorBody;

//! The 24 setting kinds of a SceneDumpSetting (the jump tables of
//! phys_fn_003997 at 0x10090680 and phys_fn_003999 at 0x10090d48).
enum SceneDumpSettingKind
	{
	NX_DUMP_POSITION			= 0,
	NX_DUMP_ORIENTATION			= 1,
	NX_DUMP_DENSITY				= 2,
	NX_DUMP_SIDES				= 3,
	NX_DUMP_LOCALPOSITION		= 4,
	NX_DUMP_LOCALORIENTATION	= 5,
	NX_DUMP_PLANE				= 6,
	NX_DUMP_HEIGHT				= 7,
	NX_DUMP_RADIUS				= 8,
	NX_DUMP_MATERIAL			= 9,
	NX_DUMP_COM					= 10,
	NX_DUMP_COMROT				= 11,
	NX_DUMP_INERTIA				= 12,
	NX_DUMP_MASS				= 13,
	NX_DUMP_VELOCITY			= 14,
	NX_DUMP_ANGULARVELOCITY		= 15,
	NX_DUMP_FORCE				= 16,
	NX_DUMP_TORQUE				= 17,
	NX_DUMP_WAKEUPCOUNTER		= 18,
	NX_DUMP_LINEARDAMPING		= 19,
	NX_DUMP_ANGULARDAMPING		= 20,
	NX_DUMP_MAXANGULARVELOCITY	= 21,
	NX_DUMP_SOLVERCOUNT			= 22,
	NX_DUMP_GROUP				= 23,
	NX_DUMP_SETTING_COUNT		= 24
	};

//! One setting record, 0x1c bytes. The asset writer (phys_fn_004051) sets
//! mKind, mFirst, mPending, mBinary and mFile of all 24 at its start and
//! leaves mValue as it is (the first store overwrites it).
struct SceneDumpSetting
	{
	//! phys_fn_003997 (0x0008ffa0, 1757 B). Stores a value (four floats,
	//! `ret 0x10`); returns mEmit.
	bool store(NxReal v0, NxReal v1, NxReal v2, NxReal v3);

	//! phys_fn_003999 (0x000906e0, 1637 B). Writes the stored value inline
	//! when mEmit is set; returns mEmit.
	bool print();

	NxU32		mKind;			//!< +0x00, a SceneDumpSettingKind
	bool		mFirst;			//!< +0x04
	bool		mEmit;			//!< +0x05
	bool		mBinary;		//!< +0x06, the float token's mode
	bool		mPending;		//!< +0x07
	NxReal		mValue[4];		//!< +0x08
	FILE*		mFile;			//!< +0x18
	};

static_assert(sizeof(SceneDumpSetting) == 0x1c, "a setting record is 0x1c bytes");
static_assert(offsetof(SceneDumpSetting, mValue) == 0x08, "value at +0x08");
static_assert(offsetof(SceneDumpSetting, mFile) == 0x18, "stream at +0x18");

//! The counters phys_fn_004062 keeps in its frame (0x1c, 0x20, 0x24) and
//! hands to the asset writer by address: the shape-group counter
//! (`PsShapeBegin Shape%d`, shared across scenes), and the mesh-name table
//! phys_fn_003991 reads and grows (+4 count, +8 array; `tmesh%d`, capped at
//! 0x7fff). The array is the 0x20000-byte block 004062 allocates first:
//! 0x8000 pointers.
struct SceneDumpNames
	{
	NxU32		mShapeCount;	//!< +0x00
	NxU32		mMeshCount;		//!< +0x04
	const void**	mMeshes;	//!< +0x08
	};

static_assert(sizeof(SceneDumpNames) == 0x0c, "the three frame words 0x1c..0x24");

//! The settings object phys_fn_004062 keeps in its frame and passes as `this`
//! to the asset writer, 0x540 bytes: the 24 records and a saved copy of them
//! (the asset writer saves and restores the block around a multi-shape
//! actor). No row constructs it; it has no constructor.
class SceneDump
	{
	public:
	//! phys_fn_004002 (0x00090db0, 92 B). True when the name holds a
	//! delimiter the script syntax reserves.
	bool hasDelimiter(const char* name);

	//! phys_fn_004004 (0x00090e10, 196 B). The joint's script name, in the
	//! joint-name buffer .data 0x10126d80.
	const char* jointName(Joint* joint);

	//! phys_fn_004006 (0x00090ee0, 208 B). The actor's script name, written
	//! to `out`; `@world` for a null actor body.
	void actorName(char* out, JointActorBody* body);

	//! phys_fn_004007 (0x00090fb0, 630 B). The frame lines of a joint block.
	void writeJointFrames(const NxJointDesc& desc, FILE* file, Joint* joint, bool binary);

	//! phys_fn_004009 (0x00091230, 121 B). "%s,%s,%s,  %s,%s,%s" of a limit pair.
	const char* limitPairText(const NxJointLimitPairDesc& limit, bool binary);

	//! phys_fn_004011 (0x000912b0, 79 B). "%s,%s,%s" of three floats: the
	//! listing passes an NxJointLimitDesc (swing limit) and NxSpringDesc
	//! (the springs) alike.
	const char* tripleText(const NxReal* values, bool binary);

	//! phys_fn_004013 (0x00091300, 83 B). "%s,%s,%s" of a motor.
	const char* motorText(const NxMotorDesc& motor, bool binary);

	//! phys_fn_004015 (0x00091360, 135 B). `PsJoint <joint> <actor0> <actor1>`.
	//! The third argument is not read.
	void writeJointLine(FILE* file, Joint* joint, bool binary);

	//! phys_fn_004037 (0x00092060, 409 B), with its continuations
	//! phys_fn_004039, phys_fn_004041 and phys_fn_004043. One joint block.
	void writeJoint(FILE* file, Joint* joint, bool binary);

	//! phys_fn_004051 (0x00094130, 409 B). The per-scene asset writer
	//! (effector-and-coredump Task 3b).
	bool writeAsset(NxSceneInternal* scene, FILE* file, bool binary, NxU32 index, SceneDumpNames* names);

	SceneDumpSetting	mSettings[NX_DUMP_SETTING_COUNT];	//!< +0x000
	SceneDumpSetting	mSaved[NX_DUMP_SETTING_COUNT];		//!< +0x2a0
	};

static_assert(sizeof(SceneDump) == 0x540, "the settings object is 0x540 bytes");
static_assert(offsetof(SceneDump, mSaved) == 0x2a0, "the saved copy at +0x2a0");

//! phys_fn_003992 (0x0008fde0, 77 B). The date and time of the file header.
void sceneDumpDateTime(char* date, char* time);

//! phys_fn_003994 (0x0008fe30, 32 B). "%s__%I64x" of a prefix and a pointer,
//! in the name buffer .data 0x10126878.
const char* sceneDumpPointerName(const char* prefix, const void* pointer);

//! phys_fn_003995 (0x0008fe50, 323 B). One float token, from a ring of 16
//! buffers of 64 bytes.
const char* sceneDumpToken(NxReal value, bool binary);

#endif
