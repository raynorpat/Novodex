# Phase 7's remaining rows

Twenty-six Phase 7 rows (21 functions) in `Physics/src/Phase7Rows.cpp`, with 16 helper functions. They are naked
listing transcriptions, 5,599 instructions in 37 functions.

| Rows | What they are |
|---|---|
| 002268 to 002316 (`0x00056690` to `0x0005a16a`) | the continuous-collision sweep geometry: box, capsule and triangle sweep tests (several functions the inventory splits at jump targets), the sweep cache and its table, the per-shape sweep dispatch |
| 002332, 002336 | the character controllers' creation (`NpScene::createControl` reaches 002336) |
| 002350 | a 1,982-byte row of the simulation step (000655 calls it) |
| 002400 | the step's thread-side wrapper over the scene's `simulateFrame` with the scene lock |

`raw/phase7-rows-20261030/manifest.json` lists each row's calls, as read from the listing.

## What the transcription names

* **The candidate's own code and data where it has them:**
  * `nxIceIdentityPoses` (001639);
  * ICE's box tables through `AABB::GetVertexNormals`/`GetEdges`/`GetEdgeNormals` and `OBB::GetEdges`;
  * `nxPOSetCenterExtents` (000923);
  * the scene rows `NxSceneInternal::initialise`/`simulateFrame`/`overlapAABBShapes`;
  * `BoxShape::nxFillShapeDescriptor`;
  * `TriangleMesh::createEdgeList`;
  * the SDK containers and allocator.
* **New statics in the file:** the sweep cache and its guard (the oracle's `0x1012466c`, `0x10124670`) and the six
  signed axes (`0x101222f8`, a table the image never writes after start-up).
* **Controller tables built entry for entry:** `0x10108740`, `0x1010878c..0x101087ac` (which the oracle's
  `0x10108798`, `0x1010879c` and `0x101087a8` point into) and `0x10108818..0x10108830`. Their slots are rows
  transcribed as helpers (002312, 002314, 002322, 002324, 002326, 002328, 002334, 004031) and the CRT's `_purecall`.
  The `NxShapeDesc` table is the candidate's.
* **Helpers** (records unchanged): the table entries above, and 000001, 001281, 001957, 001960, 002371, 002373,
  002375, 002387 and 002389. Several of these are the step's thread primitives over kernel32's `SetEvent`,
  `ResetEvent`, `WaitForSingleObject` and `ExitThread`. `autocfg` now decorates `__stdcall` imports.
* **CRT routines** are the candidate CRT's: `malloc`, `free`, `_fpclass`, `__chkstk`.

`evidence/phase7-rows-listing-compare.py` finds every instruction equal, with 111 call mappings as expected
(`raw/phase7-rows-20261030/listing-compare.txt`). `listing_compare_lib` now also reads a `word ptr` compare against a
negative immediate as its 16-bit value: the assembler chose the sign-extended 8-bit immediate where the oracle has
the 16-bit one, which is the same compare.

**Not wired.** The candidate's step, sweep and controller paths keep their own implementations. The rows are listed
for the testing pass.

The rows await the testing pass.
