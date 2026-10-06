# Grounded controller step-probe flags

The approved low-step scene now covers the blocked, grounded +Y box-controller
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

Verification on the rebuilt Release DLL: Phase 5 passes 2,042/2,042 coverage
assertions; Phase 7 passes 1,355/1,355; all 48 Viewer CTest selections pass,
including all 39 scene entries (34 pass, five existing signature-verified
oracle asset cases skip) and both focused Viewer physics checks. The
reconstruction tooling suite passes 770 tests. Inventory validation reports
6,338 functions, 5,138 data objects, and zero unexplained records.
