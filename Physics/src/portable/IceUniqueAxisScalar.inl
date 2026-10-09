#include <cmath>
#include <cstring>
// Actual001661: canonical sign by x's sign bit; strict original0.9999f gate.
bool nxIceAddUniqueAxis(IceCore::Container* axes,const IceMaths::Point* axis)
{
    IceMaths::Point local=*axis;NxU32 bits;std::memcpy(&bits,&local.x,4);
    if(bits&0x80000000u){local.x=-local.x;local.y=-local.y;local.z=-local.z;}
    const NxU32* words=axes->GetEntries();
    for(NxU32 i=0;i<axes->GetNbEntries()/3;++i){IceMaths::Point stored;std::memcpy(&stored,words+3*i,12);
        const double value=(double(local.z)*stored.z+double(local.y)*stored.y)+double(local.x)*stored.x;
        if(std::fabs(value)>double(kIceMeshToolsAxisLimit))return false;}
    NxU32 output[3];std::memcpy(output,&local,12);nxIceContainerAddPoint(axes,output);return true;
}
