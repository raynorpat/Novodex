/*----------------------------------------------------------------------------*\
|
|                                NovodeX Technology
|
|                               www.novodex.com
|
\*----------------------------------------------------------------------------*/

#include "NxSdkAllocator.h"
#include <math.h>
#include <stdint.h>
#include <string.h>

struct NxFluidCollisionBucketHashDescriptor
	{
	unsigned* buckets;
	unsigned bucketCount;
	};

struct NxFluidCollisionInt16Vector3Array
	{
	short* data;
	int count;
	unsigned capacity;
	};

static_assert(sizeof(short) == 2,
	"phys_fn_003526 hashes packed signed 16-bit cell coordinates");

// phys_fn_003526 (RVA 0x00086ad0) adds each occupied cell to the bucket
// selected by the signed XYZ hash. The array rows are twelve bytes and the
// pending count is the third dword in each row.
extern "C" int __cdecl nxFluidCollisionCountBuckets(
	NxFluidCollisionBucketHashDescriptor* descriptor,
	const NxFluidCollisionInt16Vector3Array* cells)
	{
	const int count = cells->count;
	if(count > 0)
		{
		for(int i = 0; i < count; ++i)
			{
			const short* const cell = cells->data + i * 3;
			const int hash = static_cast<int>(cell[0])
				+ 101 * static_cast<int>(cell[1])
				+ 7919 * static_cast<int>(cell[2]);
			const unsigned bucket = static_cast<unsigned>(hash)
				% descriptor->bucketCount;
			descriptor->buckets[bucket * 3 + 2]++;
			}
		}
	return count;
	}

static bool nxFluidCollisionTriangleSeparates(const float vertices[3][3],
	const float halfSize[3], const float axis[3])
	{
	float minimum = vertices[0][0] * axis[0] + vertices[0][1] * axis[1]
		+ vertices[0][2] * axis[2];
	float maximum = minimum;
	for(unsigned vertex = 1; vertex != 3; ++vertex)
		{
		const float projection = vertices[vertex][0] * axis[0]
			+ vertices[vertex][1] * axis[1] + vertices[vertex][2] * axis[2];
		if(projection < minimum) minimum = projection;
		if(projection > maximum) maximum = projection;
		}
	const float radius = halfSize[0] * fabsf(axis[0])
		+ halfSize[1] * fabsf(axis[1])
		+ halfSize[2] * fabsf(axis[2]);
	return minimum > radius || maximum < -radius;
	}

static bool nxFluidCollisionTriangleOverlapsBox(const float center[3],
	const float halfSize[3], const float first[3], const float second[3],
	const float third[3])
	{
	float vertices[3][3] = {
		{ first[0] - center[0], first[1] - center[1], first[2] - center[2] },
		{ second[0] - center[0], second[1] - center[1], second[2] - center[2] },
		{ third[0] - center[0], third[1] - center[1], third[2] - center[2] }
		};
	static const float basis[3][3] = {
		{ 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f },
		{ 0.0f, 0.0f, 1.0f }
		};
	for(unsigned axis = 0; axis != 3; ++axis)
		if(nxFluidCollisionTriangleSeparates(vertices, halfSize, basis[axis]))
			return false;
	float edges[3][3];
	for(unsigned edge = 0; edge != 3; ++edge)
		for(unsigned component = 0; component != 3; ++component)
			edges[edge][component] = vertices[(edge + 1) % 3][component]
				- vertices[edge][component];
	for(unsigned edge = 0; edge != 3; ++edge)
		for(unsigned axis = 0; axis != 3; ++axis)
			{
			const unsigned other = (axis + 1) % 3;
			const unsigned last = (axis + 2) % 3;
			float mutableAxis[3] = { 0.0f, 0.0f, 0.0f };
			mutableAxis[other] = edges[edge][last];
			mutableAxis[last] = -edges[edge][other];
			if(nxFluidCollisionTriangleSeparates(vertices, halfSize, mutableAxis))
				return false;
			}
	const float normal[3] = {
		edges[0][1] * edges[1][2] - edges[0][2] * edges[1][1],
		edges[0][2] * edges[1][0] - edges[0][0] * edges[1][2],
		edges[0][0] * edges[1][1] - edges[0][1] * edges[1][0]
		};
	return !nxFluidCollisionTriangleSeparates(vertices, halfSize, normal);
	}

// phys_fn_003524 (RVA 0x000866d0) enumerates overlapped signed-16-bit grid
// cells for one triangle. The extension's triBoxOverlap export is not part of
// NxPhysics.dll, so the reconstructed DLL carries the equivalent SAT test.
extern "C" short __cdecl nxFluidCollisionTriangleCells(
	NxFluidCollisionInt16Vector3Array* cells, const float* first,
	unsigned __int64 secondAndThird, float margin, float cellSize)
	{
	const float* const second = reinterpret_cast<const float*>(
		static_cast<uintptr_t>(secondAndThird));
	const float* const third = reinterpret_cast<const float*>(
		static_cast<uintptr_t>(secondAndThird >> 32));
	float minimum[3] = { first[0], first[1], first[2] };
	float maximum[3] = { first[0], first[1], first[2] };
	for(unsigned axis = 0; axis != 3; ++axis)
		{
		if(second[axis] < minimum[axis]) minimum[axis] = second[axis];
		if(third[axis] < minimum[axis]) minimum[axis] = third[axis];
		if(second[axis] > maximum[axis]) maximum[axis] = second[axis];
		if(third[axis] > maximum[axis]) maximum[axis] = third[axis];
		}
	const double inverseCellSize = 1.0 / static_cast<double>(cellSize);
	short minimumCell[3];
	short dimensions[3];
	for(unsigned axis = 0; axis != 3; ++axis)
		{
		const float expandedMinimum = minimum[axis] - margin;
		const float expandedMaximum = maximum[axis] + margin;
		const float scaledMinimum = static_cast<float>(
			static_cast<double>(expandedMinimum) * inverseCellSize);
		const float scaledMaximum = static_cast<float>(
			static_cast<double>(expandedMaximum) * inverseCellSize);
		minimumCell[axis] = static_cast<short>(floor(static_cast<double>(scaledMinimum)));
		const short maximumCell = static_cast<short>(floor(static_cast<double>(scaledMaximum)));
		dimensions[axis] = static_cast<short>(maximumCell - minimumCell[axis] + 1);
		}
	const int possibleCells = static_cast<int>(dimensions[0])
		* static_cast<int>(dimensions[1]) * static_cast<int>(dimensions[2]);
	if(static_cast<int>(cells->capacity) < possibleCells)
		{
		const unsigned newCapacity = static_cast<unsigned>(possibleCells + 1);
		void* const resized = nxGetSdkAllocator()->realloc(cells->data,
			static_cast<size_t>(newCapacity) * 3 * sizeof(short));
		if(resized)
			{
			cells->data = static_cast<short*>(resized);
			cells->capacity = newCapacity;
			}
		else
			return 0;
		}
	const float half = cellSize * 0.5f + margin;
	const float halfSize[3] = { half, half, half };
	short completedLayers = static_cast<short>(possibleCells);
	for(short z = 0; z != dimensions[2]; ++z)
		{
		for(short y = 0; y != dimensions[1]; ++y)
			for(short x = 0; x != dimensions[0]; ++x)
				{
				const float center[3] = {
					static_cast<float>((static_cast<double>(static_cast<short>(minimumCell[0] + x)) + 0.5) * cellSize),
					static_cast<float>((static_cast<double>(static_cast<short>(minimumCell[1] + y)) + 0.5) * cellSize),
					static_cast<float>((static_cast<double>(static_cast<short>(minimumCell[2] + z)) + 0.5) * cellSize)
					};
				if(nxFluidCollisionTriangleOverlapsBox(center, halfSize,
					first, second, third))
					{
					short* const output = cells->data + cells->count * 3;
					output[0] = static_cast<short>(minimumCell[0] + x);
					output[1] = static_cast<short>(minimumCell[1] + y);
					output[2] = static_cast<short>(minimumCell[2] + z);
					++cells->count;
					}
				}
		completedLayers = static_cast<short>(z + 1);
		}
	return completedLayers;
	}

struct NxFluidCollisionTriangleMesh
	{
	unsigned triangleCount;
	const unsigned* indices;
	unsigned reserved;
	const float* vertices;
	float transform[12];
	};

static_assert(offsetof(NxFluidCollisionTriangleMesh, transform) == 0x10,
	"phys_fn_003528 reads the 3x4 transform at mesh descriptor +0x10");
static_assert(sizeof(NxFluidCollisionTriangleMesh) == 0x40,
	"phys_fn_003528 reads a 0x40-byte static collision mesh descriptor");

static void nxFluidCollisionTransformVertex(const float* input,
	const float* transform, float* output)
	{
	const float x = input[0];
	const float y = input[1];
	const float z = input[2];
	output[0] = ((x * transform[0] + y * transform[3])
		+ z * transform[6]) + transform[9];
	output[1] = ((x * transform[1] + y * transform[4])
		+ z * transform[7]) + transform[10];
	output[2] = ((x * transform[2] + y * transform[5])
		+ z * transform[8]) + transform[11];
	}

// phys_fn_003528 (RVA 0x00086b40) transforms each indexed mesh triangle,
// enumerates its overlapped cells, then adds those cells to the bucket hash.
extern "C" void __cdecl nxFluidCollisionBuildTriangleCells(
	NxFluidCollisionBucketHashDescriptor* hash,
	const NxFluidCollisionTriangleMesh* mesh, float margin, float cellSize,
	NxFluidCollisionInt16Vector3Array* cells)
	{
	if(!cells)
		return;
	for(unsigned triangle = 0; triangle < mesh->triangleCount; ++triangle)
		{
		const unsigned* const indices = mesh->indices + triangle * 3;
		const float* const first = mesh->vertices + indices[0] * 3;
		const float* const second = mesh->vertices + indices[1] * 3;
		const float* const third = mesh->vertices + indices[2] * 3;
		float transformed[3][3];
		nxFluidCollisionTransformVertex(first, mesh->transform, transformed[0]);
		nxFluidCollisionTransformVertex(second, mesh->transform, transformed[1]);
		nxFluidCollisionTransformVertex(third, mesh->transform, transformed[2]);
		cells->count = 0;
		const unsigned __int64 secondAndThird =
			(static_cast<unsigned __int64>(reinterpret_cast<uintptr_t>(transformed[2])) << 32)
			| reinterpret_cast<uintptr_t>(transformed[1]);
		nxFluidCollisionTriangleCells(cells, transformed[0], secondAndThird,
			margin, cellSize);
		nxFluidCollisionCountBuckets(hash, cells);
		}
	}

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

// phys_fn_003530 (RVA 0x00086e00) folds grid-cell coordinates into each
// collision bucket, merging duplicates through the bucket's contiguous range.
extern "C" int __cdecl nxFluidCollisionAccumulateCellCounts(
	int* cellStorage, NxFluidCollisionBucketHashDescriptor* hash,
	const NxFluidCollisionInt16Vector3Array* input)
	{
	if(input->count > 0)
		{
		unsigned char* const records = reinterpret_cast<unsigned char*>(
			static_cast<uintptr_t>(*cellStorage));
		unsigned char* const buckets = reinterpret_cast<unsigned char*>(hash->buckets);
		for(int inputIndex = 0; inputIndex < input->count; ++inputIndex)
			{
			const short* const coordinate = input->data + inputIndex * 3;
			const unsigned xy = static_cast<unsigned short>(coordinate[0])
				| (static_cast<unsigned>(static_cast<unsigned short>(coordinate[1])) << 16);
			const short z = coordinate[2];
			const int signedHash = static_cast<int>(coordinate[0])
				+ 101 * static_cast<int>(coordinate[1])
				+ 7919 * static_cast<int>(z);
			const unsigned bucketIndex = static_cast<unsigned>(signedHash)
				% hash->bucketCount;
			unsigned char* const bucket = buckets + bucketIndex * 12;
			const unsigned cellCount = *reinterpret_cast<unsigned*>(bucket);
			const unsigned firstCell = *reinterpret_cast<unsigned*>(bucket + 4);
			int found = -1;
			for(unsigned i = 0; i < cellCount; ++i)
				{
				unsigned char* const cell = records + (firstCell + i) * 16;
				if(*reinterpret_cast<unsigned*>(cell) == xy
					&& *reinterpret_cast<short*>(cell + 4) == z)
					{
					found = static_cast<int>(firstCell + i);
					break;
					}
				}
			if(found == -1)
				{
				const unsigned cellIndex = firstCell + cellCount;
				*reinterpret_cast<unsigned*>(bucket) = cellCount + 1;
				unsigned char* const cell = records + cellIndex * 16;
				*reinterpret_cast<unsigned*>(cell + 8) = 1;
				*reinterpret_cast<unsigned*>(cell + 12) = 0xffffffffu;
				*reinterpret_cast<unsigned*>(cell) = xy;
				*reinterpret_cast<short*>(cell + 4) = z;
				}
			else
				{
			unsigned char* const cell = records + static_cast<unsigned>(found) * 16;
				++*reinterpret_cast<unsigned*>(cell + 8);
				}
			}
		}
	return 1;
	}

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
