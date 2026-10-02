#include "Opcode.h"

// phys_fn_004814 (0x000b4080): full sweep-and-prune across two pools. The
// oracle runs the sorted A->B pass, followed by the remaining B->A pass; this
// preserves its pair order, including equal-minimum boundary cases.
bool Opcode::BipartiteBoxPruning(udword count0, const AABB** boxes0,
	udword count1, const AABB** boxes1, Pairs& pairs, const Axes& axes)
{
	if(!count0 || !boxes0 || !count1 || !boxes1)
		return false;
	const udword axis0 = axes.mAxis0;
	const udword axis1 = axes.mAxis1;
	const udword axis2 = axes.mAxis2;
	float* minima0 = new float[count0];
	float* minima1 = new float[count1];
	for(udword i = 0; i < count0; ++i)
		minima0[i] = boxes0[i]->GetMin(axis0);
	for(udword i = 0; i < count1; ++i)
		minima1[i] = boxes1[i]->GetMin(axis0);
	static RadixSort* sorter0 = 0;
	static RadixSort* sorter1 = 0;
	if(!sorter0) sorter0 = new RadixSort;
	if(!sorter1) sorter1 = new RadixSort;
	const udword* sorted0 = sorter0->Sort(minima0, count0).GetRanks();
	const udword* sorted1 = sorter1->Sort(minima1, count1).GetRanks();
	const udword* const last0 = sorted0 + count0;
	const udword* const last1 = sorted1 + count1;
	const udword* running0 = sorted0;
	const udword* running1 = sorted1;
	udword first, second;
	while(running1 < last1 && sorted0 < last0)
		{
		first = *sorted0++;
		while(running1 < last1 && minima1[*running1] < minima0[first])
			++running1;
		const udword* candidate = running1;
		while(candidate < last1
			&& minima1[second = *candidate++] <= boxes0[first]->GetMax(axis0))
			if(boxes0[first]->Intersect(*boxes1[second], axis1)
				&& boxes0[first]->Intersect(*boxes1[second], axis2))
				pairs.AddPair(first, second);
		}
	while(running0 < last0 && sorted1 < last1)
		{
		first = *sorted1++;
		while(running0 < last0 && minima0[*running0] <= minima1[first])
			++running0;
		const udword* candidate = running0;
		while(candidate < last0
			&& minima0[second = *candidate++] <= boxes1[first]->GetMax(axis0))
			if(boxes0[second]->Intersect(*boxes1[first], axis1)
				&& boxes0[second]->Intersect(*boxes1[first], axis2))
				pairs.AddPair(second, first);
		}
	delete[] minima1;
	delete[] minima0;
	return true;
}

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
