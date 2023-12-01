/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/

#include "Nxf.h"
#include "NxCapsule.h"
#include "NxBox.h"

NxU32 ClosestAxis(const NxVec3& p)
{
	const float* Vals = &p.x;
	NxU32 m = 0;
	if(NX_AIR(Vals[1]) > NX_AIR(Vals[m])) m = 1;
	if(NX_AIR(Vals[2]) > NX_AIR(Vals[m])) m = 2;
	return m;
}

void ComputeBasis2(const NxVec3& dir, NxVec3& right, NxVec3& up)
{
/*
	// Generic code :

	Point P(0.0f, 0.0f, 0.0f);
//			if(dir.ClosestAxis()==0)	P.z = 1.0f;
//	else	if(dir.ClosestAxis()==1)	P.x = 1.0f;
//	else	if(dir.ClosestAxis()==2)	P.y = 1.0f;
	udword Coord = dir.ClosestAxis() + 2;	// 2 <= Coord <= 4
	if(Coord>2)	Coord -= 3;					// 0 <= Coord <= 2
	P[Coord] = 1.0f;

	// Derive two remaining vectors
	right	= (P ^ dir);//.Normalize();	// ### Normalize not always needed
	up		= dir ^ right;
*/

	// Optimized code :

	// right = (P ^ dir) =
	//	P.y * dir.z - P.z * dir.y
	//	P.z * dir.x - P.x * dir.z
	//	P.x * dir.y - P.y * dir.x

	// up = dir ^ right = 
	//	dir.y * right.z - dir.z * right.y
	//	dir.z * right.x - dir.x * right.z
	//	dir.x * right.y - dir.y * right.x

//	NxU32 Coord = dir.ClosestAxis();
	NxU32 Coord = ClosestAxis(dir);
	if(Coord==0)
	{
		// P = (0,0,1)
		right.x = - dir.y;
		right.y = dir.x;
		right.z = 0.0f;

		up.x = - dir.z * dir.x;
		up.y = - dir.z * dir.y;
		up.z = dir.x * dir.x + dir.y * dir.y;
	}
	else if(Coord==1)
	{
		// P = (1,0,0)
		right.x = 0.0f;
		right.y = - dir.z;
		right.z = dir.y;

		up.x = dir.y * dir.y + dir.z * dir.z;
		up.y = - dir.x * dir.y;
		up.z = - dir.x * dir.z;
	}
	else //if(dir.ClosestAxis()==2)
	{
		// P = (0,1,0)
		right.x = dir.z;
		right.y = 0.0f;
		right.z = - dir.x;

		up.x = - dir.y * dir.x;
		up.y = dir.z * dir.z + dir.x * dir.x;
		up.z = - dir.y * dir.z;
	}
right.normalize();	// ### added after above fix, to do better
}

void ComputeBasis2(const NxVec3& p0, const NxVec3& p1, NxVec3& dir, NxVec3& right, NxVec3& up)
{
	// Compute the new direction vector
	dir = p1 - p0;
	dir.normalize();

	// Derive two remaining vectors
	ComputeBasis2(dir, right, up);
}



/**
 *	Computes an OBB surrounding the capsule.
 *	\param		box		[out] the OBB
 */
void NxComputeBoxAroundCapsule(const NxCapsule& capsule, NxBox& box)
{
	// Box center = center of the two capsule's endpoints
	box.center = (capsule.p0 + capsule.p1)*0.5f;

	// Box extents
	box.extents.x = capsule.radius + (capsule.p0.distance(capsule.p1) * 0.5f);
	box.extents.y = capsule.radius;
	box.extents.z = capsule.radius;

	// Box orientation
	NxVec3 Dir, Right, Up;
	ComputeBasis2(capsule.p0, capsule.p1, Dir, Right, Up);
	box.rot.setRow(0, Dir);
	box.rot.setRow(1, Right);
	box.rot.setRow(2, Up);
}
