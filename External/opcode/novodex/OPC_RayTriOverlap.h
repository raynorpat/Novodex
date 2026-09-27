/*
 * NOVODEX LOCAL MODIFICATION
 * upstream: External/opcode/upstream/Opcode/OPC_RayTriOverlap.h
 *
 * [1] The culling branch widens the barycentric bounds by RayCollider's added
 *     member at +0x88 (OPC_RayCollider.h [1]): U and V may undershoot 0 and
 *     U+V may overshoot det by that tolerance, where stock rejects on the sign
 *     bit and on det exactly. Both bounds are formed once as floats. The
 *     non-culling branch and the distance test are stock.
 *     V is compared and summed as the unrounded register value: the image
 *     stores it to mStabbedFace.mV as a float (`fst [esi+0x58]`) but tests
 *     `fcom [lower]` and forms U+V with `fadd st(1)` from the register, so V's
 *     lifetime is a double here (the project's x87 convention) and only the
 *     member is float. U is compared as the float it was stored as.
 *     established at 0x000b873b (fld [esi+0x88]; fchs; fstp -> the lower bound),
 *     0x000b874e (mU < lower -> reject), 0x000b8765 (fadd [esi+0x88] -> det +
 *     tolerance), 0x000b8773 (mU > upper -> reject), 0x000b87d9 (mV < lower),
 *     0x000b87eb (mU + mV > upper), 0x000b8824 (sign test of mDistance, stock),
 *     all in RayCollider::_RayStab(const AABBCollisionNode*) at 0x000b84c0, and
 *     0x000b87d9..0x000b87ef for V (fcom; fst [esi+0x58]; fld U; fadd st(1)).
 *     The block is inlined at 14 sites in all (the `fld [reg+0x88]` of each):
 *     RayCollider::InitQuery 0x000b5aef, 0x000b5f75; _SegmentStab 0x000b65be,
 *     0x000b6bc3, 0x000b7113, 0x000b755d, 0x000b7b30, 0x000b7f7a; _RayStab
 *     0x000b873b, 0x000b8d50, 0x000b92d4, 0x000b9715, 0x000b9d1d, 0x000ba15e.
 *     The non-culling arm at 0x000b8851
 *     keeps stock's IS_NEGATIVE_FLOAT and IEEE_1_0 tests (0x000b88c9, 0x000b88d4).
 */
#define LOCAL_EPSILON 0.000001f

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Computes a ray-triangle intersection test.
 *	Original code from Tomas Möller's "Fast Minimum Storage Ray-Triangle Intersection".
 *	It's been optimized a bit with integer code, and modified to return a non-intersection if distance from
 *	ray origin to triangle is negative.
 *
 *	\param		vert0	[in] triangle vertex
 *	\param		vert1	[in] triangle vertex
 *	\param		vert2	[in] triangle vertex
 *	\return		true on overlap. mStabbedFace is filled with relevant info.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline_ BOOL RayCollider::RayTriOverlap(const Point& vert0, const Point& vert1, const Point& vert2)
{
	// Stats
	mNbRayPrimTests++;

	// Find vectors for two edges sharing vert0
	Point edge1 = vert1 - vert0;
	Point edge2 = vert2 - vert0;

	// Begin calculating determinant - also used to calculate U parameter
	Point pvec = mDir^edge2;

	// If determinant is near zero, ray lies in plane of triangle
	float det = edge1|pvec;

	if(mCulling)
	{
		if(det<LOCAL_EPSILON)														return FALSE;
		// From here, det is > 0. So we can use integer cmp.

		// Calculate distance from vert0 to ray origin
		Point tvec = mOrigin - vert0;

		// Calculate U parameter and test bounds
		mStabbedFace.mU = tvec|pvec;
//		if(IR(u)&0x80000000 || u>det)					return FALSE;
		// NOVODEX [1]: bounds widened by the +0x88 tolerance.
		const float LowerBound = -mNovodeXSetting88;
		if(mStabbedFace.mU<LowerBound)												return FALSE;
		const float UpperBound = det + mNovodeXSetting88;
		if(mStabbedFace.mU>UpperBound)												return FALSE;

		// Prepare to test V parameter
		Point qvec = tvec^edge1;

		// Calculate V parameter and test bounds
		// NOVODEX [1]: V tested and summed unrounded; only the member is float.
		// (Point::operator| returns a float, which /fp:precise rounds on return.)
		const double V = double(mDir.x)*qvec.x + double(mDir.y)*qvec.y + double(mDir.z)*qvec.z;
		mStabbedFace.mV = float(V);
		if(V<LowerBound || mStabbedFace.mU+V>UpperBound)							return FALSE;

		// Calculate t, scale parameters, ray intersects triangle
		mStabbedFace.mDistance = edge2|qvec;
		// Det > 0 so we can early exit here
		// Intersection point is valid if distance is positive (else it can just be a face behind the orig point)
		if(IS_NEGATIVE_FLOAT(mStabbedFace.mDistance))								return FALSE;
		// Else go on
		float OneOverDet = 1.0f / det;
		mStabbedFace.mDistance *= OneOverDet;
		mStabbedFace.mU *= OneOverDet;
		mStabbedFace.mV *= OneOverDet;
	}
	else
	{
		// the non-culling branch
		if(det>-LOCAL_EPSILON && det<LOCAL_EPSILON)									return FALSE;
		float OneOverDet = 1.0f / det;

		// Calculate distance from vert0 to ray origin
		Point tvec = mOrigin - vert0;

		// Calculate U parameter and test bounds
		mStabbedFace.mU = (tvec|pvec) * OneOverDet;
//		if(IR(u)&0x80000000 || u>1.0f)					return FALSE;
		if(IS_NEGATIVE_FLOAT(mStabbedFace.mU) || IR(mStabbedFace.mU)>IEEE_1_0)		return FALSE;

		// prepare to test V parameter
		Point qvec = tvec^edge1;

		// Calculate V parameter and test bounds
		mStabbedFace.mV = (mDir|qvec) * OneOverDet;
		if(IS_NEGATIVE_FLOAT(mStabbedFace.mV) || mStabbedFace.mU+mStabbedFace.mV>1.0f)	return FALSE;

		// Calculate t, ray intersects triangle
		mStabbedFace.mDistance = (edge2|qvec) * OneOverDet;
		// Intersection point is valid if distance is positive (else it can just be a face behind the orig point)
		if(IS_NEGATIVE_FLOAT(mStabbedFace.mDistance))								return FALSE;
	}
	return TRUE;
}
