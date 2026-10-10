#include <algorithm>
#include <cfenv>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <float.h>
#include "SceneVisitedGuardAllocator.h"
#include "../PhysicsPairLoader.h"
#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
static unsigned failures,group;
struct Observation { unsigned kind,group,index,reserved,word; };
static std::vector<Observation> observations;
static void check(bool value,const char* message)
{ if(!value) { std::fprintf(stderr,"FAIL group=%u %s\n",group,message); ++failures; } }
static void exact(unsigned value) { observations.push_back({0,group,unsigned(observations.size()),0,value}); }
#include "SceneActorIdDomain.h"
class OriginalAllocator : public GuardAllocator {
public:
    OriginalAllocator() : GuardAllocator(check) {}
    void* scene=nullptr; bool cleared=false,preserved=false;
    void free(void* p) override {
        if(p==scene) {
            const unsigned char* member=static_cast<const unsigned char*>(p)+0x6d0;
            cleared=idPointer(member,4)==nullptr && idPointer(member,8)==nullptr && idPointer(member,12)==nullptr;
            preserved=idWord(member)==0;
        }
        GuardAllocator::free(p);
    }
};
struct OriginalHost {
    unsigned char* module; OriginalAllocator& allocator;
    NxPhysicsSDK* sdk=nullptr; NxScene* scene=nullptr; unsigned char* internal=nullptr;
    bool released=false,cleared=false,preserved=false; unsigned neighbors[4];
    using CreateSDK=NxPhysicsSDK* (NX_CALL_CONV*)(NxU32,NxUserAllocator*,NxUserOutputStream*);
    using Take=unsigned (__thiscall*)(void*);
    using Give=void (__thiscall*)(void*,unsigned);
    void* pool() { return internal+0x6d0; }
    void begin() {
        sdk=reinterpret_cast<CreateSDK>(GetProcAddress(reinterpret_cast<HMODULE>(module),"NxCreatePhysicsSDK"))(NX_PHYSICS_SDK_VERSION,&allocator,nullptr);
        check(sdk!=nullptr,"genuine shipped SDK");
        NxSceneDesc d; d.setToDefault(); scene=sdk->createScene(d); check(scene!=nullptr,"genuine shipped empty Scene");
        internal=*reinterpret_cast<unsigned char**>(reinterpret_cast<unsigned char*>(scene)+0x24);
        check(scene->getNbActors()==0,"no live actors in helper domain");
        allocator.scene=internal; allocator.cleared=allocator.preserved=false;
        const unsigned offsets[]={0x6c8,0x6cc,0x6e0,0x6e4};
        for(unsigned i=0;i<4;++i) neighbors[i]=idWord(internal+offsets[i]);
    }
    unsigned take() { return reinterpret_cast<Take>(module+0x1430)(pool()); }
    void give(unsigned value) { reinterpret_cast<Give>(module+0x1b90)(pool(),value); }
    void seed(unsigned value) { std::memcpy(pool(),&value,4); }
    void guards() {
        allocator.canaries(); const unsigned offsets[]={0x6c8,0x6cc,0x6e0,0x6e4};
        for(unsigned i=0;i<4;++i) check(neighbors[i]==idWord(internal+offsets[i]),"actual Scene adjacent words preserved");
    }
    void end() {
        void* owned=idPointer(pool(),4); const size_t first=allocator.freed.size();
        sdk->releaseScene(*scene); scene=nullptr;
        auto buffer=std::find(allocator.freed.begin()+first,allocator.freed.end(),owned);
        auto outer=std::find(allocator.freed.begin()+first,allocator.freed.end(),internal);
        released=buffer!=allocator.freed.end() && outer!=allocator.freed.end() && buffer<outer;
        cleared=allocator.cleared; preserved=allocator.preserved; allocator.scene=nullptr;
        sdk->release(); sdk=nullptr; check(allocator.blocks.empty(),"genuine Scene and SDK complete allocator balance");
    }
};
int wmain(int argc,wchar_t** argv) {
    if(argc!=3) return 2; wchar_t pair[MAX_PATH]; if(!nxPairDirectory(argv[1],pair)) return 2;
    HMODULE physics=nxLoadPhysics(pair); if(!physics || nxReportPairIdentity(pair)) return 3;
    wchar_t binary[MAX_PATH]; char hash[65]; GetModuleFileNameW(physics,binary,MAX_PATH);
    check(nxSha256(binary,hash) && !std::strcmp(hash,"4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c"),"pinned shipped Physics");
    GetModuleFileNameW(GetModuleHandleW(L"NxFoundation.dll"),binary,MAX_PATH);
    check(nxSha256(binary,hash) && !std::strcmp(hash,"7e0596e45af2f1ab937a948100528e51c80dfd02b295ba678898081883ae0990"),"pinned shipped Foundation");
    _control87(_PC_53|_RC_NEAR|_MCW_EM,_MCW_PC|_MCW_RC|_MCW_EM); unsigned short raw; __asm { fnstcw raw }
    const unsigned crt=_control87(0,0); const int fe=std::fegetround();
    check(raw==0x027f && (crt&(_MCW_PC|_MCW_RC|_MCW_EM))==(_PC_53|_RC_NEAR|_MCW_EM) && fe==FE_TONEAREST,"actual raw/CRT/fenv proof");
    std::printf("reference raw=%04x crt=%08x fenv=%d\n",raw,crt,fe);
    OriginalAllocator allocator; OriginalHost host{reinterpret_cast<unsigned char*>(physics),allocator}; actorIdDomain(host);
    if(nxReportPairIdentity(pair)) return 6; if(failures) return 1;
    FILE* file=_wfopen(argv[2],L"wb"); if(!file) return 7;
    std::fwrite("NXPF",1,4,file); unsigned words[]={1,unsigned(observations.size())*20,20}; std::fwrite(words,4,3,file);
    for(const auto& row:observations) std::fwrite(&row,20,1,file); std::fclose(file);
    std::printf("shipped_scene_actor_ids groups=%u observations=%zu allocations=%u releases=%u failures=%u\n",group,observations.size(),allocator.allocations,allocator.releases,failures);
    return 0;
}
