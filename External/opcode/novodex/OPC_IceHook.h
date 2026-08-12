/*
 * NOVODEX LOCAL MODIFICATION
 * upstream: External/opcode/upstream/Opcode/OPC_IceHook.h
 *
 * [1] SetIceError replaced by a reporter taking (message, file, line) and
 *     returning false. The second stock argument is discarded.
 *     established at 0x000539b0, 31 bytes: it takes three cdecl arguments, forwards them to
 *     the global error-stream pointer at .data:0x001041b4 as
 *     (2, file, line, 0, message), and returns false with `xor al,al`.
 *     Call sites push message, file, line in that order: 0x000e903b pushes
 *     0xe6 = 230 with .rdata:0x0011ba74 (OPC_MeshInterface.cpp) and
 *     .rdata:0x0011ba48 ("MeshInterface::SetPointers: pointer is null"), and
 *     0x000e912f pushes 0x93 = 147 with .rdata:0x0011bb24 (OPC_Model.cpp).
 *     230 is the stock line number in OPC_MeshInterface.cpp exactly.
 * [2] Log is NOT modified. An earlier pass recorded it as replaced alongside
 *     SetIceError; measured, it is stock.
 *     established at Model::Build's `if(NbDegenerate) Log(...)` compiles to nothing in the
 *     image: 0x000e9150 calls CheckTopology and 0x000e9155 goes straight to
 *     Release with no test of the result and no call in between. A real Log
 *     would leave both.
 */

// Should be included by Opcode.h if needed

	#define ICE_DONT_CHECK_COMPILER_OPTIONS

	// From Windows...
	typedef int                 BOOL;
	#ifndef FALSE
	#define FALSE               0
	#endif

	#ifndef TRUE
	#define TRUE                1
	#endif

	#include <stdio.h>
	#include <stdlib.h>
	#include <assert.h>
	#include <string.h>
	#include <float.h>
	#include <Math.h>

	#ifndef ASSERT
		#define	ASSERT(exp)	{}
	#endif
	#define ICE_COMPILE_TIME_ASSERT(exp)	extern char ICE_Dummy[ (exp) ? 1 : -1 ]

	// NOVODEX: Log stays the stock no-op -- see [2]. SetIceError became a real
	// reporter carrying __FILE__ and __LINE__ -- see [1]. The stock spelling is a
	// two-argument call whose second argument is an error code the reporter drops.
	#define	Log				{}
	#include "OpcodeNovodeXHost.h"
	#define	SetIceError(message, code)	opcNovodeXSetIceError(message, __FILE__, __LINE__)
	#define	EC_OUTOFMEMORY	"Out of memory"

	#include ".\Ice\IcePreprocessor.h"

	#undef ICECORE_API
	#define ICECORE_API	OPCODE_API

	#include ".\Ice\IceTypes.h"
	#include ".\Ice\IceFPU.h"
	#include ".\Ice\IceMemoryMacros.h"

	namespace IceCore
	{
		#include ".\Ice\IceUtils.h"
		#include ".\Ice\IceContainer.h"
		#include ".\Ice\IcePairs.h"
		#include ".\Ice\IceRevisitedRadix.h"
		#include ".\Ice\IceRandom.h"
	}
	using namespace IceCore;

	#define ICEMATHS_API	OPCODE_API
	namespace IceMaths
	{
		#include ".\Ice\IceAxes.h"
		#include ".\Ice\IcePoint.h"
		#include ".\Ice\IceHPoint.h"
		#include ".\Ice\IceMatrix3x3.h"
		#include ".\Ice\IceMatrix4x4.h"
		#include ".\Ice\IcePlane.h"
		#include ".\Ice\IceRay.h"
		#include ".\Ice\IceIndexedTriangle.h"
		#include ".\Ice\IceTriangle.h"
		#include ".\Ice\IceTriList.h"
		#include ".\Ice\IceAABB.h"
		#include ".\Ice\IceOBB.h"
		#include ".\Ice\IceBoundingSphere.h"
		#include ".\Ice\IceSegment.h"
		#include ".\Ice\IceLSS.h"
	}
	using namespace IceMaths;
