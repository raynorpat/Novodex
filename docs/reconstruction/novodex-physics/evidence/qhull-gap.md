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
| `qhull_paths` | 10 | DIVERGENT: the runs whose search path differs (below) |
| `qhull_paths_t4` | 1 | DIVERGENT: the cube at trace level 4, which shows how the search differs (below) |
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
One setting, `{2, "QG-0 Pg"}` (the lattice, good facets from point 0, print good facets), is in
both `qhull_options` and `qhull_merge2`, so it runs twice in `qhull_exact_other`: 138 runs, 137
distinct settings.

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

**Divergent, registered to the oracle digest and held by `kDivergentCeilings`: 16 families.**
Each ceiling is today's measurement. The four `qhull_paths` families are also held to their exact
`length_delta` (`kLengthCeilings`: 0, 0, 9 and 2).

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
| `qhull_paths` | 10 | 10 | – | 0 | 0 | 0 |
| `qhull_paths_x87` | 186 | 0 | 1.6e15 | 14 | 0 | 2.0e-15 |
| `qhull_paths_t4` | 3,202 | 3,194 | – | 0 | 0 | 0 |
| `qhull_paths_t4_x87` | 458 | 1 | inf | 334 | 85 | inf |
| `qhull_rotation` | 268 | 268 | – | 0 | 0 | 0 |
| `qhull_rotation_x87` | 2,001 | 0 | inf | 547 | 20 | 2 |

I checked the ceilings in both directions. Lowering `qhull_output_x87`'s `beyond` to 42 and
(before the T4 split) `qhull_paths`' discrete to 3,202 failed exactly those two families, and
recording `qhull_paths_t4`'s `length_delta` as 8 failed it.

`qhull_paths_t4_x87`'s `beyond_abs` is `inf`, so that one cap holds nothing: after the T4 trace's
extra line every later number is compared against a different one, including the `inf` the trace
prints for an unset distance. Its words, discrete, `beyond`, `inf_words` and `length_delta` caps
hold it.

**Attribution.**

- **The float families (`_x87`, all with `discrete=0`).** These are the `qhull_hull_x87` class:
  qhull's doubles differ where the oracle's summation order and register lifetimes are not
  reproduced (`qh_distplane` and the rest of the summation-order unit).
  - Every word more than 4 ulp apart is either a value of magnitude below 1 differing by at most
    3.3e-13, or a value next to zero.
  - The largest, `qhull_merge_x87`'s 3.26e-13, is run 11 (the eight tight clusters, `C-0.0001`):
    the ratio `qh_printsummary` prints as ` (%.1fx)` after "Maximum distance of point above
    facet" (`qhull.c:1379`, `outerplane/(qh ONEmerge + qh DISTround)`). The oracle passes
    0.53661773568130933 and the candidate 0.53661773568098348, a relative difference of 6e-13 in
    a quotient of two roundoff-sized quantities; both print `0.5`. (`NXQHGAP_FLOAT_MIN=1e-13`
    lists such words with their format.)
  - The `inf` words are next to zero with opposite signs: trace's `dist= 0` against
    `dist=-2.775558e-17`, and direct's 1.1e-16 against -1.1e-16. Some may also be a print
    artefact rather than a value difference: the 2003 CRT prints a negative zero as ` 0`, so a
    trace `det= 0` on the oracle against `det=-0` on the candidate parses to +0 against -0, which
    the tape counts as a sign change, whatever the oracle's value was.
- **`qhull_paths`: the hull is the same on both sides; how it was searched is not.** Ten runs:
  - the 4-d lattice with `s`, `C-0`, `Qx` and `Qv`;
  - `d Qbb` and `d Qt` over the 2-d square;
  - `C0.01` over the sphere and over the box;
  - `Qr` and `QR-5 Qr` over the lattice.

  In each, the only differing discrete word is one counter that `qh_printsummary` prints,
  `Number of distance tests for qhull`: `discrete=10`, `length_delta=0`, and the doubles within
  2.0e-15.
- **`qhull_paths_t4`: the cube at `T4`** shows the mechanism. At
  `qh_findbest: neighbors of f7, bestdist -1.2`, the candidate finds a neighbour further than
  `bestdist` by a last bit, moves to `f5`, and prints one more trace line. The oracle does not.
  Both then partition the point into `f6`. So a last-bit distance against a tie at `bestdist`
  sends one side's directed search one facet further (the `qh_distplane` class): a count
  changes, not the result. The candidate's tape is 9 words longer, and the words after that line
  are misalignment.
- **The ten `qhull_paths` runs are attributed to that mechanism by analogy, and it was shown on
  one of them.** A one-off run of the lattice with `Qr T4` (not registered) prints, in the
  oracle, `qh_findbest: neighbors of f9, bestdist 1.1e-16` where the candidate prints
  `bestdist 0`; the candidate then visits `f6` twice more (`bestdist -0.76`, `-0.38`), which the
  oracle does not. That is the same last-bit tie, in `qhull_paths`' `Qr` run's input. The other
  nine runs are not traced at level 4.
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
| `NxPhysicsThirdPartyTests.exe` | `07a435eb308d2d03e7343fcc7f06c9a5243d0a108165f18b75ec3f6fe61e80e4` |

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

- **The exact class rests on the `qhull_exact_*` reruns, next to divergent families.** Of the
  87 held-back groups that read `exact`, 86 are exact only through the runs repeated as
  `qhull_exact_output` / `qhull_exact_other` (`qh_setequal` is also exact in `qh_set`). Over all
  qhull groups it is 231 of 240. Every one of the 86 also ran in ceilinged divergent families,
  which `vendored_coverage.csv`'s `differential` column lists per group.
- **The controller's rule for Task 2.**
  - A group's `exact` class counts only if its proof lists, and attributes, every divergent
    family the group also ran in. This applies to arm (i) and to the DIFF-equivalent arm.
  - A group that is itself a named divergence source can be promoted at most as outcome-exact,
    with its bounds stated, and never on the exact arm. That covers `qh_distplane`, and any
    group owning `sum_grouping.csv` sites that a divergent family's attribution implicates.

- No row changes state in this task. Promotion, including the DIFF-equivalent arm, is the next
  task's.
- An `exact` execution of a DIFF group is evidence for its outputs on these inputs. The arm also
  needs its review and its static proof.
- The six OPCODE groups that ran in the candidate-tree clean-up now read `release` where they
  read `END`, because of the new boundary at `nxDriveQhullGap`. Their outcomes do not change.
  The OPCODE trace excerpt is not regenerated.

### Registration and gates

The registrations are in `tools/gate_targets.ps1`:

- 32 lines: the 30 families, then the totals pair (`driven=` / `oracle digest=`);
- the exact families are registered whole, the divergent ones up to `oracle=`;
- no existing line was edited;
- the Phase 4 floor goes from 135 to 167, and `test_gate_targets.py MINIMUM` matches it.

The gate results are in the Timing row below.

## Task 2: promotion under the rules, including the DIFF-equivalent arm

### The rule applied

The DIFF-equivalent arm is recorded in `vendored-correspondence.md`, "Promotion policy for Task 5"
(`0fe9b4e`), before any row uses it. A held-back qhull group's `discovered` rows are promoted when
its review verdict is exactly `equivalent` and it meets one arm:

| Arm | Match class | Condition |
|---|---|---|
| (i) exact | MATCH, SHAPE, REVIEW | Some execution matched in every word (`vendored_coverage.csv` `outcome=exact`). |
| (ii) outcome-exact | MATCH, SHAPE, REVIEW | The Task 5a conditions: (a) a non-vacuous discrete tape that is exact; (b) its float twin has `discrete=0` and `length_delta=0`; (c) the proof states the twin's worst ulp, `beyond` and `beyond_abs`; (d) `kDivergentCeilings` holds those figures; (e) the proof lists and attributes every discrete-mismatch family the group ran in. |
| static-only | MATCH, SHAPE | No x87 code, no `inlining:` note, no other-immediates note. |
| DIFF-equivalent | DIFF | Review `equivalent` citing addresses; execution class `exact`; the proof states the matcher difference and why it is equivalent. |

On top of the arms:

- **The Task 2 rule.** For arms (i) and DIFF-equivalent, the `exact` class counts only because every
  proof lists each divergent family the group also ran in (`vendored_coverage.csv`
  `differential`), with its figures and its attribution. A named divergence source is excluded
  from those two arms: `qh_distplane`, and the groups owning `sum_grouping.csv` sites
  (`qh_normalize2`, `qh_sethyperplane_det`, both promoted by vendored-correspondence Task 5b; the
  qhull float families' attribution implicates all three). Among the held-back groups only
  `qh_distplane` is one, and it is held back anyway, as option-gated.
- **Float-returning groups.** An `inf` distance counts as a discrete outcome unless each such word
  is attributed on its own. Four families have `inf` words. `qhull_rotation_x87` (20) and
  `qhull_paths_t4_x87` (85) belong to executions that are discrete-mismatch as a whole and are
  attributed as a whole (QRn; the T4 misalignment). The other two float twins are attributed word
  by word, below, so no `inf` word is left unexplained.
- **Never promoted:** MISSING/AMBIGUOUS; DIFF groups without an `exact` execution; option-gated
  reviews whose option arm still diverges; unexecuted x87 groups and unexecuted groups with notes;
  groups whose only compared executions have a discrete difference; and the deferred NovodeX rows
  (the 31 unmapped hull rows are not matcher groups, and Tasks 3-4 own them).

**The matcher re-run.** `tools/vendored_match.py` over the gate's candidate DLL (`f9075db4...`) gives
`qhull_match.csv`, `opcode_match.csv` and `vendored_data_map.csv` byte-identical to the committed
ones, so no class changed since Task 1.

### Attributing the inf words one by one

`NXQHGAP_SIGNS=1` (`ffd6a47`, stderr only) lists every float or double whose sign bit differs
between the sides. It is uncapped, and it shows each word's source. The doubles
`nxQhullDirect` pushes itself are labelled `<direct push N>`, in push order: push 0 is
`qh_maxouter`, then per facet `qh_facetarea`, `qh_nearvertex`'s distance and `qh_distnorm`.
Without the variable, stdout and stderr are byte-identical to the Task 1 run. The 15 words:

| Family | Run | Words | Oracle / candidate | Source |
|---|---|---|---|---|
| `qhull_trace_x87` | 19 (set 7, `T2`) | 7584, 7594 | `0` / `-0` | `qh_detsimplex`'s trace `det= 0` against `det=-0`. It is a zero on both sides, and both print `nearzero? 1`. The 2003 CRT prints a negative zero without its sign, so this is either that print artefact or a zero of the other sign from operand order in the inlined `qh_determinant` (`fsub` against `fsubr`, not compared). The decision is the same either way. |
| `qhull_trace_x87` | 19 | 8602, 8610, 8666, 8846, 9090, 9094 | `0` / `-2.775558e-17` | `qh_findhorizon`'s trace `point p%d is coplanar to horizon f%d, dist=`: a distance `qh_distplane` computed for a point coplanar with a horizon facet. Both sides take the same branch (below `qh MINvisible`). |
| `qhull_direct_x87` | 2, 7, 8, 9, 10, 11 | 4457, 4541, 11909, 13527, 15600, 18659, 21081 | up to 1.5e-16, opposite signs | `<direct push 6/9/15/18>`: `qh_distnorm` of a vertex of the facet itself, which is zero in exact arithmetic. |

`qh_distnorm`'s code is the same operation for operation on both sides:

- oracle `0x0005ed6e`..`0x0005ed82`, candidate `0x0007c38c`..`0x0007c3a1`;
- `fld` of the offset, then `fld`/`fmul`/`faddp` for each coordinate, in the same order.

So the sign comes from its inputs, the facet's normal and offset, which the hull build computes
(the `qh_distplane` class). `qh_distnorm` is therefore promoted on arm (i).

### The DIFF groups' field, immediate and x87-class tokens

The matcher classes a DIFF before it filters the field and immediate tokens. So a DIFF row's
`diff_fields` and `diff_imms` are the raw tokens, before:

- the `derived`/`other` pointer-class merge;
- the narrowing rule.

The review lines of the DIFF groups explain the DIFF causes, not these tokens. So, for the proofs,
a provenance run of `vendored_match.py` over the same binaries recorded which instruction adds
each token, and recomputed the residue a REVIEW class would carry. Then:

- **7 of the 31 groups have no residue:** `qh_findbestnew`, `qh_init_B`,
  `qh_printfacet3geom_points`, `qh_printfacet3math`, `qh_printpoints_out`, `qh_printspheres`
  and `qh_memstatistics`.
- **The other 24 have a residue.** Each proof names every residual token's site and its listing
  form:
  - host vptr loads (`mov r,[eax]` after `mov eax,[0x10125080]`);
  - the same flag tested through a high byte, or through a register-held mask;
  - format codes compared through registers (`qh_initflags`: `cmp ecx,edi/ebx/esi`, the codes 1,
    13 and 19);
  - set-element walks;
  - the `PRINTout` loop as a pointer walk against an index (`qh_produce_output`);
  - the tokens of callees one side inlines, which the reviews already name: `qh_gethash`,
    `qh_point_add`, `qh_printextremes_2d`, `qh_allstatA..I`, `qh_appendmergeset` and `memcmp`;
  - `fadd st,st` for a folded `2*x`, or `fmul -2.0` against `fchs`/`fadd`.

  Six tokens are not in the group's own body. They come from the out-of-line oracle callee that
  the candidate inlines, whose body the inlining check merged into the oracle side:
  - **`qh_init_A`'s `--1`** is `qh_initqhull_start`'s `or eax,0xffffffff` (`0x00062416`), which
    materialises -1 before storing it. The candidate's inlined copy stores -1 immediates
    (`0x00076fe1`, `0x00077012`, `0x00077044`, `0x0007704e`).
  - **`qh_test_appendmerge`'s five** are `qh_appendmergeset`'s MRGdegen/MRGmirror arms
    (`0x0006e0cc`, `0x0006e0c0`, `0x0006e145`, `0x0006e18a`). The oracle calls that function at
    `0x0006e0b0` with a constant mergetype: 2 at `0x00070988`, 3 at `0x00070b1d` and 1 at
    `0x00070b8a`. For mergetype 1 to 3 those arms are dead, and the candidate's inlined copies
    drop them.

  **Review corrections (Task 2 review).** Three Task 2 review lines had the inlining the wrong way
  round, or blamed constant propagation:
  - **`qh_test_appendmerge`**: the candidate inlines `qh_appendmergeset`; the oracle does not.
  - **`qh_maxmin`**: the candidate inlines `qh_printpoints`, which the oracle calls at
    `0x00061974`. The oracle-only `" %d"` is that callee's null-string arm (`0x0005fcda`), dead
    for `qh_maxmin`'s constant string.
  - **`qh_printfacetheader`**: the oracle calls `qh_printpoint` four times (`0x0006b7a1`,
    `0x0006b800`, `0x0006b89a`, `0x0006b8f9`), and the candidate inlines two of those calls. So
    the `" p%d: "` literal and the `id != -1` test (candidate `0x00072156`, `0x00072296`) sit in
    the oracle's out-of-line `qh_printpointid` (`0x00067f44`). The same reason gives four
    `qh_pointid` calls against two. It is not constant propagation.

  **The review lines are fixed.** `qhull_review.csv` now carries the residue attribution with its
  addresses in all 24 lines (a `qhull-gap Task 2 residue` entry in `note`, the sites in
  `addresses`), so the review itself cites the addresses the arm requires.

### Inlined copies (Task 2 review)

A cdb breakpoint sees only the candidate's out-of-line copy of a group. So a group the candidate
inlines into a caller missed the families that caller ran in.

`vendored_trace.py coverage` (`599631a`) now credits such a group with each caller's executed
drives, marked `<harness>[via-inline:<caller>]`. The caller is named either:
- by the matcher's `candidate inlines G` note; or
- by an oracle-only call to G in the caller's `diff_calls`.

These entries list families only. `outcome` and `family_best` still describe the out-of-line
body, which is what identity checks. The credit over-approximates, because a caller's drive need
not reach the inlined code on every input. Two tests cover it.

The regenerated `vendored_coverage.csv` gives 91 groups via-inline entries, and changes no
other column.

**The Task 2 rows gaining divergent families.** Every one keeps its arm, and every proof is
rewritten to list and attribute the new families:
- arm (i): `qh_projectpoint` (via `qh_getcentrum`; its own arithmetic is the same operation for
  operation, oracle `0x0005d980`, candidate `0x00080f50`), `qh_rotatepoints` (above),
  `qh_maxouter`, `qh_detjoggle`, `qh_facetarea`, `qh_printfacet2math`, `qh_printfacet3vertex`,
  `qh_printfacetNvertex_simplicial`, `qh_printextremes`, `qh_printextremes_2d`, `qh_printpoint`,
  `qh_printpoint3`, `qh_printpointvect2`, `qh_pointvertex`, `qh_printlists`, `qh_furthestnext`,
  `qh_setdelsorted`, `qh_printstatlevel` and `qh_newstats`;
- arm (ii): `qh_sethyperplane_gauss`, `qh_printvdiagram` and `qh_gethash`;
- DIFF-equivalent: `qh_printfacet2geom`, `qh_printspheres` and `qh_setequal`.

No new family's attribution implicates the group's own code, so nothing is demoted. Two
attribution texts were sharpened:
- `qhull_hull_rotated` now names the rotation matrix (`qh_randommatrix`, and
  `qh_gram_schmidt`'s reciprocal) rather than `qh_rotatepoints`;
- `qhull_random_x87` now also names the reciprocal random-number scaling of the held-back
  option-gated groups.

**Rows promoted before Task 2.** 25 earlier-promoted rows gain families the same way:
- 22 Task 5b qhull rows, `qh_isvertex`, two OPCODE `AABBTreeCollider::Collide` wrappers, and the
  `AABBTreeOfTrianglesBuilder` constructor;
- each `static_proof` gets a `qhull-gap Task 2 review addendum` listing the new families with
  their figures and attribution.

One addendum names an implication. `qh_determinant` is the copy inlined in `qh_detsimplex`, so
the two `det= 0`/`det=-0` trace words are its output. They are a zero on both sides, with the
same `nearzero` decision. Any sign difference would come from operand order, which is not
compared. It stays promoted.

### Results

**Promoted, `discovered` to `reconstructed`: 167 rows, 61,010 bytes** (`5c07d5d`).

| Arm | Groups | Rows | Bytes |
|---|---:|---:|---:|
| (i) exact | 51 | 66 | 16,104 |
| (ii) outcome-exact | 24 | 32 | 8,908 |
| static-only | 0 | 0 | 0 |
| DIFF-equivalent | 31 | 69 | 35,998 |
| **total** | **106** | **167** | **61,010** |

By Task 5b's held-back reason:

| Held back as | Arm | Groups | Rows | Bytes |
|---|---|---:|---:|---:|
| DIFF | DIFF-equivalent | 31 | 69 | 35,998 |
| not executed, x87 | (i) | 33 | 40 | 10,903 |
| not executed, x87 | (ii) | 9 | 15 | 5,252 |
| not executed, notes | (i) | 17 | 25 | 5,109 |
| not executed, notes | (ii) | 8 | 10 | 2,008 |
| QR1-only | (i) | 1 | 1 | 92 |
| QR1-only | (ii) | 7 | 7 | 1,648 |

- **The (i) groups** are the 50 x87/notes/QR1-only groups that Task 1 lists as exact, plus
  `qh_rotatepoints`.
  - Every one of the 51 is exact only through the `qhull_exact_*` reruns.
  - **`qh_rotatepoints` stays on arm (i).**
    - Its out-of-line body ran in `qhull_direct` and the exact rerun.
    - The candidate also inlines it into `qh_rotateinput` (oracle call `0x00061d1d`), so it
      ran in `qhull_rotation`, `qhull_rotation_x87` and `qhull_hull_rotated` too. Its proof now
      lists and attributes those families.
    - It owns no `sum_grouping.csv` site. Its own arithmetic is the same operation for
      operation (oracle `0x0005ff40`, candidate `0x0007e400`). The QRn families differ through
      the matrix it is given, from `qh_gram_schmidt`'s reciprocal, so it is not a divergence
      source.
- **The (ii) groups** are the 24 groups Task 1 lists as outcome-exact outside DIFF and
  option-gated.
  - Each proof cites the first inf-free pair it ran in: `qhull_output_delaunay`, `qhull_options`,
    `qhull_random`, `qhull_merge2` or `qhull_output_dims`.
  - The exception is `qh_printvdiagram`, whose only qualifying pair is `qhull_direct`. Its seven
    inf words are attributed one by one above.
- **The DIFF-equivalent groups** are Task 1's 32 exact DIFF groups less `qh_initqhull_globals`,
  whose review is `equivalent-option-gated`.
  - 30 of the 31 are exact only through the reruns.
  - `qh_setequal` is also exact in `qh_set`.

**Still held back: 71 rows, 22,664 bytes** (49 groups).

| Reason | Groups | Rows | Bytes | Groups named |
|---|---:|---:|---:|---|
| DIFF, outcome-exact only: the arm needs execution class `exact` | 8 | 15 | 4,542 | `qh_detvridge3`, `qh_find_newvertex`, `qh_printvdiagram2`, `qh_printvoronoi`, `qh_scalelast`, `qh_setdelaunay`, `qh_vertexridges`, `qh_voronoi_center` |
| DIFF, discrete only | 1 | 1 | 390 | `qh_errprint` |
| DIFF, not executed | 6 | 9 | 2,189 | `qh_eachvoronoi`, `qh_init_qhull_command`, `qh_initqhull_mem`, `qh_initqhull_start`, `qh_mergecycle`, `qh_printallstatistics` |
| review `equivalent-option-gated` (the option arms still diverge in `qhull_random_x87`) | 9 | 10 | 6,539 | `qh_distplane`, `qh_getangle`, `qh_getdistance`, `qh_initialvertices`, `qh_joggleinput`, `qh_nextfurthest`, `qh_randommatrix`, `qh_tracemerging`, and the DIFF `qh_initqhull_globals` |
| not executed, x87 code | 6 | 10 | 3,083 | `qh_appendmergeset`, `qh_findgooddist`, `qh_furthestout`, `qh_initqhull_buffers`, `qh_matchduplicates`, `qh_maydropneighbor` |
| not executed, inlining or other-immediate note | 17 | 24 | 5,587 | the merge internals the exe inlines, the `qh_mergecycle_*` family, `qh_checkvertex`, `qh_degen_redundant_facet`, `qh_matchvertices`, `qh_printhashtable`, `qh_setprint`, `qh_triangulate_mirror` |
| only compared executions are QRn (discrete) | 2 | 2 | 334 | `qh_gram_schmidt`, `qh_rotateinput` |

**The ledger** (`gates/phase4-closure.json`): 167 deferrals move from `vendored_not_falsified` to
`reconstructed_not_falsified`.

- The counts go from 257 to 90 and from 462 to 629.
- Each moved row's note names this task and its arm.
- The reason prose and the ledger note are updated.
- `validate_inventory` exits 0 (`unexplained=0`).
- 90 vendored rows stay `discovered`: 71 qhull and 19 OPCODE.
- `work_units.json` and the `gap:Controller.cpp..fluids\Fluid.cpp` bundle are regenerated. Only
  the row states change.

**What each promoted row carries.** The Task 5b format:

- `implementation` and `source`: the defining overlay or upstream file.
- `implementation_symbol`: the function's source name.
- `static_proof`, which states:
  - the matcher class, what it compares and the "Not compared" list;
  - the summation-order and register-lifetime caveat, and that the group owns no
    `sum_grouping.csv` site;
  - the review verdict with its addresses;
  - for a DIFF-equivalent group, every `diff_*` token, the review's reason and the residue
    attribution;
  - the execution class and its exact families, or the (ii) conditions (a)-(e) with the figures;
  - every divergent family the group also ran in, with figures and attribution.
- `dynamic_proof` on every traced row. It cites:
  - `vendored-trace-qhull.txt`;
  - the traced exe `07a435eb...` and the DLL `f9075db4...`;
  - the identity result.

**Binaries.** The traced exe is Task 1's `07a435eb...`. The gate now runs `9ab62bd8...`
(`ffd6a47`, and `8a92d81`: the sign listing reads the trailing float and both sides' word
kinds), which differs from it in harness code only. Its identity check against the DLL gives
the same verdict as the traced exe for all 562 groups. (`24a45e0b...` was the exe at `ffd6a47`.)

### What this does not settle

- **Arm (i) and DIFF-equivalent rest on reruns selected by measurement.** 81 of the 82 promoted
  exact groups are exact only in `qhull_exact_output` / `qhull_exact_other`. The proofs attribute
  the divergent families those groups also ran in. They do not make the groups' other executions
  exact.
- **The residue attribution is a provenance listing, not a statement-by-statement re-read.** For
  each DIFF group it names the instruction behind every residual token and its listing form. The
  exact execution, both tapes, is the evidence that those forms compute the same thing on these
  inputs.
- **The ten `qhull_paths` runs are still attributed to the tie mechanism by analogy.** Only the
  cube and the lattice `Qr` run were read at trace level 4.

## Timing

| Task | Start | End | Rows written | Bytes written | Notes |
|---|---|---|---:|---:|---|
| 1 | 2026-09-28T07:01:00 | 2026-09-28T08:38:45 | 0 | 0 | Execution families for the 238 held-back qhull rows. 28 families (14 discrete/float pairs, 434 runs) in NxPhysicsThirdPartyTests through the NovodeX driver sequence: print formats in 2-d/3-d/4-d and Delaunay/Voronoi, trace levels T1-T5, option-gated arms, merge-heavy inputs, QJ/Qr/R, direct calls into 33 out-of-line printers/helpers, and the 138 runs exact on both tapes as qhull_exact_output/_other. Capture: the host object's five output slots to one sink on both sides (tests/PhysicsThirdPartyHost.cpp routes the candidate's hooks; ThirdPartyHost.cpp and the DLL unchanged), each CRT's output to files its own fopen opened (oracle static-CRT fopen 0x000f4251/_fclose 0x000f42b0), text compared as tokens; test exe links legacy_stdio_float_rounding.obj. 14 exact + 14 divergent (ceilinged: the qh_distplane float class, qhull_paths' distance-test counts and a trace-4 search step, qhull_rotation's QRn). qhull groups exact 9 -> 240, not executed 151 -> 47; held-back groups exact/outcome-exact/discrete/none: DIFF 32/8/1/6, x87 33/9/0/6, notes 17/8/0/17, QR1-only 1/7/2/0, option-gated 4/2/1/1. Phase 4 floor 135 -> 165. No ledger change; gates 2, 3, 4, 6, 7 pass, Phase 5 red only on `CANDIDATE-MISSING family=vtables`. |
| 1 review | 2026-09-28T08:45:00 | 2026-09-28T09:13:45 | 0 | 0 | Review fixes: the cube's T4 run split from qhull_paths into qhull_paths_t4/_t4_x87; qhull_paths now 10 aligned runs (discrete=10, length_delta=0, beyond_abs 2.0e-15); kLengthCeilings holds the four paths families to their exact length_delta (0/0/9/2). Caveat recorded: 86 of 87 held-back exact groups (231 of 240) are exact only through the qhull_exact_* reruns and also ran in ceilinged divergent families; controller rule for Task 2 recorded. 3.26e-13 word named; signed-zero print artefact; tie mechanism shown on the lattice Qr run at T4; QG-0 Pg duplicate noted. Re-traced (exe 07a435eb...), no class changes. Phase 4 floor 165 -> 167. Gates 2, 3, 4, 6, 7 pass (with -RepoRoot/-BuildRoot on the worktree), Phase 5 red only on `CANDIDATE-MISSING family=vtables`. |
| 2 | 2026-09-28T09:14:00 | 2026-09-28T09:44:09 | 167 | 61010 | Promotion under the rules. DIFF-equivalent arm recorded in the promotion policy first (0fe9b4e). Matcher re-run byte-identical. Harness gains the stderr-only NXQHGAP_SIGNS listing (ffd6a47): the 15 inf words of qhull_trace_x87 (2 signed-zero print artefacts in qh_detsimplex's trace, 6 qh_findhorizon coplanar distances from qh_distplane) and qhull_direct_x87 (7 qh_distnorm returns for a facet's own vertex; qh_distnorm's code identical op for op) attributed one by one. DIFF groups' raw field/immediate/x87 tokens reduced to the REVIEW residue by a provenance run of the matcher and each residual token's site named. Promoted 106 groups / 167 rows / 61,010 B: (i) 51/66/16,104, (ii) 24/32/8,908, static 0, DIFF-equivalent 31/69/35,998; every proof lists and attributes each divergent family the group ran in; no named divergence source promoted. Held back 49 groups / 71 rows / 22,664 B: DIFF outcome-exact only 8/15, DIFF discrete 1/1, DIFF not executed 6/9, option-gated 9/10, unexecuted x87 6/10, unexecuted notes 17/24, QRn-only 2/2. Ledger vendored_not_falsified 257 -> 90, reconstructed_not_falsified 462 -> 629; validate_inventory exits 0; work_units.json and the Controller..Fluid bundle regenerated. 753 tool tests OK, verify_vendored_sources pass; gates 2, 3 (103), 4 (167 of 167, exe 24a45e0b...), 6 (403), 7 (276) pass, Phase 5 red only on `CANDIDATE-MISSING family=vtables`. |
| 2 review | 2026-09-28T09:50:00 | 2026-09-28T10:11:22 | 0 | 0 | Review fixes: vendored_trace.py credits a group the candidate inlines with its callers' executed drives (via-inline, from the matcher's inlining note or an oracle-only call; families only, outcome unchanged; 2 tests, 755); vendored_coverage.csv regenerated (91 groups gain entries). All 167 Task 2 proofs regenerated on it: qh_rotatepoints gains the QRn families via qh_rotateinput, qh_projectpoint the hull/paths/rotation families via qh_getcentrum (both op-identical), 25 Task 2 groups in all gain families; no divergence source, no demotion, arms unchanged (i 51/66, ii 24/32, DIFF-equivalent 31/69; 167 rows / 61,010 B). 25 earlier-promoted rows get an addendum with their new families. qhull_review.csv: qh_test_appendmerge, qh_printfacetheader, qh_maxmin reasons corrected (inlining direction; not constant propagation), qh_init_A's -1 named, residue with addresses on all 24 residue lines. NXQHGAP_SIGNS reads the trailing float and both word kinds (same 114 lines). Validator 0, 755 tool tests OK, verify_vendored_sources pass; gates 2, 3 (103), 4 (167 of 167, exe 9ab62bd8...), 6 (403), 7 (276) pass. |
| 3 | 2026-09-28T10:12:00 | 2026-09-28T10:33:52 | 0 | 0 | Contract for the NovodeX convex-cooking layer (`units/convex-cooking-contract.md`). Public entry: `NxPhysicsSDK::createTriangleMesh` (NpPhysicsSDK vtable 0x00105860 slot 8 = 000242) with `NX_MF_CONVEX|NX_MF_COMPUTE_CONVEX`; 000478 PhysicsSDK::createTriangleMesh, 002251 TriangleMesh ctor, 002260 loadFromDesc (slot 2 of 0x00108608), 002233 the hull call. The 37 hull-span rows are a HullLibrary (HullDesc/HullResult, CreateConvexHull 003279, ReleaseResult 003255) over qhull "o": JOHNRAT-tracked host object (vtable 0x00113614, 9 slots; slot +0x10 formats then errexit(1)), cleanupVertices, box fallback + one setjmp retry, QHULL_FAIL dump on (flags 0xb7 bit 7), and band B = Wu's colour quantizer reducing to 256 vertices. HullDesc 0xb7/n/points/stride/1e-5f/256/0.8f (0.8f unused on NovodeX's path). No control-word change on the chain. Task 4 write set 40 rows / 12,493 B (34 discovered / 11,968 B) in 5 pieces (4c, 4d x87-heavy); public chain and post-hull TriangleMesh/ConvexHull closure (~63 KB) deferred; public-API differential needs a controller decision. Bundles generated for NpPhysicsSDK, PhysicsSDK, TriangleMesh, InternalTriangleMesh, NpTriangleMesh, NpTriangleMesh..TriangleMesh, ConvexHull, ConvexHull..IceAdjacencies (Controller..Fluid unchanged). No product code change. |
| 4a+4b | 2026-09-28T10:38:18 | 2026-09-28T11:01:58 | 22 | 4024 | Physics/src/QhullHost.cpp (new, /arch:IA32, /EHs-c-) + include/QhullHost.h: band A of the hull library in address order under stable-ID lines. 4a (c1e29b1): the host object -- releaseArrays, rawAlloc/rawFree, ctor, offBegin, point3, print (formats, then errexit(1)), size, errexit (release, longjmp to 0x00125040), facet, the JOHNRAT tracked allocator over 4,096 slots (CRT free on a full table kept), dtor; globals 0x00125040/0x00125080; qhull's nine hooks moved from ThirdPartyHost.cpp shims into QhullHost.cpp, forwarding to the published object. NxPhysicsThirdPartyTests compiles QhullHost.cpp with the hooks renamed and routes per call (product host if published, else Task 1 sink, else the pre-4a stand-in), so no registered qhull line moved. 14 rows / 1,334 B. 4b (0cba68f): runQhull, writeOkObj, writeFailObj, boxFallback, ReleaseResult, buildResult, CreateConvexHull (setjmp retry, rescale, dead +4 arm, OK/FAIL dumps, counter 0x00125084); cleanupVertices is a marked placeholder until 4c. 8 rows / 2,690 B. Local check vs the pinned DLL (not committed): identical HullResults and OK files where the oracle's clean-up is transparent, same FAIL-dump/boxFallback/retry on a diagonal plane. Ledger not_reconstructed_in_phase 306 -> 290, reconstructed_not_falsified 629 -> 645; validator 0; 755 tool tests OK; gates 2, 3 (103), 4 (167 of 167, candidate mismatches 0), 6 (403), 7 (276) pass, Phase 5 red only on `CANDIDATE-MISSING family=vtables`. |
