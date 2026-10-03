// Sphere/triangle-mesh contact generation (matrix A [SPHERE][MESH]).
// Matrix A [SPHERE][MESH]. The handler queries the owned OPCODE model for
// sphere candidates and reproduces the ordinary-mesh face-side rejection
// before constructing the point/triangle contact stream. Heightfield-specific
// normals, all of 001927's edge cases and matrix-B overlap remain open.

#include "ContactGeneration.h"
#include "NxTriangleDistance.h"
#include "TriangleMesh.h"

#include <string.h>

static NxReal nxSphereMeshSqrt(double value)
	{
	NxReal result;
	__asm
		{
		fld value
		fsqrt
		fstp result
		}
	return result;
	}

static NxU32 nxSphereMeshBits(NxReal value)
	{
	NxU32 result;
	memcpy(&result, &value, sizeof(result));
	return result;
	}

static void nxSphereMeshToLocal(const NxCollisionShape* meshShape,
	const NxVec3& world, NxVec3& local)
	{
	const NxReal x = (NxReal) ((double) world.x - meshShape->translation[0]);
	const NxReal y = (NxReal) ((double) world.y - meshShape->translation[1]);
	const NxReal z = (NxReal) ((double) world.z - meshShape->translation[2]);
	const NxReal* const r = meshShape->rotation;
	local.x = (NxReal) (((double) x * r[0] + (double) y * r[3]) + (double) z * r[6]);
	local.y = (NxReal) (((double) x * r[1] + (double) y * r[4]) + (double) z * r[7]);
	local.z = (NxReal) (((double) x * r[2] + (double) y * r[5]) + (double) z * r[8]);
	}

static void nxSphereMeshToWorldVector(const NxCollisionShape* meshShape,
	const NxVec3& local, NxVec3& world)
	{
	const NxReal* const r = meshShape->rotation;
	world.x = (NxReal) (((double) r[0] * local.x + (double) r[1] * local.y)
		+ (double) r[2] * local.z);
	world.y = (NxReal) (((double) r[3] * local.x + (double) r[4] * local.y)
		+ (double) r[5] * local.z);
	world.z = (NxReal) (((double) r[6] * local.x + (double) r[7] * local.y)
		+ (double) r[8] * local.z);
	}

void __cdecl NxContactSphereMesh(const NxCollisionShape* sphere,
	const NxCollisionShape* meshShape, NxContactSink* sink, void* context)
	{
	(void) context;
	const TriangleMesh* const mesh = reinterpret_cast<const TriangleMesh*>(
		static_cast<size_t>(*(const NxU32*) &meshShape->geometry[0]));
	if(!mesh || !mesh->mInternal.mVertices || !mesh->mInternal.mTriangles)
		return;
	// 001929 clears the two transient query bits and selects the mesh-query mode
	// in the shared pair context before it touches the triangle candidates.
	NxU32& contextFlags = *reinterpret_cast<NxU32*>(static_cast<NxU8*>(context) + 0xb4);
	contextFlags &= ~NxU32(3);
	if(mesh->mHeightFieldVerticalAxis == 0xff)
		contextFlags &= ~NxU32(0x10);
	else
		contextFlags |= 0x10;

	const NxReal radius = sphere->geometry[0];
	const NxReal radiusSquared = radius * radius;
	NxVec3 centerLocal;
	nxSphereMeshToLocal(meshShape,
		NxVec3(sphere->translation[0], sphere->translation[1], sphere->translation[2]), centerLocal);
	const NxReal* const vertices = static_cast<const NxReal*>(mesh->mInternal.mVertices);
	const NxU32* const triangles = static_cast<const NxU32*>(mesh->mInternal.mTriangles);
	const NxU32 sphereFeature = *(const NxU16*)((const NxU8*) sphere + 0xda);
	IceCore::Container candidates;
	Opcode::SphereCache cache;
	cache.TouchedPrimitives = &candidates;
	IceMaths::Sphere querySphere(
		IceMaths::Point(centerLocal.x, centerLocal.y, centerLocal.z), radius);
	Opcode::SphereCollider collider;
	if(!mesh->mInternal.mModel
		|| !collider.Collide(cache, querySphere,
			*static_cast<const Opcode::Model*>(mesh->mInternal.mModel)))
		return;

	const NxU32 candidateCount = candidates.GetNbEntries();
	const NxU32* const candidateFaces = candidates.GetEntries();
	for(NxU32 candidate = 0; candidate < candidateCount; ++candidate)
		{
		const NxU32 face = candidateFaces[candidate];
		const NxU32* const tri = triangles + face * 3;
		const NxReal* const v0 = vertices + tri[0] * 3;
		const NxReal* const v1 = vertices + tri[1] * 3;
		const NxReal* const v2 = vertices + tri[2] * 3;
		const NxReal e10 = v1[0] - v0[0];
		const NxReal e11 = v1[1] - v0[1];
		const NxReal e12 = v1[2] - v0[2];
		const NxReal e20 = v2[0] - v0[0];
		const NxReal e21 = v2[1] - v0[1];
		const NxReal e22 = v2[2] - v0[2];
		NxVec3 faceNormal(e11 * e22 - e12 * e21,
			e12 * e20 - e10 * e22, e10 * e21 - e11 * e20);
		const NxReal faceNormalLength = nxSphereMeshSqrt(
			(double) faceNormal.x * faceNormal.x
			+ (double) faceNormal.y * faceNormal.y
			+ (double) faceNormal.z * faceNormal.z);
		if(faceNormalLength == 0.0f)
			continue;
		faceNormal.x /= faceNormalLength;
		faceNormal.y /= faceNormalLength;
		faceNormal.z /= faceNormalLength;
		const NxReal signedFaceDistance =
			(centerLocal.x - v0[0]) * faceNormal.x
			+ (centerLocal.y - v0[1]) * faceNormal.y
			+ (centerLocal.z - v0[2]) * faceNormal.z;
		if(signedFaceDistance < 0.0f || signedFaceDistance > radius)
			continue;

		NxReal s = 0.0f;
		NxReal t = 0.0f;
		const double distanceSquared = NxPointTriangleSquareDistance(
			&centerLocal.x, v0, v1, v2, &s, &t);
		if(!(distanceSquared <= (double) radiusSquared) || distanceSquared == 0.0)
			continue;

		NxVec3 closestLocal;
		closestLocal.x = (NxReal) ((double) v0[0]
			+ (double) s * ((double) v1[0] - v0[0])
			+ (double) t * ((double) v2[0] - v0[0]));
		closestLocal.y = (NxReal) ((double) v0[1]
			+ (double) s * ((double) v1[1] - v0[1])
			+ (double) t * ((double) v2[1] - v0[1]));
		closestLocal.z = (NxReal) ((double) v0[2]
			+ (double) s * ((double) v1[2] - v0[2])
			+ (double) t * ((double) v2[2] - v0[2]));

		NxVec3 normalLocal(centerLocal.x - closestLocal.x,
			centerLocal.y - closestLocal.y, centerLocal.z - closestLocal.z);
		const NxReal distance = nxSphereMeshSqrt(distanceSquared);
		const NxReal inverseDistance = 1.0f / distance;
		normalLocal.x *= inverseDistance;
		normalLocal.y *= inverseDistance;
		normalLocal.z *= inverseDistance;
		NxVec3 normalWorld;
		nxSphereMeshToWorldVector(meshShape, normalLocal, normalWorld);

		NxVec3 point(sphere->translation[0] - radius * normalWorld.x,
			sphere->translation[1] - radius * normalWorld.y,
			sphere->translation[2] - radius * normalWorld.z);
		const NxReal separation = distance - radius;
		const NxU32 material = mesh->mInternal.mMaterialIndices
			? mesh->mInternal.mMaterialIndices[face] : 0xffff;
		const NxU32 remappedFace = mesh->mInternal.mFaceRemap
			? mesh->mInternal.mFaceRemap[face] : face;

		NxEmitContactFeatures(sink, 0, sphere->collisionObject, meshShape->collisionObject,
			nxSphereMeshBits(separation), &point, &normalWorld,
			sphereFeature, material, 0xffffffff, remappedFace);
		}
	}
