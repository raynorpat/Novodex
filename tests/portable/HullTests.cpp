#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include <map>
#include <string>
#include <cstddef>
#include "FixtureSupport.h"
#include "ConvexHull.h"
#include "IceMeshBuilder2.h"
#include "NxSdkAllocator.h"
#undef for
static_assert(sizeof(HullPolygon)==0x24&&sizeof(HullEdge)==8&&sizeof(EdgeDesc)==8,"raw32 hull array element contracts");
static_assert(offsetof(ConvexHull,mPolygons)==0x28&&offsetof(ConvexHull,mEdgePolygons)==0x48,"raw32 hull receiver contract");
static_assert(sizeof(Valencies)==0x14&&offsetof(Valencies,mAdjVerts)==0x10,"raw32 graph receiver contract");
#if NX_PHYSICS_USE_X87
#include <float.h>
#define RECEIVER(receiver) receiver,0
#else
#define RECEIVER(receiver) receiver
#endif
static int failures;
static unsigned reportLine;
static void check(bool ok,const char* why) { if(!ok){std::fprintf(stderr,"FAIL %s\n",why);++failures;} }
// Explicit host boundary. Production allocator setter/getter and every kernel
// and vendor algorithm in the target are the actual production definitions.
bool opcNovodeXSetIceError(const char*,const char*,int line) { reportLine=line;return false; }
void* opcNovodeXAlloc(size_t n) { return nxGetSdkAllocator()->malloc(n,NX_MEMORY_PERSISTENT); }
void opcNovodeXFree(void* p) { nxGetSdkAllocator()->free(p); }
class GuardAllocator : public SdkAllocator {
public:
    std::map<void*,size_t> blocks;
    bool failNext=false;
    size_t failScratchSize=0;
    void* malloc(size_t n,NxMemoryType type) { if(failNext){failNext=false;return 0;}if(type==NX_MEMORY_TEMP&&n==failScratchSize){failScratchSize=0;return 0;}unsigned char* raw=(unsigned char*)std::malloc(n+32);
        if(!raw)return 0;std::memset(raw,0x6a,16);std::memset(raw+16,0xcd,n);std::memset(raw+16+n,0x7b,16);blocks[raw+16]=n;return raw+16; }
    void* mallocDEBUG(size_t n,const char*,int,const char*,NxMemoryType t) { return malloc(n,t); }
    void* realloc(void* p,size_t n) { if(!p)return malloc(n,NX_MEMORY_PERSISTENT);void* q=malloc(n,NX_MEMORY_PERSISTENT);
        if(q){std::memcpy(q,p,n<blocks[p]?n:blocks[p]);free(p);}return q; }
    void free(void* p) { if(!p)return;auto i=blocks.find(p);check(i!=blocks.end(),"owned allocator block");if(i==blocks.end())return;
        unsigned char* raw=(unsigned char*)p-16;for(unsigned j=0;j<16;++j)check(raw[j]==0x6a&&raw[16+i->second+j]==0x7b,"allocator canary");blocks.erase(i);std::free(raw); }
};
struct Observation { unsigned kind,id,index,word; };
static std::vector<Observation> observations;
static unsigned fixtureId;
static double maxUnitResidual;
static void discrete(unsigned v) { observations.push_back({0,fixtureId,(unsigned)observations.size(),v}); }
static void numeric(unsigned kind,float v) { unsigned w;std::memcpy(&w,&v,4);observations.push_back({kind,fixtureId,(unsigned)observations.size(),w}); }
static float f(unsigned w) { float v;std::memcpy(&v,&w,4);return v; }
static void point(unsigned kind,const IceMaths::Point& p) { numeric(kind,p.x);numeric(kind,p.y);numeric(kind,p.z); }
static void unit(const IceMaths::Point& p) { point(1,p);const double square=(double(p.x)*p.x+double(p.y)*p.y)+double(p.z)*p.z;
    if(square){const double residual=std::fabs(std::sqrt(square)-1);if(residual>maxUnitResidual)maxUnitResidual=residual;} }
static void releaseHull(ConvexHull& h) {
    if(h.mPolygons)nxIceDeleteArray(h.mPolygons);if(h.mEdges)nxIceDeleteArray(h.mEdges);
    if(h.mPolygonVRefs)nxIceFree(h.mPolygonVRefs);if(h.mPolygonERefs)nxIceFree(h.mPolygonERefs);
    if(h.mEdgeNormals)nxIceFree(h.mEdgeNormals);if(h.mEdgeToPolygons)nxIceFree(h.mEdgeToPolygons);if(h.mEdgePolygons)nxIceFree(h.mEdgePolygons);
    if(h.mEdgeAxes){h.mEdgeAxes->~Container();std::free(h.mEdgeAxes);}
}
static void observeHull(const ConvexHull& h) {
    discrete(h.mWord00);point(2,h.mCentroid);discrete(h.mNbPolygons);
    if(h.mPolygons)for(unsigned i=0;i<h.mNbPolygons;++i){const HullPolygon& p=h.mPolygons[i];discrete(p.mNbVerts);
        discrete((unsigned)(p.mVRefs-h.mPolygonVRefs));for(unsigned j=0;j<p.mNbVerts;++j)discrete(p.mVRefs[j]);
        unit(p.mPlane.n);numeric(2,p.mPlane.d);numeric(2,p.mMin);numeric(2,p.mMax);
        discrete(p.mERefs!=0);if(p.mERefs){discrete((unsigned)(p.mERefs-h.mPolygonERefs));for(unsigned j=0;j<p.mNbVerts;++j)discrete(p.mERefs[j]);}}
    discrete(h.mNbEdges);if(h.mEdges)for(unsigned i=0;i<h.mNbEdges;++i){discrete(h.mEdges[i].mRef0);discrete(h.mEdges[i].mRef1);
        unit(h.mEdgeNormals[i]);const EdgeDesc& e=h.mEdgeToPolygons[i];discrete(e.Flags);discrete(e.Count);discrete(e.Offset);
        for(unsigned j=0;j<e.Count;++j)discrete(h.mEdgePolygons[e.Offset+j]);}
    discrete(h.mEdgeAxes!=0);if(h.mEdgeAxes){discrete(h.mEdgeAxes->GetNbEntries());const unsigned* words=h.mEdgeAxes->GetEntries();
        for(unsigned i=0;i<h.mEdgeAxes->GetNbEntries();++i)numeric(1,f(words[i]));}
}
static const NxU16 cubeFaces[]={0,2,1,0,3,2,4,5,6,4,6,7,0,1,5,0,5,4,3,7,6,3,6,2,0,4,7,0,7,3,1,2,6,1,6,5};
static const NxU16 tetraFaces[]={0,2,1,0,1,3,0,3,2,1,2,3};
// Literal binary32 transforms. The finite coordinate domain is [-3,6],
// including affine nonuniform scales/shears and degenerate leaf polygons.
static const unsigned transforms[][12]={
    {0x3f800000,0,0,0,0x3f800000,0,0,0,0x3f800000,0,0,0},
    {0x40000000,0,0,0,0x3f000000,0,0,0,0x3fc00000,0x3f800000,0xc0000000,0x40400000},
    {0x3f800000,0x3f000000,0,0,0x3f800000,0x3e800000,0x3e800000,0,0x3f800000,0xbf800000,0x40000000,0xc0400000},
    {0x3f800000,0x3dcccccd,0,0,0x3f800000,0x3e4ccccd,0x3e99999a,0,0x3f800000,0x3e800000,0xbf000000,0x3fa00000}
};
static const float directions[][3]={{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1},{1,1,0},{1,1,1},{-1,2,-3},{0.25f,-0.5f,1.25f},{0,0,0}};
static void domain() {
    GuardAllocator allocator;nxSetSdkAllocatorBridge(&allocator);fixtureId=0;
    for(unsigned tr=0;tr<4;++tr)for(unsigned shape=0;shape<2;++shape)for(unsigned reverse=0;reverse<1;++reverse){
        IceMaths::Point verts[8];const unsigned nb=shape?4:8;
        static const float base[8][3]={{0,0,0},{1,0,0},{1,1,0},{0,1,0},{0,0,1},{1,0,1},{1,1,1},{0,1,1}};
        static const float tetra[4][3]={{0,0,0},{1,0,0},{0,1,0},{0,0,1}};
        for(unsigned i=0;i<nb;++i){const float* b=shape?tetra[i]:base[i];const unsigned* t=transforms[tr];
            verts[i].Set((b[0]*f(t[0])+b[1]*f(t[1]))+b[2]*f(t[2])+f(t[9]),(b[0]*f(t[3])+b[1]*f(t[4]))+b[2]*f(t[5])+f(t[10]),(b[0]*f(t[6])+b[1]*f(t[7]))+b[2]*f(t[8])+f(t[11]));}
        NxU16 refs[36];unsigned nf=shape?4:12;std::memcpy(refs,shape?tetraFaces:cubeFaces,nf*6);
        if(reverse)for(unsigned i=0;i<nf;++i){NxU16 v=refs[3*i];refs[3*i]=refs[3*i+2];refs[3*i+2]=v;}

        ConvexHull h;std::memset(&h,0,sizeof(h));h.mWord00=0xcdcd1234;h.mNbVerts=nb;h.mVerts=verts;h.mNbFaces=nf;h.mFaces=refs;
        IceMaths::Point center;discrete(nxHullComputeCentroid(RECEIVER(&h),&center));point(2,center);
        for(unsigned i=0;i<nf;++i){numeric(3,nxHullTriangleArea(RECEIVER(refs+3*i),verts));nxHullTriangleCenter(RECEIVER(refs+3*i),verts,&center);point(2,center);}
        // Exercise real lazy callers before explicit rebuilds; no fixture hull
        // polygon/edge construction substitutes for a production dependency.
        IceMaths::Point firstAxis(1,0,0);unsigned firstKind=0xcdcd4567;
        if(tr==0&&shape==1){discrete(nxHullSupportFace(RECEIVER(&h),&firstAxis,0,&firstKind));discrete(firstKind);}
        if(tr==1&&shape==0)discrete(nxHullComputeEdgeAxes(&h));
        if(tr==2&&shape==1)discrete(nxHullSupportPolygon(RECEIVER(&h),&firstAxis,0));
        if(tr==3&&shape==0)discrete(nxHullComputeEdges(&h));
        // Any previous normal array is saved for post-observation bookkeeping:
        // historical001502 does not free it when rebuilding.
        IceMaths::Point* lazyNormals=h.mEdgeNormals;
        reportLine=0;discrete(nxHullComputePolygons(&h));discrete(reportLine);check(h.mNbPolygons>0,"closed hull polygons");
        if(h.mNbPolygons){discrete(nxHullComputeEdges(&h));discrete(nxHullComputeEdgeAxes(&h));observeHull(h);
            float pose[16]={0,1,0,0,-1,0,0,0,0,0,1,0,3,-2,1,1};
            for(const auto& xyz:directions){IceMaths::Point dir(xyz[0],xyz[1],xyz[2]);
                for(unsigned posed=0;posed<2;++posed){unsigned kind=0xcdcd5678;discrete(nxHullSupportPolygon(RECEIVER(&h),&dir,posed?pose:0));
                    discrete(nxHullSupportFace(RECEIVER(&h),&dir,posed?pose:0,&kind));discrete(kind);
                    discrete(nxHullSupportFace(RECEIVER(&h),&dir,posed?pose:0,0));}}
            // Rebuilding exercises releases and the arrays' borrowed pointer runs.
            IceMaths::Point* previousNormals=h.mEdgeNormals;
            discrete(nxHullComputePolygons(&h));discrete(nxHullComputeEdges(&h));discrete(nxHullComputeEdgeAxes(&h));observeHull(h);
            discrete(previousNormals!=h.mEdgeNormals);nxIceFree(previousNormals);
        }
        Valencies graph;VALENCESCREATE c={nb,nf,0,refs,true};discrete(graph.Compute(c));discrete(graph.mNbVerts);discrete(graph.mNbAdjVerts);
        if(graph.mAdjVerts){for(unsigned i=0;i<nb;++i){discrete(graph.mValencies[i]);discrete(graph.mOffsets[i]);}for(unsigned i=0;i<graph.mNbAdjVerts;++i)discrete(graph.mAdjVerts[i]);
            unsigned visited[10];visited[0]=0xdeadbeef;visited[9]=0xcafebabe;std::memset(visited+1,0,32);visited[nb+1]=0xcafebabe;
            unsigned stamp=1;for(const auto& xyz:directions)for(unsigned start=0;start<nb;++start){unsigned index=start;IceMaths::Point dir(xyz[0],xyz[1],xyz[2]);
                discrete(nxHullClimbSupportVertex(&index,&dir,verts,&graph,stamp++,visited+1));discrete(index);for(unsigned i=0;i<nb;++i)discrete(visited[i+1]);
                check(visited[0]==0xdeadbeef&&visited[nb+1]==0xcafebabe,"graph immediate scratch canaries");}
            discrete(visited[0]);discrete(visited[9]);}
        releaseHull(h);if(lazyNormals)nxIceFree(lazyNormals);++fixtureId;
    }
    IceMaths::Point verts[4]={{0,0,0},{2,0,0},{0,1,0},{0,0,0}},center(17,19,23);NxU16 refs[3]={0,1,2};
    numeric(3,nxHullTriangleArea(RECEIVER(refs),verts));nxHullTriangleCenter(RECEIVER(refs),verts,&center);point(2,center);
    numeric(3,nxHullTriangleArea(RECEIVER(refs),0));nxHullTriangleCenter(RECEIVER(refs),0,&center);point(2,center);
    unsigned vr[3]={0,1,2};IceMaths::Plane p;discrete(nxHullPolygonPlane(&p,3,vr,verts));point(1,p.n);numeric(2,p.d);
    vr[0]=2;vr[2]=0;discrete(nxHullPolygonPlane(&p,3,vr,verts));point(1,p.n);numeric(2,p.d);
    vr[0]=0;vr[2]=2;
    vr[1]=vr[2]=0;discrete(nxHullPolygonPlane(&p,3,vr,verts));point(1,p.n);numeric(2,p.d);
    discrete(nxHullPolygonPlane(&p,0,vr,verts));discrete(nxHullPolygonPlane(&p,3,0,verts));discrete(nxHullPolygonPlane(&p,3,vr,0));
    ConvexHull empty;std::memset(&empty,0,sizeof(empty));center.Set(17,19,23);discrete(nxHullComputeCentroid(RECEIVER(&empty),&center));point(2,center);
    reportLine=0;discrete(nxHullComputePolygons(&empty));discrete(reportLine);releaseHull(empty);
    unsigned index=2,visited[4]={0,0,0,0};IceMaths::Point dir(1,0,0);Valencies missing;
    discrete(nxHullClimbSupportVertex(&index,&dir,0,&missing,7,visited));discrete(nxHullClimbSupportVertex(&index,&dir,verts,0,7,visited));
    discrete(nxHullClimbSupportVertex(&index,&dir,verts,&missing,7,0));discrete(nxHullClimbSupportVertex(&index,&dir,verts,&missing,7,visited));discrete(index);
    // Recovered constructors have exact write sets, including untouched fields.
    HullPolygon polygon;std::memset(&polygon,0xcd,sizeof(polygon));discrete(nxHullPolygonConstruct(&polygon)==&polygon);discrete(polygon.mNbVerts);discrete(polygon.mVRefs==0);discrete(polygon.mERefs==0);numeric(4,polygon.mMin);
    HullEdge edge={17,23};discrete(nxIceIdentityConstruct(&edge)==&edge);discrete(edge.mRef0);discrete(edge.mRef1);
    EdgeDesc desc={1,2,3};discrete(nxEdgeDescConstruct(&desc)==&desc);discrete(desc.Flags);discrete(desc.Count);discrete(desc.Offset);
    HullPolygon constructed[3];std::memset(constructed,0xcd,sizeof(constructed));nxIceVectorConstruct(constructed,sizeof(HullPolygon),3,nxHullPolygonConstruct);
    for(const auto& e:constructed){discrete(e.mNbVerts);discrete(e.mVRefs==0);discrete(e.mERefs==0);numeric(4,e.mMin);}
    nxIceVectorConstruct(constructed,sizeof(HullPolygon),0,nxHullPolygonConstruct);numeric(4,constructed[2].mMax);
    {IceCore::Container words;const unsigned values[]={0x80000000,0x3f800000,0xbf800000};
        discrete(&nxIceContainerAddPoint(RECEIVER(&words),values)==&words);discrete(words.GetNbEntries());for(unsigned i=0;i<3;++i)discrete(words.GetEntries()[i]);}
    unsigned reverse[5]={1,2,3,4,5};discrete(nxIceReverseArray(5,reverse));for(unsigned v:reverse)discrete(v);discrete(nxIceReverseArray(0,reverse));discrete(nxIceReverseArray(1,0));
    {IceCore::Container axes;IceMaths::Point axis(1,0,0),opposite(-1,0,0);discrete(nxIceAddUniqueAxis(RECEIVER(&axes),&axis));discrete(nxIceAddUniqueAxis(RECEIVER(&axes),&opposite));discrete(axes.GetNbEntries());}
    for(unsigned x=0x3f7ff971;x<=0x3f7ff973;++x){IceCore::Container axes;IceMaths::Point axis(1,0,0),near(f(x),0.015f,0);
        discrete(nxIceAddUniqueAxis(RECEIVER(&axes),&axis));discrete(nxIceAddUniqueAxis(RECEIVER(&axes),&near));discrete(axes.GetNbEntries());}
    {IceCore::Container axes;IceMaths::Point axis(f(0x80000000),-1,0);discrete(nxIceAddUniqueAxis(RECEIVER(&axes),&axis));
        for(unsigned i=0;i<3;++i)discrete(axes.GetEntries()[i]);}
    {IceCore::Container faces;AdjTriangle adj[2];for(auto& a:adj)for(unsigned& w:a.ATri)w=0x20000000;
        adj[0].ATri[0]=1;adj[1].ATri[0]=0;unsigned char marks[4]={0x6a,0,0,0x7b};nxHullGatherFaces(&faces,adj,0,marks+1);
        discrete(faces.GetNbEntries());for(unsigned i=0;i<faces.GetNbEntries();++i)discrete(faces.GetEntries()[i]);for(unsigned char v:marks)discrete(v);}
    // Supported open-mesh failure and output append semantics.
    {ConvexHull h;std::memset(&h,0,sizeof(h));h.mNbFaces=1;h.mFaces=refs;h.mNbVerts=3;h.mVerts=verts;
        IceCore::Container data;data.Add(unsigned(0xdeadbeef));unsigned count=0xcafebabe;reportLine=0;
        discrete(nxHullExtractPolygons(&count,&data,&h));discrete(count);discrete(data.GetNbEntries());discrete(data.GetEntries()[0]);discrete(reportLine);}
    {ConvexHull h;std::memset(&h,0,sizeof(h));h.mNbFaces=4;h.mFaces=tetraFaces;h.mNbVerts=4;h.mVerts=verts;
        // The first production output allocation's existing false-return path.
        allocator.failNext=true;IceCore::Container data;unsigned count=0x12345678;check(!nxHullExtractPolygons(&count,&data,&h),"topology output allocation failure");check(count==0x12345678,"failed extract count canary");}
#if !NX_PHYSICS_USE_X87

    // Adjacencies succeeds first. The newly owned mark bytes fail before any
    // output write; size12 TEMP is distinct from the topology rank scratch.
    {IceMaths::Point good[8]={{0,0,0},{1,0,0},{1,1,0},{0,1,0},{0,0,1},{1,0,1},{1,1,1},{0,1,1}};ConvexHull h;std::memset(&h,0,sizeof(h));h.mNbFaces=12;h.mFaces=cubeFaces;h.mNbVerts=8;h.mVerts=good;
        allocator.failScratchSize=12;IceCore::Container data;unsigned count=0x12345678;check(!nxHullExtractPolygons(&count,&data,&h),"mark scratch failure returns false");
        check(allocator.failScratchSize==0&&count==0x12345678&&data.GetNbEntries()==0,"failed mark scratch output canaries");}
#endif
    check(allocator.blocks.empty(),"all hull allocator blocks released");nxSetSdkAllocatorBridge(0);
}
static void put(FILE* out,unsigned v){unsigned char b[4]={(unsigned char)v,(unsigned char)(v>>8),(unsigned char)(v>>16),(unsigned char)(v>>24)};std::fwrite(b,1,4,out);}
static unsigned get(const unsigned char* p){return unsigned(p[0])|(unsigned(p[1])<<8)|(unsigned(p[2])<<16)|(unsigned(p[3])<<24);}
int main(int argc,char** argv) {
    if(argc!=2)return 2;
#if NX_PHYSICS_USE_X87
    if((_controlfp(0,0)&(_MCW_PC|_MCW_RC))!=(_PC_53|_RC_NEAR))return 3;
#endif
    domain();
#if NX_PHYSICS_USE_X87
    FILE* out=std::fopen(argv[1],"wb");if(!out)return 4;std::fwrite("NXPF",1,4,out);put(out,1);put(out,(unsigned)observations.size()*20);put(out,20);
    for(const auto& o:observations){put(out,o.kind);put(out,o.id);put(out,o.index);put(out,0);put(out,o.word);}std::fclose(out);
#else
    std::vector<unsigned char> bytes;std::string error;check(nxReadFixture(argv[1],bytes,error),error.c_str());check(bytes.size()==observations.size()*20,"hull semantic record count");
    double maxima[4]={0,0,0,0},relative[4]={0,0,0,0};unsigned mismatch[4]={0,0,0,0};
    if(bytes.size()==observations.size()*20)for(unsigned i=0;i<observations.size();++i){const unsigned char* p=&bytes[i*20];const auto& o=observations[i];
        check(get(p)==o.kind&&get(p+4)==o.id&&get(p+8)==o.index&&get(p+12)==0,"semantic identity");unsigned ref=get(p+16);
        if(o.kind==0||o.kind==4){if(o.word!=ref)std::fprintf(stderr,"exact row=%u kind=%u group=%u actual=%x reference=%x\n",i,o.kind,o.id,o.word,ref);check(o.word==ref,"exact discrete/canary");}
        else{double delta=std::fabs(double(f(o.word))-double(f(ref)));if(delta>maxima[o.kind])maxima[o.kind]=delta;if(o.word!=ref)++mismatch[o.kind];
            if(f(ref)!=0){const double rel=delta/std::fabs(double(f(ref)));if(rel>relative[o.kind])relative[o.kind]=rel;}
            // Fixed controller-approved budgets pinned in hull-acceptance.json.
            // Units:1 dimensionless;2 length;3 area (length squared).
            const double absolute=o.kind==2?1e-6:2e-7;
            check(f(ref)==0?o.word==ref:nxWithinBudget(f(o.word),f(ref),absolute,2e-7),"approved hull unit budget/exact zero");}}
    check(maxUnitResidual<=2e-7,"approved valid unit-normal residual");
    std::printf("Hull groups=%u observations=%zu normal_maxabs=%.17g length_maxabs=%.17g area_maxabs=%.17g normal_diffs=%u length_diffs=%u area_diffs=%u failures=%d\n",fixtureId+1,observations.size(),maxima[1],maxima[2],maxima[3],mismatch[1],mismatch[2],mismatch[3],failures);
    std::printf("Hull normal_maxrel=%.17g length_maxrel=%.17g area_maxrel=%.17g unit_residual=%.17g\n",relative[1],relative[2],relative[3],maxUnitResidual);
#endif
    return failures?1:0;
}
