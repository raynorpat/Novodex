// Sphere/triangle-mesh contact generation (matrix A [SPHERE][MESH]).
// Matrix A [SPHERE][MESH]. The handler queries the owned OPCODE model for
// sphere candidates and reproduces the ordinary-mesh face-side rejection
// before constructing the point/triangle contact stream. The tested ordinary-
// mesh face and one boundary-edge normal path are exact. Heightfield-specific
// normals, remaining edge/corner cases and broader mesh response remain open.

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

static void nxSphereMeshNormalizeDelta(const NxVec3& delta,
	double distanceSquared, NxVec3& normal)
	{
	// The oracle-matching path keeps the double squared-distance in x87 through
	// sqrt, reciprocal and each component product. Narrowing either value changes
	// the observed patch normal by one or more ULPs.
	const NxReal deltaX = delta.x;
	const NxReal deltaY = delta.y;
	const NxReal deltaZ = delta.z;
	NxReal nx;
	NxReal ny;
	NxReal nz;
	__asm
		{
		fld distanceSquared
		fsqrt
		fld1
		fdiv st(0), st(1)
		fld deltaX
		fmul st(0), st(1)
		fstp nx
		fld deltaY
		fmul st(0), st(1)
		fstp ny
		fld deltaZ
		fmul st(0), st(1)
		fstp nz
		fstp st(0)
		fstp st(0)
		}
	normal.x = nx;
	normal.y = ny;
	normal.z = nz;
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
	if(nxSphereMeshBits(r[0]) == 0x3f800000 && nxSphereMeshBits(r[1]) == 0
		&& nxSphereMeshBits(r[2]) == 0 && nxSphereMeshBits(r[3]) == 0
		&& nxSphereMeshBits(r[4]) == 0x3f800000 && nxSphereMeshBits(r[5]) == 0
		&& nxSphereMeshBits(r[6]) == 0 && nxSphereMeshBits(r[7]) == 0
		&& nxSphereMeshBits(r[8]) == 0x3f800000)
		{
		world = local;
		return;
		}
	world.x = (NxReal) (((double) r[0] * local.x + (double) r[1] * local.y)
		+ (double) r[2] * local.z);
	world.y = (NxReal) (((double) r[3] * local.x + (double) r[4] * local.y)
		+ (double) r[5] * local.z);
	world.z = (NxReal) (((double) r[6] * local.x + (double) r[7] * local.y)
		+ (double) r[8] * local.z);
	}

static bool nxSphereMeshProjectionInside(const NxReal* a, const NxReal* b,
	const NxReal* c, const NxVec3& point, NxU32 verticalAxis)
	{
	const NxU32 u = verticalAxis == 0 ? 1 : 0;
	const NxU32 v = verticalAxis == 2 ? 1 : 2;
	const double abU = (double) b[u] - a[u];
	const double abV = (double) b[v] - a[v];
	const double acU = (double) c[u] - a[u];
	const double acV = (double) c[v] - a[v];
	const double apU = (double) point[u] - a[u];
	const double apV = (double) point[v] - a[v];
	const double denominator = abU * acV - acU * abV;
	if(denominator == 0.0)
		return false;
	const double s = (apU * acV - acU * apV) / denominator;
	const double t = (abU * apV - apU * abV) / denominator;
	return s >= 0.0 && t >= 0.0 && s + t <= 1.0;
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
	const bool isHeightfield = mesh->mHeightFieldVerticalAxis != 0xff;
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
	NxU32 heightfieldFace = 0xffffffff;
	if(isHeightfield)
		for(NxU32 candidate = 0; candidate < candidateCount; ++candidate)
			{
			const NxU32* const tri = triangles + candidateFaces[candidate] * 3;
			const NxReal* const v0 = vertices + tri[0] * 3;
			const NxReal* const v1 = vertices + tri[1] * 3;
			const NxReal* const v2 = vertices + tri[2] * 3;
			if(nxSphereMeshProjectionInside(v0, v1, v2, centerLocal,
				mesh->mHeightFieldVerticalAxis))
				{
				heightfieldFace = candidateFaces[candidate];
				break;
				}
			}
	for(NxU32 candidate = 0; candidate < candidateCount; ++candidate)
		{
		const NxU32 face = candidateFaces[candidate];
		if(heightfieldFace != 0xffffffff && face != heightfieldFace)
			continue;
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
		if((!isHeightfield && signedFaceDistance < 0.0f) || signedFaceDistance > radius)
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

		NxVec3 contactDelta(centerLocal.x - closestLocal.x,
			centerLocal.y - closestLocal.y, centerLocal.z - closestLocal.z);
		NxVec3 normalLocal;
		if(isHeightfield)
			normalLocal = faceNormal;
		else
		nxSphereMeshNormalizeDelta(contactDelta, distanceSquared, normalLocal);
		NxVec3 normalWorld;
		nxSphereMeshToWorldVector(meshShape, normalLocal, normalWorld);
		if(isHeightfield)
			{
			NxReal normalLengthSquared = normalWorld.x * normalWorld.x
				+ normalWorld.y * normalWorld.y;
			normalLengthSquared += normalWorld.z * normalWorld.z;
			const NxReal normalLength = nxSphereMeshSqrt(normalLengthSquared);
			if(normalLength != 0.0f)
				{
				const NxReal inverseNormalLength = 1.0f / normalLength;
				normalWorld.x *= inverseNormalLength;
				normalWorld.y *= inverseNormalLength;
				normalWorld.z *= inverseNormalLength;
				}
			}

		NxVec3 point;
		if(isHeightfield)
			{
			point.x = sphere->translation[0];
			point.y = sphere->translation[1];
			point.z = sphere->translation[2];
			}
		else
			{
			nxSphereMeshToWorldVector(meshShape, closestLocal, point);
			point.x += meshShape->translation[0];
			point.y += meshShape->translation[1];
			point.z += meshShape->translation[2];
			}
		NxReal contactDistanceSquared;
		NxReal separation;
		__asm
			{
			// 001927 stores the squared local delta before taking sqrt and subtracting r.
			fld contactDelta.x
			fmul contactDelta.x
			fld contactDelta.y
			fmul contactDelta.y
			faddp st(1), st(0)
			fld contactDelta.z
			fmul contactDelta.z
			faddp st(1), st(0)
			fstp contactDistanceSquared
			fld contactDistanceSquared
			fsqrt
			fsub radius
			fstp separation
			}
		const NxU32 material = mesh->mInternal.mMaterialIndices
			? mesh->mInternal.mMaterialIndices[face] : 0xffff;
		const NxU32 remappedFace = mesh->mInternal.mFaceRemap
			? mesh->mInternal.mFaceRemap[face] : face;

		NxEmitContactFeatures(sink, 0, sphere->collisionObject, meshShape->collisionObject,
			nxSphereMeshBits(separation), &point, &normalWorld,
			sphereFeature, material, 0xffffffff, remappedFace);
		}
	}

// phys_fn_001925 (0x0004a820), matrix B [SPHERE][MESH]. The oracle selects
// first-contact mode, disables temporal coherence and primitive-test skipping,
// then reports OPCODE's sphere/model contact bit.
bool __cdecl NxOverlapSphereMesh(const NxCollisionShape* sphere,
	const NxCollisionShape* meshShape, void* context)
	{
	NxU32& contextFlags = *reinterpret_cast<NxU32*>(static_cast<NxU8*>(context) + 0xb4);
	contextFlags |= 1;
	contextFlags &= ~NxU32(2 | 0x10);
	const TriangleMesh* const mesh = reinterpret_cast<const TriangleMesh*>(
		static_cast<size_t>(*(const NxU32*) &meshShape->geometry[0]));
	if(!mesh || !mesh->mInternal.mModel)
		{
		contextFlags &= ~NxU32(4);
		return false;
		}

	NxVec3 centerLocal;
	nxSphereMeshToLocal(meshShape,
		NxVec3(sphere->translation[0], sphere->translation[1], sphere->translation[2]), centerLocal);
	IceCore::Container candidates;
	Opcode::SphereCache cache;
	cache.TouchedPrimitives = &candidates;
	IceMaths::Sphere querySphere(
		IceMaths::Point(centerLocal.x, centerLocal.y, centerLocal.z), sphere->geometry[0]);
	Opcode::SphereCollider collider;
	collider.SetFirstContact(true);
	collider.SetTemporalCoherence(false);
	collider.SetPrimitiveTests(true);
	const bool querySucceeded = collider.Collide(cache, querySphere,
		*static_cast<const Opcode::Model*>(mesh->mInternal.mModel));
	const bool overlap = querySucceeded && collider.GetContactStatus() != 0;
	if(overlap)
		contextFlags |= 4;
	else
		contextFlags &= ~NxU32(4);
	return overlap && (contextFlags & 4) != 0;
	}
