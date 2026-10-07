# Grounded controller step-probe flags

The approved low-step scene covers the blocked, grounded +Y box-controller
path in a paired simulation differential. A static floor and a low static box
are queried while a controller requests forward and downward motion. The
descriptor enables step probing and supplies a 0.5 step offset.

IDA evidence comes from the controller constructor at VA `0x1005a0f0` and the
resolver at `0x10059320`. The constructor records whether the descriptor's
step-setting word at `+0x1c` is nonzero. The resolver checks that setting, a
downward component along its selected up axis, and a qualifying collision
normal before issuing an additional downward query. Its initial upward,
horizontal, and adjusted movement probes contribute separate flag bits.

In this fixture, the oracle ends at `(0.5, 0.5, 0)` with collision flags `0x5`.
Before the candidate change, its single swept-box path ended at the same pose
but reported `0x6`. The candidate now records whether step probing is enabled
and translates the tested side-plus-down flag combination to the oracle's
side-plus-probe result. The focused differential is exact (`stdout_delta=0`,
`stderr_exact=True`; `build/step-scene-green-diff.log`).

This closes only the observed blocked grounded probe and its flag combination.
Successful step-up motion, other up axes, slopes, transformed obstacles,
multiple contacts, and hit callbacks remain open. No public Physics header was
changed.

Verification on the earlier rebuilt Release DLL: Phase 5 passes 2,042/2,042
coverage assertions; Phase 7 passes 1,355/1,355; all 48 Viewer CTest selections
pass, including all 39 scene entries (34 pass, five existing signature-verified
oracle asset cases skip) and both focused Viewer physics checks. The
reconstruction tooling suite passes 770 tests. Inventory validation reports
6,338 functions, 5,138 data objects, and zero unexplained records.

## Current follow-up: grounded +Y probe order

The registered simulation fixture starts a +Y box controller at `(0, 0.8, 0)`
with half-extents `(0.5, 0.5, 0.5)`, step probing enabled, and a `0.5` step
offset. It moves `(3, -0.5, 0)` toward a `0.2`-high static box on a static
floor.

The first paired run was RED. The pinned oracle stopped at `(0.5, 0.5, 0)` with
flags `0x5`; the candidate slid across the obstacle top to `(3, 0.7, 0)` with
flags `0x2`. IDA's `phys_fn_002306` (`NxPhysics.dll+0x59320`) performs separate
side and downward sweeps. For the tested enabled +Y downward probe, the
candidate now resolves the vertical movement before the horizontal movement,
preserving the oracle's blocked result. The restored staged-pair differential
matches exactly (`stdout_delta=0`, exact stderr).

This case does not demonstrate successful step-over. It covers only this
grounded +Y downward probe and leaves successful step-up, other up axes,
transformed obstacles, generalized multi-contact sliding, and callbacks open.
The complete resolver row `phys_fn_002306` remains partial and is not closed.

On the rebuilt Release candidate, Phase 5 passes 2,250/2,250, Phase 6 passes
1,057/1,057, and Phase 7 passes 1,373/1,373 registered assertions. The full
Viewer CTest selection completes 48/48: 43 pass and five signature-verified
pinned-oracle asset cases skip, covering all 39 scenes. Coverage registry tests
pass after correcting the Phase 5/6 floors to their actual registration counts.
Public Physics headers are unchanged.
