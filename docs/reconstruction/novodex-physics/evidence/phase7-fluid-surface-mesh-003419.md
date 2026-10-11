# Phase 7 implicit fluid surface-mesh dispatch (`phys_fn_003419`)

IDA identifies `phys_fn_003419` at RVA `0x84a50` as the optional implicit-mesh
dispatch. It reads the mesh record from the fluid object at `+0x24`; the mesh
record supplies the fluid pointer at `+0x04`, mesh handle at `+0x18`, and
vertex count at `+0x28`. The callback receives the backend context at
`fluid.backend+0x30`, fluid handle at `fluid+0x80`, mesh handle, and
`vertexCount / 3` triangles. The call is cdecl and its return value is passed
through.

`NxPhysicsFluidEmitterAbiTests` calls the pinned row and map-resolved candidate
helper against the same callback fixture. With context `0x87654321`, fluid
handle `0x12345678`, mesh handle `0x44556677`, and 21 vertices, both sides call
`ImplicitFluidMeshRun(0x87654321, 0x12345678, 0x44556677, 7)`, return the same
value, and preserve the x86 stack. The empty-mesh branch is also checked for no
callback and balanced stack; its oracle EAX value is unspecified and is not
claimed.

The test-first run was red: the oracle made one expected callback while the
candidate row was unresolved. After implementation, the restored direct test
reports:

```text
fluid surface-mesh dispatch cases=1 oracle_callbacks=1 candidate_callbacks=1 args=1 return=1 stack=1/1 mismatches=0
fluid surface-mesh empty cases=1 oracle_callbacks=0 candidate_callbacks=0 stack=1/1 mismatches=0
```

A targeted mutation changes the conversion from `vertexCount / 3` to
`vertexCount / 6`. The direct differential rejects it with `args=0`,
`return=0`, and `mismatches=1`; the empty-mesh case remains unaffected. No
public Physics headers changed.
