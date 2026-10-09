#ifndef NX_SMOOTH_NORMALS_ANGLE_H
#define NX_SMOOTH_NORMALS_ANGLE_H
#include "NxPhysicsBackend.h"
#include "NxVec3.h"
#if NX_PHYSICS_USE_X87
// Legacy eax vertex, edx indices, esi vertices, float value in st(0).
void nxSmoothNormalsAngleAtVertex();
#else
// Ordinary typed equivalent. Inputs are borrowed, valid vertex/index arrays;
// no writes, allocation, alias restriction or index validation. Returns the
// deliberately narrowed binary32 interior angle (radians).
NxReal nxSmoothNormalsAngleAtVertex(NxU32 vertex, const NxU32* index, const NxVec3* verts);
#endif
#endif
