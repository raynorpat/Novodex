# Fluid particle write-data copy helpers (`phys_fn_003425`, `phys_fn_003427`)

The helper at RVA `0x84d10` copies eleven dwords from `this+0x28` to the output buffer; the helper at `0x84d30` copies eleven dwords from its input to `this+0x28`. The candidate now implements these as private, no-inline methods in `Physics/src/Scene.cpp`, and the locked public `NxFluid` read/write wrappers call those methods. The public Physics headers are unchanged.

The registered `NxPhysicsFluidEmitterAbiTests` probe invokes each pinned-oracle RVA and its map-resolved candidate DLL method on three distinct 44-byte records. It compares all copied bytes, checks sentinels before and after the copy region, and verifies x86 stack balance. The getter result is treated as void because the oracle helper leaves EAX unspecified.

Mutation proof: truncating `copyTo` by one dword is rejected with `getter_mismatches=3`; truncating `copyFrom` by one dword is rejected with `setter_mismatches=3`. Each mutant leaves the oracle successful and makes the candidate probe exit 1. Restored output reports `getter_mismatches=0 setter_mismatches=0 mismatches=0`, and the staged differential passes exactly.

The focused mutation and restored logs are archived under `evidence/raw/phase7-fluid-particle-write-data-copy-003425-003427-20261017/`.

Fresh Phase 5 and Phase 7 gates pass at 2,885/2,885 and 3,024/3,024 coverage assertions. Compressed gate and mutation transcripts plus the restored control are retained in the raw evidence archive.
