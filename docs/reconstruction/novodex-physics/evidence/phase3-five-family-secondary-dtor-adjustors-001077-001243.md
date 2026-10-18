# Five-family secondary deleting-destructor adjustors

IDA confirms that each 8-byte thunk subtracts `0x0c` from `ECX` and jumps to its family scalar deleting-destructor body. Each thunk is referenced by that family's secondary vtable. The candidate emits the corresponding MSVC adjustor from the final collision-object class and `EmbeddedHookBase` multiple-inheritance layout. `NxPhysicsShapeVtableTests` invokes the secondary slot with flags 0 and 1 for scratch and heap objects, checks complete-object return, vptr transition, stack balance, and frees, and exercises a member-only negative control.

| Row | Oracle thunk | Destructor body | Candidate map symbol | Candidate VA |
|---|---:|---:|---|---:|
| `phys_fn_001077` (Box) | `0x000235c0` | `0x000235d0` | `??_ECollisionObjectBox@@WM@AEPAXI@Z` | `0x100464c5` |
| `phys_fn_001125` (Capsule) | `0x00023cf0` | `0x00023d00` | `??_ECollisionObjectCapsule@@WM@AEPAXI@Z` | `0x100464cd` |
| `phys_fn_001161` (Plane) | `0x00024290` | `0x000242a0` | `??_ECollisionObjectPlane@@WM@AEPAXI@Z` | `0x100464dd` |
| `phys_fn_001195` (Sphere) | `0x00024800` | `0x00024810` | `??_ECollisionObjectSphere@@WM@AEPAXI@Z` | `0x100464e5` |
| `phys_fn_001243` (Mesh) | `0x00024e80` | `0x00024e90` | `??_ECollisionObjectMesh@@WM@AEPAXI@Z` | `0x100464d5` |

In the restored current-main run, every family reports `mismatches=0`, the aggregate reports `collision dtor families=5 mismatches=0`, and the member-only negative control reports `mismatches=5`. The test process exits 0 and identifies the candidate sources linked into the harness. Its transcript is archived under [evidence/raw/phase3-box-secondary-dtor-adjustor-001077-20261018](raw/phase3-box-secondary-dtor-adjustor-001077-20261018/).
