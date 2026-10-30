# The public shape handles' rows (001099 to 001235)

Twenty-eight Phase 3 rows in `Physics/src/NpShapeHandleRows.cpp`: naked listing transcriptions, 779 instructions.
They are the public shape handles' methods (the oracle's `NpBoxShape.cpp`, `NpPlaneShape.cpp`, `NpSphereShape.cpp`,
`NpCapsuleShape.cpp` and `NpTriangleMeshShape.cpp`) that had not been reconstructed. Each one:

1. takes the handle's read lock (`+0x14`, `ReadWriteLock::lock`) or tries its write lock (`+0x10`,
   `ReadWriteLock::tryLock`), where a refused write lock reports through the Foundation error stream;
2. calls the internal shape at `+0x18` (the `ShapeRows.cpp` rows, `ShapeBase::nxSetMaterialIndex`,
   `PlaneShape::nxPlaneSetEquation` or the name binding's `SdkPointerBindingThunk::setValue`);
3. unlocks.

| Row | RVA | Method | Slot (tables) |
|---|---|---|---|
| 001099 | `0x00023940` | getGlobalPosition | 20 (box, capsule, plane, sphere, mesh) |
| 001101 | `0x00023970` | getGlobalOrientation | 21 (box, capsule, plane, sphere, mesh) |
| 001103 | `0x000239a0` | setMaterial | 25 (capsule) |
| 001105 | `0x00023a00` | setName | 29 (capsule) |
| 001131 | `0x00023da0` | setFlag | 5 (plane) |
| 001133 | `0x00023e00` | setLocalPose | 7 (plane) |
| 001135 | `0x00023e60` | setLocalPosition | 8 (plane) |
| 001137 | `0x00023ec0` | setLocalOrientation | 9 (plane) |
| 001139 | `0x00023f20` | setGlobalPose | 16 (plane) |
| 001141 | `0x00023f80` | setGlobalPosition | 17 (plane) |
| 001143 | `0x00023fe0` | setGlobalOrientation | 18 (plane) |
| 001145 | `0x00024040` | setMaterial | 25 (plane) |
| 001147 | `0x000240a0` | setName | 29 (plane) |
| 001151 | `0x00024130` | getGlobalPose by value | 22 (box, capsule, plane, sphere, mesh) |
| 001153 | `0x00024160` | getGlobalPosition by value | 23 (box, capsule, plane, sphere, mesh) |
| 001157 | `0x000241f0` | setPlane | 31 (plane) |
| 001167 | `0x00024340` | setFlag | 5 (sphere) |
| 001181 | `0x000245e0` | setMaterial | 25 (sphere) |
| 001211 | `0x00024990` | setFlag | 5 (mesh) |
| 001213 | `0x000249f0` | setLocalPose | 7 (mesh) |
| 001215 | `0x00024a50` | setLocalPosition | 8 (mesh) |
| 001217 | `0x00024ab0` | setLocalOrientation | 9 (mesh) |
| 001225 | `0x00024ba0` | setGlobalPose | 16 (mesh) |
| 001227 | `0x00024c00` | setGlobalPosition | 17 (mesh) |
| 001229 | `0x00024c60` | setGlobalOrientation | 18 (mesh) |
| 001231 | `0x00024cc0` | getGlobalPose | 19 (box, capsule, plane, sphere, mesh) |
| 001233 | `0x00024cf0` | setMaterial | 25 (mesh) |
| 001235 | `0x00024d50` | setName | 29 (mesh) |

The method names are the candidate box table's names for those slots.

`evidence/phase3-shape-handle-rows-listing-compare.py` finds every instruction equal, with 84 call mappings as
expected (`raw/phase3-shape-handle-rows-20261030/listing-compare.txt`). The API reentry message is
`gNxApiReentryMessage` by name; the Foundation imports go through the candidate's import slots.

## Wiring

`raw/.../facade_slots.txt` lists every slot of the five oracle handle tables (`0x00106dc8` box, `0x00106f40`
capsule, `0x001070b0` plane, `0x00107218` sphere, `0x00107388` mesh) that holds one of these rows: 48 slots.
`NpActor.cpp` assigns each one in the candidate's table for that family (`nxBoxShapePublicVtable`,
`nxMeshShapePublicVtable`, and `nxShapePublicVtable`'s plane, sphere and capsule tables). This replaces the C++
handle methods the box table had lent those families, and the box table's own getters for slots 19 to 23.

What changes in behaviour:
* The getters now take the read lock, which the C++ getters skipped.
* The capsule's slot 25 is 001103, as the oracle has it, where the candidate had placed `nxCapsuleHandleSetMaterial`
  (row 001111, which the oracle's capsule table holds at slot 36).

Rows with indirect calls: every write-lock row (the Foundation error import).

The rows await the testing pass: there is no differential or mutation aimed at them yet.
