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
static void check(bool value,const char* message) { if(!value) { std::fprintf(stderr,"FAIL group=%u %s\n",group,message); ++failures; } }
static void exact(unsigned value) { observations.push_back({0,group,unsigned(observations.size()),0,value}); }
#include "SceneLockDomain.h"
class OriginalAllocator : public GuardAllocator {
public:
    OriginalAllocator():GuardAllocator(check) {}
    void* links[2]={}; bool nulled[2]={}; void* sdk=nullptr; bool sdkNulled=false;
    void free(void* p) override {
        for(unsigned i=0;i<2;++i) if(p==links[i]) nulled[i]=lockPointer(p)==nullptr;
        if(p==sdk) sdkNulled=lockPointer(p,8)==nullptr;
        GuardAllocator::free(p);
    }
};
struct OriginalHost {
    unsigned char* module; OriginalAllocator& allocator;
    NxPhysicsSDK* sdk=nullptr; NxScene* scene=nullptr;
    void* links[2]={}; void* states[2]={}; unsigned mainThread=0,workerThread=0; unsigned cleanup[4]={};
    using CreateSDK=NxPhysicsSDK* (NX_CALL_CONV*)(NxU32,NxUserAllocator*,NxUserOutputStream*);
    using Method=bool (__thiscall*)(void*);
    void begin() {
        mainThread=GetCurrentThreadId(); workerThread=0;
        sdk=reinterpret_cast<CreateSDK>(GetProcAddress(reinterpret_cast<HMODULE>(module),"NxCreatePhysicsSDK"))(NX_PHYSICS_SDK_VERSION,&allocator,nullptr);
        check(sdk!=nullptr,"genuine shipped SDK"); NxSceneDesc desc; desc.setToDefault(); scene=sdk->createScene(desc);
        check(scene!=nullptr && scene->getNbActors()==0,"genuine shipped empty Scene producer");
        for(unsigned i=0;i<2;++i) {
            links[i]=lockPointer(scene,0xc+i*4); states[i]=lockPointer(links[i]);
            allocator.links[i]=links[i]; allocator.nulled[i]=false;
            check(allocator.blocks.at(links[i])==4 && allocator.blocks.at(states[i])==0x20,"actual original link/state allocations");
        }
        // The worker's live SDK binding is retained, and the real SDK lock is exercised.
        void* sdkLock=static_cast<unsigned char*>(static_cast<void*>(sdk))+8;
        check(reinterpret_cast<Method>(module+0x5b700)(sdkLock),"genuine SDK lock acquire");
        check(lockWord(lockPointer(sdkLock),0x1c)==mainThread,"genuine SDK lock owner");
        check(reinterpret_cast<Method>(module+0x5b790)(sdkLock),"genuine SDK lock balanced release");
        allocator.sdk=sdk; allocator.sdkNulled=false;
    }
    unsigned linkBytes(unsigned i) { return unsigned(allocator.blocks.at(links[i])); }
    unsigned blockBytes(unsigned i) { return unsigned(allocator.blocks.at(states[i])); }
    unsigned state(unsigned i,unsigned offset) { return lockWord(states[i],offset); }
    bool operation(unsigned i,unsigned op) { const unsigned rvas[]={0x5b700,0x5b730,0x5b790}; return reinterpret_cast<Method>(module+rvas[op])(links[i]); }
    void guards() { allocator.canaries(); check(lockPointer(scene,0xc)==links[0] && lockPointer(scene,0x10)==links[1],"genuine Scene attached links preserved"); }
    void end() {
        const size_t first=allocator.freed.size(); sdk->releaseScene(*scene); scene=nullptr;
        auto pos=[&](void* p){ auto it=std::find(allocator.freed.begin()+first,allocator.freed.end(),p); check(it!=allocator.freed.end(),"original lock ownership released"); return it; };
        cleanup[0]=allocator.nulled[0]; cleanup[1]=allocator.nulled[1];
        cleanup[2]=pos(states[0])<pos(links[0]) && pos(states[1])<pos(links[1]);
        cleanup[3]=pos(links[0])<pos(states[1]);
        void* sdkState=lockPointer(static_cast<unsigned char*>(static_cast<void*>(sdk))+8);
        sdk->release(); check(allocator.sdkNulled,"SDK lock nulled before actual SDK wrapper release");
        check(pos(sdkState)<pos(allocator.sdk),"actual SDK state freed before wrapper"); allocator.sdk=nullptr; sdk=nullptr;
        allocator.links[0]=allocator.links[1]=nullptr;
        check(allocator.blocks.empty(),"supplied allocator spans complete original Scene/SDK teardown");
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
    unsigned crt=_control87(0,0); int fe=std::fegetround();
    check(raw==0x027f && (crt&(_MCW_PC|_MCW_RC|_MCW_EM))==(_PC_53|_RC_NEAR|_MCW_EM) && fe==FE_TONEAREST,"actual raw/CRT/fenv proof");
    std::printf("reference raw=%04x crt=%08x fenv=%d\n",raw,crt,fe);
    OriginalAllocator allocator; OriginalHost host{reinterpret_cast<unsigned char*>(physics),allocator}; sceneLockDomain(host);
    if(nxReportPairIdentity(pair)) return 6; if(failures) return 1;
    FILE* file=_wfopen(argv[2],L"wb"); if(!file) return 7;
    std::fwrite("NXPF",1,4,file); unsigned words[]={1,unsigned(observations.size())*20,20}; std::fwrite(words,4,3,file);
    for(const auto& row:observations) std::fwrite(&row,20,1,file); std::fclose(file);
    std::printf("shipped_scene_locks groups=%u observations=%zu allocations=%u releases=%u failures=%u\n",group,observations.size(),allocator.allocations,allocator.releases,failures);
    return 0;
}
