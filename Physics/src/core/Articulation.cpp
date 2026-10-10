/*----------------------------------------------------------------------------*\
|
|                              Novodex Technology
|
\*----------------------------------------------------------------------------*/
// Articulation island rebuild: phys_fn_004169/004171/004172, with the
// recursive node construction helper phys_fn_004159. The 16-byte records,
// score weights, body/joint links, flags and allocator calls follow the
// oracle listings at RVAs 0x9adb0, 0x9ab00, 0x9abb0 and 0x9b0d0.

#include "BodyStep.h"
#include "core/Joint.h"
#include "core/JointSupport.h"
#include "FoundationSDK.h"
#include "NxUserAllocator.h"

#include <new>
#include <string.h>

namespace
{
struct ArticulationCandidate
	{
	void* body;
	void* reserved;
	void* selectedJoint;
	NxU32 score;
	};

struct ArticulationIsland
	{
	void** firstBegin;
	void** firstEnd;
	void** firstCapacity;
	NxU32 unknown00c;
	void** secondBegin;
	void** secondEnd;
	void** secondCapacity;
	NxU32 unknown01c;
	};

class ArticulationNode
	{
	public:
	ArticulationNode(void* parent, void* body, void* joint)
		: mUnknown004(0), mParent(parent), mFirstChild(0), mNextSibling(0),
		  mBody(body), mJoint(joint), mLastChild(0), mChildCount(0), mUnknown024(0)
		{
		if(mParent)
			{
			ArticulationNode* parentNode = static_cast<ArticulationNode*>(mParent);
			mNextSibling = parentNode->mFirstChild;
			parentNode->mLastChild = this;
			parentNode->mFirstChild = this;
			++parentNode->mChildCount;
			}
		}

	virtual ~ArticulationNode()
		{
		ArticulationNode* child = static_cast<ArticulationNode*>(mFirstChild);
		while(child)
			{
			ArticulationNode* next = static_cast<ArticulationNode*>(child->mNextSibling);
			delete child;
			child = next;
			}
		mParent = 0;
		mFirstChild = 0;
		mNextSibling = 0;
		mBody = 0;
		mJoint = 0;
		mChildCount = 0;
		}

	virtual void slot1(void*, void*, void*) {}
	virtual void slot2() {}
	virtual void slot3() {}
	virtual int solveProjection()
		{
		Joint* joint = static_cast<Joint*>(mJoint);
		if(joint && joint->mProjectionMode != NX_JPM_NONE)
			{
			const JointBodyRecord* body0 = static_cast<const JointBodyRecord*>(joint->mBody[0]);
			const JointBodyRecord* body1 = static_cast<const JointBodyRecord*>(joint->mBody[1]);
			if(((body0 && !(body0->mUnknown10c & 0x80))
				|| (body1 && !(body1->mUnknown10c & 0x80)))
				&& !((static_cast<NxU32>(joint->mProjectionMode) >> 2) & 1))
				joint->row_slot8(mBody);
			}
		int result = 0;
		for(ArticulationNode* child = static_cast<ArticulationNode*>(mFirstChild);
			child; child = static_cast<ArticulationNode*>(child->mNextSibling))
			result = child->solveProjection();
		return result;
		}
	virtual int step(NxReal dt)
		{
		int result = 0;
		Joint* joint = static_cast<Joint*>(mJoint);
		if(joint)
			{
			joint->mUnknown160[0] = 0xffffffffu;
			joint->mUnknown160[1] = 0;
			joint->Joint::row_slot6(dt);
			}
		for(ArticulationNode* child = static_cast<ArticulationNode*>(mFirstChild);
			child; child = static_cast<ArticulationNode*>(child->mNextSibling))
			result = child->step(dt);
		return result;
		}
	virtual void visualize(NxDebugRenderable*) {}
	virtual void slot7() {}

	static void operator delete(void* memory)
		{
		if(memory)
			nxFoundationSDKAllocator->free(memory);
		}

	private:
	NxU32 mUnknown004;
	void* mParent;
	void* mFirstChild;
	void* mNextSibling;
	void* mBody;
	void* mJoint;
	void* mLastChild;
	NxU32 mChildCount;
	NxU32 mUnknown024;
	};

static_assert(sizeof(ArticulationNode) == 0x28, "articulation node allocation is 0x28 bytes");
static_assert(sizeof(ArticulationIsland) == 0x20, "articulation island allocation is 0x20 bytes");
static_assert(sizeof(ArticulationCandidate) == 0x10, "articulation candidate is 0x10 bytes");

static void articulationSwap(ArticulationCandidate* a, ArticulationCandidate* b)
	{
	ArticulationCandidate temp = *a;
	*a = *b;
	*b = temp;
	}

// phys_fn_004161: recursive in-place quicksort over 16-byte candidates,
// ordering the unsigned score at +0x0c from highest to lowest.
static void articulationSort(ArticulationCandidate* first, ArticulationCandidate* last)
	{
	ArticulationCandidate* low = first;
	while(low < last)
		{
		const NxU32 pivot = (low + ((last - low) >> 2))->score;
		ArticulationCandidate* left = low;
		ArticulationCandidate* right = last;
		while(left <= right)
			{
			while(left <= right && left->score > pivot)
				++left;
			while(left <= right && right->score < pivot)
				--right;
			if(left > right)
				break;
			if(left != right)
				articulationSwap(left, right);
			++left;
			if(right == first)
				break;
			--right;
			}
		if(low < right)
			articulationSort(low, right);
		low = left;
		}
	}

static ArticulationNode* articulationMakeNode(void* parent, void* body, void* joint)
	{
	void* memory = nxFoundationSDKAllocator->malloc(0x28, NX_MEMORY_PERSISTENT);
	return memory ? new(memory) ArticulationNode(parent, body, joint) : 0;
	}

// phys_fn_004159: mark one body and recursively build its connected dynamic
// projection-joint subtree, omitting the edge by which the caller arrived.
static ArticulationNode* articulationBuildNode(void* parent, void* body, void* arrivedBy)
	{
	*reinterpret_cast<NxU32*>(static_cast<NxU8*>(body) + 0x1e4) |= 8u;
	ArticulationNode* node = articulationMakeNode(parent, body, arrivedBy);
	if(!node)
		return 0;
	for(unsigned list = 0; list != 2; ++list)
		{
		const NxU32 headOffset = list ? 0x1dc : 0x1d8;
		const NxU32 linkOffset = list ? 0x38 : 0x34;
		for(void* joint = *reinterpret_cast<void**>(static_cast<NxU8*>(body) + headOffset);
			joint; joint = *reinterpret_cast<void**>(static_cast<NxU8*>(joint) + linkOffset))
			{
			if(joint == arrivedBy)
				continue;
			void* other = *reinterpret_cast<void**>(static_cast<NxU8*>(joint) + 8);
			if(other == body)
				other = *reinterpret_cast<void**>(static_cast<NxU8*>(joint) + 0xc);
			if(other && !(static_cast<NxI8>(*(static_cast<NxU8*>(other) + 0x10c)) < 0)
				&& !(*reinterpret_cast<NxU32*>(static_cast<NxU8*>(other) + 0x1e4) & 8u))
				articulationBuildNode(node, other, joint);
			}
		}
	return node;
	}

static bool articulationPush(void*** begin, void*** end, void*** capacity, void* value)
	{
	if(*capacity <= *end)
		{
		const NxU32 count = *begin ? static_cast<NxU32>(*end - *begin) : 0u;
		const NxU32 newCount = count * 2u + 2u;
		void** replacement = static_cast<void**>(nxFoundationSDKAllocator->malloc(
			newCount * sizeof(void*), NX_MEMORY_PERSISTENT));
		if(!replacement)
			return false;
		for(NxU32 i = 0; i < count; ++i)
			replacement[i] = (*begin)[i];
		if(*begin)
			nxFoundationSDKAllocator->free(*begin);
		*begin = replacement;
		*end = replacement + count;
		*capacity = replacement + newCount;
		}
	**end = value;
	++*end;
	return true;
	}

static ArticulationIsland* articulationRebuild(void* root)
	{
	ArticulationCandidate* begin = 0;
	ArticulationCandidate* end = 0;
	ArticulationCandidate* capacity = 0;
	for(void* body = root; body; body = *reinterpret_cast<void**>(static_cast<NxU8*>(body) + 0x1d0))
		{
		NxU32 score = 0;
		void* selected = 0;
		*reinterpret_cast<NxU32*>(static_cast<NxU8*>(body) + 0x1e4) &= ~8u;
		for(unsigned list = 0; list != 2; ++list)
			{
			const NxU32 headOffset = list ? 0x1dc : 0x1d8;
			const NxU32 linkOffset = list ? 0x38 : 0x34;
			for(void* joint = *reinterpret_cast<void**>(static_cast<NxU8*>(body) + headOffset);
				joint; joint = *reinterpret_cast<void**>(static_cast<NxU8*>(joint) + linkOffset))
				{
				void* other = *reinterpret_cast<void**>(static_cast<NxU8*>(joint) + 8);
				if(other == body)
					other = *reinterpret_cast<void**>(static_cast<NxU8*>(joint) + 0xc);
				if(!other || (static_cast<NxI8>(*(static_cast<NxU8*>(other) + 0x10c)) < 0))
					{
					score += 100000u;
					selected = joint;
					if(*reinterpret_cast<NxU32*>(static_cast<NxU8*>(joint) + 0x44))
						score += 100u;
					}
				else
					score += 1000u;
				}
			}
		if(end == capacity)
			{
			const NxU32 count = begin ? static_cast<NxU32>(end - begin) : 0u;
			const NxU32 newCount = count * 2u + 2u;
			ArticulationCandidate* replacement = static_cast<ArticulationCandidate*>(
				nxFoundationSDKAllocator->malloc(newCount * sizeof(ArticulationCandidate), NX_MEMORY_PERSISTENT));
			if(!replacement)
				{
				if(begin) nxFoundationSDKAllocator->free(begin);
				return 0;
				}
			if(count) memcpy(replacement, begin, count * sizeof(ArticulationCandidate));
			if(begin) nxFoundationSDKAllocator->free(begin);
			begin = replacement;
			end = replacement + count;
			capacity = replacement + newCount;
			}
		end->body = body;
		end->reserved = 0;
		end->selectedJoint = selected;
		end->score = score;
		++end;
		}

	if(begin)
		articulationSort(begin, end - 1);
	ArticulationIsland* island = 0;
	for(ArticulationCandidate* item = begin; item != end; ++item)
		{
		if(*reinterpret_cast<NxU32*>(static_cast<NxU8*>(item->body) + 0x1e4) & 8u)
			continue;
		ArticulationNode* group = articulationBuildNode(0, item->body, item->selectedJoint);
		if(!group)
			continue;
		if(!island)
			{
			island = static_cast<ArticulationIsland*>(nxFoundationSDKAllocator->malloc(
				0x20, NX_MEMORY_PERSISTENT));
			if(!island)
				break;
			island->firstBegin = island->firstEnd = island->firstCapacity = 0;
			island->secondBegin = island->secondEnd = island->secondCapacity = 0;
			island->unknown01c = 0;
			}
		if(!articulationPush(&island->secondBegin, &island->secondEnd,
			&island->secondCapacity, group))
			break;
		}
	if(begin)
		nxFoundationSDKAllocator->free(begin);
	return island;
	}
}

// phys_fn_004172 (0x9b0d0): clear the dirty bit and rebuild the root's
// articulation island object. The return value is intentionally ignored.
void __cdecl nxBodyIslandRebuild004172(void* root)
	{
	if(!root)
		return;
	NxU32* flags = reinterpret_cast<NxU32*>(static_cast<NxU8*>(root) + 0x1e4);
	*flags &= ~2u;
	ArticulationIsland* island = articulationRebuild(root);
	if(!island)
		{
		NxFoundation::FoundationSDK::dbMessage(NX_WARN,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\core\\Articulation.cpp",
			0x37, "No articulateable joints in group.\n");
		return;
		}
	*reinterpret_cast<void**>(static_cast<NxU8*>(root) + 0x1e0) = island;
	}

// phys_fn_004165 (0x9ace0): visit each root node in the island's second
// pointer vector and dispatch its projection slot (vtable +0x10).
void __cdecl nxBodyIslandProject004165(void* island)
	{
	if(!island)
		return;
	ArticulationIsland* const groups = static_cast<ArticulationIsland*>(island);
	for(void** item = groups->secondBegin; item && item != groups->secondEnd; ++item)
		static_cast<ArticulationNode*>(*item)->solveProjection();
	}
