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
#include "NxSceneVisitedBuffers.h"
#include "IcePruner.h"
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

struct GuardedPrefix {
    unsigned before[4];
    alignas(NxPolygonScratch) unsigned char prefix[0x18];
    unsigned after[4];
    void poison() { std::memset(this,0xcd,sizeof(*this)); }
    void guards() { for(unsigned i=0;i<4;++i) check(before[i]==0xcdcdcdcd && after[i]==0xcdcdcdcd,"prefix guards"); }
};
static void visitedDomain(GuardAllocator& allocator)
{
    for(unsigned pass=0;pass<3;++pass) {
        const size_t baseline=allocator.blocks.size();
        GuardedPrefix storage; storage.poison();
        NxPolygonScratch* prefix=nxSceneScratchConstruct(storage.prefix,0x13579bdf);
        SdkContainer first, second, third;
        StaticPruner staticPruner;
        DynamicPruner dynamicPruner;
        struct CollectionStorage {
            unsigned before[4];
            alignas(NxScenePrunerCollection) unsigned char bytes[0x10];
            unsigned after[4];
        } collectionStorage;
        std::memset(&collectionStorage,0xcd,sizeof(collectionStorage));
        NxScenePrunerCollection& pruners=*nxScenePrunerCollectionConstruct(collectionStorage.bytes);
        for(unsigned k=0;k<4;++k) check(pruners.mPruners[k]==nullptr,"actual collection constructor clears all slots");
        pruners.mPruners[0]=&staticPruner; pruners.mPruners[2]=&dynamicPruner;
        ++group;
        check(sizeof(StaticPruner)==0x90 &&
            reinterpret_cast<unsigned char*>(&staticPruner.mTouched)-reinterpret_cast<unsigned char*>(&staticPruner)==0x40,
            "genuine static touched-container placement");
        check(sizeof(IceCore::Container)==0x10 && alignof(IceCore::Container)==4,"actual Ice container extent/alignment");
        exact(prefix->mOpaque00!=0); exact(prefix->mVisitedCount); exact(prefix->mVisited!=nullptr);
        exact(prefix->mOpaque0C); exact(prefix->mOpaque10); exact(prefix->mStamp);
        // Real previously owned receiver buffers must be released by first handoff.
        check(first.resize(3) && second.resize(2) && third.resize(4),"actual owned receiver allocations");
        staticPruner.mTouched.Add(udword(0x12345678));
        unsigned* owned[] = {first.mEntries,second.mEntries,third.mEntries,staticPruner.mTouched.GetEntries()};
        const unsigned requests[] = {0,1,255,256,257,512,513,1024,1023};
        const unsigned capacities[] = {0,256,256,256,512,512,768,1280,1280};
        for(unsigned i=0;i<9;++i) {
            ++group;
            unsigned* oldVisits=prefix->mVisited;
            unsigned* oldShared=reinterpret_cast<unsigned*>(prefix->mOpaque0C);
            const unsigned oldCapacity=prefix->mVisitedCount;
            const unsigned attempts=allocator.allocations;
            const size_t released=allocator.freed.size();
            prefix->mOpaque10=0x2468ace0; // producer must preserve mesh-shape count word
            if(oldVisits) { oldVisits[0]=0xaabbccdd; oldShared[0]=0x11223344; prefix->mStamp=0xfffffffe; }
            nxSceneVisitedBuffersUpdate(*prefix,first,second,third,pruners,requests[i]);
            exact(prefix->mVisitedCount); exact(prefix->mStamp); exact(allocator.allocations-attempts);
            check(prefix->mVisitedCount==capacities[i],"literal boundary capacity");
            check(prefix->mOpaque00==0x13579bdf && prefix->mOpaque10==0x2468ace0,"unconsumed prefix words preserved");
            if(capacities[i]>oldCapacity) {
                check(allocator.allocations-attempts==2,"two genuine persistent allocations per growth");
                if(oldVisits) {
                    check(allocator.freed.size()>=released+2,"growth released both old allocations");
                    if(allocator.freed.size()>=released+2) check(allocator.freed[released]==oldVisits && allocator.freed[released+1]==oldShared,"original visits then shared free order");
                } else if(i==1) {
                    check(allocator.freed.size()>=released+4,"owned receiver handoff releases actual allocations");
                    if(allocator.freed.size()>=released+4) for(unsigned k=0;k<4;++k) check(allocator.freed[released+k]==owned[k],"original receiver handoff order");
                }
                for(unsigned k=0;k<capacities[i];++k) check(prefix->mVisited[k]==0,"whole visited array cleared");
                check(prefix->mStamp==capacities[i],"stamp reset on growth");
                unsigned* shared=reinterpret_cast<unsigned*>(prefix->mOpaque0C);
                for(unsigned k=0;k<capacities[i];++k) check(shared[k]==0xcdcdcdcd,"shared scratch array intentionally uninitialized");
                for(SdkContainer* receiver:{&first,&second,&third}) {
                    check(receiver->mCapacity==capacities[i] && receiver->mCount==0 && receiver->mEntries==shared && receiver->mGrowthFactor==-1.0f,"genuine receivers borrow same buffer");
                    exact(receiver->mCapacity); exact(receiver->mCount); exact(bits(&receiver->mGrowthFactor)); exact(receiver->mEntries==shared);
                }
                check(staticPruner.mTouched.GetEntries()==shared && staticPruner.mTouched.GetNbEntries()==0 && staticPruner.mTouched.GetGrowthFactor()==-1.0f,"actual virtual static-pruner receiver");
                const unsigned char* touched=reinterpret_cast<const unsigned char*>(&staticPruner.mTouched);
                check(bits(touched)==capacities[i] && bits(touched+4)==staticPruner.mTouched.GetNbEntries() && bits(touched+8)==unsigned(shared) && bits(touched+12)==bits(&first.mGrowthFactor),"actual private field byte layout against public getters");

            } else {
                check(allocator.allocations==attempts && allocator.freed.size()==released,"no-op must not allocate or free");
                check(prefix->mVisited==oldVisits && prefix->mOpaque0C==unsigned(oldShared),"no-op preserves arrays");
                if(oldVisits) check(oldVisits[0]==0xaabbccdd && oldShared[0]==0x11223344 && prefix->mStamp==0xfffffffe,"reuse preserves stamp and contents");
            }
            storage.guards(); allocator.canaries();
        }
        ++group;
        unsigned* visits=prefix->mVisited; unsigned* shared=reinterpret_cast<unsigned*>(prefix->mOpaque0C);
        const size_t released=allocator.freed.size();
        nxSceneScratchReleaseBuffers(*prefix);
        check(allocator.freed.size()>=released+2,"release both genuine prefix buffers");
        if(allocator.freed.size()>=released+2) check(allocator.freed[released]==visits && allocator.freed[released+1]==shared,"original Scene buffer release order");
        check(prefix->mVisited==nullptr && prefix->mOpaque0C==0,"original release clears both pointers");
        check(prefix->mOpaque00==0x13579bdf,"prefix remains alive until Scene destructor end");
        first.empty(); second.empty(); third.empty(); staticPruner.mTouched.Empty();
        check(allocator.blocks.size()==baseline,"borrowed receivers never free prefix buffers twice");
        exact(allocator.blocks.size()-baseline); storage.guards();
        for(unsigned k=0;k<4;++k) check(collectionStorage.before[k]==0xcdcdcdcd && collectionStorage.after[k]==0xcdcdcdcd,"actual collection storage guards");
        pruners.~NxScenePrunerCollection();
        nxSceneScratchDestroy(prefix);
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
    nxSetSdkAllocatorBridge(&host); visitedDomain(allocator);
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
        check(std::memcmp(bytes.data(),observations.data(),bytes.size())==0,"exact protected original producer observations");
#endif
    std::printf("scene_visited_buffers sizeof=%zu align=%zu groups=%u observations=%zu allocations=%u releases=%u failures=%u\n",
                sizeof(NxPolygonScratch),alignof(NxPolygonScratch),group,observations.size(),allocator.allocations,allocator.releases,failures);
    return failures?1:0;
}
