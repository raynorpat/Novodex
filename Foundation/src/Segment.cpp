/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/

#include "Nxf.h"
#include "NxSegment.h"

/**
A segment is defined by S(t) = mP0 * (1 - t) + mP1 * t, with 0 <= t <= 1
Alternatively, a segment is S(t) = Origin + t * Direction for 0 <= t <= 1.
Direction is not necessarily unit length. The end points are Origin = mP0 and Origin + Direction = mP1.
*/

NxF32 NxComputeSquareDistance(const NxSegment& seg, const NxVec3& point, NxF32* t)
{
	NxVec3 Diff = point - seg.p0;
	NxVec3 Dir = seg.p1 - seg.p0;
	NxF32 fT = Diff.dot(Dir);

	if(fT<=0.0f)
	{
		fT = 0.0f;
	}
	else
	{
		NxF32 SqrLen= Dir.magnitudeSquared();
		if(fT>=SqrLen)
		{
			fT = 1.0f;
			Diff -= Dir;
		}
		else
		{
			fT /= SqrLen;
			Diff -= fT*Dir;
		}
	}

	if(t)	*t = fT;

	return Diff.magnitudeSquared();
}
