/*
 * NOVODEX LOCAL MODIFICATION
 * upstream: External/opcode/upstream/Opcode/OPC_VolumeCollider.h
 *
 * [1] VolumeCache holds a Container* at +0 and the owning model at +4 (8 bytes; stock
 *     embeds the Container, 20 bytes), so every derived cache's own fields start at +8:
 *     SphereCache's Center, FatRadius2 and FatCoeff are read at +8, +0x14 and +0x18
 *     (0x000dea87, 0x000dea58, 0x000dea7f) and cache.Model at +4 (0x000dea64). The
 *     constructor stores null in both words: the NovodeX owner at 0x000e56d0 constructs a
 *     SphereCache at +0x50 and an AABBCache at +0x6c with both words zero (0x000e56ef,
 *     0x000e56f2, 0x000e5709, 0x000e570c) and then points both at its own Container at
 *     +0x40 (0x000e5731, 0x000e5734); 0x00059832..0x00059846 builds an AABBCache on the
 *     stack the same way. Whoever owns the cache supplies the Container; the colliders
 *     never allocate it. Established at the five InitQuery bodies, which load the pointer
 *     from the cache's first word and store it in mTouchedPrimitives (+0x10) with no null
 *     test: LSSCollider 0x000d36f2, OBBCollider 0x000d57ab, SphereCollider 0x000de925,
 *     PlanesCollider 0x000e173d, AABBCollider 0x000e9c00.
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
 *	Contains base volume collider class.
 *	\file		OPC_VolumeCollider.h
 *	\author		Pierre Terdiman
 *	\date		June, 2, 2001
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Include Guard
#ifndef __OPC_VOLUMECOLLIDER_H__
#define __OPC_VOLUMECOLLIDER_H__

	struct OPCODE_API VolumeCache
	{
							VolumeCache() : TouchedPrimitives(null), Model(null)	{}
							~VolumeCache()					{}

		Container*			TouchedPrimitives;	//!< Indices of touched primitives, owned by the caller
		const BaseModel*	Model;				//!< Owner
	};

	class OPCODE_API VolumeCollider : public Collider
	{
		public:
		// Constructor / Destructor
											VolumeCollider();
		virtual								~VolumeCollider() = 0;

		// Collision report

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Gets the number of touched primitives after a collision query.
		 *	\see		GetContactStatus()
		 *	\see		GetTouchedPrimitives()
		 *	\return		the number of touched primitives
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		inline_				udword			GetNbTouchedPrimitives()	const	{ return mTouchedPrimitives ? mTouchedPrimitives->GetNbEntries() : 0;	}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Gets the list of touched primitives after a collision query.
		 *	\see		GetContactStatus()
		 *	\see		GetNbTouchedPrimitives()
		 *	\return		the list of touched primitives (primitive indices)
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		inline_		const	udword*			GetTouchedPrimitives()		const	{ return mTouchedPrimitives ? mTouchedPrimitives->GetEntries() : null;	}

		// Stats

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Stats: gets the number of Volume-BV overlap tests after a collision query.
		 *	\see		GetNbVolumePrimTests()
		 *	\return		the number of Volume-BV tests performed during last query
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		inline_				udword			GetNbVolumeBVTests()		const	{ return mNbVolumeBVTests;												}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Stats: gets the number of Volume-Triangle overlap tests after a collision query.
		 *	\see		GetNbVolumeBVTests()
		 *	\return		the number of Volume-Triangle tests performed during last query
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		inline_				udword			GetNbVolumePrimTests()		const	{ return mNbVolumePrimTests;											}

		// Settings

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Validates current settings. You should call this method after all the settings / callbacks have been defined for a collider.
		 *	\return		null if everything is ok, else a string describing the problem
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		override(Collider)	const char*		ValidateSettings();

		protected:
		// Touched primitives
							Container*		mTouchedPrimitives;	//!< List of touched primitives

		// Dequantization coeffs
							Point			mCenterCoeff;
							Point			mExtentsCoeff;
		// Stats
							udword			mNbVolumeBVTests;	//!< Number of Volume-BV tests
							udword			mNbVolumePrimTests;	//!< Number of Volume-Primitive tests
		// Internal methods
							void			_Dump(const AABBCollisionNode* node);
							void			_Dump(const AABBNoLeafNode* node);
							void			_Dump(const AABBQuantizedNode* node);
							void			_Dump(const AABBQuantizedNoLeafNode* node);

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Initializes a query
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		override(Collider) inline_	void	InitQuery()
											{
												// Reset stats & contact status
												mNbVolumeBVTests	= 0;
												mNbVolumePrimTests	= 0;
												Collider::InitQuery();
											}

		inline_				BOOL			IsCacheValid(VolumeCache& cache)
											{
												// We're going to do a volume-vs-model query.
												if(cache.Model!=mCurrentModel)
												{
													// Cached list was for another model so we can't keep it
													// Keep track of new owner and reset cache
													cache.Model = mCurrentModel;
													return FALSE;
												}
												else
												{
													// Same models, no problem
													return TRUE;
												}
											}
	};

#endif // __OPC_VOLUMECOLLIDER_H__
