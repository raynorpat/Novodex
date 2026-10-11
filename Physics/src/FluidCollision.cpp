/*----------------------------------------------------------------------------*\
|
|                                NovodeX Technology
|
|                               www.novodex.com
|
\*----------------------------------------------------------------------------*/

#include "NxSdkAllocator.h"
#include <string.h>

struct NxFluidCollisionBucket
	{
	int cellCount;
	unsigned firstCell;
	unsigned opaque;
	};

struct NxFluidCollisionCell
	{
	unsigned words[4];
	};

struct NxFluidCollisionCopyDescriptor
	{
	NxFluidCollisionBucket* buckets;
	unsigned bucketCapacity;
	unsigned bucketCount;
	NxFluidCollisionCell* cells;
	unsigned cellCount;
	unsigned cellCapacity;
	unsigned* indices;
	unsigned indexCount;
	unsigned indexCapacity;
	};

static_assert(sizeof(NxFluidCollisionBucket) == 12,
	"phys_fn_003518 advances collision buckets by twelve bytes");
static_assert(sizeof(NxFluidCollisionCell) == 16,
	"phys_fn_003518 copies sixteen-byte cell records");
static_assert(sizeof(NxFluidCollisionCopyDescriptor) == 36,
	"phys_fn_003518 reads the descriptor through the index count");

// phys_fn_003518 (RVA 0x00086590) flattens the bucket-referenced cell ranges,
// then copies the complete 8-byte index records. The oracle writes an
// uninitialized stack word into each bucket's third dword; retain the copied
// source word here, since that byte has no defined oracle value.
extern "C" unsigned __cdecl nxFluidCollisionStaticCopy(
	NxFluidCollisionCopyDescriptor* destination,
	const NxFluidCollisionCopyDescriptor* source)
	{
	SdkAllocator* const allocator = nxGetSdkAllocator();
	destination->buckets = static_cast<NxFluidCollisionBucket*>(
		allocator->malloc(source->bucketCount * sizeof(NxFluidCollisionBucket),
			NX_MEMORY_PERSISTENT));
	destination->bucketCapacity = source->bucketCapacity;
	destination->bucketCount = source->bucketCount;
	if(source->bucketCount)
		memcpy(destination->buckets, source->buckets,
			source->bucketCount * sizeof(NxFluidCollisionBucket));

	int totalCells = 0;
	for(unsigned i = 0; i < source->bucketCount; ++i)
		totalCells += static_cast<int>(source->buckets[i].cellCount);
	destination->cells = static_cast<NxFluidCollisionCell*>(
		allocator->malloc(static_cast<size_t>(totalCells)
			* sizeof(NxFluidCollisionCell), NX_MEMORY_PERSISTENT));
	destination->cellCount = 0;
	destination->cellCapacity = static_cast<unsigned>(totalCells);
	for(unsigned bucket = 0; bucket < source->bucketCount; ++bucket)
		{
		const NxFluidCollisionBucket& input = source->buckets[bucket];
		const unsigned firstOutputCell = destination->cellCount;
		for(int cell = 0; cell < input.cellCount; ++cell)
			{
			const NxFluidCollisionCell& inputCell =
				source->cells[input.firstCell + cell];
			destination->cells[destination->cellCount++] = inputCell;
			}
		NxFluidCollisionBucket& output = destination->buckets[bucket];
		output.cellCount = destination->cellCount - firstOutputCell;
		output.firstCell = firstOutputCell;
		}

	destination->indices = static_cast<unsigned*>(allocator->malloc(
		source->indexCount * 2 * sizeof(unsigned), NX_MEMORY_PERSISTENT));
	destination->indexCount = 0;
	destination->indexCapacity = source->indexCount;
	while(destination->indexCount < source->indexCount)
		{
		const unsigned offset = destination->indexCount * 2;
		destination->indices[offset] = source->indices[offset];
		destination->indices[offset + 1] = source->indices[offset + 1];
		++destination->indexCount;
		}
	return source->indexCount;
	}

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

// phys_fn_003516 (RVA 0x00086520) assigns each occupied cell its start index
// in the flattened collision-index array, then clears the pending cell count.
extern "C" unsigned __cdecl nxFluidCollisionCellPrefix(void* descriptor)
	{
	unsigned char* const bytes = static_cast<unsigned char*>(descriptor);
	unsigned char* const buckets = *reinterpret_cast<unsigned char**>(bytes);
	const unsigned bucketCount = *reinterpret_cast<unsigned*>(bytes + 8);
	unsigned char* const cells = *reinterpret_cast<unsigned char**>(bytes + 12);
	unsigned* const totalIndices = reinterpret_cast<unsigned*>(bytes + 28);
	for(unsigned bucket = 0; bucket < bucketCount; ++bucket)
		{
		unsigned char* const row = buckets + bucket * 12;
		const int cellCount = *reinterpret_cast<int*>(row);
		if(cellCount <= 0)
			continue;
		const unsigned firstCell = *reinterpret_cast<unsigned*>(row + 4);
		for(int i = 0; i < cellCount; ++i)
			{
			unsigned char* const cell = cells + (firstCell + static_cast<unsigned>(i)) * 16;
			*reinterpret_cast<unsigned*>(cell + 12) = *totalIndices;
			*totalIndices += *reinterpret_cast<unsigned*>(cell + 8);
			*reinterpret_cast<unsigned*>(cell + 8) = 0;
			}
		}
	return bucketCount;
	}
