#error obsolete!  

#ifndef NX_FOUNDATION_DEFAULTTRIANGLEMESH
#define NX_FOUNDATION_DEFAULTTRIANGLEMESH
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "NxUserDynamicMesh.h"
#include "NxAllocateable.h"

/**
Default implementation of NxUserTriangleMesh class.
*/
namespace NxFoundation
	{
class DefaultTriangleMesh : public NxAllocateable, public NxUserDynamicMesh
	{
	public:
	DefaultTriangleMesh();
	~DefaultTriangleMesh();

	//!methods of NxUserTriangleMesh:
	virtual NxU32 getNumVertices() const;

	virtual NxU32 getNumTriangles() const;

	virtual NxU32 getVertexStride() const;

	virtual NxU32 getTriangleStride() const;

	virtual const Point * getPoints() const;

	virtual const Normal * getNormals() const;

	virtual const TexCoord * getTexCoords() const;

	virtual const Triangle * getTriangles() const;

	virtual NxF32 getTriangleNormalSign() const;

	//!methods of NxUserDynamicMesh:

	virtual void startChanges();

	virtual void endChanges();

	virtual void clear();

	virtual void purge();

	virtual void hintReserve(NxU32 maxVertices, NxU32 maxTriangles);
	
	virtual NxU32 appendVertex(const Point & vertex);

	virtual NxU32 appendTriangle(const Triangle & triangle);

	virtual void appendVertices(NxU32 numVertices, const Point * vertices);

	virtual void appendTriangles(NxU32 numTriangles, const Triangle * triangles);

	virtual void setPoint(NxU32 vertexIndex, const Point & newPosition);

	virtual void setNormal(NxU32 vertexIndex, const Normal & newNormal);

	virtual void setTexCoord(NxU32 vertexIndex, const TexCoord & newTC);

	virtual void setTriangle(NxU32 trigIndex, const Triangle & trig);

	protected:
	class GraphicsVertex
		{
		public:
		Point pos;
		Normal normal;
		TexCoord t;
		};

	NxU32 numVertices;
	NxU32 numVertsAllocated;

	NxU32 numTriangles;
	NxU32 numTrigsAllocated;

	GraphicsVertex * vertices;
	Triangle * triangles;
	bool lockedForChanges;
	};
	}
#endif
