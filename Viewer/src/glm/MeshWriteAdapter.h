/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#ifndef MESHWRITEADAPTER_H
#define MESHWRITEADAPTER_H

#include "MeshReadAdapter.h"
#include "NxUserDynamicMesh.h"

/**
This converts GLM models the novodex mesh read interface.
*/

class MeshWriteAdapter : public NxUserDynamicMesh			
	{
	public:
	MeshWriteAdapter(GLMmodel & glmModel);
	~MeshWriteAdapter();

	//implementing NxUserTriangleMesh:
	NxU32 getNumVertices() const;
	NxU32 getNumTriangles() const;
	NxU32 getVertexStride() const;
	NxU32 getTriangleStride() const;
	const Point * getPoints() const;
	const Normal * getNormals() const;
	const TexCoord * getTexCoords() const;
	const Triangle * getTriangles() const;
	NxF32 getTriangleNormalSign() const;

	//implementing NxUserDynamicMesh: 
	virtual void startChanges();
	virtual void endChanges();
	virtual void clear();
	virtual void purge();
	virtual void hintReserve(NxU32 maxVertices, NxU32 maxTriangles);
	virtual NxU32 appendVertex(const Point & point);
	virtual NxU32 appendTriangle(const Triangle & triangle);
	virtual void appendVertices(NxU32 numVertices, const Point * vertices);
	virtual void appendTriangles(NxU32 numTriangles, const Triangle * triangles);
	virtual void setPoint(NxU32 vertexIndex, const Point & newPosition);
	virtual void setNormal(NxU32 vertexIndex, const Normal & newNormal);
	virtual void setTexCoord(NxU32 vertexIndex, const TexCoord & newTC);
	virtual void setTriangle(NxU32 trigIndex, const Triangle & trig);

	protected:
	GLMmodel & glmModel;
	};

#endif