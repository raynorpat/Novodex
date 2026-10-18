# Implicit-mesh run callback (`phys_fn_003659`)

IDA identifies `phys_fn_003659` at RVA `0x0008ac10` as the FluidModel `ImplicitFluidMeshRun` dispatcher. It computes the triangle count from the mesh record's vertex count and forwards the backend context, fluid handle, and mesh handle. The candidate now exposes this callback as `nxImplicitFluidMeshRun`, and the emitter's existing surface-mesh dispatch routes through it. The ABI fixture resolves `@nxImplicitFluidMeshRun@8` from the candidate linker map and checks the callback arguments and result against the pinned oracle.

The restored direct differential reports `mapped=1 args=1 mismatches=0` with oracle and candidate exit 0. A return-0 mutation retains the map entry but removes the callback arguments; the registered probe reports `mapped=1 args=0 mismatches=1`, oracle exit 0, and candidate exit 1.

The first complete Phase 5 run also exposed an existing 30-second timeout in the nested invalid-surface-mesh child: under the gate's accumulated load, the expected access violation had not surfaced before the parent killed the child. Its wait budget now matches the 60-second allocation-failure probes. A complete Phase 5 simulation differential then reports `exit=c0000005 reached=1 returned=0 exact=1` for both pairs, and its x87 loader check passes.

Fresh Phase 5 and Phase 7 gates pass at `2,910/2,910` and `3,049/3,049` registered coverage assertions. The full logs and the mutation/restored transcripts are retained in `evidence/raw/phase7-implicit-mesh-run-003659-20261018/`.
