#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <new>
#include <string>
#include <vector>
#include <cfenv>
#include <float.h>
#include "ContactGeneration.h"
#include "NxPlane.h"
#include "NxFoundationSDK.h"
#include "NxUserOutputStream.h"
#include "ContactPairManager.h"
#include "Containers.h"
#include "NxSdkAllocator.h"
#include "FixtureSupport.h"
#include "ContactStreamLifecycle.inl"
#include "ContactStreamRelease.inl"

struct Observation { unsigned kind, group, index, reserved, word; };
static std::vector<Observation> observations;
static unsigned failures, group;
static void check(bool ok, const char* why) {
    if(!ok) { std::fprintf(stderr,"FAIL group=%u %s\n",group,why); ++failures; }
}
static unsigned bits(float value) { unsigned out; std::memcpy(&out,&value,4); return out; }
static float number(unsigned word) { float out; std::memcpy(&out,&word,4); return out; }
static void exact(unsigned word) { observations.push_back({0,group,unsigned(observations.size()),0,word}); }
static void numeric(float value, unsigned kind) { observations.push_back({kind,group,unsigned(observations.size()),0,bits(value)}); }

class GuardAllocator : public SdkAllocator {
public:
    std::map<void*,size_t> blocks;
    unsigned allocs=0, frees=0, attempts=0, failAt=0;
    void* malloc(size_t size,NxMemoryType type) override {
        if(++attempts==failAt) {check(type==NX_MEMORY_TEMP,"injected only scratch allocation");return nullptr;}
        unsigned char* p=static_cast<unsigned char*>(std::malloc(size+32));
        if(!p) std::abort();
        std::memset(p,0x6a,16); std::memset(p+16,0xcd,size); std::memset(p+16+size,0x7b,16);
        blocks[p+16]=size; ++allocs; return p+16;
    }
    void* mallocDEBUG(size_t s,const char*,int,const char*,NxMemoryType t) override { return malloc(s,t); }
    void* realloc(void*,size_t) override { std::abort(); }
    void guard() {
        for(const auto& block:blocks) {
            auto p=static_cast<const unsigned char*>(block.first);
            for(unsigned i=0;i<16;++i) check(p[int(i)-16]==0x6a && p[block.second+i]==0x7b,"allocator canaries");
        }
    }
    void free(void* p) override {
        guard(); auto it=blocks.find(p); check(it!=blocks.end(),"sole allocator ownership");
        if(it==blocks.end()) return;
        blocks.erase(it); ++frees; std::free(static_cast<unsigned char*>(p)-16);
    }
};

// These are measured borrowed emitter records, not physical shapes/bodies or vtables.
// Emitter only reads shape+04/+9c/+de, owner+08 and holder+240; object+08.
struct Holder { unsigned char bytes[0x240]; unsigned material; };
struct Owner { unsigned words[2]; Holder* holder; };
struct Object { unsigned words[2]; NxCollisionShape* shape; };
static_assert(offsetof(Owner,holder)==8,"emitter owner holder");
static_assert(offsetof(Object,shape)==8,"emitter collision shape");
struct World {
    unsigned head[4]; NxActorPair pair; unsigned tail[4];
    Holder holder[2]; Owner owner[2]; NxCollisionShape shape[2]; Object object[2];
    World() {
        std::memset(this,0xcd,sizeof(*this));
        for(unsigned i=0;i<4;++i) head[i]=0x6a6a6a6a,tail[i]=0x7b7b7b7b;
        for(unsigned i=0;i<2;++i) {
            holder[i].material=i+2; owner[i].holder=&holder[i];
            shape[i].owner=&owner[i]; shape[i].collisionObject=&object[i];
            shape[i].tail[0xde-0xd4]=0; object[i].shape=&shape[i];
        }
        // Original shared 002356 constructs the real Container and reserves counter.
        cpmOpen002356(pair.mBytes+0x10);
        pair.at<void*>(8)=owner[1].holder;
    }
    ~World() { nxContainerAddThunk(pair.mBytes+0x10); }
    NxContactSink* sink() { return reinterpret_cast<NxContactSink*>(&pair); }
    SdkContainer& stream() { return *reinterpret_cast<SdkContainer*>(pair.mBytes+0x38); }
    void reset() { cpmOpen002354(pair.mBytes+0x10); }
    void guard() {
        for(unsigned i=0;i<4;++i) check(head[i]==0x6a6a6a6a && tail[i]==0x7b7b7b7b,"pair canaries");
        check(stream().mGrowthFactor==2.0f,"genuine owned growth factor");
    }
    void record() {
        guard(); exact(pair.at<unsigned>(0x10)); exact(stream().mCount);
        unsigned pairs=stream().mEntries[0]; exact(pairs);
        unsigned at=1;
        for(unsigned p=0;p<pairs;++p) {
            check(at+3<=stream().mCount,"pair stream structure");
            for(unsigned objectWord=0;objectWord<2;++objectWord) {
                unsigned word=stream().mEntries[at++];
                check(word==reinterpret_cast<unsigned>(&object[0]) || word==reinterpret_cast<unsigned>(&object[1]),"real object identities");
                exact(word==reinterpret_cast<unsigned>(&object[0])?0:1);
            }
            unsigned header=stream().mEntries[at++]; exact(header);
            for(unsigned n=0;n<(header&0xffff);++n) {
                for(unsigned k=0;k<3;++k) numeric(number(stream().mEntries[at++]),2);
                unsigned contacts=stream().mEntries[at++]; exact(contacts);
                for(unsigned c=0;c<contacts;++c) {
                    for(unsigned k=0;k<3;++k) numeric(number(stream().mEntries[at++]),1);
                    unsigned separation=stream().mEntries[at++]; numeric(number(separation&0x7fffffff),3); exact(separation>>31);
                    if(header&0x10000) exact(stream().mEntries[at++]);
                    if(header&0x40000) {
                        exact(stream().mEntries[at++]);
                        if(separation>>31) exact(stream().mEntries[at++]);
                    }
                }
            }
        }
        check(at==stream().mCount,"complete stream consumption");
        exact(pair.at<unsigned>(0x14)); exact(pair.at<unsigned>(0x18)); exact(pair.at<unsigned>(0x1c));
        exact(pair.at<unsigned>(0x34));
    }
};

static unsigned contains(unsigned count,const NxVec3* vertices,float x,float y) {
#if NX_PHYSICS_USE_X87
    unsigned out;
    __asm {
        push y
        push x
        mov eax,count
        mov ecx,vertices
        call nxPolygonContainsPoint
        add esp,8
        mov out,eax
    }
    return out;
#else
    return nxPolygonContainsPoint(count,vertices,x,y);
#endif
}
static unsigned clip(const NxVec3* a,const float* plane,NxVec3* point,const NxVec3* displacement,
    const NxVec3* b,const NxVec3* b1,const NxVec3* normal,const NxVec3* a1,float* parameter) {
#if NX_PHYSICS_USE_X87
    unsigned out;
    __asm {
        push ebx
        push esi
        push parameter
        push a1
        push normal
        push b1
        push b
        mov edx,a
        mov ecx,plane
        mov esi,point
        mov ebx,displacement
        call nxClipEdgeToPolygonPlane
        add esp,0x14
        pop esi
        pop ebx
        mov out,eax
    }
    return out;
#else
    return nxClipEdgeToPolygonPlane(a,plane,point,displacement,b,b1,normal,a1,parameter);
#endif
}
using PolygonFn=void(__cdecl*)(unsigned,const NxVec3*,const unsigned*,const float*,const NxPlane*,
    unsigned,const NxVec3*,const unsigned*,const float*,const NxPlane*,const NxVec3*,const float*,
    const float*,const NxCollisionShape*,const NxCollisionShape*,NxContactSink*,unsigned,unsigned,
    unsigned,unsigned,unsigned,unsigned);
static const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
static const NxVec3 polygons[][6]={
    {{-1,-1,0},{1,-1,0},{1,1,0},{-1,1,0},{0,0,0},{0,0,0}},
    {{-1,-1,0},{1,-1,0},{0,1,0},{0,0,0},{0,0,0},{0,0,0}},
    {{-2,0,0},{-1,-1,0},{1,-1,0},{2,0,0},{0,2,0},{0,0,0}},
    {{-2,0,0},{-1,-1,0},{1,-1,0},{2,0,0},{1,1,0},{-1,1,0}},
    {{-0.8f,-1,0.6f},{0.8f,-1,-0.6f},{0.8f,1,-0.6f},{-0.8f,1,0.6f},{0,0,0},{0,0,0}}
};
static const unsigned counts[]={4,3,5,6,4};
static const NxVec3 normals[]={{0,0,1},{0,0,1},{0,0,1},{0,0,1},{0.6f,0,0.8f}};
static const unsigned forward[]={0,1,2,3,4,5};
static const unsigned backward[][6]={{3,2,1,0,0,0},{2,1,0,0,0,0},{4,3,2,1,0,0},{5,4,3,2,1,0},{3,2,1,0,0,0}};
static const float translations[][3]={{0,0,-0.25f},{0,0,0},{0,0,0.25f},{0.5f,0.25f,-0.5f},
    {2,0,-0.25f},{4,0,-0.25f},{-0.25f,0.5f,-0.125f}};

static void domain(GuardAllocator& allocator) {
    {
        World w; const NxVec3 point(0.125f,-0.5f,2), normal(0.6f,0,0.8f);
        const unsigned ids[]={0,7,0xffff}, features[]={0,7,0xffff,0x10000,0x12345678};
        for(unsigned orientation=0;orientation<2;++orientation)
            for(unsigned flags=0;flags<4;++flags)
                for(unsigned id:ids) for(unsigned feature:features) {
                    ++group; w.reset(); w.pair.at<void*>(8)=w.owner[orientation].holder;
                    w.shape[0].tail[0xde-0xd4]=(flags&1)?0x20:0;
                    w.shape[1].tail[0xde-0xd4]=(flags&2)?0x20:0;
                    for(unsigned repeat=0;repeat<3;++repeat)
                        NxEmitContactFeatures(w.sink(),0x1234,&w.object[1],&w.object[0],bits(-0.25f),&point,&normal,
                            id,9,feature,feature^0xffff);
                    w.record(); allocator.guard();
                }
    }
    check(allocator.blocks.empty(),"stream release after repeated reuse");
    static const float points[][2]={{0,0},{1,1},{1,0},{-1,0},{0,-1},{1.00000011920928955078125f,0},{2,0},{-0.5f,0.25f}};
    for(unsigned p=0;p<5;++p) for(unsigned winding=0;winding<2;++winding) {
        NxVec3 vertices[6];
        for(unsigned i=0;i<counts[p];++i) vertices[i]=polygons[p][winding?backward[p][i]:i];
        for(const auto& point:points) { ++group; exact(contains(counts[p],vertices,point[0],point[1])); }
    }
    // All nine clip arguments and partial-failure writes are captured directly.
    const NxVec3 starts[]={{-1,0,0},{0,0,0},{1,0,0},{-1,0.25f,0},{-1,2,0}};
    const NxVec3 ends[]={{1,0,0},{1,0,0},{2,0,0},{1,0.25f,0},{1,2,0}};
    const NxVec3 b(0,-1,-0.25f),b1(0,1,-0.25f),n(0,0,1),axis(0,0,0.25f);
    const float plane[]={1,0,0,0};
    for(unsigned i=0;i<5;++i) for(unsigned reuse=0;reuse<2;++reuse) {
        ++group; struct Output { unsigned head; NxVec3 point; float t; unsigned tail; } out;
        out.head=0x6a6a6a6a;out.tail=0x7b7b7b7b; out.point.set(17,18,19);out.t=23;
        exact(clip(&starts[i],plane,&out.point,&axis,&b,&b1,&n,&ends[i],&out.t));
        for(unsigned k=0;k<3;++k) numeric((&out.point.x)[k],1); numeric(out.t,3);
        exact(out.head);exact(out.tail);
    }
    for(unsigned p=0;p<5;++p) for(unsigned q=0;q<5;++q)
        for(unsigned winding=0;winding<2;++winding) for(const auto& t:translations)
            for(unsigned order=0;order<2;++order) {
                ++group; World w;
                float pose1[16],ab[16],ba[16];
                std::memcpy(pose1,identity,sizeof(identity)); std::memcpy(ab,identity,sizeof(identity));std::memcpy(ba,identity,sizeof(identity));
                for(unsigned i=0;i<3;++i) pose1[12+i]=ba[12+i]=t[i],ab[12+i]=-t[i];
                const NxVec3 displacement(0,0,0.25f),flipped(0,0,-0.25f);
                const unsigned* r0=winding?backward[p]:forward; const unsigned* r1=winding?backward[q]:forward;
                NxPlane n0(normals[p],0),n1(normals[q],0); if(winding) n0.normal=-n0.normal,n1.normal=-n1.normal;
                auto fn=reinterpret_cast<PolygonFn>(&NxConvexPolygonContacts);
                for(unsigned repeat=0;repeat<2;++repeat) {
                    w.reset();
                    if(!order) fn(counts[p],polygons[p],r0,identity,&n0,counts[q],polygons[q],r1,pose1,&n1,&displacement,ab,ba,
                        &w.shape[0],&w.shape[1],w.sink(),repeat?0x01020304:0xdeadbeef,repeat?0x50607080:0xaabbccdd,7,9,0x12345,0x54321);
                    else fn(counts[q],polygons[q],r1,pose1,&n1,counts[p],polygons[p],r0,identity,&n0,&flipped,ba,ab,
                        &w.shape[1],&w.shape[0],w.sink(),repeat?0xfedcba98:0x01234567,repeat?0x76543210:0x89abcdef,9,7,0x54321,0x12345);
                    w.record(); allocator.guard();
                }
            }
    check(allocator.blocks.empty(),"all real streams released");
    // Independent successful clip, endpoint partial failure and zero parameter.
    const NxVec3 edgeDirections[]={{0,2,0},{0,4,0},{0,2,0.5f}};
    for(const NxVec3& direction:edgeDirections) for(unsigned i=0;i<5;++i) {
        ++group; NxVec3 point(17,18,19);float parameter=23;
        unsigned hit=clip(&starts[i],plane,&point,&direction,&b,&b1,&n,&ends[i],&parameter);
        exact(hit); for(unsigned k=0;k<3;++k)numeric((&point.x)[k],1);numeric(parameter,3);
        if(i==0 && direction.z==0) check(hit==1 && point.x==0 && point.y==0 && point.z==-0.25f && parameter==0.25f,"analytic successful edge clip");
    }
    // Literal nonuniform vertex geometry, nonzero local plane d, two real rigid
    // poses and inverse-relative producers. Each order varies both unread words.
    const NxVec3 offsetPolygon[]={{-4,-0.125f,2},{4,-0.125f,2},{4,0.125f,2},{-4,0.125f,2}};
    const NxVec3 otherPolygon[]={{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}};
    const float rigid[][16]={{0,1,0,0,-1,0,0,0,0,0,1,0,0.5f,0.25f,-0.5f,1},
        {0.6f,0.8f,0,0,-0.8f,0.6f,0,0,0,0,1,0,0,0,-0.5f,1}};
    const float inverseRigid[][16]={{0,-1,0,0,1,0,0,0,0,0,1,0,-0.25f,0.5f,0.5f,1},
        {0.6f,-0.8f,0,0,0.8f,0.6f,0,0,0,0,1,0,0,0,0.5f,1}};
    for(unsigned pose=0;pose<2;++pose)for(unsigned winding=0;winding<2;++winding)for(unsigned order=0;order<2;++order) {
        ++group; World w;
        const NxPlane p0(0,0,winding?-1.0f:1.0f,winding?2.0f:-2.0f),p1(0,0,winding?-1.0f:1.0f,winding?1.0f:-1.0f);
        const unsigned* refs=winding?backward[0]:forward;
        const NxVec3 displacement(0,0,0.5f),flipped(0,0,-0.5f);
        auto fn=reinterpret_cast<PolygonFn>(&NxConvexPolygonContacts);
        if(!order) fn(4,offsetPolygon,refs,identity,&p0,4,otherPolygon,refs,rigid[pose],&p1,&displacement,inverseRigid[pose],rigid[pose],
            &w.shape[0],&w.shape[1],w.sink(),0x11223344,0x55667788,7,9,17,23);
        else fn(4,otherPolygon,refs,rigid[pose],&p1,4,offsetPolygon,refs,identity,&p0,&flipped,rigid[pose],inverseRigid[pose],
            &w.shape[1],&w.shape[0],w.sink(),0x99aabbcc,0xddeeff00,9,7,23,17);
        w.record();allocator.guard();
    }
    check(allocator.blocks.empty(),"supplement streams released");
}

static bool exactFinite(unsigned actual,unsigned reference) {return actual==reference;}
static bool exactPartial(unsigned actual,unsigned reference) {return reference==0xff800000 && actual==reference;}
#if !NX_PHYSICS_USE_X87
class ErrorStream : public NxUserOutputStream {
public:
    unsigned errors=0;NxErrorCode code=NXE_NO_ERROR;std::string message,file;int line=0;
    void reportError(NxErrorCode c,const char* m,const char* f,int l) override {++errors;code=c;message=m;file=f;line=l;}
    NxAssertResponse reportAssertViolation(const char*,const char*,int) override {std::abort();}
    void print(const char*) override {std::abort();}
};
static void scratchFailureDomain(GuardAllocator& allocator) {
    ErrorStream errors;
    NxFoundationSDK* foundation=NxCreateFoundationSDK(NX_FOUNDATION_SDK_VERSION,&errors,nullptr);
    check(foundation!=nullptr,"genuine Foundation error owner");
    if(!foundation)return;
    const NxPlane plane0(0,0,1,0),plane1(0,0,1,0); const NxVec3 displacement(0,0,0.25f);
    auto fn=reinterpret_cast<PolygonFn>(&NxConvexPolygonContacts);
    for(unsigned failure=1;failure<=3;++failure) {
        World w;unsigned char before[sizeof(NxActorPair)];std::memcpy(before,&w.pair,sizeof(before));
        const unsigned count=w.stream().mCount;std::vector<unsigned> stream(w.stream().mEntries,w.stream().mEntries+count);
        const size_t live=allocator.blocks.size();const unsigned previousErrors=errors.errors;
        allocator.failAt=failure<=2?allocator.attempts+failure:0;
        fn(failure==3?0xffffffffu:4,polygons[0],forward,identity,&plane0,4,polygons[0],forward,identity,&plane1,&displacement,
            identity,identity,&w.shape[0],&w.shape[1],w.sink(),0x1122,0x3344,7,9,17,23);
        allocator.failAt=0;
        check(errors.errors==previousErrors+1 && errors.code==NXE_OUT_OF_MEMORY &&
            errors.message=="Convex polygon contact scratch allocation failed" &&
            errors.file.find("ContactPolygonScalar.inl")!=std::string::npos && errors.line>0,"actual complete Foundation error payload/code");
        check(std::memcmp(before,&w.pair,sizeof(before))==0 && count==w.stream().mCount &&
            std::equal(stream.begin(),stream.end(),w.stream().mEntries),"failure leaves complete sink/stream unchanged");
        check(allocator.blocks.size()==live,"successful scratch released on second failure/overflow");allocator.guard();
    }
    foundation->release();
    check(allocator.blocks.empty(),"failure streams released");
}
#endif

int main(int argc,char** argv) {
    if(argc!=2 && argc!=3) return 2;
#if NX_PHYSICS_USE_X87
    _control87(_PC_53|_RC_NEAR|_MCW_EM,_MCW_PC|_MCW_RC|_MCW_EM);
    unsigned short raw; __asm { fnstcw raw }
    unsigned crt=_control87(0,0); int fe=std::fegetround();
    std::printf("raw_control=%04x crt_control=%08x fe_round=%d\n",raw,crt,fe);
    check(raw==0x027f && (crt&(_MCW_PC|_MCW_RC|_MCW_EM))==(_PC_53|_RC_NEAR|_MCW_EM) && fe==FE_TONEAREST,"real reference nearest53 control");
#else
    check(std::fegetround()==FE_TONEAREST,"scalar nearest environment");
#endif
    GuardAllocator allocator; nxSetSdkAllocatorBridge(&allocator); domain(allocator);
#if !NX_PHYSICS_USE_X87
    scratchFailureDomain(allocator);
#endif
    check(exactFinite(0x3f800000,0x3f800000) && !exactFinite(0x3f800001,0x3f800000) &&
        !exactFinite(0x80000000,0) && !exactFinite(0xbf800000,0x3f800000),"negative finite budget/zero/sign controls");
    check(exactPartial(0xff800000,0xff800000) && !exactPartial(0x7f800000,0xff800000) &&
        !exactPartial(0x7fc00000,0xff800000) && !exactPartial(0xbf800000,0xff800000),"negative infinity/NaN/finite replacement controls");
    check(allocator.allocs==allocator.frees,"allocation/free balance"); nxSetSdkAllocatorBridge(nullptr);
#if NX_PHYSICS_USE_X87
    FILE* f=std::fopen(argv[1],"wb"); if(!f)return 2;
    const unsigned header[]={0x4650584e,1,unsigned(observations.size()*20),20};
    std::fwrite(header,16,1,f);std::fwrite(observations.data(),20,observations.size(),f);std::fclose(f);
#else
    if(argc==3) {
        FILE* f=std::fopen(argv[2],"wb"); if(!f)return 2;
        const unsigned header[]={0x4650584e,1,unsigned(observations.size()*20),20};
        std::fwrite(header,16,1,f);std::fwrite(observations.data(),20,observations.size(),f);std::fclose(f);
    }
    std::vector<unsigned char> data;std::string error;
    if(!nxReadFixture(argv[1],data,error)) {std::fprintf(stderr,"%s\n",error.c_str());return 2;}
    check(data.size()==observations.size()*20,"protected observation count");
    unsigned discrete=0,differences[5]={},totals[5]={},nonfinite=0;double absolute[5]={},relative[5]={},minimum[5]={},maximum[5]={};
    for(unsigned i=0;i<std::min(data.size()/20,observations.size());++i) {
        Observation ref;std::memcpy(&ref,&data[i*20],20);const auto& a=observations[i];
        check(ref.kind==a.kind && ref.group==a.group && ref.index==i && !ref.reserved,"protected fixture identities");
        if(a.kind) {
            const unsigned quantity=(a.kind==2 && a.group>210)?4:a.kind;
            double av=number(a.word),rv=number(ref.word);
            if(!std::isfinite(rv)) {
                ++nonfinite; check(exactPartial(a.word,ref.word) && ref.kind==3 && ref.group>=201 && ref.group<=210 && ref.word==0xff800000,"exact original negative-infinity partial failure");
                continue;
            }
            check(std::isfinite(av),"finite measured outputs");
            check(exactFinite(a.word,ref.word),"controller approved exact finite quantity budget");
            if(!totals[quantity]) minimum[quantity]=maximum[quantity]=rv;
            minimum[quantity]=std::min(minimum[quantity],rv); maximum[quantity]=std::max(maximum[quantity],rv);
            ++totals[quantity];
            if(a.word!=ref.word) {
                ++differences[quantity];absolute[quantity]=std::max(absolute[quantity],std::fabs(av-rv));
                if(rv)relative[quantity]=std::max(relative[quantity],std::fabs((av-rv)/rv));
                if(differences[quantity]<10)std::fprintf(stderr,"difference group=%u row=%u kind=%u a=%08x r=%08x\n",group,i,a.kind,a.word,ref.word);
                check(av&&rv&&std::signbit(av)==std::signbit(rv),"exact intentional zeros/signs");
            }
        } else if(a.word!=ref.word)++discrete;
    }
    std::printf("discrete=%u\n",discrete);check(!discrete,"exact discrete decisions/writes/canaries");
    for(unsigned k=1;k<5;++k)std::printf("quantity=%u count=%u differences=%u min=%.17g max=%.17g max_abs=%.17g max_rel=%.17g\n",k,totals[k],differences[k],minimum[k],maximum[k],absolute[k],relative[k]);
    check(nonfinite==8,"eight exact original nonfinite partial writes");
#endif
    std::printf("contact_polygon groups=%u observations=%zu allocations=%u releases=%u failures=%u\n",group,observations.size(),allocator.allocs,allocator.frees,failures);
    return failures?1:0;
}
