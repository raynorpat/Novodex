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

# Task 5: the remaining qhull rows (output, initialisation and option paths).
R['phys_fn_002528'] = ('qh_setdelaunay', 'void qh_setdelaunay(int dim, int count, pointT *points)')
R['phys_fn_002532'] = ('qh_voronoi_center', 'pointT *qh_voronoi_center(int dim, setT *points)')
NAMES['phys_fn_002534'] = 'qh_voronoi_center'
NAMES['phys_fn_002536'] = 'qh_voronoi_center'
NAMES['phys_fn_002538'] = 'qh_voronoi_center'
R['phys_fn_002566'] = ('qh_init_qhull_command', 'void qh_init_qhull_command(int argc, char *argv[])')
NAMES['phys_fn_002568'] = 'qh_init_qhull_command'
NAMES['phys_fn_002570'] = 'qh_init_qhull_command'
R['phys_fn_002575'] = ('qh_initqhull_mem', 'void qh_initqhull_mem(void)')
R['phys_fn_002577'] = ('qh_initqhull_start', 'void qh_initqhull_start(FILE *infile, FILE *outfile, FILE *errfile)')
R['phys_fn_002670'] = ('qh_initqhull_globals', 'void qh_initqhull_globals(coordT *points, int numpoints, int dim, boolT ismalloc)')
NAMES['phys_fn_002672'] = 'qh_initqhull_globals'
R['phys_fn_002699'] = ('qh_detvridge3', 'setT *qh_detvridge3(vertexT *atvertex, vertexT *vertex)')
NAMES['phys_fn_002701'] = 'qh_detvridge3'
R['phys_fn_002703'] = ('qh_eachvoronoi', 'int qh_eachvoronoi(FILE *fp, printvridgeT printvridge, vertexT *atvertex, boolT visitall, qh_RIDGE innerouter, boolT inorder)')
NAMES['phys_fn_002705'] = 'qh_eachvoronoi'
R['phys_fn_002743'] = ('qh_printvdiagram2', 'int qh_printvdiagram2(FILE *fp, printvridgeT printvridge, setT *vertices, qh_RIDGE innerouter, boolT inorder)')
R['phys_fn_002813'] = ('qh_printvoronoi', 'void qh_printvoronoi(FILE *fp, int format, facetT *facetlist, setT *facets, boolT printall)')
NAMES['phys_fn_002815'] = 'qh_printvoronoi'
R['phys_fn_003133'] = ('qh_printhashtable', 'void qh_printhashtable(FILE *fp)')
NAMES['phys_fn_003135'] = 'qh_printhashtable'
R['phys_fn_003304'] = ('qh_setprint', 'void qh_setprint(FILE *fp, char* string, setT *set)')
R['phys_fn_003412'] = ('qh_printallstatistics', 'void qh_printallstatistics(FILE *fp, char *string)')
R['phys_fn_003415'] = ('qh_errprint', 'void qh_errprint(char *string, facetT *atfacet, facetT *otherfacet, ridgeT *atridge, vertexT *atvertex)')
NAMES.update({k: v[0] for k, v in R.items()})
DECLS.update({k: '__declspec(naked) ' + v[1] for k, v in R.items()})
# Task 5: more of the oracle's static CRT, two short strings, and the oracle CRT's stderr (&_iob[2], .data 0x00122640),
# which qh_detvridge3 passes as the stream of the host print before qh_errexit. The UCRT's stderr is a function's result,
# not a link-time address, so the transcription pushes an object of its own there (an error-exit print only).
SYMBOLS.update({0x000f5130: '_strrchr', 0x000f50a0: '_strstr', 0x000f5068: '_exit', 0x000f5315: '_sprintf',
                0x000f51e0: '_strncat'})
STRINGS.update({0x1010c888: 'nxQhStrPercentD', 0x1010c88c: 'nxQhStrCloseBrace'})
PUSHES[0x10122640] = 'nxQhOracleStderr'
DATADECL += ['static char nxQhOracleStderr[0x20];   /* stands for the oracle CRT\'s stderr (.data 0x00122640) */']
# Any other address inside qh_qh, qhmem or qh_qhstat by its offset: their oracle bases are constant over every
# vendored_data_map.csv entry (0x00124678, 0x00124fd8, 0x00125490), so the layouts agree field by field.
for _base, _end, _name in ((0x10124678, 0x10124fd8, 'qh_qh'), (0x10124fd8, 0x10125040, 'qhmem'),
                           (0x10125490, 0x10126400, 'qh_qhstat')):
    for _a in range(_base, _end):
        DATA.setdefault(_a, _name + (' + 0x%x' % (_a - _base) if _a != _base else ''))
        PUSHES.setdefault(_a, DATA[_a])
