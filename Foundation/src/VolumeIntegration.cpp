/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
/*
*	This code computes volume integrals needed to compute mass properties of polyhedral bodies.
*	Based on public domain code by Brian Mirtich.
*/
#include <string.h>
#include "Nxf.h"
#include "NxVolumeIntegration.h"
#include "NxSimpleTriangleMesh.h"

#include "NxUserAllocator.h"
extern NXF_DLL_EXPORT NxUserAllocator * nxFoundationSDKAllocator;	//the SDK defines this.

// A volume integrator
class VolumeIntegrator
	{
	public:
		// Constructor/Destructor
		VolumeIntegrator(NxSimpleTriangleMesh mesh, NxF64	mDensity);
		~VolumeIntegrator();
		bool	computeVolumeIntegrals(NxIntegrals& ir);
	private:
		struct Normal
			{
			NxPoint normal;
			NxF32 w;
			};
		
		struct Face
			{
			NxF64 Norm[3];
			NxF64	w;
			NxU32	Verts[3];
			} f;
		
		// Data structures
		NxF64				mMass;					//!< Mass
		NxF64				mDensity;				//!< Density
		NxSimpleTriangleMesh mesh;	
		Normal	*			faceNormals;			//!< temp face normal data structure
		
		
		
		
		int					mA;						//!< Alpha
		int					mB;						//!< Beta
		int					mC;						//!< Gamma
		
		// Projection integrals
		NxF64				mP1;
		NxF64				mPa;					//!< Pi Alpha
		NxF64				mPb;					//!< Pi Beta
		NxF64				mPaa;					//!< Pi Alpha^2
		NxF64				mPab;					//!< Pi AlphaBeta
		NxF64				mPbb;					//!< Pi Beta^2
		NxF64				mPaaa;					//!< Pi Alpha^3
		NxF64				mPaab;					//!< Pi Alpha^2Beta
		NxF64				mPabb;					//!< Pi AlphaBeta^2
		NxF64				mPbbb;					//!< Pi Beta^3
		
		// Face integrals
		NxF64				mFa;					//!< FAlpha
		NxF64				mFb;					//!< FBeta
		NxF64				mFc;					//!< FGamma
		NxF64				mFaa;					//!< FAlpha^2
		NxF64				mFbb;					//!< FBeta^2
		NxF64				mFcc;					//!< FGamma^2
		NxF64				mFaaa;					//!< FAlpha^3
		NxF64				mFbbb;					//!< FBeta^3
		NxF64				mFccc;					//!< FGamma^3
		NxF64				mFaab;					//!< FAlpha^2Beta
		NxF64				mFbbc;					//!< FBeta^2Gamma
		NxF64				mFcca;					//!< FGamma^2Alpha
		
		// The 10 volume integrals
		NxF64				mT0;					//!< ~Total mass
		NxF64				mT1[3];					//!< Location of the center of mass
		NxF64				mT2[3];					//!< Moments of inertia
		NxF64				mTP[3];					//!< Products of inertia
		
		// Internal methods
		//				bool				Init();
		NxVec3				computeCenterOfMass();
		void				computeInertiaTensor(NxF64* J);
		void				computeCOMInertiaTensor(NxF64* J);
		void				computeFaceNormal(Face & f, NxU32 * indices);
		
		void				computeProjectionIntegrals(const Face& f);
		void				computeFaceIntegrals(const Face& f);
	};

#define X 0
#define Y 1
#define Z 2

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
*	Constructor.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VolumeIntegrator::VolumeIntegrator(NxSimpleTriangleMesh mesh, NxF64	density)
	{
	mDensity	= density;
	mMass		= 0.0;
	this->mesh	= mesh;
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
*	Destructor.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VolumeIntegrator::~VolumeIntegrator()
	{
	}

void VolumeIntegrator::computeFaceNormal(Face & f, NxU32 * indices)
	{
	const NxU8 * vertPointer = (const NxU8 *)mesh.points;
	const NxU8 * trigPointer = (const NxU8 *)mesh.triangles;
	
	//two edges
	NxPoint d1 = (*(NxPoint *)(vertPointer + mesh.pointStrideBytes * indices[1] )) - (*(NxPoint *)(vertPointer + mesh.pointStrideBytes * indices[0] ));
	NxPoint d2 = (*(NxPoint *)(vertPointer + mesh.pointStrideBytes * indices[2] )) - (*(NxPoint *)(vertPointer + mesh.pointStrideBytes * indices[1] ));
	
	
	NxVec3 normal = d1.cross(d2);
	
	normal.normalize();
	
	f.w = - normal.dot(		(*(NxPoint *)(vertPointer + mesh.pointStrideBytes * indices[0] ))	); 
	
	f.Norm[0] = normal.x;
	f.Norm[1] = normal.y;
	f.Norm[2] = normal.z;
	
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
*	Computes volume integrals for a polyhedron by summing surface integrals over its faces.
*	\param		ir	[out] a result structure.
*	\return		true if success
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool VolumeIntegrator::computeVolumeIntegrals(NxIntegrals& ir)
	{
	if(!nxFoundationSDKAllocator)	return false;
	
	// Clear all integrals
	mT0 = mT1[X] = mT1[Y] = mT1[Z] = mT2[X] = mT2[Y] = mT2[Z] = mTP[X] = mTP[Y] = mTP[Z] = 0;
	
	Face f;
	const NxU8 * trigPointer = (const NxU8 *)mesh.triangles;
	for(NxU32 i=0;i<mesh.numTriangles;i++, trigPointer += mesh.triangleStrideBytes)
		{
		
		if (mesh.flags & NX_MF_16_BIT_INDICES)
			{
			f.Verts[0] = ((NxU16 *)trigPointer)[0];
			f.Verts[1] = ((NxU16 *)trigPointer)[1];
			f.Verts[2] = ((NxU16 *)trigPointer)[2];
			}
		else
			{
			f.Verts[0] = ((NxU32 *)trigPointer)[0];
			f.Verts[1] = ((NxU32 *)trigPointer)[1];
			f.Verts[2] = ((NxU32 *)trigPointer)[2];
			}

		if (mesh.flags & NX_MF_FLIPNORMALS)
			{
			NxU32 t = f.Verts[1];
			f.Verts[1] = f.Verts[2];
			f.Verts[2] = t;
			}

		
		//compute face normal:
		computeFaceNormal(f,f.Verts);
		
		
		// Compute alpha/beta/gamma as the right-handed permutation of (x,y,z) that maximizes |n|
		NxF64 nx = fabs(f.Norm[X]);
		NxF64 ny = fabs(f.Norm[Y]);
		NxF64 nz = fabs(f.Norm[Z]);
		if (nx > ny && nx > nz) mC = X;
		else mC = (ny > nz) ? Y : Z;
		mA = (mC + 1) % 3;
		mB = (mA + 1) % 3;
		
		// Compute face contribution
		computeFaceIntegrals(f);
		
		// Update integrals
		mT0 += f.Norm[X] * ((mA == X) ? mFa : ((mB == X) ? mFb : mFc));
		
		mT1[mA] += f.Norm[mA] * mFaa;
		mT1[mB] += f.Norm[mB] * mFbb;
		mT1[mC] += f.Norm[mC] * mFcc;
		
		mT2[mA] += f.Norm[mA] * mFaaa;
		mT2[mB] += f.Norm[mB] * mFbbb;
		mT2[mC] += f.Norm[mC] * mFccc;
		
		mTP[mA] += f.Norm[mA] * mFaab;
		mTP[mB] += f.Norm[mB] * mFbbc;
		mTP[mC] += f.Norm[mC] * mFcca;
		}
	
	mT1[X] /= 2; mT1[Y] /= 2; mT1[Z] /= 2;
	mT2[X] /= 3; mT2[Y] /= 3; mT2[Z] /= 3;
	mTP[X] /= 2; mTP[Y] /= 2; mTP[Z] /= 2;
	
	// Fill result structure
	ir.COM = computeCenterOfMass();
	computeInertiaTensor((NxF64*)ir.inertiaTensor);
	computeCOMInertiaTensor((NxF64*)ir.COMInertiaTensor);
	ir.mass = mMass;
	return true;
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
*	Computes the center of mass.
*	\return		The center of mass.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
NxVec3 VolumeIntegrator::computeCenterOfMass()
	{
	NxVec3 COM;
	// Compute center of mass
	COM.x = float(mT1[X] / mT0);
	COM.y = float(mT1[Y] / mT0);
	COM.z = float(mT1[Z] / mT0);
	
	return COM;
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
*	Setups the inertia tensor relative to the origin.
*	\param		it	[out] the returned inertia tensor.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VolumeIntegrator::computeInertiaTensor(NxF64* it)
	{
	NxF64 J[3][3];
	
	// Compute inertia tensor
	// Moments d'inertie
	// mT2[X] = intégrale de X^2 sur le solide
	// mT2[Y] = intégrale de Y^2 sur le solide
	// mT2[Z] = intégrale de Z^2 sur le solide
	J[X][X] = mDensity * (mT2[Y] + mT2[Z]);
	J[Y][Y] = mDensity * (mT2[Z] + mT2[X]);
	J[Z][Z] = mDensity * (mT2[X] + mT2[Y]);
	
	// Produits d'inertie
	// mTP[X] = intégrale de xy sur le solide
	// mTP[Y] = intégrale de yz sur le solide
	// mTP[Z] = intégrale de zx sur le solide
	J[X][Y] = J[Y][X] = - mDensity * mTP[X];
	J[Y][Z] = J[Z][Y] = - mDensity * mTP[Y];
	J[Z][X] = J[X][Z] = - mDensity * mTP[Z];
	
	memcpy(it, J, 9*sizeof(NxF64));
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
*	Setups the inertia tensor relative to the COM.
*	\param		it	[out] the returned inertia tensor.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VolumeIntegrator::computeCOMInertiaTensor(NxF64* it)
	{
	NxF64 J[3][3];
	
	mMass = mDensity * mT0;
	
	NxVec3 COM = computeCenterOfMass();
	
	// Compute initial inertia tensor
	computeInertiaTensor((NxF64*)J);
	
	// Translate inertia tensor to center of mass
	// Pour les moments d'inertie: (théorème d'Huyghens)
	// Jx'x' = Jxx - m*(YG^2+ZG^2)
	// Jy'y' = Jyy - m*(ZG^2+XG^2)
	// Jz'z' = Jzz - m*(XG^2+YG^2)
	// XG, YG, ZG = nouvelle origine (== centre de masse donc)
	// YG^2+ZG^2 = dx^2, on retrouve le fameux rayon de giration du solide...
	J[X][X] -= mMass * (COM.y*COM.y + COM.z*COM.z);
	J[Y][Y] -= mMass * (COM.z*COM.z + COM.x*COM.x);
	J[Z][Z] -= mMass * (COM.x*COM.x + COM.y*COM.y);
	// Pour les produits d'inertie: (théorème d'Huyghens)
	// Jx'y' = Jxy - m*XG*YG
	// Jy'z' = Jyz - m*YG*ZG
	// Jz'x' = Jzx - m*ZG*XG
	// ### IS THE SIGN CORRECT ?
	J[X][Y] = J[Y][X] += mMass * COM.x * COM.y;
	J[Y][Z] = J[Z][Y] += mMass * COM.y * COM.z;
	J[Z][X] = J[X][Z] += mMass * COM.z * COM.x;
	
	memcpy(it, J, 9*sizeof(NxF64));
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
*	Computes integrals over a face projection from the coordinates of the projections vertices.
*	\param		f	[in] a face structure.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VolumeIntegrator::computeProjectionIntegrals(const Face& f)
	{
	mP1 = mPa = mPb = mPaa = mPab = mPbb = mPaaa = mPaab = mPabb = mPbbb = 0.0;
	
	const NxU8 * vertPointer = (const NxU8 *)mesh.points;
	for(NxU32 i=0;i<3;i++)
		{
		NxPoint & p0 = (*(NxPoint *)(vertPointer + mesh.pointStrideBytes * (f.Verts[i]) ));
		NxPoint & p1 = (*(NxPoint *)(vertPointer + mesh.pointStrideBytes * (f.Verts[(i+1) % 3]) ));
		
		
		NxF64 a0 = p0[mA];
		NxF64 b0 = p0[mB];
		NxF64 a1 = p1[mA];
		NxF64 b1 = p1[mB];
		
		NxF64 da = a1 - a0;				// DeltaA
		NxF64 db = b1 - b0;				// DeltaB
		
		NxF64 a0_2 = a0 * a0;				// Alpha0^2
		NxF64 a0_3 = a0_2 * a0;			// ...
		NxF64 a0_4 = a0_3 * a0;
		
		NxF64 b0_2 = b0 * b0;
		NxF64 b0_3 = b0_2 * b0;
		NxF64 b0_4 = b0_3 * b0;
		
		NxF64 a1_2 = a1 * a1;
		NxF64 a1_3 = a1_2 * a1; 
		
		NxF64 b1_2 = b1 * b1;
		NxF64 b1_3 = b1_2 * b1;
		
		NxF64 C1 = a1 + a0;
		
		NxF64 Ca = a1*C1 + a0_2;
		NxF64 Caa = a1*Ca + a0_3;
		NxF64 Caaa = a1*Caa + a0_4;
		
		NxF64 Cb = b1*(b1 + b0) + b0_2;
		NxF64 Cbb = b1*Cb + b0_3;
		NxF64 Cbbb = b1*Cbb + b0_4;
		
		NxF64 Cab = 3*a1_2 + 2*a1*a0 + a0_2;
		NxF64 Kab = a1_2 + 2*a1*a0 + 3*a0_2;
		
		NxF64 Caab = a0*Cab + 4*a1_3;
		NxF64 Kaab = a1*Kab + 4*a0_3;
		
		NxF64 Cabb = 4*b1_3 + 3*b1_2*b0 + 2*b1*b0_2 + b0_3;
		NxF64 Kabb = b1_3 + 2*b1_2*b0 + 3*b1*b0_2 + 4*b0_3;
		
		mP1		+= db*C1;
		mPa		+= db*Ca;
		mPaa	+= db*Caa;
		mPaaa	+= db*Caaa;
		mPb		+= da*Cb;
		mPbb	+= da*Cbb;
		mPbbb	+= da*Cbbb;
		mPab	+= db*(b1*Cab + b0*Kab);
		mPaab	+= db*(b1*Caab + b0*Kaab);
		mPabb	+= da*(a1*Cabb + a0*Kabb);
		}
	
	mP1		/= 2.0;
	mPa		/= 6.0;
	mPaa	/= 12.0;
	mPaaa	/= 20.0;
	mPb		/= -6.0;
	mPbb	/= -12.0;
	mPbbb	/= -20.0;
	mPab	/= 24.0;
	mPaab	/= 60.0;
	mPabb	/= -60.0;
	}

#define		SQR(x)			((x)*(x))						//!< Returns x square
#define		CUBE(x)			((x)*(x)*(x))					//!< Returns x cube

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
*	Computes surface integrals over a polyhedral face from the integrals over its projection.
*	\param		f	[in] a face structure.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VolumeIntegrator::computeFaceIntegrals(const Face& f)
	{
	computeProjectionIntegrals(f);
	
	NxF64 w = f.w;
	const NxF64* n = f.Norm;
	NxF64 k1 = 1 / n[mC];
	NxF64 k2 = k1 * k1;
	NxF64 k3 = k2 * k1;
	NxF64 k4 = k3 * k1;
	
	mFa = k1 * mPa;
	mFb = k1 * mPb;
	mFc = -k2 * (n[mA]*mPa + n[mB]*mPb + w*mP1);
	
	mFaa = k1 * mPaa;
	mFbb = k1 * mPbb;
	mFcc = k3 * (SQR(n[mA])*mPaa + 2*n[mA]*n[mB]*mPab + SQR(n[mB])*mPbb + w*(2*(n[mA]*mPa + n[mB]*mPb) + w*mP1));
	
	mFaaa = k1 * mPaaa;
	mFbbb = k1 * mPbbb;
	mFccc = -k4 * (CUBE(n[mA])*mPaaa + 3*SQR(n[mA])*n[mB]*mPaab 
		+ 3*n[mA]*SQR(n[mB])*mPabb + CUBE(n[mB])*mPbbb
		+ 3*w*(SQR(n[mA])*mPaa + 2*n[mA]*n[mB]*mPab + SQR(n[mB])*mPbb)
		+ w*w*(3*(n[mA]*mPa + n[mB]*mPb) + w*mP1));
	
	mFaab = k1 * mPaab;
	mFbbc = -k2 * (n[mA]*mPabb + n[mB]*mPbbb + w*mPbb);
	mFcca = k3 * (SQR(n[mA])*mPaaa + 2*n[mA]*n[mB]*mPaab + SQR(n[mB])*mPabb + w*(2*(n[mA]*mPaa + n[mB]*mPab) + w*mPa));
	}


// Wrapper
bool NxComputeVolumeIntegrals(const NxSimpleTriangleMesh& mesh, NxReal density, NxIntegrals& integrals)
	{
	VolumeIntegrator v(mesh, density);
	return v.computeVolumeIntegrals(integrals);
	}
