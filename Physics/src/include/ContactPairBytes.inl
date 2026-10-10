template <class T> static NX_INLINE T& cpmAt(void* p, NxU32 offset)
	{
	return *reinterpret_cast<T*>(static_cast<NxU8*>(p) + offset);
	}

template <class T> static NX_INLINE const T& cpmAt(const void* p, NxU32 offset)
	{
	return *reinterpret_cast<const T*>(static_cast<const NxU8*>(p) + offset);
	}

// `mov r,[src+k]; mov [dst+k],r` runs: a copy that never touches the FPU.
static NX_INLINE void cpmCopyWords(void* dst, const void* src, NxU32 words)
	{
	NxU32* d = static_cast<NxU32*>(dst);
	const NxU32* s = static_cast<const NxU32*>(src);
	for(NxU32 i = 0; i < words; i++)
		d[i] = s[i];
	}

