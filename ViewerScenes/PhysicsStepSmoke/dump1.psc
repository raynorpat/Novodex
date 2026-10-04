################################################################
### Core Dump from Novodex Physics SDK
### Core Dump Generated on Sunday, October 04, 2026 at 06:37 PM
### Contains one Asset.
PsReset
PsVersion 1.4
PsNameSpace PhysicsSDK__19ae40a0


### Begin Material Definition ####
PsMatBegin mat1
PsMatDynamicFriction 0
PsMatStaticFriction 0
PsMatSpinFriction 0
PsMatRollFriction 0
PsMatRestitution 0
PsMatDynamicFrictionV 0
PsMatStaticFrictionV 0
PsMatDirOfAnisotropy 1 0 0
PsMatDirOfMotion 1 0 0
PsMatSpeedOfMotion 0
PsMatAnisotropic false
PsMatMovingSurface false
PsMatEnd
### End Material Definition ####


### Begin Material Definition ####
PsMatBegin mat2
PsMatDynamicFriction 0.5
PsMatStaticFriction 0.5
PsMatSpinFriction 0
PsMatRollFriction 0
PsMatRestitution 0
PsMatDynamicFrictionV 0
PsMatStaticFrictionV 0
PsMatDirOfAnisotropy 1 0 0
PsMatDirOfMotion 1 0 0
PsMatSpeedOfMotion 0
PsMatAnisotropic false
PsMatMovingSurface false
PsMatEnd
### End Material Definition ####

PsAssetBegin Asset__19d89320
################################################################
### Novodex Specific Physics Simulation Constants
################################################################
PsSetConstant NX_PENALTY_FORCE 0.800000012
PsSetConstant NX_MIN_SEPARATION_FOR_PENALTY -0.050000001
PsSetConstant NX_DEFAULT_SLEEP_LIN_VEL_SQUARED 0.022500001
PsSetConstant NX_DEFAULT_SLEEP_ANG_VEL_SQUARED 0.0196
PsSetConstant NX_BOUNCE_TRESHOLD -2
PsSetConstant NX_DYN_FRICT_SCALING 1
PsSetConstant NX_STA_FRICT_SCALING 1
PsSetConstant NX_MAX_ANGULAR_VELOCITY 7
PsSetConstant NX_MESH_MESH_LEVEL 4
PsSetConstant NX_ENABLE_MESH_DEBUG 0
PsSetConstant NX_COLL_INFINITY fltmax
PsSetConstant NX_CONTINUOUS_CD 0
PsSetConstant NX_MESH_HINT_SPEED 1
################################################################

###########################################################
#### Asset Information
###########################################################
## Scene in initial configuration.
###########################################################
PsGravity 0 -9.81000042 0
PsBox sides(0.5,0.5,0.5) localposition(0,0,0) localorientation(0,0,0,1) material(mat2) group(0) name(FallingBox___19bff170) position(0,10,0) orientation(0,0,0,1) com(0,0,0) comrot(0,0,0,1) inertia(0.041666668,0.041666668,0.041666668) mass(1) solvercount(4) velocity(0,0,0) angularvelocity(0,0,0) wakeupcounter(0.399999976) lineardamping(0) angulardamping(0.050000001) maxangularvelocity(7) 
PsAssetEnd
##################################################################################


PsSetScene 0
PsAsset Asset__19d89320 position(0,0,0) orientation(0,0,0,1)

PsSetScene 0
