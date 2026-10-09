# Indirect tail-thunk ABI resolution — 2026-10-09

The three remaining cleanup rows are indirect tail jumps. Their own bytes do
not contain a `ret`; cleanup belongs to the exact callback each row forwards
to. The pinned oracle hash is
`4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

| Row | Exact tail target | ABI proof | Callee cleanup |
|---|---|---|---:|
| `phys_fn_003509`, RVA `0x863f0` | Function pointer at VA `0x10126494`, named `StaticCollisionDestroy` | `GetProcAddress` stores the typed `int (__cdecl *)(void*)` callback; the caller pushes the collision pointer. The differential passes a marked pointer and checks it plus the integer return. | 0 |
| `phys_fn_003936`, RVA `0x8eeb0` | IAT slot `0x10104194`, `??1Observable@NxFoundation@@QAE@XZ` | The row first installs its vtable, then forwards `this` to the zero-explicit-argument Observable destructor. The differential checks the observed `this` pointer. | 0 |
| `phys_fn_004387`, RVA `0xaf2c4` | IAT slot `0x10104198`, `?event@Observable@NxFoundation@@UAEXIAAV12@@Z` | The decorated member signature has two stack parameters: `unsigned` and `Observable&`. The differential checks `this` and both forwarded values. | 8 |

`audit_call_conventions.py` now resolves only these three explicit rows, only
when their complete bytes match, and only for the pinned oracle SHA-256. All
other indirect jumps remain undecidable. The `PhysicsObjectLayoutTests`
differential and the Phase 5/6 gates require all three ABI records.

Validation: the focused scanner suite passes 12 tests, the audit reports 276
resolved cleanup sites and zero mismatches, and the oracle/candidate layout
fixture passes all three marked-forwarding checks.
