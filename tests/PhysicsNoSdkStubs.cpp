// The asset harness executes TriangleMesh cooking without constructing a
// PhysicsSDK. InternalTriangleMesh's model builder still links its optional
// continuous-collision parameter query, so provide the uninitialized singleton
// and an inert fallback for that isolated path.
#include "PhysicsSDK.h"

PhysicsSDK* PhysicsSDK::instance = 0;

NxReal PhysicsSDK::getParameter(NxParameter) const
	{
	return 0.0f;
	}
