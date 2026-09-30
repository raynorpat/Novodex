/*
 * NxPhysicsThirdPartyTests' copy of the product's two host files,
 * Physics/src/ThirdPartyHost.cpp (OPCODE's seam) and Physics/src/QhullHost.cpp
 * (qhull's host object and its nine hooks), with qhull's hooks renamed so the
 * harness can decide per call where each one goes.
 *
 * The oracle's qhull reaches its host through one object, the pointer at
 * .data:0x00125080: slots +0x00/+0x04/+0x08/+0x0c take typed geometry, +0x10
 * is qhull's plain fprintf, +0x14/+0x18 allocate, +0x1c is the narrow-hull
 * hook and +0x20 the error exit. The harness replaces that object with its own
 * for the families that drive qhull directly (gQhHostVtable). The candidate's
 * qhull calls the same nine hooks as free functions (QhullNovodeXHost.h). This
 * file compiles QhullHost.cpp unchanged with the nine names renamed
 * (nxProductQhNovodeX*), and defines the real names here. Each one:
 *
 *   * forwards to the product hook when the product's own host object is
 *     published (gQhullHost non-NULL): that is a candidate CreateConvexHull
 *     run, and the product code is what is under test;
 *   * otherwise, for the five output hooks, forwards to the harness's sink
 *     when one is installed (gNxQhSink, qhull-gap Task 1);
 *   * otherwise does what the product's hooks did before qhull-gap Task 4a
 *     gave qhull a real host: the output hooks and the narrow-hull hook do
 *     nothing, fprintf returns 0, malloc/free go to the SDK allocator
 *     untracked and the error exit aborts (the harness catches SIGABRT,
 *     PhysicsThirdPartyQhull.c). The harness drives qhull outside any driver,
 *     where the shipped object would be a dangling or NULL pointer; this is
 *     the stand-in every registered qhull family was measured against, kept
 *     unchanged so none of them moves.
 *
 * A harness that runs the candidate's CreateConvexHull must set gQhullHost
 * back to NULL afterwards: the product, like the oracle, leaves it pointing at
 * the driver's dead frame.
 * malloc, free, the narrow-hull hook and the error exit are not renamed: the
 * test exe's candidate allocates, frees and exits exactly as the product's
 * ThirdPartyHost.cpp does. NxPhysics.dll itself still links
 * Physics/src/ThirdPartyHost.cpp, and nothing here reaches it.
 *
 * The SetIceError seam (opcNovodeXSetIceError, the candidate side of
 * phys_fn_002160) is renamed the same way (convex-mesh gap Task 2c): the real
 * name forwards to gNxIceErrorSink while the harness installs one -- so the
 * families of the ICE-shaped rows (EdgeList, IceAdjacencies, Valencies) can
 * compare the (message, file, line) each report carries with what the oracle's
 * rows push -- and otherwise does what the shim does and returns its false.
 */

#define opcNovodeXSetIceError nxProductOpcNovodeXSetIceError
#include "../Physics/src/ThirdPartyHost.cpp"
#undef opcNovodeXSetIceError

#define qhNovodeXOffBegin		nxProductQhNovodeXOffBegin
#define qhNovodeXPoint3			nxProductQhNovodeXPoint3
#define qhNovodeXFacet3Vertex	nxProductQhNovodeXFacet3Vertex
#define qhNovodeXSize			nxProductQhNovodeXSize
#define qhNovodeXFprintf		nxProductQhNovodeXFprintf
#define qhNovodeXMalloc			nxProductQhNovodeXMalloc
#define qhNovodeXFree			nxProductQhNovodeXFree
#define qhNovodeXNarrowHull		nxProductQhNovodeXNarrowHull
#define qhNovodeXErrexit		nxProductQhNovodeXErrexit
#include "../Physics/src/QhullHost.cpp"
#undef qhNovodeXOffBegin
#undef qhNovodeXPoint3
#undef qhNovodeXFacet3Vertex
#undef qhNovodeXSize
#undef qhNovodeXFprintf
#undef qhNovodeXMalloc
#undef qhNovodeXFree
#undef qhNovodeXNarrowHull
#undef qhNovodeXErrexit

#include <stdarg.h>

// The harness's sink: one table, the same functions the harness's stand-in for
// the oracle's host object calls from its slots.
struct NxQhSink
	{
	void (*offBegin)(int dim, int numpoints, int numfacets, int numridges);
	void (*point3)(float x, float y, float z);
	void (*facet3Vertex)(int count, int* pointids);
	void (*size)(float totarea, float totvol);
	void (*vprintf)(const void* stream, const char* format, va_list args);
	};

NxQhSink* gNxQhSink = 0;

extern "C" {

void qhNovodeXOffBegin(int dim, int numpoints, int numfacets, int numridges)
	{
	if(gQhullHost)
		nxProductQhNovodeXOffBegin(dim, numpoints, numfacets, numridges);
	else if(gNxQhSink)
		gNxQhSink->offBegin(dim, numpoints, numfacets, numridges);
	}

void qhNovodeXPoint3(float x, float y, float z)
	{
	if(gQhullHost)
		nxProductQhNovodeXPoint3(x, y, z);
	else if(gNxQhSink)
		gNxQhSink->point3(x, y, z);
	}

void qhNovodeXFacet3Vertex(int count, int* pointids)
	{
	if(gQhullHost)
		nxProductQhNovodeXFacet3Vertex(count, pointids);
	else if(gNxQhSink)
		gNxQhSink->facet3Vertex(count, pointids);
	}

void qhNovodeXSize(float totarea, float totvol)
	{
	if(gQhullHost)
		nxProductQhNovodeXSize(totarea, totvol);
	else if(gNxQhSink)
		gNxQhSink->size(totarea, totvol);
	}

int qhNovodeXFprintf(FILE* stream, const char* format, ...)
	{
	va_list args;
	va_start(args, format);
	if(gQhullHost)
		{
		// The product hook's body (QhullHost.cpp): `...` cannot be forwarded
		// to it, so it is repeated here -- format, then the host's print slot
		// (003263) with the text as "%s", which errexits and does not return.
		char buffer[0x2000];
		vsprintf(buffer, format, args);
		gQhullHost->print(stream, "%s", buffer);
		}
	if(gNxQhSink)
		gNxQhSink->vprintf(stream, format, args);
	va_end(args);
	return 0;
	}

void* qhNovodeXMalloc(size_t size)
	{
	if(gQhullHost)
		return nxProductQhNovodeXMalloc(size);
	return nxGetSdkAllocator()->malloc(size, NX_MEMORY_PERSISTENT);
	}

void qhNovodeXFree(void* memory)
	{
	if(gQhullHost)
		{
		nxProductQhNovodeXFree(memory);
		return;
		}
	nxGetSdkAllocator()->free(memory);
	}

void qhNovodeXNarrowHull()
	{
	if(gQhullHost)
		nxProductQhNovodeXNarrowHull();
	}

void qhNovodeXErrexit(int exitcode)
	{
	if(gQhullHost)
		nxProductQhNovodeXErrexit(exitcode);
	abort();
	}

}

// The SetIceError seam. The shim's result is returned either way: the rows
// return what their report returns.
bool (*gNxIceErrorSink)(const char* message, const char* file, int line) = 0;

bool opcNovodeXSetIceError(const char* message, const char* file, int line)
	{
	const bool returned = nxProductOpcNovodeXSetIceError(message, file, line);
	if(gNxIceErrorSink)
		gNxIceErrorSink(message, file, line);
	return returned;
	}
