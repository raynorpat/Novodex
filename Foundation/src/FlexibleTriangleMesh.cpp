/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#error Obsolete!
#include "Nxf.h"
#include "NxFlexibleTriangleMesh.h"

	#define VF_POSITION_MASK	0x00E
	#define VF_TEXCOUNT_MASK	0xf00
	#define VF_TEXCOUNT_SHIFT	8

	#define VF_TEXTUREFORMAT2	0			//!< Two floating point values
	#define VF_TEXTUREFORMAT1	3			//!< One floating point value
	#define VF_TEXTUREFORMAT3	1			//!< Three floating point values
	#define VF_TEXTUREFORMAT4	2			//!< Four floating point values

	#define VF_TEXCOORDSIZE3(CoordIndex)	(VF_TEXTUREFORMAT3 << (CoordIndex*2 + 16))
	#define VF_TEXCOORDSIZE2(CoordIndex)	(VF_TEXTUREFORMAT2)
	#define VF_TEXCOORDSIZE4(CoordIndex)	(VF_TEXTUREFORMAT4 << (CoordIndex*2 + 16))
	#define VF_TEXCOORDSIZE1(CoordIndex)	(VF_TEXTUREFORMAT1 << (CoordIndex*2 + 16))

	#define BOOL	int
	#define	INVALID_ID	0xffffffff

NX_INLINE NxU32 ComputeTexCount(VertexFormat fvf)
{
	return (fvf & VF_TEXCOUNT_MASK)>>VF_TEXCOUNT_SHIFT;
}

#define FVF_CACHE

#ifdef FVF_CACHE
// Little cache
static NxU32 gPreviousFVF = 0;
static NxU32 gPreviousSize = 0;
static NxU32 gNbCalls = 0;
static NxU32 gCacheHits = 0;
#endif

/**
 *	Computes the size of a vertex - given that a vertex is an entity made of a point, a normal, a color, and so on.
 *	This method has been reverse-engineered from D3DX.
 *	\param		fvf		[in] flexible vertex format
 *	\return		the vertex size in bytes
 */
NxU32 ComputeFVFSize(VertexFormat fvf)
{
#ifdef FVF_CACHE
	gNbCalls++;
	if(fvf==gPreviousFVF)
	{
		gCacheHits++;
		return gPreviousSize;
	}
	gPreviousFVF = fvf;
#endif

	NxU32 VertexSize = 0;

	switch(fvf & VF_POSITION_MASK)
	{
		case NX_VF_XYZ:		VertexSize = 12;	break;
		case NX_VF_XYZRHW:	VertexSize = 16;	break;
		case NX_VF_XYZB1:	VertexSize = 16;	break;
		case NX_VF_XYZB2:	VertexSize = 20;	break;
		case NX_VF_XYZB3:	VertexSize = 24;	break;
		case NX_VF_XYZB4:	VertexSize = 28;	break;
		case NX_VF_XYZB5:	VertexSize = 32;	break;
	}

	if(fvf & NX_VF_NORMAL)		VertexSize+=12;
	if(fvf & NX_VF_PSIZE)		VertexSize+=4;
	if(fvf & NX_VF_DIFFUSE)		VertexSize+=4;
	if(fvf & NX_VF_SPECULAR)	VertexSize+=4;

//	int TexCount = (fvf & VF_TEXCOUNT_MASK)>>VF_TEXCOUNT_SHIFT;
	int TexCount = ComputeTexCount(fvf);
	if(!TexCount)
	{
#ifdef FVF_CACHE
		gPreviousSize = VertexSize;
#endif
		return VertexSize;
	}

	fvf>>=16;	// Comes from the definitions of VF_TEXCOORDSIZE macros
	if(!fvf)
	{
		// No VF_TEXCOORDSIZE macros defined here
		VertexSize+=TexCount<<3;
#ifdef FVF_CACHE
		gPreviousSize = VertexSize;
#endif
		return VertexSize;
	}

	do
	{
		switch(fvf & 3)
		{
			case VF_TEXTUREFORMAT1:	VertexSize+=4;	break;
			case VF_TEXTUREFORMAT2:	VertexSize+=8;	break;
			case VF_TEXTUREFORMAT3:	VertexSize+=12;	break;
			case VF_TEXTUREFORMAT4:	VertexSize+=16;	break;
		}
		fvf>>=2;

	}while(--TexCount);

/*
			if(fvf & VF_XYZ)		VertexSize += sizeof(NxF32)*3;
			if(fvf & VF_XYZRHW)		VertexSize += sizeof(NxF32)*4;
			if(fvf & VF_NORMAL)		VertexSize += sizeof(NxF32)*3;
			if(fvf & VF_DIFFUSE)	VertexSize += sizeof(NxU32);
			if(fvf & VF_SPECULAR)	VertexSize += sizeof(NxU32);
			if(fvf & VF_TEX1)		VertexSize += sizeof(NxF32)*2*1;
	else	if(fvf & VF_TEX2)		VertexSize += sizeof(NxF32)*2*2;
	else	if(fvf & VF_TEX3)		VertexSize += sizeof(NxF32)*2*3;
	else	if(fvf & VF_TEX4)		VertexSize += sizeof(NxF32)*2*4;
	else	if(fvf & VF_TEX5)		VertexSize += sizeof(NxF32)*2*5;
	else	if(fvf & VF_TEX6)		VertexSize += sizeof(NxF32)*2*6;
	else	if(fvf & VF_TEX7)		VertexSize += sizeof(NxF32)*2*7;
	else	if(fvf & VF_TEX8)		VertexSize += sizeof(NxF32)*2*8;
	// NB: classic bug here: VF_TEX0 actually means *no* texture coordinates. (that's why the flag is null)
*/
#ifdef FVF_CACHE
	gPreviousSize = VertexSize;
#endif
	return VertexSize;
}

// ### Hacked, not completely tested, handle with care.
NxU32 ComputeOffset(VertexFormat fvf, NxFVFComponent component)
{
	// Parse a big-fat FVF

	// 1) Position, RHW, blending weights
	NxU32 Offset = 0;
	{
		if(component==NX_FVF_POSITION)	return Offset;

		// Update offset
		switch(fvf & VF_POSITION_MASK)
		{
			case NX_VF_XYZ:		Offset += 12;	break;
			case NX_VF_XYZRHW:	Offset += 16;	break;
			case NX_VF_XYZB1:	Offset += 16;	break;
			case NX_VF_XYZB2:	Offset += 20;	break;
			case NX_VF_XYZB3:	Offset += 24;	break;
			case NX_VF_XYZB4:	Offset += 28;	break;
			case NX_VF_XYZB5:	Offset += 32;	break;
		}
	}

	// 2) Vertex normal
	{
		BOOL HasNormal = fvf & NX_VF_NORMAL;

		if(component==NX_FVF_NORMAL)	return HasNormal ? Offset : INVALID_ID;

		// Update offset
		if(HasNormal)	Offset += 12;
	}

	// 3) Point size
	{
		BOOL HasPSize = fvf & NX_VF_PSIZE;

		if(component==NX_FVF_PSIZE)	return HasPSize ? Offset : INVALID_ID;

		// Update offset
		if(HasPSize)	Offset += 4;
	}

	// 4) Diffuse
	{
		BOOL HasDiffuse = fvf & NX_VF_DIFFUSE;

		if(component==NX_FVF_DIFFUSE)	return HasDiffuse ? Offset : INVALID_ID;

		// Update offset
		if(HasDiffuse)	Offset += 4;
	}

	// 5) Specular
	{
		BOOL HasSpecular = fvf & NX_VF_SPECULAR;

		if(component==NX_FVF_SPECULAR)	return HasSpecular ? Offset : INVALID_ID;

		// Update offset
		if(HasSpecular)	Offset += 4;
	}

	// 6) UVs

//	int TexCount = (fvf & VF_TEXCOUNT_MASK)>>VF_TEXCOUNT_SHIFT;
	int TexCount = ComputeTexCount(fvf);
	if(!TexCount)	return INVALID_ID;

	int ID = component - NX_FVF_UV0;

	fvf>>=16;	// Comes from the definitions of VF_TEXCOORDSIZE macros
	if(!fvf)
	{
		// No VF_TEXCOORDSIZE macros defined here
		for(int i=0;i<TexCount;i++)
		{
			if(ID==i)	return Offset;
			Offset+=8;
		}
		return INVALID_ID;
//		VertexSize+=TexCount<<3;
//		return VertexSize;
	}

	int CurID = 0;
	do
	{
		NxU32 TexSize = 0;
		switch(fvf & 3)
		{
			case VF_TEXTUREFORMAT1:	TexSize=4;	break;
			case VF_TEXTUREFORMAT2:	TexSize=8;	break;
			case VF_TEXTUREFORMAT3:	TexSize=12;	break;
			case VF_TEXTUREFORMAT4:	TexSize=16;	break;
		}
		fvf>>=2;

		if(CurID==ID)	return Offset;
		Offset+=TexSize;
		CurID++;

	}while(--TexCount);

	return INVALID_ID;
}
