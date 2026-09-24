# Dynamic-first pruning initialization (intermediate)

`NxPhysicsDynamicFirstTests` builds the lifecycle harness as a separate Win32
process that creates a dynamic box as the scene's first actor. This matters
because the static-first lifecycle test has already initialized OPCODE's
process-wide pool and the Scene's cache arrays. Both executables load staged
oracle/candidate DLL pairs and report module identity.

The pinned oracle makes 34 allocations and six `0x800` staging-buffer frees
for the dynamic-first actor. A temporary allocator call-site instrument showed
that `FUN_100124d0` registers the dynamic record in Scene+`0x56c`, then calls
`FUN_100100a0` to allocate the two `0x400` Scene cache arrays. The `0x3c`
dynamic pruner follows. It creates the same process-wide `0x1c` OPCODE pool
and four small buffers as the static pruner, then `0x60`/`0x10` entry/reference
buffers and a Scene pending `0x8` buffer. The actor-list `0x8` allocation is
last. The candidate now matches the oracle's entire allocation and free-size
sequences in that order.

The dynamic pruner is linked from Scene+`0x648` and shape+`0xc4`; its tested
count/capacity is `1/4`. Five registered transcript lines raise the Phase 5
coverage floor from 332 to 337. A temporary mutation that cleared shape+`0xc4`
made the dynamic-first differential fail and was reverted. The ordinary
static-first actor differential remains exact after the registration call was
moved to the actor-load path.

This does not close dynamic pruning: spatial entry payloads, pruning queries,
capacity growth, removal across multiple shapes, the internal vtable, and
Scene teardown still need reconstruction and proof. Phase 5 remains red at the
explicit final-vtable marker. Public headers were not changed.
