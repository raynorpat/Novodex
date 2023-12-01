/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/

#include "Nxf.h"
#include "NxRay.h"

/**
	A ray is a half-line P(t) = mOrig + mDir * t, with 0 <= t <= +infinity

	O = Origin = impact point
	i = normalized vector along the x axis
	j = normalized vector along the y axis = actually the normal vector in O
	D = Direction vector, norm |D| = 1
	N = Projection of D on y axis, norm |N| = normal reaction
	T = Projection of D on x axis, norm |T| = tangential reaction
	R = Reflexion vector

              ^y
              |
              |
              |
       _  _  _| _ _ _
       *      *      *|
        \     |     /
         \    |N   /  |
         R\   |   /D
           \  |  /    |
            \ | /
    _________\|/______*_______>x
               O    T

	Let define theta = angle between D and N. Then cos(theta) = |N| / |D| = |N| since D is normalized.

	j|D = |j|*|D|*cos(theta) => |N| = j|D

	Then we simply have:

	D = N + T

	To compute tangential reaction :

	T = D - N

	To compute reflexion vector :

	R = N - T = N - (D-N) = 2*N - D
*/

NxF32 NxComputeDistanceSquared(const NxRay& ray, const NxVec3& point, NxF32* t)
{
	NxVec3 Diff = point - ray.orig;
	NxF32 fT = Diff.dot(ray.dir);

	if(fT<=0.0f)
	{
		fT = 0.0f;
	}
	else
	{
		fT /= ray.dir.magnitudeSquared();
		Diff -= fT*ray.dir;
	}

	if(t) *t = fT;

	return Diff.magnitudeSquared();
}
