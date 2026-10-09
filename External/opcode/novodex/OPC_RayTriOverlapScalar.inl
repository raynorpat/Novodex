// Preserve the binary32 Point and dot-result stores after each complete
// three-term expression; products and cross differences remain binary64.
static inline Point nxOpcodeRayCross(const Point &a, const Point &b)
{
    return Point(float(double(a.y) * b.z - double(a.z) * b.y), float(double(a.z) * b.x - double(a.x) * b.z),
                 float(double(a.x) * b.y - double(a.y) * b.x));
}
static inline float nxOpcodeRayDot(const Point &a, const Point &b)
{
    return float((double(a.x) * b.x + double(a.y) * b.y) + double(a.z) * b.z);
}

inline_ BOOL RayCollider::RayTriOverlap(const Point &vert0, const Point &vert1, const Point &vert2)
{
    mNbRayPrimTests++;

    Point edge1 = vert1 - vert0;
    Point edge2 = vert2 - vert0;

    Point pvec = nxOpcodeRayCross(mDir, edge2);

    float det = nxOpcodeRayDot(edge1, pvec);

    if (mCulling)
    {
        if (det < LOCAL_EPSILON)
            return FALSE;

        Point tvec = mOrigin - vert0;

        mStabbedFace.mU = nxOpcodeRayDot(tvec, pvec);
        const float LowerBound = -mNovodeXSetting88;
        if (mStabbedFace.mU < LowerBound)
            return FALSE;
        const float UpperBound = det + mNovodeXSetting88;
        if (mStabbedFace.mU > UpperBound)
            return FALSE;

        Point qvec = nxOpcodeRayCross(tvec, edge1);

        const double V = double(mDir.x) * qvec.x + double(mDir.y) * qvec.y + double(mDir.z) * qvec.z;
        mStabbedFace.mV = float(V);
        if (V < LowerBound || mStabbedFace.mU + V > UpperBound)
            return FALSE;

        mStabbedFace.mDistance = nxOpcodeRayDot(edge2, qvec);
        if (IS_NEGATIVE_FLOAT(mStabbedFace.mDistance))
            return FALSE;
        float OneOverDet = 1.0f / det;
        mStabbedFace.mDistance *= OneOverDet;
        mStabbedFace.mU *= OneOverDet;
        mStabbedFace.mV *= OneOverDet;
    }
    else
    {
        if (det > -LOCAL_EPSILON && det < LOCAL_EPSILON)
            return FALSE;
        float OneOverDet = 1.0f / det;

        Point tvec = mOrigin - vert0;

        mStabbedFace.mU = (nxOpcodeRayDot(tvec, pvec)) * OneOverDet;
        if (IS_NEGATIVE_FLOAT(mStabbedFace.mU) || IR(mStabbedFace.mU) > IEEE_1_0)
            return FALSE;

        Point qvec = nxOpcodeRayCross(tvec, edge1);

        mStabbedFace.mV = (nxOpcodeRayDot(mDir, qvec)) * OneOverDet;
        if (IS_NEGATIVE_FLOAT(mStabbedFace.mV) || double(mStabbedFace.mU) + mStabbedFace.mV > 1.0f)
            return FALSE;

        mStabbedFace.mDistance = (nxOpcodeRayDot(edge2, qvec)) * OneOverDet;
        if (IS_NEGATIVE_FLOAT(mStabbedFace.mDistance))
            return FALSE;
    }
    return TRUE;
}
