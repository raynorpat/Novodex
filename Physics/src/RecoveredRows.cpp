// Internal rows reconstructed from the pinned NxPhysics image.

// phys_fn_000002 (RVA 0x1030) walks a strided range in reverse. The oracle
// computes the one-past-end address, decrements before each thiscall, and
// returns the last callback result (or count - 1 when the count is nonpositive).
extern "C" int __stdcall phys_fn_000002(
	int first, int stride, int count, int (__thiscall *callback)(int))
	{
	int address = count * stride + first;
	int result = count - 1;
	if(result >= 0)
		{
		for(int remaining = count; remaining != 0; --remaining)
			{
			address -= stride;
			result = callback(address);
			}
		}
	return result;
	}
