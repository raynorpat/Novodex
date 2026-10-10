#include "GeometrySdkHeaderSeam.h"
#include "GeometryDomain.h"
#include "GeometryDomainInputs.h"
#include "GeometryBudgets.h"
#include "FixtureSupport.h"
#include "NxIntersectionRayTriangle.h"
#include "NxBoxDistance.h"
#include "NxTriangleDistance.h"
#include "ContactGeneration.h"
#include "portable/NxScalarGeometry.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <fstream>
#include <algorithm>
static float fromWord(unsigned w) { float v;std::memcpy(&v,&w,4);return v; }
static unsigned toWord(float v) { unsigned w;std::memcpy(&w,&v,4);return w; }
static NxVec3 vertex(const float* p) { return NxVec3(p[0],p[1],p[2]); }
struct Observation { unsigned discrete, count; double out[16]; };
static Observation observe(const GeometryDomainInput& row) {
    float input[32];for(unsigned i=0;i<32;++i) input[i]=fromWord(row.in[i]);
    Observation out={0,0,{}};
    float s=-123.25f,t=-123.25f,r=-123.25f,closest[3]={-123.25f,-123.25f,-123.25f};
    switch(row.op) {
    case 0:
        out.discrete=NxRayTriIntersect(vertex(input),vertex(input+3),vertex(input+6),vertex(input+9),vertex(input+12),r,s,t,row.in[15]!=0);
        out.count=3;out.out[0]=r;out.out[1]=s;out.out[2]=t;break;
    case 1:
        out.count=3;out.out[0]=NxPointTriangleSquareDistance(input,input+3,input+6,input+9,&s,&t);
        out.out[1]=s;out.out[2]=t;break;
    case 2: {
        NxSegment segment;segment.p0=vertex(input);segment.p1=vertex(input+3);
        out.count=4;out.out[0]=NxSegmentTriangleSquareDistance(&segment,input+6,input+9,input+12,&r,&s,&t);
        out.out[1]=r;out.out[2]=s;out.out[3]=t;break;
    }
    case 3: {
        NxSegment segment;segment.p0=vertex(input);segment.p1=vertex(input+3);
        out.count=5;out.out[0]=NxSegmentBoxSquareDistance(&segment,input+6,input+9,input+12,&r,closest);
        out.out[1]=r;for(unsigned i=0;i<3;++i)out.out[2+i]=closest[i];break;
    }
    case 4:
        out.count=4;out.out[0]=NxPointBoxSquareDistance(input,input+3,input+6,input+9,closest);
        for(unsigned i=0;i<3;++i)out.out[1+i]=closest[i];break;
    case 5: {
        NxDistanceLine line;std::memcpy(line.origin,input,12);std::memcpy(line.direction,input+3,12);
        NxCollisionBoxData box;std::memcpy(box.center,input+6,12);std::memcpy(box.extents,input+9,12);std::memcpy(box.rotation,input+12,36);
        out.count=5;out.out[0]=NxLineBoxSquareDistance(&line,&box,&r,closest,closest+1,closest+2);
        out.out[1]=r;for(unsigned i=0;i<3;++i)out.out[2+i]=closest[i];break;
    }
    case 6: {
        float a[3],b[3];NxLineLineClosestPoints(a,b,input,input+3,input+6,input+9);
        out.count=6;for(unsigned i=0;i<3;++i) {out.out[i]=a[i];out.out[3+i]=b[i];}break;
    }
    case 7: case 8: case 9: {
        NxCollisionShape shape={};shape.rotation[0]=shape.rotation[4]=shape.rotation[8]=1;
        std::memcpy(shape.translation,input+6,12);shape.geometry[0]=input[9];shape.geometry[1]=input[10];
        if(row.op==8) {shape.geometry[0]=0;shape.geometry[1]=0;shape.geometry[2]=-1;shape.geometry[3]=0;}
        NxRay ray;ray.orig=vertex(input);ray.dir=vertex(input+3);
        // Initialize object representation through the permitted byte view;
        // the test subsequently assigns all semantic floating fields explicitly.
        NxRaycastHit hit;unsigned char* bytes=reinterpret_cast<unsigned char*>(&hit);
        for(unsigned i=0;i<sizeof(hit);++i)bytes[i]=0;
        hit.distance=-123.25f;hit.worldImpact=NxVec3(-123.25f);hit.worldNormal=NxVec3(-123.25f);
        hit.faceID=0x12345678;hit.flags=0x12345678;hit.u=hit.v=-123.25f;
        const NxCollisionShape* returned=row.op==7?NxShapeRaycastSphere(&shape,0,&ray,input[11],0,row.in[12]?NX_RAYCAST_NORMAL:0,&hit):
            row.op==8?NxShapeRaycastPlane(&shape,0,&ray,input[11],0,row.in[12]?NX_RAYCAST_NORMAL:0,&hit):
            NxShapeRaycastCapsule(&shape,0,&ray,input[11],0,row.in[12]?NX_RAYCAST_NORMAL:0,&hit);
        out.discrete=(returned?1u:0u)|(hit.flags<<1);out.count=10;
        out.out[0]=hit.distance;out.out[1]=hit.worldImpact.x;out.out[2]=hit.worldImpact.y;out.out[3]=hit.worldImpact.z;
        out.out[4]=hit.worldNormal.x;out.out[5]=hit.worldNormal.y;out.out[6]=hit.worldNormal.z;
        out.out[7]=hit.faceID;out.out[8]=hit.u;out.out[9]=hit.v;break;
    }
    case 10:
#if NX_PHYSICS_USE_X87
        // The exact PMap listing, isolated for reference capture. The scalar
        // consumer calls the production helper used by PMap, never this copy.
        if(input[0]!=0||input[1]!=0||input[2]!=0) {
            const float one=1;
            __asm {
                fld dword ptr [input+8]
                fmul st(0),st(0)
                fld dword ptr [input+4]
                fmul st(0),st(0)
                faddp st(1),st
                fld dword ptr [input]
                fmul st(0),st(0)
                faddp st(1),st
                fsqrt
                fdivr dword ptr [one]
                fld dword ptr [input]
                fmul st(0),st(1)
                fstp dword ptr [input]
                fld dword ptr [input+4]
                fmul st(0),st(1)
                fstp dword ptr [input+4]
                fld dword ptr [input+8]
                fmul st(0),st(1)
                fstp dword ptr [input+8]
                fstp st(0)
            }
        }
#else
        nxScalarNormalizeDirection3(input);
#endif
        out.count=3;for(unsigned i=0;i<3;++i)out.out[i]=input[i];break;
    }
    return out;
}
static void put32(std::ostream& out,unsigned w) { for(unsigned i=0;i<4;++i)out.put((char)(w>>(8*i))); }
static unsigned get32(const unsigned char* p) { return (unsigned)p[0]|((unsigned)p[1]<<8)|((unsigned)p[2]<<16)|((unsigned)p[3]<<24); }
static void putDouble(std::ostream& out,double d) { unsigned char b[8];std::memcpy(b,&d,8);out.write((char*)b,8); }
int nxGeometryExport(const char* destination) {
    std::ifstream existing(destination,std::ios::binary);if(existing.good())return 2;
    std::ofstream out(destination,std::ios::binary);
    const unsigned records=sizeof(geometryDomainInputs)/sizeof(geometryDomainInputs[0]);
    out.write("NXPF",4);put32(out,1);put32(out,records*272);put32(out,272);
    for(const auto& row:geometryDomainInputs) {
        put32(out,row.op);put32(out,row.id);for(unsigned w:row.in)put32(out,w);
        Observation result=observe(row);put32(out,result.discrete);put32(out,result.count);
        for(double d:result.out)putDouble(out,d);
    }
    return out.good()?0:3;
}
int nxGeometryDomain(const char* fixture,bool measure) {
    std::vector<unsigned char> bytes;std::string error;
    if(!nxReadFixture(fixture,bytes,error)) {std::fprintf(stderr,"%s\n",error.c_str());return 1;}
    const unsigned count=sizeof(geometryDomainInputs)/sizeof(geometryDomainInputs[0]);
    if(bytes.size()!=count*272)return 1;
    unsigned failures=0;double maxAbs[11]={},maxRel[11]={};
    for(unsigned c=0;c<count;++c) {
        const auto& row=geometryDomainInputs[c];const unsigned char* p=bytes.data()+c*272;
        if(get32(p)!=row.op||get32(p+4)!=row.id)return 1;
        for(unsigned i=0;i<32;++i)if(get32(p+8+4*i)!=row.in[i])return 1;
        Observation result=observe(row);
        if(result.discrete!=get32(p+136)||result.count!=get32(p+140)) {++failures;std::fprintf(stderr,"FAIL discrete op%u id%u\n",row.op,row.id);}
        for(unsigned i=0;i<result.count;++i) {
            double ref;std::memcpy(&ref,p+144+8*i,8);double actual=result.out[i];
            bool accepted;
            if(std::isfinite(actual)&&std::isfinite(ref)) {
                double e=std::fabs(actual-ref);maxAbs[row.op]=std::max(maxAbs[row.op],e);
                if(ref!=0)maxRel[row.op]=std::max(maxRel[row.op],e/std::fabs(ref));
                accepted=geometryOutputAccepted(row.op,i,actual,ref);
            }else accepted=std::isnan(ref)?std::isnan(actual):actual==ref;
            if(!measure&&!accepted) {++failures;std::fprintf(stderr,"FAIL numeric op%u id%u field%u actual%.17g ref%.17g\n",row.op,row.id,i,actual,ref);}
        }
        unsigned normalField=row.op==10?0:4;
        if(row.op==10||((row.op>=7&&row.op<=9)&&(result.discrete&1)&&(result.discrete&8))) {
            double reference[3];std::memcpy(reference,p+144+8*normalField,24);
            if(!geometryNormalAccepted(result.out+normalField,reference)) {++failures;std::fprintf(stderr,"FAIL normal geometry op%u id%u\n",row.op,row.id);}
        }
    }
    Observation baseRay={},baseNormal={};
    for(const auto& row:geometryDomainInputs) {
        if(row.id==300&&row.op==0)baseRay=observe(row);
        if(row.id==300&&row.op==10)baseNormal=observe(row);
        if(row.op==0&&(row.id==301||row.id==302)) {
            Observation transformed=observe(row);
            if(baseRay.discrete!=transformed.discrete)++failures;
            for(unsigned i=0;i<3;++i) {GeometryBudget b=geometryBudget(0,i);
                if(!nxWithinBudget(transformed.out[i],baseRay.out[i],b.absolute,b.relative))++failures;}
        }
        if(row.op==10&&row.id==301) {
            Observation scaled=observe(row);
            if(!geometryNormalAccepted(scaled.out,baseNormal.out))++failures;
        }
    }
    for(unsigned op=0;op<11;++op)std::printf("domain op=%u max_absolute=%.17g max_relative=%.17g\n",op,maxAbs[op],maxRel[op]);
    return failures?1:0;
}
