/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// NovodeX's hull library around qhull, band A of the qhull span
// (0x0007d420-0x0007ed43): the qhull host object and its tracked allocator
// (qhull-gap Task 4a), the hull driver, its result, the box fallback and the
// OBJ writers (Task 4b). Contract: units/convex-cooking-contract.md
// (docs/reconstruction/novodex-physics); the bundle is
// units/gap__Controller.cpp__to__fluids__Fluid.cpp.md.
//
// The file name is DESCRIPTIVE, like every name in QhullHost.h. The one
// constraint the image puts on it: band A links between qhull.c and qset.c,
// so if NovodeX's objects were linked in the same alphabetical order as
// qhull's, its name sorts between those two.
//
// Rows are in address order, each under its stable-ID line. Floating point
// follows core/Joint.cpp: this translation unit is x87 in the oracle and is
// built /arch:IA32 here; a value the listing keeps on the FPU stack is a
// `double`, a value it stores (fstp dword) is an `NxReal`. No row here touches
// the control word; cooking runs under the caller's.
//
// What the shipped object does that the earlier shims did not, reproduced on
// purpose:
//
//   * slot +0x10 (print) formats the message and then calls errexit(1): ANY
//     qhull print through the host ends the hull attempt;
//   * slot +0x20 (errexit) releases the arrays and longjmps to the driver's
//     jmp_buf at .data:0x00125040;
//   * every qhull allocation is a tracked block ("JOHNRAT" header, 4,096
//     slots), reclaimed by the destructor -- the driver never calls
//     qh_freeqhull;
//   * a full slot table frees the new block with the CRT's free even when the
//     user allocator made it (0x0007e8d4). An oracle defect, kept.

#define _CRT_SECURE_NO_WARNINGS

#include "QhullHost.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <float.h>
#include <math.h>

// qhull's C interface, as its own qhull_interface.cpp includes it. The merged
// tree (build/External/qhull-tree) comes with NxQhull's include directory.
extern "C" {
#include "qhull.h"
}

// .data:0x00125040, the only jmp_buf in the band: _setjmp3 in 003279
// (0x0007eae7) and longjmp in 003267 (0x0007e54f).
jmp_buf gQhullJump;
// .data:0x00125080. Written once, at 0x0007ea51, with the address of
// CreateConvexHull's stack object, and never cleared: after the driver returns
// it points at a dead frame, as the oracle's does.
QhullHost* gQhullHost = 0;
// .data:0x00125084 (phys_data_003856), the OBJ file counter both writers
// pre-increment (0x0007deac, 0x0007e05c).
int gQhullObjCounter = 0;

// phys_fn_003236 (0x0007d420, 214 B)
// The NovodeX qhull driver: unix.c's main() sequence on the "o" output format,
// with the points widened to double (fld dword / fstp qword, exact). It never
// calls qh_freeqhull: every block qhull took is a tracked one and the host's
// destructor frees it. The double array is allocated and freed through the
// published object.
int runQhull(int argc, char** argv, int n, const NxReal* points)
	{
	qh_init_A(stdin, stdout, stderr, argc, argv);
	qh_initflags(qh qhull_command);
	int count = n * 3;
	coordT* coords = (coordT*) gQhullHost->trackedMalloc(sizeof(coordT) * count);
	for(int i = 0; i < count; i++)
		coords[i] = points[i];
	qh_init_B(coords, n, 3, True);
	qh_qhull();
	qh_check_output();
	qh_produce_output();
	gQhullHost->trackedFree(coords);
	return 0;
	}

// phys_fn_003238 (0x0007d500, 106 B)
void QhullHost::releaseArrays()
	{
	if(mPoints)
		{
		trackedFree(mPoints);
		mPoints = 0;
		}
	if(mRemap)
		{
		trackedFree(mRemap);
		mRemap = 0;
		}
	if(mOutputVertices)
		{
		trackedFree(mOutputVertices);
		mOutputVertices = 0;
		mOutputCount = 0;
		}
	if(mIndices)
		{
		trackedFree(mIndices);
		mIndices = 0;
		mIndexCount = 0;
		}
	mNumPoints = 0;
	}

// phys_fn_003240 (0x0007d570, 32 B)
void* QhullHost::rawAlloc(size_t size)
	{
	if(mAllocator)
		return mAllocator->malloc(size);
	return ::malloc(size);
	}

// phys_fn_003241 (0x0007d590, 31 B)
void QhullHost::rawFree(void* memory)
	{
	if(mAllocator)
		{
		mAllocator->free(memory);
		return;
		}
	::free(memory);
	}

// Inlined eight times in 003253 (0x0007e226-0x0007e2f5) and eight times in
// each of cleanupVertices' two boxes (0x0007dd1f-0x0007de27,
// 0x0007db24-0x0007de27): the vertex goes to the end of the array and the
// count is bumped after each store.
static inline void addPoint(NxU32& vcount, NxReal* p, NxReal x, NxReal y, NxReal z)
	{
	NxReal* dest = &p[vcount * 3];
	dest[0] = x;
	dest[1] = y;
	dest[2] = z;
	vcount++;
	}

// phys_fn_003243 (0x0007d5b0, 933 B)
// phys_fn_003245 (0x0007d960, 1331 B)
// One function: 003245 starts inside the bounding-box loop over the cleaned
// points, reached by the jump at 0x0007d953. Ratcliff's CleanupVertices with
// two NovodeX arms: the weld is optional (`weld`, 0x0007d7f0) and a cleaned
// cloud larger than maxVertices goes to band B's quantizer (`reduce`,
// 0x0007da0c-0x0007da38).
//
// The constants are the image's: 1e-6f (.rdata:0x00106880), 0.5f
// (0x001043cc), 1.0f (0x001041ec), FLT_MAX (0x00106858, and the 0x7f7fffff /
// 0xff7fffff immediates of the box initialisers), 0.05f (0x001135b8), and the
// 0.01f immediate 0x3c23d70a.
//
// Floating point, from the listing:
//   * the extents, the centres, the reciprocals, py and pz are stored (fstp
//     dword) and are NxReal; px, the running shortest extent `len`, the weld
//     differences, the squared distances and the box corners stay on the FPU
//     stack and are double (the listing also spills y2 and z2 to float for
//     the last corners, 0x0007dd4b/0x0007dd5f; a float store of the same
//     value, so every corner word is the same);
//   * in the weld, the listing spills the stored vertex's y and z offsets
//     from the centre (0x0007d871, 0x0007d87d) and keeps the other four on
//     the stack; both squared distances are summed z, then y, then x:
//     (z*z + y*y) + x*x (0x0007d880-0x0007d8a6);
//   * the vertex kept is the one farther from the centre: the new point
//     replaces the stored one when dist(stored) < dist(new) (fcompp, test
//     ah,5; jp -- false on NaN);
//   * the first box takes an extent as the shortest when it is > 1e-6f
//     (test ah,0x41), the second when it is >= 1e-6f (test ah,1).
bool QhullHost::cleanupVertices(NxU32 svcount, const NxReal* svertices, NxU32 stride,
	NxU32& vcount, NxReal* vertices, NxReal normalepsilon, NxReal* scale,
	bool weld, bool reduce, NxU32 maxVertices)
	{
	if(svcount == 0)
		return false;

	vcount = 0;

	NxReal recip[3];

	if(scale)
		{
		scale[0] = 1.0f;
		scale[1] = 1.0f;
		scale[2] = 1.0f;
		}

	NxReal bmin[3] = { FLT_MAX, FLT_MAX, FLT_MAX };
	NxReal bmax[3] = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

	const char* vtx = (const char*) svertices;

	for(NxU32 i = 0; i < svcount; i++)
		{
		const NxReal* p = (const NxReal*) vtx;
		vtx += stride;
		for(int j = 0; j < 3; j++)
			{
			if(p[j] < bmin[j])		// fcomp; test ah,5; jp
				bmin[j] = p[j];
			if(p[j] > bmax[j])		// fcomp; test ah,0x41; jne
				bmax[j] = p[j];
			}
		}

	NxReal dx = bmax[0] - bmin[0];
	NxReal dy = bmax[1] - bmin[1];
	NxReal dz = bmax[2] - bmin[2];

	NxReal center[3];
	center[0] = dx * 0.5f + bmin[0];
	center[1] = dy * 0.5f + bmin[1];
	center[2] = dz * 0.5f + bmin[2];

	if(dx < 0.000001f || dy < 0.000001f || dz < 0.000001f || svcount < 3)
		{
		// 0x0007dc7a. The first test against the running shortest is folded to
		// the constant (fcomp dword [0x00106858]).
		double len = FLT_MAX;
		if(dx > 0.000001f && dx < FLT_MAX)
			len = dx;
		if(dy > 0.000001f && dy < len)
			len = dy;
		if(dz > 0.000001f && dz < len)
			len = dz;

		if(len == FLT_MAX)		// fucompp; test ah,0x44; jp
			{
			dx = dy = dz = 0.01f;
			}
		else
			{
			if(dx < 0.000001f)
				dx = (NxReal) (0.05f * len);
			if(dy < 0.000001f)
				dy = (NxReal) (0.05f * len);
			if(dz < 0.000001f)
				dz = (NxReal) (0.05f * len);
			}

		double x1 = center[0] - dx;
		double x2 = center[0] + dx;
		double y1 = center[1] - dy;
		double y2 = center[1] + dy;
		double z1 = center[2] - dz;
		double z2 = center[2] + dz;

		addPoint(vcount, vertices, (NxReal) x1, (NxReal) y1, (NxReal) z1);
		addPoint(vcount, vertices, (NxReal) x2, (NxReal) y1, (NxReal) z1);
		addPoint(vcount, vertices, (NxReal) x2, (NxReal) y2, (NxReal) z1);
		addPoint(vcount, vertices, (NxReal) x1, (NxReal) y2, (NxReal) z1);
		addPoint(vcount, vertices, (NxReal) x1, (NxReal) y1, (NxReal) z2);
		addPoint(vcount, vertices, (NxReal) x2, (NxReal) y1, (NxReal) z2);
		addPoint(vcount, vertices, (NxReal) x2, (NxReal) y2, (NxReal) z2);
		addPoint(vcount, vertices, (NxReal) x1, (NxReal) y2, (NxReal) z2);

		return true;
		}

	if(scale)
		{
		// 0x0007d73c: 1.0f / extent, then one multiply per coordinate.
		scale[0] = dx;
		scale[1] = dy;
		scale[2] = dz;

		recip[0] = 1.0f / dx;
		recip[1] = 1.0f / dy;
		recip[2] = 1.0f / dz;

		center[0] = recip[0] * center[0];
		center[1] = recip[1] * center[1];
		center[2] = recip[2] * center[2];
		}

	vtx = (const char*) svertices;

	for(NxU32 i = 0; i < svcount; i++)
		{
		const NxReal* p = (const NxReal*) vtx;
		vtx += stride;

		double px = p[0];
		NxReal py = p[1];
		NxReal pz = p[2];

		if(scale)
			{
			px = px * recip[0];
			py = py * recip[1];
			pz = pz * recip[2];
			}

		if(weld)
			{
			NxU32 j;
			for(j = 0; j < vcount; j++)
				{
				NxReal* v = &vertices[j * 3];

				if(fabs(v[0] - px) < normalepsilon
					&& fabs(v[1] - (double) py) < normalepsilon
					&& fabs(v[2] - (double) pz) < normalepsilon)
					{
					// Close enough: keep whichever of the two is farther from the
					// cloud's centre.
					double ax = px - center[0];
					double ay = py - (double) center[1];
					double az = pz - (double) center[2];
					double bx = v[0] - (double) center[0];
					NxReal by = (NxReal) (v[1] - (double) center[1]);
					NxReal bz = (NxReal) (v[2] - (double) center[2]);

					double dist1 = (az * az + ay * ay) + ax * ax;
					double dist2 = ((double) bz * bz + (double) by * by) + bx * bx;

					if(dist2 < dist1)
						{
						v[0] = (NxReal) px;
						v[1] = py;
						v[2] = pz;
						}

					break;
					}
				}

			if(j == vcount)
				{
				NxReal* dest = &vertices[vcount * 3];
				dest[1] = py;
				dest[0] = (NxReal) px;
				dest[2] = pz;
				vcount++;
				}
			}
		else
			{
			NxReal* dest = &vertices[vcount * 3];
			dest[1] = py;
			dest[0] = (NxReal) px;
			dest[2] = pz;
			vcount++;
			}
		}

	// Make sure the clean-up did not leave the cloud degenerate.
	NxReal bmin2[3] = { FLT_MAX, FLT_MAX, FLT_MAX };
	NxReal bmax2[3] = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

	for(NxU32 i = 0; i < vcount; i++)
		{
		const NxReal* p = &vertices[i * 3];
		for(int j = 0; j < 3; j++)
			{
			if(p[j] < bmin2[j])		// fcomp; test ah,5; jp
				bmin2[j] = p[j];
			if(p[j] > bmax2[j])		// fcomp; test ah,0x41; jne
				bmax2[j] = p[j];
			}
		}

	NxReal dx2 = bmax2[0] - bmin2[0];
	NxReal dy2 = bmax2[1] - bmin2[1];
	NxReal dz2 = bmax2[2] - bmin2[2];

	if(dx2 < 0.000001f || dy2 < 0.000001f || dz2 < 0.000001f || vcount < 3)
		{
		// 0x0007da49.
		NxReal cx = dx2 * 0.5f + bmin2[0];
		NxReal cy = dy2 * 0.5f + bmin2[1];
		NxReal cz = dz2 * 0.5f + bmin2[2];

		double len = FLT_MAX;
		if(dx2 >= 0.000001f && dx2 < FLT_MAX)
			len = dx2;
		if(dy2 >= 0.000001f && dy2 < len)
			len = dy2;
		if(dz2 >= 0.000001f && dz2 < len)
			len = dz2;

		if(len == FLT_MAX)		// fucompp; test ah,0x44; jp
			{
			dx2 = dy2 = dz2 = 0.01f;
			}
		else
			{
			if(dx2 < 0.000001f)
				dx2 = (NxReal) (0.05f * len);
			if(dy2 < 0.000001f)
				dy2 = (NxReal) (0.05f * len);
			if(dz2 < 0.000001f)
				dz2 = (NxReal) (0.05f * len);
			}

		double x1 = cx - dx2;
		double x2 = cx + dx2;
		double y1 = cy - dy2;
		double y2 = cy + dy2;
		double z1 = cz - dz2;
		double z2 = cz + dz2;

		vcount = 0;

		addPoint(vcount, vertices, (NxReal) x1, (NxReal) y1, (NxReal) z1);
		addPoint(vcount, vertices, (NxReal) x2, (NxReal) y1, (NxReal) z1);
		addPoint(vcount, vertices, (NxReal) x2, (NxReal) y2, (NxReal) z1);
		addPoint(vcount, vertices, (NxReal) x1, (NxReal) y2, (NxReal) z1);
		addPoint(vcount, vertices, (NxReal) x1, (NxReal) y1, (NxReal) z2);
		addPoint(vcount, vertices, (NxReal) x2, (NxReal) y1, (NxReal) z2);
		addPoint(vcount, vertices, (NxReal) x2, (NxReal) y2, (NxReal) z2);
		addPoint(vcount, vertices, (NxReal) x1, (NxReal) y2, (NxReal) z2);

		return true;
		}

	if(reduce && vcount > maxVertices)
		{
		HullVertexReducer reducer;
		reducer.reduceVertices(mAllocator, vcount, vertices, vcount, vertices, maxVertices);
		}

	return true;
	}

// phys_fn_003247 (0x0007dea0, 121 B)
// phys_fn_003249 (0x0007df20, 299 B)
// One function: 003249 is the body of the vertex loop onward, reached by the
// jump at 0x0007df17. Written into the working directory, "wb". Every face
// index is written 1-based and in reverse order, the triangles' as well as
// the polygons' (0x0007e004-0x0007e01a).
void QhullHost::writeOkObj(const HullResult& result)
	{
	char fname[512];
	sprintf(fname, "QHULL_OK_%04d.obj", ++gQhullObjCounter);
	FILE* fph = fopen(fname, "wb");
	if(!fph)
		return;
	::fprintf(fph, "#Vertices: %8d\r\n", result.mNumOutputVertices);
	::fprintf(fph, "#Faces   : %8d\r\n", result.mNumFaces);
	for(NxU32 i = 0; i < result.mNumOutputVertices; i++)
		{
		const NxReal* v = &result.mOutputVertices[i * 3];
		::fprintf(fph, "v %0.9f %0.9f %0.9f\r\n", v[0], v[1], v[2]);
		}
	const NxU32* idx = result.mIndices;
	if(result.mPolygons)
		{
		for(NxU32 i = 0; i < result.mNumFaces; i++)
			{
			::fprintf(fph, "f ");
			NxU32 pcount = *idx++;
			for(int j = (int) pcount - 1; j >= 0; j--)
				::fprintf(fph, "%d ", idx[j] + 1);
			::fprintf(fph, "\r\n");
			idx += pcount;
			}
		}
	else
		{
		for(NxU32 i = 0; i < result.mNumTriangles; i++)
			{
			::fprintf(fph, "f %d %d %d\r\n", idx[2] + 1, idx[1] + 1, idx[0] + 1);
			idx += 3;
			}
		}
	fclose(fph);
	}

// phys_fn_003251 (0x0007e050, 158 B)
void QhullHost::writeFailObj(NxU32 vcount, const NxReal* vertices, NxU32 stride)
	{
	char fname[512];
	sprintf(fname, "QHULL_FAIL_%04d.obj", ++gQhullObjCounter);
	FILE* fph = fopen(fname, "wb");
	if(!fph)
		return;
	::fprintf(fph, "#Vertices: %8d\r\n", vcount);
	const char* vtx = (const char*) vertices;
	for(NxU32 i = 0; i < vcount; i++)
		{
		const NxReal* p = (const NxReal*) vtx;
		::fprintf(fph, "v %0.9f %0.9f %0.9f\r\n", p[0], p[1], p[2]);
		vtx += stride;
		}
	fclose(fph);
	}

// phys_fn_003253 (0x0007e0f0, 526 B)
// The retry's input: the eight corners of centre +- extent of the cloud's box,
// so a box twice the cloud's. The extents and the centre are spilled (fstp
// dword); the six corner coordinates stay on the FPU stack until each is
// stored.
void QhullHost::boxFallback(NxU32& vcount, NxReal* vertices)
	{
	NxReal bmin[3] = { FLT_MAX, FLT_MAX, FLT_MAX };
	NxReal bmax[3] = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
	for(NxU32 i = 0; i < vcount; i++)
		{
		const NxReal* p = &vertices[i * 3];
		for(int j = 0; j < 3; j++)
			{
			if(p[j] < bmin[j])		// fcomp; test ah,5; jp
				bmin[j] = p[j];
			if(p[j] > bmax[j])		// fcomp; test ah,0x41; jne
				bmax[j] = p[j];
			}
		}
	vcount = 0;
	NxReal dx = bmax[0] - bmin[0];
	NxReal dy = bmax[1] - bmin[1];
	NxReal dz = bmax[2] - bmin[2];
	NxReal center[3];
	center[0] = dx * 0.5f + bmin[0];
	center[1] = dy * 0.5f + bmin[1];
	center[2] = dz * 0.5f + bmin[2];
	double x1 = center[0] - dx;
	double x2 = center[0] + dx;
	double y1 = center[1] - dy;
	double y2 = center[1] + dy;
	double z1 = center[2] - dz;
	double z2 = center[2] + dz;
	addPoint(vcount, vertices, (NxReal) x1, (NxReal) y1, (NxReal) z1);
	addPoint(vcount, vertices, (NxReal) x2, (NxReal) y1, (NxReal) z1);
	addPoint(vcount, vertices, (NxReal) x2, (NxReal) y2, (NxReal) z1);
	addPoint(vcount, vertices, (NxReal) x1, (NxReal) y2, (NxReal) z1);
	addPoint(vcount, vertices, (NxReal) x1, (NxReal) y1, (NxReal) z2);
	addPoint(vcount, vertices, (NxReal) x2, (NxReal) y1, (NxReal) z2);
	addPoint(vcount, vertices, (NxReal) x2, (NxReal) y2, (NxReal) z2);
	addPoint(vcount, vertices, (NxReal) x1, (NxReal) y2, (NxReal) z2);
	}

// phys_fn_003255 (0x0007e300, 99 B)
// Frees through the library's own allocator, not a host's (there is none by
// now).
HullError HullLibrary::ReleaseResult(HullResult& result)
	{
	if(result.mOutputVertices)
		{
		if(mAllocator)
			mAllocator->free(result.mOutputVertices);
		else
			::free(result.mOutputVertices);
		result.mOutputVertices = 0;
		}
	if(result.mIndices)
		{
		if(mAllocator)
			mAllocator->free(result.mIndices);
		else
			::free(result.mIndices);
		result.mIndices = 0;
		}
	return QE_OK;
	}

// phys_fn_003257 (0x0007e370, 95 B)
QhullHost::QhullHost(HullAllocator* allocator)
	{
	mAllocator = allocator;
	mPoints = 0;
	mRemap = 0;
	mLiveBlocks = 0;
	mLiveBytes = 0;
	mPeakBytes = 0;
	mSearchStart = 0;
	mIndices = 0;
	mOutputVertices = 0;
	mOutputCount = 0;
	mIndexCount = 0;
	memset(mLive, 0, sizeof(mLive));
	mArea = 0.0f;
	mVolume = 0.0f;
	}

// phys_fn_003259 (0x0007e3d0, 210 B)
// The fourth argument is ignored (ret 0x10).
void QhullHost::offBegin(int dim, NxU32 numpoints, NxU32 numfacets, int /*numridges*/)
	{
	releaseArrays();
	mDim = dim;
	mNumFacets = numfacets;
	mNumPoints = numpoints;
	mTriangleCount = 0;
	mPointCount = 0;
	mFacetCount = 0;
	mOutputCount = 0;
	mIndexCount = 0;
	mIndexCapacity = 0;
	if(numpoints)
		{
		mPoints = (NxReal*) trackedMalloc(sizeof(NxReal) * 3 * numpoints);
		mRemap = (NxU32*) trackedMalloc(sizeof(NxU32) * mNumPoints);
		memset(mRemap, 0, sizeof(NxU32) * mNumPoints);
		memset(mPoints, 0, sizeof(NxReal) * 3 * mNumPoints);
		mOutputVertices = (NxReal*) trackedMalloc(sizeof(NxReal) * 3 * mNumPoints);
		mIndexCapacity = mNumPoints * 16;
		mIndices = (NxU32*) trackedMalloc(sizeof(NxU32) * mIndexCapacity);
		}
	}

// phys_fn_003261 (0x0007e4b0, 43 B)
void QhullHost::point3(NxReal x, NxReal y, NxReal z)
	{
	if(mPointCount < mNumPoints)
		{
		NxReal* dest = &mPoints[mPointCount * 3];
		dest[0] = x;
		dest[1] = y;
		dest[2] = z;
		mPointCount++;
		}
	}

// phys_fn_003263 (0x0007e4e0, 60 B)
// Variadic, so `this` is on the stack ([esp+4]). The 0x2000-byte buffer is
// formatted (the three-argument CRT call at 0x000f621b) and never read.
int QhullHost::print(FILE* /*stream*/, const char* format, ...)
	{
	char buffer[0x2000];
	va_list args;
	va_start(args, format);
	vsprintf(buffer, format, args);
	va_end(args);
	errexit(1);
	return 0;
	}

// phys_fn_003265 (0x0007e520, 23 B)
void QhullHost::size(NxReal area, NxReal volume)
	{
	mArea = area;
	mVolume = volume;
	}

// phys_fn_003267 (0x0007e540, 32 B)
void QhullHost::errexit(int code)
	{
	releaseArrays();
	longjmp(gQhullJump, code);
	}

// phys_fn_003268 (0x0007e560, 210 B)
// The output vertices are qhull's point ids compacted in first-use order;
// mRemap holds each one's 1-based output id.
void QhullHost::facet(NxU32 count, const NxU32* ids)
	{
	if(mFacetCount < mNumFacets)
		{
		mTriangleCount += count - 2;
		if(mIndexCount < mIndexCapacity)
			mIndices[mIndexCount++] = count;
		for(NxU32 i = 0; i < count; i++)
			{
			NxU32 id = ids[i];
			if(id < mNumPoints)
				{
				if(mRemap[id] == 0)
					{
					const NxReal* source = &mPoints[id * 3];
					NxReal* dest = &mOutputVertices[mOutputCount * 3];
					dest[0] = source[0];
					dest[1] = source[1];
					dest[2] = source[2];
					mOutputCount++;
					mRemap[id] = mOutputCount;
					}
				if(mIndexCount < mIndexCapacity)
					mIndices[mIndexCount++] = mRemap[id] - 1;
				}
			}
		mFacetCount++;
		}
	}

// phys_fn_003270 (0x0007e640, 454 B)
// Fails, with the arrays released either way, unless qhull delivered points,
// the remap, output vertices and an index list. Triangle mode fans each
// polygon [n, i0, i1, ..., in-1] from i0; reversed, each triangle is written
// last vertex first.
bool QhullHost::buildResult(HullResult& result, bool triangles, bool reverse)
	{
	bool ret = false;
	result.mNumOutputVertices = 0;
	result.mOutputVertices = 0;
	result.mNumFaces = 0;
	result.mNumTriangles = 0;
	result.mNumIndices = 0;
	result.mIndices = 0;
	result.mPolygons = true;
	if(mPoints && mRemap && mOutputCount && mIndexCount)
		{
		result.mNumOutputVertices = mOutputCount;
		result.mOutputVertices = (NxReal*) rawAlloc(sizeof(NxReal) * 3 * mOutputCount);
		memcpy(result.mOutputVertices, mOutputVertices, sizeof(NxReal) * 3 * result.mNumOutputVertices);
		result.mNumFaces = mNumFacets;
		result.mNumTriangles = mTriangleCount;
		result.mNumIndices = mIndexCount;
		if(triangles)
			{
			result.mPolygons = false;
			result.mIndices = (NxU32*) rawAlloc(sizeof(NxU32) * 3 * mTriangleCount);
			result.mNumIndices = mTriangleCount * 3;
			const NxU32* source = mIndices;
			NxU32* dest = result.mIndices;
			for(NxU32 i = 0; i < mNumFacets; i++)
				{
				NxU32 pcount = source[0];
				NxU32 i1 = source[1];
				NxU32 i2 = source[2];
				NxU32 i3 = source[3];
				source += 4;
				if(reverse)
					{
					*dest++ = i3;
					*dest++ = i2;
					*dest++ = i1;
					}
				else
					{
					*dest++ = i1;
					*dest++ = i2;
					*dest++ = i3;
					}
				for(NxU32 j = 0; j < pcount - 3; j++)
					{
					i2 = i3;
					i3 = *source++;
					if(reverse)
						{
						*dest++ = i3;
						*dest++ = i2;
						*dest++ = i1;
						}
					else
						{
						*dest++ = i1;
						*dest++ = i2;
						*dest++ = i3;
						}
					}
				}
			}
		else
			{
			result.mPolygons = true;
			result.mIndices = (NxU32*) rawAlloc(sizeof(NxU32) * mIndexCount);
			memcpy(result.mIndices, mIndices, sizeof(NxU32) * mIndexCount);
			}
		ret = true;
		}
	releaseArrays();
	return ret;
	}

// phys_fn_003272 (0x0007e810, 222 B)
// Scans mLive from mSearchStart (never advanced) for a free slot, at most
// 0x1000 entries. With none free the block goes back to the CRT's free even
// when mAllocator made it (0x0007e8d4).
void* QhullHost::trackedMalloc(size_t size)
	{
	BlockHeader* block;
	if(mAllocator)
		block = (BlockHeader*) mAllocator->malloc(size + sizeof(BlockHeader));
	else
		block = (BlockHeader*) ::malloc(size + sizeof(BlockHeader));
	if(!block)
		return block;
	NxU32 slot = mSearchStart;
	for(NxU32 i = 0; i < 0x1000; i++)
		{
		if(!mLive[slot])
			break;
		slot++;
		if(slot == 0x1000)
			slot = 0;
		}
	if(mLive[slot])
		{
		::free(block);
		return 0;
		}
	mLiveBlocks++;
	mLiveBytes += size;
	block->init(size, slot);
	mLive[slot] = block;
	if(mLiveBytes > mPeakBytes)
		mPeakBytes = mLiveBytes;
	return block + 1;
	}

// phys_fn_003274 (0x0007e8f0, 48 B)
void BlockHeader::init(size_t size, NxU32 slot)
	{
	mTag[0] = 'J';
	mTag[1] = 'O';
	mTag[2] = 'H';
	mTag[3] = 'N';
	mTag[4] = 'R';
	mTag[5] = 'A';
	mTag[6] = 'T';
	mTag[7] = 0;
	mSize = size;
	mSlot = slot;
	}

// phys_fn_003275 (0x0007e920, 92 B)
// A pointer without the header (first four bytes "JOHN", non-zero size) is
// ignored.
void QhullHost::trackedFree(void* memory)
	{
	if(!memory)
		return;
	BlockHeader* block = (BlockHeader*) memory - 1;
	if(block->mTag[0] == 'J' && block->mTag[1] == 'O' && block->mTag[2] == 'H' && block->mTag[3] == 'N'
		&& block->mSize)
		{
		mLive[block->mSlot] = 0;
		mLiveBlocks--;
		mLiveBytes -= block->mSize;
		if(mAllocator)
			mAllocator->free(block);
		else
			::free(block);
		}
	}

// phys_fn_003277 (0x0007e980, 130 B)
// Frees every live tracked block: the inlined trackedFree, including its null
// test on the user pointer (0x0007e99f).
QhullHost::~QhullHost()
	{
	releaseArrays();
	for(NxU32 i = 0; i < 0x1000; i++)
		{
		if(mLive[i])
			QhullHost::trackedFree(mLive[i] + 1);
		}
	}

// phys_fn_003279 (0x0007ea10, 819 B)
// Returns QE_OK (0) or QE_FAIL (1). The host is a 0x4054-byte object on this
// frame (_chkstk 0x40d8) and its address is published to gQhullHost; the
// vertex buffer is allocated and freed through that global. One retry: the
// first longjmp back from qhull dumps the cleaned points (QF_WRITE_FAIL_OBJ),
// replaces them with boxFallback's eight corners and runs qhull again; the
// jmp_buf is not re-armed, so a second longjmp lands on the same setjmp with
// the flag clear and falls through to buildResult, which then fails on the
// arrays errexit released.
HullError HullLibrary::CreateConvexHull(const HullDesc& desc, HullResult& result)
	{
	HullError ret = QE_FAIL;
	char* argv[2];
	argv[0] = (char*) "qhull";
	argv[1] = (char*) "o";
	QhullHost host(mAllocator);
	gQhullHost = &host;

	NxU32 vcount = desc.mVcount;
	if(vcount < 8)
		vcount = 8;
	NxReal* vsource = (NxReal*) gQhullHost->trackedMalloc(sizeof(NxReal) * 3 * (vcount + 1));

	NxU32 ovcount;
	NxReal scale[3];
	NxReal* pscale = 0;
	if(desc.mFlags & QF_NORMALIZE)
		pscale = scale;
	bool ok = host.cleanupVertices(desc.mVcount, desc.mVertices, desc.mVertexStride, ovcount, vsource,
		desc.mNormalEpsilon, pscale, (desc.mFlags & QF_WELD) != 0, (desc.mFlags & QF_REDUCE) != 0,
		desc.mMaxVertices);
	if(ok)
		{
		// In memory in the oracle ([ebp+0x6f]); it has to survive the longjmp.
		volatile bool firstTry = true;
		if(setjmp(gQhullJump) == 0)
			runQhull(2, argv, ovcount, vsource);
		else if(firstTry)
			{
			firstTry = false;
			if(desc.mFlags & QF_WRITE_FAIL_OBJ)
				host.writeFailObj(ovcount, vsource, 12);
			host.boxFallback(ovcount, vsource);
			runQhull(2, argv, ovcount, vsource);
			}

		if(host.buildResult(result, (desc.mFlags & QF_TRIANGLES) != 0, (desc.mFlags & QF_REVERSE_ORDER) != 0))
			{
			ret = QE_OK;
			if(pscale)
				{
				for(NxU32 i = 0; i < result.mNumOutputVertices; i++)
					{
					NxReal* v = &result.mOutputVertices[i * 3];
					v[0] = scale[0] * v[0];
					v[1] = scale[1] * v[1];
					v[2] = scale[2] * v[2];
					}
				}

			// Dead in NovodeX: 002233 passes mPolygonizer = NULL
			// (0x0007ebd6-0x0007ecd5).
			if(mPolygonizer && (desc.mFlags & QF_POLYGONIZER))
				{
				HullPolygonizerResult out;
				out.mFlag = false;
				out.mVcount = 0;
				out.mVertices = 0;
				out.mFaceCount = 0;
				out.mIndices = 0;
				bool built;
				if(result.mPolygons)
					built = mPolygonizer->fromPolygons(out, desc.mUnknown18, result.mNumOutputVertices,
						result.mOutputVertices, 12, result.mNumFaces, result.mIndices);
				else
					built = mPolygonizer->fromTriangles(out, desc.mUnknown18, result.mNumOutputVertices,
						result.mOutputVertices, 12, result.mNumFaces, result.mIndices);
				if(built)
					{
					if(result.mPolygons)
						built = mPolygonizer->finishPolygons(out);
					if(built)
						{
						host.rawFree(result.mIndices);
						host.rawFree(result.mOutputVertices);
						result.mNumOutputVertices = out.mVcount;
						result.mOutputVertices = (NxReal*) host.rawAlloc(sizeof(NxReal) * 3 * out.mVcount);
						memcpy(result.mOutputVertices, out.mVertices, sizeof(NxReal) * 3 * out.mVcount);
						result.mNumFaces = out.mFaceCount;
						result.mNumTriangles = out.mFaceCount;
						result.mIndices = (NxU32*) host.rawAlloc(sizeof(NxU32) * out.mIndexCount);
						memcpy(result.mIndices, out.mIndices, sizeof(NxU32) * out.mIndexCount);
						}
					mPolygonizer->release(out);
					}
				}
			}
		}

	if(ret == QE_OK)
		{
		if(desc.mFlags & QF_WRITE_OK_OBJ)
			host.writeOkObj(result);
		}
	else if(desc.mFlags & QF_WRITE_FAIL_OBJ)
		host.writeFailObj(desc.mVcount, desc.mVertices, desc.mVertexStride);

	gQhullHost->trackedFree(vsource);
	return ret;
	}

// Slot +0x1c is phys_fn_001583 (0x0002ea70), a one-byte `ret` shared by
// folding: the narrow-hull hook does nothing. Not a row of this unit.
void QhullHost::narrowHull()
	{
	}

//////////////////////////////////////////////////////////////////////////////
// The seam. qhull's call sites reach the host as `mov ecx,[0x10125080];
// mov edx,[ecx]; call [edx+slot]`; the vendored tree calls these nine C hooks
// at the same sites (External/qhull/novodex/QhullNovodeXHost.h), and each
// makes that call on the published object. They are not rows.

extern "C" {
#include "..\..\External\qhull\novodex\QhullNovodeXHost.h"
}

void qhNovodeXOffBegin(int dim, int numpoints, int numfacets, int numridges)
	{
	gQhullHost->offBegin(dim, (NxU32) numpoints, (NxU32) numfacets, numridges);
	}

void qhNovodeXPoint3(float x, float y, float z)
	{
	gQhullHost->point3(x, y, z);
	}

void qhNovodeXFacet3Vertex(int count, int* pointids)
	{
	gQhullHost->facet((NxU32) count, (const NxU32*) pointids);
	}

void qhNovodeXSize(float totarea, float totvol)
	{
	gQhullHost->size(totarea, totvol);
	}

// Slot +0x10 is variadic and C cannot forward `...`, so the hook runs 003263's
// body against the published object: format into 0x2000 bytes, then
// errexit(1) through the vtable. It does not return.
int qhNovodeXFprintf(FILE* /*stream*/, const char* format, ...)
	{
	char buffer[0x2000];
	va_list args;
	va_start(args, format);
	vsprintf(buffer, format, args);
	va_end(args);
	gQhullHost->errexit(1);
	return 0;
	}

void* qhNovodeXMalloc(size_t size)
	{
	return gQhullHost->trackedMalloc(size);
	}

void qhNovodeXFree(void* memory)
	{
	gQhullHost->trackedFree(memory);
	}

void qhNovodeXNarrowHull()
	{
	gQhullHost->narrowHull();
	}

// Reached from qh_errexit (phys_fn_003413): releases the arrays and longjmps
// to the driver. It does not return.
void qhNovodeXErrexit(int exitcode)
	{
	gQhullHost->errexit(exitcode);
	}
