///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains a custom array class.
 *	\file		IceCustomArray.h
 *	\author		Pierre Terdiman
 *	\date		January, 15, 1999
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Include Guard
#ifndef __ICECUSTOMARRAY_H__
#define __ICECUSTOMARRAY_H__

//#include "NxCollisionSDK.h"
#include "NxFoundationSDK.h"
#include <stdio.h>

	//! Default size of a memory block
	#define CUSTOMARRAY_BLOCKSIZE	(4*1024)		// 4 Kb => heap size

	//! A memory block
	class CustomBlock
	{
		public:
							CustomBlock();
							~CustomBlock();

		void*				Addy;					//!< Stored data
		NxU32				Size;					//!< Length of stored data
		NxU32				Max;					//!< Heap size
	};

	//! A linked list of blocks
	class CustomCell
	{
		public:
							CustomCell();
							~CustomCell();

		CustomCell*			NextCustomCell;
		CustomBlock			Item;
	};

	class CustomArray
	{
		public:
		// Constructor / Destructor
								CustomArray(NxU32 start_size=CUSTOMARRAY_BLOCKSIZE, void* input_buffer=NULL);
								CustomArray(CustomArray& array);
								CustomArray(const char* filename);
								~CustomArray();

				CustomArray&	Clean();

		// Store methods

			// Store a BOOL
				CustomArray&	Store(NX_BOOL bo);
			// Store a bool
				CustomArray&	Store(bool bo);

			// Store a char (different from "signed char" AND "unsigned char")
				CustomArray&	Store(char b);
			// Store a NxU8
		NX_INLINE	CustomArray&	Store(NxU8 b)
								{
									NX_ASSERT(sizeof(NxU8)==sizeof(char));
									return Store(char(b));
								}
			// Store a NxI8
		NX_INLINE	CustomArray&	Store(NxI8 b)
								{
									NX_ASSERT(sizeof(NxI8)==sizeof(char));
									return Store(char(b));
								}

			// Store a short / sword
				CustomArray&	Store(short w);
			// Store a NxU16
		NX_INLINE	CustomArray&	Store(NxU16 w)
								{
									NX_ASSERT(sizeof(NxU16)==sizeof(short));
									return Store(short(w));
								}

			// Store a unsigned int / NxU32. "int / sdword" is handled as a BOOL.
				CustomArray&	Store(NxU32 d);
			// Store a long
		NX_INLINE	CustomArray&	Store(long d)
								{
									NX_ASSERT(sizeof(long)==sizeof(NxU32));
									return Store(NxU32(d));
								}
			// Store a unsigned long
		NX_INLINE	CustomArray&	Store(unsigned long d)
								{
									NX_ASSERT(sizeof(unsigned long)==sizeof(NxU32));
									return Store(NxU32(d));
								}

			// Store a float
				CustomArray&	Store(float f);
			// Store a double
				CustomArray&	Store(double f);

			// Store a string
				CustomArray&	Store(const char* string);
			// Store a buffer
				CustomArray&	Store(void* buffer, NxU32 size);
			// Store a CustomArray
				CustomArray&	Store(CustomArray* array);

		// ASCII store methods

			// Store a BOOL in ASCII
				CustomArray&	StoreASCII(NX_BOOL bo);
			// Store a bool in ASCII
				CustomArray&	StoreASCII(bool bo);

			// Store a char in ASCII
				CustomArray&	StoreASCII(char b);
			// Store a NxI8 in ASCII
		NX_INLINE	CustomArray&	StoreASCII(NxI8 b)		{ return StoreASCII(char(b));	}
			// Store a NxU8 in ASCII
				CustomArray&	StoreASCII(NxU8 b);

			// Store a short in ASCII
				CustomArray&	StoreASCII(short w);
			// Store a NxU16 in ASCII
				CustomArray&	StoreASCII(NxU16 w);

			// Store a long in ASCII
				CustomArray&	StoreASCII(long d);
			// Store a unsigned long in ASCII
				CustomArray&	StoreASCII(unsigned long d);

			// Store a int in ASCII
			//	CustomArray&	StoreASCII(int d);
			// Store a unsigned int in ASCII
				CustomArray&	StoreASCII(unsigned int d);

			// Store a float in ASCII
				CustomArray&	StoreASCII(float f);
			// Store a double in ASCII
				CustomArray&	StoreASCII(double f);

			// Store a string in ASCII
				CustomArray&	StoreASCII(const char* string);
			// Store a formated string in ASCII
//				CustomArray&	StoreASCII(LPSTR fmt, ...);

		// Bit storage - inlined since mostly used in data compression where speed is welcome

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Stores a bit.
		 *	\param		bit		[in] the bit to store
		 *	\return		Self-Reference
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		NX_INLINE	CustomArray&	StoreBit(bool bit)
								{
									mBitMask<<=1;				// Shift current bitmask
//									if(bit)	mBitMask |= 1;		// Set the LSB if needed
									mBitMask |= (NxU8)bit;		// Set the LSB if needed - no conditional branch here
									mBitCount++;				// Count number of active bits in bitmask
									if(mBitCount==8)			// If bitmask is full...
									{
										mBitCount = 0;			// ...then reset counter...
										Store(mBitMask);		// ...and store bitmask.
									}
									return *this;
								}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Store bits.
		 *	\param		bits	[in] the bit pattern to store
		 *	\param		nb_bits	[in] the number of bits to store
		 *	\return		Self-Reference
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		NX_INLINE	CustomArray&	StoreBits(NxU32 bits, NxU32 nb_bits)
								{
									NxU32 Mask=1<<(nb_bits-1);
									while(Mask)
									{
										StoreBit((bits&Mask)!=0);
										Mask>>=1;
									}
									return *this;
								}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Reads a bit.
		 *	\return		the bit
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		NX_INLINE	bool			GetBit()	const
								{
									if(!mBitCount)
									{
										mBitMask = GetByte();
										mBitCount = 0x80;
									}
									bool Bit = (mBitMask&mBitCount)!=0;
									mBitCount>>=1;
									return Bit;
								}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Reads bits.
		 *	\param		nb_bits		[in] the number of bits to read
		 *	\return		the bits
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		NX_INLINE	NxU32			GetBits(NxU32 nb_bits)	const
								{
									NxU32 Bits = 0;
									while(nb_bits--)
									{
										Bits<<=1;
										bool Bit = GetBit();
										Bits|=NxU32(Bit);
									}
									return Bits;
								}

		// Padds extra bits to reach a byte
				CustomArray&	EndBits();

		// Management methods
				bool			ExportToDisk(const char* filename, const char* access=NULL);
				bool			ExportToDisk(FILE* fp);

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Gets current number of bytes stored.
		 *	\return		number of bytes stored
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				NxU32			GetOffset()	const;
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Padds offset on a 8 bytes boundary.
		 *	\return		Self-Reference
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				CustomArray&	Padd();
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Collapses a CustomArray into a single continuous buffer. This invalidates all pushed addies.
		 *	If you provide your destination buffer original bytes are copied into it, then it's safe using them.
		 *	If you don't, returned address is valid until the array's destructor is called. Beware of memory corruption...
		 *	\param		user_buffer		[out] destination buffer (provided or not)
		 *	\return		destination buffer
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				void*			Collapse(void* user_buffer=NULL);

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Gets the current address within the current block.
		 *	\return		destination buffer
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		NX_INLINE	void*			GetAddress()	const
								{
									char* CurrentAddy = (char*)mCurrentCell->Item.Addy;
									CurrentAddy+=mCurrentCell->Item.Size;
									return CurrentAddy;
								}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Gets used size within current block.
		 *	\return		used size / offset
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		NX_INLINE	NxU32			GetCellUsedSize()	const
								{
									return mCurrentCell->Item.Size;
								}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Gets max size within current block.
		 *	\return		max size / limit
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		NX_INLINE	NxU32			GetCellMaxSize()	const
								{
									return mCurrentCell->Item.Max;
								}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Gets remaining size within current block.
		 *	\return		remaining size / limit - offset
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		NX_INLINE	NxU32			GetCellRemainingSize()	const
								{
									return mCurrentCell->Item.Max - mCurrentCell->Item.Size;
								}

		// Address methods
				bool			PushAddress();
				CustomArray&	PopAddressAndStore(NX_BOOL Bo);
				CustomArray&	PopAddressAndStore(bool Bo);
				CustomArray&	PopAddressAndStore(char b);
				CustomArray&	PopAddressAndStore(NxU8 b);
				CustomArray&	PopAddressAndStore(short w);
				CustomArray&	PopAddressAndStore(NxU16 w);
				CustomArray&	PopAddressAndStore(long d);
				CustomArray&	PopAddressAndStore(unsigned long d);
			//	CustomArray&	PopAddressAndStore(int d);
				CustomArray&	PopAddressAndStore(unsigned int d);
				CustomArray&	PopAddressAndStore(float f);
				CustomArray&	PopAddressAndStore(double f);

		// Read methods
				NxU8			GetByte()	const;			//!< Read a byte from the array
				NxU16			GetWord()	const;			//!< Read a word from the array
				NxU32			GetDword()	const;			//!< Read a dword from the array
				float			GetFloat()	const;			//!< Read a float from the array
				NxU8*			GetString()	const;			//!< Read a string from the array

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Sets the current address within current block. Input offset can't be greater than current block's length.
		 *	\param		offset	[in] the wanted offset within the block.
		 *	\return		Self-Reference.
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				CustomArray&	Reset(NxU32 offset=0);
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Finds a given chunk (a sequence of bytes) in current buffer.
		 *	\param		chunk	[in] chunk you're looking for (must end with a NULL byte).
		 *	\return		address where the chunk has been found, or NULL if not found.
		 *	\warning	chunk length limited to 1024 bytes...
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				void*			GetChunk(const char* chunk);

				CustomArray&	operator=(CustomArray& array);
		private:
				CustomCell*		mCurrentCell;				//!< Current block cell
				CustomCell*		mInitCell;					//!< First block cell

				void*			mCollapsed;					//!< Possible collapsed buffer
				void**			mAddresses;					//!< Stack to store addresses
				void*			mLastAddress;				//!< Last address used in current block cell
				NxU16			mNbPushedAddies;			//!< Number of saved addies
				NxU16			mNbAllocatedAddies;			//!< Number of allocated addies
		// Bit storage
		mutable	NxU8			mBitCount;					//!< Current number of valid bits in the bitmask
		mutable	NxU8			mBitMask;					//!< Current bitmask
		// Management methods
				void			Init(NxU32 start_size, void* input_buffer, FILE* fp, NxU32 used_size);
				CustomArray&	Empty();
				CustomArray&	CheckArray(NxU32 bytes_needed);
				bool			NewBlock(CustomCell* previous_cell, NxU32 size=0);
				bool			SaveCell(CustomCell* p, FILE* fp);
				CustomArray&	StoreASCIICode(char code);
				void*			PrepareAccess(NxU32 size);
	};

#endif // __ICECUSTOMARRAY_H__
