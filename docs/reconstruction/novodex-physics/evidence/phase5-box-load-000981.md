# Phase 5 mutation proof: `phys_fn_000981`

`BoxShape::nxBoxLoadFromDesc` copies three dimensions from descriptor offsets `+0x4c`, `+0x50`, and `+0x54`, rebuilds the box hull, then applies the base descriptor. The registered `NxPhysicsObjectLayoutTests` oracle differential directly drives this row through a constructed box and checks that its loaded dimensions match the oracle.

Changing the first dimension read at `+0x21998` from `record+0x4c` to `record+0x50` makes the candidate check fail (`boxload candidate ok=0`, `layout candidate mismatches=1`, exit 1; `build/phase5-000981-mutant.log`). Restoring `record+0x4c` gives `boxload candidate ok=1`, zero mismatches, and `layout result=differential-pass` (`build/phase5-000981-restored.log`). Public headers were not changed.
