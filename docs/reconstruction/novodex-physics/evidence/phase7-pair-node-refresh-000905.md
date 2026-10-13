# Pair-node per-step refresh `phys_fn_000905`

`NxPairNode::row000905` refreshes the actor-pair state for the current
simulation frame and feeds the contact-report path. The registered public
contact-report fixture checks callback delivery, pair identity, start/touch
events, and force words across fetched simulation steps.

For the mutation check, the row was replaced with a no-op in a throwaway
source edit and `NxPhysics` was rebuilt. The oracle exited 0; the candidate
failed its contact-report fixture (`candidate_exit=1`, `stdout_delta=3985`,
`stderr_exact=False` on the fixture failure path). Restoring the row and
rebuilding returned an exact differential (`both exits=0`, `stdout_delta=0`,
`stderr_exact=True`). The mutant and restored candidate Physics DLL hashes
were `10a172ce7c3f8be7d17dd6436f8430b2feb99274c7d3474bb80b40be06149329` and
`36834e2b38a07193d021482959f3a334a1aa0b31cb62ba7296345f757ebe1e52`;
the pinned oracle hash is
`4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

Raw mutant and restored logs and the source mutation are in
`evidence/raw/phase7-pair-node-refresh-000905-20261013/`.
