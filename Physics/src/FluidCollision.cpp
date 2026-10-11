/*----------------------------------------------------------------------------*\
|
|                                NovodeX Technology
|
|                               www.novodex.com
|
\*----------------------------------------------------------------------------*/

// phys_fn_003514 (RVA 0x000864e0) computes the exclusive prefix of the
// per-mesh collision-bucket counts. The input descriptor stores the record
// pointer at +0 and row count at +8; each 12-byte row stores its output prefix
// at +4 and its contribution at +8. The second argument receives the total,
// and EAX returns the descriptor's row count.
extern "C" unsigned __cdecl nxFluidCollisionBucketPrefix(void* descriptor,
	unsigned* total)
	{
	unsigned char* const bytes = static_cast<unsigned char*>(descriptor);
	unsigned char* const records = *reinterpret_cast<unsigned char**>(bytes);
	const unsigned count = *reinterpret_cast<unsigned*>(bytes + 8);
	*total = 0;
	for(unsigned i = 0; i < count; ++i)
		{
		unsigned char* const record = records + i * 12;
		*reinterpret_cast<unsigned*>(record + 4) = *total;
		*total += *reinterpret_cast<unsigned*>(record + 8);
		}
	return count;
	}
