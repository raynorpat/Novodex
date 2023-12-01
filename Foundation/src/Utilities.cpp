/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "Nxf.h"
#include "NxUtilities.h"
#include "NxMath.h"
#include "NxVec3.h"
#include "NxMat33.h"

#ifndef M_SQRT1_2	//1/sqrt(2)
#define M_SQRT1_2 double(0.7071067811865475244008443621048490)
#endif

void NxComputeBounds(NxBounds3& bounds, NxU32 nbVerts, const NxVec3* verts)
	{
	if(!nbVerts || !verts)
		return;

	NxVec3 max(NX_MIN_F32, NX_MIN_F32, NX_MIN_F32);
	NxVec3 min(NX_MAX_F32, NX_MAX_F32, NX_MAX_F32);
		{
		while(nbVerts--)
			{
			if(verts->x > max.x)	max.x = verts->x;
			if(verts->x < min.x)	min.x = verts->x;

			if(verts->y > max.y)	max.y = verts->y;
			if(verts->y < min.y)	min.y = verts->y;

			if(verts->z > max.z)	max.z = verts->z;
			if(verts->z < min.z)	min.z = verts->z;

			verts ++;
			}
		}
	bounds.set(min, max);
	}

void NxNormalToTangents(const NxVec3 & n, NxVec3 & t1, NxVec3 & t2)
	{
	if (fabs(n.z) > M_SQRT1_2) 
		{
		NxReal a = n.y*n.y + n.z*n.z;
		NxReal k = NxReal(1.0)/NxMath::sqrt(a);
		t1.set(0,-n.z*k,n.y*k);
		t2.set(a*k,-n.x*t1.z,n.x*t1.y);
		}
	else 
		{
		NxReal a = n.x*n.x + n.y*n.y;
		NxReal k = NxReal(1.0)/NxMath::sqrt(a);
		t1.set(-n.y*k,n.x*k,0);
		t2.set(-n.z*t1.y,n.z*t1.x,a*k);
		}
	t1.normalize();
	t2.normalize();
	}

// Diagonalize a matrix
bool NxJacobiTransform(NxI32 n, NxF64 a[], NxF64 w[])
	{
	const NxF64	TINY_		= 1E-20f;
	const NxF64	EPS			= 1E-6f;
	const NxI32 MAX_ITER	= 100;
	
#define	rotate(a, i, j, k, l) {		\
	NxF64 x=a[i*n+j], y=a[k*n+l];	\
	a[i*n+j] = x*c - y*s;			\
	a[k*n+l] = x*s + y*c;			\
		}
	
	NxF64	t, c, s, tolerance, offdiag;
	
	s = offdiag = 0;
	for(NxI32 j=0;j<n;j++)
		{
		NxI32 k;
		for(k=0;k<n;k++) 
			w[j*n+k] = 0;
		
		w[j*n+j] = 1;
		s += a[j*n+j] * a[j*n+j];
		
		for(k=j+1;k<n;k++)	
			offdiag += a[j*n+k] * a[j*n+k];
		}
	tolerance = EPS * EPS * (s / 2 + offdiag);
	
	for(NxI32 iter=0;iter<MAX_ITER;iter++)
		{
		offdiag = 0;
		NxI32 j;
		for(j=0;j<n-1;j++)
			{
			for(NxI32 k=j+1;k<n;k++)
				{
				offdiag += a[j*n+k] * a[j*n+k];
				}
			}
		
		if(offdiag < tolerance)	return true;
		
		for(j=0; j<n-1; j++)
			{
			for(NxI32 k=j+1; k<n; k++)
				{
				if(fabs(a[j*n+k]) < TINY_) continue;
				
				t = (a[k*n+k] - a[j*n+j]) / (2 * a[j*n+k]);
				
				if (t >= 0)	t = 1 / (t + sqrt(t * t + 1));
				else		t = 1 / (t - sqrt(t * t + 1));
				
				c = 1.0 / sqrt(t * t + 1);
				s = t * c;
				t *= a[j*n+k];
				a[j*n+j] -= t;
				a[k*n+k] += t;
				a[j*n+k] = 0;
				NxI32 i;
				for(i=0;i<j;i++)	rotate(a, i, j, i, k);
				for(i=j+1;i<k;i++)		rotate(a, j, i, i, k);
				for(i=k+1;i<n;i++)		rotate(a, j, i, k, i);
				for(i=0;i<n;i++)		rotate(w, j, i, k, i);
				}
			}
		}
#undef	rotate
	return false;
	}

bool NxDiagonalizeInertiaTensor(const NxMat33 & denseInertia, NxVec3 & diagonalInertia, NxMat33 & rotation)
	{
	// We just convert to doubles temporarily, for higher precision
	double A[3*3], R[3*3];

	A[0*3+0]=denseInertia(0,0);	A[0*3+1]=denseInertia(1,0);	A[0*3+2]=denseInertia(2,0);
	A[1*3+0]=denseInertia(0,1);	A[1*3+1]=denseInertia(1,1);	A[1*3+2]=denseInertia(2,1);
	A[2*3+0]=denseInertia(0,2);	A[2*3+1]=denseInertia(1,2);	A[2*3+2]=denseInertia(2,2);

		// Here we diagonalize the inertia tensor
	if(!NxJacobiTransform( 3, A, R ))
		{
		// Can't diagonalize inertia tensor!

		// Setups a default tensor in sake of robustness
		diagonalInertia.set(1,1,1);
		return false;
		}	
	else
		{
		// Save eigenvalues
		diagonalInertia.set( (float)A[0*3+0], (float)A[1*3+1], (float)A[2*3+2] );

		rotation(0,0) = (float)R[0*3+0];	rotation(0,1) = (float)R[1*3+0];	rotation(0,2) = (float)R[2*3+0];
		rotation(1,0) = (float)R[0*3+1];	rotation(1,1) = (float)R[1*3+1];	rotation(1,2) = (float)R[2*3+1];
		rotation(2,0) = (float)R[0*3+2];	rotation(2,1) = (float)R[1*3+2];	rotation(2,2) = (float)R[2*3+2];
/*
		rotation(0,0) = (float)R[0*3+0];	rotation(1,0) = (float)R[1*3+0];	rotation(2,0) = (float)R[2*3+0];
		rotation(0,1) = (float)R[0*3+1];	rotation(1,1) = (float)R[1*3+1];	rotation(2,1) = (float)R[2*3+1];
		rotation(0,2) = (float)R[0*3+2];	rotation(1,2) = (float)R[1*3+2];	rotation(2,2) = (float)R[2*3+2];
*/
		return true;
		}	
	}


/*
 * A function for creating a rotation matrix that rotates a vector called
 * "from" into another vector called "to".
 * Input : from[3], to[3] which both must be *normalized* non-zero vectors
 * Output: mtx[3][3] -- a 3x3 matrix in colum-major form
 * Authors: Tomas Möller, John Hughes 1999

adapted by Adam M.
 */
//void fromToRotation(float from[3], float to[3], float mtx[3][3]) 
void NxFindRotationMatrix(const NxVec3 & from, const NxVec3 & to, NxMat33 & mtx)
	{
	NxVec3 v;
	NxReal e, h, f;
	static const NxReal EPSILON = 0.000001f;
	
	v.cross(from, to);
	e = from.dot(to);
	f = (e < 0)? -e:e;
	if (f > 1.0 - EPSILON)     /* "from" and "to"-vector almost parallel */
		{
		NxVec3 u, v; /* temporary storage vectors */
		NxVec3 x;       /* vector most nearly orthogonal to "from" */
		NxReal c1, c2, c3; /* coefficients for later use */
		int i, j;
		
		x[0] = (from[0] > 0.0)? from[0] : -from[0];
		x[1] = (from[1] > 0.0)? from[1] : -from[1];
		x[2] = (from[2] > 0.0)? from[2] : -from[2];
		
		if (x[0] < x[1])
			{
			if (x[0] < x[2])
				{
				x[0] = 1.0; x[1] = x[2] = 0.0;
				}
			else
				{
				x[2] = 1.0; x[0] = x[1] = 0.0;
				}
			}
		else
			{
			if (x[1] < x[2])
				{
				x[1] = 1.0; x[0] = x[2] = 0.0;
				}
			else
				{
				x[2] = 1.0; x[0] = x[1] = 0.0;
				}
			}
		
		u[0] = x[0] - from[0]; u[1] = x[1] - from[1]; u[2] = x[2] - from[2];
		v[0] = x[0] - to[0];   v[1] = x[1] - to[1];   v[2] = x[2] - to[2];
		
		c1 = 2.0f / u.dot(u);
		c2 = 2.0f / v.dot(v);
		c3 = c1 * c2  * u.dot(v);
		
		for (i = 0; i < 3; i++) 
			{
			for (j = 0; j < 3; j++) 
				{
				mtx(j,i) =  - c1 * u[i] * u[j] - c2 * v[i] * v[j] + c3 * v[i] * u[j];
				}
			mtx(i,i) += 1.0;
			}
		}
	else  /* the most common case, unless "from"="to", or "from"=-"to" */
		{
#if 0
		/* unoptimized version - a good compiler will optimize this. */
		/* h = (1.0 - e)/DOT(v, v); old code */
		h = 1.0/(1.0 + e);      /* optimization by Gottfried Chen */
		mtx(0,0) = e + h * v[0] * v[0];
		mtx(1,0) = h * v[0] * v[1] - v[2];
		mtx(2,0) = h * v[0] * v[2] + v[1];
		
		mtx(0,1) = h * v[0] * v[1] + v[2];
		mtx(1,1) = e + h * v[1] * v[1];
		mtx(2,1) = h * v[1] * v[2] - v[0];
		
		mtx(0,2) = h * v[0] * v[2] - v[1];
		mtx(1,2) = h * v[1] * v[2] + v[0];
		mtx(2,2) = e + h * v[2] * v[2];
#else
		/* ...otherwise use this hand optimized version (9 mults less) */
		NxReal hvx, hvz, hvxy, hvxz, hvyz;
		/* h = (1.0 - e)/DOT(v, v); old code */
		h = 1.0f/(1.0f + e);      /* optimization by Gottfried Chen */
		hvx = h * v[0];
		hvz = h * v[2];
		hvxy = hvx * v[1];
		hvxz = hvx * v[2];
		hvyz = hvz * v[1];
		mtx(0,0) = e + hvx * v[0];
		mtx(1,0) = hvxy - v[2];
		mtx(2,0) = hvxz + v[1];
		
		mtx(0,1) = hvxy + v[2];
		mtx(1,1) = e + h * v[1] * v[1];
		mtx(2,1) = hvyz - v[0];
		
		mtx(0,2) = hvxz - v[1];
		mtx(1,2) = hvyz + v[0];
		mtx(2,2) = e + hvz * v[2];
#endif
		}
	}


