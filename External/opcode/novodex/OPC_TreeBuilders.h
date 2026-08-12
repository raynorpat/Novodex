/*
 * NOVODEX LOCAL MODIFICATION
 * upstream: External/opcode/upstream/Opcode/OPC_TreeBuilders.h
 *
 * [1] BuildSettings grows from 8 bytes to 20: three udwords added after mRules.
 *     Consequence: sizeof(OPCODECREATE) 16 -> 32 and
 *     sizeof(AABBTreeOfTrianglesBuilder) 32 -> 72. The three are NOT named,
 *     because nothing measured says what they are. An earlier pass read two of
 *     them as a height-field axis and extent; that reading is consistent with
 *     the measurement but is not established by it.
 *     established at 0x000e92b0 OPCODECREATE::OPCODECREATE runs the sub-object's constructor
 *     FIRST -- +0x0c=0x7fffffff, +0x10=0, +0x14=0xffffffff, +0x18=0, +0x08=1 --
 *     and only then its own body, which is where a member sub-object lands.
 *     0x000e9060 AABBTreeOfTrianglesBuilder's constructor writes the same five
 *     constants at the same relative offsets and contains no OPCODECREATE.
 *     0x000e91cc-0x000e91fd copies exactly those five dwords as a unit into the
 *     builder, then mNbPrimitives -- a 20-byte struct assignment.
 *     0x000e9129 reads mLimit at OPCODECREATE+0x08, not +0x04.
 * [2] AABBTreeBuilder gained 28 bytes between mNodeBase and mCount, so
 *     sizeof(AABBTreeOfTrianglesBuilder) is 72 rather than the 44 the modified
 *     BuildSettings alone would give. NOT in the eleven modifications the
 *     evidence documents record; found by this task when the vendored header
 *     produced 44.
 *     established at 0x000e9060, the builder's constructor, which writes
 *     mSettings' five dwords at +0x04..+0x14, then mNbPrimitives at +0x18 and
 *     mNodeBase at +0x1c, and then jumps to +0x3c and +0x40 -- exactly the four
 *     members stock initialises, so mCount is at +0x3c and mNbInvalidSplits at
 *     +0x40, not at +0x20 and +0x24. Model::Build writes mIMesh at builder+0x44
 *     (0x000e91d1, `mov [esp+0x50],eax` against `lea ecx,[esp+0x0c]` at
 *     0x000e91c1) and its frame is `sub esp,0x48` at 0x000e9100. The 28 bytes at
 *     +0x20..+0x3b are written by nothing this task disassembled.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 *	OPCODE - Optimized Collision Detection
 *	Copyright (C) 2001 Pierre Terdiman
 *	Homepage: http://www.codercorner.com/Opcode.htm
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains code for tree builders.
 *	\file		OPC_TreeBuilders.h
 *	\author		Pierre Terdiman
 *	\date		March, 20, 2001
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Include Guard
#ifndef __OPC_TREEBUILDERS_H__
#define __OPC_TREEBUILDERS_H__

	//! Tree splitting rules
	enum SplittingRules
	{
		// Primitive split
		SPLIT_LARGEST_AXIS		= (1<<0),		//!< Split along the largest axis
		SPLIT_SPLATTER_POINTS	= (1<<1),		//!< Splatter primitive centers (QuickCD-style)
		SPLIT_BEST_AXIS			= (1<<2),		//!< Try largest axis, then second, then last
		SPLIT_BALANCED			= (1<<3),		//!< Try to keep a well-balanced tree
		SPLIT_FIFTY				= (1<<4),		//!< Arbitrary 50-50 split
		// Node split
		SPLIT_GEOM_CENTER		= (1<<5),		//!< Split at geometric center (else split in the middle)
		//
		SPLIT_FORCE_DWORD		= 0x7fffffff
	};

	//! Simple wrapper around build-related settings [Opcode 1.3]
	struct OPCODE_API BuildSettings
	{
		// NOVODEX: three members added, see [1]. The initialiser list carries their
		// defaults, read out of OPCODECREATE::OPCODECREATE at 0x000e92b0.
		inline_	BuildSettings() : mLimit(1), mRules(SPLIT_FORCE_DWORD),
								 mNovodeXSetting08(0), mNovodeXSetting0c(0xffffffff),
								 mNovodeXSetting10(0)	{}

		udword	mLimit;		//!< Limit number of primitives / node. If limit is 1, build a complete tree (2*N-1 nodes)
		udword	mRules;		//!< Building/Splitting rules (a combination of SplittingRules flags)
		udword	mNovodeXSetting08;	//!< NOVODEX: added, default 0
		udword	mNovodeXSetting0c;	//!< NOVODEX: added, default 0xffffffff
		udword	mNovodeXSetting10;	//!< NOVODEX: added, default 0
	};

	class OPCODE_API AABBTreeBuilder
	{
		public:
		//! Constructor
													AABBTreeBuilder() :
														mNbPrimitives(0),
														mNodeBase(null),
														mCount(0),
														mNbInvalidSplits(0)		{}
		//! Destructor
		virtual										~AABBTreeBuilder()			{}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Computes the AABB of a set of primitives.
		 *	\param		primitives		[in] list of indices of primitives
		 *	\param		nb_prims		[in] number of indices
		 *	\param		global_box		[out] global AABB enclosing the set of input primitives
		 *	\return		true if success
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		virtual						bool			ComputeGlobalBox(const udword* primitives, udword nb_prims, AABB& global_box)	const	= 0;

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Computes the splitting value along a given axis for a given primitive.
		 *	\param		index			[in] index of the primitive to split
		 *	\param		axis			[in] axis index (0,1,2)
		 *	\return		splitting value
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		virtual						float			GetSplittingValue(udword index, udword axis)	const	= 0;

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Computes the splitting value along a given axis for a given node.
		 *	\param		primitives		[in] list of indices of primitives
		 *	\param		nb_prims		[in] number of indices
		 *	\param		global_box		[in] global AABB enclosing the set of input primitives
		 *	\param		axis			[in] axis index (0,1,2)
		 *	\return		splitting value
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		virtual						float			GetSplittingValue(const udword* primitives, udword nb_prims, const AABB& global_box, udword axis)	const
													{
														// Default split value = middle of the axis (using only the box)
														return global_box.GetCenter(axis);
													}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Validates node subdivision. This is called each time a node is considered for subdivision, during tree building.
		 *	\param		primitives		[in] list of indices of primitives
		 *	\param		nb_prims		[in] number of indices
		 *	\param		global_box		[in] global AABB enclosing the set of input primitives
		 *	\return		TRUE if the node should be subdivised
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		virtual						BOOL			ValidateSubdivision(const udword* primitives, udword nb_prims, const AABB& global_box)
													{
														// Check the user-defined limit
														if(nb_prims<=mSettings.mLimit)	return FALSE;

														return TRUE;
													}

									BuildSettings	mSettings;			//!< Splitting rules & split limit [Opcode 1.3]
									udword			mNbPrimitives;		//!< Total number of primitives.
									void*			mNodeBase;			//!< Address of node pool [Opcode 1.3]
									// NOVODEX: 28 bytes added here. See [2]. Unidentified: the
									// constructor never writes them, so nothing measured says what
									// they are. Named for their offset, not guessed at.
									udword			mNovodeXBuilder20[7];
		// Stats
		inline_						void			SetCount(udword nb)				{ mCount=nb;				}
		inline_						void			IncreaseCount(udword nb)		{ mCount+=nb;				}
		inline_						udword			GetCount()				const	{ return mCount;			}
		inline_						void			SetNbInvalidSplits(udword nb)	{ mNbInvalidSplits=nb;		}
		inline_						void			IncreaseNbInvalidSplits()		{ mNbInvalidSplits++;		}
		inline_						udword			GetNbInvalidSplits()	const	{ return mNbInvalidSplits;	}

		private:
									udword			mCount;				//!< Stats: number of nodes created
									udword			mNbInvalidSplits;	//!< Stats: number of invalid splits
	};

	class OPCODE_API AABBTreeOfVerticesBuilder : public AABBTreeBuilder
	{
		public:
		//! Constructor
													AABBTreeOfVerticesBuilder() : mVertexArray(null)	{}
		//! Destructor
		virtual										~AABBTreeOfVerticesBuilder()						{}

		override(AABBTreeBuilder)	bool			ComputeGlobalBox(const udword* primitives, udword nb_prims, AABB& global_box)	const;
		override(AABBTreeBuilder)	float			GetSplittingValue(udword index, udword axis)									const;
		override(AABBTreeBuilder)	float			GetSplittingValue(const udword* primitives, udword nb_prims, const AABB& global_box, udword axis)	const;

		const						Point*			mVertexArray;		//!< Shortcut to an app-controlled array of vertices.
	};

	class OPCODE_API AABBTreeOfAABBsBuilder : public AABBTreeBuilder
	{
		public:
		//! Constructor
													AABBTreeOfAABBsBuilder() : mAABBArray(null)	{}
		//! Destructor
		virtual										~AABBTreeOfAABBsBuilder()					{}

		override(AABBTreeBuilder)	bool			ComputeGlobalBox(const udword* primitives, udword nb_prims, AABB& global_box)	const;
		override(AABBTreeBuilder)	float			GetSplittingValue(udword index, udword axis)									const;

		const						AABB*			mAABBArray;			//!< Shortcut to an app-controlled array of AABBs.
	};

	class OPCODE_API AABBTreeOfTrianglesBuilder : public AABBTreeBuilder
	{
		public:
		//! Constructor
													AABBTreeOfTrianglesBuilder() : mIMesh(null)										{}
		//! Destructor
		virtual										~AABBTreeOfTrianglesBuilder()													{}

		override(AABBTreeBuilder)	bool			ComputeGlobalBox(const udword* primitives, udword nb_prims, AABB& global_box)	const;
		override(AABBTreeBuilder)	float			GetSplittingValue(udword index, udword axis)									const;
		override(AABBTreeBuilder)	float			GetSplittingValue(const udword* primitives, udword nb_prims, const AABB& global_box, udword axis)	const;

		const				MeshInterface*			mIMesh;			//!< Shortcut to an app-controlled mesh interface
	};

#endif // __OPC_TREEBUILDERS_H__
