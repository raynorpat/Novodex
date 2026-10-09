# Internal call cleanup audit — 2026-10-09

The call-convention scanner now bounds each disassembly to the exact function
size recorded in `inventory.json` and walks reachable control flow rather than
stopping at the first unconditional jump. It joins all reachable return paths
and reports a cleanup only when every reachable `ret` pops the same number of
bytes. Direct jumps are followed only within the row. Absolute indexed switch
tables are accepted only for the observed `cmp index, limit; ja default; jmp
[index*4 + VA]` form, when the complete table is inside a raw PE section and
every table entry targets the same function row.

The pinned oracle used by the scan has SHA-256
`4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.
Command:

```powershell
python docs/reconstruction/novodex-physics/tools/audit_call_conventions.py `
  --repo-root C:\Users\raynorpat\.codex\worktrees\nxphysics-abi-audit\Novodex `
  --oracle-root D:\FlamingEnt__\Unreal_3\Binaries
```

Result: 365 typedefs and 276 cast sites; 273 sites have a decidable row-local
cleanup and zero disagree with their declared calling convention. The other
three are intentionally unresolved indirect tail thunks: `T3509` at RVA
`0x863f0`, `T3936` at RVA `0x8eeb0`, and `T4387` at RVA `0xaf2c4`. Their targets
are selected indirectly, so this scanner does not infer cleanup from nearby
bytes or a guessed callee.

Validation: the focused scanner tests pass (11 tests); the full reconstruction
tooling suite passes (814 tests and 732 subtests); inventory validation passes
with 6,338 functions, 5,138 data objects, and zero unexplained rows; and
`git diff --check` is clean. This establishes stack cleanup only. Preservation
of nonvolatile registers and structure-return behavior remains open under M1.
