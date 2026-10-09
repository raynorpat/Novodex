# Cylindrical joint integration drift

An exploratory public-SDK scene probe found a remaining numerical mismatch in
the cylindrical joint's repeated simulation path. The probe used a zero-gravity
scene, one dynamic sphere attached to the world by a cylindrical joint on the Z
axis, an initial Y velocity of `0.25`, and eight fixed steps of `0.02` seconds.

The oracle and candidate both exited successfully with exact stderr, but the
simulation transcript differed on 24 lines, all from this eight-step probe. At
step 1, the position matched while the Y velocity and angular velocity did not:

| Value | Oracle | Candidate |
|---|---|---|
| position | `41500000.40800000.00000000` | `41500000.40800000.00000000` |
| velocity | `00000000.2998496d.00000000` | `00000000.2998499c.00000000` |
| orientation | `28826827.00000000.00000000.3f7fffff` | `28826850.00000000.00000000.3f7fffff` |
| angular velocity | `29984978.00000000.00000000` | `299849c0.00000000.00000000` |

The focused cylindrical row-record probe matched the oracle in both tested x87
control modes. A matrix run with candidate Physics and oracle Foundation also
matched, while candidate Physics with candidate Foundation reproduced the
drift. This localizes the discrepancy to Physics behavior after the captured
row inputs, but does not yet identify the responsible solver operation.

The temporary probe and its oracle-pinned output literals were removed from the
checked-in simulation test and gate registry after the experiment so they would
not make the normal Phase 5/6 gate fail on a known mismatch. The original
diagnostic is retained in `build/phase6-cylindrical-red.log`. This remains an
open simulation-fidelity item for the joint/simulation completion work; no
expected oracle output was changed.
