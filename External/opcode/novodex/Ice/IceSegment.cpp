/*
 * NOVODEX LOCAL MODIFICATION
 * upstream: External/opcode/upstream/Opcode/Ice/IceSegment.cpp
 *
 * Segment::SquareDistance at oracle 0x000f0560 keeps the point delta in x87
 * registers, stores the dot product once as a float at 0x000f05a4, and stores
 * only the y/z interior products as floats at 0x000f0612 and 0x000f061c.
 * The 2003 compiler's register lifetimes differ from a modern Point temporary.
 * Preserve the measured float stores while computing the other terms wide.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains code for segments.
 *	\file		IceSegment.cpp
 *	\author		Pierre Terdiman
 *	\date		April, 4, 2000
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Segment class.
 *	A segment is defined by S(t) = mP0 * (1 - t) + mP1 * t, with 0 <= t <= 1
 *	Alternatively, a segment is S(t) = Origin + t * Direction for 0 <= t <= 1.
 *	Direction is not necessarily unit length. The end points are Origin = mP0 and Origin + Direction = mP1.
 *
 *	\class		Segment
 *	\author		Pierre Terdiman
 *	\version	1.0
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Precompiled Header
#include "Stdafx.h"

using namespace IceMaths;

// phys_fn_005493: OPCODE's segment-point squared-distance kernel.
float Segment::SquareDistance(const Point& point, float* t)	const
{
	double dx = (double)point.x - mP0.x;
	double dy = (double)point.y - mP0.y;
	double dz = (double)point.z - mP0.z;
	const float dirX = mP1.x - mP0.x;
	const float dirY = mP1.y - mP0.y;
	const double dirZWide = (double)mP1.z - mP0.z;
	const float dirZ = (float)dirZWide;

	const double dotWide = ((dirZWide * dz) + ((double)dirX * dx)) + ((double)dirY * dy);
	const float dotStored = (float)dotWide;
	double parameter = 0.0;
	if(!(dotWide <= 0.0))
	{
		const double sqrLen = (((double)dirZ * dirZ) + ((double)dirY * dirY))
			+ ((double)dirX * dirX);
		if((double)dotStored >= sqrLen)
		{
			parameter = 1.0;
			dx -= dirX;
			dy -= dirY;
			dz -= dirZ;
		}
		else
		{
			parameter = (double)dotStored / sqrLen;
			const double xProduct = (double)dirX * parameter;
			const float yProduct = (float)((double)dirY * parameter);
			const float zProduct = (float)((double)dirZ * parameter);
			dx -= xProduct;
			dy -= yProduct;
			dz -= zProduct;
		}
	}
	if(t) *t = (float)parameter;
	return (float)(((dz * dz) + (dx * dx)) + (dy * dy));
}
