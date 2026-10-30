# Two pruning rows: 001935 and 001947

Two functions in `Physics/src/opcode/PrunableRows.cpp`: naked listing transcriptions, 80 instructions.

* **001935** (`0x0004b9c0`): the callback that the pruners' visualisation walk (001967) passes by address. A node
  with a box at `+0x18` is drawn through `PhysicsSDK::getDebugRenderable` (slot `+0x2c`, colour `0xffff0000`), and
  the callback returns true.
* **001947** (`0x0004bc20`): the inventory splits 001945 (`0x0004bbd0`) at its loop target, so 001947 is the second
  half of that function. The row transcribes the whole function (`nxPrunerOwnerDestroyRow`):
  1. the owner's primary prunable is removed through `PrunableOwnerDispatcher::Dispatch`, and each compound child's
     after it;
  2. the manager's SweepAndPrune is destroyed and freed through the SDK allocator;
  3. the owner's prunable link is cleared.

  001945 keeps its existing record.

`evidence/phase3-prunable-rows-listing-compare.py` finds every instruction equal, with 7 call mappings as expected
(`raw/phase3-prunable-rows-20261030/listing-compare.txt`). `PhysicsSDK::instance` (`.data 0x10123c04`) is reached
by name, through an alias.

Not wired yet; both are listed for the testing pass:
* `NxScenePrunerOwnerDestroy` (`IcePrunable.cpp`) keeps its C++ body. The ObjectModel-only test targets link it
  without the joint-support rows and OPCODE's SweepAndPrune that the transcription calls.
* The candidate's visualisation walk (`SceneVisualize.cpp`) uses its own callback shape, so nothing passes 001935
  yet.

Rows with indirect calls: both (the renderable's slot and the SDK allocator's).

The rows await the testing pass.
