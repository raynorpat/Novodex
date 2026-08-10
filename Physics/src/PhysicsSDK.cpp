/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "PhysicsSDK.h"
#include "NpPhysicsSDK.h"
#include "NxDebugRenderable.h"
#include "NxUserOutputStream.h"

#include <stddef.h>

// The oracle's line numbers for the two error sites in this file and the one in
// getParameter, taken from the immediates at 0x0000db6b, 0x0000dbe5 and
// 0x0000dc25. They are the __LINE__ values of the build tree the shipped DLL was
// compiled from, so they cannot be produced by __LINE__ here.
static const int gSetParameterEnumErrorLine = 263;
static const int gSetParameterRangeErrorLine = 292;
static const int gGetParameterEnumErrorLine = 306;

// .data 0x001238b8, 0x001239a8 and 0x001237c8. The constructor fills all three,
// then copies the defaults into the live values at .data 0x00123b18.
static NxReal gParameterDefault[NX_PARAMS_NUM_VALUES];
static NxReal gParameterMin[NX_PARAMS_NUM_VALUES];
static NxReal gParameterMax[NX_PARAMS_NUM_VALUES];
static NxReal gParameter[NX_PARAMS_NUM_VALUES];

// .data 0x00123a98: one 32 bit mask per collision group, all bits set by the
// constructor so every group pair starts enabled. The accessors that read it
// are phys_fn_000452 and phys_fn_000431, which this component does not own.
static NxU32 gGroupCollisionMask[32];

// .data 0x001220a0. Zero in the image and filled by the constructor, which is
// why it is assigned rather than constructed here.
static NxMaterial gDefaultMaterial;

// .data 0x001220e8, 0x00123c08, 0x00123c14 and 0x00123c18.
static SdkAllocatorBridge gAllocatorBridge;
static NxFoundationSDK* gFoundation = 0;
static NxDebugRenderable* gDebugRenderable = 0;
static ShapePairFunctionTable* gShapePairFunctionTable = 0;

PhysicsSDK* PhysicsSDK::instance = 0;

// Bit 31 of NxMaterial::flags. NxMaterial.h reserves bits 16-31 for internal
// use; 0x0000e9ee sets this one on the template after the default material has
// been copied into the material array, and nothing in Phase 2 reads it back.
static const NxU32 NX_MF_INTERNAL_BIT31 = 0x80000000;

static void defineParameter(NxParameter paramEnum, NxReal defaultValue, NxReal minValue, NxReal maxValue)
	{
	gParameterDefault[paramEnum] = defaultValue;
	gParameterMin[paramEnum] = minValue;
	gParameterMax[paramEnum] = maxValue;
	}

// 0x0000fb44 allocates 0x38 bytes for this object and 0x0000fb59 reads the
// wrapper back out of +4.
static_assert(sizeof(PhysicsSDK) == 0x38, "PhysicsSDK is 56 bytes in the oracle");
static_assert(offsetof(PhysicsSDK, mNp) == 4, "the wrapper pointer is read from +4 at 0x0000fb59");
static_assert(offsetof(PhysicsSDK, mScenes) == 8, "phys_fn_000448 reads the scene array at +8 and +0xc");
static_assert(offsetof(PhysicsSDK, mTriangleMeshes) == 0x18, "phys_fn_000478 grows the mesh array at +0x18");
static_assert(offsetof(PhysicsSDK, mMaterials) == 0x28, "phys_fn_000458 reads the material array at +0x28 and +0x2c");
static_assert(sizeof(NxArraySDK<NxMaterial>) == 0x10, "NxArray is four words: first, last, memEnd, empty allocator");
static_assert(sizeof(NxMaterial) == 0x48, "the material array strides by 0x48 in the oracle");

PhysicsSDK::PhysicsSDK()
	{
	// Every triple below is measured from the immediates phys_fn_000472 stores
	// into the three tables; NX_PENALTY_FORCE really does default to 0.8 and not
	// to the 0.6 the public header's comment claims.
	defineParameter(NX_PENALTY_FORCE,					0.8f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_MIN_SEPARATION_FOR_PENALTY,		-0.05f,				-NX_MAX_REAL,	0.0f);
	defineParameter(NX_DEFAULT_SLEEP_LIN_VEL_SQUARED,	(0.15f*0.15f),		0.0f,			NX_MAX_REAL);
	defineParameter(NX_DEFAULT_SLEEP_ANG_VEL_SQUARED,	(0.14f*0.14f),		0.0f,			NX_MAX_REAL);
	defineParameter(NX_BOUNCE_TRESHOLD,					-2.0f,				-NX_MAX_REAL,	0.0f);
	defineParameter(NX_DYN_FRICT_SCALING,				1.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_STA_FRICT_SCALING,				1.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_MAX_ANGULAR_VELOCITY,			7.0f,				0.0f,			NX_MAX_REAL);

	defineParameter(NX_MESH_MESH_LEVEL,					4.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_ENABLE_MESH_DEBUG,				0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_COLL_INFINITY,					NX_MAX_REAL,		0.0f,			0.0f);
	defineParameter(NX_CONTINUOUS_CD,					0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_MESH_HINT_SPEED,					1.0f,				0.0f,			NX_MAX_REAL);

	defineParameter(NX_VISUALIZATION_SCALE,				0.0f,				0.0f,			NX_MAX_REAL);

	defineParameter(NX_VISUALIZE_WORLD_AXES,			0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_VISUALIZE_BODY_AXES,				0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_VISUALIZE_BODY_MASS_AXES,		0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_VISUALIZE_BODY_LIN_VELOCITY,		0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_VISUALIZE_BODY_ANG_VELOCITY,		0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_VISUALIZE_BODY_LIN_MOMENTUM,		0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_VISUALIZE_BODY_ANG_MOMENTUM,		0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_VISUALIZE_BODY_LIN_ACCEL,		0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_VISUALIZE_BODY_ANG_ACCEL,		0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_VISUALIZE_BODY_LIN_FORCE,		0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_VISUALIZE_BODY_ANG_FORCE,		0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_VISUALIZE_BODY_REDUCED,			0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_VISUALIZE_BODY_JOINT_GROUPS,		0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_VISUALIZE_BODY_CONTACT_LIST,		0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_VISUALIZE_BODY_JOINT_LIST,		0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_VISUALIZE_BODY_DAMPING,			0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_VISUALIZE_BODY_SLEEP,			0.0f,				0.0f,			NX_MAX_REAL);

	defineParameter(NX_VISUALIZE_JOINT_LOCAL_AXES,		0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_VISUALIZE_JOINT_WORLD_AXES,		0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_VISUALIZE_JOINT_LIMITS,			0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_VISUALIZE_JOINT_ERROR,			0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_VISUALIZE_JOINT_FORCE,			0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_VISUALIZE_JOINT_REDUCED,			0.0f,				0.0f,			NX_MAX_REAL);

	defineParameter(NX_VISUALIZE_CONTACT_POINT,			0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_VISUALIZE_CONTACT_NORMAL,		0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_VISUALIZE_CONTACT_ERROR,			0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_VISUALIZE_CONTACT_FORCE,			0.0f,				0.0f,			NX_MAX_REAL);

	defineParameter(NX_VISUALIZE_ACTOR_AXES,			0.0f,				0.0f,			NX_MAX_REAL);

	defineParameter(NX_VISUALIZE_COLLISION_AABBS,		0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_VISUALIZE_COLLISION_SHAPES,		0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_VISUALIZE_COLLISION_AXES,		0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_VISUALIZE_COLLISION_COMPOUNDS,	0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_VISUALIZE_COLLISION_VNORMALS,	0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_VISUALIZE_COLLISION_FNORMALS,	0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_VISUALIZE_COLLISION_SPHERES,		0.0f,				0.0f,			NX_MAX_REAL);

	defineParameter(NX_VISUALIZE_COLLISION_SAP,			0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_VISUALIZE_COLLISION_STATIC,		0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_VISUALIZE_COLLISION_DYNAMIC,		0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_VISUALIZE_COLLISION_FREE,		0.0f,				0.0f,			NX_MAX_REAL);

#if NX_USE_FLUID_API
	defineParameter(NX_VISUALIZE_FLUID_EMITTERS,		0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_VISUALIZE_FLUID_POSITION,		0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_VISUALIZE_FLUID_VELOCITY,		0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_VISUALIZE_FLUID_KERNEL_RADIUS,	0.0f,				0.0f,			NX_MAX_REAL);
	defineParameter(NX_VISUALIZE_FLUID_BOUNDS,			0.0f,				0.0f,			NX_MAX_REAL);
#endif

	defineParameter(NX_ADAPTIVE_FORCE,					1.0f,				0.0f,			NX_MAX_REAL);

	for(NxU32 i = 0; i < NX_PARAMS_NUM_VALUES; i++)
		gParameter[i] = gParameterDefault[i];

	gShapePairFunctionTable = NX_NEW(ShapePairFunctionTable)();

	for(NxU32 i = 0; i < 32; i++)
		gGroupCollisionMask[i] = 0xffffffff;

	gDefaultMaterial.setToDefault();
	mMaterials.pushBack(gDefaultMaterial);
	gDefaultMaterial.flags |= NX_MF_INTERNAL_BIT31;

	mNp = NX_NEW(NpPhysicsSDK)(this);
	}

PhysicsSDK::~PhysicsSDK()
	{
	NX_DELETE_SINGLE(mNp);

	// Not reconstructed here, in the oracle's order and all inert for the whole
	// of this component's differential because the SDK owns no scene, no mesh
	// and no pruning cache at this point:
	//   - the array at .data 0x00123c0c, which phys_fn_000454 and phys_fn_000480
	//     populate and this destructor deletes;
	//   - the scene release loop over mScenes (phys_fn_001275), Phase 3;
	//   - the mesh release loop over mTriangleMeshes (phys_fn_002253), Phase 4;
	//   - the three cache teardowns phys_fn_004834, phys_fn_004828 and
	//     phys_fn_004149, each guarded by a pointer that is still null.

	gFoundation->releaseDebugRenderable(gDebugRenderable);
	if(gFoundation)
		{
		gFoundation->release();
		gFoundation = 0;
		}
	NX_DELETE_SINGLE(gShapePairFunctionTable);
	}

void PhysicsSDK::release()
	{
	NX_DELETE_SINGLE(instance);
	}

bool PhysicsSDK::setParameter(NxParameter paramEnum, NxReal paramValue)
	{
	if(paramEnum >= NX_PARAMS_NUM_VALUES)
		{
		NxFoundation::FoundationSDK::error(NXE_INVALID_PARAMETER, NX_PHYSICS_SDK_CPP, gSetParameterEnumErrorLine,
			0, "setParameter: parameter value out of range.");
		return false;
		}

	// An empty range means the parameter is unbounded, so the bounds check is
	// skipped rather than failed.
	if((gParameterMin[paramEnum] == 0.0f && gParameterMax[paramEnum] == 0.0f)
		|| (paramValue >= gParameterMin[paramEnum] && paramValue <= gParameterMax[paramEnum]))
		{
		gParameter[paramEnum] = paramValue;
		return true;
		}

	NxFoundation::FoundationSDK::error(NXE_INVALID_PARAMETER, NX_PHYSICS_SDK_CPP, gSetParameterRangeErrorLine,
		0, "setParameter: parameter value out of range.");
	return false;
	}

NxReal PhysicsSDK::getParameter(NxParameter paramEnum) const
	{
	if(paramEnum >= NX_PARAMS_NUM_VALUES)
		{
		NxFoundation::FoundationSDK::error(NXE_INVALID_PARAMETER, NX_PHYSICS_SDK_CPP, gGetParameterEnumErrorLine,
			0, "getParameter: param is not an enum.");
		return 0.0f;
		}
	return gParameter[paramEnum];
	}

NxU32 PhysicsSDK::getNbScenes() const
	{
	return mScenes.size();
	}

Scene* PhysicsSDK::getScene(NxU32 index)
	{
	if(index >= mScenes.size())
		return 0;
	return mScenes[index];
	}

NxU32 PhysicsSDK::getNbMaterials() const
	{
	return mMaterials.size();
	}

NxPhysicsSDK* NX_CALL_CONV NxCreatePhysicsSDK(NxU32 sdkVersion, NxUserAllocator* allocator, NxUserOutputStream* outputStream)
	{
	if(!gFoundation)
		{
		gFoundation = NxCreateFoundationSDK(NX_FOUNDATION_SDK_VERSION, outputStream, allocator);
		if(!gFoundation)
			return 0;
		if(allocator)
			{
			gAllocatorBridge.mAllocator = allocator;
			nxSetSdkAllocatorBridge(&gAllocatorBridge);
			}
		}

	// The Foundation is created before this check, so a version mismatch still
	// leaves a Foundation SDK bound to whatever allocator was passed with it.
	if(sdkVersion != NX_PHYSICS_SDK_VERSION)
		return 0;

	if(!PhysicsSDK::instance)
		PhysicsSDK::instance = NX_NEW(PhysicsSDK)();
	// Unguarded, exactly as at 0x0000fb65: if the allocation above failed the
	// oracle dereferences null here too.
	return PhysicsSDK::instance->getNp();
	}
