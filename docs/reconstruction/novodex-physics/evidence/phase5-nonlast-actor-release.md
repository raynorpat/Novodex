# Phase 5 non-last dynamic actor release — 2026-09-24

The ordinary staged actor lifecycle now releases the first of two surviving
dynamic box actors after the two-box group has been released. Oracle and
candidate outputs match with both exits 0 and `stdout_delta=0`. Eighteen
registered lines raise the Phase 5 floor from 223 to 241.

The oracle's Scene auxiliary manager retains stable physical slots. On
releasing physical slot 0, `+40[0]` becomes zero and `+60[0]` becomes the
`d00beed0` vacant marker. Physical slot 1 stays occupied; the `+50` active
list compacts from `[0,1]` to `[1]`, and reverse index `+60[1]` changes
from 1 to 0. The live count becomes one while the 256-slot buffers remain.
The candidate previously moved slot 1's record and occupancy into slot 0.

The public actor wrapper's `+0xc` word is overwritten by a shared Scene lock
link. Its stable actor ID is copied into the outer body at `+0xc`; the tested
actor has ID 1. On release, the Scene+`0x6d4` actor-ID free list grows from
`2/2` containing `3.0` to `3/6` containing `3.0.1`. Oracle allocator stack
capture traced its reserve call to RVA `0x1bd6`, and a pre-release Scene
snapshot identified the freed 0x8 buffer at `+0x6d4`. The resulting
allocation is 0x18; free order is `18.260.8.1c.228.50`. Broadphase count
falls from `2/8` to `1/8`; the shape-ID free list grows from `3/6` to
`4/6`, adding ID 1. The temporary stack/owner tracing was removed after
the registered checks were added.

A sensitivity mutation kept the surviving slot's reverse index at 1 instead
of compacting it to 0. After rebuilding the DLL, the staged differential
failed with `stdout_delta=2` and printed the altered `+60[1]` value. Restoring
the assignment and rebuilding returned `stdout_delta=0`. The tool suite
passed 628 tests, and the Phase 5 gate evaluated 241/241 registered checks;
its only remaining candidate layout mismatch is the explicit final-vtable
marker.

The next dynamic actor also takes the vacated physical auxiliary slot 0,
while the compacted active list becomes `[1,0]` and the reverse indices
become `[1,0]`. It reuses actor ID 1 and shape ID 1, restores broadphase
count 2/8, and creates its body graph with the five allocations
`50.18.228.1c.260`. Eleven further registered lines raise the Phase 5 floor
to 252. This closes the measured remove-then-reuse path. Arbitrary removal
order across shape classes and Scene destruction remain open. The Phase 5
final-vtable marker remains an explicit gate failure.
