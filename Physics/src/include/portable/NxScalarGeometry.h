#ifndef NX_SCALAR_GEOMETRY_H
#define NX_SCALAR_GEOMETRY_H
#include "NxPhysicsBackend.h"
#include <math.h>
// PMap ray direction: z, y, x grouping, reciprocal kept as binary64, then
// deliberate x/y/z binary32 stores. Nearest arithmetic; caller state unchanged.
// Zero retains all components including signed zeros; nonfinite inputs follow
// ordinary IEEE arithmetic. Borrowed three-element array, normalized in place.
static inline void nxScalarNormalizeDirection3(float direction[3])
    {
    if(direction[0] != 0.0f || direction[1] != 0.0f || direction[2] != 0.0f)
        {
        const double inverse = 1.0 / sqrt((direction[2] * (double) direction[2]
            + direction[1] * (double) direction[1]) + direction[0] * (double) direction[0]);
        direction[0] = (float) (direction[0] * inverse);
        direction[1] = (float) (direction[1] * inverse);
        direction[2] = (float) (direction[2] * inverse);
        }
    }
#endif
