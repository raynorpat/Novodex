# Pair-list refresh and insertion `phys_fn_000909` / `phys_fn_000911`

`NxPairList::row000909` walks persistent pair nodes and refreshes those with
active bodies. `NxPairList::row000911` orders a broadphase element pair,
honors the SDK group-collision mask, allocates the pair node, and dispatches
its constructor. Both are reached by the registered contact simulations.

The two rows were mutation-tested independently through
`NxPhysicsSimulationTests`:

- Replacing `row000909` with a no-op suppressed per-step refresh and failed
  the candidate contact fixture (`candidate_exit=1`, `stdout_delta=3985`).
- Replacing `row000911` with an immediate-null return prevented pair creation
  and failed the candidate contact fixture (`candidate_exit=1`,
  `stdout_delta=3985`).

Each restored build matched the oracle exactly (`both exits=0`,
`stdout_delta=0`, `stderr_exact=True`). The mutant/restored candidate DLL hashes
were `d4662c9124fd748c28ae83a75aa82f0ee23621955eea65c7c6ee7f80bb5ed534` /
`0cfe0e69f8a5d7715e22100743b6b300deb9fe23519a8e3c91a4e92e0bb99fb1` for
`000909`, and
`fcbf7ac94b639076fa8cb777afce364ff9ef7166b84c33edb7c0b87bc7df1832` /
`16b3773423ee034ea9799b33ca1872d12c6a720d1c1351900b94ecdf225a8257` for
`000911`. The pinned oracle hash is
`4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

Raw mutant and restored logs and both source patches are in
`evidence/raw/phase7-pair-list-refresh-insert-000909-000911-20261013/`.

Post-merge verification on `main` passes Phase 7 at 1,744/1,744, with both public-header manifests and inventory validation passing. Mainline-built candidate DLL SHA-256: `b80a1ad8feb6f1036217d9f06b006db30a3680ee26481b388ec8676edd22e2bb`. Transcript: `evidence/raw/phase7-pair-list-refresh-insert-000909-000911-20261013/phase7-postmerge-gate.log.gz`.
