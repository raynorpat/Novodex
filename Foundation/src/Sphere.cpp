/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/

#include "Nxf.h"
#include "NxMath.h"
#include "NxSphere.h"
#include "NxMat33.h"

// An Efficient Bounding Sphere
// by Jack Ritter
// from "Graphics Gems", Academic Press, 1990

/* Routine to calculate tight bounding sphere over    */
/* a set of points in 3D */
/* This contains the routine find_bounding_sphere(), */
/* the struct definition, and the globals used for parameters. */
/* The abs() of all coordinates must be < BIGNUMBER */
/* Code written by Jack Ritter and Lyle Rains. */

bool NxFastComputeSphere(NxSphere& sphere, unsigned nb_verts, const NxVec3* verts)
	{
	unsigned i;
	// Checkings
	if(!verts)	return false;

	NxVec3 xmin, xmax, ymin, ymax, zmin, zmax;

	// FIRST PASS: find 6 minima/maxima points
	xmin.x = ymin.y = zmin.z= NX_MAX_REAL;
	xmax.x = ymax.y = zmax.z= NX_MIN_REAL;

	for(i=0;i<nb_verts;i++)
	{
		if(verts[i].x < xmin.x)	xmin = verts[i];
		if(verts[i].x > xmax.x)	xmax = verts[i];
		if(verts[i].y < ymin.y)	ymin = verts[i];
		if(verts[i].y > ymax.y)	ymax = verts[i];
		if(verts[i].z < zmin.z)	zmin = verts[i];
		if(verts[i].z > zmax.z)	zmax = verts[i];
	}

	// Set xspan = distance between the 2 points xmin & xmax (squared)
	NxF32 dx = xmax.x - xmin.x;
	NxF32 dy = xmax.y - xmin.y;
	NxF32 dz = xmax.z - xmin.z;
	NxF32 xspan = dx*dx + dy*dy + dz*dz;

	// Same for y & z spans
	dx = ymax.x - ymin.x;
	dy = ymax.y - ymin.y;
	dz = ymax.z - ymin.z;
	NxF32 yspan = dx*dx + dy*dy + dz*dz;

	dx = zmax.x - zmin.x;
	dy = zmax.y - zmin.y;
	dz = zmax.z - zmin.z;
	NxF32 zspan = dx*dx + dy*dy + dz*dz;

	// Set points dia1 & dia2 to the maximally separated pair
	NxVec3 dia1 = xmin;
	NxVec3 dia2 = xmax; // assume xspan biggest
	NxF32 maxspan = xspan;
	if(yspan>maxspan)
	{
		maxspan = yspan;
		dia1 = ymin; dia2 = ymax;
	}
	if(zspan>maxspan)
	{
		dia1 = zmin; dia2 = zmax;
	}

	// dia1,dia2 is a diameter of initial sphere
	// calc initial center
	sphere.center.x = (dia1.x + dia2.x) * 0.5f;
	sphere.center.y = (dia1.y + dia2.y) * 0.5f;
	sphere.center.z = (dia1.z + dia2.z) * 0.5f;

	// calculate initial radius**2 and radius
	dx = dia2.x - sphere.center.x; // x component of radius vector
	dy = dia2.y - sphere.center.y; // y component of radius vector
	dz = dia2.z - sphere.center.z; // z component of radius vector
	NxF32 rad_sq = dx*dx + dy*dy + dz*dz;
	sphere.radius = sqrtf(rad_sq);

	// SECOND PASS: increment current sphere
	for(i=0;i<nb_verts;i++)
	{
		dx = verts[i].x - sphere.center.x;
		dy = verts[i].y - sphere.center.y;
		dz = verts[i].z - sphere.center.z;

		NxF32 old_to_p_sq = dx*dx + dy*dy + dz*dz;

		if(old_to_p_sq > rad_sq)	// do r**2 test first
		{
			// this point is outside of current sphere
			NxF32 old_to_p = sqrtf(old_to_p_sq);

			// calc radius of new sphere
			sphere.radius = (sphere.radius + old_to_p) * 0.5f;
			rad_sq = sphere.radius*sphere.radius;	// for next r**2 compare
			NxF32 old_to_new = old_to_p - sphere.radius;

			// calc center of new sphere
			sphere.center.x = (sphere.radius*sphere.center.x + old_to_new*verts[i].x) / old_to_p;
			sphere.center.y = (sphere.radius*sphere.center.y + old_to_new*verts[i].y) / old_to_p;
			sphere.center.z = (sphere.radius*sphere.center.z + old_to_new*verts[i].z) / old_to_p;
		}
	}
	return true;
}

/**
 *	Contains code to compute minimal bounding spheres.
 *	\file		IceMiniball.h
 *	\author		Pierre Terdiman, original code by Nicolas Capens
 *	\date		June, 29, 2001
 */

/**
 *	A minimal bounding sphere based on Welzl's algorithm.
 *	For more information see http://www.flipcode.com/cgi-bin/msg.cgi?showThread=COTD-SmallestEnclosingSpheres&forum=cotd&id=-1
 *
 *	\class		Miniball
 *	\author		Nicolas Capens
 *	\version	1.0
 */

	class Miniball
	{
		public:
		// Constructors
							Miniball();
							Miniball(const Miniball& X);
							Miniball(const NxVec3& O);   // Point-Miniball
							Miniball(const NxVec3& O, NxF32 R);   // Center and radius (not squared)
							Miniball(const NxVec3& O, const NxVec3& A);   // Miniball through two points
							Miniball(const NxVec3& O, const NxVec3& A, const NxVec3& B);   // Miniball through three points
							Miniball(const NxVec3& O, const NxVec3& A, const NxVec3& B, const NxVec3& C);   // Miniball through four points
		// Destructor
		NX_INLINE			~Miniball()	{}

				NxVec3		mCenter;
				NxF32		mRadius;

				Miniball&	operator=(const Miniball& S);

				NxF32		d(const NxVec3& P) const;  // Distance from p to boundary of the Miniball
				NxF32		d2(const NxVec3& P) const;  // Square distance from p to boundary of the Miniball

		static	NxF32		d(const Miniball& S, const NxVec3& P);  // Distance from p to boundary of the Miniball
		static	NxF32		d(const NxVec3& P, const Miniball& S);  // Distance from p to boundary of the Miniball

		static	NxF32		d2(const Miniball& S, const NxVec3& P);  // Square distance from p to boundary of the Miniball
		static	NxF32		d2(const NxVec3& P, const Miniball& S);  // Square distance from p to boundary of the Miniball

		static	Miniball	miniBall(NxVec3 P[], unsigned int p);   // Smallest enclosing Miniball
		static	Miniball	smallBall(NxVec3 P[], unsigned int p);   // Enclosing Miniball approximation

		private:
		static	Miniball	recurseMini(NxVec3* P[], unsigned int p, unsigned int b = 0);
	};

const NxF32 radiusEpsilon = 1e-4f;	// NOTE: To avoid numerical inaccuracies

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Constructor.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Miniball::Miniball() : mRadius(-1.0f)
Miniball::Miniball() : mRadius(-0.001f)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Constructor.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Miniball::Miniball(const Miniball& S)
{
	mRadius = S.mRadius;
	mCenter = S.mCenter;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Constructor.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Miniball::Miniball(const NxVec3& O)
{
	mRadius = 0.0f + radiusEpsilon;
	mCenter = O;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Constructor.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Miniball::Miniball(const NxVec3& O, NxF32 R)
{
	mRadius = R;
	mCenter = O;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Constructor.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Miniball::Miniball(const NxVec3& O, const NxVec3& A)
{
	NxVec3 a = A - O;

	NxVec3 o = 0.5f * a;

	mRadius = o.magnitude() + radiusEpsilon;
	mCenter = O + o;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Constructor.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Miniball::Miniball(const NxVec3& O, const NxVec3& A,  const NxVec3& B)
{
	NxVec3 a = A - O;
	NxVec3 b = B - O;

	NxF32 Denominator = 2.0f * (a.cross(b)).magnitudeSquared();

	NxVec3 o = ((b.magnitudeSquared()) * ((a.cross(b)).cross(a)) + (a.magnitudeSquared()) * (b.cross(a.cross(b)))) / Denominator;

	mRadius = o.magnitude() + radiusEpsilon;
	mCenter = O + o;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Constructor.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Miniball::Miniball(const NxVec3& O, const NxVec3& A, const NxVec3& B, const NxVec3& C)
{
	NxVec3 a = A - O;
	NxVec3 b = B - O;
	NxVec3 c = C - O;

/*	NxMat33 Mat(	a.x, a.y, a.z,
					b.x, b.y, b.z,
					c.x, c.y, c.z);*/
	NxMat33 Mat;
	NX_ASSERT(!"To fix");

	NxF32 Denominator = 2.0f * Mat.determinant();

	NxVec3 o = ((c.magnitudeSquared()) * (a.cross(b)) +
		        (b.magnitudeSquared()) * (c.cross(a)) +
		        (a.magnitudeSquared()) * (b.cross(c))) / Denominator;

	mRadius = o.magnitude() + radiusEpsilon;
	mCenter = O + o;
}

Miniball& Miniball::operator=(const Miniball& S)
{
	mRadius = S.mRadius;
	mCenter = S.mCenter;
	return *this;
}

NxF32 Miniball::d(const NxVec3& P) const
{
	return P.distance(mCenter) - mRadius;
}

NxF32 Miniball::d2(const NxVec3& P) const
{
	return P.distanceSquared(mCenter) - mRadius*mRadius;
}

NxF32 Miniball::d(const Miniball& S, const NxVec3& P)
{
	return P.distance(S.mCenter) - S.mRadius;
}

NxF32 Miniball::d(const NxVec3& P, const Miniball& S)
{
	return P.distance(S.mCenter) - S.mRadius;
}

NxF32 Miniball::d2(const Miniball& S, const NxVec3& P)
{
	return P.distanceSquared(S.mCenter) - S.mRadius*S.mRadius;
}

NxF32 Miniball::d2(const NxVec3 &P, const Miniball &S)
{
	return P.distanceSquared(S.mCenter) - S.mRadius*S.mRadius;
}

Miniball Miniball::recurseMini(NxVec3* P[], unsigned int p, unsigned int b)
{
	Miniball MB;

	switch(b)
	{
		case 0:
			MB = Miniball();
			break;
		case 1:
			MB = Miniball(*P[-1]);
			break;
		case 2:
			MB = Miniball(*P[-1], *P[-2]);
			break;
		case 3:
			MB = Miniball(*P[-1], *P[-2], *P[-3]);
			break;
		case 4:
			MB = Miniball(*P[-1], *P[-2], *P[-3], *P[-4]);
			return MB;
	}

	for(unsigned int i = 0; i < p; i++)
		if(MB.d2(*P[i]) > 0.0f)   // Signed square distance to sphere
		{
			for(unsigned int j = i; j > 0; j--)
			{
				NxVec3* T = P[j];
				P[j] = P[j - 1];
				P[j - 1] = T;
			}

			MB = recurseMini(P + 1, i, b + 1);
		}

	return MB;
}

Miniball Miniball::miniBall(NxVec3 P[], unsigned int p)
{
	NxVec3** L = new NxVec3*[p];

	for(unsigned int i = 0; i < p; i++)
		L[i] = &P[i];

	Miniball MB = recurseMini(L, p);

	delete[] L;

	return MB;
}

Miniball Miniball::smallBall(NxVec3 P[], unsigned int p)
{
	NxVec3 mCenter;
	NxF32 mRadius = -1.0f;
	unsigned i;

	if(p > 0)
	{
		mCenter.zero();

		for(i = 0; i < p; i++)
			mCenter += P[i];

		mCenter /= (NxF32)p;

		for(i = 0; i < p; i++)
		{
			NxF32 d2 = P[i].distanceSquared(mCenter);
		
			if(d2 > mRadius)
				mRadius = d2;
		}

		mRadius = sqrtf(mRadius) + radiusEpsilon;
	}

	return Miniball(mCenter, mRadius);
}

NxBSphereMethod NxComputeSphere(NxSphere& sphere, unsigned nb_verts, const NxVec3* verts)
{
	// Checkings
	if(!nb_verts || !verts)	return NX_BS_NONE;

	// Play it safe : first create a robust-but-not-optimal sphere
	NxSphere SafeSphere;
	NxFastComputeSphere(SafeSphere, nb_verts, verts);

	// Then try the more-advanced-yet-sometimes-buggy way
	Miniball MB;
	Miniball S = MB.miniBall((NxVec3*)verts, nb_verts);

//	if(IsNAN(S.mCenter.x) || IsNAN(S.mCenter.y) || IsNAN(S.mCenter.z) || IsNAN(S.mRadius) || S.mRadius>SafeSphere.mRadius || S.mRadius<0.0f)
	if(NxMath::isFinite(S.mCenter.x) || NxMath::isFinite(S.mCenter.y) || NxMath::isFinite(S.mCenter.z) || NxMath::isFinite(S.mRadius) || S.mRadius>SafeSphere.radius || S.mRadius<0.0f)
	{
		sphere.center = SafeSphere.center;
		sphere.radius = SafeSphere.radius;
		return NX_BS_GEMS;
	}
	else
	{
		sphere.center = S.mCenter;
		sphere.radius = S.mRadius;
		return NX_BS_MINIBALL;
	}
	return NX_BS_NONE;
}
