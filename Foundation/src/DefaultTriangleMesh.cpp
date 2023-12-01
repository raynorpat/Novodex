#error obsolete!  Adam: Remove this file from the Visual Studio .net project too please!

/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "Nxf.h"
#include "FoundationSDK.h"
#include "DefaultTriangleMesh.h"
#include "CustomAssert.h"

namespace NxFoundation
	{

DefaultTriangleMesh::DefaultTriangleMesh()
	{
	numVertices = 0;
	numVertsAllocated = 0;

	numTriangles = 0;
	numTrigsAllocated = 0;

	vertices = 0;
	triangles = 0;
	lockedForChanges = false;
	}

DefaultTriangleMesh::~DefaultTriangleMesh()
	{
	lockedForChanges = true;
	purge();
	}

NxU32 DefaultTriangleMesh::getNumVertices() const
	{
	return numVertices;
	}

NxU32 DefaultTriangleMesh::getNumTriangles() const
	{
	return numTriangles;	
	}

NxU32 DefaultTriangleMesh::getVertexStride() const
	{
	return sizeof(GraphicsVertex);
	}

NxU32 DefaultTriangleMesh::getTriangleStride() const
	{	
	return sizeof(Triangle);
	}

const NxUserTriangleMesh::Point * DefaultTriangleMesh::getPoints() const
	{
	if (vertices)
		return &(vertices[0].pos);
	else
		return 0;
	}

const NxUserTriangleMesh::Normal * DefaultTriangleMesh::getNormals() const
	{
	if (vertices)
		return &(vertices[0].normal);
	else
		return 0;
	}


const NxUserTriangleMesh::TexCoord * DefaultTriangleMesh::getTexCoords() const
	{
	if (vertices)
		return &(vertices[0].t);
	else
		return 0;
	}


const NxUserTriangleMesh::Triangle * DefaultTriangleMesh::getTriangles() const
	{
	return triangles;
	}

NxF32 DefaultTriangleMesh::getTriangleNormalSign() const
	{
	return 1.0f;
	}

void DefaultTriangleMesh::startChanges()
	{
	lockedForChanges = true;
	}

void DefaultTriangleMesh::endChanges()
	{
	lockedForChanges = false;
	//TODO: recompute normals and texcoords, update AGP/VRAM copy.
	}


void DefaultTriangleMesh::clear()
	{
	NX_ASSERT(lockedForChanges);

	numVertices = 0;
	numTriangles = 0;
	}

void DefaultTriangleMesh::purge()
	{
	NX_ASSERT(lockedForChanges);

	if (vertices)
		{
		nxFoundationSDKAllocator->free(vertices);
		vertices = 0;
		}
	if (triangles)
		{
		nxFoundationSDKAllocator->free(triangles);
		triangles = 0;
		}	

	numVertices = 0;
	numVertsAllocated = 0;
	numTriangles = 0;
	numTrigsAllocated = 0;
	}


void DefaultTriangleMesh::hintReserve(NxU32 maxVertices, NxU32 maxTriangles)
	{
	if (numVertsAllocated)
		{
		if (numVertsAllocated < maxVertices)
			{
			numVertsAllocated = maxVertices;
			vertices = (GraphicsVertex *)nxFoundationSDKAllocator->realloc(vertices, maxVertices * sizeof(GraphicsVertex));
			}
		}
	else
		{
		NX_ASSERT(!vertices);
		numVertsAllocated = maxVertices;
		vertices = (GraphicsVertex *)nxFoundationSDKAllocator->malloc(maxVertices * sizeof(GraphicsVertex));
		}

	if (numTrigsAllocated)
		{
		if (numTrigsAllocated < maxTriangles)
			{
			numTrigsAllocated = maxTriangles;
			triangles = (Triangle *)nxFoundationSDKAllocator->realloc(triangles, maxTriangles * sizeof(Triangle));
			}
		}
	else
		{
		NX_ASSERT(!triangles);
		numTrigsAllocated = maxTriangles;
		triangles = (Triangle *)nxFoundationSDKAllocator->malloc(maxTriangles * sizeof(Triangle));
		}
	}

NxU32 DefaultTriangleMesh::appendVertex(const NxUserTriangleMesh::Point & vertex)
	{
	NX_ASSERT(lockedForChanges);
	NxU32 vertexIndex = numVertices;

	if (numVertsAllocated <= numVertices)
		hintReserve(2*numVertsAllocated, numTrigsAllocated);

	NX_ASSERT(numVertsAllocated > numVertices);
	++numVertices;

	vertices[vertexIndex].pos = vertex;

	return vertexIndex;
	}

NxU32 DefaultTriangleMesh::appendTriangle(const NxUserTriangleMesh::Triangle & triangle)
	{
	NX_ASSERT(lockedForChanges);


	NxU32 trigIndex = numTriangles;

	if (numTrigsAllocated <= numTriangles)
		hintReserve(numVertsAllocated, 2*numTrigsAllocated);

	NX_ASSERT(numTrigsAllocated > numTriangles);
	++numTriangles;

	triangles[trigIndex] = triangle;

	return trigIndex;
	}

void DefaultTriangleMesh::appendVertices(NxU32 num, const NxUserTriangleMesh::Point * verts)
	{
	NX_ASSERT(lockedForChanges);
	NX_ASSERT(num > 0);

	NxU32 vertexIndex = numVertices;

	if (numVertsAllocated <= numVertices + num - 1)
		hintReserve(2*numVertsAllocated + num, numTrigsAllocated);

	NX_ASSERT(numVertsAllocated > numVertices + num - 1);
	numVertices += num;

	for (NxU32 i=0; i<num; i++)
		vertices[vertexIndex + i].pos = verts[i];
	}

void DefaultTriangleMesh::appendTriangles(NxU32 num, const NxUserTriangleMesh::Triangle * trigs)
	{
	NX_ASSERT(lockedForChanges);
	NX_ASSERT(num > 0);

	NxU32 trigIndex = numTriangles;

	if (numTrigsAllocated <= numTriangles + num - 1)
		hintReserve(numVertsAllocated, 2*numTrigsAllocated + num);

	NX_ASSERT(numTrigsAllocated > numTriangles + num - 1);
	numTriangles += num;

	for (NxU32 i=0; i<num; i++)
		triangles[trigIndex + i] = trigs[i];
	}

void DefaultTriangleMesh::setPoint(NxU32 vertexIndex, const NxUserTriangleMesh::Point & newPosition)
	{
	NX_ASSERT(lockedForChanges);
	NX_ASSERT(vertexIndex < numVertices);
	vertices[vertexIndex].pos = newPosition;
	}

void DefaultTriangleMesh::setNormal(NxU32 vertexIndex, const NxUserTriangleMesh::Normal & newNormal)
	{
	NX_ASSERT(lockedForChanges);
	NX_ASSERT(vertexIndex < numVertices);
	vertices[vertexIndex].normal = newNormal;
	}


void DefaultTriangleMesh::setTexCoord(NxU32 vertexIndex, const NxUserTriangleMesh::TexCoord & newTC)
	{
	NX_ASSERT(lockedForChanges);
	NX_ASSERT(vertexIndex < numVertices);
	vertices[vertexIndex].t = newTC;
	}


void DefaultTriangleMesh::setTriangle(NxU32 trigIndex, const NxUserTriangleMesh::Triangle & trig)
	{
	NX_ASSERT(lockedForChanges);
	NX_ASSERT(trigIndex < numTriangles);
	triangles[trigIndex] = trig;
	}

}