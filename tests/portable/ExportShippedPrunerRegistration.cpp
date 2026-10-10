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
static unsigned failures,group;
struct Observation { unsigned kind,group,index,reserved,word; };
static std::vector<Observation> observations;
static void check(bool value,const char* message) { if(!value) { std::fprintf(stderr,"FAIL group=%u %s\n",group,message);++failures; } }
static void exact(unsigned value) { observations.push_back({0,group,unsigned(observations.size()),0,value}); }
#include "PrunerRegistrationDomain.h"
struct OriginalHost {
    unsigned char* module; GuardAllocator& allocator;NxPhysicsSDK* sdk=nullptr;
    using CreateSDK=NxPhysicsSDK* (NX_CALL_CONV*)(NxU32,NxUserAllocator*,NxUserOutputStream*);
    using Factory=void* (__stdcall*)(unsigned);
    using Delete=void* (__thiscall*)(void*,unsigned);
    using Update=bool (__thiscall*)(void*,void*);
    using Remove=void (__thiscall*)(void*,unsigned);
    void* pool() { return registryPointer(module,0x12846c); }
    void begin() { sdk=reinterpret_cast<CreateSDK>(GetProcAddress(reinterpret_cast<HMODULE>(module),"NxCreatePhysicsSDK"))(NX_PHYSICS_SDK_VERSION,&allocator,nullptr);check(sdk!=nullptr,"genuine shipped SDK constructor"); }
    void* create(unsigned type) { return reinterpret_cast<Factory>(module+0xb5090)(type); }
    void destroy(void* pruner) { void** table=*static_cast<void***>(pruner);reinterpret_cast<Delete>(table[0])(pruner,1); }
    void update(void* pruner) { void** table=*static_cast<void***>(pruner);check(reinterpret_cast<Update>(table[3])(pruner,nullptr),"actual original virtual UpdateObject unread argument"); }
    void remove(unsigned handle) { reinterpret_cast<Remove>(module+0xef5c0)(pool(),handle); }
    void guards() { allocator.canaries(); }
    void end() {
        void* owner=pool();void* pointers[]={registryPointer(owner,20),registryPointer(owner,16),registryPointer(owner,12),registryPointer(owner,0),owner};
        const size_t start=allocator.freed.size();sdk->release();sdk=nullptr;
        size_t previous=start;
        for(unsigned i=0;i<5;++i) { auto at=std::find(allocator.freed.begin()+previous,allocator.freed.end(),pointers[i]);check(at!=allocator.freed.end(),"original actual process free order generations/reverse/forward/objects/header");if(at!=allocator.freed.end()) previous=size_t(at-allocator.freed.begin())+1; }
        check(allocator.blocks.empty(),"original SDK release balances installed live allocator");
    }
};
int wmain(int argc,wchar_t** argv) {
    if(argc!=3) return 2; wchar_t pair[MAX_PATH];if(!nxPairDirectory(argv[1],pair)) return 2;
    HMODULE physics=nxLoadPhysics(pair);if(!physics || nxReportPairIdentity(pair)) return 3;
    wchar_t binary[MAX_PATH];char hash[65];GetModuleFileNameW(physics,binary,MAX_PATH);
    check(nxSha256(binary,hash) && !std::strcmp(hash,"4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c"),"pinned shipped Physics");
    GetModuleFileNameW(GetModuleHandleW(L"NxFoundation.dll"),binary,MAX_PATH);
    check(nxSha256(binary,hash) && !std::strcmp(hash,"7e0596e45af2f1ab937a948100528e51c80dfd02b295ba678898081883ae0990"),"pinned shipped Foundation");
    _control87(_PC_53|_RC_NEAR|_MCW_EM,_MCW_PC|_MCW_RC|_MCW_EM);unsigned short raw;__asm { fnstcw raw }
    const unsigned crt=_control87(0,0);const int fe=std::fegetround();check(raw==0x027f && (crt&(_MCW_PC|_MCW_RC|_MCW_EM))==(_PC_53|_RC_NEAR|_MCW_EM) && fe==FE_TONEAREST,"actual raw/CRT/fenv proof");
    std::printf("reference raw=%04x crt=%08x fenv=%d\n",raw,crt,fe);
    GuardAllocator allocator(check);OriginalHost host{reinterpret_cast<unsigned char*>(physics),allocator};registryDomain(host);
    if(nxReportPairIdentity(pair)) return 6;if(failures) return 1;
    FILE* file=_wfopen(argv[2],L"wb");if(!file) return 7;std::fwrite("NXPF",1,4,file);unsigned words[]={1,unsigned(observations.size())*20,20};std::fwrite(words,4,3,file);for(const auto& row:observations) std::fwrite(&row,20,1,file);std::fclose(file);
    std::printf("shipped_pruner_registration groups=%u observations=%zu allocations=%u releases=%u failures=%u\n",group,observations.size(),allocator.allocations,allocator.releases,failures);return 0;
}
