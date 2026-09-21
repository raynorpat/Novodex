/*----------------------------------------------------------------------------*\
|																			  |
|								NovodeX Technology							  |
|																			  |
|								 www.novodex.com							  |
|																			  |
\*----------------------------------------------------------------------------*/

// The two exported joint-descriptor rows Phase 6 owns.
//
//   NxJointDesc_SetGlobalAnchor  0x000980b0  (census phys_fn_004115, 558 bytes)
//   NxJointDesc_SetGlobalAxis    0x000982e0  (census phys_fn_004117, 1131 bytes)
//
// Neither is a setter. Each walks the descriptor's two actors and, for every
// actor that has a body, transforms the passed WORLD value into that actor's
// local frame; for an actor with no body it copies the world value straight
// through. The axis row normalises its input first and also derives
// localNormal from it.
//
// WHY THE ADDRESS ARITHMETIC IS SPELLED THIS WAY. The oracle reaches each
// actor's world pose through its own object graph -- actor+0x10 is the actor
// descriptor, the descriptor's shape array, the shape's body pointer, and
// body+0x19c is the pose. Those are internal offsets with no public accessor,
// so they are written here as byte offsets with a comment naming what each one
// is, rather than as calls through the public interface. A call through the
// public interface would be a different computation, not the same one spelled
// differently.
//
// The rotation matrix is built as `m[i][j]` so that `m[j][i]` in the transform
// below is the matrix the oracle builds: the transcription stores the six
// products in the order the decompiler reports them, and that order is what
// makes the row read back the words the oracle reads back.
//
// The joint differential (tests/PhysicsJointTests.cpp) drives both rows and
// prints every output word, so a transcription error shows up as a moved word
// rather than as a silent difference.

#include "NxJointDesc.h"
#include "NxActor.h"
#include "NxVec3.h"
#include "NxUtilities.h"

#include <string.h>

namespace
	{

// The pose block the oracle reads at body+0x19c. A non-null second word means
// the body carries a cached matrix; otherwise the pose is a quaternion whose
// components the caller composes into one.
struct NxJointPoseView
	{
	unsigned char pad00[8];
	void* cached;			// +0x08: cached world matrix, or null
	};

struct NxJointBodyView
	{
	unsigned char pad00[0x19c];
	NxJointPoseView* pose;	// +0x19c
	};

// The quaternion the pose falls back to, and the translation beside it. The
// oracle reads x at +0x5c, y at +0x60, z at +0x64, w at +0x68, and the
// translation at +0x50, +0x54, +0x58.
struct NxJointQuatView
	{
	unsigned char pad00[0x50];
	float tx;				// +0x50
	float ty;				// +0x54
	float tz;				// +0x58
	float x;				// +0x5c
	float y;				// +0x60
	float z;				// +0x64
	float w;				// +0x68
	};

// Returns the actor's world matrix in `m` and its translation in `t`, or null
// when the actor carries no body. This mirrors the oracle's guard exactly:
// actor -> actor descriptor -> first shape -> body, and a null anywhere on that
// chain means "no body".
//
// The composed matrix is written into a workspace the caller owns for the
// duration of one row call; the oracle composes into its own stack frame, and
// the row never hands the pointer back to its caller.
inline float* nxJointMatrixWorkspace()
	{
	static float workspace[12];
	return workspace;
	}

inline const float* nxJointWorldMatrix(NxActor* actor, const float*& t)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(actor);

	// actor+0x14 is the actor's BODY, which is the chain the oracle itself uses:
	// Scene::createJoint reads the same word to test whether an actor is dynamic, and
	// nxActorBuildBody is what links it here. The previous version walked
	// actor+0x10 -> descriptor -> shape -> body, which is a chain nothing in this
	// reconstruction builds -- it found null at the first level and returned 0, so
	// every actor took the copy-through arm and the transform never executed.
	void* body = *reinterpret_cast<void**>(p + 0x14);
	if(body == 0)
		return 0;

	NxJointBodyView* bodyView = reinterpret_cast<NxJointBodyView*>(body);
	NxJointPoseView* pose = bodyView->pose;
	if(pose == 0)
		return 0;
	void* cached = *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(pose) + 8);
	if(cached != 0)
		{
		// The cached matrix is a 3x3 followed by the translation; the caller
		// reads elements 9, 10 and 11 as the translation.
		const float* m = reinterpret_cast<const float*>(cached);
		t = m + 9;
		return m;
		}

	// Compose the matrix from the quaternion. The doubled products are spelled
	// `a * a + a * a` rather than `2 * a * a` because that is the order the
	// oracle's x87 stream evaluates, and the differential compares bits.
	NxJointQuatView* q = reinterpret_cast<NxJointQuatView*>(pose);
	const float qx = q->x, qy = q->y, qz = q->z, qw = q->w;
	t = reinterpret_cast<const float*>(&q->tx);

	// The composed matrix lives in the same per-actor workspace the caller
	// already owns; it is written and consumed inside one call.
	float* m = nxJointMatrixWorkspace();

	// The doubled products are spelled `a * a + a * a` because that is the order the
	// oracle's x87 stream evaluates, and the differential compares bits.
	const float fVar10 = qy * qy + qy * qy;		// y*y + y*y
	const float fVar7 = qz * qz + qz * qz;		// z*z + z*z
	const float fVar6 = qy * qx + qy * qx;		// y*x + y*x
	const float fVar8 = qz * qw + qz * qw;		// z*w + z*w
	const float fVar9 = qz * qx + qz * qx;		// z*x + z*x
	const float fVar11 = qy * qw + qy * qw;		// y*w + y*w
	const float fVar3b = qz * qy + qz * qy;		// z*y + z*y
	const float fVar1b = qx * qw + qx * qw;		// x*w + x*w

	m[0] = 1.0f - (qx * qx + qx * qx);
	m[1] = fVar6 - fVar8;
	m[2] = fVar11 + fVar9;
	m[3] = fVar1b + fVar3b;
	m[4] = 1.0f - (fVar10 + fVar7);
	m[5] = fVar9 - fVar11;
	m[6] = fVar8 - fVar6;
	m[7] = fVar3b + fVar1b;
	m[8] = 1.0f - (fVar10 + fVar7);
	m[9] = q->tx;
	m[10] = q->ty;
	m[11] = q->tz;
	return m;
	}

	} // namespace

NX_C_EXPORT NXP_DLL_EXPORT void NX_CALL_CONV NxJointDesc_SetGlobalAnchor(
	NxJointDesc& dis, const NxVec3& wsAnchor)
	{
	for(int i = 0; i < 2; ++i)
		{
		const float* t = 0;
		const float* m = dis.actor[i] ? nxJointWorldMatrix(dis.actor[i], t) : 0;
		if(m == 0)
			{
			// No body: the world value is the local value.
			dis.localAnchor[i] = wsAnchor;
			continue;
			}

		const float dx = wsAnchor.x - t[0];
		const float dy = wsAnchor.y - t[1];
		const float dz = wsAnchor.z - t[2];
		dis.localAnchor[i].x = m[0] * dx + m[3] * dy + m[6] * dz;
		dis.localAnchor[i].y = m[1] * dx + m[4] * dy + m[7] * dz;
		dis.localAnchor[i].z = m[2] * dx + m[5] * dy + m[8] * dz;
		}
	}

NX_C_EXPORT NXP_DLL_EXPORT void NX_CALL_CONV NxJointDesc_SetGlobalAxis(
	NxJointDesc& dis, const NxVec3& wsAxis)
	{
	NxVec3 axis = wsAxis;
	const float length = NxMath::sqrt(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
	if(length != 0.0f)
		{
		// The oracle holds `1/length` in an x87 register and multiplies each
		// component by it before any rounding to 32 bits (`fdivr` at
		// 0x10098339, then three `fmul st(1)` and three `fstp dword`). Keeping
		// the reciprocal and the products in double is how this transcription
		// gets the same intermediate precision; a float reciprocal rounds first
		// and moves the normalised axis by one ULP, which the joint differential
		// catches on case 0.
		const double inv = 1.0 / static_cast<double>(length);
		axis.x = static_cast<float>(static_cast<double>(axis.x) * inv);
		axis.y = static_cast<float>(static_cast<double>(axis.y) * inv);
		axis.z = static_cast<float>(static_cast<double>(axis.z) * inv);
		}

	// The oracle derives the two tangents from the normalised axis before it
	// touches either actor, so both actors share them. It writes the SECOND
	// tangent into localNormal: the call site pushes the first tangent at
	// [esp+0xc], the normal at [esp+0x48] and the second at [esp+0x30], and the
	// localNormal stores read the [esp+0x30] block. The differential pins this --
	// reading the first tangent instead moves localNormal on every case while
	// leaving localAxis identical.
	NxVec3 tangent;
	NxVec3 binormal;
	NxNormalToTangents(axis, tangent, binormal);

	for(int i = 0; i < 2; ++i)
		{
		const float* t = 0;
		const float* m = dis.actor[i] ? nxJointWorldMatrix(dis.actor[i], t) : 0;
		if(m == 0)
			{
			dis.localAxis[i] = axis;
			dis.localNormal[i] = binormal;
			continue;
			}

		dis.localAxis[i].x = m[0] * axis.x + m[3] * axis.y + m[6] * axis.z;
		dis.localAxis[i].y = m[1] * axis.x + m[4] * axis.y + m[7] * axis.z;
		dis.localAxis[i].z = m[2] * axis.x + m[5] * axis.y + m[8] * axis.z;
		dis.localNormal[i].x = m[0] * binormal.x + m[3] * binormal.y + m[6] * binormal.z;
		dis.localNormal[i].y = m[1] * binormal.x + m[4] * binormal.y + m[7] * binormal.z;
		dis.localNormal[i].z = m[2] * binormal.x + m[5] * binormal.y + m[8] * binormal.z;
		}
	}