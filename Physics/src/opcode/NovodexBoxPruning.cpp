#include "Opcode.h"

// phys_fn_004816 and continuation phys_fn_004818 at 0x000b4530. OPCODE's
// SweepAndPrune::Init calls this NovodeX helper, while the standalone source
// calls the absent OPC_BoxPruning.cpp implementation. The oracle sorts all box
// minima plus a MAX_FLOAT sentinel and emits intersecting pairs in rank order.
// Its one persistent sorter is visible as the .data pointer at 0x00128460.
bool Opcode::CompleteBoxPruning(udword count, const AABB** boxes, Pairs& pairs,
	const Axes& axes)
{
	if(!count || !boxes)
		return false;

	const udword axis0 = axes.mAxis0;
	const udword axis1 = axes.mAxis1;
	const udword axis2 = axes.mAxis2;
	float* minima = new float[count + 1];
	for(udword i = 0; i < count; ++i)
		minima[i] = boxes[i]->GetMin(axis0);
	minima[count] = MAX_FLOAT;
	++count;

	static RadixSort* sorter = 0;
	if(!sorter)
		sorter = new RadixSort;
	const udword* sorted = sorter->Sort(minima, count).GetRanks();
	const udword* const last = sorted + count;
	const udword* running = sorted;
	while(running < last && sorted < last)
		{
		const udword first = *sorted++;
		while(minima[*running++] < minima[first]) {}
		if(running < last)
			{
			const udword* candidate = running;
			udword second;
			while(minima[second = *candidate++] <= boxes[first]->GetMax(axis0))
				{
				if(boxes[first]->Intersect(*boxes[second], axis1) &&
					boxes[first]->Intersect(*boxes[second], axis2))
					pairs.AddPair(first, second);
				}
			}
		}
	delete[] minima;
	return true;
}
