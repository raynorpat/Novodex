# Phase 5: public box group and material

The pinned oracle's public box final routes slots 2, 3, 25, and 26 to group and material setters/getters. Ghidra's internal leaves show group at shape+`0xd8`, material at shape+`0xda`, and the `1 << group` filter mask at shape+`0xc8`. Setter leaves mark flags `4` and `8` through the Scene+`0x48` auxiliary table (internal shape+4 → outer body+4 → Scene). The descriptor loader copies the two descriptor values into the same packed word.

The staged lifecycle pair observes a newly created default box as `actor box defaults=0.0.1.0` and, after `setGroup(5)` and `setMaterial(1)`, observes `actor box mutated=5.1.20.10005`. These four components are the two public getter results, the mask dword, and the packed group/material dword. Oracle and source-built candidate both exit zero with `stdout_delta=0`. The two lines are registered in the Phase 5 gate, raising its coverage floor from 337 to 339. The public headers are unchanged.

An aimed mutation to the candidate group mask (`1 << (group - 1)`) changed the mutated line to `5.1.10.10005`; the staged differential failed with `stdout_delta=2`. Restoring `1 << group` returned both actor targets to `stdout_delta=0`. The 628 reconstruction-tool tests pass. The Phase 5 gate evaluates `339/339` coverage assertions and still fails only at its explicit final-vtable marker.

The observed instance already has `flags[shapeId] == 0xffffffff`, so this drive does not distinguish the dirty helper's zero-to-queued branch or growth. Material validation, nondefault descriptor values, invalid group reporting, other shape finals, and most box virtual slots remain separate work. This packet does not claim full-DLL completion.
