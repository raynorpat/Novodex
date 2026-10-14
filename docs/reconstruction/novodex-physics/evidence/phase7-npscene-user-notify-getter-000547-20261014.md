# Phase 7: Scene user-notify getter (`phys_fn_000547`)

`phys_fn_000547` is the 7-byte field getter for `Scene+0x6ac`. The private
`NxSceneInternal::getUserNotify` method now implements that row directly and is
kept out of line. `NpScene::getUserNotify` takes the wrapper's read lock at
`this+0x10`, calls the helper on the Scene at `this+0x24`, and releases the
same lock before returning. The public Physics headers are unchanged.

The registered `NxPhysicsSimulationTests` joint-break fixture checks that the
public getter returns the descriptor's null default and then returns the exact
callback pointer installed with `setUserNotify`. The baseline candidate matched
the pinned DLL before routing through the explicit helper; the restored helper
and locked wrapper also match exactly (`oracle_exit=0`, `candidate_exit=0`,
`stdout_delta=0`, `stderr_exact=True`).

A row-specific mutant changed `NxSceneInternal::getUserNotify` to return null.
The same differential rejected it at the setter/getter assertion
(`candidate_exit=1`, `stdout_delta=831`; oracle exit 0). The mutant
`NxPhysics.dll` SHA-256 was
`07d2380800e9d241a4bd45800eb9518289fa2c377571918bc383fe6fe947fb46`; the
restored candidate SHA-256 is
`1d0d58b02c944296e6ac08959b45b7fb291ee292b867470f3801b2be0deb8c3b`.

The oracle Physics image is pinned at
`4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.
Full restored and mutation transcripts, with staged module identities, are
archived under
[`raw/phase7-npscene-user-notify-getter-000547-20261014/`](raw/phase7-npscene-user-notify-getter-000547-20261014/).
