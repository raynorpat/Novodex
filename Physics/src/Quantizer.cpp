/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// NovodeX's hull library around qhull, band B of the qhull span
// (0x0007fda0-0x000814ed): Xiaolin Wu's colour quantizer ("Efficient
// Statistical Computations for Optimal Color Quantization", Graphics Gems II)
// turned on a point cloud, and reduceVertices, which runs it to cut a cleaned
// cloud down to maxVertices points (qhull-gap piece 4d). Contract:
// units/convex-cooking-contract.md (docs/reconstruction/novodex-physics); the
// bundle is units/gap__Controller.cpp__to__fluids__Fluid.cpp.md.
//
// The file and class names are DESCRIPTIVE; no original identifier is
// evidenced. The Wu member names follow the published algorithm (M3d, Vol,
// Bottom, Top, Var, Maximize, Cut, Hist3d, Quantize), each confirmed against
// the listing by its index arithmetic and call shape. The one constraint the
// image puts on the file name: band B links between qset.c and stat.c, so if
// NovodeX's objects were linked in qhull's alphabetical order its name sorts
// between those two.
//
// Rows are in address order, each under its stable-ID line. Floating point
// follows core/Joint.cpp and QhullHost.cpp: this translation unit is x87 in the
// oracle and is built /arch:IA32 here; a value the listing keeps on the FPU
// stack is a `double`, a value it stores (fstp dword) is an `NxReal`. No row
// here touches the control word. The quantizer's moments are 32-bit integers
// (Wu's `long`) and its squared-moment plane is float, as in the listing.
//
// What the shipped object does, reproduced on purpose:
//
//   * the 0xaf794-byte moment table is allocated (user allocator slot 0 or
//     CRT malloc) and NEVER ZEROED (0x00081178-0x000811d3). Wu's histogram
//     needs zeroed moments; the oracle gets them only because a block that
//     large comes from fresh pages;
//   * reduceVertices sets the output count to min(n, maxVertices) whatever
//     Quantize returns (0x000812db): when Quantize finds fewer boxes, or its
//     calloc fails, the rest of the palette is the allocator's uninitialised
//     bytes, and they are dequantised as vertices;
//   * the quantized coordinate is an inline `fistp qword` under the current
//     rounding mode, and only its low dword is clamped (0x00081208-0x00081271).

#define _CRT_SECURE_NO_WARNINGS

#include "QhullHost.h"

#include <stdlib.h>
#include <math.h>

// Wu's box: [r0,r1] x [g0,g1] x [b0,b1] (exclusive lower bounds) and its cell
// count. 0x1c bytes; Quantize callocs k of them (0x00080c45).
struct WuBox
	{
	NxI32	r0;		// +0x00
	NxI32	r1;		// +0x04
	NxI32	g0;		// +0x08
	NxI32	g1;		// +0x0c
	NxI32	b0;		// +0x10
	NxI32	b1;		// +0x14
	NxI32	vol;	// +0x18
	};

// Wu's directions (the byte Bottom and Top switch on).
enum WuDir
	{
	WU_BLUE		= 0,
	WU_GREEN	= 1,
	WU_RED		= 2
	};

#define WU_CELLS	(33 * 33 * 33)
#define WU_IX(r, g, b)	(((r) * 33 + (g)) * 33 + (b))

// The moment table reduceVertices allocates (0xaf794 bytes, push at
// 0x00081178): five [33][33][33] planes 0x23184 bytes apart (the lea chain at
// 0x00080c73-0x00080c88). Every Wu row is a thiscall on it; Vol, Bottom and Top
// do not read `this`.
class WuQuantizer
	{
	public:
	void					M3d(NxI32* vwt, NxI32* vmr, NxI32* vmg, NxI32* vmb, NxReal* vm2);
	NxI32					Vol(const WuBox* cube, const NxI32* mmt);
	NxI32					Bottom(const WuBox* cube, NxU8 dir, const NxI32* mmt);
	NxI32					Top(const WuBox* cube, NxU8 dir, NxI32 pos, const NxI32* mmt);
	double					Var(const WuBox* cube);
	double					Maximize(const WuBox* cube, NxU8 dir, NxI32 first, NxI32 last, NxI32* cut,
								NxI32 wholeR, NxI32 wholeG, NxI32 wholeB, NxI32 wholeW);
	int						Cut(WuBox* set1, WuBox* set2);
	void					Hist3d(const NxU8* rgb);
	NxU32					Quantize(NxU8* out, NxU32 k);

	NxI32					wt[WU_CELLS];	// +0x00000  point count
	NxI32					mr[WU_CELLS];	// +0x23184  sum of r (x)
	NxI32					mg[WU_CELLS];	// +0x46308  sum of g (y)
	NxI32					mb[WU_CELLS];	// +0x6948c  sum of b (z)
	NxReal					m2[WU_CELLS];	// +0x8c610  sum of r*r + g*g + b*b
	};

typedef char WuQuantizerSizeCheck[sizeof(WuQuantizer) == 0xaf794 ? 1 : -1];
typedef char WuBoxSizeCheck[sizeof(WuBox) == 0x1c ? 1 : -1];

// .data:0x00125088 (phys_data_003857) and .data:0x00125488 (phys_data_003859),
// both in the zero-filled tail of .data: Hist3d's square table and the flag
// that says it is built. Process globals, like the rest of the band's.
static NxI32 gWuSquares[256];
static NxI32 gWuSquaresBuilt;

// 255.0f, .rdata:0x00106888.
static const NxReal gWu255 = 255.0f;

// `fld x; fmul dword [255.0f]; fistp qword; mov eax, low dword` -- the
// listing's inline conversion (0x00081208-0x0008123c), rounding under the
// live control word. Not a row. A C cast would call _ftol2 and truncate; a
// naked helper keeps the multiply and the store on the FPU at the caller's
// word, as X87Sqrt.h's helpers do for fsqrt (the same qword-argument caveat
// applies: the one `double` operand, the x coordinate, is narrowed to 53 bits
// at the pass under a 64-bit precision word only).
#if defined(_MSC_VER) && defined(_M_IX86)
static __declspec(naked) NxI32 __cdecl wuFistp255(double /*x*/)
	{
	__asm
		{
		fld		qword ptr [esp + 4]
		fmul	dword ptr [gWu255]
		fistp	qword ptr [esp + 4]
		mov		eax, dword ptr [esp + 4]
		ret
		}
	}
#else
static NxI32 wuFistp255(double x)
	{
	return (NxI32) llrint(x * gWu255);
	}
#endif

// phys_fn_003347 (0x0007fda0, 125 B)
// phys_fn_003349 (0x0007fe20, 1145 B)
// phys_fn_003351 (0x000802a0, 351 B)
// One function: 003349 is the r loop (from its jump target 0x0007fe20, the
// area arrays' zeroing onward) and 003351 the g loop (0x000802a0). Wu's M3d:
// the moments become cumulative. stdcall-shaped (`ret 0x14`) with ecx = the
// table, the five planes passed explicitly (0x00080c73-0x00080c92). line2 is
// held on the FPU stack across the b loop (fld 0.0f at 0x000802a0), and the
// sum line2 + area2[b] is both stored to area2[b] (fst) and, still unrounded,
// added to m2[ind2] (0x000802f2-0x00080385).
void WuQuantizer::M3d(NxI32* vwt, NxI32* vmr, NxI32* vmg, NxI32* vmb, NxReal* vm2)
	{
	NxI32 area[33];
	NxI32 area_r[33];
	NxI32 area_g[33];
	NxI32 area_b[33];
	NxReal area2[33];

	for(int r = 1; r <= 32; ++r)
		{
		for(int i = 0; i <= 32; ++i)
			{
			area2[i] = 0.0f;
			area[i] = area_r[i] = area_g[i] = area_b[i] = 0;
			}
		for(int g = 1; g <= 32; ++g)
			{
			double line2 = 0.0f;
			NxI32 line = 0;
			NxI32 line_r = 0;
			NxI32 line_g = 0;
			NxI32 line_b = 0;
			for(int b = 1; b <= 32; ++b)
				{
				int ind1 = WU_IX(r, g, b);
				int ind2 = ind1 - 33 * 33;
				line += vwt[ind1];
				line_r += vmr[ind1];
				line_g += vmg[ind1];
				line_b += vmb[ind1];
				line2 = line2 + vm2[ind1];
				area[b] += line;
				area_r[b] += line_r;
				area_g[b] += line_g;
				area_b[b] += line_b;
				double sum2 = line2 + area2[b];
				area2[b] = (NxReal) sum2;
				vwt[ind1] = vwt[ind2] + area[b];
				vmr[ind1] = vmr[ind2] + area_r[b];
				vmg[ind1] = vmg[ind2] + area_g[b];
				vmb[ind1] = vmb[ind2] + area_b[b];
				vm2[ind1] = (NxReal) (sum2 + vm2[ind2]);
				}
			}
		}
	}

// phys_fn_003353 (0x00080400, 138 B)
// Wu's Vol: the moment summed over a box. Integer, wrapping.
NxI32 WuQuantizer::Vol(const WuBox* cube, const NxI32* mmt)
	{
	return	  mmt[WU_IX(cube->r1, cube->g1, cube->b1)]
			- mmt[WU_IX(cube->r1, cube->g1, cube->b0)]
			- mmt[WU_IX(cube->r1, cube->g0, cube->b1)]
			+ mmt[WU_IX(cube->r1, cube->g0, cube->b0)]
			- mmt[WU_IX(cube->r0, cube->g1, cube->b1)]
			+ mmt[WU_IX(cube->r0, cube->g1, cube->b0)]
			+ mmt[WU_IX(cube->r0, cube->g0, cube->b1)]
			- mmt[WU_IX(cube->r0, cube->g0, cube->b0)];
	}

// phys_fn_003355 (0x00080490, 210 B)
// Wu's Bottom: the part of Vol that does not depend on the cut position.
// `ret 0xc`; an unknown direction returns 0 (0x000804a4).
NxI32 WuQuantizer::Bottom(const WuBox* cube, NxU8 dir, const NxI32* mmt)
	{
	switch(dir)
		{
		case WU_RED:
			return	- mmt[WU_IX(cube->r0, cube->g1, cube->b1)]
					+ mmt[WU_IX(cube->r0, cube->g1, cube->b0)]
					+ mmt[WU_IX(cube->r0, cube->g0, cube->b1)]
					- mmt[WU_IX(cube->r0, cube->g0, cube->b0)];
		case WU_GREEN:
			return	- mmt[WU_IX(cube->r1, cube->g0, cube->b1)]
					+ mmt[WU_IX(cube->r1, cube->g0, cube->b0)]
					+ mmt[WU_IX(cube->r0, cube->g0, cube->b1)]
					- mmt[WU_IX(cube->r0, cube->g0, cube->b0)];
		case WU_BLUE:
			return	- mmt[WU_IX(cube->r1, cube->g1, cube->b0)]
					+ mmt[WU_IX(cube->r1, cube->g0, cube->b0)]
					+ mmt[WU_IX(cube->r0, cube->g1, cube->b0)]
					- mmt[WU_IX(cube->r0, cube->g0, cube->b0)];
		}
	return 0;
	}

// phys_fn_003357 (0x00080570, 261 B)
// Wu's Top: the rest of Vol, for a cut at `pos`. `ret 0x10`; an unknown
// direction returns 0 (0x00080588).
NxI32 WuQuantizer::Top(const WuBox* cube, NxU8 dir, NxI32 pos, const NxI32* mmt)
	{
	switch(dir)
		{
		case WU_RED:
			return	  mmt[WU_IX(pos, cube->g1, cube->b1)]
					- mmt[WU_IX(pos, cube->g1, cube->b0)]
					- mmt[WU_IX(pos, cube->g0, cube->b1)]
					+ mmt[WU_IX(pos, cube->g0, cube->b0)];
		case WU_GREEN:
			return	  mmt[WU_IX(cube->r1, pos, cube->b1)]
					- mmt[WU_IX(cube->r1, pos, cube->b0)]
					- mmt[WU_IX(cube->r0, pos, cube->b1)]
					+ mmt[WU_IX(cube->r0, pos, cube->b0)];
		case WU_BLUE:
			return	  mmt[WU_IX(cube->r1, cube->g1, pos)]
					- mmt[WU_IX(cube->r1, cube->g0, pos)]
					- mmt[WU_IX(cube->r0, cube->g1, pos)]
					+ mmt[WU_IX(cube->r0, cube->g0, pos)];
		}
	return 0;
	}

// phys_fn_003359 (0x00080680, 245 B)
// Wu's Var: the box's weighted variance. dr, dg and db are fild'ed and stay on
// the FPU stack (0x000806a3-0x000806d3); xx is summed in Wu's order from the
// float plane (0x00080714-0x00080745); the squares are summed
// (db*db + dg*dg) + dr*dr (0x00080748-0x00080756) and fidiv'ed by the weight.
// The result is returned in st(0) unrounded; the caller stores it.
// Vol(cube, wt) is called last in the listing (0x00080758), with the other
// values on the FPU stack; it is pure, so it is called first here.
double WuQuantizer::Var(const WuBox* cube)
	{
	NxI32 ir = Vol(cube, mr);
	NxI32 ig = Vol(cube, mg);
	NxI32 ib = Vol(cube, mb);
	NxI32 iw = Vol(cube, wt);

	double dr = (double) ir;
	double dg = (double) ig;
	double db = (double) ib;

	double xx =	(double) m2[WU_IX(cube->r1, cube->g1, cube->b1)]
				- m2[WU_IX(cube->r1, cube->g1, cube->b0)]
				- m2[WU_IX(cube->r1, cube->g0, cube->b1)]
				+ m2[WU_IX(cube->r1, cube->g0, cube->b0)]
				- m2[WU_IX(cube->r0, cube->g1, cube->b1)]
				+ m2[WU_IX(cube->r0, cube->g1, cube->b0)]
				+ m2[WU_IX(cube->r0, cube->g0, cube->b1)]
				- m2[WU_IX(cube->r0, cube->g0, cube->b0)];

	return xx - ((db * db + dg * dg) + dr * dr) / (double) iw;
	}

// phys_fn_003361 (0x00080780, 405 B)
// Wu's Maximize: the best cut of `cube` along `dir` in [first, last). `ret
// 0x24`. max starts as 0.0f on the FPU stack (0x000807d5). Each half's
// (r*r + g*g) + b*b is formed from fild'ed moments and fidiv'ed by its weight
// (0x0008085d-0x000808a7, 0x000808b7-0x000808d3); an empty half skips the
// position (je 0x000808fe, and je 0x000808fc on the flags of the weight's
// sub). The sum temp is stored to float (fst at 0x000808db) but compared
// unrounded: max takes the stored float when temp > max (fcomp; test
// ah,0x41; jne -- false on NaN), 0x000808e5-0x000808f8.
double WuQuantizer::Maximize(const WuBox* cube, NxU8 dir, NxI32 first, NxI32 last, NxI32* cut,
	NxI32 wholeR, NxI32 wholeG, NxI32 wholeB, NxI32 wholeW)
	{
	NxI32 baseR = Bottom(cube, dir, mr);
	NxI32 baseG = Bottom(cube, dir, mg);
	NxI32 baseB = Bottom(cube, dir, mb);
	NxI32 baseW = Bottom(cube, dir, wt);

	double max = 0.0f;
	*cut = -1;

	for(NxI32 i = first; i < last; ++i)
		{
		NxI32 halfR = baseR + Top(cube, dir, i, mr);
		NxI32 halfG = baseG + Top(cube, dir, i, mg);
		NxI32 halfB = baseB + Top(cube, dir, i, mb);
		NxI32 halfW = baseW + Top(cube, dir, i, wt);
		if(halfW == 0)
			continue;

		double hr = (double) halfR;
		double hg = (double) halfG;
		double hb = (double) halfB;
		double temp = ((hr * hr + hg * hg) + hb * hb) / (double) halfW;

		halfR = wholeR - halfR;
		halfG = wholeG - halfG;
		halfB = wholeB - halfB;
		halfW = wholeW - halfW;
		if(halfW == 0)
			continue;

		hr = (double) halfR;
		hg = (double) halfG;
		hb = (double) halfB;
		temp = temp + ((hr * hr + hg * hg) + hb * hb) / (double) halfW;

		NxReal stored = (NxReal) temp;
		if(temp > max)
			{
			max = stored;
			*cut = i;
			}
		}

	return max;
	}

// phys_fn_003363 (0x00080920, 486 B)
// Wu's Cut: split set1 at the best of the three directions' cuts, into set1
// and set2. `ret 8`. The maxima are stored to float (0x0008098c, 0x000809b4,
// 0x000809dc) and compared as >= (fcomp; test ah,1; jne -- false on NaN).
// Unlike Wu's, every direction, not only red, returns 0 when its cut is < 0
// (0x00080a0a, 0x00080a3a, 0x00080a4e).
int WuQuantizer::Cut(WuBox* set1, WuBox* set2)
	{
	NxI32 wholeR = Vol(set1, mr);
	NxI32 wholeG = Vol(set1, mg);
	NxI32 wholeB = Vol(set1, mb);
	NxI32 wholeW = Vol(set1, wt);

	NxI32 cutr;
	NxI32 cutg;
	NxI32 cutb;
	NxReal maxr = (NxReal) Maximize(set1, WU_RED, set1->r0 + 1, set1->r1, &cutr, wholeR, wholeG, wholeB, wholeW);
	NxReal maxg = (NxReal) Maximize(set1, WU_GREEN, set1->g0 + 1, set1->g1, &cutg, wholeR, wholeG, wholeB, wholeW);
	NxReal maxb = (NxReal) Maximize(set1, WU_BLUE, set1->b0 + 1, set1->b1, &cutb, wholeR, wholeG, wholeB, wholeW);

	NxU8 dir;
	if(maxr >= maxg && maxr >= maxb)
		{
		dir = WU_RED;
		if(cutr < 0)
			return 0;
		}
	else if(maxg >= maxr && maxg >= maxb)
		{
		dir = WU_GREEN;
		if(cutg < 0)
			return 0;
		}
	else
		{
		dir = WU_BLUE;
		if(cutb < 0)
			return 0;
		}

	set2->r1 = set1->r1;
	set2->g1 = set1->g1;
	set2->b1 = set1->b1;

	switch(dir)
		{
		case WU_RED:
			set2->r0 = set1->r1 = cutr;
			set2->g0 = set1->g0;
			set2->b0 = set1->b0;
			break;
		case WU_GREEN:
			set2->g0 = set1->g1 = cutg;
			set2->r0 = set1->r0;
			set2->b0 = set1->b0;
			break;
		case WU_BLUE:
			set2->b0 = set1->b1 = cutb;
			set2->r0 = set1->r0;
			set2->g0 = set1->g0;
			break;
		}

	set1->vol = (set1->r1 - set1->r0) * (set1->g1 - set1->g0) * (set1->b1 - set1->b0);
	set2->vol = (set2->r1 - set2->r0) * (set2->g1 - set2->g0) * (set2->b1 - set2->b0);
	return 1;
	}

// phys_fn_003365 (0x00080b10, 235 B)
// Wu's Hist3d for one quantized point (three bytes, x y z as r g b). `ret 4`.
// The square table is built on the first call (0x00080b11-0x00080b34). The
// squared moment is the integer sum fild'ed and added to the float plane
// (0x00080bed-0x00080bf5).
void WuQuantizer::Hist3d(const NxU8* rgb)
	{
	if(!gWuSquaresBuilt)
		{
		for(NxI32 i = 0; i < 256; ++i)
			gWuSquares[i] = i * i;
		gWuSquaresBuilt = 1;
		}

	NxI32 r = rgb[0];
	NxI32 g = rgb[1];
	NxI32 b = rgb[2];
	NxI32 inr = (r >> 3) + 1;
	NxI32 ing = (g >> 3) + 1;
	NxI32 inb = (b >> 3) + 1;
	NxI32 ind = WU_IX(inr, ing, inb);

	++wt[ind];
	mr[ind] += r;
	mg[ind] += g;
	mb[ind] += b;
	NxI32 sq = gWuSquares[b] + gWuSquares[g] + gWuSquares[r];
	m2[ind] = (NxReal) ((double) sq + m2[ind]);
	}

// phys_fn_003367 (0x00080c00, 648 B)
// Wu's Quantize: split the whole cube into up to k boxes and write each box's
// mean as a palette entry (three bytes). `ret 8`. k is capped at 256
// (0x00080c12, unsigned). CRT calloc for the variances and the boxes; 0 is
// returned when k is 0 or either calloc fails. The next box to split is the
// first with the largest variance (vv[k] > temp: fcom; test ah,5; jp), and the
// loop ends early when that variance is <= 0 (test ah,0x41; jnp -- false on
// NaN), 0x00080db6-0x00080de1. The mean is a signed idiv truncated to a byte;
// an empty box is (0,0,0). Returns the box count.
NxU32 WuQuantizer::Quantize(NxU8* out, NxU32 k)
	{
	if(k == 0)
		return 0;
	if(k > 256)
		k = 256;

	NxReal* vv = (NxReal*) calloc(k, sizeof(NxReal));
	if(!vv)
		return 0;
	WuBox* cube = (WuBox*) calloc(k, sizeof(WuBox));
	if(!cube)
		{
		free(vv);
		return 0;
		}

	M3d(wt, mr, mg, mb, m2);

	cube[0].r0 = cube[0].g0 = cube[0].b0 = 0;
	cube[0].r1 = cube[0].g1 = cube[0].b1 = 32;

	NxU32 next = 0;
	for(NxU32 i = 1; i < k; ++i)
		{
		if(Cut(&cube[next], &cube[i]))
			{
			// The volume test keeps a one-cell box from being cut.
			vv[next] = (cube[next].vol > 1) ? (NxReal) Var(&cube[next]) : 0.0f;
			vv[i] = (cube[i].vol > 1) ? (NxReal) Var(&cube[i]) : 0.0f;
			}
		else
			{
			vv[next] = 0.0f;	// do not try to split this box again
			i--;				// box i was not made
			}

		next = 0;
		NxReal temp = vv[0];
		for(NxU32 j = 1; j <= i; ++j)
			{
			if(vv[j] > temp)
				{
				temp = vv[j];
				next = j;
				}
			}

		if(temp <= 0.0f)
			{
			k = i + 1;
			break;
			}
		}

	for(NxU32 i = 0; i < k; ++i)
		{
		NxI32 weight = Vol(&cube[i], wt);
		if(weight)
			{
			out[i * 3 + 0] = (NxU8) (Vol(&cube[i], mr) / weight);
			out[i * 3 + 1] = (NxU8) (Vol(&cube[i], mg) / weight);
			out[i * 3 + 2] = (NxU8) (Vol(&cube[i], mb) / weight);
			}
		else
			{
			out[i * 3 + 2] = 0;
			out[i * 3 + 1] = 0;
			out[i * 3 + 0] = 0;
			}
		}

	free(cube);
	free(vv);
	return k;
	}

// phys_fn_003369 (0x00080e90, 93 B)
// phys_fn_003371 (0x00080ef0, 1536 B)
// One function: 003371 begins inside the bounding-box loop (the unrolled
// loop's head, reached by the jump at 0x00080eeb). thiscall on an object with
// no fields (cleanupVertices builds it in a dead argument slot), `ret 0x18`.
// It returns `this` (mov eax,[esp+0x10], the ecx saved at 0x00080e9e); the
// caller does not read it.
//
//   * the box: min and max start at vertex 0, bit copies; x and y compare
//     through fcomp (min: test ah,5; jp, max: test ah,0x41; jne), and the
//     running max z stays on the FPU stack (fcom; test ah,5; jp);
//   * extents and their reciprocals (1.0f / extent) are stored to float
//     (0x0008116a-0x000811bb);
//   * each point is quantized to 8 bits per axis: x's (x - min) * recip stays
//     on the stack, y's and z's are stored to float; each is multiplied by
//     255.0f and converted by fistp qword (0x000811e0-0x00081238), and the
//     low dword is clamped to [0, 255] (signed compares);
//   * Quantize runs on min(n, maxVertices) boxes into a palette from the
//     allocator, and the output count is that number, not Quantize's result;
//   * dequantization is byte * (extent * 0.003921569f) + min, the scale
//     stored to float (0x000812cd-0x000812fe, .rdata:0x00113a10).
//
// The input and output may be the same buffer (cleanupVertices passes its
// own): every read of svertices happens before the first write.
HullVertexReducer* HullVertexReducer::reduceVertices(HullAllocator* allocator, NxU32 svcount,
	const NxReal* svertices, NxU32& vcount, NxReal* vertices, NxU32 maxVertices)
	{
	NxReal bmin[3];
	NxReal bmax[3];
	bmin[0] = svertices[0];
	bmin[1] = svertices[1];
	bmin[2] = svertices[2];
	bmax[0] = svertices[0];
	bmax[1] = svertices[1];
	bmax[2] = svertices[2];

	for(NxU32 i = 1; i < svcount; i++)
		{
		const NxReal* p = &svertices[i * 3];
		if(p[0] < bmin[0])		// fcomp; test ah,5; jp
			bmin[0] = p[0];
		if(p[1] < bmin[1])
			bmin[1] = p[1];
		if(p[2] < bmin[2])
			bmin[2] = p[2];
		if(p[0] > bmax[0])		// fcomp; test ah,0x41; jne
			bmax[0] = p[0];
		if(p[1] > bmax[1])
			bmax[1] = p[1];
		if(bmax[2] < p[2])		// fcom (max z on the stack); test ah,5; jp
			bmax[2] = p[2];
		}

	NxReal dx = bmax[0] - bmin[0];
	NxReal dy = bmax[1] - bmin[1];
	NxReal dz = bmax[2] - bmin[2];

	NxReal recip[3];
	recip[0] = 1.0f / dx;
	recip[1] = 1.0f / dy;
	recip[2] = 1.0f / dz;

	// Not zeroed: see the file comment.
	WuQuantizer* wu;
	if(allocator)
		wu = (WuQuantizer*) allocator->malloc(sizeof(WuQuantizer));
	else
		wu = (WuQuantizer*) malloc(sizeof(WuQuantizer));

	for(NxU32 i = 0; i < svcount; i++)
		{
		const NxReal* p = &svertices[i * 3];

		double fx = ((double) p[0] - bmin[0]) * recip[0];
		NxReal fy = (NxReal) (((double) p[1] - bmin[1]) * recip[1]);
		NxReal fz = (NxReal) (((double) p[2] - bmin[2]) * recip[2]);

		NxI32 ix = wuFistp255(fx);
		NxI32 iy = wuFistp255(fy);
		NxI32 iz = wuFistp255(fz);

		if(ix < 0)
			ix = 0;
		if(iy < 0)
			iy = 0;
		if(iz < 0)
			iz = 0;
		if(ix > 255)
			ix = 255;
		if(iy > 255)
			iy = 255;
		if(iz > 255)
			iz = 255;

		NxU8 rgb[3];
		rgb[0] = (NxU8) ix;
		rgb[1] = (NxU8) iy;
		rgb[2] = (NxU8) iz;
		wu->Hist3d(rgb);
		}

	NxU32 k = svcount;
	if(k > maxVertices)
		k = maxVertices;

	NxU8* palette;
	if(allocator)
		palette = (NxU8*) allocator->malloc(k * 3);
	else
		palette = (NxU8*) malloc(k * 3);

	wu->Quantize(palette, k);

	dx = dx * 0.003921569f;
	dy = dy * 0.003921569f;
	dz = dz * 0.003921569f;

	vcount = k;

	for(NxU32 i = 0; i < k; i++)
		{
		vertices[i * 3 + 0] = (NxReal) ((double) palette[i * 3 + 0] * dx + bmin[0]);
		vertices[i * 3 + 1] = (NxReal) ((double) palette[i * 3 + 1] * dy + bmin[1]);
		vertices[i * 3 + 2] = (NxReal) ((double) palette[i * 3 + 2] * dz + bmin[2]);
		}

	if(allocator)
		{
		allocator->free(palette);
		allocator->free(wu);
		}
	else
		{
		free(palette);
		free(wu);
		}

	return this;
	}
