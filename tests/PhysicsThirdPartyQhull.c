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

/* Runs the driver sequence. 0 on success, 0x100|code after an error exit.
   `dim` is 3 for the NovodeX driver; the qhull-gap families also build 2-d
   and 4-d hulls (the printers and merges only those dimensions reach). */
int nxQhullRunDim(const NxQhullEntries* e, void* state, coordT* points, int numpoints, int dim,
	const char* options, int projectDelaunay)
	{
	static char* argv[2] = { "qhull", 0 };
	char command[320];
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
		/* What qh_readpoints does for 'd'/'v' input and a library caller must do
		   itself: the points carry `dim` coordinates, and qh_init_B's
		   qh_projectinput lifts them (qh_setdelaunay) into hull_dim = dim + 1. */
		if(projectDelaunay)
			((qhT*) state)->PROJECTdelaunay = True;
		e->initB(points, numpoints, dim, False);
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

int nxQhullRun(const NxQhullEntries* e, coordT* points, int numpoints, const char* options)
	{
	return nxQhullRunDim(e, 0, points, numpoints, 3, options, 0);
	}

static unsigned nxPointId(const pointT* point, const coordT* points, int numpoints);

/* Runs through qh_initbuild only, to inspect initial outside sets before
   qh_buildhull consumes the first furthest point. Diagnostic harness use. */
int nxQhullRunInitBuild(const NxQhullEntries* e, void* initbuild, coordT* points, int numpoints,
	const char* options)
	{
	static char* argv[2] = { "qhull", 0 };
	char command[320];
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
		((NxQhVoid) initbuild)();
		gNxQhullArmed = 0;
		}
	else
		result = jumped;
	signal(SIGABRT, previous ? previous : SIG_DFL);
	return result;
	}

void nxQhullFurthestProbe(const void* state, const coordT* points, int numpoints, int set, int side)
	{
	const qhT* q = (const qhT*) state;
	const facetT* f;
	for(f = q->facet_list; f && f->next; f = f->next)
		{
		unsigned long long bits = 0;
		unsigned long long nx = 0, ny = 0, nz = 0, offset = 0;
		memcpy(&bits, &f->furthestdist, sizeof(bits));
		if(f->normal)
			{
			memcpy(&nx, &f->normal[0], sizeof(nx));
			memcpy(&ny, &f->normal[1], sizeof(ny));
			memcpy(&nz, &f->normal[2], sizeof(nz));
			}
		memcpy(&offset, &f->offset, sizeof(offset));
		(fprintf)(stderr, "NXHULL_FURTHEST set=%d side=%d facet=%d distance=%016I64x plane=%016I64x,%016I64x,%016I64x,%016I64x points=",
			set, side, f->id, bits, nx, ny, nz, offset);
		if(f->outsideset)
			{
			pointT* const* pp;
			for(pp = (pointT* const*) &f->outsideset->e[0].p; *pp; ++pp)
				(fprintf)(stderr, "%u,", nxPointId(*pp, points, numpoints));
			}
		(fprintf)(stderr, "\n");
		}
	(fprintf)(stderr, "NXHULL_NEXT set=%d side=%d facet=%d\n", set, side,
		q->facet_next ? q->facet_next->id : -1);
	}

void nxQhullDistanceProbe(const void* state, const coordT* points, int numpoints, int set, int side,
	void* distplane)
	{
	const qhT* q = (const qhT*) state;
	const facetT* f;
	void (__cdecl* runDistplane)(pointT*, facetT*, realT*) = (void (__cdecl*)(pointT*, facetT*, realT*)) distplane;
	for(f = q->facet_list; f && f->next; f = f->next)
		if(f->outsideset)
			{
			pointT* const* pp;
			for(pp = (pointT* const*) &f->outsideset->e[0].p; *pp; ++pp)
				{
				realT dist = 0;
				unsigned long long bits = 0;
				unsigned long long x = 0, y = 0, z = 0;
				runDistplane(*pp, (facetT*) f, &dist);
				memcpy(&bits, &dist, sizeof(bits));
				memcpy(&x, &(*pp)[0], sizeof(x));
				memcpy(&y, &(*pp)[1], sizeof(y));
				memcpy(&z, &(*pp)[2], sizeof(z));
				(fprintf)(stderr, "NXHULL_DIST set=%d side=%d facet=%d point=%u bits=%016I64x xyz=%016I64x,%016I64x,%016I64x\n",
					set, side, f->id, nxPointId(*pp, points, numpoints), bits, x, y, z);
				}
		}
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
	unsigned doubleWord = 0;
	#define NX_QH_PUSH_DOUBLE(label, value, facetid) do { \
		if(getenv("NXHULL_X87_LABEL")) \
			(fprintf)(stderr, "NXHULL_DOUBLE points=%d word=%u facet=%d label=%s value=%.17g\n", \
				q->num_points, doubleWord, facetid, label, (double) (value)); \
		pushDouble(floats, value); \
		doubleWord += 2; \
	} while(0)

	push(tape, (unsigned) q->hull_dim);
	push(tape, (unsigned) q->num_facets);
	push(tape, (unsigned) q->num_vertices);
	push(tape, (unsigned) q->num_visible);
	push(tape, q->facet_id);
	push(tape, q->ridge_id);
	push(tape, q->vertex_id);
	NX_QH_PUSH_DOUBLE("qh.max_outside", q->max_outside, -1);
	NX_QH_PUSH_DOUBLE("qh.min_vertex", q->min_vertex, -1);
	NX_QH_PUSH_DOUBLE("qh.DISTround", q->DISTround, -1);
	NX_QH_PUSH_DOUBLE("qh.ONEmerge", q->ONEmerge, -1);
	NX_QH_PUSH_DOUBLE("qh.MINvisible", q->MINvisible, -1);
	NX_QH_PUSH_DOUBLE("qh.MAXcoplanar", q->MAXcoplanar, -1);
	NX_QH_PUSH_DOUBLE("qh.totarea", q->totarea, -1);
	NX_QH_PUSH_DOUBLE("qh.totvol", q->totvol, -1);

	for(f = q->facet_list; f && f->next; f = f->next)
		{
		push(tape, f->id);
		nxPushFacetFlags(push, tape, f);
#if !qh_COMPUTEfurthest
		NX_QH_PUSH_DOUBLE("facet.furthestdist", f->furthestdist, f->id);
#endif
#if qh_MAXoutside
		NX_QH_PUSH_DOUBLE("facet.maxoutside", f->maxoutside, f->id);
#endif
		NX_QH_PUSH_DOUBLE("facet.offset", f->offset, f->id);
		if(f->isarea)
			NX_QH_PUSH_DOUBLE("facet.area", f->f.area, f->id);
		if(f->normal)
			for(k = 0; k < 3; ++k)
				NX_QH_PUSH_DOUBLE("facet.normal", f->normal[k], f->id);
		else
			push(tape, 0xfffffffcu);
		if(f->center && !f->tricoplanar)
			for(k = 0; k < 3; ++k)
				NX_QH_PUSH_DOUBLE("facet.center", f->center[k], f->id);
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
	#undef NX_QH_PUSH_DOUBLE

void nxQhullAddressProbe(const void* state, const coordT* points, int numpoints, int set, int side)
	{
	const qhT* q = (const qhT*) state;
	const facetT* f;
	(fprintf)(stderr, "NXHULL_ADDR_STATE set=%d side=%d state=%p facets=%p count=%d\n",
		set, side, state, (const void*) q->facet_list, q->num_facets);
	for(f = q->facet_list; f && f->next; f = f->next)
		{
		vertexT* const* vp;
		if(!f->vertices)
			continue;
		for(vp = (vertexT* const*) &f->vertices->e[0].p; *vp; ++vp)
			(fprintf)(stderr, "NXHULL_ADDR set=%d side=%d facet=%d vertex=%d point=%u address=%p\n",
				set, side, f->id, (*vp)->id, nxPointId((*vp)->point, points, numpoints), (const void*) *vp);
		}
}

void nxQhullPlaneProbe(const void* state, int facetId, void* oracleSetPlane, void* candidateSetPlane,
	void* oracleNormalize, void* candidateNormalize)
	{
	const qhT* q = (const qhT*) state;
	const facetT* f;
	for(f = q->facet_list; f && f->next; f = f->next)
		if(f->id == facetId && f->vertices)
			{
			coordT* rows[4] = { 0, 0, 0, 0 };
			coordT oracleNormal[4] = { 0, 0, 0, 0 }, candidateNormal[4] = { 0, 0, 0, 0 };
			realT oracleOffset = 0, candidateOffset = 0;
		coordT oracleNormalized[4] = { 0, 0, 0, 0 }, candidateNormalized[4] = { 0, 0, 0, 0 };
			boolT oracleNearzero = False, candidateNearzero = False;
			vertexT* const* vp = (vertexT* const*) &f->vertices->e[0].p;
			void (__cdecl* runOracle)(int, coordT**, coordT*, boolT, coordT*, realT*, boolT*) =
				(void (__cdecl*)(int, coordT**, coordT*, boolT, coordT*, realT*, boolT*)) oracleSetPlane;
			void (__cdecl* runCandidate)(int, coordT**, coordT*, boolT, coordT*, realT*, boolT*) =
				(void (__cdecl*)(int, coordT**, coordT*, boolT, coordT*, realT*, boolT*)) candidateSetPlane;
		void (__cdecl* runOracleNormalize)(coordT*, int, boolT, realT*, boolT*) =
			(void (__cdecl*)(coordT*, int, boolT, realT*, boolT*)) oracleNormalize;
		void (__cdecl* runCandidateNormalize)(coordT*, int, boolT, realT*, boolT*) =
			(void (__cdecl*)(coordT*, int, boolT, realT*, boolT*)) candidateNormalize;
			int i;
			for(i = 0; i < q->hull_dim; ++i)
				rows[i] = vp[i]->point;
		oracleNormalized[0] = det2_(dY(2,0), dZ(2,0), dY(1,0), dZ(1,0));
		oracleNormalized[1] = det2_(dX(1,0), dZ(1,0), dX(2,0), dZ(2,0));
		oracleNormalized[2] = det2_(dX(2,0), dY(2,0), dX(1,0), dY(1,0));
		candidateNormalized[0] = oracleNormalized[0];
		candidateNormalized[1] = oracleNormalized[1];
		candidateNormalized[2] = oracleNormalized[2];
		(fprintf)(stderr, "NXHULL_PLANE_RAW facet=%d normal=%.17g,%.17g,%.17g\n",
			facetId, oracleNormalized[0], oracleNormalized[1], oracleNormalized[2]);
		runOracleNormalize(oracleNormalized, q->hull_dim, f->toporient, NULL, NULL);
		runCandidateNormalize(candidateNormalized, q->hull_dim, f->toporient, NULL, NULL);
		(fprintf)(stderr, "NXHULL_NORMALIZE oracle normal=%.17g,%.17g,%.17g candidate normal=%.17g,%.17g,%.17g\n",
			oracleNormalized[0], oracleNormalized[1], oracleNormalized[2],
			candidateNormalized[0], candidateNormalized[1], candidateNormalized[2]);
			(fprintf)(stderr, "NXHULL_PLANE_INPUT facet=%d points=", facetId);
			for(i = 0; i < q->hull_dim; ++i)
				(fprintf)(stderr, "%u,", nxPointId(rows[i], q->first_point, q->num_points));
			(fprintf)(stderr, " toporient=%d\n", f->toporient);
			runOracle(q->hull_dim, rows, rows[0], f->toporient, oracleNormal, &oracleOffset, &oracleNearzero);
			runCandidate(q->hull_dim, rows, rows[0], f->toporient, candidateNormal, &candidateOffset, &candidateNearzero);
			(fprintf)(stderr, "NXHULL_PLANE oracle facet=%d offset=%.17g normal=%.17g,%.17g,%.17g nearzero=%d\n",
				facetId, oracleOffset, oracleNormal[0], oracleNormal[1], oracleNormal[2], oracleNearzero);
			(fprintf)(stderr, "NXHULL_PLANE candidate facet=%d offset=%.17g normal=%.17g,%.17g,%.17g nearzero=%d\n",
				facetId, candidateOffset, candidateNormal[0], candidateNormal[1], candidateNormal[2], candidateNearzero);
			return;
			}
	}

/* The qhull-gap families' walk of a finished hull of any dimension: the same
   words nxQhullTape pushes, with point ids taken the way qh_pointid takes them
   (against qh first_point, which qh_projectinput and qh_joggleinput replace),
   hull_dim coordinates per normal and centrum, and the Voronoi/Delaunay and
   "good" flags already inside the facet flag words. */
static unsigned nxPointIdDim(const qhT* q, const pointT* point)
	{
	if(!point)
		return 0xfffffffeu;
	if(point >= q->first_point && point < q->first_point + q->num_points * q->hull_dim)
		return (unsigned) ((point - q->first_point) / q->hull_dim);
	return 0xffffffffu;
	}

static void nxPushPointSetDim(NxQhPush push, void* tape, const qhT* q, const setT* set)
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
		push(tape, nxPointIdDim(q, (const pointT*) *e));
	}

void nxQhullTapeDim(const void* state, NxQhPush push, void* tape, NxQhPushDouble pushDouble, void* floats)
	{
	const qhT* q = (const qhT*) state;
	const facetT* f;
	const vertexT* v;
	int k;

	push(tape, (unsigned) q->hull_dim);
	push(tape, (unsigned) q->num_points);
	push(tape, (unsigned) q->num_facets);
	push(tape, (unsigned) q->num_vertices);
	push(tape, (unsigned) q->num_good);
	push(tape, q->facet_id);
	push(tape, q->ridge_id);
	push(tape, q->vertex_id);
	pushDouble(floats, q->max_outside);
	pushDouble(floats, q->min_vertex);
	pushDouble(floats, q->DISTround);
	pushDouble(floats, q->ONEmerge);
	pushDouble(floats, q->totarea);
	pushDouble(floats, q->totvol);
	if(q->first_point)
		for(k = 0; k < q->num_points * q->hull_dim && k < 4096; ++k)
			pushDouble(floats, q->first_point[k]);

	for(f = q->facet_list; f && f->next; f = f->next)
		{
		push(tape, f->id);
		nxPushFacetFlags(push, tape, f);
		pushDouble(floats, f->offset);
		if(f->normal)
			for(k = 0; k < q->hull_dim; ++k)
				pushDouble(floats, f->normal[k]);
		else
			push(tape, 0xfffffffcu);
		if(f->vertices)
			{
			vertexT* const* vp;
			for(vp = (vertexT* const*) &f->vertices->e[0].p; *vp; ++vp)
				{
				push(tape, (*vp)->id);
				push(tape, nxPointIdDim(q, (*vp)->point));
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
		nxPushPointSetDim(push, tape, q, f->outsideset);
		nxPushPointSetDim(push, tape, q, f->coplanarset);
		}
	push(tape, 0xfffffff7u);
	for(v = q->vertex_list; v && v->next; v = v->next)
		{
		push(tape, v->id);
		push(tape, nxPointIdDim(q, v->point));
		}
	push(tape, 0xfffffff6u);
	}


/* qhull-gap Task 1: direct calls into qhull's out-of-line printers and
   helpers, on a finished hull.

   The test exe's compiler inlines a number of io.c/geom2.c/poly2.c functions
   into their only callers, and its /OPT:REF then drops the out-of-line body
   (vendored_coverage.csv's "thirdparty:absent"). NxPhysics.dll keeps those
   bodies (/OPT:NOREF), and the oracle calls its rows out of line. So the body
   the matcher compared is executed only by calling it: this table names each
   function, which keeps the candidate's body in the exe, and the harness calls
   the oracle's row at its RVA with the same arguments, taken from each side's
   own finished hull. Each call's return value is taped; what it prints goes
   through the same capture as the runs'. */
typedef struct NxQhDirect
	{
	void		(*printextremes)(FILE*, facetT*, setT*, int);
	void		(*printextremes_2d)(FILE*, facetT*, setT*, int);
	void		(*printextremes_d)(FILE*, facetT*, setT*, int);
	void		(*printfacet2math)(FILE*, facetT*, int, int);
	void		(*printfacet3vertex)(FILE*, facetT*, int);
	void		(*printfacetNvertex_simplicial)(FILE*, facetT*, int);
	void		(*printfacetNvertex_nonsimplicial)(FILE*, facetT*, int, int);
	void		(*printpointid)(FILE*, char*, int, pointT*, int);
	void		(*printfacet2geom)(FILE*, facetT*, realT*);
	void		(*printpointvect2)(FILE*, pointT*, coordT*, pointT*, realT);
	void		(*printspheres)(FILE*, setT*, realT);
	void		(*printvdiagram)(FILE*, int, facetT*, setT*, boolT);
	void		(*printstatlevel)(FILE*, int, int);
	realT		(*maxouter)(void);
	realT		(*facetarea)(facetT*);
	realT		(*detjoggle)(pointT*, int, int);
	void		(*rotatepoints)(realT*, int, int, realT**);
	vertexT*	(*isvertex)(pointT*, setT*);
	void		(*printpoint)(FILE*, char*, pointT*);
	void		(*printvertex)(FILE*, vertexT*);
	void		(*printcenter)(FILE*, int, char*, facetT*);
	void		(*printpoint3)(FILE*, pointT*);
	realT		(*distnorm)(int, pointT*, pointT*, realT*);
	void		(*printline3geom)(FILE*, pointT*, pointT*, realT*);
	setT*		(*pointvertex)(void);
	setT*		(*facetvertices)(facetT*, setT*, boolT);
	pointT*		(*facetcenter)(setT*);
	vertexT*	(*nearvertex)(facetT*, pointT*, realT*);
	void		(*printmatrix)(FILE*, char*, realT**, int, int);
	void		(*printfacetlist)(facetT*, setT*, boolT);
	void		(*printneighborhood)(FILE*, int, facetT*, facetT*, boolT);
	void		(*printlists)(void);
	void		(*settempfree)(setT**);
	} NxQhDirect;

/* Oracle RVAs, from qhull_match.csv (each group's first row), in the order above. */
static const unsigned kNxQhDirectRva[] =
	{
	0x00068ce0, 0x00068de0, 0x00068fa0, 0x000677a0, 0x00067c10, 0x00067e00, 0x00067ca0, 0x00067f20,
	0x000690b0, 0x00069ab0, 0x00069c10, 0x00069cd0, 0x00084240, 0x0005f590, 0x00061170, 0x00060a40,
	0x0005ff40, 0x00076200, 0x00069690, 0x00068070, 0x00067500, 0x000696c0, 0x0005ed60, 0x00069520,
	0x00076d20, 0x00068700, 0x000612a0, 0x00076730, 0x0005fbb0, 0x000849b0, 0x0006d6f0, 0x00076fb0,
	0x0007fd20,
	};

void nxQhullDirectEntries(unsigned char* oracleBase, void* out, unsigned size)
	{
	NxQhDirect d;
	if(size != sizeof(NxQhDirect) || sizeof(kNxQhDirectRva) != sizeof(NxQhDirect))
		abort();
	if(oracleBase)
		{
		unsigned i;
		void** slots = (void**) &d;
		for(i = 0; i < sizeof(kNxQhDirectRva) / sizeof(kNxQhDirectRva[0]); ++i)
			slots[i] = oracleBase + kNxQhDirectRva[i];
		}
	else
		{
		d.printextremes = qh_printextremes;
		d.printextremes_2d = qh_printextremes_2d;
		d.printextremes_d = qh_printextremes_d;
		d.printfacet2math = qh_printfacet2math;
		d.printfacet3vertex = qh_printfacet3vertex;
		d.printfacetNvertex_simplicial = qh_printfacetNvertex_simplicial;
		d.printfacetNvertex_nonsimplicial = qh_printfacetNvertex_nonsimplicial;
		d.printpointid = qh_printpointid;
		d.printfacet2geom = qh_printfacet2geom;
		d.printpointvect2 = qh_printpointvect2;
		d.printspheres = qh_printspheres;
		d.printvdiagram = qh_printvdiagram;
		d.printstatlevel = qh_printstatlevel;
		d.maxouter = qh_maxouter;
		d.facetarea = qh_facetarea;
		d.detjoggle = qh_detjoggle;
		d.rotatepoints = qh_rotatepoints;
		d.isvertex = qh_isvertex;
		d.printpoint = qh_printpoint;
		d.printvertex = qh_printvertex;
		d.printcenter = qh_printcenter;
		d.printpoint3 = qh_printpoint3;
		d.distnorm = qh_distnorm;
		d.printline3geom = qh_printline3geom;
		d.pointvertex = qh_pointvertex;
		d.facetvertices = qh_facetvertices;
		d.facetcenter = qh_facetcenter;
		d.nearvertex = qh_nearvertex;
		d.printmatrix = qh_printmatrix;
		d.printfacetlist = qh_printfacetlist;
		d.printneighborhood = qh_printneighborhood;
		d.printlists = qh_printlists;
		d.settempfree = qh_settempfree;
		}
	memcpy(out, &d, sizeof(d));
	}

unsigned nxQhullDirectSize(void)
	{
	return (unsigned) sizeof(NxQhDirect);
	}

/* Calls every entry of `d` on the finished hull in `state` (the same side's
   qh), printing to `fp`. 0, or 0x100|code if a call took qhull's error exit. */
int nxQhullDirect(const void* entries, const void* state, FILE* fp, NxQhPush push, void* tape,
	NxQhPushDouble pushDouble, void* floats)
	{
	const NxQhDirect* d = (const NxQhDirect*) entries;
	const qhT* q = (const qhT*) state;
	volatile int result = 0;
	int jumped;
	void (__cdecl* previous)(int) = signal(SIGABRT, nxQhullAbortHandler);
	jumped = setjmp(gNxQhullJump);
	if(jumped == 0)
		{
		static realT color[3] = { 0.25, 0.5, 0.75 };
		/* qh_rotatepoints uses row[dim] as its scratch row, so there is a fifth. */
		static realT matrix[5][4] = { { 0.8, 0.6, 0, 0 }, { -0.6, 0.8, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 }, { 0, 0, 0, 0 } };
		realT* rows[5];
		realT copy[16];
		const int dim = q->hull_dim;
		facetT* f;
		facetT* first = 0;
		facetT* last = 0;
		int k = 0, i;
		setT* set;
		realT dist;
		gNxQhullArmed = 1;
		for(i = 0; i < 5; ++i)
			rows[i] = matrix[i];
		push(tape, 0x44495200u | (unsigned) dim);	/* "DIR" */
		pushDouble(floats, d->maxouter());
		if(q->DELAUNAY)
			d->printextremes_d(fp, q->facet_list, NULL, True);
		else if(dim == 2)
			d->printextremes_2d(fp, q->facet_list, NULL, True);
		else
			d->printextremes(fp, q->facet_list, NULL, True);
		for(f = q->facet_list; f && f->next && k < 6; f = f->next)
			{
			vertexT* v0;
			vertexT* v1;
			vertexT* found;
			if(f->visible || !f->vertices || !f->normal)
				continue;
			if(!first)
				first = f;
			last = f;
			v0 = (vertexT*) f->vertices->e[0].p;
			v1 = (vertexT*) f->vertices->e[1].p;
			push(tape, f->id);
			pushDouble(floats, d->facetarea(f));
			if(dim == 2)
				{
				d->printfacet2math(fp, f, qh_PRINTmathematica, k);
				d->printfacet2geom(fp, f, color);
				}
			if(dim == 3)
				{
				d->printfacet3vertex(fp, f, qh_PRINToff);
				d->printpoint3(fp, v0->point);
				d->printpointvect2(fp, v0->point, f->normal, v1->point, 0.125);
				d->printline3geom(fp, v0->point, v1->point, color);
				}
			if(f->simplicial)
				d->printfacetNvertex_simplicial(fp, f, qh_PRINToff);
			else
				d->printfacetNvertex_nonsimplicial(fp, f, k, qh_PRINToff);
			d->printcenter(fp, qh_PRINTfacets, "c%d", f);
			d->printvertex(fp, v0);
			found = d->nearvertex(f, v0->point, &dist);
			push(tape, found ? found->id : 0xffffffffu);
			pushDouble(floats, dist);
			found = d->isvertex(v1->point, f->vertices);
			push(tape, found ? found->id : 0xffffffffu);
			d->printpointid(fp, "p", dim, v0->point, k);
			d->printpoint(fp, "q", v1->point);
			pushDouble(floats, d->distnorm(dim, v0->point, f->normal, &f->offset));
			++k;
			}
		set = d->facetvertices(q->facet_list, NULL, True);
		push(tape, (unsigned) (set ? set->e[set->maxsize].i : 0));
		if(dim == 3)
			d->printspheres(fp, set, 0.0625);
		if(q->DELAUNAY && first && first->vertices)
			{
			pointT* center = d->facetcenter(first->vertices);
			for(i = 0; i < dim - 1; ++i)
				pushDouble(floats, center[i]);
			}
		d->settempfree(&set);
		set = d->pointvertex();
		push(tape, (unsigned) (set ? set->e[set->maxsize].i : 0));
		d->settempfree(&set);
		if(q->VORONOI)
			d->printvdiagram(fp, qh_PRINTvertices, q->facet_list, NULL, True);
		if(first && last && first != last)
			d->printneighborhood(fp, qh_PRINTfacets, first, last, True);
		if(last)
			d->printfacetlist(last, NULL, True);
		d->printlists();
		for(i = 0; i < ZEND; ++i)
			d->printstatlevel(fp, i, 0);
		if(q->first_point && dim <= 4)
			{
			int n = q->num_points < 4 ? q->num_points : 4;
			pushDouble(floats, d->detjoggle(q->first_point, q->num_points, dim));
			memcpy(copy, q->first_point, sizeof(realT) * n * dim);
			d->rotatepoints(copy, n, dim, rows);
			for(i = 0; i < n * dim; ++i)
				pushDouble(floats, copy[i]);
			d->printmatrix(fp, "m", rows, dim, dim);
			}
		gNxQhullArmed = 0;
		}
	else
		result = jumped;
	signal(SIGABRT, previous ? previous : SIG_DFL);
	return result;
	}
