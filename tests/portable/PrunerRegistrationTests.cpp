#include <algorithm>
#include <cfenv>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <vector>
#include "SceneVisitedGuardAllocator.h"
#include "FoundationSDK.h"
#include "NxSdkAllocator.h"
#include "FixtureSupport.h"
#include "IcePruner.h"
#include "NxPrunerRegistration.h"
static unsigned failures,group;
struct Observation { unsigned kind,group,index,reserved,word; };
static std::vector<Observation> observations;
static void check(bool value,const char* message) { if(!value) { std::fprintf(stderr,"FAIL group=%u %s\n",group,message);++failures; } }
static void exact(unsigned value) { observations.push_back({0,group,unsigned(observations.size()),0,value}); }
#include "PrunerRegistrationDomain.h"
class FoundationHostAllocator : public SdkAllocator {
public:
    void* malloc(size_t size,NxMemoryType type) override { return nxFoundationSDKAllocator->malloc(size,type); }
    void* mallocDEBUG(size_t size,const char* file,int line,const char* name,NxMemoryType type) override { return nxFoundationSDKAllocator->mallocDEBUG(size,file,line,name,type); }
    void* realloc(void* pointer,size_t size) override { return nxFoundationSDKAllocator->realloc(pointer,size); }
    void free(void* pointer) override { nxFoundationSDKAllocator->free(pointer); }
};
struct ProductionHost {
    GuardAllocator& allocator;FoundationHostAllocator bridge;NxFoundationSDK* foundation=nullptr;
    void* pool() { return nxPrunerProcessPool(); }
    void begin() { foundation=NxCreateFoundationSDK(NX_FOUNDATION_SDK_VERSION,nullptr,&allocator);check(foundation!=nullptr,"genuine Foundation constructor");nxSetSdkAllocatorBridge(&bridge); }
    void* create(unsigned type) {
        Pruner* p=nullptr;const unsigned sizes[]={sizeof(StaticPruner),sizeof(BoundedDynamicPruner),sizeof(DynamicPruner)};
        void* memory=nxGetSdkAllocator()->malloc(sizes[type],NX_MEMORY_PERSISTENT);
        if(type==0) p=new(memory) StaticPruner;
        if(type==1) p=new(memory) BoundedDynamicPruner;
        if(type==2) p=new(memory) DynamicPruner;
        check(sizeof(Pruner)==0x3c && alignof(Pruner)==4 && sizeof(NxPrunerRegistration)==8 && alignof(NxPrunerRegistration)==4,"actual fixed32 Pruner and member");
        check(reinterpret_cast<unsigned char*>(&p->mRegistration)-reinterpret_cast<unsigned char*>(p)==0x34 && reinterpret_cast<unsigned char*>(&p->mRegistration.mTimestamp)-reinterpret_cast<unsigned char*>(p)==0x38,"actual ordinary member addresses");
        return p;
    }
    void destroy(void* object) { Pruner* p=static_cast<Pruner*>(object);p->~Pruner();nxGetSdkAllocator()->free(p); }
    void update(void* p) { check(static_cast<Pruner*>(p)->UpdateObject(nullptr),"actual production virtual UpdateObject unread argument"); }
    void remove(unsigned handle) { nxPrunerProcessPool()->remove(handle); }
    void guards() { allocator.canaries(); }
    void end() {
        NxPrunerProcessPool* owner=nxPrunerProcessPool();void* pointers[]={owner->mGenerations,owner->mReverse,owner->mForward,owner->mObjects,owner};
        // SDK's established order releases Foundation first; bridge's supplied allocator stays alive.
        foundation->release();foundation=nullptr;const size_t start=allocator.freed.size();nxOpcodeReleasePool();
        check(allocator.freed.size()==start+5,"exact five actual process owner frees");
        if(allocator.freed.size()==start+5) for(unsigned i=0;i<5;++i) check(allocator.freed[start+i]==pointers[i],"actual process free order generations/reverse/forward/objects/header");
        nxSetSdkAllocatorBridge(nullptr);check(allocator.blocks.empty(),"complete actual constructed owner allocator balance");
    }
};
int main(int argc,char** argv) {
    if(argc!=2) return 2;check(std::fegetround()==FE_TONEAREST,"actual scalar nearest fenv");GuardAllocator allocator(check);ProductionHost host{allocator};registryDomain(host);
    // Actual initialized registration helper boundary: no failed Pruner or
    // indeterminate handle is observed/destructed. Shipped004830's null return.
    host.begin();const unsigned baseline=allocator.allocations;allocator.failAt=baseline+1;
    struct GuardedRegistration { unsigned before[4];NxPrunerRegistration member;unsigned after[4]; } guarded;
    for(unsigned i=0;i<4;++i) guarded.before[i]=guarded.after[i]=0x13579bdfu;
    guarded.member.mHandle=0x89abcdefu;guarded.member.mTimestamp=0x12345678u;
    nxPrunerRegister(guarded.member);
    check(host.pool()==nullptr && guarded.member.mHandle==0x89abcdefu && guarded.member.mTimestamp==0,"header OOM on initialized member preserves handle and zeros timestamp");
    nxPrunerUnregister(guarded.member);
    for(unsigned i=0;i<4;++i) check(guarded.before[i]==0x13579bdfu && guarded.after[i]==0x13579bdfu,"actual initialized registration member guards");
    allocator.failAt=0;
    Pruner* live=static_cast<Pruner*>(host.create(2));
    { Prunable object;check(live->AddObject(&object),"actual constructed Prunable and embedded pool allocation");check(live->RemoveObject(&object),"actual embedded pool removal"); }
    void* boxes=live->mPool.mWorldBoxes;void* objects=live->mPool.mObjects;const size_t start=allocator.freed.size();host.destroy(live);
    check(nxPrunerProcessPool()->mCount==0 && allocator.freed.size()==start+3,"unregistered actual Pruner releases embedded pool and self");
    if(allocator.freed.size()==start+3) check(allocator.freed[start]==boxes && allocator.freed[start+1]==objects,"actual embedded box/object free order");host.end();
    std::vector<unsigned char> bytes;std::string error;check(nxReadFixture(argv[1],bytes,error),error.c_str());check(bytes.size()==observations.size()*20,"exact protected observation count");
    if(bytes.size()==observations.size()*20) check(std::memcmp(bytes.data(),observations.data(),bytes.size())==0,"exact protected shipped registration observations");
    // A changed handle word must violate the fixed discrete budget.
    if(!bytes.empty()) { bytes[16]^=1;check(std::memcmp(bytes.data(),observations.data(),bytes.size())!=0,"one-bit discrete mutation is rejected"); }
    std::printf("pruner_registration groups=%u observations=%zu attempts=%u allocation_failures=1 successful_allocations=%u releases=%u failures=%u\n",group,observations.size(),allocator.allocations,allocator.allocations-1,allocator.releases,failures);return failures?1:0;
}
