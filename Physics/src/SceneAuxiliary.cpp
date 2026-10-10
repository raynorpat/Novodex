#include "SceneAuxiliary.h"
#include "NxUserAllocator.h"

void nxSceneAuxDestroy(void* auxiliary)
	{
	static const unsigned offsets[10] =
		{ 0x90, 0x80, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00 };
	unsigned char* bytes = static_cast<unsigned char*>(auxiliary);
	for(unsigned i = 0; i != 10; ++i)
		{
		unsigned offset = offsets[i];
		void*& entries = *reinterpret_cast<void**>(bytes + offset);
		if(entries)
			nxFoundationSDKAllocator->free(entries);
		*reinterpret_cast<unsigned*>(bytes + offset + 0x00) = 0;
		*reinterpret_cast<unsigned*>(bytes + offset + 0x04) = 0;
		*reinterpret_cast<unsigned*>(bytes + offset + 0x08) = 0;
		}
	}
