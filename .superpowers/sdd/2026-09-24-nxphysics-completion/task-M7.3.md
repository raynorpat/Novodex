# M7.3 — implicit fluid surface-mesh dispatch

## Goal

Reconstruct `phys_fn_003419` (RVA `0x84a50`) and add a direct pinned-oracle
differential for its extension-backed mesh callback. Keep public headers
unchanged and retain Phase 8 full-image closure as a separate requirement.

## Contract

- Read the optional mesh record from fluid object `+0x24`.
- If present, read fluid pointer `+0x04`, mesh handle `+0x18`, and vertex count
  `+0x28`; derive triangle count by unsigned division by three.
- Forward backend context, fluid handle, mesh handle, and triangle count to
  `FluidModel.DLL!ImplicitFluidMeshRun` using cdecl, and return the callback
  result.
- If no mesh is present, emit no callback. The caller ignores EAX on that arm;
  the oracle's stale register value is not a stable return contract.

## Verification

Use the existing `NxPhysicsFluidEmitterAbiTests` oracle fixture. Require a
nonzero callback payload and return differential, x86 stack balance, an empty
mesh no-callback case, and a targeted `/3` to `/6` mutation failure. Register
both new coverage lines in Phase 5 and Phase 7 because the shared ABI target is
in both gates. Re-run inventory validation and both phase gates.
