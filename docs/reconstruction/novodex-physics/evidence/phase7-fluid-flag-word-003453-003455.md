# Fluid internal flag-word rows 003453 and 003455

The internal Fluid record stores its flag word at `+0x58`. The pinned image has two separate helper rows: `phys_fn_003453` conditionally ORs or clears the supplied mask and returns with `ret 8`; `phys_fn_003455` returns the word ANDed with its mask and returns with `ret 4`. `Physics/src/Scene.cpp` now implements both as private no-inline `NxFluidFlagsAt58` methods and routes the real `NxFluid::setFlag` and `NxFluid::getFlag` accessors through them. No public headers changed.

`NxPhysicsFluidEmitterAbiTests` directly invokes each pinned RVA and the corresponding map-resolved candidate DLL method. Four seeded records cover set, clear, overlapping/high-bit masks, full-word clearing, unaffected surrounding bytes, return values, and x86 stack balance. The restored differential reports `cases=4 set_mismatches=0 get_mismatches=0 stack=8/8 mismatches=0`.

Independent candidate mutations were detected. Replacing the setter's enabled OR with a clear operation produced `set_mismatches=2`, `get_mismatches=0`, `stack=8/8`, and test exit 1. Replacing the getter's AND with OR produced `set_mismatches=0`, `get_mismatches=4`, `stack=8/8`, and test exit 1. The restored control passed. Raw build and test logs are in `evidence/raw/phase7-fluid-flag-word-003453-003455-20261017/`.

Fresh registered gates pass: Phase 5 evaluated 2,886/2,886 coverage assertions and Phase 7 evaluated 3,025/3,025. Both included the flag-word row marker.
The reconstruction tooling suite passes all 817 tests. Inventory validation reports 6,338 function rows, 5,138 data objects, and zero unexplained entries. Public SDK headers remain unchanged.
