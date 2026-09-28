/*
 * NxPhysicsThirdPartyTests' copy of Physics/src/ThirdPartyHost.cpp, with the
 * candidate side of qhull's output hooks made capturable (qhull-gap Task 1).
 *
 * The oracle's qhull prints through one host object, the pointer at
 * .data:0x00125080: slots +0x00/+0x04/+0x08/+0x0c take typed geometry and
 * slot +0x10 is qhull's plain fprintf. The harness replaces that object with
 * its own for the families that compare output. The candidate's qhull calls
 * the same five hooks as free functions (QhullNovodeXHost.h), which
 * ThirdPartyHost.cpp defines as shims that drop what they are handed. To give
 * both sides one sink, this file compiles ThirdPartyHost.cpp unchanged with
 * those five names renamed, and defines the real names here: each forwards to
 * the harness's sink when one is installed (gNxQhSink, set by the harness for
 * the duration of a run) and otherwise does what the shim does, nothing.
 *
 * malloc, free, the narrow-hull hook and the error exit are not renamed: the
 * test exe's candidate allocates, frees and exits exactly as the product's
 * ThirdPartyHost.cpp does. NxPhysics.dll itself still links
 * Physics/src/ThirdPartyHost.cpp, and nothing here reaches it.
 */

#define qhNovodeXOffBegin		nxProductQhNovodeXOffBegin
#define qhNovodeXPoint3			nxProductQhNovodeXPoint3
#define qhNovodeXFacet3Vertex	nxProductQhNovodeXFacet3Vertex
#define qhNovodeXSize			nxProductQhNovodeXSize
#define qhNovodeXFprintf		nxProductQhNovodeXFprintf
#include "../Physics/src/ThirdPartyHost.cpp"
#undef qhNovodeXOffBegin
#undef qhNovodeXPoint3
#undef qhNovodeXFacet3Vertex
#undef qhNovodeXSize
#undef qhNovodeXFprintf

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
	if(gNxQhSink)
		gNxQhSink->offBegin(dim, numpoints, numfacets, numridges);
	else
		nxProductQhNovodeXOffBegin(dim, numpoints, numfacets, numridges);
	}

void qhNovodeXPoint3(float x, float y, float z)
	{
	if(gNxQhSink)
		gNxQhSink->point3(x, y, z);
	else
		nxProductQhNovodeXPoint3(x, y, z);
	}

void qhNovodeXFacet3Vertex(int count, int* pointids)
	{
	if(gNxQhSink)
		gNxQhSink->facet3Vertex(count, pointids);
	else
		nxProductQhNovodeXFacet3Vertex(count, pointids);
	}

void qhNovodeXSize(float totarea, float totvol)
	{
	if(gNxQhSink)
		gNxQhSink->size(totarea, totvol);
	else
		nxProductQhNovodeXSize(totarea, totvol);
	}

int qhNovodeXFprintf(FILE* stream, const char* format, ...)
	{
	if(!gNxQhSink)
		return nxProductQhNovodeXFprintf(stream, format);
	va_list args;
	va_start(args, format);
	gNxQhSink->vprintf(stream, format, args);
	va_end(args);
	return 0;
	}

}
