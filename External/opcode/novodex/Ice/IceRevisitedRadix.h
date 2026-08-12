/*
 * NOVODEX LOCAL MODIFICATION
 * upstream: External/opcode/upstream/Opcode/Ice/IceRevisitedRadix.h
 *
 * [1] One bool added after mNbHits: the same borrowed-buffer marker Container got,
 *     at offset +0x14 -- one byte past the end of the stock object, which is
 *     exactly five udwords. Neither this member nor any ownership flag occurs
 *     anywhere in either pinned OPCODE tree.
 *     established at 0x000e32c0 RadixSort::RadixSort zeroes +4/+8/+0x0c/+0x10, writes
 *     `mov byte ptr [eax+0x14], 1`, then 0x80000000 to mCurrentSize
 *     (INVALIDATE_RANKS). 0x000e32e3 is `mov al,[esi+0x14]; test al,al; je` and
 *     the destructor frees mRanks2 then mRanks only inside it. 0x000e3333 is the
 *     same guard in Resize.
 *
 * [4] SetRankBuffers, the row that CLEARS the marker. Reconstructed by P4 Task 2b.
 *     established at 0x000e3ea0 phys_fn_005177, 36 bytes, __thiscall, `ret 8`.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains source code from the article "Radix Sort Revisited".
 *	\file		IceRevisitedRadix.h
 *	\author		Pierre Terdiman
 *	\date		April, 4, 2000
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Include Guard
#ifndef __ICERADIXSORT_H__
#define __ICERADIXSORT_H__

	//! Allocate histograms & offsets locally
	#define RADIX_LOCAL_RAM

	enum RadixHint
	{
		RADIX_SIGNED,		//!< Input values are signed
		RADIX_UNSIGNED,		//!< Input values are unsigned

		RADIX_FORCE_DWORD = 0x7fffffff
	};

	class ICECORE_API RadixSort
	{
		public:
		// Constructor/Destructor
								RadixSort();
								~RadixSort();
		// Sorting methods
				RadixSort&		Sort(const udword* input, udword nb, RadixHint hint=RADIX_SIGNED);
				RadixSort&		Sort(const float* input, udword nb);

		//! Access to results. mRanks is a list of indices in sorted order, i.e. in the order you may further process your data
		inline_	const udword*	GetRanks()			const	{ return mRanks;		}

		//! mIndices2 gets trashed on calling the sort routine, but otherwise you can recycle it the way you want.
		inline_	udword*			GetRecyclable()		const	{ return mRanks2;		}

		//! NOVODEX ADDITION: lend the sorter two rank buffers it does not own. See [4].
				bool			SetRankBuffers(udword* ranks1, udword* ranks2);

		// Stats
				udword			GetUsedRam()		const;
		//! Returns the total number of calls to the radix sorter.
		inline_	udword			GetNbTotalCalls()	const	{ return mTotalCalls;	}
		//! Returns the number of eraly exits due to temporal coherence.
		inline_	udword			GetNbHits()			const	{ return mNbHits;		}

		private:
#ifndef RADIX_LOCAL_RAM
				udword*			mHistogram;			//!< Counters for each byte
				udword*			mOffset;			//!< Offsets (nearly a cumulative distribution function)
#endif
				udword			mCurrentSize;		//!< Current size of the indices list
				udword*			mRanks;				//!< Two lists, swapped each pass
				udword*			mRanks2;
		// Stats
				udword			mTotalCalls;		//!< Total number of calls to the sort routine
				udword			mNbHits;			//!< Number of early exits due to coherence
				bool			mDeleteRanks;		//!< NOVODEX: false => mRanks/mRanks2 are borrowed. See [1].
		// Internal methods
				void			CheckResize(udword nb);
				bool			Resize(udword nb);
	};

#endif // __ICERADIXSORT_H__
