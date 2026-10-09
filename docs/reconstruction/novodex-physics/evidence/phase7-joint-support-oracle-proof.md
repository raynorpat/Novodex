# Phase 7 oracle-only proof: joint support

`NxPhysicsJointSupportTests` now has a machine-readable oracle baseline in
`oracle-only-baselines.json`. It pins the shipped `NxPhysics.dll` SHA-256, the
fixture source SHA-256, two input cases, the input digest, and the oracle
output digest. The verifier intentionally does not use the candidate digest;
the test exit status and the staged-pair differential continue to check the
candidate separately.

The fresh Win32 Phase 7 gate was run from the `codex/nxphysics-oracle-pins`
worktree with:

```powershell
docs/reconstruction/novodex-physics/tools/run_phase_gate.ps1 -Phase 7 -RepoRoot . -BuildRoot build/oracle-proof-phase7-gate -OracleRoot D:\FlamingEnt__\Unreal_3
```

The gate built the registered Phase 7 targets, passed all 1,429 coverage
assertions, and ran the oracle-proof verifier against the pinned DLL. The
retained proof is `oracle-only-proofs/phase7-joint-support.json`; it records
input digest `85a7065a061c36cd`, oracle-output digest `88b713b7bc0870c9`, and
two cases. The proof-lines SHA-256 is
`01d33a9a82e816a3a73afad61606687e7e4bbf4eec8b3e2c8bf79f801a4dd264`, and the
proof record's self-hash is
`fb090027cfde413b546f6ba4671fb43b02b48bd9c0532ea30cea1b831885ebc6`.

The captured build log is retained locally at
`build/oracle-proof-phase7-gate/phase7.log`. This closes the proof-format and
runner-integration slice for one oracle-only target. Other oracle-only targets
still need corresponding records before the M1 checklist item can close.
