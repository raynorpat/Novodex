"""qhull's vendored rows (External/qhull/novodex/QhullRows.c): compare the built candidate functions with the oracle listing, instruction by instruction.

Usage (from the repository root, after building NxPhysics Release Win32):

    python docs/reconstruction/novodex-physics/evidence/vendored-rows-qhull-listing-compare.py build/Release/NxPhysics.dll build/Release/NxPhysics.map
"""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import listing_compare_lib as lib

ROWS = [('qh_distplane', 0x5c5c0, 0x5c7aa),
        ('qh_getangle', 0x5d410, 0x5d49e),
        ('qh_getdistance', 0x5d560, 0x5d668),
        ('qh_findgooddist', 0x5f170, 0x5f352),
        ('qh_gram_schmidt', 0x5f360, 0x5f458),
        ('qh_randommatrix', 0x5fec0, 0x5ff35),
        ('qh_scalelast', 0x5fff0, 0x60125),
        ('qh_joggleinput', 0x61490, 0x6177b),
        ('qh_rotateinput', 0x61cd0, 0x61d26),
        ('qh_initqhull_buffers', 0x621c0, 0x62315),
        ('qh_tracemerging', 0x70e90, 0x70f4e),
        ('qh_furthestout', 0x76100, 0x761cf),
        ('qh_matchduplicates', 0x76230, 0x7660f),
        ('qh_initialvertices', 0x79720, 0x79b77),
        ('qh_nextfurthest', 0x7abb0, 0x7ae1e)]
EXPECTED = {0x5c5c0: 'qh_distplane',
            0x5d560: 'qh_getdistance',
            0x5ea60: 'qh_copypoints',
            0x5ec50: 'qh_detsimplex',
            0x5ee20: 'qh_divzero',
            0x5f600: 'qh_maxsimplex',
            0x5fe20: 'qh_rand',
            0x5ff40: 'qh_rotatepoints',
            0x60420: 'qh_setdelaunay',
            0x60a40: 'qh_detjoggle',
            0x62560: 'qh_option',
            0x6da50: 'qh_memalloc',
            0x73d20: 'qh_appendfacet',
            0x742f0: 'qh_gethash',
            0x744a0: 'qh_matchvertices',
            0x74710: 'qh_pointid',
            0x74780: 'qh_removefacet',
            0x76100: 'qh_furthestout',
            0x76930: 'qh_newvertex',
            0x76d80: 'qh_prependfacet',
            0x79350: 'qh_furthestnext',
            0x7ab80: 'qh_errexit2',
            0x7ae20: 'qh_precision',
            0x7ed50: 'qh_setdel',
            0x7edb0: 'qh_setdellast',
            0x7ef90: 'qh_setfree',
            0x7f020: 'qh_setin',
            0x7f0a0: 'qh_setlast',
            0x7f0d0: 'qh_setnew',
            0x7f2a0: 'qh_setsize',
            0x7f670: 'qh_setdelnth',
            0x7f9b0: 'qh_setaddnth',
            0x7fa60: 'qh_setappend',
            0x7fba0: 'qh_settemp',
            0x7fd20: 'qh_settempfree',
            0x84800: 'qh_errexit',
            0x84820: 'qh_errprint',
            0xf4722: '_malloc',
            0xf4d5a: '_fprintf',
            0xf4dc0: '__ftol2_sse',
            0xf515d: '_clock',
            0xf5bff: '__time32',
            0xf5fa4: '__localtime32'}
TABLES = {0x1005c7ac: 7}
DATA = [(0x10122340, '_qh_rand_seed', 'qh_rand_seed'), (0x10125080, 'gQhullHost', 'qhull_host')]
STATIC_RANGES = [(0x10124678, 0x10124fd8, '_qh_qh'), (0x10124fd8, 0x10125040, '_qhmem'), (0x10125490, 0x10126400, '_qh_qhstat')]
lib.run(ROWS, EXPECTED, TABLES, DATA, static_ranges=STATIC_RANGES)
