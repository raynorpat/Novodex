# Novodex Physics oracle census

Phase 1 evidence for reconstructing the UE3-shipped Win32 Release `NxPhysics.dll`.
This directory holds the census schemas, the pinned analysis toolchain, and the
tools that keep both honest. Later phases consume this evidence; they do not
re-derive it.

## A warning about dependency maps, for every phase

**A closure over direct `call rel32` edges is not a dependency map for code that
dispatches virtually, and this image is full of vtables.**

Phase 3 built such a closure to order its remaining work, published the numbers,
and then found by reading one of the functions that four of the eight entries it
had costed reach a further row through `call dword ptr [reg + 0x14]` — slot 5 of
a shape's vtable — which no direct-edge walk can see. The published costs were
lower bounds, and they were lowest for exactly the entries that had been
recommended on the strength of them.

Phases 4 through 7 will each build one of these graphs. Two things follow:

1. **Treat a closure figure as a lower bound** unless the functions in it have
   been checked for indirect calls. `capstone` records every instruction with
   `indirect: true`; scanning an entry's byte range for those is cheap and is
   the difference between a cost estimate and a guess.
2. **Resolving a virtual target means resolving it per receiver type**, from the
   vtable store in each constructor, not once. Phase 3's slot-5 table is in
   `evidence/phase3-narrow-phase.md` and is reusable: five shape types, each
   vtable address, each resolved target with its row, phase and size. One of the
   five resolves into a *different phase*, which is the kind of thing that turns
   a planned entry into a stop.

3. **Fold every continuation row into its entry before walking.** A function whose body the census splits at internal alignment padding appears as an entry row plus continuation rows, and a continuation is not a call target. Phase 3 costed `box/box` twice and missed 1,498 bytes both times, because all four calls to `phys_fn_001739` are inside `phys_fn_001743` -- the continuation of `phys_fn_001741` -- and every one of them is a plain `call rel32`. Nothing exotic hid them; the walk stopped at the pad.

The same shape of error had already occurred once in Phase 3 with a reference
scan that stopped at two of three call sites. In both cases the number looked
authoritative and reading the function corrected it.

## A warning about generators, for every phase

**A generator may not do arithmetic on values it also allows to be non-finite.**

Every differential in this program pins a digest over the oracle's own answers,
and the whole weight of that pin is the claim that nothing on the reconstruction
side can move it. Phase 3 found a block where that claim was false in a way no
review had caught: the generator aimed one shape at another by multiplying a
plane normal it also allowed to be a NaN one draw in four, and **SSE propagates
the payload of one operand or the other depending on which the compiler put
first**. So the *generated input* differed between two builds of the same
harness, the oracle's own digest moved with it, and every branch counter, width
and word count stayed identical because both sides saw the same changed shape.
A pinned oracle digest that moves when the reconstruction is recompiled is not
evidence of anything.

Four hypotheses were killed by measurement before the cause was named -- an x87
stack probe, a stack fill from one binary across three runs, two object
addresses from one binary, and finally deleting the candidate call altogether,
after which the oracle *alone* still moved. That last one is what named it.

Three things follow for Phases 4 through 7:

1. **Aim only through finite values.** Keep the non-finite coverage by feeding
   raw bit patterns straight into the shape, never through a multiply or a
   subtract used to place one thing relative to another. Make the rule hold by
   construction rather than by care.
2. **A digest is only oracle-side if every input to the fold is.** The same
   phase found a differential that folded both sides over the *shorter* of the
   two stream lengths, so a reconstruction that emitted fewer words truncated
   the oracle's digest. Write the fold so the candidate's data is not in scope
   where the oracle's digest is computed.
3. **Test reproducibility, not just determinism.** Running the same binary twice
   proves nothing here -- Windows assigns an image base per boot, and a
   NaN-payload difference is deterministic within a build. Rebuild with a
   deliberate perturbation and check the pinned digest again.

Both traps look like sound instrument design, which is why they belong next to
the dependency-map warning above rather than buried in a task report.

## A REQUIRED STEP for mutation measurements, for every phase

**Touch every source after extracting a `git archive` copy, before building.**
Not a tip; the measurement is wrong without it.

`git archive` writes every file with the *commit's* mtime, which is older than
the object files a previous run in that directory left behind. MSBuild compares
timestamps, decides the translation unit is up to date, and links the **previous
variant's object**. The run then completes, prints a full transcript and reports
numbers that belong to the mutant before it — a measuring instrument returning a
stale reading as a fresh one, with nothing in the output saying so.

This was found in Phase 3 the only way it can be: a degradation run that changed
the *harness* while leaving the reconstruction file restored-and-stale printed
the previous mutation's digest and mismatch count, and it was noticed only
because that digest was recognisable. A run that had happened to follow a
zero-moving variant would have printed a clean zero and been believed.

```bash
git archive HEAD | tar -x -C "$DIR"
cd "$DIR" && find <source dirs> -name '*.cpp' -o -name '*.h' | xargs touch
```

Three things follow, and this programme has taken a great many mutation
measurements from archive copies:

1. **A mutant that rewrites the file it mutates is accidentally safe** — the
   rewrite gives that one file a current mtime — and every other file in the
   tree is not. So the hazard is invisible exactly until a task mutates two
   files, or mutates the harness instead of the reconstruction.
2. **Re-measure with a forced rebuild before reporting a zero.** A non-zero
   delta that carries over from a neighbour is usually noticed; a zero is not.
   Phase 3 re-ran all eight of its `box/box` mutations and its four zero-moving
   carrier attempts this way and every one reproduced, but that is a measurement
   and not something to assume for earlier tasks.
3. **Run the un-mutated control in the same directory, before and after the
   set.** Two controls bracket the whole run rather than certifying its first
   build alone.

## A warning about green transcripts, for every phase

**A zero mismatch count is a statement about the inputs that were driven, and
about nothing else. Read the transcription back against the disassembly after it
is green.**

Phase 3 closed the sphere/box contact geometry with every digest matching over
3,480,000 checks under both control words, every coverage line registered, and
every generator degradation failing the gate as it should. Then a read-back of
the row against the instruction stream found a defect the whole apparatus was
blind to: the oracle takes a magnitude with `fabs`, which clears the sign bit,
and the reconstruction spelled it `x < 0 ? -x : x`, which leaves `-0.0`
negative. The two differ only when a box extent is *also* `-0.0` -- a value the
raw-bit half of the generator can produce but had never paired with the other one
-- and the sign of the resulting zero goes straight into the contact stream.

Three things follow, and none of them is "write a better generator":

1. **A differential is a filter with a shape.** It answers "do these two agree on
   what I asked them" and never "did I ask the right things". No iteration count
   fixes a corner the generator's *joint* distribution does not reach; this one
   needed two independent `-0.0`s in the same call.
2. **Re-reading catches a class no differential does**, cheaply, and it is the
   only instrument that does. Budget it as a step rather than as diligence: once
   the digests are zero, walk the disassembly beside the reconstruction looking
   for the places where C++ and the machine disagree about a *representation* --
   signed zero, NaN payloads, the rounding of a store -- rather than about a
   value.
3. **A fix that changes no digest is evidence, not an anticlimax.** Every digest
   and every counter was byte-identical after the correction, which is what says
   it was a correctness change on an input nothing had reached rather than a
   rewrite that happened to still pass.

The temptation this exists against is stopping at green. Every phase will feel
it, because green is what the gate is for.

## A design signal, not a post-hoc story: `double` liveness across a branch

Four Phase 3 rows were closed on algorithmic agreement under a control word they
do not execute under, with the real-context divergence pinned -- 662, 242, 43 and
24 words. The retry below took the 24 to **zero** and showed the 242 belongs to a
callee, so the Phase 3 close carries **two**: `phys_fn_001690` at 662 and
`phys_fn_001775` at 43. Every one traces to the same cause: **MSVC spills a `double` to
an 8-byte slot, which truncates a 64-bit significand to 53, and no C++ construct
says where the spills go.** That began as an explanation. `plane/box` turned it
into a rule: it keeps no `double` live across a branch, so a spill has nothing to
truncate, and it is bit-exact under both words.

Phase 3 then used it as a **forecast** rather than an account, and it held. Both
sphere entries of matrix A were read for a `double` crossing a branch *before*
they were transcribed, both had none, and both came out bit-exact under both
words over 2,061,456 + 2,453,136 + 3,480,000 checks.

So for every row still to be written, in this phase and the four after it:

- **Read the row for a value that must survive a branch before transcribing it.**
  If one must, expect the divergence class, pin the count, and say which control
  word the row actually executes under -- rather than spending measurements
  fighting a spill no source language controls.
- **The cause is the spill, and a branch is only its commonest reason.** Phase 3 built an instrument that counts x87 depth at each conditional branch, validated it against nine rows and used it correctly -- and then closed `phys_fn_001739`, which has no branch above depth 0 in the region that mattered and still differed on 88 words under `0x0f7f` as first written. MSVC had spilled five `double`s that the oracle keeps in `st(1)`..`st(6)`, purely from register pressure in straight-line code. Read a row for **what has to be live at once**, not only for what has to survive a branch.
- **Find the carrier, not the spill.** `NxBuildSmoothNormals` had six spilled `double`s and only one was observable: every other one is narrowed to `NxReal` before it reaches an output. Ask which spilled value reaches the answer *while still wide*, rather than counting spills. Its 24 went to **0** that way, which upgraded the row from a pinned divergence to a real bit-exact closure.
- **Find it by READING THE LISTING, and do that first.** `dumpbin /disasm` the object and scan for an 8-byte store whose previous instruction is arithmetic — `fst`/`fstp qword ptr` after an `fadd`, `fsub`, `fmul`, `fdiv` or `fsqrt`. Every spill MSVC chose is on that list and nothing else is, so it names the whole candidate set in one command. `phys_fn_001741` is the worked example and it is the strongest case for the rule: **the depth-at-branch heuristic mispredicted it.** The instrument named the clip quotient, a wide `fdivp` result crossing two branches — and moving that into a `noinline` helper changed **not one bit**, as did the delta projection, all nine dot products and the stage 4 depth. Four builds, four zeroes. The scan then found the real carrier immediately: four values held across the *straight-line* corner construction, each used by four corners, each materialised as `fadd qword ptr [X]; fstp qword ptr [X]`. Removing those four took the row from 276 differing words to **0**. Read the listing, then spend the build. **Count the stores that follow an arithmetic instruction, not the qword stores.** `phys_fn_001745` is the negative control: it is bit-exact on the first build, and the same scan reports 26 qword stores in it, every one widening a value that is already exactly a `float`, and **zero** after an arithmetic instruction. The first number says nothing; the second one is the measurement.
- **The technique needs an empty live set at the call.** MSVC spills every live x87 value before a call, so moving a hot expression into a `__declspec(noinline)` helper only helps where nothing else is live. Tried on `phys_fn_001690`'s constant term -- which is surrounded by five other live values -- it took 662 words to **29,529**. Check the precondition before spending a build.
- **Check whose row a divergence is.** `phys_fn_001010`'s pinned 242 is not `phys_fn_001010`'s: that function compiles with zero spill slots, and all 242 come from `NxRayCapsuleIntersect`, a Task 2 export closed by its own differential and carrying 37 of them. A pinned count can belong to a callee.
- **A divergence in this class is not automatically something to pin.** The same row went 88, 20,060, 68 and then **0** across four spellings with identical arithmetic; the fourth put the hot expression in a `__declspec(noinline)` helper so the live set fit. All four pinned rows were then retried: one went to zero, one was a callee's, one is immovable by this technique with the boundary named, and one is unchanged. "We searched and the search has a named boundary" is the claim, not "no construct controls it".
- **A value whose every possible setting is exactly a `float` is not in the
  class**, even when it lives in `st(0)` across three branches, because a spill
  cannot truncate it. `phys_fn_001917`'s clamped z is the worked example.

## A trend, not four incidents: the simulate window keeps growing

Task 2 closed 27 exports under the CRT default control word, correctly, because
nothing then known reached them from the simulation step. Phase 3 has since
found **five** of them inside it -- `NxBoxBoxIntersect`, `NxBuildSmoothNormals`,
`NxRayTriIntersect`, `NxRaySphereIntersect` and `NxRayCapsuleIntersect` -- and
the last two needed two levels of indirection to see: a function pointer into
the dispatch matrix and then a vtable slot. Every one was invisible when it was
measured.

That is over half of the exports Phase 3 has had reason to drive under both
words. **Phase 4 should expect the same for whatever it closes**: a row's
evidence is relative to the reachability graph known at the time, and recovering
any indirect-dispatch table grows it. Record which control word each closed row
was measured under.

## Pins

| Pin | Value |
| --- | --- |
| Oracle | `Binaries/NxPhysics.dll`, SHA-256 `4b7db3e1…79602c`, 1253376 bytes, 41 named exports |
| Link oracle | `Development/External/Novodex/Physics/lib/win32/Release/NxPhysics.lib`, SHA-256 `a99cddd2…d83c47` |
| Public headers | `Development/External/Novodex/` at SDK identity `2.1.2.6000` |

Paths in `inventory.json` are POSIX and relative to the repository root so the
worktree can move. `public_header_hashes.json` pins the recursive Physics
`include` tree (80 files). Shared Foundation types stay pinned by the sibling
project's `../novodex-foundation/dumps/immutable_public_hashes.json`, which this
project references and never modifies.

## Files

- `inventory.json` — the census. Top-level keys are exactly `schema_version`,
  `pins`, `sections`, `functions`, `data_objects`, `exports`, `imports`,
  `coverage`, `phases`, `gates`.
- `labels.json` — the label ledger: every non-`stable-id` label with its
  evidence reference and reason.
- `analysis_toolchain.json` — the measured Ghidra/Capstone pin.
- `public_header_hashes.json` — the recursive public-header manifest.
- `oracle/pe.json` — the PE oracle, read from the file format alone.
- `oracle/ghidra/manifest.json` — the Ghidra oracle: functions, instruction
  ranges, symbols, strings, references, vtables, RTTI and data.
- `oracle/capstone/manifest.json` — the Capstone oracle: the recursive-descent
  disassembly, its chunks and entries, and an account of every executable byte
  as an instruction byte, an undecodable byte or an unclassified coverage gap.
- `oracle/coverage.json` — the code census: what each ownership rule claimed, the
  the recovered targets, the rows per phase, and how much the Capstone descent
  would lose if the Ghidra oracle were dropped.
- `oracle/data-coverage.json` — the data census: referenced bytes by owner class,
  the recovered dispatch structures, and every reference that leaves the image.
- `oracle/dependencies.dot` — the call graph the phase assignment falls back to,
  with each node's phase.
- `ghidra/physics_type_inputs.json` — everything the ABI shim is derived from.
- `ghidra/physics_x86_msvc.h` — the generated C ABI shim. Generated, never edited.
- `ghidra/ExportPhysicsAnalysis.java`, `ghidra/ApplyPhysicsTypes.java` — the
  headless exporter and the typed second pass.

## The Ghidra oracle

`generate_ghidra_types.py` derives `physics_x86_msvc.h` from the pinned public
headers alone. It hand-authors no type bodies: enums, layout structs, base
embeddings and exported prototypes all come from header text whose bytes both
manifests pin, and it re-verifies those hashes before parsing. Its rules `R0`-`R8`
are recorded in `physics_type_inputs.json` alongside the Win32/MSVC preprocessor
definitions, the excluded headers and the forced declarations, each with a reason.

**Anything the derived subset cannot express is rejected by file and line.**
Nothing is skipped in silence, because a dropped member moves every member after
it. The three escape hatches are all explicit and reviewable, and a stale entry in
any of them is itself an error:

- `excluded_headers` names a header that is not part of a Win32 configuration.
- `excluded_types` names a single type withheld because its layout is not
  derivable under the target ABI. It is not a way to make a hard error go away
  quietly. Each entry carries a reason; the generator rejects an entry no header
  declares; the withheld type leaves a `/* excluded type: … */` marker in the
  generated header where it would have appeared, which `ApplyPhysicsTypes.java`
  reads back out of the shim it applied and records in the manifest's
  `type_coverage.excluded_types`; and the generator **hard-errors** if a withheld
  type is ever reachable from an export signature, a vtable-bearing class, or a
  member of any other emitted type. Unreachability is checked, not assumed.
- `forced_declarations` supplies a declaration for a class template
  *instantiation*, which the generator does not instantiate. A forced declaration
  may never supply a layout for a type the headers define; the generator rejects
  one that tries. Its `reason` is emitted as a comment above the declaration, so
  the caveat travels with the artifact instead of staying in the inputs file.

Every emitted enum is widened to the 4-byte underlying type MSVC gives enums, by
appending a generator-added `…__nx_force_32bit` enumerator to any enum whose own
values do not already reach 32 bits. Without it a C parser sizes an enum from its
value range, and a one-byte enum moves every member after it. `ApplyPhysicsTypes`
reads the widths of the shim's own enums back out of Ghidra and fails the run if
any is not 4 bytes.

A struct gains a leading `void **__vftable` only when it introduces virtual
members and does not already inherit a vftable from its base chain. A class that
introduces virtual members over a base with no vftable is rejected, because MSVC
places the pointer *before* the base subobject and reorders everything after it.
A `virtual` member of a nested type belongs to that nested type, not to its
enclosing class — getting that wrong gave `NxProfiler` a vftable pointer it does
not have.

`normalize_ghidra.py` turns the raw headless stream into deterministic evidence.
It converts addresses to RVAs, keeps only the metadata that survives a fresh
analysis, canonicalizes type strings, orders every table by address, and refuses a
record carrying a field the schema does not define. Targets outside the image —
an import in Ghidra's `EXTERNAL` space, a stack slot in the `stack` space — keep a
space-local offset and a null RVA rather than being dropped or given an address
the image does not have.

Decompiler output is stripped of the host line separator, so the manifest hash is
independent of the operating system by construction rather than by luck.

That property has to hold for **every** hashed artifact, not just this manifest.
Each tool writes with `newline="
"`, and `docs/reconstruction/.gitattributes`
declares `text eol=lf` for the JSON, `.h` and `.dot` evidence so a checkout
cannot reintroduce CRLF. Both matter: without the writers a regeneration on
Windows produces CRLF, and without the attributes a clone with
`core.autocrlf=true` converts LF blobs back on the way out. A pinned hash taken
over CRLF bytes verifies only on the machine that produced it.

The generated shim names the manifests it was derived from and **does not quote
their hashes**. Quoting them coupled the shim to the pin: re-pinning rewrote the
shim, whose own hash `type_coverage.header_sha256` records, so a one-line pin
change cascaded through a 27 MB manifest and every reference into it. The hashes
live in `physics_type_inputs.json`, which is where the generator checks them.

Both extractors re-check the pin, and it is **compared rather than echoed**:

- `ExportPhysicsAnalysis.java` fails the run if any of the six pinned analyzer
  names is unregistered or holds a value other than the pinned one. Because that
  table is restated in Java, a test parses it back out of the source and asserts
  it equals `verify_toolchain.DEFAULT_ANALYSIS_OPTIONS`, so the two cannot drift.
- `normalize_ghidra.py` loads `analysis_toolchain.json` from beside the tools and
  rejects a stream whose Ghidra version, language, compiler spec or analyzer
  values differ from it, or whose canonical analysis-options hash does not equal
  the one the pin records.

## Inventory schema

Every function and data row carries one stable ID, one phase owner, and one
state. Stable IDs (`phys_fn_%06d`, `phys_data_%06d`) are assigned from sorted
entry RVAs and persist through later label changes.

Function rows use these fields, and no others:

```json
{
  "id": "phys_fn_000001",
  "rva": "0x00000000",
  "size": 1,
  "kind": "code",
  "label": "phys_fn_000001",
  "label_confidence": "stable-id",
  "section": ".text",
  "phase": 1,
  "state": "discovered",
  "source": null,
  "ghidra_ref": "ghidra/functions/phys_fn_000001.json",
  "capstone_ref": "capstone/phys_fn_000001.json",
  "static_proof": null,
  "dynamic_proof": null,
  "notes": ""
}
```

Data rows record `id`, `rva`, `size`, `type`, `owner`, `references`, `section`,
`phase`, `state`, `source`, `label`, `label_confidence`, `structural_proof`,
and `notes`.

- **States** are exactly `discovered`, `typed`, `decompiled`, `reconstructed`,
  `statically_reviewed`, `dynamically_gated`, `closed`.
- **Kinds** are `code` and `compiler_artifact`. Compiler and runtime artifacts
  retain byte ownership but must record a classification proof in
  `static_proof` and must not claim product `source`.
- **Label confidence** is `stable-id` (machine-assigned) or `semantic`. Every
  `semantic` label needs a `labels.json` entry carrying evidence and a reason,
  and the ledger entry's `confidence` must match the row's `label_confidence`.
- Every function row records both `ghidra_ref` and `capstone_ref`, so each owned
  code byte is backed by two independent extractors.
- `gates` are `pending`, `pass`, or `fail`. `phase_1_oracle_census` cannot pass
  ahead of the census itself.

### Coverage rules

`coverage.census.status` is `pending`, `pass`, or `fail`. These hold at every
status:

- `explained_executable_bytes + unexplained_executable_bytes == executable_bytes`,
  so a `pass` cannot be manufactured by zeroing the unexplained counter alone.
- No two rows overlap, **across `functions` and `data_objects` together**. They
  partition one address space, and the phases that follow hand-edit rows and
  re-run this validator rather than the reconciliation that built them, so the
  invariant the census exists to establish is checked where those phases will
  meet it.
- `unexplained_referenced_data_bytes <= referenced_data_bytes`.
- A populated `exports` table must contain exactly `pins.oracle.named_exports`
  entries. An empty table is allowed only while the census is not passing.

A `pass` additionally requires:

- zero unexplained executable bytes, unresolved executable targets, unexplained
  referenced data bytes, overlaps, and duplicate ownership;
- `executable_bytes > 0` and `referenced_data_bytes > 0`;
- an `exports` table of exactly `pins.oracle.named_exports` entries.

The last two exist because the zero-counter rules alone are satisfied by a
document that enumerated nothing. A passing census must show it actually
censused the oracle.

## Byte ownership

`reconcile_analysis.py` paints the executable extent one owner deep, in a fixed
precedence, and reports what each rule claimed in `oracle/coverage.json`. Nothing
it cannot reach is handed to a neighbour: an unpainted byte fails the census.

| Rule | Evidence |
| --- | --- |
| Capstone instruction bytes | a chunk the recursive descent decoded |
| Walked switch tables | a decoded `jmp` names the table and the PE oracle relocates every slot |
| Ghidra-only instruction bytes | Ghidra decoded bytes the descent refused |
| Derived switch tables | two or more consecutive relocated slots the corpus published as unresolved |
| Ghidra data in `.text` | Ghidra typed the bytes as data |
| ASCII blobs | a printable run inside the extent |
| Alignment padding | int3 and the canonical MSVC NOP encodings, nothing else |
| Alignment jumps | a `jmp` whose target is the first byte after the run it skips |
| Unreferenced code | the sweep decodes the run and nothing in any oracle reaches it |

A row breaks at every function entry and at every address a rule recorded
evidence for, so no row carries a proof that holds for only part of it. Rows are
maximal contiguous extents, which is why a function whose Ghidra body is split by
a jump table appears as an entry row plus continuation rows.

A lone unresolved relocation slot is **not** a derived table. It is as likely to
be the imm32 of an instruction neither oracle decoded, and claiming it would
carve a data object out of a live instruction. Such sites are published in
`coverage.json` under `relocation_sites_too_lone_to_be_a_table` and their bytes
stay with the run around them.

The data census runs the same way over the non-executable image. Its owner
classes are `relocation_metadata`, `export_directory`, `resource`,
`import_address_table`, `dispatch_table`, `pointer_slot`, `string`,
`ghidra_data` and `code_addressed_global`, highest precedence first, and only an
object some reference names is significant enough to enter the inventory.

`code_addressed_global` is a catch-all: a reference proves an object starts at
that address, and the next anchor in the same section is all that bounds it. Its
coverage is reported as `bounded_referenced_bytes`, apart from the
`explained_referenced_bytes` of the seven classes that prove what they own.
**Because the catch-all claims whatever nothing else took,
`unexplained_referenced_data_bytes` cannot rise above zero on this image**; read
the zero as a statement about the other seven classes, not as a certificate over
the bounded bytes.

**The three oracles are independent, and the census measures how much.**
`seed_sources.ghidra_function: 2527` reads as heavy dependence on Ghidra and is
not: drop every entry only Ghidra names and walk the corpus's own recorded edges
from what is left, and **5 of 22,081 entries** stay unreachable. All 2,527 Ghidra
functions are Capstone entries, and the two independently produced
instruction-byte totals - 1,027,076 and 1,029,064 - differ by 0.19%.
`oracle_independence` in `oracle/coverage.json` publishes the measurement.

**The image has no RTTI and no vtable record.** It was built `/GR-`, so
`vtables` and `rtti` are empty in the Ghidra oracle and stay empty here. The
dispatch a C++ image still needs survives as runs of consecutive relocated slots
that every one name a function entry; those are recovered from the PE pointer
table instead and recorded as `dispatch_table` data objects.

## Phase ownership

Phases come from the layout the linker produced, and fall back to the call graph
only where the layout says nothing. The image names 57 translation units through
`NX_ASSERT` `__FILE__` strings, and the 459 entries carrying them occupy 57
address spans that neither overlap nor interleave, because each object's code was
emitted contiguously. A span is therefore evidence about every entry inside it,
and `check_translation_unit_spans` stops the run if the spans ever cease to be
disjoint.

Every entry is placed by the first of these rules that reaches it, and records
which one that was:

- `runtime_artifact` — Ghidra's Function ID names it, or nothing in either oracle
  reaches it, or every caller it has is itself runtime code. That last, transitive
  route accounts for 55 of the 351 entries this rule places.
- `runtime_tail` — above `0x000e9100`, the last entry naming a translation unit.
  No entry above it carries a product source at all, and 318 of the 319 Function
  ID-named runtime functions live there: that region is the statically linked
  runtime, not unplaced product code.
- `slot_ruling` — named by `shape_slot_ruling.json`, the shape-class vtable
  ruling: a slot the collision pipeline dispatches through is Phase 3's, every
  other slot of the shape classes is Phase 5's, and the file says which rows
  follow which slot. The verdicts are not taken from the file: the collision
  pipeline is walked from the dispatch matrices over the rows Phases 3 and 4
  own (`collision_pipeline`, which the generator and the validator both run),
  and a table whose verdicts disagree with what it dispatches stops both.
  It outranks a span because the shape classes' own units are split by slot
  rather than owned whole, and it seeds the caller layer. Unlike the
  propagation rules it is recomputed: `validate_inventory.py` checks the file
  against the oracle and every ruled row against the file.
- `translation_unit` — inside the address span of a named translation unit.
- `enclosed_by_one_phase` — between two spans one phase owns, so the unnamed
  translation units between them are bracketed by that phase.
- `callers` — every caller already placed agrees on one phase, at the moment the
  entry is placed. Two later rules can move a caller afterwards, so this does not
  hold of the finished artifact; see the caveat below.
- `layout_adjacency` — the nearer span by address. The weakest rule here, named
  apart so its weakness stays visible in the report.

Two rules are then applied over the result rather than in that sequence, and both
override what the layers decided:

- `shared_by_callers` — no translation unit of its own and reached from callers
  that several phases own, which is what shared runtime means. It runs after all
  seven layers and **overrides `callers` and `layout_adjacency`**, neither of which
  is translation-unit evidence. It does **not** override `translation_unit`,
  `enclosed_by_one_phase`, `slot_ruling`, `runtime_tail` or `runtime_artifact`: an entry a span
  names, or that two spans of one phase bracket, is physically inside that
  translation unit, and the phase that owns the unit reconstructs it along with
  the rest of it. Being called from several subsystems is ordinary C++, not
  evidence of a separate owner — the bracket answers "which unit is this in",
  the caller spread only answers "who uses it", and only the first is an
  ownership claim. Of the 79 entries it places, 41 had adjacency and 38 callers.
  It reads the phases the layers settled on, not the phases it is itself
  assigning, so which entry it examines first cannot change the answer.
- `export_pin` — the umbrella plan locks this export's owner. Applied last and to
  the export alone, because a pin is a plan decision rather than evidence about
  the code, and propagating it would launder it into the neighbours.

Two more are applied to the finished function rows alone, like the export pin,
and never reach a data object:

- The third-party layer. A row the correspondence maps in
  `evidence/phase4-third-party-map/` grade `mapped` or `probable` is compiled from
  qhull or OPCODE, so it is Phase 4's; where no rule above already put it there it
  is placed with `translation_unit`, naming its upstream unit. This re-derives 119
  of the 191 rows P4 Task 1c moved to Phase 4.
- `phase_pins.json`. The other 72 are rows this evidence cannot derive, each pinned
  to the phase and provenance the census carries, with the reason and the commit
  that decided it. The generator records what each pinned row would have been
  without its pin.

`validate_inventory.py` regenerates the committed census with the ruling, the
maps and the pins, and requires every function row and data object to carry the
generator's phase and provenance; a pin the generator already agrees with has to
be struck, so the list stays exact.

**The caveat on `callers`.** Because `shared_by_callers` runs afterwards, an
entry can hold a phase it inherited from a caller that the override then moved to
Phase 2. The inherited phase is the one the evidence gave and it stands, but the
provenance then names a caller that no longer carries it. In the committed
artifact this affects **16 entries**, of which **3** have no surviving caller on
their own phase at all: `phys_fn_002432`, `phys_fn_002493` and `phys_fn_002532`.
Audit those three from the layout around them rather than from the call that
placed them.

Phase names are the phase plan titles. Phase 8 is the full semantic audit, and
the 3,617 function rows it owns are the compiler and runtime artifacts that audit
classifies; owning them is part of Phase 8, not the definition of it.

A `__FILE__` reference is resolved through the census's own owner map, not
through Ghidra's function bodies, which cover only 925,415 of the 1,056,977
executable bytes; `phase_seeds.source_references_with_no_owner` counts anything
that still resolves to nothing, so a future drop is visible rather than silent.
The graph also carries an edge from each dispatch table's installer to every
method it holds, without which those methods look unreachable.

Every row carries `phase_provenance`, so a worker holding one row can tell
evidence from propagation without re-deriving the assignment. Beside the nine
rules above it takes three values for rows no layer reaches: `padding` for an
alignment run, `pe_structure` for the PE structures pinned to shared runtime, and
`reading_sites` for a data object phased from the code that reads it.
`oracle/coverage.json` records the same thing in aggregate, per rule and per
phase.

## Tools

```powershell
# Validate the census and its label ledger. The ledger is resolved from pins.labels.
python docs/reconstruction/novodex-physics/tools/validate_inventory.py docs/reconstruction/novodex-physics/inventory.json

# Pin and verify the recursive public-header tree.
python docs/reconstruction/novodex-physics/tools/verify_public_headers.py --write-manifest --root D:\FlamingEnt__\Unreal_3\Development\External\Novodex\Physics\include --output docs/reconstruction/novodex-physics/public_header_hashes.json
python docs/reconstruction/novodex-physics/tools/verify_public_headers.py --manifest docs/reconstruction/novodex-physics/public_header_hashes.json --root D:\FlamingEnt__\Unreal_3\Development\External\Novodex\Physics\include

# Pin and verify the analysis toolchain.
python docs/reconstruction/novodex-physics/tools/verify_toolchain.py --write-pin --ghidra-home $env:NOVODEX_GHIDRA_HOME --output docs/reconstruction/novodex-physics/analysis_toolchain.json
python docs/reconstruction/novodex-physics/tools/verify_toolchain.py --pin docs/reconstruction/novodex-physics/analysis_toolchain.json --ghidra-home $env:NOVODEX_GHIDRA_HOME

# Regenerate the ABI shim from the pinned headers.
python docs/reconstruction/novodex-physics/tools/generate_ghidra_types.py --headers D:\FlamingEnt__\Unreal_3\Development\External\Novodex --inputs docs/reconstruction/novodex-physics/ghidra/physics_type_inputs.json --output docs/reconstruction/novodex-physics/ghidra/physics_x86_msvc.h

# Normalize a raw headless stream into the committed Ghidra oracle.
python docs/reconstruction/novodex-physics/tools/normalize_ghidra.py --input $analysisRoot\ghidra.raw.jsonl --output docs/reconstruction/novodex-physics/oracle/ghidra/manifest.json

# Disassemble the executable extent into the committed Capstone oracle.
python docs/reconstruction/novodex-physics/tools/capstone_manifest.py --binary D:\FlamingEnt__\Unreal_3\Binaries\NxPhysics.dll --pe docs/reconstruction/novodex-physics/oracle/pe.json --ghidra docs/reconstruction/novodex-physics/oracle/ghidra/manifest.json --output docs/reconstruction/novodex-physics/oracle/capstone/manifest.json

# Reconcile the three oracles into the inventory, the ledger and both reports.
python docs/reconstruction/novodex-physics/tools/reconcile_analysis.py --pe docs/reconstruction/novodex-physics/oracle/pe.json --ghidra docs/reconstruction/novodex-physics/oracle/ghidra/manifest.json --capstone docs/reconstruction/novodex-physics/oracle/capstone/manifest.json --inventory docs/reconstruction/novodex-physics/inventory.json --labels docs/reconstruction/novodex-physics/labels.json
```

`reconcile_analysis.py` reads the report paths out of `coverage.census`, so it
writes `oracle/coverage.json`, `oracle/data-coverage.json` and
`oracle/dependencies.dot` beside the inventory it was given. It is idempotent:
running it on its own output reproduces every artifact byte for byte.

The Ghidra project and the `*.raw.jsonl` streams live under
`D:\FlamingEnt__\novodex-analysis\novodex-physics` and are never committed. That
path is outside the repository because Ghidra 12.1.2 refuses any project path
containing an element that begins with `.`, so the plan's original `.analysis`
location cannot host a project (`NamingUtilities.checkName`: *Path element
starting with '.' is not permitted*).

Each tool prints its measured result and exits `0`, prints `error:` lines to
stderr and exits `1` on a substantive failure, or exits `2` on unreadable input.

**Every extractor must verify the toolchain pin before producing evidence.** A
changed Ghidra version, headless path or hash, Capstone version or package hash,
or analysis-option hash invalidates existing evidence, so Tasks 3 and 4 call
`verify_toolchain.py --pin` first.

The pin records the Ghidra version, headless launcher path and hash, Capstone
version, Capstone package-root content hash, Python version, the
processor/language/compiler spec, analyzer enablement, the analysis timeout, and
a canonical order-independent hash of the analysis options. Bytecode caches are
excluded from the package hash so the pin survives interpreter changes. The
Capstone package path is recorded but not verified, because it moves with the
Python environment while Ghidra is pinned to one explicit installation; the
Python version is likewise recorded for provenance rather than gated.

## Tests

```powershell
$env:PYTHONDONTWRITEBYTECODE='1'
python -m unittest discover -s docs/reconstruction/novodex-physics/tools/tests -p 'test_*.py' -v
```
