/*
 * NOVODEX LOCAL MODIFICATION
 * upstream: External/opcode/upstream/Opcode/Ice/IceRevisitedRadix.cpp
 *
 * [1] The borrowed-buffer marker: set true by the constructor and the only guard on
 *     the two frees in the destructor.
 *     established at 0x000e32c0 `mov byte ptr [eax+0x14], 1`; 0x000e32e3
 *     `mov al,[esi+0x14]; test al,al; je 0x000e3326`, freeing mRanks2 before
 *     mRanks, which is IceRevisitedRadix.cpp:186's order.
 * [2] Resize carries the same guard and does not restore the marker.
 *     established at 0x000e3333 is the same three instructions; the allocations at 0x000e337c
 *     and 0x000e339f write no byte at +0x14 anywhere in the row, which ends at
 *     0x000e33b6 with `mov al,1; ret 4`.
 * [3] the host allocator seam. Every allocation and free in this file goes
 *     through the engine's allocator singleton instead of operator new/delete.
 *     The seam is applied per file pair whose sites are all enumerable, not
 *     globally: half a conversion frees a compiler-allocated block through the
 *     host and corrupts the heap.
 *     established at 0x000e3342/0x000e3360 free through `call dword ptr [edx+0x0c]` after the
 *     allocator getter at 0x000b4000; 0x000e337c/0x000e339f allocate through
 *     `call dword ptr [edx]` with (nb*4, 0).
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains source code from the article "Radix Sort Revisited".
 *	\file		IceRevisitedRadix.cpp
 *	\author		Pierre Terdiman
 *	\date		April, 4, 2000
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Revisited Radix Sort.
 *	This is my new radix routine:
 *  - it uses indices and doesn't recopy the values anymore, hence wasting less ram
 *  - it creates all the histograms in one run instead of four
 *  - it sorts words faster than dwords and bytes faster than words
 *  - it correctly sorts negative floating-point values by patching the offsets
 *  - it automatically takes advantage of temporal coherence
 *  - multiple keys support is a side effect of temporal coherence
 *  - it may be worth recoding in asm... (mainly to use FCOMI, FCMOV, etc) [it's probably memory-bound anyway]
 *
 *	History:
 *	- 08.15.98: very first version
 *	- 04.04.00: recoded for the radix article
 *	- 12.xx.00: code lifting
 *	- 09.18.01: faster CHECK_PASS_VALIDITY thanks to Mark D. Shattuck (who provided other tips, not included here)
 *	- 10.11.01: added local ram support
 *	- 01.20.02: bugfix! In very particular cases the last pass was skipped in the float code-path, leading to incorrect sorting......
 *	- 01.02.02:	- "mIndices" renamed => "mRanks". That's a rank sorter after all.
 *				- ranks are not "reset" anymore, but implicit on first calls
 *	- 07.05.02:	- offsets rewritten with one less indirection.
 *	- 11.03.02:	- "bool" replaced with RadixHint enum
 *
 *	\class		RadixSort
 *	\author		Pierre Terdiman
 *	\version	1.4
 *	\date		August, 15, 1998
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
To do:
	- add an offset parameter between two input values (avoid some data recopy sometimes)
	- unroll ? asm ?
	- 11 bits trick & 3 passes as Michael did
	- prefetch stuff the day I have a P3
	- make a version with 16-bits indices ?
*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Precompiled Header
#include "Stdafx.h"
#include "..\OpcodeNovodeXHost.h"

using namespace IceCore;

#define INVALIDATE_RANKS	mCurrentSize|=0x80000000
#define VALIDATE_RANKS		mCurrentSize&=0x7fffffff
#define CURRENT_SIZE		(mCurrentSize&0x7fffffff)
#define INVALID_RANKS		(mCurrentSize&0x80000000)

#define CHECK_RESIZE(n)																		\
	if(n!=mPreviousSize)																	\
	{																						\
				if(n>mCurrentSize)	Resize(n);												\
		else						ResetRanks();											\
		mPreviousSize = n;																	\
	}

#define CREATE_HISTOGRAMS(type, buffer)														\
	/* Clear counters/histograms */															\
	ZeroMemory(mHistogram, 256*4*sizeof(udword));											\
																							\
	/* Prepare to count */																	\
	ubyte* p = (ubyte*)input;																\
	ubyte* pe = &p[nb*4];																	\
	udword* h0= &mHistogram[0];		/* Histogram for first pass (LSB)	*/					\
	udword* h1= &mHistogram[256];	/* Histogram for second pass		*/					\
	udword* h2= &mHistogram[512];	/* Histogram for third pass			*/					\
	udword* h3= &mHistogram[768];	/* Histogram for last pass (MSB)	*/					\
																							\
	bool AlreadySorted = true;	/* Optimism... */											\
																							\
	if(INVALID_RANKS)																		\
	{																						\
		/* Prepare for temporal coherence */												\
		type* Running = (type*)buffer;														\
		type PrevVal = *Running;															\
																							\
		while(p!=pe)																		\
		{																					\
			/* Read input buffer in previous sorted order */								\
			type Val = *Running++;															\
			/* Check whether already sorted or not */										\
			if(Val<PrevVal)	{ AlreadySorted = false; break; } /* Early out */				\
			/* Update for next iteration */													\
			PrevVal = Val;																	\
																							\
			/* Create histograms */															\
			h0[*p++]++;	h1[*p++]++;	h2[*p++]++;	h3[*p++]++;									\
		}																					\
																							\
		/* If all input values are already sorted, we just have to return and leave the */	\
		/* previous list unchanged. That way the routine may take advantage of temporal */	\
		/* coherence, for example when used to sort transparent faces.					*/	\
		if(AlreadySorted)																	\
		{																					\
			mNbHits++;																		\
			for(udword i=0;i<nb;i++)	mRanks[i] = i;										\
			return *this;																	\
		}																					\
	}																						\
	else																					\
	{																						\
		/* Prepare for temporal coherence */												\
		udword* Indices = mRanks;															\
		type PrevVal = (type)buffer[*Indices];												\
																							\
		while(p!=pe)																		\
		{																					\
			/* Read input buffer in previous sorted order */								\
			type Val = (type)buffer[*Indices++];											\
			/* Check whether already sorted or not */										\
			if(Val<PrevVal)	{ AlreadySorted = false; break; } /* Early out */				\
			/* Update for next iteration */													\
			PrevVal = Val;																	\
																							\
			/* Create histograms */															\
			h0[*p++]++;	h1[*p++]++;	h2[*p++]++;	h3[*p++]++;									\
		}																					\
																							\
		/* If all input values are already sorted, we just have to return and leave the */	\
		/* previous list unchanged. That way the routine may take advantage of temporal */	\
		/* coherence, for example when used to sort transparent faces.					*/	\
		if(AlreadySorted)	{ mNbHits++; return *this;	}									\
	}																						\
																							\
	/* Else there has been an early out and we must finish computing the histograms */		\
	while(p!=pe)																			\
	{																						\
		/* Create histograms without the previous overhead */								\
		h0[*p++]++;	h1[*p++]++;	h2[*p++]++;	h3[*p++]++;										\
	}

#define CHECK_PASS_VALIDITY(pass)															\
	/* Shortcut to current counters */														\
	udword* CurCount = &mHistogram[pass<<8];												\
																							\
	/* Reset flag. The sorting pass is supposed to be performed. (default) */				\
	bool PerformPass = true;																\
																							\
	/* Check pass validity */																\
																							\
	/* If all values have the same byte, sorting is useless. */								\
	/* It may happen when sorting bytes or words instead of dwords. */						\
	/* This routine actually sorts words faster than dwords, and bytes */					\
	/* faster than words. Standard running time (O(4*n))is reduced to O(2*n) */				\
	/* for words and O(n) for bytes. Running time for floats depends on actual values... */	\
																							\
	/* Get first byte */																	\
	ubyte UniqueVal = *(((ubyte*)input)+pass);												\
																							\
	/* Check that byte's counter */															\
	if(CurCount[UniqueVal]==nb)	PerformPass=false;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Constructor.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
RadixSort::RadixSort() : mRanks(null), mRanks2(null), mCurrentSize(0), mTotalCalls(0), mNbHits(0), mDeleteRanks(true)	// NOVODEX [1]
{
#ifndef RADIX_LOCAL_RAM
	// Allocate input-independent ram
	mHistogram	= new udword[256*4];
	mOffset		= new udword[256];
#endif
	// Initialize indices
	INVALIDATE_RANKS;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Destructor.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
RadixSort::~RadixSort()
{
	// Release everything
#ifndef RADIX_LOCAL_RAM
	DELETEARRAY(mOffset);
	DELETEARRAY(mHistogram);
#endif
	// NOVODEX [1]: borrowed rank buffers are not ours to free.
	if(mDeleteRanks)
	{
		if(mRanks2) { opcNovodeXFree(mRanks2); mRanks2 = null; }
		if(mRanks)  { opcNovodeXFree(mRanks);  mRanks  = null; }
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Resizes the inner lists.
 *	\param		nb	[in] new size (number of dwords)
 *	\return		true if success
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool RadixSort::Resize(udword nb)
{
	// Free previously used ram
	// NOVODEX [2]: the same guard, and it does NOT restore the marker -- buffers
	// allocated after a borrow are leaked by ~RadixSort.
	if(mDeleteRanks)
	{
		if(mRanks2) { opcNovodeXFree(mRanks2); mRanks2 = null; }
		if(mRanks)  { opcNovodeXFree(mRanks);  mRanks  = null; }
	}

	// Get some fresh one
	mRanks	= (udword*) opcNovodeXAlloc(nb*sizeof(udword));	CHECKALLOC(mRanks);	// NOVODEX [3]
	mRanks2	= (udword*) opcNovodeXAlloc(nb*sizeof(udword));	CHECKALLOC(mRanks2);	// NOVODEX [3]

	return true;
}

inline_ void RadixSort::CheckResize(udword nb)
{
	udword CurSize = CURRENT_SIZE;
	if(nb!=CurSize)
	{
		if(nb>CurSize)	Resize(nb);
		mCurrentSize = nb;
		INVALIDATE_RANKS;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Main sort routine.
 *	This one is for integer values. After the call, mRanks contains a list of indices in sorted order, i.e. in the order you may process your data.
 *	\param		input	[in] a list of integer values to sort
 *	\param		nb		[in] number of values to sort, must be < 2^31
 *	\param		hint	[in] RADIX_SIGNED to handle negative values, RADIX_UNSIGNED if you know your input buffer only contains positive values
 *	\return		Self-Reference
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
RadixSort& RadixSort::Sort(const udword* input, udword nb, RadixHint hint)
{
	// Checkings
	if(!input || !nb || nb&0x80000000)	return *this;

	// Stats
	mTotalCalls++;

	// Resize lists if needed
	CheckResize(nb);

#ifdef RADIX_LOCAL_RAM
	// Allocate histograms & offsets on the stack
	udword mHistogram[256*4];
//	udword mOffset[256];
	udword* mLink[256];
#endif

	// Create histograms (counters). Counters for all passes are created in one run.
	// Pros:	read input buffer once instead of four times
	// Cons:	mHistogram is 4Kb instead of 1Kb
	// We must take care of signed/unsigned values for temporal coherence.... I just
	// have 2 code paths even if just a single opcode changes. Self-modifying code, someone?
	if(hint==RADIX_UNSIGNED)	{ CREATE_HISTOGRAMS(udword, input);	}
	else						{ CREATE_HISTOGRAMS(sdword, input);	}

	// Compute #negative values involved if needed
	udword NbNegativeValues = 0;
	if(hint==RADIX_SIGNED)
	{
		// An efficient way to compute the number of negatives values we'll have to deal with is simply to sum the 128
		// last values of the last histogram. Last histogram because that's the one for the Most Significant Byte,
		// responsible for the sign. 128 last values because the 128 first ones are related to positive numbers.
		udword* h3= &mHistogram[768];
		for(udword i=128;i<256;i++)	NbNegativeValues += h3[i];	// 768 for last histogram, 128 for negative part
	}

	// Radix sort, j is the pass number (0=LSB, 3=MSB)
	for(udword j=0;j<4;j++)
	{
		CHECK_PASS_VALIDITY(j);

		// Sometimes the fourth (negative) pass is skipped because all numbers are negative and the MSB is 0xFF (for example). This is
		// not a problem, numbers are correctly sorted anyway.
		if(PerformPass)
		{
			// Should we care about negative values?
			if(j!=3 || hint==RADIX_UNSIGNED)
			{
				// Here we deal with positive values only

				// Create offsets
//				mOffset[0] = 0;
//				for(udword i=1;i<256;i++)		mOffset[i] = mOffset[i-1] + CurCount[i-1];
				mLink[0] = mRanks2;
				for(udword i=1;i<256;i++)		mLink[i] = mLink[i-1] + CurCount[i-1];
			}
			else
			{
				// This is a special case to correctly handle negative integers. They're sorted in the right order but at the wrong place.

				// Create biased offsets, in order for negative numbers to be sorted as well
//				mOffset[0] = NbNegativeValues;												// First positive number takes place after the negative ones
				mLink[0] = &mRanks2[NbNegativeValues];										// First positive number takes place after the negative ones
//				for(udword i=1;i<128;i++)		mOffset[i] = mOffset[i-1] + CurCount[i-1];	// 1 to 128 for positive numbers
				for(udword i=1;i<128;i++)		mLink[i] = mLink[i-1] + CurCount[i-1];		// 1 to 128 for positive numbers

				// Fixing the wrong place for negative values
//				mOffset[128] = 0;
				mLink[128] = mRanks2;
//				for(i=129;i<256;i++)			mOffset[i] = mOffset[i-1] + CurCount[i-1];
				for(udword i=129;i<256;i++)		mLink[i] = mLink[i-1] + CurCount[i-1];
			}

			// Perform Radix Sort
			ubyte* InputBytes	= (ubyte*)input;
			InputBytes += j;
			if(INVALID_RANKS)
			{
//				for(udword i=0;i<nb;i++)	mRanks2[mOffset[InputBytes[i<<2]]++] = i;
				for(udword i=0;i<nb;i++)	*mLink[InputBytes[i<<2]]++ = i;
				VALIDATE_RANKS;
			}
			else
			{
				udword* Indices		= mRanks;
				udword* IndicesEnd	= &mRanks[nb];
				while(Indices!=IndicesEnd)
				{
					udword id = *Indices++;
//					mRanks2[mOffset[InputBytes[id<<2]]++] = id;
					*mLink[InputBytes[id<<2]]++ = id;
				}
			}

			// Swap pointers for next pass. Valid indices - the most recent ones - are in mRanks after the swap.
			udword* Tmp	= mRanks;	mRanks = mRanks2; mRanks2 = Tmp;
		}
	}
	return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Main sort routine.
 *	This one is for floating-point values. After the call, mRanks contains a list of indices in sorted order, i.e. in the order you may process your data.
 *	\param		input			[in] a list of floating-point values to sort
 *	\param		nb				[in] number of values to sort, must be < 2^31
 *	\return		Self-Reference
 *	\warning	only sorts IEEE floating-point values
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
RadixSort& RadixSort::Sort(const float* input2, udword nb)
{
	// Checkings
	if(!input2 || !nb || nb&0x80000000)	return *this;

	// Stats
	mTotalCalls++;

	udword* input = (udword*)input2;

	// Resize lists if needed
	CheckResize(nb);

#ifdef RADIX_LOCAL_RAM
	// Allocate histograms & offsets on the stack
	udword mHistogram[256*4];
//	udword mOffset[256];
	udword* mLink[256];
#endif

	// Create histograms (counters). Counters for all passes are created in one run.
	// Pros:	read input buffer once instead of four times
	// Cons:	mHistogram is 4Kb instead of 1Kb
	// Floating-point values are always supposed to be signed values, so there's only one code path there.
	// Please note the floating point comparison needed for temporal coherence! Although the resulting asm code
	// is dreadful, this is surprisingly not such a performance hit - well, I suppose that's a big one on first
	// generation Pentiums....We can't make comparison on integer representations because, as Chris said, it just
	// wouldn't work with mixed positive/negative values....
	{ CREATE_HISTOGRAMS(float, input2); }

	// Compute #negative values involved if needed
	udword NbNegativeValues = 0;
	// An efficient way to compute the number of negatives values we'll have to deal with is simply to sum the 128
	// last values of the last histogram. Last histogram because that's the one for the Most Significant Byte,
	// responsible for the sign. 128 last values because the 128 first ones are related to positive numbers.
	udword* h3= &mHistogram[768];
	for(udword i=128;i<256;i++)	NbNegativeValues += h3[i];	// 768 for last histogram, 128 for negative part

	// Radix sort, j is the pass number (0=LSB, 3=MSB)
	for(udword j=0;j<4;j++)
	{
		// Should we care about negative values?
		if(j!=3)
		{
			// Here we deal with positive values only
			CHECK_PASS_VALIDITY(j);

			if(PerformPass)
			{
				// Create offsets
//				mOffset[0] = 0;
				mLink[0] = mRanks2;
//				for(udword i=1;i<256;i++)		mOffset[i] = mOffset[i-1] + CurCount[i-1];
				for(udword i=1;i<256;i++)		mLink[i] = mLink[i-1] + CurCount[i-1];

				// Perform Radix Sort
				ubyte* InputBytes = (ubyte*)input;
				InputBytes += j;
				if(INVALID_RANKS)
				{
//					for(i=0;i<nb;i++)	mRanks2[mOffset[InputBytes[i<<2]]++] = i;
					for(udword i=0;i<nb;i++)	*mLink[InputBytes[i<<2]]++ = i;
					VALIDATE_RANKS;
				}
				else
				{
					udword* Indices		= mRanks;
					udword* IndicesEnd	= &mRanks[nb];
					while(Indices!=IndicesEnd)
					{
						udword id = *Indices++;
//						mRanks2[mOffset[InputBytes[id<<2]]++] = id;
						*mLink[InputBytes[id<<2]]++ = id;
					}
				}

				// Swap pointers for next pass. Valid indices - the most recent ones - are in mRanks after the swap.
				udword* Tmp	= mRanks;	mRanks = mRanks2; mRanks2 = Tmp;
			}
		}
		else
		{
			// This is a special case to correctly handle negative values
			CHECK_PASS_VALIDITY(j);

			if(PerformPass)
			{
				// Create biased offsets, in order for negative numbers to be sorted as well
//				mOffset[0] = NbNegativeValues;												// First positive number takes place after the negative ones
				mLink[0] = &mRanks2[NbNegativeValues];										// First positive number takes place after the negative ones
//				for(udword i=1;i<128;i++)		mOffset[i] = mOffset[i-1] + CurCount[i-1];	// 1 to 128 for positive numbers
				for(udword i=1;i<128;i++)		mLink[i] = mLink[i-1] + CurCount[i-1];		// 1 to 128 for positive numbers

				// We must reverse the sorting order for negative numbers!
//				mOffset[255] = 0;
				mLink[255] = mRanks2;
//				for(i=0;i<127;i++)		mOffset[254-i] = mOffset[255-i] + CurCount[255-i];	// Fixing the wrong order for negative values
				for(udword i=0;i<127;i++)	mLink[254-i] = mLink[255-i] + CurCount[255-i];		// Fixing the wrong order for negative values
//				for(i=128;i<256;i++)	mOffset[i] += CurCount[i];							// Fixing the wrong place for negative values
				for(udword i=128;i<256;i++)	mLink[i] += CurCount[i];							// Fixing the wrong place for negative values

				// Perform Radix Sort
				if(INVALID_RANKS)
				{
					for(udword i=0;i<nb;i++)
					{
						udword Radix = input[i]>>24;							// Radix byte, same as above. AND is useless here (udword).
						// ### cmp to be killed. Not good. Later.
//						if(Radix<128)		mRanks2[mOffset[Radix]++] = i;		// Number is positive, same as above
//						else				mRanks2[--mOffset[Radix]] = i;		// Number is negative, flip the sorting order
						if(Radix<128)		*mLink[Radix]++ = i;		// Number is positive, same as above
						else				*(--mLink[Radix]) = i;		// Number is negative, flip the sorting order
					}
					VALIDATE_RANKS;
				}
				else
				{
					for(udword i=0;i<nb;i++)
					{
						udword Radix = input[mRanks[i]]>>24;							// Radix byte, same as above. AND is useless here (udword).
						// ### cmp to be killed. Not good. Later.
//						if(Radix<128)		mRanks2[mOffset[Radix]++] = mRanks[i];		// Number is positive, same as above
//						else				mRanks2[--mOffset[Radix]] = mRanks[i];		// Number is negative, flip the sorting order
						if(Radix<128)		*mLink[Radix]++ = mRanks[i];		// Number is positive, same as above
						else				*(--mLink[Radix]) = mRanks[i];		// Number is negative, flip the sorting order
					}
				}
				// Swap pointers for next pass. Valid indices - the most recent ones - are in mRanks after the swap.
				udword* Tmp	= mRanks;	mRanks = mRanks2; mRanks2 = Tmp;
			}
			else
			{
				// The pass is useless, yet we still have to reverse the order of current list if all values are negative.
				if(UniqueVal>=128)
				{
					if(INVALID_RANKS)
					{
						// ###Possible?
						for(udword i=0;i<nb;i++)	mRanks2[i] = nb-i-1;
						VALIDATE_RANKS;
					}
					else
					{
						for(udword i=0;i<nb;i++)	mRanks2[i] = mRanks[nb-i-1];
					}

					// Swap pointers for next pass. Valid indices - the most recent ones - are in mRanks after the swap.
					udword* Tmp	= mRanks;	mRanks = mRanks2; mRanks2 = Tmp;
				}
			}
		}
	}
	return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Gets the ram used.
 *	\return		memory used in bytes
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
udword RadixSort::GetUsedRam() const
{
	udword UsedRam = sizeof(RadixSort);
#ifndef RADIX_LOCAL_RAM
	UsedRam += 256*4*sizeof(udword);			// Histograms
	UsedRam += 256*sizeof(udword);				// Offsets
#endif
	UsedRam += 2*CURRENT_SIZE*sizeof(udword);	// 2 lists of indices
	return UsedRam;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	NOVODEX ADDITION [4]. Lends the sorter two rank buffers it does not own.
 *
 *	Reconstructed from phys_fn_005177 at 0x000e3ea0, 36 bytes, __thiscall, ret 8.
 *	The whole row is:
 *
 *	    mov edx,[esp+4]      ranks1
 *	    test edx,edx
 *	    je   fail            -- rejected BEFORE either member is written
 *	    mov eax,[esp+8]      ranks2
 *	    test eax,eax
 *	    je   fail
 *	    mov [ecx+8],eax      mRanks2 first
 *	    mov [ecx+4],edx      then mRanks
 *	    mov byte [ecx+0x14],0  mDeleteRanks = false -- the marker [1] guards
 *	    mov al,1 / ret 8
 *	  fail:
 *	    xor al,al / ret 8
 *
 *	Three things the row establishes and this body keeps: both arguments are
 *	tested before ANY member is written, so a rejected call leaves the object
 *	untouched; the two pointers are stored mRanks2 first; and mCurrentSize is NOT
 *	invalidated, so the borrowed buffers are adopted at whatever size the sorter
 *	already believes it has.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool RadixSort::SetRankBuffers(udword* ranks1, udword* ranks2)
{
	if(!ranks1)	return false;
	if(!ranks2)	return false;

	mRanks2			= ranks2;
	mRanks			= ranks1;
	mDeleteRanks	= false;
	return true;
}
