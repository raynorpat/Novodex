/*
 * NOVODEX LOCAL MODIFICATION
 * upstream: External/opcode/upstream/Opcode/OPC_RayAABBOverlap.h
 *
 * Preserve the oracle's x87 intermediate for each ray/AABB cross-axis
 * comparison. The original image compares the extended `f` expression against
 * the extended radius sum; materializing `f` as a float can flip an exact
 * boundary ray at the root node.
 * Oracle instructions: RayCollider::RayAABBOverlap at 0x000b912d..0x000b913f.
 */
#include "NxPhysicsBackend.h"

// Eight binary32 values passed by value. Ordered strict separation compares
// abs(a*b-c*d) with e*f+g*h; equality and unordered values continue the next
// axis. No receiver, scratch, output write or FPU-state change.
static inline BOOL nxRayAABBCrossSeparated(float a, float b, float c, float d,
	float e, float f, float g, float h)
	{
#if NX_PHYSICS_USE_X87
	unsigned separated;
	__asm
		{
		fld dword ptr [a]
		fmul dword ptr [b]
		fld dword ptr [c]
		fmul dword ptr [d]
		fsubp st(1), st
		fabs
		fld dword ptr [e]
		fmul dword ptr [f]
		fld dword ptr [g]
		fmul dword ptr [h]
		faddp st(1), st
		fxch st(1)
		fcompp
		fnstsw ax
		sahf
		seta al
		movzx eax, al
		mov separated, eax
		}
	return separated != 0;
#else
    const double cross = double(a) * b - double(c) * d;
    const double radius = double(e) * f + double(g) * h;
    return fabs(cross) > radius;
#endif
	}

#if NX_PHYSICS_USE_X87
inline_ BOOL RayCollider::SegmentAABBOverlap(const Point& center, const Point& extents)
	{
	mNbRayBVTests++;
	float Dx = mData2.x - center.x; if(fabsf(Dx) > extents.x + mFDir.x) return FALSE;
	float Dy = mData2.y - center.y; if(fabsf(Dy) > extents.y + mFDir.y) return FALSE;
	float Dz = mData2.z - center.z; if(fabsf(Dz) > extents.z + mFDir.z) return FALSE;
	float f;
	f = mData.y * Dz - mData.z * Dy; if(fabsf(f) > extents.y*mFDir.z + extents.z*mFDir.y) return FALSE;
	f = mData.z * Dx - mData.x * Dz; if(fabsf(f) > extents.x*mFDir.z + extents.z*mFDir.x) return FALSE;
	f = mData.x * Dy - mData.y * Dx; if(fabsf(f) > extents.x*mFDir.y + extents.y*mFDir.x) return FALSE;
	return TRUE;
	}

#else
inline_ BOOL RayCollider::SegmentAABBOverlap(const Point &center, const Point &extents)
{
    mNbRayBVTests++;
    float Dx = mData2.x - center.x;
    if (fabsf(Dx) > double(extents.x) + mFDir.x)
        return FALSE;
    float Dy = mData2.y - center.y;
    if (fabsf(Dy) > double(extents.y) + mFDir.y)
        return FALSE;
    float Dz = mData2.z - center.z;
    if (fabsf(Dz) > double(extents.z) + mFDir.z)
        return FALSE;
    float f;
    f = float(double(mData.y) * Dz - double(mData.z) * Dy);
    if (fabsf(f) > double(extents.y) * mFDir.z + double(extents.z) * mFDir.y)
        return FALSE;
    f = float(double(mData.z) * Dx - double(mData.x) * Dz);
    if (fabsf(f) > double(extents.x) * mFDir.z + double(extents.z) * mFDir.x)
        return FALSE;
    f = float(double(mData.x) * Dy - double(mData.y) * Dx);
    if (fabsf(f) > double(extents.x) * mFDir.y + double(extents.y) * mFDir.x)
        return FALSE;
    return TRUE;
}

#endif
inline_ BOOL RayCollider::RayAABBOverlap(const Point& center, const Point& extents)
	{
	mNbRayBVTests++;
	float Dx = mOrigin.x - center.x; if(GREATER(Dx, extents.x) && Dx*mDir.x >= 0.0f) return FALSE;
	float Dy = mOrigin.y - center.y; if(GREATER(Dy, extents.y) && Dy*mDir.y >= 0.0f) return FALSE;
	float Dz = mOrigin.z - center.z; if(GREATER(Dz, extents.z) && Dz*mDir.z >= 0.0f) return FALSE;
	const bool cross0 = nxRayAABBCrossSeparated(mDir.y, Dz, mDir.z, Dy,
		extents.y, mFDir.z, extents.z, mFDir.y);
	if(cross0) return FALSE;
	const bool cross1 = nxRayAABBCrossSeparated(mDir.z, Dx, mDir.x, Dz,
		extents.x, mFDir.z, extents.z, mFDir.x);
	if(cross1) return FALSE;
	const bool cross2 = nxRayAABBCrossSeparated(mDir.x, Dy, mDir.y, Dx,
		extents.x, mFDir.y, extents.y, mFDir.x);
	if(cross2) return FALSE;
	return TRUE;
	}
