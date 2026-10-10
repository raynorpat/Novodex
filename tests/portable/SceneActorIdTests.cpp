#include <algorithm>
#include <cfenv>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "SceneVisitedGuardAllocator.h"
#include "FoundationSDK.h"
#include "FixtureSupport.h"
#include "NxSceneActorIdPool.h"
#include "Scene.h"
static unsigned failures,group;
struct Observation { unsigned kind,group,index,reserved,word; };
static std::vector<Observation> observations;
static void check(bool value,const char* message)
{ if(!value) { std::fprintf(stderr,"FAIL group=%u %s\n",group,message); ++failures; } }
static void exact(unsigned value) { observations.push_back({0,group,unsigned(observations.size()),0,value}); }
#include "SceneActorIdDomain.h"
struct ScalarHost {
    GuardAllocator& allocator;
    unsigned before[4]; alignas(NxSceneActorIdPool) unsigned char storage[0x10]; unsigned after[4];
    NxSceneActorIdPool* member=nullptr;
    bool released=false,cleared=false,preserved=false;
    void* pool() { return member; }
    void begin() {
        std::memset(before,0x6a,sizeof(before)); std::memset(storage,0xcd,sizeof(storage)); std::memset(after,0x7b,sizeof(after));
        member=nxSceneActorIdPoolConstruct(storage);
        check(static_cast<void*>(member)==static_cast<void*>(storage),"placement returns exact member storage");
    }
    unsigned take() { return member->take(); }
    void give(unsigned value) { member->returnId(value); }
    void seed(unsigned value) { member->mNext=value; }
    void guards() {
        allocator.canaries();
        for(unsigned i=0;i<4;++i) check(before[i]==0x6a6a6a6a && after[i]==0x7b7b7b7b,"actual class placement guards");
    }
    void end() {
        void* owned=member->mBegin; unsigned freeCount=allocator.releases;
        nxSceneActorIdPoolDestroy(member); member=nullptr;
        released=allocator.releases==freeCount+1 && allocator.freed.back()==owned;
        // Read object representations from the still-owned character storage;
        // never access a typed field after its class lifetime ended.
        cleared=idWord(storage+4)==0 && idWord(storage+8)==0 && idWord(storage+12)==0;
        preserved=idWord(storage)==0; guards();
    }
};
int main(int argc,char** argv) {
    if(argc!=2) return 2;
    check(std::fegetround()==FE_TONEAREST,"actual scalar nearest fenv");
    check(sizeof(NxSceneInternal)==0x710 && alignof(NxSceneInternal)==4,"actual Scene fixed32 extent/alignment");
    GuardAllocator allocator(check);
    NxFoundationSDK* foundation=NxCreateFoundationSDK(NX_FOUNDATION_SDK_VERSION,nullptr,&allocator);
    check(foundation && nxFoundationSDKAllocator==&allocator,"actual Foundation allocator identity");
    ScalarHost host{allocator}; actorIdDomain(host);
    // A separate live-object release proves preserving nonzero next and
    // idempotent empty cleanup, then a fresh return proves reusable lifetime.
    {
        NxSceneActorIdPool member; member.mNext=0x89abcdefu; member.returnId(7);
        member.releaseBuffer();
        check(member.mNext==0x89abcdefu && !member.mBegin && !member.mEnd && !member.mCapacity,"live release preserves next and clears only pointers");
        unsigned freed=allocator.releases; member.releaseBuffer();
        check(allocator.releases==freed,"empty release has no free");
        member.returnId(0xffffffffu); check(member.take()==0xffffffffu,"same actual class lifetime reusable after release");
    }
    foundation->release(); check(allocator.blocks.empty(),"actual Foundation complete allocator balance");
    std::vector<unsigned char> bytes; std::string error;
    check(nxReadFixture(argv[1],bytes,error),error.c_str());
    check(bytes.size()==observations.size()*20,"exact original observation count");
    if(bytes.size()==observations.size()*20) check(std::memcmp(bytes.data(),observations.data(),bytes.size())==0,"exact protected shipped actor-ID observations");
    std::printf("scene_actor_ids sizeof=%zu align=%zu groups=%u observations=%zu allocations=%u releases=%u failures=%u\n",sizeof(NxSceneActorIdPool),alignof(NxSceneActorIdPool),group,observations.size(),allocator.allocations,allocator.releases,failures);
    return failures?1:0;
}
