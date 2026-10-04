/*----------------------------------------------------------------------------*\
|
|                                 NovodeX Technology
|
|                                     www.novodex.com
|
\*----------------------------------------------------------------------------*/

// Plane/triangle-mesh entries 001893 and 001895. The oracle first asks OPCODE
// for triangles intersecting the transformed plane. The candidate walks the
// internal triangle list directly; for each intersecting triangle it emits
// each unique vertex on the negative side. This preserves the recovered
// contact construction while the OPCODE query-context adapter is completed.

#include "ContactGeneration.h"
#include "TriangleMesh.h"

#include <float.h>
#include <stdlib.h>
#include <string.h>

namespace
	{
	static TriangleMesh* nxPlaneMeshData(const NxCollisionShape* shape)
		{
		return reinterpret_cast<TriangleMesh*>(
			*reinterpret_cast<void* const*>(shape->geometry));
		}

	static NxVec3 nxPlaneMeshWorldVertex(const NxCollisionShape* shape, const NxVec3& point)
		{
		const NxReal* r = shape->rotation;
		const NxReal* t = shape->translation;
		NxVec3 world;
		world.x = static_cast<NxReal>(((double) point.x * r[0] +
			(double) point.z * r[2]) + (double) point.y * r[1] + t[0]);
		world.y = static_cast<NxReal>(((double) point.x * r[3] +
			(double) point.z * r[5]) + (double) point.y * r[4] + t[1]);
		world.z = static_cast<NxReal>(((double) point.x * r[6] +
			(double) point.z * r[8]) + (double) point.y * r[7] + t[2]);
		return world;
		}

	static NxReal nxPlaneMeshDistance(const NxCollisionShape* plane, const NxVec3& point)
		{
		const NxReal* n = plane->geometry;
		const double distance = (double) point.x * n[0] +
			(double) point.y * n[1] + (double) point.z * n[2] + plane->geometry[3];
		return static_cast<NxReal>(distance);
		}

	static NxU32 nxPlaneMeshBits(NxReal value)
		{
		NxU32 bits;
		memcpy(&bits, &value, sizeof(bits));
		return bits;
		}

	}

bool __cdecl NxOverlapPlaneMesh(const NxCollisionShape* plane, const NxCollisionShape* shape,
	void*)
	{
	TriangleMesh* mesh = nxPlaneMeshData(shape);
	if(!mesh || !mesh->mInternal.mVertices || !mesh->mInternal.mTriangles)
		return false;
	const NxTriangle32* triangles = static_cast<const NxTriangle32*>(mesh->mInternal.mTriangles);
	const NxVec3* vertices = static_cast<const NxVec3*>(mesh->mInternal.mVertices);
	for(NxU32 i = 0; i < mesh->mInternal.mTriangleCount; ++i)
		{
		const NxU32* indices = triangles[i].v;
		if(indices[0] >= mesh->mInternal.mVertexCount ||
			indices[1] >= mesh->mInternal.mVertexCount ||
			indices[2] >= mesh->mInternal.mVertexCount)
			continue;
		NxReal minimum = FLT_MAX;
		NxReal maximum = -FLT_MAX;
		for(unsigned corner = 0; corner < 3; ++corner)
			{
			const NxVec3 point = nxPlaneMeshWorldVertex(shape, vertices[indices[corner]]);
			const NxReal distance = nxPlaneMeshDistance(plane, point);
			if(distance < minimum) minimum = distance;
			if(distance > maximum) maximum = distance;
			}
		if(minimum <= 0.0f && maximum >= 0.0f)
			return true;
		}
	return false;
	}

void __cdecl NxContactPlaneMesh(const NxCollisionShape* plane, const NxCollisionShape* shape,
	NxContactSink* sink, void*)
	{
	TriangleMesh* mesh = nxPlaneMeshData(shape);
	if(!mesh || !mesh->mInternal.mVertices || !mesh->mInternal.mTriangles ||
		!mesh->mInternal.mTriangleCount || !mesh->mInternal.mVertexCount)
		return;
	const NxTriangle32* triangles = static_cast<const NxTriangle32*>(mesh->mInternal.mTriangles);
	const NxVec3* vertices = static_cast<const NxVec3*>(mesh->mInternal.mVertices);
	NxU8* emitted = static_cast<NxU8*>(malloc(mesh->mInternal.mVertexCount));
	if(!emitted)
		return;
	memset(emitted, 0, mesh->mInternal.mVertexCount);
	const NxU16 planeMaterial = *reinterpret_cast<const NxU16*>(
		reinterpret_cast<const NxU8*>(plane) + 0xda);

	for(NxU32 i = 0; i < mesh->mInternal.mTriangleCount; ++i)
		{
		const NxU32* indices = triangles[i].v;
		if(indices[0] >= mesh->mInternal.mVertexCount ||
			indices[1] >= mesh->mInternal.mVertexCount ||
			indices[2] >= mesh->mInternal.mVertexCount)
			continue;
		NxVec3 world[3];
		NxReal distances[3];
		for(unsigned corner = 0; corner < 3; ++corner)
			{
			world[corner] = nxPlaneMeshWorldVertex(shape, vertices[indices[corner]]);
			distances[corner] = nxPlaneMeshDistance(plane, world[corner]);
			}
		const NxReal minimum = distances[0] < distances[1]
			? (distances[0] < distances[2] ? distances[0] : distances[2])
			: (distances[1] < distances[2] ? distances[1] : distances[2]);
		const NxReal maximum = distances[0] > distances[1]
			? (distances[0] > distances[2] ? distances[0] : distances[2])
			: (distances[1] > distances[2] ? distances[1] : distances[2]);
		if(minimum > 0.0f || maximum < 0.0f)
			continue;
		const NxU16 meshMaterial = mesh->mInternal.mMaterialIndices
			? mesh->mInternal.mMaterialIndices[i] : 0xffffu;
		for(unsigned corner = 0; corner < 3; ++corner)
			{
			const NxU32 vertexIndex = indices[corner];
			if(emitted[vertexIndex])
				continue;
			emitted[vertexIndex] = 1;
			if(distances[corner] < 0.0f)
				NxEmitContact(sink, shape->collisionObject, plane->collisionObject,
					nxPlaneMeshBits(distances[corner]), &world[corner],
					reinterpret_cast<const NxVec3*>(plane->geometry), meshMaterial,
					planeMaterial);
			}
		}
	free(emitted);
	}
