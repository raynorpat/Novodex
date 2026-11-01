# The public triangle mesh's rows (002095 to 002138)

Twelve functions (24 Phase 4 rows) in `Physics/src/NpTriangleMeshRows.cpp`: naked listing transcriptions, 720
instructions. They are the oracle's `NpTriangleMesh.cpp` methods, slots of the public triangle mesh's vtable at
`.rdata 0x00108298`. Each method:

1. takes every scene's read or write lock, walking the SDK's scene list (`PhysicsSDK::getNbScenes`/`getScene`,
   the scene's `+0x6cc` block);
2. calls the internal mesh at `+4` through its own table, or reads a word of it;
3. releases the locks.

The write-lock methods report a refused lock through the Foundation error stream. The inventory splits each function
into a thunk and a body at the first loop's jump target.

| Rows | RVA | Public slot | Lock | Internal slot called |
|---|---|---:|---|---|
| 002095, 002097 | `0x00052a60` | 1 | write | 2 |
| 002098, 002100 | `0x00052b50` | 2 | write | 3 |
| 002101, 002103 | `0x00052c40` | 8 | write | 9 |
| 002104, 002106 | `0x00052d30` | 9 | read | 10 |
| 002108, 002110 | `0x00052dc0` | 10 | read | 11 |
| 002112, 002114 | `0x00052e50` | 11 | read | 12 |
| 002116, 002118 | `0x00052ef0` | 12 | read | 13 |
| 002120, 002122 | `0x00052f80` | 3 | read | (reads +0xc) |
| 002124, 002126 | `0x00053010` | 4 | read | 5 |
| 002128, 002130 | `0x000530b0` | 5 | read | 6 |
| 002132, 002134 | `0x00053150` | 6 | read | 7 |
| 002136, 002138 | `0x000531f0` | 7 | read | 8 |

`evidence/phase4-np-triangle-mesh-rows-listing-compare.py` finds every instruction equal, with 102 call mappings as
expected (`raw/phase4-np-triangle-mesh-rows-20261030/listing-compare.txt`). `PhysicsSDK::instance` is reached by
name, through an alias.

**Wired and driven.** The DLL's `NxTriangleMesh` wrapper (`TriangleMesh.cpp`, under `NX_NP_TRIANGLE_MESH_ROWS`) calls the twelve methods, and the internal `TriangleMesh` carries a table for the slots they call (slots 3 and 9 take one argument; the `push ebx` before it is the row saving the register). `NxPhysicsTriangleMeshApiTests` drives every method, with a scene alive so the lock loops run. All 24 rows are falsified by a mutation inside their range (`raw/testing-pass-np-triangle-mesh-rows-20261031/`); 20 of them by a mutation that faults the candidate while its output differs (`--count-crash`), the rest by an output difference.

Rows with indirect calls: all except 002120/002122 (the internal mesh's table) and the write-lock rows (the error
import).

