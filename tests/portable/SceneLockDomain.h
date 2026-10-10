#ifndef NX_SCENE_LOCK_DOMAIN_H
#define NX_SCENE_LOCK_DOMAIN_H
#include <windows.h>
#include <cstring>
// Shared test schedule and observers only; original and scalar methods are independent.
static unsigned lockWord(const void* p,unsigned offset=0) { unsigned v; std::memcpy(&v,static_cast<const unsigned char*>(p)+offset,4); return v; }
static void* lockPointer(const void* p,unsigned offset=0) { void* v; std::memcpy(&v,static_cast<const unsigned char*>(p)+offset,4); return v; }
template<class F> static DWORD WINAPI lockTestWorker(void* context) { (*static_cast<F*>(context))(); return 0; }
template<class Host> static void lockSnapshot(Host& host,unsigned index,bool ownerValid) {
    ++group; exact(host.state(index,0x18));
    unsigned role=0;
    if(ownerValid) { unsigned id=host.state(index,0x1c); role=id==host.mainThread?1:id==host.workerThread?2:3; }
    exact(role); exact(host.state(index,8)); exact(host.state(index,4)); host.guards();
}
template<class Host> static void blockedLock(Host& host,unsigned index,bool nestedTry) {
    ++group; exact(host.operation(index,0));
    if(nestedTry) { exact(host.operation(index,0)); exact(host.operation(index,2)); }
    HANDLE ready=CreateEventA(nullptr,TRUE,FALSE,nullptr), acquired=CreateEventA(nullptr,TRUE,FALSE,nullptr), release=CreateEventA(nullptr,TRUE,FALSE,nullptr);
    check(ready && acquired && release,"successful test synchronization events");
    const unsigned heldCount=host.state(index,4);
    bool acquiredResult=false,unlockedResult=false;
    auto workerBody=[&] {
        host.workerThread=GetCurrentThreadId(); SetEvent(ready);
        acquiredResult=host.operation(index,nestedTry?1:0); SetEvent(acquired);
        WaitForSingleObject(release,10000); unlockedResult=host.operation(index,2);
    };
    HANDLE worker=CreateThread(nullptr,0,lockTestWorker<decltype(workerBody)>,&workerBody,0,nullptr);
    check(worker!=nullptr,"successful Win32 test contender thread");
    check(WaitForSingleObject(ready,10000)==WAIT_OBJECT_0,"contender ready");
    const DWORD deadline=GetTickCount()+10000;
    while(host.state(index,4)==heldCount && LONG(GetTickCount()-deadline)<0) SwitchToThread();
    check(host.state(index,4)!=heldCount,"actual critical section has blocked contender");
    exact(WaitForSingleObject(acquired,0)==WAIT_TIMEOUT);
    lockSnapshot(host,index,true); ++group; exact(host.operation(index,2));
    check(WaitForSingleObject(acquired,10000)==WAIT_OBJECT_0,"contender acquires after balanced owner release");
    exact(acquiredResult); lockSnapshot(host,index,true);
    SetEvent(release); check(WaitForSingleObject(worker,10000)==WAIT_OBJECT_0,"contender quiescent exit"); CloseHandle(worker); ++group; exact(unlockedResult);
    lockSnapshot(host,index,true);
    CloseHandle(release); CloseHandle(acquired); CloseHandle(ready);
}
template<class Host> static void sceneLockDomain(Host& host) {
    for(unsigned cycle=0;cycle<3;++cycle) {
        host.begin();
        for(unsigned index=0;index<2;++index) {
            ++group; exact(host.linkBytes(index)); exact(host.blockBytes(index));
            lockSnapshot(host,index,false);
            ++group; exact(host.operation(index,1)); lockSnapshot(host,index,true);
            ++group; exact(host.operation(index,1)); lockSnapshot(host,index,true);
            ++group; exact(host.operation(index,2)); lockSnapshot(host,index,true);
            ++group; exact(host.operation(index,1)); lockSnapshot(host,index,true);
            ++group; exact(host.operation(index,2)); lockSnapshot(host,index,true);
            ++group; exact(host.operation(index,2)); lockSnapshot(host,index,true);
            ++group; exact(host.operation(index,0));
            bool contender=false;
            auto attemptBody=[&] { host.workerThread=GetCurrentThreadId(); contender=host.operation(index,1); if(contender) host.operation(index,2); };
            HANDLE attempt=CreateThread(nullptr,0,lockTestWorker<decltype(attemptBody)>,&attemptBody,0,nullptr);
            check(attempt!=nullptr && WaitForSingleObject(attempt,10000)==WAIT_OBJECT_0,"foreign try contender quiescent exit"); CloseHandle(attempt);
            exact(contender); lockSnapshot(host,index,true);
            ++group; exact(host.operation(index,2)); lockSnapshot(host,index,true);
            blockedLock(host,index,false);
            blockedLock(host,index,true);
            ++group; exact(host.operation(index,1)); lockSnapshot(host,index,true);
            ++group; exact(host.operation(index,2)); lockSnapshot(host,index,true);
        }
        host.end(); ++group;
        for(unsigned i=0;i<4;++i) exact(host.cleanup[i]);
    }
}
#endif
