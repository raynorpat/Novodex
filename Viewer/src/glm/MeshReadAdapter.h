/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#ifndef MESHREADADAPTER_H
#define MESHREADADAPTER_H

#include "NxUserTriangleMesh.h"

struct GLMmodel;
/**
This converts GLM models the novodex mesh read interface.
*/

class MeshReadAdapter : public NxUserTriangleMesh				
	{
	public:
	MeshReadAdapter(GLMmodel & glmModel);
	~MeshReadAdapter();

	NxU32 getNumVertices() const;
	NxU32 getNumTriangles() const;
	NxU32 getVertexStride() const;
	NxU32 getTriangleStride() const;
	const Point * getPoints() const;
	const Normal * getNormals() const;
	const TexCoord * getTexCoords() const;
	const Triangle * getTriangles() const;
	NxF32 getTriangleNormalSign() const;

	protected:
	GLMmodel & glmModel;
	};

#endif