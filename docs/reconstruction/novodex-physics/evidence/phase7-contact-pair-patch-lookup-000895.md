# Contact-pair friction-patch lookup `phys_fn_000895`

`NxActorPair::row000895` transforms the contact normal into body-local space,
searches the embedded or allocated friction patches for a matching material,
shape pair, and normal, and otherwise returns a newly initialized patch. The
registered sliding-contact simulation reaches the lookup through
`NxActorPair::row000897`.

For the mutation check, the function was replaced with an immediate-null return
in a throwaway source edit and `NxPhysics` was rebuilt. The oracle simulation
exited 0; the candidate access-violated when contact processing used the
missing patch (`candidate_exit=-1073741819`, `stdout_delta=3887`,
`stderr_exact=True`). Restoring the function and rebuilding returned an exact
staged differential (`both exits=0`, `stdout_delta=0`, `stderr_exact=True`).
The mutant and restored candidate Physics DLL hashes were
`99a9a8e1174b21f81d5ba56db6b2e3cec68d8fe2ec7506e552941012b372258f` and
`28dc72d32e6880f7eac6aa61f53b044a82233c136b707b2bc1621dfa0a47a8b0`;
the pinned oracle hash is
`4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

This proves the lookup's required first embedded-patch path for the registered
contact fixture. Matching multiple patches, allocation failure, and dynamic
array growth remain open behavior for the terminal full-image audit.

Raw mutant and restored differential logs and the source mutation are in
`evidence/raw/phase7-contact-pair-patch-lookup-000895-20261013/`.

Post-merge verification on `main` passes Phase 7 at 1,744/1,744, with both public-header manifests and inventory validation passing. Mainline-built candidate DLL SHA-256: `b80a1ad8feb6f1036217d9f06b006db30a3680ee26481b388ec8676edd22e2bb`. Transcript: `evidence/raw/phase7-contact-pair-patch-lookup-000895-20261013/phase7-postmerge-gate.log.gz`.
