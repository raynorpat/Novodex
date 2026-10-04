/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "glm.h"

#include "MeshWriteAdapter.h"

MeshWriteAdapter::MeshWriteAdapter(GLMmodel & gm) : glmModel(gm)
	{
	//we will make the arrays dynamic, so store the allocated mem sizes:
//	nVerticesAllocated = glmModel.numvertices + 1;
//	nNormals
	}

MeshWriteAdapter::~MeshWriteAdapter()
	{
	}

void MeshWriteAdapter::startChanges()
	{
	//nothing
	}

void MeshWriteAdapter::endChanges()
	{
	//nothing
	}

void MeshWriteAdapter::clear()
	{
	//nothing


	}

void MeshWriteAdapter::purge()
	{
	//we don't need this to be implemented
	NX_ASSERT(0);
	}

void MeshWriteAdapter::hintReserve(NxU32 maxVertices, NxU32 maxTriangles)
	{
	}

NxU32 MeshWriteAdapter::appendVertex(const Point & point)
	{
	return 0;
	}

NxU32 MeshWriteAdapter::appendTriangle(const Triangle & triangle)
	{
	return 0;
	}

void MeshWriteAdapter::appendVertices(NxU32 numVertices, const Point * vertices)
	{
	}

void MeshWriteAdapter::appendTriangles(NxU32 numTriangles, const Triangle * triangles)
	{
	}

void MeshWriteAdapter::setPoint(NxU32 vertexIndex, const Point & newPosition)
	{
	}

void MeshWriteAdapter::setNormal(NxU32 vertexIndex, const Normal & newNormal)
	{
	}

void MeshWriteAdapter::setTexCoord(NxU32 vertexIndex, const TexCoord & newTC)
	{
	}

void MeshWriteAdapter::setTriangle(NxU32 trigIndex, const Triangle & trig)
	{
	}








//stuff copied from MeshReadAdapter:

NxU32 MeshWriteAdapter::getNumVertices() const
	{
	return glmModel.numvertices;
	}

NxU32 MeshWriteAdapter::getNumTriangles() const
	{
	return glmModel.numtriangles;
	}

NxU32 MeshWriteAdapter::getVertexStride() const
	{
	return sizeof(float) * 3;
	}

NxU32 MeshWriteAdapter::getTriangleStride() const
	{
	return sizeof(GLMtriangle);
	}

const NxUserTriangleMesh::Point * MeshWriteAdapter::getPoints() const
	{
	return (Point *)glmModel.vertices;	//note: 0th one isn't ever indexed.
	}

const NxUserTriangleMesh::Normal * MeshWriteAdapter::getNormals() const
	{
	//return (Normal *)glmModel.normals;	//note: 0th one isn't ever indexed.
	return 0;	//can't provide this because the model doesn't store exactly the same number of normals as vertices.
	}

const NxUserTriangleMesh::TexCoord * MeshWriteAdapter::getTexCoords() const
	{
	return 0;	//we can't provide this through the current interface because it has a different stride.
	}

const NxUserTriangleMesh::Triangle * MeshWriteAdapter::getTriangles() const
	{
	return (Triangle *)glmModel.triangles[0].vindices;
	}

NxF32 MeshWriteAdapter::getTriangleNormalSign() const
	{
	return 1.0f;
	}
