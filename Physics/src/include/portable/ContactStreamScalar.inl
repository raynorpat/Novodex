#include <cstring>
// 002354's pointer is a byte view of the genuine stream subobject at pair+10.
// The Container was constructed by the same 002356 producer used by x87.
void __fastcall NxContactSinkResetState(NxU32* state)
{
    unsigned char* bytes = reinterpret_cast<unsigned char*>(state);
    SdkContainer* stream = reinterpret_cast<SdkContainer*>(bytes + 0x28);
    if(stream->mCount != 0) stream->mCount = 0;
    const NxU32 oldCount = stream->mCount;
    if(stream->mCount == stream->mCapacity) stream->resize(1);
    stream->mEntries[stream->mCount++] = 0;
    std::memset(bytes, 0, 0x28);
    std::memcpy(bytes + 4, &oldCount, sizeof(oldCount));
}
