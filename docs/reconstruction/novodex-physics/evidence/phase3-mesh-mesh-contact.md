# Matrix A [MESH][MESH] (001876)

`Physics/src/ContactMeshMeshRow.cpp` holds phys_fn_001876 (`0x00046ab0`, 1,597 bytes) as a naked listing
transcription of 450 instructions. The row dispatches by pair type:

* convex pairs go to 001820;
* height fields go to 001847 or 001869;
* penetration maps go to 001880;
* a convex side goes to 001853;
* otherwise the two OPCODE trees are walked by 004982 with 001874 as the callback, which fills the
  ContactAccumulator block. The sums are passed to `NxNormalToTangents`, and the four extremal contacts are emitted
  through 000873 (`NxEmitContactThiscall`, the oracle's thiscall emitter).

`evidence/phase3-mesh-mesh-contact-listing-compare.py` finds every instruction equal, with 11 call mappings as expected
(`raw/phase3-mesh-mesh-contact-20261030/listing-compare.txt`).

## Statics

The oracle's statics at `0x10123cf8` to `0x101240a0` are the candidate's own variables by name:

| Oracle address | Candidate variable |
|---|---|
| `0x10123cf8` | 001872's `nxMeshContactNormalAndMaterial` (32 words are addressed) |
| `0x10123d78`, `0x10123d7c` | 001874's matrix slots, `nxTask2lSphereMatrixA` and `B` |
| `0x10123d80`, `0x10123d88` | two words only this row writes, new statics in the file |
| `0x10123d8c` | the contact count |
| `0x10123d90` | the sums |
| `0x10123da0` | the normals |
| `0x10123f20` | the vertices |

`transcribe_rows` now names a `[register + absolute]` operand through the config's `DATA`.

## Wiring

`NxContactMeshMesh` (`ContactMeshMesh.cpp`, which has the same cdecl signature) is now a forward to the row, and its
C++ body and helpers are deleted. Gates 3 to 7 pass with it.

Rows with indirect calls: none besides the `NxNormalToTangents` import.

The row awaits the testing pass.
