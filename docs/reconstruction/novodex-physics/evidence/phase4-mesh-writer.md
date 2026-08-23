# Phase 4 Task 4: the mesh stream writer and the block stream's store half

This is the evidence behind `NxPhysicsAssetTests` gaining nine writer cases and
staying at `candidate mismatches=0`: the oracle digest moves `b114fa73` ->
`eaefc573` because nine new probes joined the fold, `driven=30
accepted=13 rejected=16 errors=10`, and every registered line re-verified.

| repository | commit | contents |
| --- | --- | --- |
| this repository | `e8129d4` | `Physics/src/TriangleMesh.cpp`, `Physics/src/include/TriangleMesh.h`, `Physics/src/MemoryStream.cpp`, `Physics/src/include/MemoryStream.h`, `tests/PhysicsAssetTests.cpp` |
| this repository | `1337106` | census rows, labels, gate registrations, floor, transcript |

Oracle re-verified before and after:
`4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`,
1,253,376 bytes. All probing in a `git archive` copy of `1337106`; the
production tree was never mutated during a measurement run; no `git stash`.

---

## 1. What was reconstructed

**phys_fn_002162 (`TriangleMesh::save`), 0x000539d0, 413 bytes** -- slot 18 of
the nineteen-slot TriangleMesh vtable at `.rdata:0x00108608`, transcribed in
full from the Capstone listing:

| # | field | writer source | established by |
| ---: | --- | --- | --- |
| 1-2 | tags | `kTriangleMeshTag0/1` constants | pushes at `0x000539de`/`0x000539ea` |
| 3 | the global | file-scope zero, `.data 0x00124120`'s image value | `mov ecx,[0x10124120]` `0x000539f4` |
| 4 | flags | four conditions, nothing else touches eax | `0x00053a05`..`0x00053a2d` |
| 5 | convex threshold | storeFloat | push `0x00053a3d`, call +0x28 `0x00053a40` |
| 6 | height-field axis | storeDword | `0x00053a48`/`0x00053a4b` |
| 7 | height-field extent | storeFloat | `0x00053a56`/`0x00053a59` |
| 8-9 | counts | internal +0x00/+0x04 | `0x00053a61`/`0x00053a6c` |
| 10-11 | vertices, triangles | count*12, unconditional | `lea/shl` `0x00053a7a` and `0x00053a8f` |
| 12-13 | materials, face remap | non-null tested, *2 and *4 | `0x00053a9f`..`0x00053ac4` |
| 14-15 | presence flags A/B | storeDword, unconditional | `0x00053acf`/`0x00053ade` |
| 16-17 | arrays A/B | non-null tested, *4 each | `0x00053ae9`..`0x00053b12` |
| 18-19 | blob length + bytes | growable stream round trip | ctor `0x00053b20`, save `[edx+0x14]` `0x00053b2f`, length `0x00053b36`, collapse `0x00053b4e` |

The model dispatch is BaseModel slot 5 -- the Model vtable at
`.rdata:0x0011badc` holds `0x000e9440` there, whose second act is
`call [edx+0x14]` into the tree's own save. The reconstruction types the
pointer as the vendored `Opcode::BaseModel*`, whose seven-slot shape
NxPhysicsThirdPartyTests already asserts against the image.

**Eleven block-stream rows** moved from `discovered` to `reconstructed` --
the whole store half Task 3 had left unreconstructed:

| row | rva | what |
| --- | --- | --- |
| `phys_fn_004766` | `0x000b39b0` | appendBlock: doubling growth, linked through prev->next |
| `phys_fn_004768` | `0x000b3a30` | getLength = the sum of block->offset over the list |
| `phys_fn_004770` | `0x000b3a60` | storeByte |
| `phys_fn_004780` | `0x000b3b30` | seek: current block only, strictly in bounds |
| `phys_fn_004782` | `0x000b3b50` | the block allocator, four stack args, `ret 0x10` |
| `phys_fn_004784` | `0x000b3bf0` | destructor body |
| `phys_fn_004786` | `0x000b3c70` | flushBits |
| `phys_fn_004788` | `0x000b3ce0` | constructor over phys_fn_004782 |
| `phys_fn_004791` | `0x000b3db0` | five-byte jmp alias of phys_fn_004784 |
| `phys_fn_004795` | `0x000b3e30` | collapse, allocating and caller-buffer arms |
| `phys_fn_004797` | `0x000b3f00` | storeDword -- the row Model::Save calls directly |

Two facts the listings forced that the read side never showed:

- **A stream over given bytes is born FULL** -- the allocator stores
  `initialOffset` (= size, from both known callers) only when a buffer was
  copied, so reading starts past the end until phys_fn_004780 rewinds it.
  The harness has called ctor-then-seek(0) since Task 3 without knowing why
  the seek was load-bearing; now it is a transcription, not a ritual.
- **Task 3's inline-block stand-in is gone.** The image has ONE constructor
  row and its blocks come from the SDK allocator; `MemoryStream` now does
  too, with `sizeof == 0x1c` asserted. Every pmap case stayed green across
  the swap, which is the differential saying the two shapes were equivalent
  on everything Task 3 drove.

## 2. The differential, and what stands in for the tree serialiser

Each writer fixture is a mesh description handed identically to both sides;
each side serialises it through its own module, and the comparison is the
complete store log -- kind, value, and byte content of every storeDword,
storeFloat and storeBuffer -- plus the return value. Nine fixtures cover:
all optionals present, none present, zero counts, each conditional arm alone
(materials, remap, array A, array B), hull pointer present, hull mode bit
set, and everything together.

What stands in for the OPCODE model is INPUT, not reconstruction: both sides
write the same four deterministic dwords (derived from the fixture name and
counts) through their own module's stream API -- on the oracle side by calling
phys_fn_004797 at its recorded RVA exactly as the real `Model::Save` at
`0x000e9440` does. Everything the writer itself does AROUND that call --
field order, sizes, the four conditional arms, the length store, the collapsed
buffer, the destructor -- is pinned by the comparison. The tree serialiser
itself stays unmapped census work, named here rather than approximated.

The per-case digests are distinct, including `array_a_only` /
`array_b_only`, which differ in exactly one buffer's contents; registering
both is what catches a fold that stops at the first buffer. (An earlier draft
of the fold did exactly that and merged those two cases; it was fixed before
any registration was recorded.)

## 3. Falsification

Every run rebuilds and re-runs `NxPhysicsAssetTests` in an archive copy of
HEAD after restoring the production sources, controls bracketing the set.
The oracle digest is `eaefc573` and `expect_mismatches=0` in every single
run -- which is what says the mutations moved the candidate and not the
measurement.

| # | row | mutation | measured |
| --- | --- | --- | --- |
| 0 | -- | control | green, `candidate mismatches=0` |
| A | phys_fn_002162 | the two tags stored in swapped order | **red, 9** -- every writer case |
| B | phys_fn_002162 | `if(mConvexMesh) flags \|= 4;` removed whole | **red, 2** -- `writer.full`, `writer.hull_present`, first differing event = the flags store |
| C | phys_fn_002162 | vertices size `* 12` -> `* 8` | **red, 8** -- every case except `writer.empty_mesh`, which has no vertices to mis-size |
| D | phys_fn_002162 | threshold field replaced by a literal `0.0f` | **red, 9** |
| E | phys_fn_002162 | the blob length store deleted | **red, 9** |
| F | phys_fn_002162 | array A size `* 4` -> `* 2` | **red, 2** -- `writer.full`, `writer.array_a_only`, the only fixtures with the array present |
| Z | -- | control again | green, `candidate mismatches=0` |

The red counts localise the way the fixture design intended: each conditional
arm has at least one case that dies with it and cases that survive it, so no
single green case can hide behind a mutant's survival elsewhere.

One mutation-hygiene note worth keeping. An earlier form of B deleted only
the `flags |= 4;` statement text, leaving the bare `if(mConvexMesh)` to bind
the NEXT if as its body -- an accidental nested condition that turned a third
case red for a reason the mutation description did not claim. The run was
red either way; it was re-measured with the whole statement removed so that
the table says what was actually tested. A mutation that rewrites more or
less than its description claims produces a table entry that is red for the
wrong reason, which is no better than green.

## 4. The control word

None of the twelve reconstructed rows executes a floating-point ARITHMETIC
instruction: the writer's storeFloat path pushes raw bits and the stream's
store half moves bytes. An SSE2 build cannot disagree with the oracle on any
of them for codegen reasons, so the `/arch:IA32` rule correctly does not name
these files. This is a reading of the listings, not a measurement; if a later
task reconstructs a row in these translation units that does arithmetic on
floats, the CMakeLists note about the next kernel translation unit applies.

## 5. Registration

- `asset fixtures pmap=14 mesh=6 writer=9 release=1`
- `asset coverage driven=30 accepted=13 rejected=16 errors=10`
- `asset oracle digest=eaefc573`
- nine `writer case=` lines, each folding the ORACLE-side store log only

Phase 4's coverage floor moves 91 -> 100 (34 asset lines + 66 third-party),
with the independent minimum in `test_gate_targets.py` raised to match.

## 6. Environment escalations carried, not resolved

- ~~The pinned Nxp.h cannot pass against both roots at once.~~ **RESOLVED by
  moving the UE3 oracle tree onto the Foundation header.** The UE3 tree's
  `Physics/include/Nxp.h` and `Foundation/include/NxFoundationSDK.h` gave up
  their root-relative climbs -- the same change db44d18 made on this side --
  and `NxVersionNumber.h` (byte-identical, sha256 `0b0ee61a…`) was placed in
  the UE3 tree's `Foundation/include`. Both files now hash identically to
  this repository's copies (`ab032b9c…`, `31426e59…`),
  `public_header_hashes.json` was regenerated from this repository's tree,
  and verify_public_headers passes against BOTH roots; `run_phase_gate.ps1`
  -Phase 4 reports `status=pass at coverage_assertions_evaluated=100
  floor=100`, and -Phase 3 stays pass at 103.
- The vendored-source correspondence trees are staged at
  `.analysis/novodex-physics/thirdparty` (git-excluded copies of
  `External/*/upstream`), which is where validate_inventory expects them.

## 7. What this task did NOT do

- The reader's accept arm (fields 3-19) is still not reconstructed; the mesh
  column's 153 rows stand as they were.
- `NxCreatePMap`'s compute arm, the filename arm of loadPayload, the 5-bit
  cell walk, and the OPCODECREATE fill of phys_fn_002083 are unchanged holes.
- No Phase 3 deferral moved: `closure phase=3 closed=61 deferred=475` is
  unchanged, and the ten mesh-blocked rows stay blocked on the internal mesh,
  not on anything this task wrote.
