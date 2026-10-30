# The internal shape's rows (001303 to 001345)

Seventeen functions, eighteen Phase 3 rows, in `Physics/src/ShapeRows.cpp`: naked listing transcriptions, 1,861
instructions. These are the oracle's `Shape.cpp` methods that the public shape handles and the scene reach:

| Row | RVA | What it does |
|---|---|---|
| 001303 | `0x000258f0` | a CCD-path refresh: 000738 on the owner, then `Prunable::UpdateWorldAABB` |
| 001305 | `0x00025960` | the shared debug render, gated on SDK parameter elements 44 and 48 and scaled by element 13 |
| 001307 | `0x00025c10` | setGlobalPose |
| 001309 | `0x00025f60` | getGlobalPose into an out |
| 001311 | `0x00026280` | getGlobalPosition into an out |
| 001313 | `0x00026430` | getGlobalOrientation into an out |
| 001317, 001319, 001321 | `0x00026ad0`, `0x00026b70`, `0x00026ba0` | the three getters by value |
| 001325 and 001327 | `0x00026c90` | mark dirty: one function, which the inventory splits at a jump target |
| 001331 | `0x00026df0` | setFlag |
| 001333, 001335, 001337 | `0x00026e30`, `0x00026ec0`, `0x00026f50` | setLocalPose, setLocalPosition and setLocalOrientation |
| 001339, 001341 | `0x00026fd0`, `0x00027360` | setGlobalPosition and setGlobalOrientation |
| 001345 | `0x00027710` | a scalar deleting destructor over the base destructor 001323 |

`evidence/phase3-shape-rows-listing-compare.py` (on `listing_compare_lib.py`) finds every instruction equal, with
13 call mappings as expected (`raw/phase3-shape-rows-20261030/listing-compare.txt`). The generator inputs are in the
same bundle: `autocfg.py` drafts the config and header, and `tools/transcribe_rows.py` writes the file.

## Where it differs from a plain copy

* **Oracle `.data` by name.** The SDK parameter array (`0x10123b18`) is `gParameter`, which is no longer `static`.
  The API reentry flag (`0x10123c10`) is `gNxApiReentry`, now a dword, since the listings store 1 and 0 to it. Its
  message pointer (`0x10122050`) is `gNxApiReentryMessage`. The compare script names them on both sides.
* **Foundation imports** go through the candidate's own import slots (`__imp_` aliases).
* **001323**, called by 001345, is the new `ShapeBase::nxBaseDtor`: the owner arms, then `~Prunable`. It does not
  store the BASE table pointer, because the candidate's `ShapeBase` has no BASE table.

## Wiring

These callers now run the transcribed rows:

* the public handle helpers in `NpActor.cpp` that the gated setters and getters call:
  * the set and get pose, position and orientation helpers;
  * the by-value getters;
  * the set-flag paths of the box and capsule handles (001331);
* `nxSceneMarkShapeDirty` (`Scene.cpp`), which is now a forward to 001325.

Not re-pointed yet; each is listed for the testing pass:

* **001305:** `BoxShape::nxDebugRender` stays C++. `NxPhysicsShapeVtableTests` binds its guard words to the
  oracle's `.data` and toggles them there, but the transcribed row reads `gParameter`. That fixture has to change
  first.
* **001309:** `ShapeBase::nxShapeGlobalPose` stays C++ because the ObjectModel-only test targets do not link the scene
  globals. The handle getters do reach 001309.
* **001303** has no caller in the candidate: it is on the CCD path, which the candidate does not run.
* **001345** is reached through the BASE vtable, which the candidate does not build.

Rows with indirect calls: 001305 (the renderer), 001307, 001333 to 001341 (owner slot 6), 001325 (a container
slot) and 001345 (the allocator).

The rows await the testing pass: there is no differential or mutation aimed at them yet. Gate 3 passes with the
re-pointed helpers.
