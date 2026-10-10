#include <algorithm>
#include <cfenv>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "SceneVisitedGuardAllocator.h"
#include "FoundationSDK.h"
#include "FixtureSupport.h"
#include "PhysicsInternal.h"
#include "ReadWriteLockLifetime.h"
#include "NpSceneGuard.h"
static unsigned failures,group;
struct Observation { unsigned kind,group,index,reserved,word; };
static std::vector<Observation> observations;
static void check(bool value,const char* message) { if(!value) { std::fprintf(stderr,"FAIL group=%u %s\n",group,message); ++failures; } }
static void exact(unsigned value) { observations.push_back({0,group,unsigned(observations.size()),0,value}); }
#include "SceneLockDomain.h"
class ScalarAllocator : public GuardAllocator {
public:
    ScalarAllocator():GuardAllocator(check) {}
    void* links[2]={}; bool nulled[2]={};
    void free(void* p) override {
        for(unsigned i=0;i<2;++i) if(p==links[i]) nulled[i]=lockPointer(p)==nullptr;
        GuardAllocator::free(p);
    }
};
struct ScalarHost {
    ScalarAllocator& allocator; ReadWriteLock* links[2]={}; void* states[2]={};
    unsigned mainThread=0,workerThread=0,cleanup[4]={};
    void begin() {
        mainThread=GetCurrentThreadId(); workerThread=0;
        for(unsigned i=0;i<2;++i) {
            links[i]=nxSceneLockCreate(); states[i]=lockPointer(links[i]);
            allocator.links[i]=links[i]; allocator.nulled[i]=false;
        }
    }
    unsigned linkBytes(unsigned i) { return unsigned(allocator.blocks.at(links[i])); }
    unsigned blockBytes(unsigned i) { return unsigned(allocator.blocks.at(states[i])); }
    unsigned state(unsigned i,unsigned offset) { return lockWord(states[i],offset); }
    bool operation(unsigned i,unsigned op) {
        if(op==0) return links[i]->lock();
        if(op==1) return links[i]->tryLock();
        return links[i]->unlock();
    }
    void guards() { allocator.canaries(); }
    void end() {
        const size_t first=allocator.freed.size();
        nxSceneLockDestroy(links[0]); nxSceneLockDestroy(links[1]);
        auto pos=[&](void* p){ auto it=std::find(allocator.freed.begin()+first,allocator.freed.end(),p); check(it!=allocator.freed.end(),"actual scalar lock ownership released"); return it; };
        cleanup[0]=allocator.nulled[0]; cleanup[1]=allocator.nulled[1];
        cleanup[2]=pos(states[0])<pos(links[0]) && pos(states[1])<pos(links[1]);
        cleanup[3]=pos(links[0])<pos(states[1]);
        allocator.links[0]=allocator.links[1]=nullptr;
    }
};
int main(int argc,char** argv) {
    if(argc!=2) return 2; check(std::fegetround()==FE_TONEAREST,"actual scalar nearest fenv");
    ScalarAllocator allocator;
    NxFoundationSDK* foundation=NxCreateFoundationSDK(NX_FOUNDATION_SDK_VERSION,nullptr,&allocator);
    check(foundation && nxFoundationSDKAllocator==&allocator,"actual Foundation allocator identity");
    ScalarHost host{allocator}; sceneLockDomain(host);
    // Legal guard routing and ordinary embedded class lifetime supplements.
    ReadWriteLock* link=nxSceneLockCreate(); void* state=lockPointer(link);
    nxNpSceneGuardEnter(link); check(lockWord(state,0x18)==1 && lockWord(state,8)==1,"guard enter invokes actual lock");
    check(nxNpSceneGuardWriteTry(link) && lockWord(state,8)==2,"guard try invokes actual recursive method");
    nxNpSceneGuardLeave(link); nxNpSceneGuardLeave(link);
    check(lockWord(state,0x18)==0 && lockWord(state,8)==0,"guard leaves balance actual class"); nxSceneLockDestroy(link);
    { ReadWriteLock embedded; check(embedded.lock() && embedded.unlock(),"ordinary embedded class lifetime without Scene fabrication"); }
    foundation->release(); check(allocator.blocks.empty(),"all actual lock lifetimes end before Foundation release");
    std::vector<unsigned char> bytes; std::string error;
    check(nxReadFixture(argv[1],bytes,error),error.c_str());
    check(bytes.size()==observations.size()*20,"exact original observation count");
    if(bytes.size()==observations.size()*20) check(std::memcmp(bytes.data(),observations.data(),bytes.size())==0,"exact protected shipped lock observations");
    std::printf("scene_locks sizeof=%zu align=%zu groups=%u observations=%zu allocations=%u releases=%u failures=%u\n",sizeof(ReadWriteLock),alignof(ReadWriteLock),group,observations.size(),allocator.allocations,allocator.releases,failures);
    return failures?1:0;
}
