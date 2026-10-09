#ifndef NX_POLYGON_SCRATCH_H
#define NX_POLYGON_SCRATCH_H
#include "NxSimpleTypes.h"
#include <stddef.h>
// Measured prefix consumed by 000505 and 002249, and the retained contact
// families. The complete contact scratch owner is a later migration gate.
struct NxPolygonScratch
{
    NxU32 mOpaque00;
    NxU32 mVisitedCount;
    NxU32 *mVisited;
    NxU32 mOpaque0C;
    NxU32 mOpaque10;
    NxU32 mStamp;
};
#if defined(NX32)
static_assert(offsetof(NxPolygonScratch, mVisitedCount) == 4, "visited count at +04");
static_assert(offsetof(NxPolygonScratch, mVisited) == 8, "visited array at +08");
static_assert(offsetof(NxPolygonScratch, mStamp) == 0x14, "stamp at +14");
static_assert(sizeof(NxPolygonScratch) == 0x18, "measured scratch prefix");
#endif
#endif
