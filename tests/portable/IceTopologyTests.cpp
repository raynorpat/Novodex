#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include <string>
#include <map>
#include "FixtureSupport.h"
#include "IceAdjacencies.h"
#include "NxSdkAllocator.h"
#include "NxPhysicsBackend.h"
// The inherited VC6 loop macro is not needed in this modern test consumer.
#undef for
#if NX_PHYSICS_USE_X87
#include <float.h>
#endif

// External host boundary only: algorithms, objects and allocator accessor are
// the actual production definitions. Report rows are observed, never swallowed.
static unsigned reportLine;
static void check(bool ok,const char* reason);
bool opcNovodeXSetIceError(const char*,const char*,int line) { reportLine=line;return false; }
void* opcNovodeXAlloc(size_t n) { return nxGetSdkAllocator()->malloc(n,NX_MEMORY_PERSISTENT); }
void opcNovodeXFree(void* p) { nxGetSdkAllocator()->free(p); }
class TestAllocator : public SdkAllocator {
public:
    std::map<void*,size_t> blocks;
    bool failNext=false;
    void* malloc(size_t n,NxMemoryType) { if(failNext){failNext=false;return 0;}unsigned char* raw=(unsigned char*)std::malloc(n+32);if(!raw)return 0;
        std::memset(raw,0x6a,16);std::memset(raw+16,0xcd,n);std::memset(raw+16+n,0x7b,16);void* p=raw+16;blocks[p]=n;return p; }
    void* mallocDEBUG(size_t n,const char*,int,const char*,NxMemoryType t) { return malloc(n,t); }
    void* realloc(void* p,size_t n) { if(!p)return malloc(n,NX_MEMORY_PERSISTENT);void* q=malloc(n,NX_MEMORY_PERSISTENT);
        if(q){std::memcpy(q,p,n<blocks[p]?n:blocks[p]);free(p);}return q; }
    void free(void* p) { if(!p)return;auto found=blocks.find(p);check(found!=blocks.end(),"owned production allocation released");if(found==blocks.end())return;
        unsigned char* raw=(unsigned char*)p-16;for(unsigned i=0;i<16;++i)check(raw[i]==0x6a&&raw[16+found->second+i]==0x7b,"allocation output canaries");
        blocks.erase(found);std::free(raw); }
};
struct Observation { unsigned kind,id,index,word; };
static std::vector<Observation> observations;
static unsigned fixtureId;
static void discrete(unsigned v) { observations.push_back({0,fixtureId,(unsigned)observations.size(),v}); }
static void numeric(unsigned kind,float v) { unsigned w;std::memcpy(&w,&v,4);observations.push_back({kind,fixtureId,(unsigned)observations.size(),w}); }
static float f(unsigned w) { float v;std::memcpy(&v,&w,4);return v; }
static int failures;
static void check(bool ok,const char* reason) { if(!ok){std::fprintf(stderr,"FAIL %s\n",reason);++failures;} }
// Literal binary32 coordinates; shared-edge creases straddle the active-angle
// threshold, and each winding/retention/index-width combination is independent.
static const unsigned points[][12]={
    {0,0,0,0x3f800000,0,0,0,0x3f800000,0,0x3f800000,0x3f800000,0},
    {0,0,0,0x3f800000,0,0,0,0x3f800000,0,0x3f800000,0x3f800000,0x3dcccccc},
    {0,0,0,0x3f800000,0,0,0,0x3f800000,0,0x3f800000,0x3f800000,0x3dcccccd},
    {0,0,0,0x3f800000,0,0,0,0x3f800000,0,0x3f800000,0x3f800000,0x3e000000},
    {0,0,0,0x3f800000,0,0,0,0x3f800000,0,0x3f800000,0x3f800000,0xbe000000},
    {0,0,0,0x40000000,0,0,0,0x3f000000,0,0x40000000,0x3f000000,0x3e000000},
    {0,0,0,0x3f800000,0,0,0,0x3f800000,0,0x3f800000,0x3f800000,0x3d914cd3},
    {0,0,0,0x3f800000,0,0,0,0x3f800000,0,0x3f800000,0x3f800000,0x3d914cd4},
    {0,0,0,0x3f800000,0,0,0,0x3f800000,0,0x3f800000,0x3f800000,0x3d914cd5},
    {0,0,0,0x3f800000,0,0,0,0x3f800000,0,0x3f800000,0x3f800000,0x3d914cd6},
    {0,0,0,0x3f800000,0,0,0,0x3f800000,0,0x3f800000,0x3f800000,0x3d914cd7},
    {0x3f800000,0x40000000,0x40400000,0x40000000,0x40000000,0x40800000,0x3f800000,0x40400000,0x40a00000,0x40000000,0x40400000,0x40c00000},
    {0xbf800000,0xc0000000,0xc0400000,0x3f800000,0xc0000000,0xc0000000,0xbf800000,0xbf800000,0xbf800000,0x3f800000,0xbf800000,0x00000000},
    {0x3e800000,0xbf000000,0x3fa00000,0x3fa00000,0xbf000000,0x3fa00000,0x3e800000,0x3f000000,0x3fa00000,0x3fa00000,0x3f000000,0x3fa00000},
    {0,0,0,0,0,0,0,0,0,0,0,0}
};
static void observeEdge(EdgeList& edge) {
    discrete(edge.mNbEdges);discrete(edge.mEdgeFaces!=0);discrete(edge.mEdgeToTriangles!=0);discrete(edge.mFacesByEdges!=0);
    for(unsigned i=0;i<edge.mNbEdges;++i){discrete(edge.mEdges[i].Ref0);discrete(edge.mEdges[i].Ref1);
        if(edge.mEdgeToTriangles){const EdgeDesc& e=edge.mEdgeToTriangles[i];discrete(e.Flags);discrete(e.Count);discrete(e.Offset);
            for(unsigned j=0;j<e.Count;++j)discrete(edge.mFacesByEdges[e.Offset+j]);}}
    if(edge.mEdgeFaces)for(unsigned i=0;i<edge.mNbFaces;++i)for(unsigned j=0;j<3;++j)discrete(edge.mEdgeFaces[i].mLink[j]);
}
static void domain() {
    TestAllocator allocator;nxSetSdkAllocatorBridge(&allocator);
    fixtureId=0;
    for(const auto& words:points)for(unsigned reverse=0;reverse<2;++reverse)for(unsigned width=0;width<2;++width)for(unsigned flags=0;flags<4;++flags){
        IceMaths::Point verts[4];for(unsigned v=0;v<4;++v)verts[v].Set(f(words[3*v]),f(words[3*v+1]),f(words[3*v+2]));
        unsigned refs[6]={0,1,2,1,3,2};if(reverse)for(unsigned t=0;t<2;++t){unsigned tmp=refs[3*t];refs[3*t]=refs[3*t+2];refs[3*t+2]=tmp;}
        NxU16 shortRefs[6];for(unsigned i=0;i<6;++i)shortRefs[i]=(NxU16)refs[i];
        {
            EdgeList edge;EDGELISTCREATE c={2,width?0:refs,width?shortRefs:0,(flags&1)!=0,(flags&2)!=0,verts,123.0f};
            reportLine=0;discrete(edge.Init(c));discrete(reportLine);observeEdge(edge);
        }
        {
            Adjacencies adj;ADJACENCIESCREATE c={2,width?0:refs,width?shortRefs:0,verts,-456.0f};
            reportLine=0;discrete(adj.Init(c));discrete(reportLine);discrete(adj.ComputeNbBoundaryEdges());
            for(unsigned i=0;i<adj.mNbFaces;++i)for(unsigned j=0;j<3;++j)discrete(adj.mFaces[i].ATri[j]);
        }
        // Actual vendor numeric producers, with separate normal/distance units.
        IceMaths::Plane plane;plane.Set(verts[0],verts[1],verts[2]);
        numeric(1,plane.n.x);numeric(1,plane.n.y);numeric(1,plane.n.z);numeric(2,plane.d);
        IceMaths::Triangle triangle(verts[1],verts[3],verts[2]);IceMaths::Point normal;triangle.Normal(normal);
        numeric(1,normal.x);numeric(1,normal.y);numeric(1,normal.z);
        discrete(allocator.blocks.empty());++fixtureId;
    }
    unsigned stores[6]={0xdeadbeef,0,0,0,0,0xcafebabe};
    StoreDwords(stores+1,4,0x12345678);for(unsigned w:stores)discrete(w);
    StoreDwords(stores+1,0,0x87654321);for(unsigned w:stores)discrete(w);
    numeric(3,FastSqrt(4.0f));numeric(3,FastSqrt(1.0f));numeric(3,FastSqrt(0.25f));
    numeric(3,FCMax2(2,4));numeric(3,FCMin2(2,4));numeric(3,FCMax3(2,6,4));numeric(3,FCMin3(2,6,4));
    // Volatile bit-literal loads prevent /fp:precise folding -0 constants.
    volatile unsigned zeroWords[2]={0x00000000u,0x80000000u};
    volatile float zeros[2]={f(zeroWords[0]),f(zeroWords[1])};
    check(!std::signbit(zeros[0])&&std::signbit(zeros[1]),"literal signed-zero helper arguments");
    numeric(3,FCMax2(zeros[0],zeros[1]));numeric(3,FCMin2(zeros[0],zeros[1]));
    // Every recovered register-helper argument is explicitly exercised.
    AdjTriangle faces[3];AdjEdge edges[9];unsigned count=0;
    nxAdjacenciesAddTriangle(&count,edges,0,2,0,1,faces);discrete(count);
    for(unsigned i=0;i<count;++i){discrete(edges[i].Ref0);discrete(edges[i].Ref1);discrete(edges[i].FaceNb);}
    unsigned refs[]={2,0,1,0,2,3};ADJACENCIESCREATE c={2,refs,0,0,0.001f};
    nxAdjacenciesAddTriangle(&count,edges,1,0,2,3,faces);
    reportLine=0;discrete(nxAdjacenciesCreateDatabase(count,faces,edges,&c));discrete(reportLine);
    reportLine=0;discrete(nxAdjacenciesUpdateLink(&c,0,1,7,8,faces));discrete(reportLine);
    // Empty input is supported only at Init's existing checks, never forced
    // into CreateDatabase's unchecked first-rank access.
    {Adjacencies adj;ADJACENCIESCREATE empty={0,0,0,0,0};discrete(adj.Init(empty));discrete(adj.ComputeNbBoundaryEdges());}
    {EdgeList edge;reportLine=0;discrete(edge.CreateFacesToEdges(0,0,0));discrete(reportLine);
        reportLine=0;discrete(edge.ComputeActiveEdges(0,0,0,0,19.0f));discrete(reportLine);}
    {unsigned nonManifold[]={0,1,2,1,0,3,0,1,4};Adjacencies adj;ADJACENCIESCREATE bad={3,nonManifold,0,0,0};
        reportLine=0;discrete(adj.Init(bad));discrete(reportLine);}
    {unsigned refs[]={0,1,2,1,3,2};EdgeList edge;EDGELISTCREATE c={2,refs,0,true,true,0,77.0f};
        discrete(edge.Init(c));observeEdge(edge);discrete(edge.CreateFacesToEdges(2,refs,0));observeEdge(edge);}
    {Adjacencies adj;ADJACENCIESCREATE c={1,0,0,0,88.0f};discrete(adj.Init(c));discrete(adj.ComputeNbBoundaryEdges());
        for(unsigned j=0;j<3;++j)discrete(adj.mFaces[0].ATri[j]);}
#if !NX_PHYSICS_USE_X87
    // New compiler-independent scratch has the bool API's existing null-return
    // path, with no partial output writes and no exception escaping the caller.
    AdjTriangle untouched[3];for(auto& face:untouched)for(unsigned& w:face.ATri)w=0xcdcdcdcd;
    allocator.failNext=true;check(!nxAdjacenciesCreateDatabase(count,untouched,edges,&c),"scratch allocation failure returns false");
    for(const auto& face:untouched)for(unsigned w:face.ATri)check(w==0xcdcdcdcd,"scratch failure output canary");
#endif
    {EdgeList edge;allocator.failNext=true;check(!edge.CreateFacesToEdges(2,refs,0),"edge output allocation failure returns false");check(edge.mEdgeFaces==0,"failed edge output remains null");}
    {Adjacencies adj;allocator.failNext=true;check(!adj.Init(c),"adjacency output allocation failure returns false");check(adj.mFaces==0,"failed adjacency output remains null");}
    check(allocator.blocks.empty(),"topology releases owned allocations");
    nxSetSdkAllocatorBridge(0);check(nxGetSdkAllocator()!=0,"production fallback allocator");
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
    std::vector<unsigned char> bytes;std::string error;check(nxReadFixture(argv[1],bytes,error),error.c_str());
    check(bytes.size()==observations.size()*20,"complete topology semantic record count");
    double maxNormal=0,maxDistance=0,maxNormalResidual=0,maxRelative=0;
    for(unsigned i=0;i+2<observations.size();++i)if(observations[i].kind==1&&observations[i+1].kind==1&&observations[i+2].kind==1){
        double x=f(observations[i].word),y=f(observations[i+1].word),z=f(observations[i+2].word);double square=x*x+y*y+z*z;
        if(square!=0){double residual=std::fabs(std::sqrt(square)-1);if(residual>maxNormalResidual)maxNormalResidual=residual;}i+=2;}
    if(bytes.size()==observations.size()*20)for(unsigned i=0;i<observations.size();++i){const unsigned char* p=&bytes[i*20];const Observation& o=observations[i];
        check(get(p)==o.kind&&get(p+4)==o.id&&get(p+8)==o.index&&get(p+12)==0,"record operation and input identity");unsigned ref=get(p+16);
        if(o.kind==0)check(o.word==ref,"exact topology/discrete observation");
        else if(o.kind==3){if(o.word!=ref)std::fprintf(stderr,"helper observation=%u actual=%08x reference=%08x\n",i,o.word,ref);check(o.word==ref,"exact vendor helper bit/store/tie output");}
        else {double delta=std::fabs(double(f(o.word))-double(f(ref)));if(o.kind==1)maxNormal=delta>maxNormal?delta:maxNormal;else maxDistance=delta>maxDistance?delta:maxDistance;
            if(f(ref)!=0){double relative=delta/std::fabs(double(f(ref)));if(relative>maxRelative)maxRelative=relative;}
            // Controller-approved fixed budgets for this measured finite domain;
            // exact zero representation remains independent of numeric tolerance.
            check(f(ref)==0?o.word==ref:nxWithinBudget(f(o.word),f(ref),o.kind==1?2e-7:1e-6,2e-7),"approved normal/plane budget");}}
    check(maxNormalResidual<=2e-7,"unit-normal residual budget");
    std::printf("IceTopology fixtures=%u observations=%zu normal_maxabs=%.17g distance_maxabs=%.17g norm_residual=%.17g max_relative=%.17g failures=%d\n",fixtureId+1,observations.size(),maxNormal,maxDistance,maxNormalResidual,maxRelative,failures);
#endif
    return failures?1:0;
}
