/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "core/ActorMass.h"
#include "ObjectModel.h"
#include "NxMat33.h"
#include "NxUtilities.h"

#include <string.h>

// Mass from shapes (actor-mass Task 1; units/actor-mass-contract.md). The
// oracle keeps no __FILE__ for these rows.
//
// The three rows are noinline, as the listing calls them, so that a
// breakpoint on a row sees it run (the core/SpringAndDamperEffector.cpp
// convention).
//
// Precision: as in core/Joint.cpp, a value the listing keeps on the x87
// stack is a `double` here and a value it stores is an `NxReal`, with the
// listing's operand grouping kept. The file is on the /arch:IA32 list.

// The .rdata 0.0f the listing compares the masses with (0x101041f0).
static const NxReal gActorMassZero = 0.0f;

// phys_fn_000841 (0x0001c720, 43 B)
// __thiscall on a MassFrame, no stack arguments, plain `ret`: translate the
// frame by its own negated offset (three `fld; fchs; fstp m32`), which moves
// its reference point to the centre of mass (000833's centred path).
struct Row000841Fixture
	{
	void row000841();
	};

__declspec(noinline) void Row000841Fixture::row000841()
	{
	MassFrame* frame = reinterpret_cast<MassFrame*>(this);
	NxReal negated[3];
	negated[0] = -frame->mOffset.x;
	negated[1] = -frame->mOffset.y;
	negated[2] = -frame->mOffset.z;
	frame->nxMassFrameTranslate(negated);
	}

// phys_fn_000008 (0x000010a0, 751 B)
// Actor::computeMass. The frame starts zeroed (000847 with 1) and the root
// shape's slot 4 merges every non-trigger shape into it at unit density,
// with a pointer to a zeroed vector as the third argument (0x10c5..0x10dd).
//
//   * slot 4 false: return 1 (a mesh shape whose inertia failed);
//   * frame mass <= 0 (`fcomp [0x101041f0]; test ah,0x41; jp`, so an
//     unordered mass also returns): return 2 -- no non-trigger shape;
//   * otherwise the pose position takes the frame's offset (0x1132..0x1150)
//     before 000841 moves the frame to its centre, and the tensor is scaled
//     into a copy by one of three arms:
//       density <= 0 (or unordered)   by *totalMass / frameMass, the quotient
//                                     kept on the stack (0x12e6);
//       *totalMass <= 0 (or unord.)   by the density, and *totalMass becomes
//                                     frameMass * density (0x1242);
//       both positive                 by the density; *totalMass is left as
//                                     the caller gave it (0x118e);
//   * NxDiagonalizeInertiaTensor (import 0x101041b8, cdecl) writes the
//     principal moments to *massSpaceInertia and the rotation to the pose's
//     3x3; its result is ignored. Return 0.
//
// Both frames are destroyed through 001583 (0x2ea70), a one-byte `ret`.
__declspec(noinline) NxU32 Row000008Fixture::row000008(NxReal density, NxReal* totalMass,
	NxMat34* massLocalPose, NxVec3* massSpaceInertia)
	{
	unsigned char* actor = reinterpret_cast<unsigned char*>(this);
	MassFrame frame;
	frame.nxMassFrameConditionalZero(1);
	NxVec3 reserved;
	reserved.x = 0.0f;
	reserved.y = 0.0f;
	reserved.z = 0.0f;
	ActorMassShape* shape = *reinterpret_cast<ActorMassShape**>(actor + 0x10);
	if(!shape->accumulateMass(&frame, 1.0f, &reserved))
		return 1;
	if(!(frame.mMass > gActorMassZero))
		return 2;

	memcpy(&massLocalPose->t, &frame.mOffset, sizeof(NxVec3));	// integer moves
	reinterpret_cast<Row000841Fixture*>(&frame)->row000841();

	NxReal scaled[9];
	if(!(density > gActorMassZero))
		{
		const double scale = static_cast<double>(*totalMass) / frame.mMass;
		for(unsigned i = 0; i < 9; ++i)
			scaled[i] = static_cast<NxReal>(frame.mInertia[i] * scale);
		}
	else if(!(*totalMass > gActorMassZero))
		{
		*totalMass = static_cast<NxReal>(static_cast<double>(frame.mMass) * density);
		for(unsigned i = 0; i < 9; ++i)
			scaled[i] = static_cast<NxReal>(static_cast<double>(frame.mInertia[i]) * density);
		}
	else
		{
		for(unsigned i = 0; i < 9; ++i)
			scaled[i] = static_cast<NxReal>(static_cast<double>(frame.mInertia[i]) * density);
		}

	// The frame's nine words go to the import as the NxMat33 they are laid
	// out as (the rep movsd copies at 0x1197/0x1236/0x125b/0x12f6/0x136c).
	NxMat33 inertia;
	memcpy(&inertia, scaled, sizeof(scaled));
	NxDiagonalizeInertiaTensor(inertia, *massSpaceInertia, massLocalPose->M);
	return 0;
	}

static_assert(sizeof(NxMat33) == 9 * sizeof(NxReal), "the tensor is passed as nine floats");

// phys_fn_001024 (0x000229b0, 87 B)
// The compound's slot 4: every child whose flag byte +0xde has none of the
// trigger bits 0..2 set gets its own slot 4 with the three arguments
// unchanged; the first false stops the walk and returns false. True when
// every call returned true, and for an empty array.
__declspec(noinline) bool CompoundShapeMass::row001024(MassFrame* destination, NxReal density,
	const NxVec3* reserved)
	{
	const unsigned char* compound = reinterpret_cast<const unsigned char*>(this);
	unsigned char* const* shape = *reinterpret_cast<unsigned char* const* const*>(compound + 0xe0);
	unsigned char* const* end = *reinterpret_cast<unsigned char* const* const*>(compound + 0xe4);
	for(NxU32 count = static_cast<NxU32>(end - shape); count != 0; --count, ++shape)
		{
		if((*shape)[0xde] & 7)
			continue;
		if(!reinterpret_cast<ActorMassShape*>(*shape)->accumulateMass(destination,
				density, reserved))
			return false;
		}
	return true;
	}

template<typename Method>
static void* actorMassMethodAddress(Method method)
	{
	static_assert(sizeof(Method) == sizeof(void*),
		"shape table methods require the Win32 single-inheritance ABI");
	void* address;
	memcpy(&address, &method, sizeof(address));
	return address;
	}

void** nxCompoundShapeInternalVtable()
	{
	struct Table
		{
		void* slot[15];
		Table()
			{
			memset(slot, 0, sizeof(slot));
			slot[1] = actorMassMethodAddress(&ShapeBase::nxApplyDescriptor);
			slot[2] = actorMassMethodAddress(&ShapeBase::nxBaseSaveState);
			slot[4] = actorMassMethodAddress(&CompoundShapeMass::row001024);
			slot[7] = actorMassMethodAddress(&ShapeBase::nxBaseSlot7);
			slot[12] = actorMassMethodAddress(&ShapeBase::nxSelf);
			slot[13] = slot[12];
			slot[14] = slot[12];
			}
		};
	static Table table;
	return table.slot;
	}
