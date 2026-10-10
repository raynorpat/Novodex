#ifndef NX_SCENE_VISITED_GUARD_ALLOCATOR_H
#define NX_SCENE_VISITED_GUARD_ALLOCATOR_H
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <map>
#include <vector>
#include "NxUserAllocator.h"

// Shared test support only: each executable supplies its own failure reporter.
// Neither observations nor production/original Scene algorithms live here.
class GuardAllocator : public NxUserAllocator
{
public:
    explicit GuardAllocator(void (*checkFailure)(bool,const char*)) : mCheck(checkFailure) {}
    std::map<void*,size_t> blocks;
    std::vector<void*> freed;
    unsigned allocations=0, releases=0, failAt=0;
    void* mallocDEBUG(size_t size,const char*,int) override { return malloc(size); }
    void* malloc(size_t size) override
    {
        if(++allocations==failAt) return nullptr;
        unsigned char* memory=static_cast<unsigned char*>(std::malloc(size+32));
        if(!memory) std::abort();
        std::memset(memory,0x6a,16); std::memset(memory+16,0xcd,size);
        std::memset(memory+16+size,0x7b,16); blocks[memory+16]=size;
        return memory+16;
    }
    void canaries()
    {
        for(const auto& item:blocks)
            for(unsigned i=0;i<16;++i)
            {
                const unsigned char* p=static_cast<unsigned char*>(item.first);
                mCheck(p[int(i)-16]==0x6a && p[item.second+i]==0x7b,"allocator guards");
            }
    }
    void free(void* pointer) override
    {
        if(!pointer) return;
        mCheck(blocks.count(pointer)==1,"release actual ownership");
        if(!blocks.count(pointer)) std::abort();
        canaries(); blocks.erase(pointer); freed.push_back(pointer); ++releases;
        std::free(static_cast<unsigned char*>(pointer)-16);
    }
    void* realloc(void* pointer,size_t size) override
    {
        if(!pointer) return malloc(size);
        void* next=malloc(size); if(!next) return nullptr;
        std::memcpy(next,pointer,std::min(size,blocks.at(pointer))); free(pointer); return next;
    }
private:
    void (*mCheck)(bool,const char*);
};
#endif
