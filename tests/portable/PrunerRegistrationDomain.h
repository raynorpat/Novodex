#ifndef NX_PRUNER_REGISTRATION_DOMAIN_H
#define NX_PRUNER_REGISTRATION_DOMAIN_H
// Shared literal calls and observations only; no registry algorithm lives here.
static unsigned registryWord(const void* object,unsigned offset)
{ unsigned value; std::memcpy(&value,static_cast<const unsigned char*>(object)+offset,4); return value; }
static void* registryPointer(const void* object,unsigned offset)
{ void* value; std::memcpy(&value,static_cast<const unsigned char*>(object)+offset,4); return value; }
static unsigned registryHalf(const void* object,unsigned index)
{ unsigned short value; std::memcpy(&value,static_cast<const unsigned char*>(object)+index*2,2); return value; }
template<class Host> static void registryObserve(Host& host,void* const* live,unsigned expectedCount,unsigned expectedCapacity)
{
    ++group; const void* pool=host.pool(); check(pool!=nullptr,"genuine shared process owner remains live");
    if(!pool) return;
    const unsigned count=registryWord(pool,4),capacity=registryWord(pool,8);
    check(count==expectedCount && capacity==expectedCapacity,"literal registry count/capacity");
    exact(count);exact(capacity);exact(registryWord(pool,24));
    const void* objects=registryPointer(pool,0); const void* forward=registryPointer(pool,12);
    const void* reverse=registryPointer(pool,16); const void* generations=registryPointer(pool,20);
    for(unsigned i=0;i<count;++i) {
        void* member=registryPointer(objects,i*4);unsigned identity=0xffffffffu;
        for(unsigned j=0;j<6;++j) if(live[j] && member==static_cast<unsigned char*>(live[j])+0x34) identity=j;
        check(identity!=0xffffffffu,"dense entry is the actual +34 registration member");exact(identity);
    }
    for(unsigned i=0;i<capacity;++i) { exact(registryHalf(forward,i));exact(registryHalf(reverse,i));exact(registryHalf(generations,i)); }
    for(unsigned i=0;i<6;++i) if(live[i]) { exact(i);exact(registryWord(live[i],0x34));exact(registryWord(live[i],0x38)); }
    host.guards();
}
template<class Host> static void registryDomain(Host& host)
{
    for(unsigned pass=0;pass<3;++pass) {
        host.begin(); check(host.pool()==nullptr,"SDK recreation begins with no process owner");
        void* live[6]={};
        for(unsigned i=0;i<5;++i) {
            live[i]=host.create(i%3);check(live[i]!=nullptr,"genuine original/production pruner producer");
            check(registryWord(live[i],0x34)==i && registryWord(live[i],0x38)==0,"literal constructor handle and timestamp");
            registryObserve(host,live,i+1,i<2?2:i<4?4:8);
        }
        host.update(live[0]);host.update(live[0]);
        check(registryWord(live[0],0x38)==2,"actual virtual timestamp update");registryObserve(host,live,5,8);
        const unsigned stale=registryWord(live[1],0x34);host.destroy(live[1]);live[1]=nullptr;
        registryObserve(host,live,4,8);
        for(unsigned handle:{stale,0xffffu,0x00010000u,stale}) { host.remove(handle);registryObserve(host,live,4,8); }
        live[5]=host.create(2);check(registryWord(live[5],0x34)==0x00010001u,"free index reuse includes generation");registryObserve(host,live,5,8);
        host.remove(stale);registryObserve(host,live,5,8);
        for(unsigned i=0;i<3;++i) host.update(live[5]);
        check(registryWord(live[5],0x38)==3,"real virtual dynamic timestamp update");registryObserve(host,live,5,8);
        const unsigned destructionOrder[]={4,0,5,3,2};
        for(unsigned i=0;i<5;++i) { const unsigned index=destructionOrder[i];host.destroy(live[index]);live[index]=nullptr;registryObserve(host,live,4-i,8); }
        // True constructors/destructors generate the whole u16 rollover, no seeded state.
        for(unsigned i=0;i<65536;++i) {
            live[0]=host.create(2);const unsigned handle=registryWord(live[0],0x34);
            check((handle&0xffffu)==2 && (handle>>16)==((i+1)&0xffffu),"actual generation progression modulo65536");
            if(i==0 || i==65534 || i==65535) registryObserve(host,live,1,8);
            host.destroy(live[0]);live[0]=nullptr;
        }
        registryObserve(host,live,0,8);host.end();++group;exact(host.pool()==nullptr);host.guards();
    }
}
#endif
