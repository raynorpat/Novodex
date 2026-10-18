# Box secondary deleting-destructor adjustor (`phys_fn_001077`)

IDA places an 8-byte adjustor thunk at RVA `0x000235c0`: it subtracts `0x0c` from `ECX` and jumps to the family destructor body at `0x000235d0`. A reference from the secondary vtable reaches this thunk. The candidate emits the corresponding MSVC thunk from the `CollisionObjectBox` / `EmbeddedHookBase` multiple-inheritance layout; the production linker map resolves `??_ECollisionObjectBox@@WM@AEPAXI@Z` in `ObjectModel.obj`.

`NxPhysicsShapeVtableTests` calls the oracle and candidate secondary deleting-destructor slots for flags 0 and 1. Across five families it checks complete-object return pointers, vptr transitions, stack balance, and frees. All five restored family cases match (`mismatches=0`). The member-only negative control omits the `this` adjustment and is rejected for all five families (`mismatches=5`). The current staged run exits 0 and reports `candidate_sources_in_harness=True`.

The refreshed transcript is archived under [evidence/raw/phase3-box-secondary-dtor-adjustor-001077-20261018](raw/phase3-box-secondary-dtor-adjustor-001077-20261018/).
