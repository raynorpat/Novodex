#ifndef NX_SCENE_CONTACT_MEMBERS_H
#define NX_SCENE_CONTACT_MEMBERS_H

#include "NxPhysicsBackend.h"
#include "Containers.h"
#include "Opcode.h"
#include <stddef.h>
#include <new>

// The actual member window constructed by Scene row000647 at +0x450.
// This private owner provides C++ lifetimes for the scalar backend. It is not
// a Scene substitute; the rest of Scene and its scratch prefix have other owners.
struct NxSceneContactMembers
{
    Opcode::RayCollider rayCollider;
    SdkContainer edgeAxes0;
    SdkContainer edgeAxes1;

    NxSceneContactMembers() {}
    ~NxSceneContactMembers()
    {
        // Original Scene row000663: +4f0 empty, +4e0 empty, +450 destructor.
        edgeAxes1.empty();
        edgeAxes0.empty();
        // The real RayCollider destructor follows by member semantics.
    }

private:
    NxSceneContactMembers(const NxSceneContactMembers&);
    NxSceneContactMembers& operator=(const NxSceneContactMembers&);
};

// Fixed production placement is not enabled on native pointer layouts yet.
static_assert(sizeof(void*) == 4, "Scene contact member native layout remains Task9");
static_assert(sizeof(Opcode::RayCollider) == 0x90, "Scene RayCollider window is +450..+4df");
static_assert(sizeof(SdkContainer) == 0x10, "Scene edge container occupies sixteen bytes");
static_assert(offsetof(NxSceneContactMembers, rayCollider) == 0, "RayCollider begins at Scene+450");
static_assert(offsetof(NxSceneContactMembers, edgeAxes0) == 0x90, "first edges are at Scene+4e0");
static_assert(offsetof(NxSceneContactMembers, edgeAxes1) == 0xa0, "second edges are at Scene+4f0");
static_assert(sizeof(NxSceneContactMembers) == 0xb0, "contact member window ends at Scene+500");
static_assert(alignof(NxSceneContactMembers) == 4, "actual Win32 members require four-byte alignment");

inline NxSceneContactMembers* nxSceneContactMembersConstruct(void* storage)
{
    return ::new(storage) NxSceneContactMembers;
}

inline void nxSceneContactMembersDestroy(NxSceneContactMembers* members)
{
    members->~NxSceneContactMembers();
}

#endif
