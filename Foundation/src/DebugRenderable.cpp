/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "Nxf.h"
#include "NxUtilities.h"
#include "DebugRenderable.h"


namespace NxFoundation
	{

	static void computeScaledCircleSinCos(NxU32 index, NxF32 step, NxF32 radius, NxF32& sine, NxF32& cosine)
		{
#if defined(_MSC_VER) && defined(_M_IX86)
		NxI32 signedIndex = NxI32(index);
		__asm
			{
			fild signedIndex
			fmul step
			fld st(0)
			fsin
			fmul radius
			mov eax, sine
			fstp dword ptr [eax]
			fcos
			fmul radius
			mov eax, cosine
			fstp dword ptr [eax]
			}
#else
		NxF32 angle = NxF32(index) * step;
		sine = radius * sinf(angle);
		cosine = radius * cosf(angle);
#endif
		}

	static void computeScaledCircleSinCosRounded(NxU32 index, NxF32 step, NxF32 radius, NxF32& sine, NxF32& cosine)
		{
#if defined(_MSC_VER) && defined(_M_IX86)
		NxI32 signedIndex = NxI32(index);
		NxF32 angle;
		__asm
			{
			fild signedIndex
			fmul step
			fstp angle
			fld angle
			fld st(0)
			fsin
			fmul radius
			mov eax, sine
			fstp dword ptr [eax]
			fcos
			fmul radius
			mov eax, cosine
			fstp dword ptr [eax]
			}
#else
		NxF32 angle = NxF32(index) * step;
		sine = radius * sinf(angle);
		cosine = radius * cosf(angle);
#endif
		}

	DebugRenderable::DebugRenderable()
		{
		}

	DebugRenderable::~DebugRenderable()
		{
		}

	NxU32 DebugRenderable::getNbPoints() const
		{
		return pointsArray.size();
		}
	const NxDebugPoint* DebugRenderable::getPoints() const
		{
		return pointsArray.begin();
		}

	NxU32 DebugRenderable::getNbLines() const
		{
		return linesArray.size();
		}
	const NxDebugLine* DebugRenderable::getLines() const
		{
		return linesArray.begin();
		}

	NxU32 DebugRenderable::getNbTriangles() const
		{
		return trianglesArray.size();
		}
	const NxDebugTriangle* DebugRenderable::getTriangles() const
		{
		return trianglesArray.begin();
		}

	void DebugRenderable::addPoint(const NxVec3& p, NxU32 color)
		{
			NxDebugPoint tmp;
			tmp.p = p;
			tmp.color = color;
			pointsArray.pushBack(tmp);
		}

	void DebugRenderable::addLine(const NxVec3& p0, const NxVec3& p1, NxU32 color)
		{
			NxDebugLine tmp;
			tmp.p0 = p0;
			tmp.p1 = p1;
			tmp.color = color;
			linesArray.pushBack(tmp);
		}

	void DebugRenderable::addTriangle(const NxVec3& p0, const NxVec3& p1, const NxVec3& p2, NxU32 color)
		{
			NxDebugTriangle tmp;
			tmp.p0 = p0;
			tmp.p1 = p1;
			tmp.p2 = p2;
			tmp.color = color;
			trianglesArray.pushBack(tmp);
		}

	void DebugRenderable::clear()
		{
		pointsArray.clear();
		linesArray.clear();
		trianglesArray.clear();
		}

	
	void DebugRenderable::addOBB(const NxBox& box, NxU32 color, bool render_frame)
		{
		// Compute box vertices
		NxVec3 pp[8];
		box.computePoints(pp);
		
		// Draw all lines
		const NxU32* Indices = box.getEdges();
		for(NxU32 i=0;i<12;i++)
			{
			NxU32 VRef0 = *Indices++;
			NxU32 VRef1 = *Indices++;
			addLine(pp[VRef0], pp[VRef1], color);
			}
		
		// Render frame if needed
		if(render_frame)
			{
			NxVec3 Axis0; box.rot.getColumn(0, Axis0);
			NxVec3 Axis1; box.rot.getColumn(1, Axis1);
			NxVec3 Axis2; box.rot.getColumn(2, Axis2);
			
			addLine(box.center, box.center + Axis0, 0x00ff0000);
			addLine(box.center, box.center + Axis1, 0x0000ff00);
			addLine(box.center, box.center + Axis2, 0x000000ff);
			}
		}
	
	void DebugRenderable::addAABB(const NxBounds3& bounds, NxU32 color, bool renderFrame)
		{
		// Reuse OBB code...
		NxVec3 center;	bounds.getCenter(center);
		NxVec3 extents;	bounds.getExtents(extents);
		NxMat33 id;	id.id();
		addOBB(NxBox(center, extents, id), color, renderFrame);
		}

	void DebugRenderable::addArrow(const NxVec3 & position, const NxVec3 & direction, NxReal length, NxReal scale, NxU32 color)
		{
		//direction is assumed to be normalized!!
		//the arrow's tip has length		1 * scale;
		//the arrow has length				length * scale
		//
		// Written in the order of the oracle's x87 stream (NxFoundation.dll
		// 0x10001640-0x10001858), with the NxNormalToTangents rule (Utilities.cpp):
		// a value the listing keeps on the register stack is NxF64, a value it
		// stores to a dword is NxF32 and is read back as that float (_PC_53, so a
		// register lifetime is a double). With the plain NxVec3 operators the tip
		// and lobes came out one ULP off the oracle's in NxPhysicsSceneVisualizeTests.
		//
		// `fld length; fmul scale; fstp` (0x10001643-0x10001654): the length is
		// spilled; both tests are `fcomp 0.0f` with `test ah, 0x41`, so a NaN or
		// non-positive operand draws nothing.
		const NxReal arrowLength = length * scale;
		if (length > 0 && scale > 0)
			{
			// 0x10001688-0x100016db: the x and y products stay on the stack; the z
			// product is spilled before its sum.
			NxVec3 tip;
			const NxF32 dzL = direction.z * arrowLength;
			tip.x = static_cast<NxF32>(static_cast<NxF64>(arrowLength) * direction.x + position.x);
			tip.y = static_cast<NxF32>(static_cast<NxF64>(arrowLength) * direction.y + position.y);
			tip.z = dzL + position.z;
			addLine(position, tip, color);

			NxVec3 t1,t2;
			NxNormalToTangents(direction, t1, t2);

			//the arrow head should be 1/4th of the arrow length
			//all this world space guesswork is lame. we need the arrows to be constant size in
			//screenspace while they are smaller than 1/4th of the arrow.

			// `fmul 0.15f; fst dword` (0x100016f6-0x100016ff): the stored float
			// scales every term except tipBase.x, which multiplies the register
			// copy.
			const NxF64 headScaleWide = static_cast<NxF64>(arrowLength) * 0.15f;
			const NxF32 headScale = static_cast<NxF32>(headScaleWide);

/*
			NxReal headScale = scale;
			if (arrowLength < 4)
				headScale = arrowLength * 0.25f;
*/

			// tipBase (0x10001703-0x1000172f): x spilled; y and z stay on the stack
			// for all four lobes, z subtracting a spilled product.
			const NxF32 tipBaseX = static_cast<NxF32>(tip.x - headScaleWide * direction.x);
			const NxF64 tipBaseY = tip.y - static_cast<NxF64>(headScale) * direction.y;
			const NxF32 dzH = headScale * direction.z;
			const NxF64 tipBaseZ = static_cast<NxF64>(tip.z) - dzH;

			// Lobes (0x10001733-0x10001812): per tangent, the x product is
			// spilled before both its sum and its difference, the y product is
			// kept for both, and the z product is kept for the sum and spilled for
			// the difference.
			NxVec3 lobe1, lobe2, lobe3, lobe4;
			{
			const NxF32 sx = t1.x * headScale;
			const NxF64 wy = static_cast<NxF64>(t1.y) * headScale;
			const NxF64 wz = static_cast<NxF64>(t1.z) * headScale;
			const NxF32 sz = static_cast<NxF32>(wz);
			lobe1.x = sx + tipBaseX;
			lobe1.y = static_cast<NxF32>(wy + tipBaseY);
			lobe1.z = static_cast<NxF32>(wz + tipBaseZ);
			lobe2.x = tipBaseX - sx;
			lobe2.y = static_cast<NxF32>(tipBaseY - wy);
			lobe2.z = static_cast<NxF32>(tipBaseZ - sz);
			}
			{
			const NxF32 sx = t2.x * headScale;
			const NxF64 wy = static_cast<NxF64>(t2.y) * headScale;
			const NxF64 wz = static_cast<NxF64>(t2.z) * headScale;
			const NxF32 sz = static_cast<NxF32>(wz);
			lobe3.x = sx + tipBaseX;
			lobe3.y = static_cast<NxF32>(wy + tipBaseY);
			lobe3.z = static_cast<NxF32>(wz + tipBaseZ);
			lobe4.x = tipBaseX - sx;
			lobe4.y = static_cast<NxF32>(tipBaseY - wy);
			lobe4.z = static_cast<NxF32>(tipBaseZ - sz);
			}
			addLine(tip, lobe1, color);
			addLine(tip, lobe2, color);
			addLine(tip, lobe3, color);
			addLine(tip, lobe4, color);
			}
		}

	void DebugRenderable::addBasis(const NxVec3 & position, const NxMat33 & columns, const NxVec3 & lengths, NxReal scale, NxU32 colors[3])
		{
		// 0x10001860-0x1000192a: each column through the addArrow slot (+0x30);
		// a null colors array draws with colour 0 (`test ebx, ebx` per arrow).
		NxVec3 dir;
		columns.getColumn(0, dir);
		addArrow(position, dir, lengths[0], scale, colors ? colors[0] : 0);
		columns.getColumn(1, dir);
		addArrow(position, dir, lengths[1], scale, colors ? colors[1] : 0);
		columns.getColumn(2, dir);
		addArrow(position, dir, lengths[2], scale, colors ? colors[2] : 0);
		}

	void DebugRenderable::addCircle(NxU32 nbSegments, const NxMat34& matrix, NxU32 color, NxF32 radius, bool semicircle)
		{
		NxF32 step = NxTwoPiF32/NxF32(nbSegments);
		NxU32 segs = nbSegments;
		if (semicircle)	
			{
			segs /= 2;
			}

		for(NxU32 i=0;i<segs;i++)
			{
			NxU32 j=i+1;
			if(j==nbSegments)	j=0;

			NxF32 sine0, cosine0, sine1, cosine1;
			computeScaledCircleSinCos(i, step, radius, sine0, cosine0);
			computeScaledCircleSinCosRounded(j, step, radius, sine1, cosine1);
			NxVec3 p0,p1;
			matrix.multiply(NxVec3(sine0, cosine0, 0.0f), p0);
			matrix.multiply(NxVec3(sine1, cosine1, 0.0f), p1);

			addLine(p0, p1, color);
			}
		}

	}
