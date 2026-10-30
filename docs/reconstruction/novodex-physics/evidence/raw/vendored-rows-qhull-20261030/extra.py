# qhull's vendored rows: each defined under its own C name (the candidate symbol of qhull_match.csv, without its `_`).
import csv as _csv

R = {
 'phys_fn_002425': ('qh_distplane', 'void qh_distplane(pointT *point, facetT *facet, realT *dist)'),
 'phys_fn_002438': ('qh_getangle', 'realT qh_getangle(pointT *vect1, pointT *vect2)'),
 'phys_fn_002444': ('qh_getdistance', 'realT qh_getdistance(facetT *facet, facetT *neighbor, realT *mindist, realT *maxdist)'),
 'phys_fn_002485': ('qh_findgooddist', 'facetT *qh_findgooddist(pointT *point, facetT *facetA, realT *distp, facetT **facetlist)'),
 'phys_fn_002487': ('qh_gram_schmidt', 'boolT qh_gram_schmidt(int dim, realT **row)'),
 'phys_fn_002518': ('qh_randommatrix', 'void qh_randommatrix(realT *buffer, int dim, realT **rows)'),
 'phys_fn_002550': ('qh_joggleinput', 'void qh_joggleinput(void)'),
 'phys_fn_002558': ('qh_rotateinput', 'void qh_rotateinput(realT **rows)'),
 'phys_fn_002522': ('qh_scalelast', 'void qh_scalelast(coordT *points, int numpoints, int dim, coordT low, coordT high, coordT newhigh)'),  # moved from Task 5: see the ledger
 'phys_fn_002571': ('qh_initqhull_buffers', 'void qh_initqhull_buffers(void)'),
 'phys_fn_002970': ('qh_tracemerging', 'void qh_tracemerging(void)'),
 'phys_fn_003101': ('qh_furthestout', 'void qh_furthestout(facetT *facet)'),
 'phys_fn_003107': ('qh_matchduplicates', 'void qh_matchduplicates(facetT *atfacet, int atskip, int hashsize, int *hashcount)'),
 'phys_fn_003184': ('qh_initialvertices', 'setT *qh_initialvertices(int dim, setT *maxpoints, pointT *points, int numpoints)'),
 'phys_fn_003204': ('qh_nextfurthest', 'pointT *qh_nextfurthest(facetT **visible)'),
}
NAMES = {k: v[0] for k, v in R.items()}
NAMES['phys_fn_002573'] = 'qh_initqhull_buffers'
NAMES['phys_fn_003109'] = 'qh_matchduplicates'
DECLS = {k: '__declspec(naked) ' + v[1] for k, v in R.items()}
INCLUDES = ['qhull_a.h']
HEAD_COMMENT = 'see head.txt'

# Oracle .data by the candidate's own globals, as vendored_data_map.csv learned them: qh_qh, qh_qhstat, qhmem and the rest.
DATA = {}
for _r in _csv.DictReader(open('D:/github/Novodex/docs/reconstruction/novodex-physics/evidence/phase4-third-party-map/vendored_data_map.csv',
                               encoding='utf-8')):
    _s = _r['candidate_symbol']
    if _s.startswith('_') and not _s.startswith('__'):
        _base, _, _off = _s[1:].partition('+')
        DATA[0x10000000 + int(_r['oracle_rva'], 16)] = _base + (' + ' + _off if _off else '')
# The host object's pointer (.data 0x00125080, QhullNovodeXHost.h): the candidate's gQhullHost, whose QhullHost has the
# oracle's nine-slot table, so the rows call through it as the oracle does.
DATA[0x10125080] = 'nxQhHost'
DATADECL = ['extern int qh_rand_seed;   /* geom2.c, not in a header */',
            'extern void* nxQhHost;',
            '#pragma comment(linker, "/alternatename:_nxQhHost=?gQhullHost@@3PAVQhullHost@@A")']
# The oracle's static CRT, by the candidate CRT's entry points (VC7.1's time_t is 32 bits: __time32, __localtime32).
SYMBOLS = {0x000f4d5a: '_fprintf', 0x000f4722: '_malloc', 0x000f5bff: '__time32', 0x000f5fa4: '__localtime32',
           0x000f515d: '_clock', 0x000f4dc0: '_floor'}   # 0x000f4dc0 is floor: its caller fistps st(0)
PUSHES = dict(DATA)   # pushed addresses of qh_qh fields
TABLES = {0x1005c7ac: 7}   # qh_distplane's switch on qh hull_dim - 2 (cmp ecx, 6)
