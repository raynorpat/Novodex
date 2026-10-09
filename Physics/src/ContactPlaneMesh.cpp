/*----------------------------------------------------------------------------*\
|
|                                 NovodeX Technology
|
|                                     www.novodex.com
|
\*----------------------------------------------------------------------------*/

// Plane/triangle-mesh entries 001893 and 001895. Both query the mesh's OPCODE
// model with the scene's PlanesCollider/PlanesCache pair. Contact generation
// then walks OPCODE's touched-face order and stamps each vertex once per query.

#include "ContactGeneration.h"
#include "TriangleMesh.h"
#include "TriangleMeshPolygons.h"

#include <string.h>

namespace
	{
	static TriangleMesh* nxPlaneMeshData(const NxCollisionShape* shape)
		{
		return reinterpret_cast<TriangleMesh*>(
			*reinterpret_cast<void* const*>(shape->geometry));
		}

	static IceMaths::Matrix4x4 nxPlaneMeshWorldMatrix(const NxCollisionShape* shape)
		{
		const NxReal* const r = shape->rotation;
		const NxReal* const t = shape->translation;
		// 001893 and 001895 copy the shape's nine rotation floats into these
		// matrix slots in this order; Matrix4x4 stores the same sixteen words.
		return IceMaths::Matrix4x4(
			r[0], r[3], r[6], 0.0f,
			r[1], r[4], r[7], 0.0f,
			r[2], r[5], r[8], 0.0f,
			t[0], t[1], t[2], 1.0f);
		}

	static bool nxPlaneMeshQuery(const NxCollisionShape* plane,
		const NxCollisionShape* shape, void* context, bool forContact)
		{
		TriangleMesh* const mesh = nxPlaneMeshData(shape);
		if(!mesh || !mesh->mInternal.mModel)
			return false;

		NxU32& contextFlags = *reinterpret_cast<NxU32*>(
			static_cast<NxU8*>(context) + 0x64);
		if(forContact)
			contextFlags &= ~NxU32(1);
		else
			contextFlags |= 1;
		contextFlags &= ~NxU32(2);
		contextFlags &= ~NxU32(0x10);

		Opcode::PlanesCollider* const collider = reinterpret_cast<Opcode::PlanesCollider*>(
			static_cast<NxU8*>(context) + 0x60);
		Opcode::PlanesCache* const cache = reinterpret_cast<Opcode::PlanesCache*>(
			static_cast<NxU8*>(context) + 0xa8);
		const IceMaths::Plane* const queryPlane =
			reinterpret_cast<const IceMaths::Plane*>(plane->geometry);
		const IceMaths::Matrix4x4 world = nxPlaneMeshWorldMatrix(shape);
		return collider->Collide(*cache, queryPlane, 1,
			*static_cast<const Opcode::Model*>(mesh->mInternal.mModel), &world);
		}

	static NxVec3 nxPlaneMeshWorldVertex(const NxCollisionShape* shape, const NxVec3& point)
		{
		const NxReal* const r = shape->rotation;
		const NxReal* const t = shape->translation;
		NxVec3 world;
		// Keep the oracle's x87 addition order from 0x000488f0..0x00048954.
		world.x = static_cast<NxReal>(((double)r[1] * point.y +
			(double)r[2] * point.z) + (double)r[0] * point.x + t[0]);
		world.y = static_cast<NxReal>(((double)r[4] * point.y +
			(double)r[3] * point.x) + (double)r[5] * point.z + t[1]);
		world.z = static_cast<NxReal>(((double)r[7] * point.y +
			(double)r[6] * point.x) + (double)r[8] * point.z + t[2]);
		return world;
		}

	static NxReal nxPlaneMeshDistance(const NxCollisionShape* plane, const NxVec3& point)
		{
		const NxReal* const n = plane->geometry;
		const double distance = ((double)point.z * n[2] +
			(double)point.y * n[1]) + (double)point.x * n[0] + plane->geometry[3];
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
	void* context)
	{
	const bool querySucceeded = nxPlaneMeshQuery(plane, shape, context, false);
	const NxU32 contextFlags = *reinterpret_cast<const NxU32*>(
		static_cast<const NxU8*>(context) + 0x64);
	return querySucceeded && (contextFlags & 4) != 0;
	}

void __cdecl NxContactPlaneMesh(const NxCollisionShape* plane, const NxCollisionShape* shape,
	NxContactSink* sink, void* context)
	{
	TriangleMesh* const mesh = nxPlaneMeshData(shape);
	if(!mesh || !mesh->mInternal.mVertices || !mesh->mInternal.mTriangles ||
		!mesh->mInternal.mTriangleCount || !mesh->mInternal.mVertexCount)
		return;
	if(!nxPlaneMeshQuery(plane, shape, context, true))
		return;

	Opcode::PlanesCollider* const collider = reinterpret_cast<Opcode::PlanesCollider*>(
		static_cast<NxU8*>(context) + 0x60);
	const NxU32 candidateCount = collider->GetNbTouchedPrimitives();
	const udword* const candidateFaces = collider->GetTouchedPrimitives();
	if(!candidateCount || !candidateFaces)
		return;

	const NxTriangle32* const triangles =
		static_cast<const NxTriangle32*>(mesh->mInternal.mTriangles);
	const NxVec3* const vertices = static_cast<const NxVec3*>(mesh->mInternal.mVertices);
	const NxU32 stamp = nxScratchStamp(context);
	NxU32* const emitted = *reinterpret_cast<NxU32**>(
		static_cast<NxU8*>(context) + 0x08);
	const NxU16 planeMaterial = *reinterpret_cast<const NxU16*>(
		reinterpret_cast<const NxU8*>(plane) + 0xda);

	for(NxU32 candidate = 0; candidate < candidateCount; ++candidate)
		{
		const NxU32 face = candidateFaces[candidate];
		const NxU32* const indices = triangles[face].v;
		for(unsigned corner = 0; corner < 3; ++corner)
			{
			const NxU32 vertexIndex = indices[corner];
			if(emitted[vertexIndex] == stamp)
				continue;
			emitted[vertexIndex] = stamp;
			const NxVec3 world = nxPlaneMeshWorldVertex(shape, vertices[vertexIndex]);
			const NxReal distance = nxPlaneMeshDistance(plane, world);
			if(distance <= 0.0f)
				{
				const NxU16 meshMaterial = mesh->mInternal.mMaterialIndices
					? mesh->mInternal.mMaterialIndices[face] : 0xffffu;
				NxEmitContact(sink, shape->collisionObject, plane->collisionObject,
					nxPlaneMeshBits(distance), &world,
					reinterpret_cast<const NxVec3*>(plane->geometry), meshMaterial,
					planeMaterial);
			}
		}
	}
	}
