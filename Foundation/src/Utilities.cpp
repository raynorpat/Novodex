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

NxU32 NxCrc32(const void* buffer, NxU32 nbBytes)
	{
	const NxU8* bytes = static_cast<const NxU8*>(buffer);
	NxU32 crc = 0;
	while(nbBytes--)
		{
		crc ^= *bytes++;
		for(NxU32 bit=0; bit<8; bit++)
			crc = (crc & 1) ? (crc >> 1) ^ 0xedb88320 : crc >> 1;
		}
	return crc;
	}

void NxComputeBounds(NxVec3& min, NxVec3& max, NxU32 nbVerts, const NxVec3* verts)
	{
	if(!nbVerts || !verts)
		return;

	max.set(NX_MIN_F32, NX_MIN_F32, NX_MIN_F32);
	min.set(NX_MAX_F32, NX_MAX_F32, NX_MAX_F32);
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
	}

// 1.0 / x through the x87 unit, because the oracle divides with `fdivr` and an
// SSE divide differs on a degenerate input's NaN sign.
static NxF64 nxDivideOne(NxF64 x)
	{
#if defined(_M_IX86) && defined(_MSC_VER)
	NxF64 value = x;
	__asm
		{
		fld1
		fld qword ptr [value]
		fdivp st(1), st(0)
		fstp qword ptr [value]
		}
	return value;
#else
	return 1.0 / x;
#endif
	}


// NxVec3::normalize as the oracle inlines it into NxNormalToTangents
// (0x1000637e-0x100063cb for t1, 0x100063cd-0x10006415 for t2). It differs from
// the public header's in two ways that each move a word by one ULP: the squares
// are summed z first, (z*z + y*y) + x*x, and the magnitude and its reciprocal
// never leave the register stack, where the header rounds the magnitude to a
// float before dividing. The zero test is `fucompp` against 0.0f with `test ah,
// 0x44`, so only an exact zero skips the scale and a NaN magnitude does not.
static void nxNormalizeTangent(NxVec3& v)
	{
	const NxF64 zz = static_cast<NxF64>(v.z) * v.z;
	const NxF64 yy = static_cast<NxF64>(v.y) * v.y;
	const NxF64 xx = static_cast<NxF64>(v.x) * v.x;
	const NxF64 m = NxMath::sqrt((zz + yy) + xx);
	if (m != 0.0)
		{
		const NxF64 r = nxDivideOne(m);
		v.x = static_cast<NxReal>(static_cast<NxF64>(v.x) * r);
		v.y = static_cast<NxReal>(static_cast<NxF64>(v.y) * r);
		v.z = static_cast<NxReal>(static_cast<NxF64>(v.z) * r);
		}
	}

void NxNormalToTangents(const NxVec3 & n, NxVec3 & t1, NxVec3 & t2)
	{
	// Written down in the order of the oracle's x87 stream (0x100062b0-0x1000637e),
	// with one rule for types: a value the listing keeps on the register stack is
	// `NxF64`, and a value it stores to a `dword` -- a spill slot on its own frame
	// or an output component -- is `NxF32`, and is read back as that float. The
	// process runs at _PC_53, so a register lifetime is a double and each `fstp
	// dword` is one rounding. Which values are spilled is not a style choice: it
	// is where the words come from, and both arms round different things at
	// different points.
	//
	// The branch compares the float |n.z| with the double at [0x1001c3a0];
	// `test ah, 0x41` sends an unordered compare to the second arm, as `>` does.
	if (fabs(n.z) > M_SQRT1_2)
		{
		// `fstp dword ptr [esp]` at 0x100062dd: `a` is rounded to a float before
		// the square root, and the same float is read again for t2.x. An n whose
		// y*y + z*z overflows a float therefore gets k == 0, not a small k.
		const NxF32 a = static_cast<NxF32>(static_cast<NxF64>(n.y) * n.y + static_cast<NxF64>(n.z) * n.z);
		const NxF64 k = nxDivideOne(NxMath::sqrt(static_cast<NxF64>(a)));
		// `fst dword ptr [esp+0xc]`: the stored k feeds t1.y and t2.x, but t1.z is
		// multiplied from the register copy that stays on the stack.
		const NxF32 kStored = static_cast<NxF32>(k);
		const NxF64 ky = k * n.y;
		// `fst dword ptr [esp+4]`: t1.z's value is spilled here and reloaded as a
		// float for t2.y.
		const NxF32 kyStored = static_cast<NxF32>(ky);
		t1.x = 0.0f;
		// `fmul` then `fchs` (0x100062fe-0x10006307): the negation is of the
		// product. With k infinite the product is `0 * inf`, and negating the
		// operand instead would flip the sign of the NaN it produces.
		const NxF64 kz = -(static_cast<NxF64>(kStored) * n.z);
		t1.y = static_cast<NxF32>(kz);
		t1.z = static_cast<NxF32>(ky);
		// t2 = t1 x n with t1.x == 0, and neither product is scaled by k again:
		// t2.z multiplies n.x by the unrounded -(k*z) still on the stack
		// (0x10006311), t2.y by the spilled k*y (0x10006313-0x1000631d). n is read
		// after t1 is written, as the listing reads it, so an aliased call sees
		// the same words.
		const NxF64 t2z = kz * n.x;
		const NxF64 t2y = -(static_cast<NxF64>(kyStored) * n.x);
		t2.x = static_cast<NxReal>(static_cast<NxF64>(kStored) * a);
		t2.y = static_cast<NxReal>(t2y);
		t2.z = static_cast<NxReal>(t2z);
		}
	else
		{
		// This arm spills nothing: `a` and k stay on the stack throughout, so
		// both are doubles, and the only float read back is t1.x (0x10006362).
		const NxF64 a = static_cast<NxF64>(n.y) * n.y + static_cast<NxF64>(n.x) * n.x;
		// The oracle divides with `fdivr` (`0x10006345`), and the rebuilt
		// Foundation's .text contained zero `fdivr dword ptr` encodings, so the
		// division was compiled as an SSE divide. On a degenerate input the two
		// disagree about the NaN they produce, which is the sign the joint
		// differential sees. Reaching the instruction rather than the operator is
		// the same move NxMath::sqrt needed.
		const NxF64 k = nxDivideOne(NxMath::sqrt(a));
		const NxF64 kx = k * n.x;
		const NxF64 ky = k * n.y;
		t1.z = 0.0f;
		t1.x = static_cast<NxF32>(-ky);
		t1.y = static_cast<NxF32>(kx);
		// t2 = n x t1 with t1.z == 0. t2.y reloads the stored t1.x, but t2.x takes
		// the unrounded k*n.x from the stack (0x10006367-0x10006370) rather than
		// the stored t1.y -- the two differ by one ULP whenever the store rounds.
		const NxF64 t1xz = static_cast<NxF64>(t1.x) * n.z;
		const NxF64 kxz = kx * n.z;
		t2.x = static_cast<NxReal>(-kxz);
		t2.y = static_cast<NxReal>(t1xz);
		t2.z = static_cast<NxReal>(k * a);
		}
	nxNormalizeTangent(t1);
	nxNormalizeTangent(t2);
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
	
	v.cross(to, from);
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


