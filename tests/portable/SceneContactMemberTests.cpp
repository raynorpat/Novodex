#include <algorithm>
#include <cfenv>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <vector>
#include "FoundationSDK.h"
#include "NxSdkAllocator.h"
#include "FixtureSupport.h"
#include "NxSceneContactMembers.h"
#include "Scene.h"
#if NX_PHYSICS_USE_X87
#include <float.h>
#endif

static unsigned failures, group;
struct Observation { unsigned kind, group, index, reserved, word; };
static std::vector<Observation> observations;
static void check(bool value, const char* message)
{
    if(!value) { std::fprintf(stderr,"FAIL group=%u %s\n",group,message); ++failures; }
}
static void exact(unsigned word)
{ observations.push_back({0,group,unsigned(observations.size()),0,word}); }
static unsigned bits(const void* pointer)
{ unsigned word; std::memcpy(&word,pointer,4); return word; }
class GuardAllocator : public NxUserAllocator
{
public:
    std::map<void*,size_t> blocks;
    std::vector<void*> freed;
    unsigned allocations=0, releases=0, failAt=0;
    void* mallocDEBUG(size_t size,const char*,int) override { return malloc(size); }
    void* malloc(size_t size) override
    {
        if(++allocations==failAt) return nullptr;
        unsigned char* memory=static_cast<unsigned char*>(std::malloc(size+32));
        if(!memory) std::abort();
        std::memset(memory,0x6a,16); std::memset(memory+16,0xcd,size);
        std::memset(memory+16+size,0x7b,16); blocks[memory+16]=size;
        return memory+16;
    }
    void canaries()
    {
        for(const auto& item:blocks)
            for(unsigned i=0;i<16;++i)
            {
                const unsigned char* p=static_cast<unsigned char*>(item.first);
                check(p[int(i)-16]==0x6a && p[item.second+i]==0x7b,"allocator guards");
            }
    }
    void free(void* pointer) override
    {
        if(!pointer) return;
        check(blocks.count(pointer)==1,"release actual ownership");
        if(!blocks.count(pointer)) std::abort();
        canaries(); blocks.erase(pointer); freed.push_back(pointer); ++releases;
        std::free(static_cast<unsigned char*>(pointer)-16);
    }
    void* realloc(void* pointer,size_t size) override
    {
        if(!pointer) return malloc(size);
        void* next=malloc(size); if(!next) return nullptr;
        std::memcpy(next,pointer,std::min(size,blocks.at(pointer))); free(pointer); return next;
    }
};
class FoundationHostAllocator : public SdkAllocator
{
public:
    void* malloc(size_t size,NxMemoryType type) override { return nxFoundationSDKAllocator->malloc(size,type); }
    void* mallocDEBUG(size_t size,const char* file,int line,const char* name,NxMemoryType type) override
    { return nxFoundationSDKAllocator->mallocDEBUG(size,file,line,name,type); }
    void* realloc(void* pointer,size_t size) override { return nxFoundationSDKAllocator->realloc(pointer,size); }
    void free(void* pointer) override { nxFoundationSDKAllocator->free(pointer); }
};
struct GuardedWindow
{
    unsigned before[4];
    alignas(NxSceneContactMembers) unsigned char members[0xb0];
    unsigned after[4];
    void poison() { std::memset(this,0xcd,sizeof(*this)); }
    void checkGuards()
    { for(unsigned i=0;i<4;++i) check(before[i]==0xcdcdcdcd && after[i]==0xcdcdcdcd,"member window guards"); }
};
static void observeConstructor(NxSceneContactMembers& owner)
{
    const unsigned char* ray=reinterpret_cast<const unsigned char*>(&owner.rayCollider);
    exact(*reinterpret_cast<void* const*>(ray)!=nullptr);
    for(unsigned offset: {4u,8u,12u,0x5cu,0x60u,0x64u,0x68u,0x84u,0x88u}) exact(bits(ray+offset));
    exact(ray[0x8c]); exact(ray[0x8d]);
    for(const SdkContainer* array: {&owner.edgeAxes0,&owner.edgeAxes1})
    { exact(array->mCapacity); exact(array->mCount); exact(array->mEntries==nullptr); exact(bits(&array->mGrowthFactor)); }
    Opcode::Collider* actualBase=&owner.rayCollider;
    check(actualBase->ValidateSettings()==nullptr,"genuine RayCollider virtual receiver");
    check(bits(ray+0x84)==0x7f7fffff && ray[0x8c]==0 && ray[0x8d]==1,"original RayCollider settings");
}
static void rayQueries(NxSceneContactMembers& owner)
{
    using namespace Opcode;
    Point vertices[]={Point(0,0,0),Point(1,0,0),Point(0,1,0)};
    IndexedTriangle face; face.mVRef[0]=0; face.mVRef[1]=1; face.mVRef[2]=2;
    MeshInterface mesh; mesh.SetNbVertices(3); mesh.SetNbTriangles(1); mesh.SetPointers(&face,vertices);
    OPCODECREATE create; create.mIMesh=&mesh;
    Model model; check(model.Build(create),"actual model producer");
    CollisionFaces hits;
    auto& collider=owner.rayCollider;
    collider.SetDestination(&hits); collider.SetCulling(false); collider.SetClosestHit(true);
    collider.SetFirstContact(false); collider.SetTemporalCoherence(false);
    for(unsigned i=0;i<4;++i)
    {
        ++group;
        collider.SetMaxDist(i==2?.5f:2.0f);
        Ray ray(Point(i==1?2.0f:.25f,.25f,1.0f),Point(0,0,-1));
        const bool success=collider.Collide(ray,model,nullptr,nullptr);
        exact(success); exact(hits.GetNbFaces()); exact(collider.GetContactStatus());
        check(success && hits.GetNbFaces()==(i==1 || i==2?0u:1u),"meaningful hit/miss/range/reuse");
        for(unsigned h=0;h<hits.GetNbFaces();++h)
        {
            const CollisionFace& hit=hits.GetFaces()[h];
            exact(hit.mFaceID); exact(bits(&hit.mDistance)); exact(bits(&hit.mU)); exact(bits(&hit.mV));
            check(hit.mDistance==1.0f && hit.mU==.25f && hit.mV==.25f,"analytic literal triangle hit");
        }
    }
    collider.SetDestination(nullptr); // borrowed local destination cannot escape its lifetime
}
static void memberDomain(GuardAllocator& allocator)
{
    GuardedWindow window; window.poison();
    for(unsigned pass=0;pass<3;++pass)
    {
        ++group;
        const size_t baseline=allocator.blocks.size();
        NxSceneContactMembers* owner=nxSceneContactMembersConstruct(window.members);
        observeConstructor(*owner); window.checkGuards();
        check(reinterpret_cast<unsigned char*>(&owner->edgeAxes0)-window.members==0x90 &&
              reinterpret_cast<unsigned char*>(&owner->edgeAxes1)-window.members==0xa0,"actual member placement");
        rayQueries(*owner);
        ++group;
        for(SdkContainer* array:{&owner->edgeAxes0,&owner->edgeAxes1})
        {
            check(array->resize(3),"actual edge allocation");
            for(unsigned i=0;i<3;++i) array->mEntries[i]=0x81000000u+i;
            array->mCount=3;
            check(array->resize(4),"actual edge growth");
            exact(array->mCapacity); exact(array->mCount);
            for(unsigned i=0;i<3;++i) exact(array->mEntries[i]);
        }
        unsigned* old0=owner->edgeAxes0.mEntries; unsigned* old1=owner->edgeAxes1.mEntries;
        const unsigned freeCount=allocator.releases;
        if(pass==1)
        {
            allocator.failAt=allocator.allocations+1;
            exact(owner->edgeAxes0.resize(8)); allocator.failAt=0;
            check(owner->edgeAxes0.mEntries==old0 && owner->edgeAxes0.mCount==3,
                  "historical failed resize retains owned data");
            exact(owner->edgeAxes0.mCapacity); // capacity mutates before failed allocation
        }
        if(pass==2)
        {
            owner->edgeAxes0.setExternalBuffer(4,window.before);
            old0=nullptr; // already released; member now borrows guarded storage
        }
        const size_t freedBefore=allocator.freed.size();
        nxSceneContactMembersDestroy(owner);
        check(allocator.freed[freedBefore]==old1,"reverse owner cleanup frees second edge array first");
        if(old0) check(allocator.freed[freedBefore+1]==old0,"first edge array freed second");
        exact(allocator.releases-freeCount);
        check(allocator.blocks.size()==baseline,"complete member owner release on success/failure/borrowed state");
        allocator.canaries(); window.checkGuards();
        window.poison(); // reuse only after genuine member lifetime ended
    }
}
static void put(FILE* file,unsigned word) { std::fwrite(&word,4,1,file); }
int main(int argc,char** argv)
{
    if(argc!=2) return 2;
    check(sizeof(NxSceneInternal)==0x710,"actual Scene measured extent");
#if !NX_PHYSICS_USE_X87
    check(alignof(NxSceneInternal)==4,"actual scalar Scene member alignment");
#endif
#if NX_PHYSICS_USE_X87
    _control87(_PC_53|_RC_NEAR|_MCW_EM,_MCW_PC|_MCW_RC|_MCW_EM);
    unsigned short raw; __asm { fnstcw raw }
    const unsigned crt=_control87(0,0); const int fe=std::fegetround();
    check(raw==0x027f && (crt&(_MCW_PC|_MCW_RC|_MCW_EM))==(_PC_53|_RC_NEAR|_MCW_EM) && fe==FE_TONEAREST,
          "actual nearest53 raw/CRT/fenv proof");
    std::printf("reference raw=%04x crt=%08x fenv=%d\n",raw,crt,fe);
#else
    check(std::fegetround()==FE_TONEAREST,"actual scalar nearest fenv");
#endif
    GuardAllocator allocator; FoundationHostAllocator host;
    NxFoundationSDK* foundation=NxCreateFoundationSDK(NX_FOUNDATION_SDK_VERSION,nullptr,&allocator);
    check(foundation && nxFoundationSDKAllocator==&allocator,"genuine Foundation allocator producer");
    nxSetSdkAllocatorBridge(&host); memberDomain(allocator);
    check(allocator.blocks.size()==1,"only real Foundation owner remains");
    foundation->release(); nxSetSdkAllocatorBridge(nullptr);
    check(allocator.blocks.empty(),"complete Foundation cleanup");
#if NX_PHYSICS_USE_X87
    if(failures) return 1;
    FILE* file=std::fopen(argv[1],"wb"); if(!file) return 3;
    std::fwrite("NXPF",1,4,file); put(file,1); put(file,unsigned(observations.size())*20); put(file,20);
    for(const auto& row:observations) std::fwrite(&row,20,1,file);
    std::fclose(file);
#else
    std::vector<unsigned char> bytes; std::string error;
    check(nxReadFixture(argv[1],bytes,error),error.c_str());
    check(bytes.size()==observations.size()*20,"exact constructor/query/owner observation count");
    if(bytes.size()==observations.size()*20)
        check(std::memcmp(bytes.data(),observations.data(),bytes.size())==0,"exact protected original member observations");
#endif
    std::printf("scene_contact_members sizeof=%zu align=%zu groups=%u observations=%zu allocations=%u releases=%u failures=%u\n",
                sizeof(NxSceneContactMembers),alignof(NxSceneContactMembers),group,observations.size(),allocator.allocations,allocator.releases,failures);
    return failures?1:0;
}
