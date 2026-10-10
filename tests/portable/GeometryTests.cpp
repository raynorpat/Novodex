#include "GeometrySdkHeaderSeam.h"
#include "NxIntersectionRayTriangle.h"
#include "NxIntersectionSegmentCapsule.h"
#include "NxIntersectionSweptSpheres.h"
#include "NxIntersectionRayPlane.h"
#include "NxPlane.h"
#include "NxSmoothNormals.h"
#include "NxBoxDistance.h"
#include "NxTriangleDistance.h"
#include "ContactGeneration.h"
#include "FixtureSupport.h"
#include "GeometryFixtureRows.h"
#include "GeometryDomain.h"
#include "GeometryBudgets.h"
#include "NxSmoothNormalsAngle.h"
#include "NxGeometryHelpers.h"
#include <cstdio>
#include <cfenv>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <cfloat>
static int failures;
static double maxAbsolute[5], maxRelative[5];
static void check(bool ok, const char* name) { if(!ok) { ++failures; std::fprintf(stderr,"FAIL %s\n",name); } }
static float f(unsigned w) { float v; std::memcpy(&v,&w,4); return v; }
static unsigned word(float v) { unsigned w; std::memcpy(&w,&v,4); return w; }
static NxVec3 vec(const unsigned* w) { return NxVec3(f(w[0]),f(w[1]),f(w[2])); }
static void poison(void* memory,unsigned n) { for(unsigned i=0;i<n;++i) { unsigned w=0xcdcd0000u+i; std::memcpy((char*)memory+4*i,&w,4); } }
// Canary tests intentionally inspect complete object representations, including
// padding. Access through unsigned char preserves every byte without invoking
// NxVec3's memberwise assignment or a typed nontrivial-object memory operation.
static void fillBytes(void* memory,unsigned n,unsigned char byte) {
    unsigned char* p=static_cast<unsigned char*>(memory);for(unsigned i=0;i<n;++i)p[i]=byte;
}
static void compare(const GeometryFixtureRow& r,const unsigned* actual,unsigned family,bool measure) {
    for(unsigned i=0;i<r.nout;++i) {
        unsigned ref=r.out[i];
        // Poison/canaries are discrete storage observations, never numeric tolerance.
        if((ref&0xffff0000u)==0xcdcd0000u) { check(actual[i]==ref,r.name); continue; }
        double a=f(actual[i]),b=f(ref);
        if(std::isfinite(a)&&std::isfinite(b)) {
            double error=std::fabs(a-b);
            maxAbsolute[family]=std::max(maxAbsolute[family],error);
            if(b!=0) maxRelative[family]=std::max(maxRelative[family],error/std::fabs(b));
            // Families0/2 carry ray parameters;4 carries length;1 is a normal.
            GeometryBudget budget=family==1?geometryBudget(10,0):family==4?geometryBudget(7,0):geometryBudget(0,0);
            if(!measure) check(family==0?geometryOutputAccepted(0,i/3,a,b):
                b==0 ? a==0 : nxWithinBudget(a,b,budget.absolute,budget.relative),r.name);
        } else check(std::isnan(b)?std::isnan(a):(a==b),r.name);
    }
}
static void shipped(bool measure) {
    unsigned rows=0;
    for(const GeometryFixtureRow& r:geometryRows) {
        unsigned out[26]={}; bool result=false; unsigned family;
        if(std::strncmp(r.name,"NxRayTriIntersect.",18)==0) {
            family=0; float slots[11]; poison(slots,11);
            NxVec3 orig=vec(r.in), dir=vec(r.in+3), a=vec(r.in+6),b=vec(r.in+9),c=vec(r.in+12);
            float& u=r.in[16]==1 ? slots[0] : slots[4];
            result=NxRayTriIntersect(orig,dir,a,b,c,slots[0],u,slots[8],r.in[15]!=0);
            std::memcpy(out,slots,12); std::memcpy(out+3,&u,12); std::memcpy(out+6,slots+8,12);
        } else if(std::strncmp(r.name,"NxBuildSmoothNormals.",21)==0) {
            family=1;
            struct Buffer { NxVec3 verts[8]; unsigned canaries[2]; } normal, input;
            poison(&normal,26);
            for(unsigned i=0;i<8;++i) input.verts[i]=vec(r.in+5+i*3);
            input.canaries[0]=0xcdcd0018; input.canaries[1]=0xcdcd0019;
            NxU16 faces[36]; for(unsigned i=0;i<36;++i) faces[i]=(NxU16)r.in[29+i];
            NxVec3* output=r.in[4]==1?input.verts:normal.verts;
            result=NxBuildSmoothNormals(r.in[0],r.in[1],input.verts,
                (r.in[2]==0||r.in[2]==3)?r.in+29:0,(r.in[2]==1||r.in[2]==3)?faces:0,output,r.in[3]!=0);
            std::memcpy(out,output,sizeof(out));
        } else if(std::strncmp(r.name,"NxRayCapsuleIntersect.",22)==0) {
            family=2; NxVec3 orig=vec(r.in),dir=vec(r.in+3);
            NxCapsule cap; cap.p0=vec(r.in+6); cap.p1=vec(r.in+9); cap.radius=f(r.in[12]);
            float roots[4]; poison(roots,4);
            unsigned count=NxRayCapsuleIntersect(orig,dir,cap,r.in[13]==1?&orig.x:roots);
            check(count==r.result,r.name);
            if(r.in[13]==1) { out[0]=word(orig.x);out[1]=word(orig.y);out[2]=word(orig.z);out[3]=word(dir.x); }
            else std::memcpy(out,roots,16);
            result=count!=0;
        } else if(std::strncmp(r.name,"NxSweptSpheresIntersect.",24)==0) {
            family=3; NxSphere a,b; a.center=vec(r.in);a.radius=f(r.in[3]);b.center=vec(r.in+7);b.radius=f(r.in[10]);
            result=NxSweptSpheresIntersect(a,vec(r.in+4),b,vec(r.in+11));
        } else if(std::strncmp(r.name,"NxSegmentPlaneIntersect.",24)==0) {
            family=4; NxVec3 a=vec(r.in),b=vec(r.in+3);NxPlane plane;plane.normal=vec(r.in+6);plane.d=f(r.in[9]);
            struct Buffer { NxVec3 point;float canaries[2]; } output;poison(&output,5);
            // This block follows the original scratch offsets (point poison starts at 4).
            for(unsigned i=0;i<5;++i) { unsigned w=0xcdcd0004u+i;std::memcpy((char*)&output+4*i,&w,4); }
            float dist[3];poison(dist,3);
            NxVec3& point=r.in[10]==1?a:output.point;
            NxSegmentPlaneIntersect(a,b,plane,dist[0],point);
            std::memcpy(out,dist,12);std::memcpy(out+3,&point,12);
            out[6]=r.in[10]==1?r.in[3]:word(output.canaries[0]);out[7]=r.in[10]==1?r.in[4]:word(output.canaries[1]);
            check(r.returnsVoid,r.name);result=false;
        } else continue;
        if(!r.returnsVoid) check(result==(r.result!=0),r.name);
        compare(r,out,family,measure);++rows;
        if(family==1&&result)for(unsigned i=0;i<r.in[1];++i) {
            double a[3]={f(out[3*i]),f(out[3*i+1]),f(out[3*i+2])};
            double b[3]={f(r.out[3*i]),f(r.out[3*i+1]),f(r.out[3*i+2])};
            check(geometryNormalAccepted(a,b),r.name);
        }
    }
    check(rows==66,"all 66 covered shipped fixture rows consumed");
    for(unsigned i=0;i<5;++i) std::printf("family=%u max_absolute=%.17g max_relative=%.17g\n",i,maxAbsolute[i],maxRelative[i]);
}
static void contracts() {
    check(!geometryOutputAccepted(0,1,f(0x3f7fffffu),1),"barycentric endpoint rejects adjacent float");
    check(!geometryOutputAccepted(3,1,f(0x3f7fffffu),1),"segment box endpoint rejects adjacent float");
    const float a[3]={0,0,0},b[3]={4,0,0},c[3]={0,4,0};
    const float points[][3]={{1,1,2},{-1,1,2},{1,-1,2},{3,3,2},{-1,-1,2},{5,-1,2},{-1,5,2}};
    const double expected[]={4,5,5,6,6,6,6};
    for(unsigned i=0;i<7;++i) {
        float s=-99,t=-99;
        check(NxPointTriangleSquareDistance(points[i],a,b,c,&s,&t)==expected[i],"point triangle seven regions distance");
        check(s>=0&&t>=0&&s+t<=1,"point triangle valid closest parameters");
        check(NxPointTriangleSquareDistance(points[i],a,b,c,0,0)==expected[i],"point triangle null outputs");
    }
    NxSegment crossing;crossing.p0=NxVec3(1,1,-1);crossing.p1=NxVec3(1,1,1);
    float r,s,t;
    check(NxSegmentTriangleSquareDistance(&crossing,a,b,c,&r,&s,&t)==0&&r==.5f&&s==.25f&&t==.25f,"segment triangle interior crossing");
    check(NxSegmentTriangleSquareDistance(&crossing,a,b,c,0,0,0)==0,"segment triangle optional outputs");
    const float extents[3]={1,2,3};const float rotation[9]={1,0,0,0,1,0,0,0,1};
    const float point[3]={3,4,5};float closest[3];
    check(NxPointBoxSquareDistance(point,a,extents,rotation,closest)==12&&closest[0]==1&&closest[1]==2&&closest[2]==3,"point box clamped corner");
    NxDistanceLine line={{0,4,0},{1,0,0}};NxCollisionBoxData box={};
    std::memcpy(box.extents,extents,12);std::memcpy(box.rotation,rotation,36);
    float untouched[3]={-123.25f,-123.25f,-123.25f};
    check(NxLineBoxSquareDistance(&line,&box,0,untouched,untouched+1,untouched+2)==4&&untouched[0]==-123.25f&&untouched[1]==-123.25f&&untouched[2]==-123.25f,"line box null parameter suppresses all writes");
    const float min[3]={-1,-1,-1},max[3]={1,1,1},origin[3]={2,0,0},dir[3]={0,1,0};
    float nearValue=0,farValue=0;
    check(NxRayAABBSlab(min,max,origin,dir,&nearValue,&farValue)==-1&&nearValue==-FLT_MAX&&farValue==FLT_MAX,"slab early miss initializes outputs");
    NxVec3 vertices[4]={NxVec3(0,0,0),NxVec3(4,0,0),NxVec3(0,4,0),NxVec3(2,2,0)};
    const NxU32 indices[3]={0,1,2};
    check(nxWithinBudget(nxSmoothNormalsAngleAtVertex(0,indices,vertices),1.5707963267948966,1e-7,0),"typed normal angle at right corner");
    check(nxWithinBudget(nxSmoothNormalsAngleAtVertex(1,indices,vertices),.7853981633974483,1e-7,0),"typed normal angle at acute corner");
    check(nxSmoothNormalsAngleAtVertex(3,indices,vertices)==0,"typed angle vertex absent uses repeated first index");
    NxVec3 normals[4]={NxVec3(9),NxVec3(9),NxVec3(9),NxVec3(9)};
    check(!NxBuildSmoothNormals(1,3,0,indices,0,normals,false)&&normals[0].x==9,"normal null vertices failure leaves output");
    check(!NxBuildSmoothNormals(1,3,vertices,indices,0,0,false),"normal null output rejection");
    check(NxBuildSmoothNormals(1,4,vertices,indices,0,normals,false)&&normals[3].isZero(),"unused vertex initialized to zero");
    NxCollisionShape shape={};shape.rotation[0]=shape.rotation[4]=shape.rotation[8]=1;
    shape.geometry[0]=1;shape.geometry[1]=2;
    NxRay ray;ray.orig=NxVec3(0,0,-5);ray.dir=NxVec3(0,0,1);
    struct HitBuffer { unsigned before;NxRaycastHit hit;unsigned after; } guarded;
    fillBytes(&guarded,sizeof(guarded),0xa5);NxRaycastHit& hit=guarded.hit;
    check(NxShapeRaycastSphere(&shape,0,&ray,3,0,NX_RAYCAST_NORMAL,&hit)==0,"sphere maximum distance rejects");
    check(hit.distance==4&&hit.worldImpact.z==-1&&word(hit.worldNormal.x)==0xa5a5a5a5&&hit.flags==0xa5a5a5a5,"sphere rejected hit retains impact and distance only");
    check(NxShapeRaycastSphere(&shape,0,&ray,10,0,NX_RAYCAST_NORMAL,&hit)==&shape&&hit.distance==4&&hit.worldNormal.z==-1&&hit.flags==0x17,"sphere normal and accepted hit flags");
    fillBytes(&hit,sizeof(hit),0xa5);shape.geometry[0]=0;shape.geometry[1]=0;shape.geometry[2]=-1;
    check(NxShapeRaycastPlane(&shape,0,&ray,4,0,NX_RAYCAST_NORMAL,&hit)==0&&hit.worldImpact.z==0&&word(hit.distance)==0xa5a5a5a5,"plane rejected maximum writes impact only");
    // Preserve padding with byte copy. C++ memberwise assignment does not copy
    // the x64 tail padding and cannot specify a whole-buffer unchanged check.
    ray.dir.z=-1;unsigned char snapshot[sizeof(hit)];std::memcpy(snapshot,&hit,sizeof(hit));
    check(NxShapeRaycastPlane(&shape,0,&ray,10,0,0,&hit)==0&&std::memcmp(snapshot,&hit,sizeof(hit))==0,"plane facing failure writes nothing");
    shape.geometry[0]=1;shape.geometry[1]=2;shape.geometry[2]=0;ray.orig=NxVec3(-5,0,0);ray.dir=NxVec3(1,0,0);
    fillBytes(&hit,sizeof(hit),0xa5);
    check(NxShapeRaycastCapsule(&shape,0,&ray,10,0,NX_RAYCAST_NORMAL,&hit)==&shape&&hit.distance==4&&hit.flags==0x13&&word(hit.worldNormal.x)==0xa5a5a5a5,"capsule normal untouched despite hint");
    shape.geometry[1]=0;std::memcpy(snapshot,&hit,sizeof(hit));
    check(NxShapeRaycastCapsule(&shape,0,&ray,10,0,0,&hit)==0&&std::memcmp(snapshot,&hit,sizeof(hit))==0,"degenerate capsule reproducer leaves hit untouched");
    check(guarded.before==0xa5a5a5a5&&guarded.after==0xa5a5a5a5,"shape hit canaries intact");
    check(std::fegetround()==FE_TONEAREST,"geometry kernels preserve nearest environment");
}
int main(int argc,char** argv) {
    if(std::fegetround()!=FE_TONEAREST) return 2;
    bool measure=argc==3&&std::strcmp(argv[2],"--measure")==0;
    shipped(measure);
    if(argc>=2) failures+=nxGeometryDomain(argv[1],measure);
    contracts();
    NxVec3 a(0,0,0), b(4,0,0), c(0,4,0), o(1,1,5), d(0,0,-1);
    float t=99,u=99,v=99;
    if(!NxRayTriIntersect(o,d,a,b,c,t,u,v,true) || t!=5 || u!=.25f || v!=.25f) {
        check(false,"triangle hit");
    }
    if(!failures) std::puts("Geometry actual-source kernels passed"); return failures?1:0;
}
