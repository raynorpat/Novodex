/*
 * The qhull half of NxPhysicsThirdPartyTests' hull family, in its own C
 * translation unit because it needs the merged tree's qhull_a.h (its fprintf
 * redirection and `qh` macros stay out of the C++ harness).
 *
 * Two things live here:
 *
 *   * nxQhullRun: the NovodeX driver's call sequence (phys_fn_003236 at
 *     0x0007d420: qh_init_A, qh_initflags, qh_init_B, qh_qhull,
 *     qh_check_output, qh_produce_output) run through a table of entry points, so the SAME code
 *     drives the oracle (entries at base + recorded RVA) and the candidate
 *     (the vendored functions linked into this exe). An error exit comes back
 *     through a longjmp: the oracle's slot +0x20 hook and, on the candidate
 *     side, a SIGABRT handler, because ThirdPartyHost.cpp's qhNovodeXErrexit
 *     aborts.
 *   * nxQhullTape: a walk of a finished hull in a qhT image, pushing every
 *     facet, ridge and vertex word the build produced: the combinatorial ones
 *     (ids, flags, sets, point indices) on `tape` and the doubles (normals,
 *     offsets, centrums, distances) on `floats`, through `pushDouble` so
 *     that each is taped as a double. It takes the qhT by
 *     address and reads it through the vendored struct definitions, so one walk
 *     serves the candidate's qh_qh and the oracle's (.data:0x00124678, the
 *     address vendored_data_map.csv pairs with _qh_qh+0; all 209 of its qh_qh
 *     field pairs sit at the same offset). It calls no qhull function.
 */

#include <setjmp.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

#include "qhull_a.h"

typedef void (*NxQhPush)(void* tape, unsigned word);
typedef void (*NxQhPushDouble)(void* tape, double value);

typedef void (__cdecl* NxQhInitA)(FILE*, FILE*, FILE*, int, char**);
typedef void (__cdecl* NxQhInitflags)(char*);
typedef void (__cdecl* NxQhInitB)(coordT*, int, int, boolT);
typedef void (__cdecl* NxQhVoid)(void);

typedef struct NxQhullEntries
	{
	NxQhInitA		initA;
	NxQhInitflags	initflags;
	NxQhInitB		initB;
	NxQhVoid		qhull;
	NxQhVoid		checkOutput;
	NxQhVoid		produceOutput;
	FILE*			fin;
	FILE*			fout;
	FILE*			ferr;
	} NxQhullEntries;

static jmp_buf gNxQhullJump;
static int gNxQhullArmed = 0;

/* The oracle's errexit hook and the candidate's SIGABRT both land here. */
void nxQhullErrorExit(int exitcode)
	{
	if(gNxQhullArmed)
		{
		gNxQhullArmed = 0;
		longjmp(gNxQhullJump, 0x100 | (exitcode & 0xff));
		}
	abort();
	}

static void __cdecl nxQhullAbortHandler(int signal_number)
	{
	(void) signal_number;
	nxQhullErrorExit(0xff);
	}

void* nxQhullCandidateState(void)
	{
	return &qh_qh;
	}

unsigned nxQhullStateSize(void)
	{
	return (unsigned) sizeof(qhT);
	}

/* Runs the driver sequence. 0 on success, 0x100|code after an error exit. */
int nxQhullRun(const NxQhullEntries* e, coordT* points, int numpoints, const char* options)
	{
	static char* argv[2] = { "qhull", 0 };
	char command[64];
	volatile int result = 0;
	int jumped;
	void (__cdecl* previous)(int) = signal(SIGABRT, nxQhullAbortHandler);
	_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
	argv[1] = (char*) options;
	jumped = setjmp(gNxQhullJump);
	if(jumped == 0)
		{
		gNxQhullArmed = 1;
		e->initA(e->fin, e->fout, e->ferr, 2, argv);
		strcpy(command, "qhull ");
		strcat(command, options);
		e->initflags(command);
		e->initB(points, numpoints, 3, False);
		e->qhull();
		e->checkOutput();
		e->produceOutput();
		gNxQhullArmed = 0;
		}
	else
		result = jumped;
	signal(SIGABRT, previous ? previous : SIG_DFL);
	return result;
	}

static unsigned nxPointId(const pointT* point, const coordT* points, int numpoints)
	{
	if(!point)
		return 0xfffffffeu;
	if(point >= points && point < points + 3 * numpoints)
		return (unsigned) ((point - points) / 3);
	return 0xffffffffu;
	}

/* Pushes the words after `id` in a facet: the flag bitfields. */
static void nxPushFacetFlags(NxQhPush push, void* tape, const facetT* f)
	{
	const unsigned char* from = (const unsigned char*) &f->id + sizeof(f->id);
	const unsigned char* end = (const unsigned char*) f + sizeof(facetT);
	while(from + 4 <= end)
		{
		unsigned w;
		memcpy(&w, from, 4);
		push(tape, w);
		from += 4;
		}
	}

static void nxPushPointSet(NxQhPush push, void* tape, const setT* set, const coordT* points,
	int numpoints)
	{
	unsigned count = 0;
	void* const* e;
	if(!set)
		{
		push(tape, 0xfffffffdu);
		return;
		}
	for(e = (void* const*) &set->e[0].p; *e; ++e)
		++count;
	push(tape, count);
	for(e = (void* const*) &set->e[0].p; *e; ++e)
		push(tape, nxPointId((const pointT*) *e, points, numpoints));
	}

void nxQhullTape(const void* state, const coordT* points, int numpoints, NxQhPush push, void* tape,
	NxQhPushDouble pushDouble, void* floats)
	{
	const qhT* q = (const qhT*) state;
	const facetT* f;
	const vertexT* v;
	int k;

	push(tape, (unsigned) q->hull_dim);
	push(tape, (unsigned) q->num_facets);
	push(tape, (unsigned) q->num_vertices);
	push(tape, (unsigned) q->num_visible);
	push(tape, q->facet_id);
	push(tape, q->ridge_id);
	push(tape, q->vertex_id);
	pushDouble(floats, q->max_outside);
	pushDouble(floats, q->min_vertex);
	pushDouble(floats, q->DISTround);
	pushDouble(floats, q->ONEmerge);
	pushDouble(floats, q->MINvisible);
	pushDouble(floats, q->MAXcoplanar);
	pushDouble(floats, q->totarea);
	pushDouble(floats, q->totvol);

	for(f = q->facet_list; f && f->next; f = f->next)
		{
		push(tape, f->id);
		nxPushFacetFlags(push, tape, f);
#if !qh_COMPUTEfurthest
		pushDouble(floats, f->furthestdist);
#endif
#if qh_MAXoutside
		pushDouble(floats, f->maxoutside);
#endif
		pushDouble(floats, f->offset);
		if(f->isarea)
			pushDouble(floats, f->f.area);
		if(f->normal)
			for(k = 0; k < 3; ++k)
				pushDouble(floats, f->normal[k]);
		else
			push(tape, 0xfffffffcu);
		if(f->center && !f->tricoplanar)
			for(k = 0; k < 3; ++k)
				pushDouble(floats, f->center[k]);
		else
			push(tape, 0xfffffffcu);
		if(f->vertices)
			{
			vertexT* const* vp;
			for(vp = (vertexT* const*) &f->vertices->e[0].p; *vp; ++vp)
				{
				push(tape, (*vp)->id);
				push(tape, nxPointId((*vp)->point, points, numpoints));
				}
			}
		push(tape, 0xfffffffbu);
		if(f->neighbors)
			{
			facetT* const* np;
			for(np = (facetT* const*) &f->neighbors->e[0].p; *np; ++np)
				push(tape, *np == qh_MERGEridge ? 0xfffffffau : (*np)->id);
			}
		push(tape, 0xfffffffbu);
		if(f->ridges)
			{
			ridgeT* const* rp;
			for(rp = (ridgeT* const*) &f->ridges->e[0].p; *rp; ++rp)
				{
				const ridgeT* r = *rp;
				unsigned word;
				memcpy(&word, (const unsigned char*) r + offsetof(ridgeT, bottom) + sizeof(r->bottom), 4);
				push(tape, word);
				push(tape, r->top ? r->top->id : 0xfffffff9u);
				push(tape, r->bottom ? r->bottom->id : 0xfffffff9u);
				if(r->vertices)
					{
					vertexT* const* vp;
					for(vp = (vertexT* const*) &r->vertices->e[0].p; *vp; ++vp)
						push(tape, (*vp)->id);
					}
				push(tape, 0xfffffff8u);
				}
			}
		push(tape, 0xfffffffbu);
		nxPushPointSet(push, tape, f->outsideset, points, numpoints);
		nxPushPointSet(push, tape, f->coplanarset, points, numpoints);
		}
	push(tape, 0xfffffff7u);
	for(v = q->vertex_list; v && v->next; v = v->next)
		{
		unsigned word;
		memcpy(&word, (const unsigned char*) v + offsetof(vertexT, visitid) + sizeof(v->visitid), 4);
		push(tape, word);
		push(tape, nxPointId(v->point, points, numpoints));
		if(v->neighbors)
			{
			facetT* const* np;
			for(np = (facetT* const*) &v->neighbors->e[0].p; *np; ++np)
				push(tape, (*np)->id);
			}
		push(tape, 0xfffffff6u);
		}
	}
