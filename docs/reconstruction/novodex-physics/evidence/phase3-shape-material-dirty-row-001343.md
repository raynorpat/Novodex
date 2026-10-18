# Shape material dirty-table row `phys_fn_001343`

The internal row at RVA `0x000276f0` stores a 16-bit material index at `ShapeBase+0xda`, then tail-calls the owning Scene's dirty-table helper with bit `8`. The five public shape-family setters keep their existing Scene write-lock checks and now route the successful update through `ShapeBase::nxSetMaterialIndex` in `Physics/src/ObjectModel.cpp`.

`NxPhysicsActorShapeMutationTests` builds a fresh Scene and box shape, resets its dirty-list state, then calls the pinned oracle row directly and resolves the candidate helper from `NxPhysics.map`. The registered marker is `shape_mutation row=001343 material=4a39 dirty=8 queued=1 index=0 mismatches=0`. The focused restored differential reports `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, and `stderr_exact=True`.

Two independent mutations were tested. Removing only the `mHalfwordDA = material` store produced `material=0000 dirty=8 queued=1 index=0 mismatches=1` and was rejected (`stdout_delta=4`). Removing only `nxSceneMarkShapeDirty(this, 8)` produced `material=4a39 dirty=0 queued=0 index=3490442960 mismatches=1` and was rejected (`stdout_delta=2`); the dirty index is the untouched `0xd00beed0` sentinel. Rebuilding the restored source returned the focused differential to an exact match.

IDA's disassembly for the row is the static contract: a 16-bit store to `+0xda`, argument `8`, then a tail call to RVA `0x00026c90`. The current candidate map resolves `?nxSetMaterialIndex@ShapeBase@@QAEXG@Z` at RVA `0x0004e230` in `ObjectModel.obj`.

Raw red, green, both mutation, and restored transcripts are in `evidence/raw/phase3-shape-material-dirty-row-001343-20261018/`. Fresh Win32 Release Phase 5/6/7 gates pass at 2,917/2,917, 1,770/1,770, and 3,053/3,053 coverage assertions. Phase gate transcripts are compressed in the raw evidence directory.
