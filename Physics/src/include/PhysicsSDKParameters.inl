// Shared actual parameter storage, accessors, and constructor initialization.
// Include once per linked SDK owner; the full SDK constructor is the producer.
static const int gSetParameterEnumErrorLine = 263;
static const int gSetParameterRangeErrorLine = 292;
static const int gGetParameterEnumErrorLine = 306;

// .data 0x001238b8, 0x001239a8 and 0x001237c8. The constructor fills all three,
// then copies the defaults into the live values at .data 0x00123b18.
static NxReal gParameterDefault[NX_PARAMS_NUM_VALUES];
static NxReal gParameterMin[NX_PARAMS_NUM_VALUES];
static NxReal gParameterMax[NX_PARAMS_NUM_VALUES];
static NxReal gParameter[NX_PARAMS_NUM_VALUES];

PhysicsSDK *PhysicsSDK::instance = 0;

// The view of the two file-static arrays above (PhysicsSDK.h) for the rows that
// read them directly: the core dump, and the contact-pair manager rows, which
// load the live parameters and the group collision masks straight from them
// (e.g. `fld [0x10123b2c]` at 0x1001cbff, `mov eax,[eax*4 + 0x10123a98]` at
// 0x1001fe6e), not through getParameter/getGroupCollisionFlag.
const NxReal *nxPhysicsSDKParameters()
{
    return gParameter;
}

static void defineParameter(NxParameter paramEnum, NxReal defaultValue, NxReal minValue, NxReal maxValue)
{
    gParameterDefault[paramEnum] = defaultValue;
    gParameterMin[paramEnum] = minValue;
    gParameterMax[paramEnum] = maxValue;
}

static void nxPhysicsSDKInitializeParameters()
{
    // Every triple below is measured from the immediates phys_fn_000472 stores
    // into the three tables; NX_PENALTY_FORCE really does default to 0.8 and not
    // to the 0.6 the public header's comment claims.
    defineParameter(NX_PENALTY_FORCE, 0.8f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_MIN_SEPARATION_FOR_PENALTY, -0.05f, -NX_MAX_REAL, 0.0f);
    defineParameter(NX_DEFAULT_SLEEP_LIN_VEL_SQUARED, (0.15f * 0.15f), 0.0f, NX_MAX_REAL);
    defineParameter(NX_DEFAULT_SLEEP_ANG_VEL_SQUARED, (0.14f * 0.14f), 0.0f, NX_MAX_REAL);
    defineParameter(NX_BOUNCE_TRESHOLD, -2.0f, -NX_MAX_REAL, 0.0f);
    defineParameter(NX_DYN_FRICT_SCALING, 1.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_STA_FRICT_SCALING, 1.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_MAX_ANGULAR_VELOCITY, 7.0f, 0.0f, NX_MAX_REAL);

    defineParameter(NX_MESH_MESH_LEVEL, 4.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_ENABLE_MESH_DEBUG, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_COLL_INFINITY, NX_MAX_REAL, 0.0f, 0.0f);
    defineParameter(NX_CONTINUOUS_CD, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_MESH_HINT_SPEED, 1.0f, 0.0f, NX_MAX_REAL);

    defineParameter(NX_VISUALIZATION_SCALE, 0.0f, 0.0f, NX_MAX_REAL);

    defineParameter(NX_VISUALIZE_WORLD_AXES, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_VISUALIZE_BODY_AXES, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_VISUALIZE_BODY_MASS_AXES, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_VISUALIZE_BODY_LIN_VELOCITY, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_VISUALIZE_BODY_ANG_VELOCITY, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_VISUALIZE_BODY_LIN_MOMENTUM, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_VISUALIZE_BODY_ANG_MOMENTUM, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_VISUALIZE_BODY_LIN_ACCEL, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_VISUALIZE_BODY_ANG_ACCEL, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_VISUALIZE_BODY_LIN_FORCE, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_VISUALIZE_BODY_ANG_FORCE, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_VISUALIZE_BODY_REDUCED, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_VISUALIZE_BODY_JOINT_GROUPS, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_VISUALIZE_BODY_CONTACT_LIST, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_VISUALIZE_BODY_JOINT_LIST, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_VISUALIZE_BODY_DAMPING, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_VISUALIZE_BODY_SLEEP, 0.0f, 0.0f, NX_MAX_REAL);

    defineParameter(NX_VISUALIZE_JOINT_LOCAL_AXES, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_VISUALIZE_JOINT_WORLD_AXES, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_VISUALIZE_JOINT_LIMITS, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_VISUALIZE_JOINT_ERROR, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_VISUALIZE_JOINT_FORCE, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_VISUALIZE_JOINT_REDUCED, 0.0f, 0.0f, NX_MAX_REAL);

    defineParameter(NX_VISUALIZE_CONTACT_POINT, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_VISUALIZE_CONTACT_NORMAL, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_VISUALIZE_CONTACT_ERROR, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_VISUALIZE_CONTACT_FORCE, 0.0f, 0.0f, NX_MAX_REAL);

    defineParameter(NX_VISUALIZE_ACTOR_AXES, 0.0f, 0.0f, NX_MAX_REAL);

    defineParameter(NX_VISUALIZE_COLLISION_AABBS, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_VISUALIZE_COLLISION_SHAPES, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_VISUALIZE_COLLISION_AXES, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_VISUALIZE_COLLISION_COMPOUNDS, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_VISUALIZE_COLLISION_VNORMALS, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_VISUALIZE_COLLISION_FNORMALS, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_VISUALIZE_COLLISION_SPHERES, 0.0f, 0.0f, NX_MAX_REAL);

    defineParameter(NX_VISUALIZE_COLLISION_SAP, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_VISUALIZE_COLLISION_STATIC, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_VISUALIZE_COLLISION_DYNAMIC, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_VISUALIZE_COLLISION_FREE, 0.0f, 0.0f, NX_MAX_REAL);

#if NX_USE_FLUID_API
    defineParameter(NX_VISUALIZE_FLUID_EMITTERS, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_VISUALIZE_FLUID_POSITION, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_VISUALIZE_FLUID_VELOCITY, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_VISUALIZE_FLUID_KERNEL_RADIUS, 0.0f, 0.0f, NX_MAX_REAL);
    defineParameter(NX_VISUALIZE_FLUID_BOUNDS, 0.0f, 0.0f, NX_MAX_REAL);
#endif

    defineParameter(NX_ADAPTIVE_FORCE, 1.0f, 0.0f, NX_MAX_REAL);

    for (NxU32 i = 0; i < NX_PARAMS_NUM_VALUES; i++)
        gParameter[i] = gParameterDefault[i];
}

bool PhysicsSDK::setParameter(NxParameter paramEnum, NxReal paramValue)
{
    if (paramEnum >= NX_PARAMS_NUM_VALUES)
    {
        NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_PHYSICS_SDK_CPP,
                                                         gSetParameterEnumErrorLine, 0,
                                                         "setParameter: parameter value out of range.");
        return false;
    }

    // An empty range means the parameter is unbounded, so the bounds check is
    // skipped rather than failed.
    if ((gParameterMin[paramEnum] == 0.0f && gParameterMax[paramEnum] == 0.0f) ||
        (paramValue >= gParameterMin[paramEnum] && paramValue <= gParameterMax[paramEnum]))
    {
        gParameter[paramEnum] = paramValue;
        return true;
    }

    NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_PHYSICS_SDK_CPP,
                                                     gSetParameterRangeErrorLine, 0,
                                                     "setParameter: parameter value out of range.");
    return false;
}

NxReal PhysicsSDK::getParameter(NxParameter paramEnum) const
{
    if (paramEnum >= NX_PARAMS_NUM_VALUES)
    {
        NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_PARAMETER, NX_PHYSICS_SDK_CPP,
                                                         gGetParameterEnumErrorLine, 0,
                                                         "getParameter: param is not an enum.");
        return 0.0f;
    }
    return gParameter[paramEnum];
}
