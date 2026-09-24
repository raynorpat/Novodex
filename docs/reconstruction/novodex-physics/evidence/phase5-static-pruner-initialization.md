# First static actor's OPCODE pruning state (intermediate)

The pinned Win32 oracle is `D:\FlamingEnt__\Unreal_3\Binaries\NxPhysics.dll`
(SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`).
The staged `NxPhysicsActorLifecycleTests` drive now compares the full first-static
actor allocation sequence. Oracle allocation call sites were captured by a
temporary `_ReturnAddress()` instrument on the guarded user allocator, then
mapped back to the Ghidra decompilation. That instrument was removed from the
committed test; the stable test records allocation sizes and normalized state.

After the box's public `0x1c` handle, the oracle allocates a `0x90` OPCODE
pruning object at `FUN_100b5090`. Its constructor initializes a process-wide
`0x1c` pool (`FUN_100b4cc0`), followed by `0x8`, `0x4`, `0x4`, and `0x4`
pool buffers (`FUN_100ef270`). The pruner's first insertion grows four entries
and references with `0x60` and `0x10` buffers (`FUN_100effc0`). A further
`0x8` Scene pending buffer, two `0x400` Scene cache arrays, and the actor-list
`0x8` allocation finish the creation path. The oracle and candidate now report
identical 24 allocation sizes in order and three `0x800` staging-buffer frees.

The pruner is linked from Scene+`0x640` and shape+`0xc4`. The tested scalar
words in its first `0x60` bytes match, as do the Scene cache capacity of 256,
entry count/capacity `1/4`, and a reference to the shape's embedded prunable
object at +`0xa4`. Releasing the static actor leaves the pruner allocated,
reduces its count to zero, increments its mutation counter to two, and leaves
the first reference slot uncleared, matching the oracle. Six registered lines
raise the Phase 5 assertion floor from 326 to 332. A temporary mutation of
the pruner's cached capacity made the staged differential fail, then was
reverted.

This does not close OPCODE pruning. The candidate's internal pruner vtable,
spatial entry payload, later capacity growth, dynamic-first initialization,
multi-static-shape insertion, cross-scene pool lifetime, and complete Scene
teardown remain unverified or unfinished. The Phase 5 gate still fails at its
explicit final-vtable marker. Public headers were not changed.
