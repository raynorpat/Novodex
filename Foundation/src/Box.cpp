/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/

#include "Nxf.h"
#include "NxBounds3.h"
#include "NxBox.h"
#include "NxCapsule.h"
#include "NxPlane.h"

#define	INVSQRT2 0.707106781188f	//!< 1 / sqrt(2)
#define	INVSQRT3 0.577350269189f	//!< 1 / sqrt(3)

// point *= matrix33
NX_INLINE void LeftTransformPoint(const NxVec3& src, NxVec3& dst, const NxMat33& mat33)
{
	// We have to multiply by the transpose of the rotation part
	mat33.multiplyByTranspose(src, dst);
}

// point = matrix33 * point
NX_INLINE void RightTransformPoint(const NxVec3& src, NxVec3& dst, const NxMat33& mat33)
{
	// We have to multiply by the rotation part
	mat33.multiply(src, dst);
}

// point *= matrix44
NX_INLINE void TransformPoint(const NxVec3& src, NxVec3& dst, const NxMat34& mat)
{
	// We have to multiply by the transpose of the rotation part, then add the translation
	LeftTransformPoint(src, dst, mat.M);
	dst += mat.t;
}

/**
Tests if a point is contained within the box
\param		p	[in] the world point to test
\return		true if inside the box
 */
bool NxBoxContainsPoint(const NxBox& box, const NxVec3& p)
{
	// Point in OBB test using lazy evaluation and early exits

	// Translate to box space
	NxVec3 RelPoint = p - box.center;

	// Point * mRot maps from box space to world space
	// mRot * Point maps from world space to box space (what we need here)

	NxF32 f = box.rot(0, 0) * RelPoint.x + box.rot(1, 0) * RelPoint.y + box.rot(2, 0) * RelPoint.z;
	if(f >= box.extents.x || f <= -box.extents.x) return false;

	f = box.rot(0, 1) * RelPoint.x + box.rot(1, 1) * RelPoint.y + box.rot(2, 1) * RelPoint.z;
	if(f >= box.extents.y || f <= -box.extents.y) return false;

	f = box.rot(0, 2) * RelPoint.x + box.rot(1, 2) * RelPoint.y + box.rot(2, 2) * RelPoint.z;
	if(f >= box.extents.z || f <= -box.extents.z) return false;
	return true;
}

/**
Builds a box from AABB and a world transform.
\param		aabb	[in] the aabb
\param		mat		[in] the world transform
 */
void NxCreateBox(NxBox& box, const NxBounds3& aabb, const NxMat34& mat)
{
	// Note: must be coherent with Rotate()

	aabb.getCenter(box.center);
	aabb.getExtents(box.extents);
	// Here we have the same as OBB::Rotate(mat) where the obb is (mCenter, mExtents, Identity).

	// So following what's done in Rotate:
	// - x-form the center
//	box.center *= mat;
	NxVec3 dst;	TransformPoint(box.center, dst, mat);	box.center = dst;
	// - combine rotation with identity, i.e. just use given matrix
	box.rot = mat.M;
}

/**
Computes the obb planes.
\param		planes	[out] 6 box planes
\return		true if success
 */
bool NxComputeBoxPlanes(const NxBox& box, NxPlane* planes)
{
	// Checkings
	if(!planes)	return false;

//	NxVec3 Axis0; box.rot.getRow(0, Axis0);
//	NxVec3 Axis1; box.rot.getRow(1, Axis1);
//	NxVec3 Axis2; box.rot.getRow(2, Axis2);
	NxVec3 Axis0; box.rot.getColumn(0, Axis0);
	NxVec3 Axis1; box.rot.getColumn(1, Axis1);
	NxVec3 Axis2; box.rot.getColumn(2, Axis2);

	// Writes normals
	planes[0].normal = Axis0;
	planes[1].normal = -Axis0;
	planes[2].normal = Axis1;
	planes[3].normal = -Axis1;
	planes[4].normal = Axis2;
	planes[5].normal = -Axis2;

	// "Rotated extents"
	Axis0 *= box.extents.x;
	Axis1 *= box.extents.y;
	Axis2 *= box.extents.z;

	// Compute a point on each plane
	NxVec3 p0 = box.center + Axis0;
	NxVec3 p1 = box.center - Axis0;
	NxVec3 p2 = box.center + Axis1;
	NxVec3 p3 = box.center - Axis1;
	NxVec3 p4 = box.center + Axis2;
	NxVec3 p5 = box.center - Axis2;

	// Compute d
	planes[0].d = -planes[0].normal.dot(p0);
	planes[1].d = -planes[1].normal.dot(p1);
	planes[2].d = -planes[2].normal.dot(p2);
	planes[3].d = -planes[3].normal.dot(p3);
	planes[4].d = -planes[4].normal.dot(p4);
	planes[5].d = -planes[5].normal.dot(p5);

	return true;
}

/**
Computes the obb points.
\param		pts	[out] 8 box points
\return		true if success
 */
bool NxComputeBoxPoints(const NxBox& box, NxVec3* pts)
{
	// Checkings
	if(!pts)	return false;

//	NxVec3 Axis0; box.rot.getRow(0, Axis0);
//	NxVec3 Axis1; box.rot.getRow(1, Axis1);
//	NxVec3 Axis2; box.rot.getRow(2, Axis2);
	NxVec3 Axis0; box.rot.getColumn(0, Axis0);
	NxVec3 Axis1; box.rot.getColumn(1, Axis1);
	NxVec3 Axis2; box.rot.getColumn(2, Axis2);

	// "Rotated extents"
	Axis0 *= box.extents.x;
	Axis1 *= box.extents.y;
	Axis2 *= box.extents.z;

	//     7+------+6			0 = ---
	//     /|     /|			1 = +--
	//    / |    / |			2 = ++-
	//   / 4+---/--+5			3 = -+-
	// 3+------+2 /    y   z	4 = --+
	//  | /    | /     |  /		5 = +-+
	//  |/     |/      |/		6 = +++
	// 0+------+1      *---x	7 = -++

	// Original code: 24 vector ops
/*	pts[0] = box.center - Axis0 - Axis1 - Axis2;
	pts[1] = box.center + Axis0 - Axis1 - Axis2;
	pts[2] = box.center + Axis0 + Axis1 - Axis2;
	pts[3] = box.center - Axis0 + Axis1 - Axis2;
	pts[4] = box.center - Axis0 - Axis1 + Axis2;
	pts[5] = box.center + Axis0 - Axis1 + Axis2;
	pts[6] = box.center + Axis0 + Axis1 + Axis2;
	pts[7] = box.center - Axis0 + Axis1 + Axis2;*/

	// Rewritten: 12 vector ops
	pts[0] = pts[3] = pts[4] = pts[7] = box.center - Axis0;
	pts[1] = pts[2] = pts[5] = pts[6] = box.center + Axis0;

	NxVec3 Tmp = Axis1 + Axis2;
	pts[0] -= Tmp;
	pts[1] -= Tmp;
	pts[6] += Tmp;
	pts[7] += Tmp;

	Tmp = Axis1 - Axis2;
	pts[2] += Tmp;
	pts[3] += Tmp;
	pts[4] -= Tmp;
	pts[5] -= Tmp;

	return true;
}

/**
Computes vertex normals.
\param		pts	[out] 8 box points
\return		true if success
 */
bool NxComputeBoxVertexNormals(const NxBox& box, NxVec3* pts)
{
	static NxF32 VertexNormals[] = 
	{
		-INVSQRT3,	-INVSQRT3,	-INVSQRT3,
		INVSQRT3,	-INVSQRT3,	-INVSQRT3,
		INVSQRT3,	INVSQRT3,	-INVSQRT3,
		-INVSQRT3,	INVSQRT3,	-INVSQRT3,
		-INVSQRT3,	-INVSQRT3,	INVSQRT3,
		INVSQRT3,	-INVSQRT3,	INVSQRT3,
		INVSQRT3,	INVSQRT3,	INVSQRT3,
		-INVSQRT3,	INVSQRT3,	INVSQRT3
	};

	if(!pts)	return false;

	const NxVec3* VN = (const NxVec3*)VertexNormals;
	for(unsigned i=0;i<8;i++)
	{
//		pts[i] = VN[i] * box.rot;
		RightTransformPoint(VN[i], pts[i], box.rot);
	}

	return true;
}

/**
Returns edges.
\return		24 indices (12 edges) indexing the list returned by ComputePoints()
 */
const NxU32* NxGetBoxEdges()
{
	//     7+------+6			0 = ---
	//     /|     /|			1 = +--
	//    / |    / |			2 = ++-
	//   / 4+---/--+5			3 = -+-
	// 3+------+2 /    y   z	4 = --+
	//  | /    | /     |  /		5 = +-+
	//  |/     |/      |/		6 = +++
	// 0+------+1      *---x	7 = -++

	static NxU32 Indices[] = {
	0, 1,	1, 2,	2, 3,	3, 0,
	7, 6,	6, 5,	5, 4,	4, 7,
	1, 5,	6, 2,
	3, 7,	4, 0
	};
	return Indices;
}

const NxI32* NxGetBoxEdgesAxes()
{
	//     7+------+6			0 = ---
	//     /|     /|			1 = +--
	//    / |    / |			2 = ++-
	//   / 4+---/--+5			3 = -+-
	// 3+------+2 /    y   z	4 = --+
	//  | /    | /     |  /		5 = +-+
	//  |/     |/      |/		6 = +++
	// 0+------+1      *---x	7 = -++

	static NxI32 Indices[] = {
	1,	2,	-1,	-2,
	1,	-2,	-1,	2,
	3,	-3,	3,	-3
	};
	return Indices;
}

/**
 *	Returns triangles.
 *	\return		36 indices (12 triangles) indexing the list returned by ComputePoints()
 */
const NxU32* NxGetBoxTriangles()
{
	static NxU32 Indices[] = {
	0,2,1,	0,3,2,
	1,6,5,	1,2,6,
	5,7,4,	5,6,7,
	4,3,0,	4,7,3,
	3,6,2,	3,7,6,
	5,0,1,	5,4,0
	};
	return Indices;
}


/**
Returns local edge normals.
\return		edge normals in local space
 */
const NxVec3* NxGetBoxLocalEdgeNormals()
{
	static NxF32 EdgeNormals[] = 
	{
		0,			-INVSQRT2,	-INVSQRT2,	// 0-1
		INVSQRT2,	0,			-INVSQRT2,	// 1-2
		0,			INVSQRT2,	-INVSQRT2,	// 2-3
		-INVSQRT2,	0,			-INVSQRT2,	// 3-0

		0,			INVSQRT2,	INVSQRT2,	// 7-6
		INVSQRT2,	0,			INVSQRT2,	// 6-5
		0,			-INVSQRT2,	INVSQRT2,	// 5-4
		-INVSQRT2,	0,			INVSQRT2,	// 4-7

		INVSQRT2,	-INVSQRT2,	0,			// 1-5
		INVSQRT2,	INVSQRT2,	0,			// 6-2
		-INVSQRT2,	INVSQRT2,	0,			// 3-7
		-INVSQRT2,	-INVSQRT2,	0			// 4-0
	};
	return (const NxVec3*)EdgeNormals;
}

/**
Returns world edge normal
\param		edge_index		[in] 0 <= edge index < 12
\param		world_normal	[out] edge normal in world space
 */
void NxComputeBoxWorldEdgeNormal(const NxBox& box, NxU32 edge_index, NxVec3& world_normal)
{
	NX_ASSERT(edge_index<12);
//	world_normal = GetLocalEdgeNormals()[edge_index] * mRot;
	LeftTransformPoint(NxGetBoxLocalEdgeNormals()[edge_index], world_normal, box.rot);
}


NX_INLINE NxU32 LargestAxis(const NxVec3& p)
{
	const float* Vals = &p.x;
	NxU32 m = 0;
	if(Vals[1] > Vals[m]) m = 1;
	if(Vals[2] > Vals[m]) m = 2;
	return m;
}

/**
Computes an LSS surrounding the OBB.
\param		lss		[out] the LSS
 */
void NxComputeCapsuleAroundBox(const NxBox& box, NxCapsule& capsule)
{
//	NxVec3 Axis0; box.rot.getRow(0, Axis0);
//	NxVec3 Axis1; box.rot.getRow(1, Axis1);
//	NxVec3 Axis2; box.rot.getRow(2, Axis2);
	NxVec3 Axis0; box.rot.getColumn(0, Axis0);
	NxVec3 Axis1; box.rot.getColumn(1, Axis1);
	NxVec3 Axis2; box.rot.getColumn(2, Axis2);

	switch(LargestAxis(box.extents))
	{
		case 0:
			capsule.radius = (box.extents.y + box.extents.z)*0.5f;
			capsule.p0 = box.center + Axis0 * (box.extents.x - capsule.radius);
			capsule.p1 = box.center - Axis0 * (box.extents.x - capsule.radius);
			break;
		case 1:
			capsule.radius = (box.extents.x + box.extents.z)*0.5f;
			capsule.p0 = box.center + Axis1 * (box.extents.y - capsule.radius);
			capsule.p1 = box.center - Axis1 * (box.extents.y - capsule.radius);
			break;
		case 2:
			capsule.radius = (box.extents.x + box.extents.y)*0.5f;
			capsule.p0 = box.center + Axis2 * (box.extents.z - capsule.radius);
			capsule.p1 = box.center - Axis2 * (box.extents.z - capsule.radius);
			break;
	}
}

/**
Inverts a PR matrix. (which only contains a rotation and a translation)
This is faster and less subject to FPU errors than the generic inversion code.

\relates	NxMat34
\fn			InvertPRMatrix(NxMat34& dest, const NxMat34& src)
\param		dest	[out] destination matrix
\param		src		[in] source matrix
 */
void InvertPRMatrix(NxMat34& dest, const NxMat34& src)
{
	dest.M.setTransposed(src.M);

	dest.t.x = -(src.t.x*src.M(0,0) + src.t.y*src.M(0,1) + src.t.z*src.M(0,2));
	dest.t.y = -(src.t.x*src.M(1,0) + src.t.y*src.M(1,1) + src.t.z*src.M(1,2));
	dest.t.z = -(src.t.x*src.M(2,0) + src.t.y*src.M(2,1) + src.t.z*src.M(2,2));
}

/**
Checks the OBB is inside another OBB.
\param		box		[in] the other OBB
\return		TRUE if we're inside the other box
 */
bool NxIsBoxAInsideBoxB(const NxBox& a, const NxBox& b)
{
	// Make a 4x4 from the box & inverse it
	NxMat34 M0Inv;
	InvertPRMatrix(M0Inv, NxMat34(b.rot, b.center));

	// With our inversed 4x4, create box1 in space of box0
	NxBox _1in0;
	a.rotate(M0Inv, _1in0);

	// This should cancel out box0's rotation, i.e. it's now an AABB.
	// => Center(0,0,0), Rot(identity)

	// The two boxes are in the same space so now we can compare them.

	// Create the AABB of (box1 in space of box0)
	const NxMat33& mtx = _1in0.rot;

	// Then compare AABBs

	float f = fabsf(mtx(0,0) * a.extents.x) + fabsf(mtx(1,0) * a.extents.y) + fabsf(mtx(2,0) * a.extents.z) - b.extents.x;
	if(f > _1in0.center.x)	return false;
	if(-f < _1in0.center.x)	return false;

	f = fabsf(mtx(0,1) * a.extents.x) + fabsf(mtx(1,1) * a.extents.y) + fabsf(mtx(2,1) * a.extents.z) - b.extents.y;
	if(f > _1in0.center.y)	return false;
	if(-f < _1in0.center.y)	return false;

	f = fabsf(mtx(0,2) * a.extents.x) + fabsf(mtx(1,2) * a.extents.y) + fabsf(mtx(2,2) * a.extents.z) - b.extents.z;
	if(f > _1in0.center.z)	return false;
	if(-f < _1in0.center.z)	return false;

	return true;
}

const NxU32* NxGetBoxQuads()
{
	//     7+------+6			0 = ---
	//     /|     /|			1 = +--
	//    / |    / |			2 = ++-
	//   / 4+---/--+5			3 = -+-
	// 3+------+2 /    y   z	4 = --+
	//  | /    | /     |  /		5 = +-+
	//  |/     |/      |/		6 = +++
	// 0+------+1      *---x	7 = -++

	static NxU32 QuadIndices[] = {
		// +
		1,2,6,5,	// Quad 0
		2,3,7,6,	// Quad 1
		4,5,6,7,	// Quad 2
		// -
		0,4,7,3,	// Quad 3
		0,1,5,4,	// Quad 4
		0,3,2,1		// Quad 5
	};
	return QuadIndices;
}

const NxU32* NxBoxVertexToQuad(NxU32 vertexIndex)
{
	static NxU32 QuadIndices[] = {
		3,4,5,	// Vertex 0
		0,4,5,	// Vertex 1
		0,1,5,	// Vertex 2
		1,3,5,	// Vertex 3
		2,3,4,	// Vertex 4
		0,2,4,	// Vertex 5
		0,1,2,	// Vertex 6
		1,2,3	// Vertex 7
	};
	return &QuadIndices[vertexIndex*3];
}
