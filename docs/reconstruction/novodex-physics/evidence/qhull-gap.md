# qhull gap

This file records the work of the plan `docs/superpowers/plans/2026-09-28-qhull-gap.md`: finishing
the `gap:Controller.cpp..fluids\Fluid.cpp` unit. Part one gives the vendored qhull rows that
vendored-correspondence Task 5b held back their execution evidence and promotes the ones it
supports. Part two reconstructs NovodeX's convex-cooking layer around qhull. The matcher, the
review CSVs, the harness and the promotion policy are the ones in `vendored-correspondence.md`.

## Task 1: execution families for the held-back qhull rows

### The held-back rows

Task 5b left 238 qhull rows at `discovered` (`vendored-correspondence.md`, "Rows left at
`discovered`, by reason"). Recomputed from `inventory.json`, `qhull_match.csv`,
`qhull_review.csv` and the Task 5b `vendored_coverage.csv`, by the same precedence:

| Reason | Groups | Rows | Bytes |
|---|---:|---:|---:|
| DIFF | 47 | 96 | 45,975 |
| not executed, x87 code | 48 | 65 | 19,238 |
| not executed, inlining or other-immediate note | 42 | 59 | 12,704 |
| only compared execution is QR1 (discrete) | 10 | 10 | 2,074 |
| review `equivalent-option-gated` | 8 | 8 | 3,683 |

### What was added

`tests/PhysicsThirdPartyTests.cpp` (`nxDriveQhullGap`) and `tests/PhysicsThirdPartyQhull.c`
(`nxQhullRunDim`, `nxQhullTapeDim`, `nxQhullDirect`). Every run goes through the NovodeX driver
sequence the Task 4 hull family uses (`qh_init_A`, `qh_initflags`, `qh_init_B`, `qh_qhull`,
`qh_check_output`, `qh_produce_output`), on the oracle at its RVAs and on the candidate linked
into the exe, over the same points. It now takes any dimension. For `d` and `v` it sets
`qh PROJECTdelaunay` between `qh_initflags` and `qh_init_B`, which is what `qh_readpoints` does
for a file and a library caller must do itself.

Each family is a pair: a discrete tape and a float tape, like `qhull_hull` and
`qhull_hull_x87`. Each run tapes:

- whether it took an error exit;
- the finished hull, in any dimension: facets, flags, vertices, neighbours, outside and coplanar
  sets, normals, offsets, the input points after projection or joggle, and the hull totals;
- everything qhull printed, through both routes below.

| Family pair | Runs | What it drives |
|---|---:|---|
| `qhull_output` | 69 | `qh_produce_output` over the 3-d print formats: `s f i n p m G`, `FF Fi Fn Fa FA Fc FC FD Fo FI FN FO FP FQ FS Fs Ft Fv FV Fx FM Fm`, `Gv Gp Gc Gh Gr Gi Gn Go Gt`, `PG`, `Ts`, `Qt` variants; the tetrahedron, cube, lattice, sphere, slab, duplicates, cylinder and the three error exits |
| `qhull_output_dims` | 33 | the same over 2-d (a square with collinear edge points, a jittered circle, a random set) and 4-d hulls (the tesseract, random points, the 3^4 lattice) |
| `qhull_output_delaunay` | 22 | `d` and `v` over 2-d input: `s i G m Fv Qt Qz Qu FA`, `v o p Fv Fi Fo G Fc FN s`, `Qbb` |
| `qhull_output_delaunay3` | 10 | `d` and `v` over 3-d input (a 4-d hull) |
| `qhull_trace` | 26 | `T1`-`T5`, `Tc`, `TPn`, `TMn`, `TVn`, `TCn`, `TWn`, `TFn`, in 2-d, 3-d, 4-d and Delaunay |
| `qhull_options` | 58 | merge thresholds (`C`, `A`, `W`, `V`, `U`, `E`), `Qc Qi`, `Q0`-`Q9`, `Qv`, `Qm`, `Qf`, good facets (`Qg`, `QGn`, `QVn`, `Pg`), print filters (`Pd`, `PD`, `PA`, `PM`, `PF`), projection and scaling (`Qb0:0B0:0`, `Qb0:-1B0:1`) |
| `qhull_merge` | 19 | merge-heavy inputs: a cube's faces sampled at 360 points 1e-6 off the plane, 500 cospherical points, eight tight clusters, a cone, 1000 points in a box, a jittered circle, under `o`, `C-0`, `Qx`, `Qt` and thresholds |
| `qhull_merge2` | 21 | the `Qn` switches over those sets, larger thresholds, `Qf`, `d Qt` over co-circular input, `QG-0 Pg`, `TF1` |
| `qhull_random` | 9 | `QJ`, `QJ0.001`, `Qr`, `R0.001`: qhull's own random numbers without the rotation |
| `qhull_direct` | 12 | direct calls into 33 out-of-line printers and helpers on each side's finished hull (below) |
| `qhull_exact_output`, `qhull_exact_other` | 76, 62 | the 138 runs above whose discrete AND float tapes compare exactly, run again as families of their own (below) |
| `qhull_paths` | 11 | DIVERGENT: the runs whose search path differs (below) |
| `qhull_rotation` | 6 | DIVERGENT: `QRn`, as `qhull_hull_rotated` |

**Direct calls.** The test exe's compiler inlines 40 qhull groups into their only callers, and
its `/OPT:REF` then drops the out-of-line body (`vendored_coverage.csv` `identity` reads
`thirdparty:absent`). `NxPhysics.dll` keeps those bodies (`/OPT:NOREF`), and the oracle calls its
rows out of line, so the body the matcher compared runs only when it is called. `nxQhullDirect`
names 33 such functions and helpers, which keeps their bodies in the exe, and calls each on the
same side's finished hull, with the oracle's row at its RVA (`kNxQhDirectRva`, from
`qhull_match.csv`) on the oracle's hull: `qh_printextremes` (and `_2d`, `_d`),
`qh_printfacet2math`, `qh_printfacet3vertex`, `qh_printfacetNvertex_simplicial` and
`_nonsimplicial`, `qh_printpointid`, `qh_printfacet2geom`, `qh_printpointvect2`,
`qh_printspheres`, `qh_printvdiagram`, `qh_printstatlevel`, `qh_maxouter`, `qh_facetarea`,
`qh_detjoggle`, `qh_rotatepoints`, `qh_isvertex`, `qh_printpoint`, `qh_printvertex`,
`qh_printcenter`, `qh_printpoint3`, `qh_distnorm`, `qh_printline3geom`, `qh_pointvertex`,
`qh_facetvertices`, `qh_facetcenter`, `qh_nearvertex`, `qh_printmatrix`, `qh_printfacetlist`,
`qh_printneighborhood`, `qh_printlists`, `qh_settempfree`. Return values are taped; what they
print goes through the capture. The merge internals the exe also inlines are not called: they
need a merge in progress.

**Exact on both tapes.** An execution is classed by the worst family its drive fills
(`vendored_trace.py`), so every group that runs in, say, `qhull_output` gets `discrete` because
`qhull_output_x87` differs by the `qh_distplane` class. The DIFF-equivalent arm needs `exact`.
So the 138 runs whose two tapes both match (`NXQHGAP_RUNS=1` lists each run's result) run again
as `qhull_exact_output` and `qhull_exact_other`, whose both tapes are registered whole. The
selection is by measurement and is deterministic; a run that stopped matching fails them.

### Output capture

qhull leaves the library through two routes. Each side's output is captured on both routes, and
no C runtime's `FILE*` is handed to the other runtime.

- **The host object** (`[0x10125080]`; `External/qhull/MODIFICATIONS.md`). Slot `+0x10` is every
  plain qhull `fprintf`; `+0x00`, `+0x04`, `+0x08` and `+0x0c` are NovodeX's typed geometry in
  `io.c`. For the oracle the harness installs a stand-in object whose five output slots record
  into one sink; malloc, free, the narrow-hull hook and the error exit are the Task 4 stand-ins.
  For the candidate, `tests/PhysicsThirdPartyHost.cpp` compiles `Physics/src/ThirdPartyHost.cpp`
  unchanged with those five hook names renamed, and defines the real names to forward to the same
  sink while a run installs it (`gNxQhSink`) and to the product shim otherwise. `NxPhysics.dll`
  still links `Physics/src/ThirdPartyHost.cpp` itself, so no product code changed.
  - The sink formats nothing. It records the format string's hash, then each argument by its
    conversion: an integer or character as a discrete word, a `%e/%f/%g` double on the float
    tape at full precision, `%p` as a mask (a heap address), and the stream as `qh fout`,
    `qh ferr` or other.
  - A `%s` argument is tokenized like a file (below), because `qh qhull_options` carries numbers
    qhull formatted with its own CRT's `sprintf` (`qh_option`'s `%2.2g`).
  - Wall-clock values are masked: the arguments of a `CPU seconds` format, the argument before
    `CPU`, and the `At %02d:%02d:%02d` time of day. The cpu-seconds statistic is dropped with its
    description, because `qh_printstatlevel` skips a statistic that is zero, and it is zero on one
    side and not the other.
- **Each side's own CRT.** The `traceN` macros call the CRT's `fprintf` on `qh ferr`: the
  oracle's static CRT at `0x000f4d5a`, and the candidate's UCRT. `qh_printpointid`,
  `qh_printpoint` and `qh_printpointvect` `fputs` to `qh fout`.
  - So `qh fout` and `qh ferr` are real files, opened per run by each side's own CRT. The
    oracle's are opened through its static-CRT `fopen` row (`phys_fn_005671`, `0x000f4251`,
    which is `_fsopen(name, mode, _SH_DENYNO)`) and closed through its `_fclose` row
    (`phys_fn_005673`, `0x000f42b0`), which flushes. The candidate's go through the UCRT.
  - After the close the harness reads both files back with the UCRT, as bytes, and deletes them.
    `NXQHGAP_KEEP=<dir>` keeps them.

**Text is compared as tokens, not bytes.** The two CRTs format `%g` differently:

- three exponent digits against two (`1.4e-015` / `1.4e-15`), so the padding differs too;
- the old CRT prints a negative zero as ` 0`.

So in a CRT-written file:

- whitespace pushes nothing;
- a number is parsed (`strtod`) and compared on the float tape as a double;
- an integer glued to a letter (`f12`, `p3`, `v7`, `#4`) is an id and a discrete word;
- every other run of characters is hashed;
- the trace-4 bucket numbers after `hash`/`hashcount` are masked, because `qh_gethash` hashes
  vertex addresses: heap layout, which differs between the sides and between runs;
- the number before `CPU` is masked.

**`legacy_stdio_float_rounding.obj`.** The rounding of an exact tie differs as well: `%2.2g` of
-3.25 prints `-3.3` on the 2003 CRT and `-3.2` on the default UCRT. `NxPhysics.dll` links
`legacy_stdio_float_rounding.obj` (joint-open-items Task 6), so the shipped candidate prints
`-3.3`. The test exe runs the candidate's qhull, so it now links the object too (`CMakeLists.txt`).
Without it, the `d T2` trace run differed on exactly that digit. Every pre-existing harness line
prints unchanged with it.

### Results

The run is deterministic: two runs are byte-identical on stdout and stderr. The `--self` run
prints the same oracle digests.

**Exact, registered whole: 14 families.**

| Family | Words |
|---|---:|
| `qhull_output` | 113,021 |
| `qhull_output_dims` | 59,282 |
| `qhull_output_delaunay` | 37,707 |
| `qhull_output_delaunay3` | 51,622 |
| `qhull_trace` | 37,863 |
| `qhull_options` | 35,342 |
| `qhull_merge2` | 53,209 |
| `qhull_merge` | 84,575 |
| `qhull_random` | 10,722 |
| `qhull_direct` | 127,485 |
| `qhull_exact_output` and its `_x87` | 55,168 and 24,566 |
| `qhull_exact_other` and its `_x87` | 37,571 and 18,072 |

**Divergent, registered to the oracle digest and held by `kDivergentCeilings`: 14 families.**
Each ceiling is today's measurement.

| Family | Words differing | Discrete | Worst double ulp | `beyond` | `inf_words` | `beyond_abs` |
|---|---:|---:|---:|---:|---:|---:|
| `qhull_output_x87` | 2,225 | 0 | 3.4e15 | 43 | 0 | 1.67e-16 |
| `qhull_output_dims_x87` | 860 | 0 | 2.0e15 | 25 | 0 | 1.11e-16 |
| `qhull_output_delaunay_x87` | 838 | 0 | 6 | 5 | 0 | 8.33e-17 |
| `qhull_output_delaunay3_x87` | 1,233 | 0 | 128 | 62 | 0 | 1.11e-16 |
| `qhull_trace_x87` | 334 | 0 | inf | 61 | 8 | 3.33e-16 |
| `qhull_options_x87` | 1,023 | 0 | 3.4e15 | 12 | 0 | 1.03e-14 |
| `qhull_merge2_x87` | 1,700 | 0 | 2.8e15 | 103 | 0 | 5.55e-15 |
| `qhull_merge_x87` | 2,376 | 0 | 2.8e15 | 26 | 0 | 3.26e-13 |
| `qhull_random_x87` | 317 | 0 | 3.2e15 | 14 | 0 | 1.39e-14 |
| `qhull_direct_x87` | 1,062 | 0 | inf | 43 | 7 | 2.22e-16 |
| `qhull_paths` | 3,212 | 3,204 | – | 0 | 0 | 0 |
| `qhull_paths_x87` | 644 | 1 | inf | 348 | 85 | inf |
| `qhull_rotation` | 268 | 268 | – | 0 | 0 | 0 |
| `qhull_rotation_x87` | 2,001 | 0 | inf | 547 | 20 | 2 |

I checked the ceilings in both directions. Lowering `qhull_output_x87`'s `beyond` to 42 and
`qhull_paths`' discrete to 3,202 failed exactly those two families.

**Attribution.**

- **The float families (`_x87`, all with `discrete=0`).** These are the `qhull_hull_x87` class:
  qhull's doubles differ where the oracle's summation order and register lifetimes are not
  reproduced (`qh_distplane` and the rest of the summation-order unit).
  - Every word more than 4 ulp apart is either a value of magnitude below 1 differing by at most
    3.3e-13, or a value next to zero.
  - The `inf` words are distances next to zero of opposite sign, for example trace's
    `dist= 0` against `dist=-2.775558e-17`, and direct's 1.1e-16 against -1.1e-16.
- **`qhull_paths`: the hull is the same on both sides; how it was searched is not.** Eleven runs:
  - the 4-d lattice with `s`, `C-0`, `Qx` and `Qv`;
  - `d Qbb` and `d Qt` over the 2-d square;
  - `C0.01` over the sphere and over the box;
  - `Qr` and `QR-5 Qr` over the lattice;
  - `T4` over the cube.

  In the first ten, the only differing discrete word is a counter that `qh_printsummary` prints:
  `Number of distance tests for qhull`.
  - The `T4` trace shows why. At `qh_findbest: neighbors of f7, bestdist -1.2`, the candidate
    finds a neighbour further than `bestdist` by a last bit, moves to `f5`, and prints one more
    trace line. The oracle does not. Both then partition the point into `f6`.
  - So a last-bit distance against a tie at `bestdist` sends one side's directed search one
    facet further (the `qh_distplane` class). That changes a count, not the result.
  - The `T4` run is last in the family because its tape is 9 words longer. The words after the
    first difference are misalignment.
- **`qhull_rotation`**: `QRn` rotates the input by qhull's own random matrix. The oracle
  evaluates its Gram-Schmidt with a reciprocal (`0x0005f3d2`, left stock in the overlay, see
  `MODIFICATIONS.md`), and then the merges differ, as in `qhull_hull_rotated`.
- **One run was dropped, not ceilinged.** `d Qt` over the cylinder's two co-circular rings
  diverged by 1,949 discrete words: its Delaunay triangulation of co-circular points differs.
  It is outside every family.

### Coverage before and after, by held-back reason

The trace was re-run on the final build (`vendored-trace-qhull.txt`, `vendored_coverage.csv`).
The binaries are:

| Binary | sha256 |
|---|---|
| Candidate DLL (unchanged) | `f9075db446078439c34e9616c71235da522f8e483ad49504d2aedb003ba303ef` |
| `NxPhysicsThirdPartyTests.exe` | `b054fdc3dc271fc37224998cbd8069b2dce358d2ef5a7736d68e148780433793` |

Identity again finds every traced body the same code as the candidate DLL's, except the
`AABBTreeBuilder` deleting destructor. 18 qhull groups that were absent from the exe are now
present, kept by the direct calls, and their bodies match the DLL's: `qh_detjoggle`,
`qh_facetarea`, `qh_isvertex`, `qh_maxouter`, `qh_printextremes` (and `_2d`, `_d`),
`qh_printfacet2geom`, `qh_printfacet2math`, `qh_printfacet3vertex`,
`qh_printfacetNvertex_nonsimplicial`, `qh_printfacetNvertex_simplicial`, `qh_printpointid`,
`qh_printpointvect2`, `qh_printspheres`, `qh_printstatlevel`, `qh_printvdiagram` and
`qh_rotatepoints`.

Execution classes per group, as `vendored_trace.py coverage` gives them:

- **exact**: some execution matched in every word;
- **outcome-exact**: the best execution's discrete family is exact and its float family
  diverges (`outcome=discrete`, `family_best=exact`);
- **discrete only**: every family it ran in has a discrete difference.

Groups / rows per cell:

| Reason | Groups / rows | When | exact | outcome-exact | discrete only | not executed |
|---|---|---|---|---|---|---|
| DIFF | 47 / 96 | before | 1 / 1 | 20 / 55 | 2 / 4 | 24 / 36 |
| DIFF | 47 / 96 | after | 32 / 71 | 8 / 15 | 1 / 1 | 6 / 9 |
| not executed, x87 | 48 / 65 | before | 0 / 0 | 0 / 0 | 0 / 0 | 48 / 65 |
| not executed, x87 | 48 / 65 | after | 33 / 40 | 9 / 15 | 0 / 0 | 6 / 10 |
| not executed, notes | 42 / 59 | before | 0 / 0 | 0 / 0 | 0 / 0 | 42 / 59 |
| not executed, notes | 42 / 59 | after | 17 / 25 | 8 / 10 | 0 / 0 | 17 / 24 |
| QR1-only | 10 / 10 | before | 0 / 0 | 0 / 0 | 10 / 10 | 0 / 0 |
| QR1-only | 10 / 10 | after | 1 / 1 | 7 / 7 | 2 / 2 | 0 / 0 |
| option-gated | 8 / 8 | before | 0 / 0 | 4 / 4 | 1 / 1 | 3 / 3 |
| option-gated | 8 / 8 | after | 4 / 4 | 2 / 2 | 1 / 1 | 1 / 1 |

Over all 331 qhull groups, the `outcome` column goes from 9 exact / 171 discrete / 151 none to
240 exact / 44 discrete / 47 none.

**By group, after:**

- **DIFF, exact (32):** `qh_buildtracing`, `qh_detroundoff`, `qh_findbestnew`, `qh_init_A`,
  `qh_init_B`, `qh_initbuild`, `qh_initflags`, `qh_initqhull_globals`, `qh_initstatistics`,
  `qh_matchneighbor`, `qh_maxmin`, `qh_meminitbuffers`, `qh_memstatistics`, `qh_pointfacet`,
  `qh_printafacet`, `qh_printbegin`, `qh_printend4geom`, `qh_printfacet2geom`,
  `qh_printfacet3geom_nonsimplicial`, `qh_printfacet3geom_points`,
  `qh_printfacet3geom_simplicial`, `qh_printfacet3math`, `qh_printfacetheader`,
  `qh_printfacets`, `qh_printpoints_out`, `qh_printspheres`, `qh_printstatistics`,
  `qh_printsummary`, `qh_produce_output`, `qh_setequal`, `qh_setfacetplane`,
  `qh_test_appendmerge`.
- **DIFF, outcome-exact (8):** `qh_detvridge3`, `qh_find_newvertex`, `qh_printvdiagram2`,
  `qh_printvoronoi`, `qh_scalelast`, `qh_setdelaunay`, `qh_vertexridges`, `qh_voronoi_center`.
- **DIFF, discrete only (1):** `qh_errprint`.
- **DIFF, not executed (6):**
  - absent from the exe (inlined into `qh_init_A` or its callers): `qh_eachvoronoi`,
    `qh_init_qhull_command`, `qh_initqhull_mem`, `qh_initqhull_start`;
  - not reached: `qh_mergecycle`, `qh_printallstatistics`.
- **x87, exact (33):** `qh_collectstatistics`, `qh_detjoggle`, `qh_distnorm`,
  `qh_facet2point`, `qh_facetarea`, `qh_findgood`, `qh_furthestnext`, `qh_geomplanes`,
  `qh_markkeep`, `qh_maxouter`, `qh_nearvertex`, `qh_newstats`, `qh_nostatistic`,
  `qh_outerinner`, `qh_printcenter`, `qh_printcentrum`, `qh_printfacet2geom_points`,
  `qh_printfacet2math`, `qh_printfacet4geom_nonsimplicial`, `qh_printhyperplaneintersection`,
  `qh_printline3geom`, `qh_printmatrix`, `qh_printpoint3`, `qh_printpointid`,
  `qh_printpointvect`, `qh_printpointvect2`, `qh_printstatlevel`, `qh_printvertex`,
  `qh_projectdim3`, `qh_projectinput`, `qh_projectpoint`, `qh_projectpoints`,
  `qh_rotatepoints`.
- **x87, outcome-exact (9):** `qh_build_withrestart`, `qh_detvnorm`, `qh_findbest_test`,
  `qh_findbestlower`, `qh_findbestneighbor`, `qh_merge_nonconvex`,
  `qh_printfacet4geom_simplicial`, `qh_printvnorm`, `qh_sethyperplane_gauss`.
- **x87, not executed (6):**
  - not reached: `qh_appendmergeset`, `qh_findgooddist`, `qh_furthestout`,
    `qh_matchduplicates`, `qh_maydropneighbor`;
  - absent from the exe: `qh_initqhull_buffers`.
- **Notes, exact (17):** `qh_attachnewfacets`, `qh_facetvertices`, `qh_order_vertexneighbors`,
  `qh_pointvertex`, `qh_printextremes`, `qh_printextremes_2d`, `qh_printfacet3vertex`,
  `qh_printfacetNvertex_simplicial`, `qh_printfacetlist`, `qh_printfacetridges`,
  `qh_printlists`, `qh_printneighborhood`, `qh_printpoint`, `qh_printstats`,
  `qh_printvneighbors`, `qh_redundant_vertex`, `qh_test_vneighbors`.
- **Notes, outcome-exact (8):** `qh_clearcenters`, `qh_facetcenter`, `qh_freebuild`,
  `qh_markvoronoi`, `qh_printvdiagram`, `qh_printvridge`, `qh_setequal_except`,
  `qh_settempfree_all`.
- **Notes, not executed (17):**
  - absent from the exe (inlined into merge code): `qh_createsimplex`, `qh_detvridge`,
    `qh_mergeneighbors`, `qh_mergeridges`, `qh_mergevertex_neighbors`, `qh_mergevertices`,
    `qh_neighbor_intersections`, `qh_updatetested`;
  - not reached: `qh_checkvertex`, `qh_degen_redundant_facet`, `qh_matchvertices`,
    `qh_mergecycle_neighbors`, `qh_mergecycle_ridges`, `qh_mergecycle_vneighbors`,
    `qh_printhashtable`, `qh_setprint`, `qh_triangulate_mirror`.
- **QR1-only:**
  - exact: `qh_setdelsorted`;
  - outcome-exact, now reached without the rotation: `qh_delridge`, `qh_gethash`,
    `qh_outcoplanar`, `qh_renameridgevertex`, `qh_renamevertex`, `qh_setdelnthsorted`,
    `qh_vertexridges_facet`;
  - discrete only: `qh_gram_schmidt` and `qh_rotateinput`, which only `QRn` reaches.
- **Option-gated:**
  - exact: `qh_distplane`, `qh_getangle`, `qh_initialvertices`, `qh_nextfurthest`. These are
    exact executions of the groups. The `RANDOMdist`, `Qr` and `QJ` arms that carry the
    reciprocal difference ran in `qhull_random`, whose discrete tape is exact and whose doubles
    diverge.
  - outcome-exact: `qh_getdistance`, `qh_joggleinput`;
  - discrete only: `qh_randommatrix` (`QRn`);
  - not executed: `qh_tracemerging`.

**What this does not settle.**

- No row changes state in this task. Promotion, including the DIFF-equivalent arm, is the next
  task's.
- An `exact` execution of a DIFF group is evidence for its outputs on these inputs. The arm also
  needs its review and its static proof.
- The six OPCODE groups that ran in the candidate-tree clean-up now read `release` where they
  read `END`, because of the new boundary at `nxDriveQhullGap`. Their outcomes do not change.
  The OPCODE trace excerpt is not regenerated.

### Registration and gates

The registrations are in `tools/gate_targets.ps1`:

- 30 lines: the 28 families, then the totals pair (`driven=` / `oracle digest=`);
- the exact families are registered whole, the divergent ones up to `oracle=`;
- no existing line was edited;
- the Phase 4 floor goes from 135 to 165, and `test_gate_targets.py MINIMUM` matches it.

The gate results are in the Timing row below.

## Timing

| Task | Start | End | Rows written | Bytes written | Notes |
|---|---|---|---:|---:|---|
| 1 | 2026-09-28T07:01:00 | 2026-09-28T08:38:45 | 0 | 0 | Execution families for the 238 held-back qhull rows. 28 families (14 discrete/float pairs, 434 runs) in NxPhysicsThirdPartyTests through the NovodeX driver sequence: print formats in 2-d/3-d/4-d and Delaunay/Voronoi, trace levels T1-T5, option-gated arms, merge-heavy inputs, QJ/Qr/R, direct calls into 33 out-of-line printers/helpers, and the 138 runs exact on both tapes as qhull_exact_output/_other. Capture: the host object's five output slots to one sink on both sides (tests/PhysicsThirdPartyHost.cpp routes the candidate's hooks; ThirdPartyHost.cpp and the DLL unchanged), each CRT's output to files its own fopen opened (oracle static-CRT fopen 0x000f4251/_fclose 0x000f42b0), text compared as tokens; test exe links legacy_stdio_float_rounding.obj. 14 exact + 14 divergent (ceilinged: the qh_distplane float class, qhull_paths' distance-test counts and a trace-4 search step, qhull_rotation's QRn). qhull groups exact 9 -> 240, not executed 151 -> 47; held-back groups exact/outcome-exact/discrete/none: DIFF 32/8/1/6, x87 33/9/0/6, notes 17/8/0/17, QR1-only 1/7/2/0, option-gated 4/2/1/1. Phase 4 floor 135 -> 165. No ledger change; gates 2, 3, 4, 6, 7 pass, Phase 5 red only on `CANDIDATE-MISSING family=vtables`. |
