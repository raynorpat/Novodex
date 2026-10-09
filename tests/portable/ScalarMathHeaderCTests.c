#include "portable/NxScalarMath.h"
#include <stdint.h>
int main(void)
{
    int32_t result=9;
    if(!nxScalarIsRoundToNearest()) return 1;
    if(nxScalarSqrt(4)!=2 || nxScalarSqrtSum2(2,2)!=2 || nxScalarSqrtSum3(1,1,2)!=2 ||
       nxScalarSqrtSum4(1,1,1,1)!=2 || nxScalarSqrtDiffSum(6,3,1)!=2 ||
       nxScalarSqrtDiag(6,1,2)!=2 || nxScalarSqrtMulSub(3,3,5)!=2) return 2;
    if(nxScalarSqrtDot2(1,1,1,3)!=2 || nxScalarSqrtDot3(1,1,1,1,1,2)!=2 ||
       nxScalarSqrtDot4(1,1,1,1,1,1,1,1)!=2 || nxScalarSqrtQuotDot3(12,1,1,1,1,1,1)!=2) return 3;
    if(nxScalarCosHalfNorm3(0,.5,1,0,0)!=1 || nxScalarSinHalfOverNorm3(0,.5,1,0,0)!=0 ||
       nxScalarRateOverRoot(1,2,0)!=4 || nxScalarAcos(1)!=0 || nxScalarJointAcos(1)!=0 ||
       !nxScalarFinite(nxScalarJointCIacos(0)) || !nxScalarFinite(nxScalarAcosRateOverRoot(0,1))) return 4;
    if(nxScalarStoreFloat(1)!=1 || !nxScalarInt32(2.5,NX_SCALAR_NEAREST,&result) || result!=2 ||
       nxScalarInt32(2147483648.0,NX_SCALAR_CHOP,&result) || result!=2 ||
       nxScalarFistpLow32(4294967295.0)!=-1 || nxScalarInt32OrIndefinite(2147483648.0,NX_SCALAR_FLOOR)!=INT32_MIN) return 5;
    return 0;
}
