// The candidate's Triangle::Inflate (vendored OPCODE, phys_fn_005185) for
// NxPhysicsCollisionTests' ray_inflated_tris pre-flight, in a translation unit
// of its own. Opcode.h cannot be included in PhysicsCollisionTests.cpp: its
// headers set translation-unit-wide pragmas, and with them in force two
// registered oracle digests there (box_corner, box_quad_depth) moved -- the
// harness's generators were compiled differently, and whether a signalling NaN
// a generator draws reaches the oracle quieted or not depends on how its float
// travels. Here nothing else is compiled.

#include "Opcode.h"

// `corners` is nine floats, the three corners; inflated in place.
void nxCandidateTriangleInflate(float* corners, float fatCoeff, bool constantBorder)
	{
	reinterpret_cast<IceMaths::Triangle*>(corners)->Inflate(fatCoeff, constantBorder);
	}

// The candidate's Matrix4x4::Invert (vendored OPCODE, phys_fn_005197) for
// NxPhysicsCollisionTests' convex_mesh_ray pre-flight (convex-mesh gap Task 2h
// review), here for the same reason. `m` is sixteen floats, inverted in place.
void nxCandidateMatrixInvert(float* m)
	{
	reinterpret_cast<IceMaths::Matrix4x4*>(m)->Invert();
	}
