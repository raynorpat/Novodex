# PenetrationMap compute-arm rows

Rows `phys_fn_005324`, `005326`, `005328`, `005329`, `005331`, `005333`, `005335`, `005337`, `005339`, `005341`, `005343`, `005345`, `005347`, `005349`, `005351`, `005353` (the nearest-face collider, `0x000e7c50` to `0x000e8f98`), `phys_fn_002019` (`0x0004e540`), `002021`/`002023` (`0x0004e5d0`) and `002025`/`002027` (`0x0004e870`).

## What the rows are

`PenetrationMap::create` (`phys_fn_002047`) fills a cubic grid. For each cell it asks the nearest-face collider (a 0x3c-byte object: vtable, mesh interface, point copy, squared distance at `+0x2c`, face at `+0x38`) for the face nearest the cell's centre. The entry `005353` picks one of four walks by the model's tree kind (no-leaf or quantized, plain or with a world matrix), each walk (`005335` to `005343`) prunes by the point-to-box distance (`005326`) and visits leaves (`005333`) through the point-to-triangle distance (`005324`); four entry stubs (`005345` to `005351`) take the square root. `002021` classifies a cell as inside or outside by casting rays drawn from `rand` and counting hits (`002019`, `002023`), and `002025`/`002027` mark the grid's corners.

## How they were written

Naked listing transcriptions generated from the pinned image (`Physics/src/PMapCompute.cpp`), 18 functions and 2,308 instructions. `evidence/phase4-pmap-compute-listing-compare.py` compares the built candidate with the oracle instruction by instruction: all equal, 58 call and data mappings checked, none unexpected. `PenetrationMap::create` now runs them for models with a tree.

## How they are gated

`NxPhysicsThirdPartyTests` (Phases 4 and 7), through `nxDrivePMapCompute`: the oracle's rows are entered with their registers by trampolines, the candidate's by inline asm, over every harness mesh and tree kind. The CRT `rand` of both sides is put at the oracle's state before each classification so a row that draws a different count shows.

* `pmap_nearest`: 800 queries (every mesh and tree kind, with and without a world matrix and a hint), the whole 0x3c-byte object after the query and after fini/destroy; 15,200 words.
* `pmap_classify`: 1,080 words; ray parity, classification and the next `rand` draw.
* `pmap_corners`: 93,500 words; the count and the whole grid, over three resolutions per model.

## Falsification

Each mutation is a one-line edit of the listing found by its address comment, rebuilt and run against the pinned oracle, the file restored between runs.

| row | mutation | family | result |
|---|---|---|---|
| `phys_fn_005324` | inverted the `jne` at 0x000e7f13 | pmap_nearest | mismatches=687 |
| `phys_fn_005326` | inverted the `jne` at 0x000e821c | pmap_nearest | mismatches=917 |
| `phys_fn_005328` | zeroed the vtable-pointer store at 0x000e82a2 | pmap_nearest | mismatches=1600 |
| `phys_fn_005329` | zeroed the vtable-pointer store at 0x000e82b0 | pmap_nearest | mismatches=800 |
| `phys_fn_005331` | zeroed the vtable-pointer store at 0x000e82c8 | pmap_nearest | mismatches=800 |
| `phys_fn_005333` | `fmul` to `fdiv` at 0x000e8354 | pmap_nearest | mismatches=415 |
| `phys_fn_005335` | inverted the `jne` at 0x000e851b | pmap_nearest | mismatches=645 |
| `phys_fn_005337` | field offset 0x2c changed to 0x30 at 0x000e8668 | pmap_nearest | mismatches=743 |
| `phys_fn_005339` | field offset 0x2c changed to 0x30 at 0x000e87c3 | pmap_nearest | mismatches=558 |
| `phys_fn_005341` | `fmul` to `fdiv` at 0x000e8986 | pmap_corners | mismatches=21 |
| `phys_fn_005343` | inverted the `je` at 0x000e8b7f | pmap_nearest | mismatches=768 |
| `phys_fn_005345` | `fsqrt` replaced by `nop` at 0x000e8e0b | pmap_nearest | mismatches=200 |
| `phys_fn_005347` | `fsqrt` replaced by `nop` at 0x000e8e4b | pmap_nearest | mismatches=200 |
| `phys_fn_005349` | `fsqrt` replaced by `nop` at 0x000e8eb8 | pmap_nearest | mismatches=200 |
| `phys_fn_005351` | `fsqrt` replaced by `nop` at 0x000e8f28 | pmap_nearest | mismatches=200 |
| `phys_fn_005353` | inverted the `jne` that admits a non-null model at 0x000e8f46 | pmap_nearest | mismatches=9600 |
| `phys_fn_002019` | zeroed the FLT_MAX store at 0x0004e5a5 | pmap_classify | mismatches=148 |
| `phys_fn_002021` | `fmul` to `fdiv` at 0x0004e719 | pmap_classify | mismatches=7 |
| `phys_fn_002023` | `seta` to `setb` at 0x0004e859 | pmap_classify | mismatches=360 |
| `phys_fn_002025` | `jbe` to `ja` at 0x0004e89c | pmap_corners | mismatches=30073 |
| `phys_fn_002027` | `fmul` to `fdiv` at 0x0004ea87 | pmap_corners | mismatches=19499 |

## What this does not close

* Models without a tree (the single-triangle mesh) keep the old brute-force path: the oracle crashes there, so no differential exists.
* The scalar-deleting destructor's `free` branch (`005331`, flag set) is not exercised, and the `-1` initial face of `005328` is overwritten by every query, so neither is observable.
* Mutations of `005353`'s tree-kind dispatch (a stub swapped for another) crash the process instead of mismatching; the null-model test is the one whose mutation gives a tape mismatch.
