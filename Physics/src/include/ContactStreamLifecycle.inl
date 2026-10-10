// Row 002356 (0x0005b680, 22 B): constructs the stream sub-object at pair
// +0x10 (its SdkContainer at +0x28 through 004836, then 002354). thiscall.
static __declspec(noinline) void cpmOpen002354(void* streamObject);
static __declspec(noinline) void cpmOpen002356(void* streamObject)
	{
	new (static_cast<NxU8*>(streamObject) + 0x28) SdkContainer();
	NxContactSinkResetState(reinterpret_cast<NxU32*>(streamObject));
	}

// Row 002354 (0x0005b620, 86 B): resets the stream sub-object (zeroes
// +0x00..+0x33, reserves the pair-count word). thiscall.
static __declspec(noinline) void cpmOpen002354(void* streamObject)
	{
	NxContactSinkResetState(reinterpret_cast<NxU32*>(streamObject));
	}

