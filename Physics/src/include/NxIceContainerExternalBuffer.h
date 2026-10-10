#ifndef NX_ICE_CONTAINER_EXTERNAL_BUFFER_H
#define NX_ICE_CONTAINER_EXTERNAL_BUFFER_H
#include "Opcode.h"
#include <string.h>

// 005214 -> 004847. This receiver IS an actual IceCore::Container, whose
// private four fields have the original cap/count/pointer/factor representation.
// Never call a member of the unrelated SdkContainer on this storage.
inline void nxIceContainerSetExternalBuffer(IceCore::Container& container,
    udword capacity, udword* entries)
{
    static_assert(sizeof(void*)==4,"external buffer requires original Win32 layout");
    static_assert(sizeof(IceCore::Container)==0x10,"original four-word container");
    static_assert(alignof(IceCore::Container)==4,"original container alignment");
    container.Empty(); // real owner free and borrowed-buffer policy; count becomes zero
    unsigned char* bytes=reinterpret_cast<unsigned char*>(&container);
    memcpy(bytes,&capacity,4);
    memcpy(bytes+8,&entries,4);
    const float borrowed=-1.0f;
    memcpy(bytes+12,&borrowed,4);
}
#endif
