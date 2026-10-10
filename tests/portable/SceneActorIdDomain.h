#ifndef NX_SCENE_ACTOR_ID_DOMAIN_H
#define NX_SCENE_ACTOR_ID_DOMAIN_H
// Shared input/observation support only. All take/return algorithms are called
// independently on the shipped class or the real production class.
static unsigned idWord(const void* p) { unsigned n; std::memcpy(&n,p,4); return n; }
static unsigned* idPointer(const void* p,unsigned offset)
{ unsigned* v; std::memcpy(&v,static_cast<const unsigned char*>(p)+offset,4); return v; }
template<class Host> static void idSnapshot(Host& host,unsigned allocations,unsigned releases)
{
    const void* pool=host.pool();
    unsigned* begin=idPointer(pool,4); unsigned* end=idPointer(pool,8); unsigned* cap=idPointer(pool,12);
    const unsigned count=begin?unsigned(end-begin):0;
    const unsigned capacity=begin?unsigned(cap-begin):0;
    exact(idWord(pool)); exact(count); exact(capacity);
    exact(host.allocator.allocations-allocations); exact(host.allocator.releases-releases);
    for(unsigned i=0;i<count;++i) exact(begin[i]);
    check(count<=capacity,"coherent live array range"); host.guards();
}
template<class Host> static void actorIdDomain(Host& host)
{
    for(unsigned pass=0;pass<3;++pass) {
        host.begin();
        unsigned baselineAlloc=host.allocator.allocations, baselineFree=host.allocator.releases;
        ++group; idSnapshot(host,baselineAlloc,baselineFree);
        unsigned issued[15];
        for(unsigned i=0;i<15;++i) {
            ++group; issued[i]=host.take(); exact(issued[i]); check(issued[i]==i,"fresh sequential IDs");
            idSnapshot(host,baselineAlloc,baselineFree);
        }
        for(unsigned i=0;i<15;++i) {
            ++group;
            unsigned* old=idPointer(host.pool(),4);
            unsigned oldCapacity=old?unsigned(idPointer(host.pool(),12)-old):0;
            size_t oldFreed=host.allocator.freed.size();
            host.give(issued[(i*7)%15]);
            unsigned* now=idPointer(host.pool(),4);
            unsigned capacity=unsigned(idPointer(host.pool(),12)-now);
            check(capacity==(i<2?2u:i<6?6u:i<14?14u:30u),"literal original growth boundaries");
            if(capacity!=oldCapacity && old) {
                check(host.allocator.freed.size()==oldFreed+1 && host.allocator.freed.back()==old,"growth frees exactly previous array");
            }
            if(capacity!=oldCapacity) {
                for(unsigned k=i+1;k<capacity;++k) check(now[k]==0xcdcdcdcd,"unwritten capacity tail preserved");
            }
            idSnapshot(host,baselineAlloc,baselineFree);
        }
        for(unsigned i=15;i;--i) {
            ++group; unsigned value=host.take(); exact(value); check(value==issued[((i-1)*7)%15],"recycled IDs are LIFO");
            idSnapshot(host,baselineAlloc,baselineFree);
        }
        ++group; exact(host.take()); idSnapshot(host,baselineAlloc,baselineFree);
        // Helper-only supplement: no active actors. The real pool accepts all
        // words; this says nothing about public Actor ID exhaustion/validity.
        host.seed(0xfffffffeu);
        for(unsigned expected:{0xfffffffeu,0xffffffffu,0u}) {
            ++group; unsigned value=host.take(); exact(value); check(value==expected,"unsigned next wraps modulo 2^32");
            idSnapshot(host,baselineAlloc,baselineFree);
        }
        for(unsigned value:{0u,0xffffffffu,0xffffffffu,0x80000000u}) {
            ++group; host.give(value); idSnapshot(host,baselineAlloc,baselineFree);
        }
        for(unsigned expected:{0x80000000u,0xffffffffu,0xffffffffu,0u}) {
            ++group; unsigned value=host.take(); exact(value); check(value==expected,"arbitrary duplicate helper words preserved");
            idSnapshot(host,baselineAlloc,baselineFree);
        }
        host.seed(0); // Restore genuine empty original Scene's initial counter.
        ++group; idSnapshot(host,baselineAlloc,baselineFree);
        host.end();
        ++group; exact(host.released); exact(host.cleared); exact(host.preserved);
        check(host.released && host.cleared && host.preserved,"real release frees buffer, nulls pointer triple, preserves next");
    }
}
#endif
