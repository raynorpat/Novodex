#include "SceneAuxiliary.h"
#include "NxUserAllocator.h"

__declspec(noinline) void NxSceneAuxiliaryArrayBlock::destroy()
	{
	unsigned char* bytes = reinterpret_cast<unsigned char*>(this);
	for(unsigned offset = 0x30;; offset -= 0x10)
		{
		void*& entries = *reinterpret_cast<void**>(bytes + offset);
		if(entries)
			nxFoundationSDKAllocator->free(entries);
		*reinterpret_cast<unsigned*>(bytes + offset + 0x00) = 0;
		*reinterpret_cast<unsigned*>(bytes + offset + 0x04) = 0;
		*reinterpret_cast<unsigned*>(bytes + offset + 0x08) = 0;
		if(offset == 0)
			break;
		}
	}

void nxSceneAuxDestroy(void* auxiliary)
	{
	unsigned char* bytes = static_cast<unsigned char*>(auxiliary);
	for(unsigned offset = 0x90; offset >= 0x80; offset -= 0x10)
		{
		void*& entries = *reinterpret_cast<void**>(bytes + offset);
		if(entries)
			nxFoundationSDKAllocator->free(entries);
		*reinterpret_cast<unsigned*>(bytes + offset + 0x00) = 0;
		*reinterpret_cast<unsigned*>(bytes + offset + 0x04) = 0;
		*reinterpret_cast<unsigned*>(bytes + offset + 0x08) = 0;
		}
	reinterpret_cast<NxSceneAuxiliaryArrayBlock*>(bytes + 0x40)->destroy();
	reinterpret_cast<NxSceneAuxiliaryArrayBlock*>(bytes)->destroy();
	}
