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
// actor's world pose through its own object graph -- actor+0x14 is the outer
// body, body+8 is the dynamic record, and record+0x19c is the pose. These are
// internal offsets with no public accessor,
// so they are written here as byte offsets with a comment naming what each one
// is, rather than as calls through the public interface. A call through the
// public interface would be a different computation, not the same one spelled
// differently.
//
// The rotation matrix is composed row-major from the record's quaternion with
// the listing's x87 pattern, and the local value is M^T times the world value,
// summed in the listing's order in the register (53-bit here, so double).
// Joint-open-items Task 4 rewrote both from the Capstone listing: the earlier
// transcription agreed with the oracle only for identity-oriented bodies.
//
// The joint differential (tests/PhysicsJointTests.cpp) drives both rows and
// prints every output word, so a transcription error shows up as a moved word
// rather than as a silent difference.

#include "NxJointDesc.h"
#include "NxActor.h"
#include "NxVec3.h"
#include "NxUtilities.h"

#include <math.h>
#include <string.h>

namespace
	{

// The pose block the oracle reads at record+0x19c. A non-null second word
// points back to the dynamic record carrying quaternion and translation.
struct NxJointPoseView
	{
	unsigned char pad00[8];
void* cached;			// +0x08: dynamic record, or null
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

	// The exported rows walk actor+0x14 -> outer+8 -> record+0x19c.
	void* body = *reinterpret_cast<void**>(p + 0x14);
	if(body == 0)
		return 0;
	void* record = *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(body) + 8);
	if(record == 0)
		return 0;

	NxJointBodyView* bodyView = reinterpret_cast<NxJointBodyView*>(record);
	NxJointPoseView* pose = bodyView->pose;
	if(pose == 0)
		return 0;
	void* cached = *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(pose) + 8);
	if(cached == 0)
		{
		const float* m = reinterpret_cast<const float*>(
			reinterpret_cast<unsigned char*>(pose) + 0x20);
		t = m + 9;
		return m;
		}

	// Compose the matrix from the quaternion as both rows do (0x980ff-0x981c5
	// in the anchor row, 0x983c0-0x98483 in the axis row). Every doubled
	// product is `fmul; fadd st(0), st(0)` in an x87 register (53-bit here,
	// so double); five of them are spilled to float and reloaded, and the
	// nine elements are stored row-major. Joint-open-items Task 4 replaced a
	// transcription that was right only for the identity quaternion.
	NxJointQuatView* q = reinterpret_cast<NxJointQuatView*>(cached);
	const double qx = q->x, qy = q->y, qz = q->z, qw = q->w;
	t = reinterpret_cast<const float*>(&q->tx);

	float* m = nxJointMatrixWorkspace();

	const float yy2 = static_cast<float>(qy * qy + qy * qy);		// spilled
	const double zz2 = qz * qz + qz * qz;
	m[0] = static_cast<float>((1.0 - yy2) - zz2);
	const double xy2 = qy * qx + qy * qx;
	const double zw2 = qz * qw + qz * qw;
	m[1] = static_cast<float>(xy2 - zw2);
	const float xz2 = static_cast<float>(qz * qx + qz * qx);		// spilled
	const double yw2 = qy * qw + qy * qw;
	const float yw2Spill = static_cast<float>(yw2);				// fst, kept
	m[2] = static_cast<float>(yw2 + xz2);
	m[3] = static_cast<float>(zw2 + xy2);
	const double xx1 = 1.0 - (qx * qx + qx * qx);
	const float xx1Spill = static_cast<float>(xx1);				// fst, kept
	m[4] = static_cast<float>(xx1 - zz2);
	const float yz2 = static_cast<float>(qz * qy + qz * qy);		// spilled
	const double xw2 = qx * qw + qx * qw;
	m[5] = static_cast<float>(static_cast<double>(yz2) - xw2);
	m[6] = static_cast<float>(static_cast<double>(xz2) - yw2Spill);
	m[7] = static_cast<float>(xw2 + yz2);
	m[8] = static_cast<float>(static_cast<double>(xx1Spill) - yy2);
	m[9] = q->tx;
	m[10] = q->ty;
	m[11] = q->tz;
	return m;
	}

// out = M^T v, as both rows form it: each component is
// (m[6+c] * z + m[3+c] * y) + m[c] * x in an x87 register (53-bit here, so
// double), rounded to float only at the store. Joint-open-items Task 4: the
// earlier float sum in x, y, z order agreed with the listing only while the
// products were exact, i.e. for identity-oriented bodies.
inline void nxJointTransposeMultiply(const float* m, double x, double y, double z, NxVec3& out)
	{
	out.x = static_cast<float>((m[6] * z + m[3] * y) + m[0] * x);
	out.y = static_cast<float>((m[7] * z + m[4] * y) + m[1] * x);
	out.z = static_cast<float>((m[8] * z + m[5] * y) + m[2] * x);
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

		// 0x981fb-0x982a0: x - t.x stays in a register, y - t.y and z - t.z
		// are spilled to float; each component sums the column's z and y
		// products first and adds the x product last, in the register.
		const double dx = static_cast<double>(wsAnchor.x) - t[0];
		const float dy = static_cast<float>(static_cast<double>(wsAnchor.y) - t[1]);
		const float dz = static_cast<float>(static_cast<double>(wsAnchor.z) - t[2]);
		nxJointTransposeMultiply(m, dx, dy, dz, dis.localAnchor[i]);
		}
	}

NX_C_EXPORT NXP_DLL_EXPORT void NX_CALL_CONV NxJointDesc_SetGlobalAxis(
	NxJointDesc& dis, const NxVec3& wsAxis)
	{
	NxVec3 axis = wsAxis;
	// 0x982f9-0x98326: (z*z + y*y) + x*x and its square root stay in the
	// register; the float length the earlier transcription rounded to is not
	// in the listing.
	const double length = sqrt((static_cast<double>(axis.z) * axis.z +
		static_cast<double>(axis.y) * axis.y) + static_cast<double>(axis.x) * axis.x);
	if(length != 0.0)
		{
		// The oracle holds `1/length` in an x87 register and multiplies each
		// component by it before any rounding to 32 bits (`fdivr` at
		// 0x10098339, then three `fmul st(1)` and three `fstp dword`). Keeping
		// the reciprocal and the products in double is how this transcription
		// gets the same intermediate precision; a float reciprocal rounds first
		// and moves the normalised axis by one ULP, which the joint differential
		// catches on case 0.
		const double inv = 1.0 / length;
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

		// 0x984c9-0x9854d (axis) and 0x98673-0x986f5 (second tangent).
		nxJointTransposeMultiply(m, axis.x, axis.y, axis.z, dis.localAxis[i]);
		nxJointTransposeMultiply(m, binormal.x, binormal.y, binormal.z, dis.localNormal[i]);
		}
	}
