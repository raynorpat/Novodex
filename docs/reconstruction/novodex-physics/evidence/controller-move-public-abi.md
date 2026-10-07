# Controller move public ABI

The installed NxCharacter declaration in
`Development/External/PhysX/SDKs/NxCharacter/Include/NxController.h` defines
`NxController::move` with six parameters: displacement, active groups, minimum
distance, collision flags, sharpness, and an optional groups mask. The private
test probe and reconstructed controller vtable method now use that signature;
the public Physics headers in this repository remain untouched.

The simulation fixtures use the SDK defaults (`sharpness = 1.0`, null groups
mask). The reconstructed method currently accepts and ignores those final two
arguments, so this closes the call ABI only. Sharpness smoothing and groups-mask
filtering remain semantic work and are not claimed as reconstructed.

Verification on the rebuilt Release DLL:

- `NxPhysicsSimulationTests`: oracle and candidate exit 0 with identical
  stdout and stderr.
- Direct Phase 5 differential: all 14 registered targets match.
- Direct Phase 7 differential: all 11 registered targets match.
- `ctest --test-dir build -C Release -R '^Viewer' --output-on-failure`: all
  48 selections pass, representing all 39 scenes; 43 pass and five existing
  pinned-oracle asset cases skip by signature.
- The `run_phase_gate.ps1` wrapper stops during inventory preflight in this
  worktree because the pinned upstream source trees are not staged under
  `.analysis/novodex-physics/thirdparty`. The direct differential runs above
  did execute successfully.

Successful step-over, non-default sharpness, non-null groups masks, and the
remaining controller semantics are still open.
