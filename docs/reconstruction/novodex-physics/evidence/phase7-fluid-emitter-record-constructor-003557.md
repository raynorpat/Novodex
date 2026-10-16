# Phase 7 fluid-emitter record constructor (`phys_fn_003557`)

IDA at RVA `0x00087c40` shows a 0x78-byte record constructor. It installs the internal emitter vtable, clears the owner pointer and selected state words, initializes the local and world 3x3 matrices to identity, creates the 0x18-byte `NpFluidEmitter` wrapper, and sets the backend emitter handle to `0xffffffff`. The flags and frame-actor words are intentionally left untouched by this row. The source-backed helper `nxNpFluidEmitterConstructRecord` now owns these writes and is called by the real fluid-emitter creation path after the factory clears its allocation.

The registered `NxPhysicsFluidEmitterAbiTests` fixture calls the pinned oracle row and the map-resolved candidate helper on separate 0xa5-filled records. It compares the complete record while excluding image-specific vtable and wrapper pointer words, and explicitly checks both identity poses, owner, emitter sentinel, wrapper link, untouched flags/frame fields, return pointers, and x86 stack balance. The restored candidate reports `fluid emitter internal ctor cases=1 identity_local=1 identity_world=1 wrapper=1 untouched=1 stack=1/1 mismatches=0`.

For mutation testing, the candidate world-pose identity initialization was replaced with zeros. The probe rejected it with `mismatches=7` and exit 1. Rebuilding the restored implementation returned the focused differential to exit 0 with zero mismatches. The raw mutation and restored logs, full phase gates, and hashes are retained under `evidence/raw/phase7-fluid-emitter-record-constructor-003557-20261016/`.

This closes the record constructor only. Its scalar deleting destructor `phys_fn_003603` and the visualization routine `phys_fn_003607` remain open. Public headers are unchanged.
