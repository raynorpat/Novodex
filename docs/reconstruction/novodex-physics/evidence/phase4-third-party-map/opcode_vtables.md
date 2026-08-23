# OPCODE vtables in `.rdata:0x0011b5a4` - `0x0011bcf8`

Oracle: `D:\FlamingEnt__\Unreal_3\Binaries\NxPhysics.dll`, ImageBase 0x10000000; all addresses are RVAs.
Every class attribution below was verified by locating a constructor or destructor that writes the vtable RVA
into `[this]`, except the four NovodeX pruner tables and the two 1-slot tables, which are identified only by shape.
`PURECALL` marks the `_purecall` thunk at `0x000f41dc`.

Non-pointer words interleaved with the tables: `0x1e3ce508` (= 1e-20f, an un-folded ICE epsilon) appears as
padding between most COMDATs, and `0x0011bccc` holds `0x46fffe00` = 32767.0f, the `(1<<15)-1` quantisation
coefficient consumed by `AABBQuantizedTree::Build` at `0x000f3010`.

## `.rdata:0011b5a4` - Prunable  (IcePrunable.cpp - NOT OPCODE 1.3)

| slot | .rdata | target RVA | size | inferred member |
|---:|---|---|---:|---|
| 0 | `0011b5a4` | `0x000b56d0` | 64 | ~Prunable (scalar deleting) |
| 1 | `0011b5a8` | `0x000b54f0` | 40 | SetFlags(udword) |
| 2 | `0011b5ac` | `0x000b5520` | 45 | ClearFlags(udword) |
| 3 | `0011b5b0` | `0x000b5550` | 25 | ToggleFlags(udword) |
| 4 | `0011b5b4` | `0x000b5570` | 27 | SetOrClearFlags(udword,bool) |
| 5 | `0011b5b8` | `0x0000dee0` | 5 | (empty hook, ICF-folded stub) |

## `.rdata:0011b628` - RayCollider : Collider  (OPC_RayCollider.h)

| slot | .rdata | target RVA | size | inferred member |
|---:|---|---|---:|---|
| 0 | `0011b628` | `0x000ba6c0` | 36 | ~RayCollider |
| 1 | `0011b62c` | `0x000b5770` | 91 | ValidateSettings |
| 2 | `0011b630` | `0x000b5710` | 5 | InitQuery (inherited Collider::InitQuery) |

## `.rdata:0011b750` - AABBTreeCollider : Collider  (OPC_TreeCollider.h)

| slot | .rdata | target RVA | size | inferred member |
|---:|---|---|---:|---|
| 0 | `0011b750` | `0x000d0930` | 46 | ~AABBTreeCollider |
| 1 | `0011b754` | `0x000bb570` | 20 | ValidateSettings |
| 2 | `0011b758` | `0x000b5710` | 5 | InitQuery (inherited Collider::InitQuery) |

## `.rdata:0011b764` - LSSCollider : VolumeCollider  (OPC_LSSCollider.h)

| slot | .rdata | target RVA | size | inferred member |
|---:|---|---|---:|---|
| 0 | `0011b764` | `0x000d4b60` | 36 | ~LSSCollider |
| 1 | `0011b768` | `0x000213e0` | 3 | ValidateSettings (inherited VolumeCollider::) |
| 2 | `0011b76c` | `0x000d14a0` | 13 | InitQuery (inherited VolumeCollider::) |

## `.rdata:0011b774` - OBBCollider : VolumeCollider  (OPC_OBBCollider.h)

| slot | .rdata | target RVA | size | inferred member |
|---:|---|---|---:|---|
| 0 | `0011b774` | `0x000de0a0` | 36 | ~OBBCollider |
| 1 | `0011b778` | `0x000e1530` | 22 | ValidateSettings |
| 2 | `0011b77c` | `0x000d14a0` | 13 | InitQuery (inherited VolumeCollider::) |

## `.rdata:0011b784` - SphereCollider : VolumeCollider  (OPC_SphereCollider.h)

| slot | .rdata | target RVA | size | inferred member |
|---:|---|---|---:|---|
| 0 | `0011b784` | `0x000e1330` | 36 | ~SphereCollider |
| 1 | `0011b788` | `0x000213e0` | 3 | ValidateSettings (inherited VolumeCollider::) |
| 2 | `0011b78c` | `0x000d14a0` | 13 | InitQuery (inherited VolumeCollider::) |

## `.rdata:0011b794` - PlanesCollider : VolumeCollider  (OPC_PlanesCollider.h)

| slot | .rdata | target RVA | size | inferred member |
|---:|---|---|---:|---|
| 0 | `0011b794` | `0x000e2ce0` | 64 | ~PlanesCollider |
| 1 | `0011b798` | `0x000e1530` | 22 | ValidateSettings (ICF-folded with OBBCollider::ValidateSettings) |
| 2 | `0011b79c` | `0x000d14a0` | 13 | InitQuery (inherited VolumeCollider::) |

## `.rdata:0011b9c8` - NovodeX pruner, derived  - NOT an OPCODE class

| slot | .rdata | target RVA | size | inferred member |
|---:|---|---|---:|---|
| 0 | `0011b9c8` | `0x000e5810` | 86 | dtor |
| 1 | `0011b9cc` | `0x000e5210` | 56 | slot1 (query/update) |
| 2 | `0011b9d0` | `0x000e5250` | 65 | slot2 (query/update) |
| 3 | `0011b9d4` | `0x000e52a0` | 48 | slot3 (query/update) |
| 4 | `0011b9d8` | `0x000e5100` | 8 | slot4 (query/update) |
| 5 | `0011b9dc` | `0x000e5780` | 141 | slot5 (query/update) |
| 6 | `0011b9e0` | `0x000e5440` | 184 | slot6 (query/update) |
| 7 | `0011b9e4` | `0x000e5590` | 320 | slot7 (query/update) |
| 8 | `0011b9e8` | `0x000e5500` | 133 | slot8 (query/update) |

## `.rdata:0011b9f0` - NovodeX pruner, derived  - NOT an OPCODE class

| slot | .rdata | target RVA | size | inferred member |
|---:|---|---|---:|---|
| 0 | `0011b9f0` | `0x000e6430` | 78 | dtor |
| 1 | `0011b9f4` | `0x000e5a00` | 40 | slot1 (query/update) |
| 2 | `0011b9f8` | `0x000e5a30` | 93 | slot2 (query/update) |
| 3 | `0011b9fc` | `0x000e5a90` | 28 | slot3 (query/update) |
| 4 | `0011ba00` | `0x000e5890` | 3 | slot4 (query/update) |
| 5 | `0011ba04` | `0x000e5ab0` | 807 | slot5 (query/update) |
| 6 | `0011ba08` | `0x000e5de0` | 633 | slot6 (query/update) |
| 7 | `0011ba0c` | `0x000e6230` | 512 | slot7 (query/update) |
| 8 | `0011ba10` | `0x000e6060` | 202 | slot8 (query/update) |

## `.rdata:0011ba1c` - NovodeX 1-virtual class (destructor only)

| slot | .rdata | target RVA | size | inferred member |
|---:|---|---|---:|---|
| 0 | `0011ba1c` | `0x000e7670` | 31 | dtor |

## `.rdata:0011ba40` - NovodeX 1-virtual class (destructor only)

| slot | .rdata | target RVA | size | inferred member |
|---:|---|---|---:|---|
| 0 | `0011ba40` | `0x000e82c0` | 31 | dtor |

## `.rdata:0011bab4` - AABBTreeOfTrianglesBuilder : AABBTreeBuilder  (OPC_TreeBuilders.h:158)

| slot | .rdata | target RVA | size | inferred member |
|---:|---|---|---:|---|
| 0 | `0011bab4` | `0x000538b0` | 35 | ~AABBTreeBuilder (ICF fold - see notes) |
| 1 | `0011bab8` | `0x000e96b0` | 569 | ComputeGlobalBox |
| 2 | `0011babc` | `0x000e9940` | 345 | GetSplittingValue(const udword*,udword,const AABB&,udword) |
| 3 | `0011bac0` | `0x000e98f0` | 79 | GetSplittingValue(udword,udword) |
| 4 | `0011bac4` | `0x000538a0` | 14 | ValidateSubdivision (base impl, not overridden) |

## `.rdata:0011bac8` - Model : BaseModel  (OPC_Model.h:23)

| slot | .rdata | target RVA | size | inferred member |
|---:|---|---|---:|---|
| 0 | `0011bac8` | `0x000e9280` | 47 | ~Model |
| 1 | `0011bacc` | `0x000e9100` | 375 | Build |
| 2 | `0011bad0` | `0x000e90c0` | 18 | GetUsedBytes |
| 3 | `0011bad4` | `0x000e9410` | 15 | Refit (inherited BaseModel::Refit) |
| 4 | `0011bad8` | `0x000e9420` | 19 | [NovodeX] getSerialSize |
| 5 | `0011badc` | `0x000e9440` | 53 | [NovodeX] save(stream) |
| 6 | `0011bae0` | `0x000e94c0` | 133 | [NovodeX] load(stream) |

## `.rdata:0011bb5c` - BaseModel, abstract  (OPC_BaseModel.h:50)

| slot | .rdata | target RVA | size | inferred member |
|---:|---|---|---:|---|
| 0 | `0011bb5c` | `0x000e95a0` | 91 | ~BaseModel |
| 1 | `0011bb60` | `0x000f41dc` PURECALL | 20 | Build = 0 |
| 2 | `0011bb64` | `0x000f41dc` PURECALL | 20 | GetUsedBytes = 0 |
| 3 | `0011bb68` | `0x000e9410` | 15 | Refit |
| 4 | `0011bb6c` | `0x000e9420` | 19 | [NovodeX] getSerialSize |
| 5 | `0011bb70` | `0x000e9440` | 53 | [NovodeX] save(stream) |
| 6 | `0011bb74` | `0x000e94c0` | 133 | [NovodeX] load(stream) |

## `.rdata:0011bb84` - AABBCollider : VolumeCollider  (OPC_AABBCollider.h)

| slot | .rdata | target RVA | size | inferred member |
|---:|---|---|---:|---|
| 0 | `0011bb84` | `0x000ef0a0` | 36 | ~AABBCollider |
| 1 | `0011bb88` | `0x000213e0` | 3 | ValidateSettings (inherited VolumeCollider::) |
| 2 | `0011bb8c` | `0x000d14a0` | 13 | InitQuery (inherited VolumeCollider::) |

## `.rdata:0011bb98` - NovodeX pruner, derived  - NOT an OPCODE class

| slot | .rdata | target RVA | size | inferred member |
|---:|---|---|---:|---|
| 0 | `0011bb98` | `0x000ef750` | 53 | dtor |
| 1 | `0011bb9c` | `0x000e5210` | 56 | slot1 (query/update) |
| 2 | `0011bba0` | `0x000e5250` | 65 | slot2 (query/update) |
| 3 | `0011bba4` | `0x000ef690` | 50 | slot3 (query/update) |
| 4 | `0011bba8` | `0x000e5100` | 8 | slot4 (query/update) |
| 5 | `0011bbac` | `0x000ef790` | 44 | slot5 (query/update) |
| 6 | `0011bbb0` | `0x000ef7c0` | 44 | slot6 (query/update) |
| 7 | `0011bbb4` | `0x000ef820` | 39 | slot7 (query/update) |
| 8 | `0011bbb8` | `0x000ef7f0` | 39 | slot8 (query/update) |

## `.rdata:0011bbc0` - NovodeX pruner, derived  - NOT an OPCODE class

| slot | .rdata | target RVA | size | inferred member |
|---:|---|---|---:|---|
| 0 | `0011bbc0` | `0x000efe80` | 40 | dtor |
| 1 | `0011bbc4` | `0x000e50c0` | 8 | slot1 (query/update) |
| 2 | `0011bbc8` | `0x000e50d0` | 21 | slot2 (query/update) |
| 3 | `0011bbcc` | `0x000e50f0` | 8 | slot3 (query/update) |
| 4 | `0011bbd0` | `0x000e5890` | 3 | slot4 (query/update) |
| 5 | `0011bbd4` | `0x000ef870` | 536 | slot5 (query/update) |
| 6 | `0011bbd8` | `0x000efa90` | 503 | slot6 (query/update) |
| 7 | `0011bbdc` | `0x000efd60` | 273 | slot7 (query/update) |
| 8 | `0011bbe0` | `0x000efc90` | 208 | slot8 (query/update) |

## `.rdata:0011bbec` - Collider, abstract  (OPC_Collider.h:37)

| slot | .rdata | target RVA | size | inferred member |
|---:|---|---|---:|---|
| 0 | `0011bbec` | `0x000f0540` | 31 | ~Collider |
| 1 | `0011bbf0` | `0x000f41dc` PURECALL | 20 | ValidateSettings = 0 |
| 2 | `0011bbf4` | `0x000b5710` | 5 | InitQuery |

## `.rdata:0011bc00` - VolumeCollider, abstract  (OPC_VolumeCollider.h:32)

| slot | .rdata | target RVA | size | inferred member |
|---:|---|---|---:|---|
| 0 | `0011bc00` | `0x000f41dc` PURECALL | 20 | ~VolumeCollider = 0 (pure virtual destructor) |
| 1 | `0011bc04` | `0x000213e0` | 3 | ValidateSettings |
| 2 | `0011bc08` | `0x000d14a0` | 13 | InitQuery |

## `.rdata:0011bc18` - NovodeX pruner base  - NOT an OPCODE class

| slot | .rdata | target RVA | size | inferred member |
|---:|---|---|---:|---|
| 0 | `0011bc18` | `0x000f15c0` | 51 | dtor |
| 1 | `0011bc1c` | `0x000e50c0` | 8 | slot1 (query/update) |
| 2 | `0011bc20` | `0x000e50d0` | 21 | slot2 (query/update) |
| 3 | `0011bc24` | `0x000e50f0` | 8 | slot3 (query/update) |
| 4 | `0011bc28` | `0x000e5890` | 3 | slot4 (query/update) |
| 5 | `0011bc2c` | `0x000f1580` | 5 | slot5 (query/update) |
| 6 | `0011bc30` | `0x000f1580` | 5 | slot6 (query/update) |
| 7 | `0011bc34` | `0x000f1590` | 5 | slot7 (query/update) |
| 8 | `0011bc38` | `0x000f1590` | 5 | slot8 (query/update) |

## `.rdata:0011bc4c` - AABBOptimizedTree, abstract  (OPC_OptimizedTree.h:133)

| slot | .rdata | target RVA | size | inferred member |
|---:|---|---|---:|---|
| 0 | `0011bc4c` | `0x000f21a0` | 35 | ~AABBOptimizedTree |
| 1 | `0011bc50` | `0x000f41dc` PURECALL | 20 | Build = 0 |
| 2 | `0011bc54` | `0x000f41dc` PURECALL | 20 | Refit = 0 |
| 3 | `0011bc58` | `0x000f41dc` PURECALL | 20 | Walk = 0 |
| 4 | `0011bc5c` | `0x000f41dc` PURECALL | 20 | [NovodeX] getSerialSize = 0 |
| 5 | `0011bc60` | `0x000f41dc` PURECALL | 20 | [NovodeX] save = 0 |
| 6 | `0011bc64` | `0x000f41dc` PURECALL | 20 | [NovodeX] load = 0 |
| 7 | `0011bc68` | `0x000f41dc` PURECALL | 20 | GetUsedBytes = 0 |

## `.rdata:0011bc6c` - AABBCollisionTree : AABBOptimizedTree  (node = 28 bytes)

| slot | .rdata | target RVA | size | inferred member |
|---:|---|---|---:|---|
| 0 | `0011bc6c` | `0x000f3fb0` | 73 | ~AABBCollisionTree |
| 1 | `0011bc70` | `0x000f2480` | 171 | Build |
| 2 | `0011bc74` | `0x000f3cd0` | 5 | Refit |
| 3 | `0011bc78` | `0x000f3ef0` | 36 | Walk |
| 4 | `0011bc7c` | `0x000f2570` | 10 | [NovodeX] getSerialSize |
| 5 | `0011bc80` | `0x000f2580` | 43 | [NovodeX] save |
| 6 | `0011bc84` | `0x000f25b0` | 168 | [NovodeX] load |
| 7 | `0011bc88` | `0x000f2470` | 7 | GetUsedBytes |

## `.rdata:0011bc8c` - AABBNoLeafTree : AABBOptimizedTree  (node = 32 bytes)

| slot | .rdata | target RVA | size | inferred member |
|---:|---|---|---:|---|
| 0 | `0011bc8c` | `0x000f4000` | 73 | ~AABBNoLeafTree |
| 1 | `0011bc90` | `0x000f26c0` | 174 | Build |
| 2 | `0011bc94` | `0x000f2770` | 73 | Refit |
| 3 | `0011bc98` | `0x000f3f20` | 36 | Walk |
| 4 | `0011bc9c` | `0x000f2eb0` | 10 | [NovodeX] getSerialSize |
| 5 | `0011bca0` | `0x000f2ec0` | 43 | [NovodeX] save |
| 6 | `0011bca4` | `0x000f2ef0` | 168 | [NovodeX] load |
| 7 | `0011bca8` | `0x000f26b0` | 7 | GetUsedBytes |

## `.rdata:0011bcac` - AABBQuantizedTree : AABBOptimizedTree  (node = 16 bytes)

| slot | .rdata | target RVA | size | inferred member |
|---:|---|---|---:|---|
| 0 | `0011bcac` | `0x000f4050` | 73 | ~AABBQuantizedTree |
| 1 | `0011bcb0` | `0x000f3010` | 1197 | Build |
| 2 | `0011bcb4` | `0x000f3cd0` | 5 | Refit |
| 3 | `0011bcb8` | `0x000f3f50` | 36 | Walk |
| 4 | `0011bcbc` | `0x000f3620` | 10 | [NovodeX] getSerialSize |
| 5 | `0011bcc0` | `0x000f3630` | 117 | [NovodeX] save |
| 6 | `0011bcc4` | `0x0000dee0` | 5 | [NovodeX] load (folded 'return true' stub) |
| 7 | `0011bcc8` | `0x000f3000` | 7 | GetUsedBytes |

## `.rdata:0011bcd0` - AABBQuantizedNoLeafTree : AABBOptimizedTree  (node = 20 bytes)

| slot | .rdata | target RVA | size | inferred member |
|---:|---|---|---:|---|
| 0 | `0011bcd0` | `0x000f40a0` | 73 | ~AABBQuantizedNoLeafTree |
| 1 | `0011bcd4` | `0x000f36e0` | 1177 | Build |
| 2 | `0011bcd8` | `0x000f3cd0` | 5 | Refit |
| 3 | `0011bcdc` | `0x000f3f80` | 36 | Walk |
| 4 | `0011bce0` | `0x000f3d30` | 14 | [NovodeX] getSerialSize |
| 5 | `0011bce4` | `0x000f3d40` | 118 | [NovodeX] save |
| 6 | `0011bce8` | `0x000f3dc0` | 167 | [NovodeX] load |
| 7 | `0011bcec` | `0x000f36d0` | 10 | GetUsedBytes |

## Notes

* **`0x0011bab4` slots 0 and 4 point outside the OPCODE span, but they are OPCODE code, not NovodeX code.**
  `0x000538b0` installs vtable `0x0010833c`, which is `AABBTreeBuilder`'s own vtable:
  `[0]=0x000538b0` dtor, `[1]=PURECALL` ComputeGlobalBox, `[2]=0x00053880` GetSplittingValue(4-arg, base impl),
  `[3]=PURECALL` GetSplittingValue(2-arg), `[4]=0x000538a0` ValidateSubdivision. A byte scan of the whole image
  finds exactly one reference to `0x0010833c`, at `0x000538ba`, i.e. inside `0x000538b0` itself.
  `0x000538a0` is byte-exact `AABBTreeBuilder::ValidateSubdivision` (`return mSettings.mLimit < nb_prims`,
  `mLimit` at `this+4`, `ret 0xc` for three stack arguments). Because
  `AABBTreeOfTrianglesBuilder::~AABBTreeOfTrianglesBuilder` is empty, dead-store elimination removes the derived
  vptr store and the deleting destructor becomes byte-identical to the base's, so `/OPT:ICF` folded them.
  Folding is therefore the right explanation, but the surviving copy is OPCODE's own base-class destructor.

* `0x00108350` is a second builder vtable, also outside the pool:
  `[0]=0x000538b0` (same folded dtor), `[1]=0x000e9600` `AABBTreeOfAABBsBuilder::ComputeGlobalBox`
  (OPC_TreeBuilders.cpp:66), `[2]=0x00053880` (base 4-arg splitter, not overridden),
  `[3]=0x000e9680` `AABBTreeOfAABBsBuilder::GetSplittingValue(udword,udword)` (:91).

* MSVC lays out **overloaded** virtual functions in reverse declaration order, which is why the 4-argument
  `GetSplittingValue` occupies slot 2 and the 2-argument one slot 3. Independently confirmed by
  `AABBTreeNode::Split` at `0x000f0930`, which calls `builder->vtbl[8]` once per node and `builder->vtbl[0xc]`
  once per primitive - exactly matching `GetSplittingValue(primitives,nb,box,axis)` then
  `GetSplittingValue(index,axis)` in OPC_AABBTree.cpp:99.

* `HybridModel` has no vtable anywhere in the image. `BaseModel` (`0x0011bb5c`) and `Model` (`0x0011bac8`) are
  the only model vtables, confirming `OPC_HybridModel.cpp` was not compiled in.

* The four 9-slot tables `0x0011b9c8`, `0x0011b9f0`, `0x0011bb98`, `0x0011bbc0` plus their base `0x0011bc18`
  belong to a NovodeX pruner class family, not to OPCODE: no OPCODE 1.3 class declares nine virtuals, and their
  slot-0 destructors chain into NovodeX container code while their methods call the `IcePrunable.cpp` base at
  `0x000b55b0`.

* `0x0011b764` vs `0x0011b784`: both are VolumeCollider subclasses that do not override ValidateSettings, so
  they had to be told apart by member layout. `0x000de7e0` (the `0x0011b784` constructor) zeroes exactly four
  consecutive dwords at `this+0x34..+0x40`, and the inlined overlap test in its leaf `_Collide` bodies forms
  `(mCenter[i] - center[i])` from `+0x34/+0x38/+0x3c` and compares the accumulated squared clearance against
  `+0x40` - byte-exact `SphereCollider::SphereAABBOverlap` over `Point mCenter; float mRadius2;`.
  `0x0011b764`'s constructor initialises nothing (matching `LSSCollider::LSSCollider()`, whose two
  initialisers are commented out in the source) and its leaf `_Collide` bodies read a fat centre/extents/radius
  block at `+0x58..+0x70`. `0x0011b764` is therefore LSSCollider and `0x0011b784` SphereCollider.
