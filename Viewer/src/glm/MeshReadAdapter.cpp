/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "glm.h"

#include "MeshReadAdapter.h"

MeshReadAdapter::MeshReadAdapter(GLMmodel & gm) : glmModel(gm)
	{
	}

MeshReadAdapter::~MeshReadAdapter()
	{
	}

NxU32 MeshReadAdapter::getNumVertices() const
	{
	return glmModel.numvertices;
	}

NxU32 MeshReadAdapter::getNumTriangles() const
	{
	return glmModel.numtriangles;
	}

NxU32 MeshReadAdapter::getVertexStride() const
	{
	return sizeof(float) * 3;
	}

NxU32 MeshReadAdapter::getTriangleStride() const
	{
	return sizeof(GLMtriangle);
	}

const NxUserTriangleMesh::Point * MeshReadAdapter::getPoints() const
	{
	return (Point *)glmModel.vertices;	//note: 0th one isn't ever indexed.
	}

const NxUserTriangleMesh::Normal * MeshReadAdapter::getNormals() const
	{
	//return (Normal *)glmModel.normals;	//note: 0th one isn't ever indexed.
	return 0;	//can't provide this because the model doesn't store exactly the same number of normals as vertices.
	}

const NxUserTriangleMesh::TexCoord * MeshReadAdapter::getTexCoords() const
	{
	return 0;	//we can't provide this through the current interface because it has a different stride.
	}

const NxUserTriangleMesh::Triangle * MeshReadAdapter::getTriangles() const
	{
	return (Triangle *)glmModel.triangles[0].vindices;
	}

NxF32 MeshReadAdapter::getTriangleNormalSign() const
	{
	return 1.0f;
	}
