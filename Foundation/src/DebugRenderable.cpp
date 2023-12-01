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
	
	void DebugRenderable::addAABB(const NxBounds3& bounds)
		{
		// Reuse OBB code...
		NxVec3 center;	bounds.getCenter(center);
		NxVec3 extents;	bounds.getExtents(extents);
		NxMat33 id;	id.id();
		addOBB(NxBox(center, extents, id), 0x00ffff00, false);
		}

	void DebugRenderable::addArrow(const NxVec3 & position, const NxVec3 & direction, NxReal length, NxReal scale, NxU32 color)
		{
		//direction is assumed to be normalized!!
		//the arrow's tip has length		1 * scale;
		//the arrow has length				length * scale
		NxReal arrowLength = scale * length;
		if (length > 0 && scale > 0)
			{
			NxVec3 tip = position + direction * arrowLength;
			addLine(position, tip, color);

			NxVec3 t1,t2;
			NxNormalToTangents(direction, t1, t2);

			//the arrow head should be 1/4th of the arrow length
			//all this world space guesswork is lame. we need the arrows to be constant size in 
			//screenspace while they are smaller than 1/4th of the arrow.

			NxReal headScale = arrowLength * 0.15f;

/*
			NxReal headScale = scale;
			if (arrowLength < 4)
				headScale = arrowLength * 0.25f;
*/			

			NxVec3 tipBase = tip - direction * headScale;
			NxVec3 lobe1  = tipBase + t1 * headScale;
			NxVec3 lobe2  = tipBase - t1 * headScale;
			NxVec3 lobe3  = tipBase + t2 * headScale;
			NxVec3 lobe4  = tipBase - t2 * headScale;
			addLine(tip, lobe1, color);
			addLine(tip, lobe2, color);
			addLine(tip, lobe3, color);
			addLine(tip, lobe4, color);
			}
		}

	void DebugRenderable::addBasis(const NxVec3 & position, const NxMat33 & columns, const NxVec3 & lengths, NxReal scale, NxU32 colors[3])
		{
		NxVec3 dir;
		columns.getColumn(0, dir);
		addArrow(position, dir, lengths[0], scale, colors[0]);
		columns.getColumn(1, dir);
		addArrow(position, dir, lengths[1], scale, colors[1]);
		columns.getColumn(2, dir);
		addArrow(position, dir, lengths[2], scale, colors[2]);
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

			NxF32 angle0 = NxF32(i)*step;
			NxF32 angle1 = NxF32(j)*step;

			NxVec3 p0,p1;
			matrix.multiply(NxVec3(radius * sinf(angle0), radius * cosf(angle0), 0.0f), p0);
			matrix.multiply(NxVec3(radius * sinf(angle1), radius * cosf(angle1), 0.0f), p1);

			addLine(p0, p1, color);
			}
		}

	}