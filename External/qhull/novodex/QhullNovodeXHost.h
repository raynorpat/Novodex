/*
 * NOVODEX LOCAL MODIFICATION
 * upstream: none -- this file has no counterpart in qhull 2003.1.
 *
 * The seam between vendored qhull and the host engine.
 *
 * qhull's diagnostics, its allocation and its error exit all leave the library
 * through ONE object in the shipped DLL: a NovodeX class with a nine-slot
 * vtable at .rdata:0x00113614, held in a global at .data:0x00125080.
 *
 * established at 0x0007ea51, which is the ONLY write to that global anywhere in
 * the image; the object it installs is a 16,488-byte stack object whose
 * constructor at 0x0007e370 rep-stosd-zeroes a 16,384-byte inline arena at
 * this+0x34 and stores its one argument at this+0x4048.
 *
 * Slot census, recounted over all 683 occurrences of the global in .text
 * (611 classified to a slot):
 *
 *     slot +0x10    593 call sites    fprintf -- qh ferr / qh fout
 *     slot +0x14      4 call sites    malloc: 0x0006dade, 0x0006dbb3 and
 *                                     0x0006de24 in mem.c's band, and
 *                                     0x0007d466 in NovodeX's own driver row
 *                                     0x0007d420
 *     slot +0x18      3 call sites    free: 0x0006dc74 in mem.c's band, and
 *                                     0x0007d4ec / 0x0007ed26 in NovodeX's rows
 *     slot +0x20      1 call site     error exit (0x0008480d, qh_errexit)
 *     slot +0x04      6 call sites    emits three 32-bit floats -- a typed
 *                                     geometry dump, not an fprintf; see user.h
 *     +0x00/+0x08/+0x0c/+0x1c   1 each, all in io.c's printers except +0x1c,
 *                                     which is qh_initialhull (0x0007965a)
 *
 * So the object is an I/O redirection shim with allocation attached, not an
 * allocator: 593 of the 611 classified sites print. Two earlier passes reported
 * 561 / 4 / 3 and 587 / 6 / 2 for +0x10 / +0x04 / +0x14; the +0x10 differences
 * are in the classifier rather than the image -- no pass can attribute a load
 * that feeds two calls without ambiguity -- but the +0x14 and +0x18 counts are
 * unambiguous and both earlier passes were low, which is how mem.c's third
 * allocation site went unconverted for a task. What the modification rests on:
 * malloc and free survive in mem.c and in NovodeX's own rows only, the error
 * exit survives in user.c only, and printing survives everywhere else.
 *
 * The object itself is a NovodeX row inside qhull's address span. Task 2b owns
 * it. This header is the interface the vendored tree calls; the host supplies
 * the definitions.
 */
#ifndef qhDEFnovodexhost
#define qhDEFnovodexhost 1

#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* [vtable+0x00], qh_printbegin's qh_PRINToff header (io.c NOVODEX [3],
   0x0006c727): (dim, numpoints, numfacets, totneighbors/2) */
void  qhNovodeXOffBegin(int dim, int numpoints, int numfacets, int numridges);
/* [vtable+0x04], qh_printpointid's 3-d coordinates (io.c NOVODEX [1],
   0x00067f84 and its five inlined copies) */
void  qhNovodeXPoint3(float x, float y, float z);
/* [vtable+0x08], qh_printfacet3vertex's point ids (io.c NOVODEX [2],
   0x00067c7a) */
void  qhNovodeXFacet3Vertex(int count, int *pointids);
/* [vtable+0x0c], qh_printfacets' qh_PRINTsize (io.c NOVODEX [4],
   0x0006d458) */
void  qhNovodeXSize(float totarea, float totvol);
/* [vtable+0x10] */
int   qhNovodeXFprintf(FILE *stream, const char *format, ...);
/* [vtable+0x14] */
void *qhNovodeXMalloc(size_t size);
/* [vtable+0x18] */
void  qhNovodeXFree(void *memory);
/* [vtable+0x1c], qh_initialhull's narrow-hull warning (poly2.c NOVODEX [1],
   0x0007965a) */
void  qhNovodeXNarrowHull(void);
/* [vtable+0x20], reached from qh_errexit at 0x00084800 */
void  qhNovodeXErrexit(int exitcode);

#ifdef __cplusplus
}
#endif

/* The redirect of qhull's plain fprintf onto [vtable+0x10], for the qhull C
   translation units only (the C++ host that defines the hooks above must not
   see it). One definition, reached two ways: user.h (through qhull.h, for the
   ten library files built on qhull_a.h) and mem.h (for mem.c and qset.c, which
   include neither qhull.h nor user.h -- the oracle's mem.c/qset.c prints are on
   +0x10 too, e.g. qh_memalloc's errors and all 15 qset.c groups).

   Function-like on purpose. `(fprintf) args` -- a parenthesised name -- does
   not invoke a function-like macro, so qhull_a.h's traceN macros can name the
   CRT's fprintf while every plain `fprintf (qh ferr, ...)` still reaches the
   host. The oracle does exactly that split: 208 traceN sites call the CRT's
   fprintf at 0x000f4d5a directly, and no plain qhull fprintf does. stdio.h is
   included above, before the macro exists, so its own declaration of fprintf is
   untouched. */
#ifndef __cplusplus
#define fprintf(...) qhNovodeXFprintf(__VA_ARGS__)
#endif

#endif /* qhDEFnovodexhost */
