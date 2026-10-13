# Contact-pair constructor `phys_fn_000893`

`NxActorPair::row000893` initializes the pair's two shape pointers, body
pointers, embedded friction patch, contact stream, and cached-axis sentinels.
The registered `NxPhysicsSimulationTests` contact/friction scenes reach this
constructor through the pair-node construction path.

For the mutation check, the constructor body was replaced with an immediate
return in a throwaway source edit and `NxPhysics` was rebuilt. The oracle
simulation exited 0; the mutant candidate access-violated before completing
the simulation fixture (`candidate_exit=-1073741819`, `stdout_delta=3983`,
`stderr_exact=True`). The constructor was restored, rebuilt, and rerun: both
processes exited 0 and the staged output matched exactly (`stdout_delta=0`,
`stderr_exact=True`). The mutant and restored candidate Physics DLL hashes
were `cfa4b6863ca39a64d4ca8a3c9276c4a0ad02d007d79cc0b2cdc7cf593e06bf91` and
`a7a8f39510a267f08dfd612e44192261f2da223aa1a91acbb2b01fa30af07d8c`,
respectively; the pinned oracle hash is
`4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

This closes the constructor row's initialized-state behavior exercised by the
fixture. The no-op candidate does not isolate each initialized field, and the
separate `phys_fn_002356` helper remains unresolved; neither point is treated
as evidence that the helper or the entire contact-pair manager is complete.

Raw mutant and restored differential logs, plus the exact source mutation, are
in `evidence/raw/phase7-contact-pair-constructor-000893-20261013/`.

Fresh Win32 Release Phase 7 revalidation passes at 1,744/1,744 coverage assertions. The gate also validates the inventory and both 80-file public-header manifests. Restored candidate DLL SHA-256: `e8480ccdf94c2ff5fbcf75bf7f25fc831e5baef3a97f7feea099f03fb907834f`. Full gate transcript: `evidence/raw/phase7-contact-pair-constructor-000893-20261013/phase7-gate.log.gz`.

Post-merge verification on `main` also passes at 1,744/1,744, with both public-header manifests and inventory validation passing. Mainline-built candidate DLL SHA-256: `b80a1ad8feb6f1036217d9f06b006db30a3680ee26481b388ec8676edd22e2bb`. Transcript: `evidence/raw/phase7-contact-pair-constructor-000893-20261013/phase7-postmerge-gate.log.gz`.
