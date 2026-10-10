#include <algorithm>
#include <cfenv>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "SceneVisitedGuardAllocator.h"
#include <vector>
#include <float.h>
#include "../PhysicsPairLoader.h"
#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxUserAllocator.h"
#include "NxActorDesc.h"
#include "NxBodyDesc.h"
#include "NxBoxShapeDesc.h"
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

static unsigned readWord(const unsigned char* scene,unsigned offset) { return bits(scene+offset); }
static void put(FILE* file,unsigned word) { std::fwrite(&word,4,1,file); }
int wmain(int argc,wchar_t** argv)
{
    if(argc!=3) return 2;
    wchar_t pairDirectory[MAX_PATH]; HMODULE physics=nullptr;
    if(!nxPairDirectory(argv[1],pairDirectory)) return 2;
    physics=nxLoadPhysics(pairDirectory); if(!physics || nxReportPairIdentity(pairDirectory)) return 3;
    char hash[65]; wchar_t binary[MAX_PATH]; GetModuleFileNameW(physics,binary,MAX_PATH);
    check(nxSha256(binary,hash) && !std::strcmp(hash,"4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c"),"pinned shipped Physics identity");
    HMODULE foundation=GetModuleHandleW(L"NxFoundation.dll"); GetModuleFileNameW(foundation,binary,MAX_PATH);
    check(nxSha256(binary,hash) && !std::strcmp(hash,"7e0596e45af2f1ab937a948100528e51c80dfd02b295ba678898081883ae0990"),"pinned shipped Foundation identity");
    _control87(_PC_53|_RC_NEAR|_MCW_EM,_MCW_PC|_MCW_RC|_MCW_EM);
    unsigned short raw; __asm { fnstcw raw }
    const unsigned crt=_control87(0,0); const int fe=std::fegetround();
    check(raw==0x027f && (crt&(_MCW_PC|_MCW_RC|_MCW_EM))==(_PC_53|_RC_NEAR|_MCW_EM) && fe==FE_TONEAREST,"actual original raw/CRT/fenv state");
    std::printf("reference raw=%04x crt=%08x fenv=%d\n",raw,crt,fe);
    using Create=NxPhysicsSDK* (NX_CALL_CONV*)(NxU32,NxUserAllocator*,NxUserOutputStream*);
    using Update=void (__thiscall*)(void*,unsigned);
    Create create=reinterpret_cast<Create>(GetProcAddress(physics,"NxCreatePhysicsSDK"));
    Update update=reinterpret_cast<Update>(reinterpret_cast<unsigned char*>(physics)+0x100a0);
    GuardAllocator allocator(check); NxPhysicsSDK* sdk=create(NX_PHYSICS_SDK_VERSION,&allocator,nullptr);
    check(sdk!=nullptr,"genuine original SDK constructor"); if(!sdk) return 4;
    for(unsigned pass=0;pass<3;++pass) {
        NxSceneDesc descriptor; descriptor.setToDefault(); descriptor.gravity=NxVec3(0,0,0);
        NxScene* scene=sdk->createScene(descriptor); check(scene!=nullptr,"genuine original Scene constructor"); if(!scene) return 5;
        unsigned char* internal=*reinterpret_cast<unsigned char**>(reinterpret_cast<unsigned char*>(scene)+0x24);
        ++group;
        for(unsigned offset:{0u,4u,8u,12u,16u,20u}) exact(offset==0?readWord(internal,0)!=0:readWord(internal,offset));
        const unsigned requests[]={0,1,255,256,257,512,513,1024,1023};
        const unsigned capacities[]={0,256,256,256,512,512,768,1280,1280};
        for(unsigned i=0;i<9;++i) {
            ++group;
            const unsigned oldCapacity=readWord(internal,4); unsigned* oldVisits=reinterpret_cast<unsigned*>(readWord(internal,8));
            unsigned* oldShared=reinterpret_cast<unsigned*>(readWord(internal,12));
            const unsigned attempts=allocator.allocations; const size_t freed=allocator.freed.size();
            if(oldVisits) { oldVisits[0]=0xaabbccdd; oldShared[0]=0x11223344; const unsigned stamp=0xfffffffe; std::memcpy(internal+20,&stamp,4); }
            update(internal,requests[i]);
            const unsigned capacity=readWord(internal,4); unsigned* visits=reinterpret_cast<unsigned*>(readWord(internal,8));
            unsigned* shared=reinterpret_cast<unsigned*>(readWord(internal,12));
            exact(capacity); exact(readWord(internal,20)); exact(allocator.allocations-attempts);
            check(capacity==capacities[i],"literal original capacity boundary");
            if(capacity>oldCapacity) {
                check(allocator.allocations-attempts==2,"original two allocator calls");
                if(oldVisits) {
                    check(allocator.freed.size()>=freed+2,"original replaced both owned arrays");
                    if(allocator.freed.size()>=freed+2) check(allocator.freed[freed]==oldVisits && allocator.freed[freed+1]==oldShared,"original free visits then shared");
                }
                for(unsigned k=0;k<capacity;++k) check(visits[k]==0,"original clears complete visited buffer");
                check(readWord(internal,20)==capacity,"original resets stamp");
                for(unsigned offset:{0x50u,0x500u,0x510u}) {
                    exact(readWord(internal,offset)); exact(readWord(internal,offset+4)); exact(readWord(internal,offset+12)); exact(readWord(internal,offset+8)==unsigned(shared));
                    check(readWord(internal,offset)==capacity && readWord(internal,offset+4)==0 && readWord(internal,offset+8)==unsigned(shared) && readWord(internal,offset+12)==0xbf800000,"original three borrowed container receivers");
                }
            } else {
                check(allocator.allocations==attempts && allocator.freed.size()==freed,"original no-op ownership");
                if(oldVisits) check(visits==oldVisits && shared==oldShared && visits[0]==0xaabbccdd && shared[0]==0x11223344 && readWord(internal,20)==0xfffffffe,"original no-op preserves contents/stamp");
            }
            allocator.canaries();
        }
        ++group;
        unsigned* visits=reinterpret_cast<unsigned*>(readWord(internal,8)); unsigned* shared=reinterpret_cast<unsigned*>(readWord(internal,12));
        const size_t teardownStart=allocator.freed.size();
        sdk->releaseScene(*scene);
        check(!allocator.blocks.count(visits) && !allocator.blocks.count(shared),"original Scene destructor releases both arrays");
        auto v=std::find(allocator.freed.begin()+teardownStart,allocator.freed.end(),visits); auto s=std::find(allocator.freed.begin()+teardownStart,allocator.freed.end(),shared);
        check(v!=allocator.freed.end() && s!=allocator.freed.end() && v<s,"original Scene teardown buffer free order");
        exact(0); allocator.canaries();
    }

    // Genuine physical producers for original four-slot callback contract.
    {
        NxSceneDesc desc; desc.setToDefault(); NxScene* scene=sdk->createScene(desc);
        NxBoxShapeDesc box; NxActorDesc actorDesc; actorDesc.shapes.pushBack(&box);
        NxActor* fixed=scene->createActor(actorDesc); check(fixed!=nullptr,"original registered static shape producer");
        NxBodyDesc body; body.mass=1; actorDesc.body=&body; actorDesc.globalPose.t=NxVec3(5,0,0);
        NxActor* moving=scene->createActor(actorDesc); check(moving!=nullptr,"original registered dynamic shape producer");
        unsigned char* internal=*reinterpret_cast<unsigned char**>(reinterpret_cast<unsigned char*>(scene)+0x24);
        const unsigned requested=readWord(internal,4)+1;
        update(internal,requested);
        unsigned char* pruner=reinterpret_cast<unsigned char*>(readWord(internal,0x640));
        check(pruner!=nullptr && readWord(internal,0x648)!=0,"original genuine static/dynamic pruners");
        if(pruner) check(readWord(pruner,0x40)==readWord(internal,4) && readWord(pruner,0x44)==0 && readWord(pruner,0x48)==readWord(internal,12) && readWord(pruner,0x4c)==0xbf800000,"original nonnull slot4 receives both exact arguments");
        if(moving) scene->releaseActor(*moving); if(fixed) scene->releaseActor(*fixed);
        sdk->releaseScene(*scene); allocator.canaries();
    }
    sdk->release(); allocator.canaries();
    check(allocator.blocks.empty(),"original SDK/Scene complete allocator release");
    if(nxReportPairIdentity(pairDirectory)) return 6;
    if(failures) return 1;
    FILE* file=_wfopen(argv[2],L"wb"); if(!file) return 7;
    std::fwrite("NXPF",1,4,file); put(file,1); put(file,unsigned(observations.size())*20); put(file,20);
    for(const auto& row:observations) std::fwrite(&row,20,1,file); std::fclose(file);
    std::printf("shipped_scene_visited groups=%u observations=%zu allocations=%u releases=%u failures=%u\n",group,observations.size(),allocator.allocations,allocator.releases,failures);
    return 0;
}
