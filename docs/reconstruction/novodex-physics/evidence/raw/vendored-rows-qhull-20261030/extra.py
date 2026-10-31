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

# Task 4: qhull rows the existing drives reach (the merge, vertex and simplex paths).
R['phys_fn_002697'] = ('qh_detvridge', 'setT *qh_detvridge(vertexT *vertex)')
R['phys_fn_002884'] = ('qh_appendmergeset', 'void qh_appendmergeset(facetT *facet, facetT *neighbor, mergeType mergetype, realT *angle)')
R['phys_fn_002900'] = ('qh_degen_redundant_facet', 'void qh_degen_redundant_facet(facetT *facet)')
NAMES['phys_fn_002902'] = 'qh_degen_redundant_facet'
R['phys_fn_002921'] = ('qh_maydropneighbor', 'void qh_maydropneighbor(facetT *facet)')
NAMES['phys_fn_002923'] = 'qh_maydropneighbor'
NAMES['phys_fn_002925'] = 'qh_maydropneighbor'
R['phys_fn_002927'] = ('qh_mergecycle_neighbors', 'void qh_mergecycle_neighbors(facetT *samecycle, facetT *newfacet)')
R['phys_fn_002929'] = ('qh_mergecycle_ridges', 'void qh_mergecycle_ridges(facetT *samecycle, facetT *newfacet)')
NAMES['phys_fn_002931'] = 'qh_mergecycle_ridges'
R['phys_fn_002933'] = ('qh_mergecycle_vneighbors', 'void qh_mergecycle_vneighbors(facetT *samecycle, facetT *newfacet)')
R['phys_fn_002937'] = ('qh_mergeneighbors', 'void qh_mergeneighbors(facetT *facet1, facetT *facet2)')
R['phys_fn_002939'] = ('qh_mergeridges', 'void qh_mergeridges(facetT *facet1, facetT *facet2)')
R['phys_fn_002943'] = ('qh_mergevertex_neighbors', 'void qh_mergevertex_neighbors(facetT *facet1, facetT *facet2)')
NAMES['phys_fn_002945'] = 'qh_mergevertex_neighbors'
R['phys_fn_002947'] = ('qh_mergevertices', 'void qh_mergevertices(setT *vertices1, setT **vertices2)')
NAMES['phys_fn_002949'] = 'qh_mergevertices'
R['phys_fn_002951'] = ('qh_neighbor_intersections', 'setT *qh_neighbor_intersections(vertexT *vertex)')
NAMES['phys_fn_002953'] = 'qh_neighbor_intersections'
R['phys_fn_002972'] = ('qh_updatetested', 'void qh_updatetested(facetT *facet1, facetT *facet2)')
R['phys_fn_002987'] = ('qh_vertexridges', 'setT *qh_vertexridges(vertexT *vertex)')
NAMES['phys_fn_002989'] = 'qh_vertexridges'
R['phys_fn_002991'] = ('qh_find_newvertex', 'vertexT *qh_find_newvertex(vertexT *oldvertex, setT *vertices, setT *ridges)')
NAMES['phys_fn_002993'] = 'qh_find_newvertex'
R['phys_fn_002995'] = ('qh_mergecycle', 'void qh_mergecycle(facetT *samecycle, facetT *newfacet)')
R['phys_fn_003051'] = ('qh_matchvertices', 'boolT qh_matchvertices(int firstindex, setT *verticesA, int skipA, setT *verticesB, int *skipB, boolT *same)')
R['phys_fn_003093'] = ('qh_checkvertex', 'void qh_checkvertex(vertexT *vertex)')
R['phys_fn_003146'] = ('qh_triangulate_mirror', 'void qh_triangulate_mirror(facetT *facetA, facetT *facetB)')
R['phys_fn_003164'] = ('qh_createsimplex', 'void qh_createsimplex(setT *vertices)')
NAMES['phys_fn_003166'] = 'qh_createsimplex'
NAMES.update({k: v[0] for k, v in R.items()})
DECLS.update({k: '__declspec(naked) ' + v[1] for k, v in R.items()})
# Task 4: qsort and its comparators (pushed by address), and the one-character string qhull prints.
SYMBOLS[0x000f5cb0] = '_qsort'
PUSHES[0x100662e0] = 'qh_compare_facetvisit'
PUSHES[0x1006e650] = 'qh_comparemerge'
STRINGS = {0x101086e4: 'nxQhStrNewline'}
