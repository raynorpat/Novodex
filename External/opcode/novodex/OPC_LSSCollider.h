/*
 * NOVODEX LOCAL MODIFICATION
 * upstream: External/opcode/upstream/Opcode/OPC_LSSCollider.h
 *
 * [1] LSSCollider keeps the capsule's radius and three precomputed segment
 *     Points between mSeg and mRadius2: half the segment's direction, its
 *     absolute value and the segment's centre -- the members RayCollider's
 *     segment-box test uses (mData, mFDir, mData2) -- which the NovodeX
 *     LSSAABBOverlap tests against the box inflated by the radius
 *     (OPC_LSSAABBOverlap.h [1]; InitQuery fills them, OPC_LSSCollider.cpp [1]).
 *     mRadius2 moves from +0x4c to +0x74.
 *     established at 0x000d34e2 (`mov [esi+0x70], edx`: the radius) and
 *     0x000d34f6 (`fstp [esi+0x74]`: its square) in InitQuery; 0x000d3a1d..
 *     0x000d3aa9 (the three Points at +0x4c, +0x58 and +0x64); 0x000d3ad3..
 *     0x000d3bd5 (_Collide reading all of them); 0x000d3750 (`fcomp
 *     [esi+0x74]`: LSSTriOverlap's compare).
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
 *	Contains code for an LSS collider.
 *	\file		OPC_LSSCollider.h
 *	\author		Pierre Terdiman
 *	\date		December, 28, 2002
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Include Guard
#ifndef __OPC_LSSCOLLIDER_H__
#define __OPC_LSSCOLLIDER_H__

	struct OPCODE_API LSSCache : VolumeCache
	{
					LSSCache()
					{
						Previous.mP0 = Point(0.0f, 0.0f, 0.0f);
						Previous.mP1 = Point(0.0f, 0.0f, 0.0f);
						Previous.mRadius = 0.0f;
						FatCoeff = 1.1f;
					}

		// Cached faces signature
		LSS			Previous;	//!< LSS used when performing the query resulting in cached faces
		// User settings
		float		FatCoeff;	//!< mRadius2 multiplier used to create a fat LSS
	};

	class OPCODE_API LSSCollider : public VolumeCollider
	{
		public:
		// Constructor / Destructor
											LSSCollider();
		virtual								~LSSCollider();

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Generic collision query for generic OPCODE models. After the call, access the results:
		 *	- with GetContactStatus()
		 *	- with GetNbTouchedPrimitives()
		 *	- with GetTouchedPrimitives()
		 *
		 *	\param		cache			[in/out] an lss cache
		 *	\param		lss				[in] collision lss in local space
		 *	\param		model			[in] Opcode model to collide with
		 *	\param		worldl			[in] lss world matrix, or null
		 *	\param		worldm			[in] model's world matrix, or null
		 *	\return		true if success
		 *	\warning	SCALE NOT SUPPORTED. The matrices must contain rotation & translation parts only.
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
							bool			Collide(LSSCache& cache, const LSS& lss, const Model& model, const Matrix4x4* worldl=null, const Matrix4x4* worldm=null);
		// 
							bool			Collide(LSSCache& cache, const LSS& lss, const AABBTree* tree);
		protected:
		// LSS in model space
							Segment			mSeg;			//!< Segment
		// NOVODEX [1]
							Point			mSDir;			//!< Half the segment's direction, (mP1-mP0)/2
							Point			mFDir;			//!< |mSDir|, per component
							Point			mSCen;			//!< The segment's centre, (mP0+mP1)/2
							float			mRadius;		//!< LSS radius
							float			mRadius2;		//!< LSS radius squared
		// Internal methods
							void			_Collide(const AABBCollisionNode* node);
							void			_Collide(const AABBNoLeafNode* node);
							void			_Collide(const AABBQuantizedNode* node);
							void			_Collide(const AABBQuantizedNoLeafNode* node);
							void			_Collide(const AABBTreeNode* node);
							void			_CollideNoPrimitiveTest(const AABBCollisionNode* node);
							void			_CollideNoPrimitiveTest(const AABBNoLeafNode* node);
							void			_CollideNoPrimitiveTest(const AABBQuantizedNode* node);
							void			_CollideNoPrimitiveTest(const AABBQuantizedNoLeafNode* node);
			// Overlap tests
		inline_				BOOL			LSSContainsBox(const Point& bc, const Point& be);
		inline_				BOOL			LSSAABBOverlap(const Point& center, const Point& extents);
		inline_				BOOL			LSSTriOverlap(const Point& vert0, const Point& vert1, const Point& vert2);
			// Init methods
							BOOL			InitQuery(LSSCache& cache, const LSS& lss, const Matrix4x4* worldl=null, const Matrix4x4* worldm=null);
	};

	class OPCODE_API HybridLSSCollider : public LSSCollider
	{
		public:
		// Constructor / Destructor
											HybridLSSCollider();
		virtual								~HybridLSSCollider();

							bool			Collide(LSSCache& cache, const LSS& lss, const HybridModel& model, const Matrix4x4* worldl=null, const Matrix4x4* worldm=null);
		protected:
							Container		mTouchedBoxes;
	};

#endif // __OPC_LSSCOLLIDER_H__
