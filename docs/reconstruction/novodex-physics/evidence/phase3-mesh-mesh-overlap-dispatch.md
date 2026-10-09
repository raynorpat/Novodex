# Mesh/mesh trigger-overlap dispatch

`ShapePairFunctionTable` left matrix-B slot `[MESH][MESH]` null even though the
pinned oracle constructor assigns RVA `0x00046550` (`phys_fn_001870`). The
reconstructed `nxOverlapMeshMesh` already implements that matrix-B row, but the
product dispatcher could not reach it for mesh/mesh trigger pairs.

Added the product table binding to `nxOverlapMeshMesh` and a constructor check
that compares the candidate slot to the linked implementation pointer. Before
the binding, the pinned collision harness reported the expected pointer versus
null and exited 1. After the binding, it reported equal pointers and
`matrix_wrong=0`; the full pinned collision harness exited 0. The focused
`NxPhysicsCollisionTests` target and the product `NxPhysics` DLL both build in
Win32 Release. Oracle pin: `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

This closes only the matrix-B dispatch binding. Matrix-A mesh/mesh contact
(`phys_fn_001876`) and its contact-generation branches remain open.
