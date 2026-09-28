## Convex cooking: the NovodeX hull library around qhull

Recovered by qhull-gap Task 3 (plan: `docs/superpowers/plans/2026-09-28-qhull-gap.md`; timing in
`evidence/qhull-gap.md`). It uses the conventions of `units/revolute-contract.md` and
`units/joint-families-contract.md`:

- Addresses are RVAs (image base 0x10000000).
- Rows are named by stable ID.
- The Capstone listing (`oracle/capstone/manifest.json`) is authoritative over the Ghidra
  decompiles.
- A change to a layout, a name or a row list is made here, in the same commit.

The bundles for the rows named here are `units/NpPhysicsSDK.cpp.md`, `units/PhysicsSDK.cpp.md`,
`units/TriangleMesh.cpp.md`, `units/gap__Controller.cpp__to__fluids__Fluid.cpp.md` (the 37
hull-library rows), `units/InternalTriangleMesh.cpp.md`, `units/NpTriangleMesh.cpp.md`,
`units/gap__NpTriangleMesh.cpp__to__TriangleMesh.cpp.md`, `units/ConvexHull.cpp.md` and
`units/gap__ConvexHull.cpp__to__IceAdjacencies.cpp.md`. The first four are the chain. The last
five are the deferred post-hull code.

### What it is

The 37 unmapped rows in the qhull address span, plus their caller `phys_fn_002233`, are a
complete hull library. Its interface is the one of Stan Melax's and John Ratcliff's
`HullLibrary`: a descriptor, a result, `CreateConvexHull` and `ReleaseResult`. The hull core,
though, is qhull, run through the `"o"` output format and a host object, not Melax's own
algorithm.

The identification rests on these measurements:

- **The descriptor and result shapes.** `002233` builds a seven-dword descriptor (flags, count,
  pointer, stride, epsilon, vertex limit, float) and a seven-dword result, calls
  `phys_fn_003279(desc, result)` (thiscall, `ret 8`) and checks for a zero return. It then calls
  `phys_fn_003255(result)`, which frees the result's two arrays (0x0007e300-0x0007e360).
- **The 16-byte `"JOHNRAT\0"` header** that `003274` writes in front of every tracked block
  (0x0007e8f8-0x0007e913).
- **The vertex clean-up (`003243`).** It builds a bounding box, normalises by the extents, welds
  duplicates within the epsilon, and falls back to an 8-corner box for degenerate input. Its
  argument list is `CleanupVertices`'.
- **Band B is Xiaolin Wu's colour quantizer.** The moment arrays are 33x33x33 (index arithmetic
  `*0x21`, 0x00080400). There are five of them, 0x23184 bytes apart (`lea` at 0x00080c73-0x00080c88), and the table is
  0xaf794 = 718,740 = 35,937 x 20 bytes (`push 0xaf794` at 0x00081178). A 256-entry square table sits at
  `.data:0x00125088`. The quantizer reduces the point cloud to the vertex limit.

The class and file names in this contract are descriptive. **No original identifier is
evidenced.** The image carries no symbol for any of these rows, and the only strings are the
two OBJ file names and three OBJ format lines.

### Row assignment

**The public entry.** The chain is `000242 -> 000478 -> 002251/002260 -> 002233`:

| Row | RVA | Size | State | Phase | Unit (work_units.json) | What it is |
|---|---|---:|---|---:|---|---|
| `phys_fn_000242` | 0x0000b7e0 | 219 | discovered | 2 | NpPhysicsSDK.cpp | `NpPhysicsSDK::createTriangleMesh(const NxTriangleMeshDesc&)`, slot 8 of the NpPhysicsSDK vtable |
| `phys_fn_000478` | 0x0000ebe0 | 470 | discovered | 2 | PhysicsSDK.cpp | `PhysicsSDK::createTriangleMesh`: an inline `isValid`, `new TriangleMesh`, `loadFromDesc`, and the append to `mTriangleMeshes` |
| `phys_fn_002251` | 0x00055490 | 216 | discovered | 4 | TriangleMesh.cpp | the `TriangleMesh` constructor (0xe8 bytes) |
| `phys_fn_002260` | 0x00055890 | 1,044 | discovered | 4 | TriangleMesh.cpp | `TriangleMesh::loadFromDesc`, slot 2 of the vtable at `.rdata:0x00108608` |
| `phys_fn_002233` | 0x00054920 | 277 | discovered | 4 | TriangleMesh.cpp | the hull computation from `NX_MF_COMPUTE_CONVEX`: descriptor in, hull triangle descriptor out |

The public API is `NxPhysicsSDK::createTriangleMesh` (`Physics/include/NxPhysicsSDK.h:188`, the
ninth virtual). The evidence:

- The NpPhysicsSDK vtable is `.rdata:0x00105860`. The constructor stores it at 0x0000b5d6 and
  the destructor at 0x0000c2b6.
- Its slot 8 (`.rdata:0x00105880`) is 0x1000b7e0.
- Slot 2 is `phys_fn_000230` (0x0000b600, `(int, float)`, i.e. `setParameter`), which pins the
  slot order to the header's.
- The `.rdata:0x00105800` table that `phys_fn_000224` (0x0000b5b0) stores is the abstract
  `NxPhysicsSDK` vtable: `_purecall` 0x000f41dc in slots 1-23. The dependency edges
  `000224 -> 000242` come from that data, not from a call.

The desc must be an `NxTriangleMeshDesc` with `flags & NX_MF_CONVEX` (4) and
`flags & NX_MF_COMPUTE_CONVEX` (8) (`Foundation/include/NxSimpleTriangleMesh.h:28-29`).
`triangles` may be NULL; `isValid` requires `NX_MF_COMPUTE_CONVEX` in that case (`:135-139`,
inlined at 0x0000ecb1-0x0000ecbc and 0x00055aeb-0x00055af8). This is the only path into the
qhull span: `002260` calls `002233` only when `flags & 8` (`test al,8` at 0x00055942).

`000242` walks the scenes' writer locks (`002364`/`002366`, the deadlock report at
NpPhysicsSDK.cpp:0x6f). It returns `TriangleMesh+0xe4` (`piVar5[0x39]`), which is the
`NpTriangleMesh` public wrapper. `002251` allocates that wrapper (8 bytes, constructor
`phys_fn_002140`, vtable `.rdata:0x0010829c`) at 0x00055526-0x0005555d.

**The hull library: 37 rows, 12,166 B, 3,903 instructions, 750 of them x87.**

- 31 rows are `discovered`: 11,641 B, 3,737 instructions, all 750 x87 instructions.
- 6 rows are `reconstructed` as generic shapes in `Physics/src/ObjectModel.cpp`.
- Every row is `unmapped` in `phase4-third-party-map/qhull_map.csv` and sits in unit
  `gap:Controller.cpp..fluids\Fluid.cpp`.
- The instruction counts are from the Capstone listing. `vendored-correspondence.md` gives
  "31 discovered: 3,903 instructions", but 3,903 is the count for all 37 rows.

Band A (0x0007d420-0x0007ed43) links between `qhull.c` (`qh_qhull` 0x0007d180) and `qset.c`
(`qh_setdel` 0x0007ed50). Band B (0x0007fda0-0x000814f0) links between `qset.c`
(`qh_settempfree` 0x0007fd20) and `stat.c` (`qh_allstatA` 0x000814f0). The qhull objects are
linked in alphabetical order (geom.c ... user.c). So if the two NovodeX objects were linked the
same way, band A's file name sorts after `qhull.c` and before `qset.c`, and band B's after
`qset.c` and before `stat.c`. That is a naming constraint only, not evidence of a name. Both
bands are C++: they use `__thiscall` and a vtable.

| Row | RVA | Size | State | Insns / x87 | Role (descriptive name) |
|---|---|---:|---|---|---|
| `003236` | 0x0007d420 | 214 | discovered | 73 / 10 | `runQhull(argc, argv, n, const float*)`: cdecl, returns 0 |
| `003238` | 0x0007d500 | 106 | reconstructed | 41 / 0 | `QhullHost::releaseArrays()` |
| `003240` | 0x0007d570 | 32 | discovered | 11 / 0 | `QhullHost::rawAlloc(size)`: user allocator slot 0, else CRT `malloc` |
| `003241` | 0x0007d590 | 31 | discovered | 11 / 0 | `QhullHost::rawFree(p)`: user allocator slot 1, else CRT `free` |
| `003243` | 0x0007d5b0 | 933 | discovered | 274 / 117 | `QhullHost::cleanupVertices(...)`: entry block |
| `003245` | 0x0007d960 | 1,331 | discovered | 427 / 213 | `cleanupVertices`, continuation block (no callers) |
| `003247` | 0x0007dea0 | 121 | discovered | 41 / 0 | `writeOkObj(const HullResult&)` (`QHULL_OK_%04d.obj`): entry |
| `003249` | 0x0007df20 | 299 | discovered | 107 / 6 | `writeOkObj`, continuation |
| `003251` | 0x0007e050 | 158 | discovered | 58 / 6 | `writeFailObj(n, const float*, stride)` (`QHULL_FAIL_%04d.obj`) |
| `003253` | 0x0007e0f0 | 526 | discovered | 173 / 69 | `boxFallback(unsigned& n, float* v)`: 8 corners of centre +- extent |
| `003255` | 0x0007e300 | 99 | discovered | 38 / 0 | `HullLibrary::ReleaseResult(HullResult&)`: `ret 4`, returns 0 |
| `003257` | 0x0007e370 | 95 | reconstructed | 27 / 0 | `QhullHost::QhullHost(Allocator*)` |
| `003259` | 0x0007e3d0 | 210 | discovered | 74 / 0 | vtable +0x00 `offBegin(dim, numpoints, numfacets, numridges)` |
| `003261` | 0x0007e4b0 | 43 | reconstructed | 14 / 0 | vtable +0x04 `point3(x, y, z)` |
| `003263` | 0x0007e4e0 | 60 | discovered | 16 / 0 | vtable +0x10 `fprintf(FILE*, fmt, ...)`: formats, then `errexit(1)` |
| `003265` | 0x0007e520 | 23 | reconstructed | 5 / 0 | vtable +0x0c `size(area, volume)` |
| `003267` | 0x0007e540 | 32 | discovered | 17 / 0 | vtable +0x20 `errexit(code)`: `releaseArrays`, then `longjmp` |
| `003268` | 0x0007e560 | 210 | reconstructed | 66 / 0 | vtable +0x08 `facet(count, const int* ids)` |
| `003270` | 0x0007e640 | 454 | discovered | 151 / 0 | `QhullHost::buildResult(HullResult&, bool triangles, bool reverse)` |
| `003272` | 0x0007e810 | 222 | discovered | 90 / 0 | vtable +0x14 `malloc(size)`, tracked |
| `003274` | 0x0007e8f0 | 48 | reconstructed | 13 / 0 | `BlockHeader::init(size, slot)`: `"JOHNRAT\0"` |
| `003275` | 0x0007e920 | 92 | discovered | 32 / 0 | vtable +0x18 `free(p)`, tracked |
| `003277` | 0x0007e980 | 130 | discovered | 48 / 0 | `QhullHost::~QhullHost()`: frees every live tracked block |
| `003279` | 0x0007ea10 | 819 | discovered | 287 / 9 | `HullLibrary::CreateConvexHull(const HullDesc&, HullResult&)` |
| `003347` | 0x0007fda0 | 125 | discovered | 34 / 0 | Wu `M3d` (cumulative moments): entry |
| `003349` | 0x0007fe20 | 1,145 | discovered | 172 / 0 | `M3d`, continuation |
| `003351` | 0x000802a0 | 351 | discovered | 90 / 8 | `M3d`, continuation: the g loop, all five planes (Task 4d correction: this read "the float `m2` plane") |
| `003353` | 0x00080400 | 138 | discovered | 53 / 0 | Wu `Vol(box, moment)` |
| `003355` | 0x00080490 | 210 | discovered | 88 / 0 | Wu `Bottom(box, dir, moment)` |
| `003357` | 0x00080570 | 261 | discovered | 106 / 0 | Wu `Top(box, dir, pos, moment)` |
| `003359` | 0x00080680 | 245 | discovered | 91 / 24 | Wu `Var(box)` |
| `003361` | 0x00080780 | 405 | discovered | 145 / 39 | Wu `Maximize(...)` |
| `003363` | 0x00080920 | 486 | discovered | 200 / 15 | Wu `Cut(box1, box2)` |
| `003365` | 0x00080b10 | 235 | discovered | 82 / 3 | Wu `Hist3d` for one RGB triple: builds the square table once |
| `003367` | 0x00080c00 | 648 | discovered | 236 / 28 | Wu `Quantize(out, k)`: `k` capped at 256; CRT `calloc`/`free` |
| `003369` | 0x00080e90 | 93 | discovered | 30 / 1 | `reduceVertices(allocator, n, in, &nOut, out, max)`: entry |
| `003371` | 0x00080ef0 | 1,536 | discovered | 482 / 202 | `reduceVertices`, continuation (quantise, dequantise) |

The Wu names follow the published algorithm. They are matched by index arithmetic and call
shape; Task 4 confirms each against the listing before using the name.

**The TriangleMesh-side rows the library needs:**

| Row | RVA | Size | State | Role |
|---|---|---:|---|---|
| `phys_fn_002235` | 0x00054a40 | 22 | discovered | TriangleMesh vtable slot 0: `malloc(size)` through `[0x101041bc]` `+8` `(size, NX_MEMORY_PERSISTENT)` |
| `phys_fn_002237` | 0x00054a60 | 28 | discovered | TriangleMesh vtable slot 1: `free(p)`, only if `p` is non-null, through `[0x101041bc]` `+0x14` (tail `jmp`) |

`TriangleMesh`'s first two virtuals are an allocator interface, `malloc(size)` and `free(p)`.
That is the interface `CreateConvexHull` takes as its user allocator (see "The hull library
object"). Its third virtual is `loadFromDesc` (`.rdata:0x00108608`: `0x54a40, 0x54a60, 0x55890,
0x53c80, ...`, 19 slots; slot 17 is the reader `002262` and slot 18 the writer `002162`).

### Construction and call chain

```
NxPhysicsSDK::createTriangleMesh(desc)             000242  0x0000b7e0  (scene lock walk)
  PhysicsSDK::createTriangleMesh(desc)             000478  0x0000ebe0
    isValid inline                                  0x0000ebe7-0x0000ec4f, 0x0000ecb1-0x0000eccb   fail: error(1, PhysicsSDK.cpp, 0x1f2)
    mesh = alloc(0xe8,0) (0x0000ec51-0x0000ec62); TriangleMesh()   002251  (0x0000ec6f)
    mesh->loadFromDesc(desc)  [vt+8]                002260  (0x0000ec88)
      isValid inline again                          0x0005589f-0x00055b08   fail: error(1, TriangleMesh.cpp, 0xba)
      copy desc (13 dwords) into a working desc     0x00055919-0x00055929
      store convexEdgeThreshold/axis/extent         +0x6c/+0x7c/+0x80
      if flags & NX_MF_COMPUTE_CONVEX:              0x00055942
        out = NxTriangleMeshDesc()  (inline)        0x0005595a-0x000559a1  (axis 0xff, threshold 0.001f)
        this->computeHull(desc, out)                002233  (0x000559a8), false -> return false
          HullLibrary lib = { this, 0 }             0x0005492e, 0x00054971
          HullDesc hd = { 0xb7, n, points, stride, 1e-5f, 256, 0.8f }
          lib.CreateConvexHull(hd, hr)              003279  (0x00054975)
          if 0: out = desc; out.{n,t,strides,points,triangles} from hr; out.flags &= ~8
          lib.ReleaseResult(hr)                     003255  (0x00054a26)
        working desc = out; keep out.points/triangles to free
      [triangles == NULL branch: CONVEX -> createHull 002158 / else an identity index list]
      buildInternalMesh(&working)                   002256  (0x00055bb5)
      free the temporary arrays                     0x00055bbe-0x00055bdf
      internal.buildModel(axis, extent, 0)          002083  (0x00055bf1)
      if desc.pmap: [vt+0x24] loadPMap              0x00055c02
      if +0xa0 == 0 and flags & CONVEX:
        computeConvexHull((flags>>3)&1)             002164  (0x00055c20), false -> return false
      post-build                                    002255  (0x00055c92), its al is the result
    on false: ~TriangleMesh (002253) + free; return NULL
    append to mTriangleMeshes (+0x18/+0x1c/+0x20); if mesh+0xe4 == 0 -> releaseTriangleMesh (000470)
  return mesh+0xe4 (NpTriangleMesh)
```

Inside `CreateConvexHull` (`003279`, 0x0007ea10-0x0007ed40):

1. `_chkstk(0x40d8)` (`phys_fn_005695`, 0x0007ea1a). `QhullHost host(lib.mAllocator)` is
   constructed on the stack at `ebp-0x4068` (0x0007ea43), and its address is published to
   `.data:0x00125080` (0x0007ea51, the only write).
2. `n = max(hd.vcount, 8)`. A vertex buffer of `12*(n+1)` bytes is allocated through
   `host[vt+0x14]` (0x0007ea57-0x0007ea78).
3. `host.cleanupVertices(...)` (`003243`, 0x0007ead0) is called with, in order:
   `hd.vcount, hd.vertices, hd.stride, &vcount, buf, hd.epsilon, (flags&2) ? &scale : NULL,
   flags&1, flags&4, hd.maxVertices`. If it returns false, the build fails.
4. `_setjmp3(.data:0x00125040, 0)` (0x0007eae7). A first return (0) runs
   `runQhull(2, argv, vcount, buf)` (`003236`, 0x0007eb48), with `argv = {"qhull", "o"}`
   (`.rdata:0x0010a040`, `.rdata:0x0011363c`, stored at 0x0007ea35/0x0007ea3c).
5. A return through `longjmp` on the first try (flag `ebp+0x6f`) does this:
   - if `flags & 0x80`, `writeFailObj(vcount, buf, 12)` (0x0007eb28);
   - `boxFallback(&vcount, buf)` (0x0007eb38);
   - `runQhull` again.

   The `jmp_buf` is not re-armed. A second `longjmp` returns to the same `_setjmp3` with the
   flag clear and falls through to step 6.
6. `host.buildResult(hr, flags&0x10, flags&0x20)` (`003270`, 0x0007eb7a). On true, the return
   code becomes 0 (0x0007eb8a).
7. If a scale was taken, every result vertex is multiplied component by component by `scale`
   (0x0007eba0-0x0007ebc8).
8. If `lib.mPolygonizer` (`+4`) is set and `flags & 8`, the path at 0x0007ebd6-0x0007ecd5 runs
   (see "The +4 interface"). It is dead in NovodeX's only call.
9. If the build failed and `flags & 0x80`, `writeFailObj(hd.vcount, hd.vertices, hd.stride)`
   writes the original input (0x0007ed15). If it succeeded and `flags & 0x40`,
   `writeOkObj(hr)` runs (0x0007ecf0).
10. The vertex buffer is freed through `[[0x10125080]+0x18]` (0x0007ed26), and
    `host.~QhullHost()` runs (0x0007ed2f).
11. The return is `[ebp+0x60]`: 0 for OK, 1 for FAIL (set at 0x0007ea2e).

`runQhull` (`003236`) calls, in order:

- `qh_init_A(stdin, stdout, stderr, argc, argv)`, with the static CRT's `_iob` at
  0x00122600/0x00122620/0x00122640;
- `qh_initflags(qh qhull_command)` (`.data:0x001248e0`);
- a `malloc` through `[vt+0x14]` of `n*3*8` bytes, filled by widening the float points to double
  with `fld dword`/`fstp qword`, four at a time (0x0007d490-0x0007d4c0);
- `qh_init_B(points, n, 3, True)`;
- `qh_qhull` (`003234`);
- `qh_check_output` (`003198`);
- `qh_produce_output` (`002866`);
- a free of the double array through `[vt+0x18]`.

It never calls `qh_freeqhull`. qhull's memory is reclaimed by `~QhullHost` freeing every tracked
block, and after a `longjmp` by the same destructor. `qh_new_qhull` is absent from the image
(`phase4-third-party-sources.md` §2.8).

### Object layouts

**`HullDesc`: the parameter struct `002233` fills.** It lives in 002233's frame at `E-0x38`,
where `E` is 002233's entry `esp`, and is 0x1c bytes.

| Off | Value in NovodeX's call | Store | Consumer in `003279` |
|---|---|---|---|
| +0x00 | flags `0xb7` (dword) | 0x0005494d | bit tests at 0x0007ea7d-0x0007eaa2, 0x0007eb0b, 0x0007eb53-0x0007eb6b, 0x0007ebd9, 0x0007ecde |
| +0x04 | `desc.numVertices` | load 0x0005492c, store 0x0005493b | `003243` arg 1; the `max(n,8)` buffer (0x0007ea57); `writeFailObj` (0x0007ed09) |
| +0x08 | `desc.points` | load 0x00054929, store 0x00054936 | `003243` arg 2; `writeFailObj` |
| +0x0c | `desc.pointStrideBytes` | load 0x0005493f, store 0x00054955 | `003243` arg 3; `writeFailObj` |
| +0x10 | `1e-5f` (0x3727c5ac): the weld epsilon, in normalised space when bit 1 is set | 0x00054959 | `003243` arg 6 (0x0007eab3) |
| +0x14 | `0x100`: the vertex limit | 0x00054961 | `003243` arg 10 (0x0007eaa5); `003367` also caps at 256 (`0x100 < param_2`, 0x00080c00) |
| +0x18 | `0.8f` (0x3f4ccccd) | 0x00054969 | pushed only to the `+4` interface on the bit 3 path (0x0007ebff-0x0007ec05); **never read in NovodeX's configuration**. What it means is unestablished. |

The flag bits, all consumed in `003279`:

| Bit | In 0xb7 | Effect | Where |
|---|---|---|---|
| 0x01 | on | weld duplicates in `cleanupVertices` (arg 8) | 0x0007ea9d-0x0007eaa2 |
| 0x02 | on | normalise by the extents (pass `&scale`), and rescale the result | 0x0007ea7f-0x0007ea95; 0x0007eb87-0x0007ebc8 |
| 0x04 | on | reduce to `maxVertices` through the quantizer (arg 9) | 0x0007ea98-0x0007eaac |
| 0x08 | off | hand the result to the `+4` interface | 0x0007ebd6-0x0007ebe0 |
| 0x10 | on | triangulate (`buildResult` arg 2); off gives polygons | 0x0007eb60-0x0007eb6b |
| 0x20 | on | reverse the winding (`buildResult` arg 3) | 0x0007eb58-0x0007eb63 |
| 0x40 | off | write `QHULL_OK_%04d.obj` on success | 0x0007ece2-0x0007ecf0 |
| 0x80 | **on** | write `QHULL_FAIL_%04d.obj` on the first qhull failure (the cleaned points) and on overall failure (the input) | 0x0007eb10, 0x0007ecfc |

So the shipped DLL writes a file into the current directory whenever qhull fails once. The file
counter is `.data:0x00125084` (`phys_data_003856`), pre-incremented by both writers (0x0007deac,
0x0007e05c). The files are opened `"wb"` (`.rdata:0x001061bc`) and written in this order:

- `#Vertices: %8d\r\n`, then for `writeOkObj` `#Faces   : %8d\r\n`;
- one `v %0.9f %0.9f %0.9f\r\n` line per vertex (`.rdata:0x00108510`);
- faces: `f %d %d %d\r\n` (`.rdata:0x00108500`) in triangle mode, or `"f "` (`.rdata:0x001135c0`),
  `"%d "` (`.rdata:0x0010c1f4`) and `"\r\n"` (`.rdata:0x001135bc`) in polygon mode.

Every face index is written 1-based (`inc` at 0x0007dfab, 0x0007e015-0x0007e019), and in
reverse order in both modes: in polygon mode by the countdown at 0x0007df98-0x0007dfc4, in
triangle mode by the argument order (`f i2+1 i1+1 i0+1`: `edx = [esi+8]` is pushed last,
0x0007e004-0x0007e01a). (Task 4b correction: this read "in polygon mode" only.)

The two writers, `boxFallback`, `buildResult` and `cleanupVertices` are all `thiscall` on the
host (`lea ecx,[ebp-0x4068]` before each call in `003279`), though only `buildResult` and
`cleanupVertices` read it: `writeOkObj` `ret 4`, `writeFailObj` `ret 0xc`, `boxFallback`
`ret 8`, `buildResult` `ret 0xc`. The file names are formatted into a 512-byte stack buffer.

**`HullResult`** is 0x1c bytes, at 002233's `E-0x1c`. `buildResult` zeroes +0x04..+0x18 and sets
+0x00 to 1 (0x0007e64b-0x0007e65d).

| Off | Field | Written by | Read by |
|---|---|---|---|
| +0x00 | `bool polygons`: 0 in triangle mode | 0x0007e65d, 0x0007e6ea, 0x0007e7b9 | `003279` 0x0007ebe6, 0x0007ec2d; `writeOkObj` 0x0007df61 |
| +0x04 | output vertex count (= host `+0x4034`) | 0x0007e692 | `002233` 0x00054986; the rescale loop |
| +0x08 | output vertices, `float[3]` each, from `rawAlloc` | 0x0007e6b0 | `002233` 0x000549c0; `ReleaseResult` 0x0007e306 |
| +0x0c | face count (= host `+0x1c`, qhull's numfacets) | 0x0007e6ca | `writeOkObj` 0x0007defa |
| +0x10 | triangle count, `sum(n-2)` (host `+0x20`) | 0x0007e6d6 | `002233` 0x00054994 (as `numTriangles`); `writeOkObj` 0x0007dffd |
| +0x14 | index count: `3*triangles`, or host `+0x403c` in polygon mode | 0x0007e6df, 0x0007e706 | the bit 3 path |
| +0x18 | indices, `uint32`: triangles, or `[n, i0..in-1]*` in polygon mode | 0x0007e6fd, 0x0007e7cc | `002233` 0x000549f9; `ReleaseResult` 0x0007e32c |

**The hull library object** (`HullLibrary`, 8 bytes, at 002233's `E-0x40`):

| Off | Field | Evidence |
|---|---|---|
| +0x00 | user allocator interface (slot 0 `malloc(size)`, slot 1 `free(p)`). In `002233` this is the `TriangleMesh` itself (`mov [esp+8],ecx` at 0x0005492e; the caller passes `ecx = this` at 0x00055958). NULL means the CRT. | read at 0x0007ea1f and passed to the host constructor; `ReleaseResult` 0x0007e30f/0x0007e333 |
| +0x04 | a polygon-builder interface, NULL in `002233` (0x00054971) | tested at 0x0007ebcd |

**The +4 interface** is only ever NULL in this image, so its slots are known only from the call
sites:

- `+0x00` or `+0x08` is chosen by `result.polygons` and called with a 0x18-byte local plus six
  arguments (0x0007ebe6-0x0007ec22). It returns a bool.
- `+0x14(&local)` is called when `polygons` is set (0x0007ec3b).
- `+0x10(&local)` is a release (0x0007ecd5).
- On success the result's vertices and indices are replaced from the local's `+4/+8` and
  `+0xc/+0x10/+0x14` (0x0007ec46-0x0007ecc5).
- The six arguments after `&local` are `(desc +0x18, result.vcount, result.vertices, 12,
  result +0x0c, result.indices)`. The local's `+0x10` (index count) is never initialised
  (0x0007ec09-0x0007ec16 zero the byte and `+4/+8/+0xc/+0x14`), and the replacement leaves
  `result +0x14` as it was while it sets both `+0x0c` and `+0x10` from the local's `+0xc`.

Task 4 writes this path from the listing but cannot execute it through NovodeX's caller.

**The host object** (the stack arena, 0x4054 bytes). The constructor is `003257` (0x0007e370).
The object sits at `ebp-0x4068` in `003279`'s frame, and the `argv` pair follows it at
`ebp-0x14`.

| Off | Field | Evidence |
|---|---|---|
| +0x00 | vptr `.rdata:0x00113614` | 0x0007e3b4; reset in the destructor at 0x0007e985 |
| +0x04 | `dim` (offBegin arg 1) | 0x0007e3e0 |
| +0x08 | `int* remap[numpoints]`, zeroed; 1-based output id per qhull point | allocated 0x0007e42f, zeroed 0x0007e444 |
| +0x0c | `float3 points[numpoints]`, zeroed | 0x0007e41e, 0x0007e460 |
| +0x10 | `numpoints` (offBegin arg 2) | 0x0007e3ee |
| +0x14 | points received (`point3`) | 0x0007e3f4, 0x0007e4d5 |
| +0x18 | facets received | 0x0007e3f7, 0x0007e62b |
| +0x1c | `numfacets` (offBegin arg 3) | 0x0007e3e7 |
| +0x20 | triangle count `sum(count-2)` | 0x0007e3f1, 0x0007e587 |
| +0x24 | live tracked blocks | 0x0007e8b0, 0x0007e958 |
| +0x28 | live tracked bytes | 0x0007e8b3, 0x0007e95b |
| +0x2c | peak tracked bytes | 0x0007e8ca |
| +0x30 | search start slot; zeroed, never advanced | 0x0007e391, read 0x0007e840 |
| +0x34 | `void* live[0x1000]`, a 16,384-byte table | `rep stosd` 0x0007e3ac-0x0007e3ba |
| +0x4034 | output vertex count | 0x0007e3fa, 0x0007e5f2 |
| +0x4038 | output vertices `float3[numpoints]` | 0x0007e47c |
| +0x403c | index list count | 0x0007e400, 0x0007e595/0x0007e619 |
| +0x4040 | index list capacity `16*numpoints` | 0x0007e488 |
| +0x4044 | index list `uint32[16*numpoints]` | 0x0007e497 |
| +0x4048 | user allocator (constructor argument) | 0x0007e37a |
| +0x404c | total area (float, `size`) | 0x0007e528 |
| +0x4050 | total volume (float) | 0x0007e52e |

`offBegin` (`003259`) calls `releaseArrays` first. It then allocates the four arrays through its
own `[vt+0x14]`, so they are tracked blocks. It ignores its fourth argument (`ret 0x10`).

`facet` (`003268`) runs only while `+0x18 < +0x1c`. It does the following:

- adds `count-2` to `+0x20`;
- appends `count` to the index list if there is room;
- for each id below `numpoints`, the first time it is seen, copies `points[id]` to
  `outVerts[+0x4034]`, increments the count and sets `remap[id] = count`;
- appends `remap[id]-1`.

So the output vertices are qhull's point ids, compacted in first-use order. The
`nxBatchAppend3268` shape in `ObjectModel.cpp` carries a "FAILING DIFFERENTIAL" note in its
inventory proof; the product rewrite supersedes it.

`buildResult` (`003270`) fails, returning false with the arrays released (`003238` at
0x0007e7f7 on every path), if any of `+0x0c`, `+0x08`, `+0x4034` or `+0x403c` is zero.
Otherwise it copies the vertices into `rawAlloc(n*12)`. The two modes:

- **Triangle mode** fan-triangulates each polygon `[n, i0, i1, ..., in-1]`. It writes
  `(i0, i1, i2)`, then `(i0, prev, next)`, or, reversed, `(i2, i1, i0)` and
  `(next, prev, i0)` (0x0007e722-0x0007e7b1).
- **Polygon mode** copies the list (0x0007e7b9-0x0007e7ec).

**The tracked allocator** (vtable +0x14 `003272`, +0x18 `003275`, destructor `003277`):

- `malloc(size)` allocates `size+16` through `alloc[0]` (the user allocator's slot 0) or CRT
  `malloc` (0x000f4722). It scans `live[]` from `+0x30` for a zero slot, four at a time, over at
  most 0x1000 entries.
- If no slot is free, it frees the block with **CRT `free` (0x000f4734) even when the user
  allocator made it** (0x0007e8d4), and returns NULL.
- Otherwise it bumps the counts, writes the header `{"JOHNRAT\0", size, slot}` (`003274`),
  records the block and returns `block+16`.
- `free(p)` checks the header's first four bytes are `JOHN` and that `size` is non-zero, clears
  the slot, decrements the counts and frees through `alloc[1]` or CRT `free`. A pointer without
  the header is ignored.
- The destructor frees every live slot the same way (0x0007e998-0x0007e9fc) after
  `releaseArrays`.

**`CreateConvexHull`'s frame** (`ebp = entry esp - 0x74`):

| Location | Contents |
|---|---|
| `ebp-0x4068` | host |
| `ebp-0x14`/`-0x10` | `argv` |
| `ebp+0x2c..+0x34` | `scale` (`float3`) |
| `ebp+0x38..+0x4c` | the `+4` path's local: byte at +0x38; count, vertices, faces, index count, indices |
| `ebp+0x50` | `scale` pointer or NULL |
| `ebp+0x54` | `this` |
| `ebp+0x60` | return code |
| `ebp+0x64` | vertex count |
| `ebp+0x68` | vertex buffer |
| `ebp+0x6f` | first-try flag |

The arguments are at `ebp+0x78` (desc) and `ebp+0x7c` (result).

### Dispatch tables

**`.rdata:0x00113614`**: the host vtable, 9 slots (`phys_data_001920`). The inventory's
`targets` list only the first eight; the ninth is read from the PE.

| Slot | Offset | Row | Role | Reached from |
|---|---|---|---|---|
| 0 | +0x00 | `003259` | `offBegin(dim, numpoints, numfacets, numridges)` | io.c NOVODEX [3] `qh_printbegin` (0x0006c727) |
| 1 | +0x04 | `003261` | `point3(x, y, z)` | io.c NOVODEX [1] `qh_printpointid` and five inlined copies |
| 2 | +0x08 | `003268` | `facet(count, ids)` | io.c NOVODEX [2] `qh_printfacet3vertex` (0x00067c7a) |
| 3 | +0x0c | `003265` | `size(area, volume)` | io.c NOVODEX [4] `qh_printfacets` (0x0006d458) |
| 4 | +0x10 | `003263` | `fprintf(FILE*, fmt, ...)` | about 593 qhull sites (`QhullNovodeXHost.h`) |
| 5 | +0x14 | `003272` | tracked `malloc(size)` | mem.c (0x0006dade, 0x0006dbb3, 0x0006de24); `003236`; `003259`; `003279` |
| 6 | +0x18 | `003275` | tracked `free(p)` | mem.c (0x0006dc74); `003236` 0x0007d4ec; `003279` 0x0007ed26; `003238` |
| 7 | +0x1c | `phys_fn_001583` (0x0002ea70) | a one-byte `ret`: the narrow-hull hook does nothing | poly2.c NOVODEX [1] `qh_initialhull` (0x0007965a) |
| 8 | +0x20 | `003267` | `errexit(code)` | `qh_errexit` = `phys_fn_003413` (0x00084800-0x00084810) |

Slot 4 is a variadic member. MSVC passes `this` on the stack for those: `[esp+4]` is `this`,
`[esp+8]` the stream and `[esp+0xc]` the format. The row does three things:

1. It formats into a 0x2000-byte stack buffer with the 3-argument CRT call at 0x000f621b
   (`phys_fn_005780`, vsprintf-shaped), at 0x0007e4ea-0x0007e4ff.
2. It **calls `this->errexit(1)`** (`push 1; call [eax+0x20]`, 0x0007e504-0x0007e512).
3. The formatted text is never used.

So in the shipped DLL, any qhull diagnostic printed through the host ends the hull attempt. Until
Task 4a, the candidate's shim `qhNovodeXFprintf` returned 0 instead (`Physics/src/ThirdPartyHost.cpp`);
since 4a the hook repeats this row's body (see "As written (Task 4a)"). The Task 1/2 qhull
families use their own test host, so they are unaffected.

Slot 8 (`003267`) calls `releaseArrays` (0x0007e540), then
`longjmp(.data:0x00125040, code)` (`phys_fn_005776` 0x000f6124, 0x0007e54f). It does not
return.

**`.rdata:0x00108608`**, TriangleMesh's 19 slots (from the PE), as far as this contract needs
them:

| Slot | Row |
|---|---|
| 0 | `002235` malloc |
| 1 | `002237` free |
| 2 | `002260` loadFromDesc |
| 3 | `002166` |
| 4 | `002198` |
| 5 | `002200` getCount |
| 6 | `002202` getFormat |
| 7 | `002205` getBase |
| 17 | `002262` |
| 18 | `002162` |

The second table at `+4` is `.rdata:0x001085d4` (`002211`...). The public wrapper's table,
`.rdata:0x0010829c`, is `NpTriangleMesh`'s 13 slots, in `NxTriangleMesh.h` order after
`loadFromDesc`.

**`.rdata:0x00105880`**: NpPhysicsSDK slot 8 is `000242` (see "Row assignment").

### Where the hull goes next (post-hull conversion; deferred)

`002233` returns an `NxTriangleMeshDesc` made from the result:

- the whole input desc is copied first (13 dwords, 0x00054992);
- `numVertices = hr+4`, `numTriangles = hr+0x10`, both strides 12;
- `points` and `triangles` are fresh `[0x101041bc]` `+8` `(size, NX_MEMORY_TEMP)` copies
  (0x000549bb, 0x000549f3);
- `flags &= ~NX_MF_COMPUTE_CONVEX` (0x00054a16). It does **not** clear `NX_MF_16_BIT_INDICES`,
  although the triangles it writes are 32-bit with stride 12.

`002260` then does the following with that desc:

- builds the internal mesh from it (`002256`: EdgeList and adjacency rows, 63 rows and
  18,795 B in its closure);
- builds the OPCODE model (`002083`: 40 rows, 4,075 B);
- since `NX_MF_CONVEX` is set and nothing is at `+0xa0` yet, builds the convex hull object
  `002164(false)` (73 rows, 15,250 B). This allocates `new ConvexHull` (0x98 bytes,
  `phys_fn_001524` 0x0002d940) and initialises it from the mesh through its vtable `+8` with
  `{nbVerts +8, verts +0x10, nbTris +0xc, tris +0x14, 0}`. Failure reports TriangleMesh.cpp:0x23f.
- runs `002255` (65 rows, 13,075 B).

These closures overlap and include vendored OPCODE rows. The whole `createTriangleMesh` closure
reached from `002251`, with its vtable edges, is 294 rows and 62,998 B.

The public observables come from what is built on top of the qhull output, not from the qhull
output itself:

- `getCount`/`getBase` for `NX_ARRAY_VERTICES`/`TRIANGLES` read the internal mesh (`002200`
  0x00054640, `002205` 0x00054720).
- `NX_ARRAY_HULL_VERTICES` reads `ConvexHull+0xc/+0x10`.
- `NX_ARRAY_HULL_POLYGONS` reads `ConvexHull+0x24/+0x2c`. The polygons are computed lazily by
  `phys_fn_001472` (0x0002b6f0, 664 B), reached at 0x0005468d and through `[vt+0x10]` at
  0x0005477d.

Mass and inertia are not computed in cooking. The volume integration is `phys_fn_002241`
(0x00054bb0, 692 B; it owns the `negative volume` report), called only from `phys_fn_001397`
(0x00028e10, Phase 3, `gap:SphereShape.cpp..ConvexHull.cpp`) when a shape's mass is derived.
Nothing in `NxTriangleMesh` exposes planes.

### Error and degenerate paths

- **An invalid descriptor.** `000478` reports code 1 at PhysicsSDK.cpp:0x1f2 and returns NULL.
  `002260` repeats the test and reports TriangleMesh.cpp:0xba.
- **`CreateConvexHull` fails** (returns 1). `002233` leaves `out` untouched and returns false,
  so `loadFromDesc` returns false (0x000559af -> 0x00055c55). `000478` then destroys the mesh
  (`002253`) and returns NULL. `ReleaseResult` still runs.
- **`cleanupVertices` returns false** only for `vcount == 0` (0x0007d5b0), which `isValid`
  (`>= 3`) makes unreachable from the public API.
- **Degenerate input in `cleanupVertices`.** If an extent is below `1e-6f` (`.rdata:0x00106880`)
  or `vcount < 3`, the output is exactly 8 points: the box `centre +- extent`, where the centre
  is `min + 0.5*extent` (0.5f at `.rdata:0x001043cc`).
  - A degenerate axis's extent is replaced by `0.05*` the smallest non-degenerate extent, or by
    `0.01f` (0x3c23d70a) if all are degenerate.
  - The same test runs again after welding, on the welded cloud (0x0007d9d6-0x0007de73).
  - When degenerate, the scale stays `(1,1,1)`: it is set before the test (0x0007d5d0-0x0007d5e4).
- **qhull errors.** `qh_errexit` (`003413`) goes to `errexit`. The first time it causes the FAIL
  dump, `boxFallback` (the 8 corners of `centre +- extent` of the cleaned cloud's box, so twice
  the box) and a retry. A second error leaves the arrays released, so `buildResult` fails and
  `CreateConvexHull` returns 1, with a second FAIL dump of the original input. A point set that
  survives clean-up but is coplanar on a diagonal plane (for example `x+y+z=0` spread over all
  three axes) reaches this path.
- **Any qhull print through slot +0x10** takes the same path (see "Dispatch tables").
- **The tracked allocator returns NULL** when all 4,096 slots are live, and qhull's `mem.c`
  then errexits. qhull allocates in large buffers, so this is not expected in practice. The
  mismatched CRT `free` on that path is an oracle defect to reproduce, not fix.
- **Reentrancy.** The host pointer (`0x00125080`), the `jmp_buf` (`0x00125040`), the OBJ counter
  (`0x00125084`) and the square table (`0x00125088`-`0x00125487`, with its initialised flag at
  `0x00125488`) are process globals. Cooking is single-threaded and not reentrant.

### Precision and x87

- **The control word.** No row on the chain touches it: there is no `fldcw`, `fnstcw` or
  `_controlfp` call in 0x0000b7e0-0x0000b8bb, 0x0000ebe0-0x0000edb6, 0x00054920-0x00054a33,
  0x00055890-0x00055ca1 or 0x0007d420-0x000814f0 (Capstone listing, checked by mnemonic).
  Cooking runs under the caller's control word, and qhull's doubles are computed at the
  caller's precision control. A differential must pin the control word to the same value on
  both sides.
- **`cleanupVertices`** compares floats through `fcomp`/`fnstsw` against `1e-6f`. It normalises
  with reciprocals: `1.0f` (`.rdata:0x001041ec`) `fdiv` extent at 0x0007d73c-0x0007d772, then a
  multiply per vertex, not a division per vertex. The weld keeps the vertex farther from the
  centre, by comparing squared distances. The operand order in both is taken from the listing.
- **`003279`'s rescale** is `fld scale; fmul v; fstp v` per component (0x0007eba3-0x0007ebc0).
- **`003236`** widens with `fld dword`/`fstp qword`, which is exact.
- **The quantizer** converts `(v-min)*(1/extent)*255` with an inline `fistp qword` (255.0f at
  `.rdata:0x00106888`, 0x00081208-0x00081238), so the rounding is the current mode's. A C cast
  compiled without `/QIfist` calls `_ftol2` and truncates, so the candidate needs an explicit
  current-mode conversion. The result is clamped to `[0,255]`.
  - Dequantisation is `byte*(extent*0.003921569f)+min` (`.rdata:0x00113a10`).
  - `Var` and `Maximize` do float arithmetic on `fild`-loaded integer moments (0x000806a3-,
    0x0008085d-).
  - `Quantize` divides the integer moments by the integer weight (`idiv`) to get each palette
    entry.
- **The OBJ writers** print `%0.9f` of floats widened to double. Byte-identical files need the
  same CRT float formatting; the Phase 4 test exe already links `legacy_stdio_float_rounding.obj`.
- **The x87-heavy pieces** are `cleanupVertices` (330 x87 instructions), `reduceVertices`'
  continuation (202), Wu `Var`/`Maximize`/`Cut`/`Quantize` (106 together) and `boxFallback`
  (69). They go in `/arch:IA32` translation units under `Physics/src/include/X87Sqrt.h` rules.
  There is no `fsqrt` on this path.

### How the candidate's host maps onto the object

`External/qhull/novodex/QhullNovodeXHost.h` declares nine C hooks, one per slot, in slot order.
The vendored tree already calls them at the oracle's sites. Until Task 4a, all nine were shims in
`Physics/src/ThirdPartyHost.cpp`, and malloc/free went to the SDK allocator untracked.

The product version keeps the hooks and makes each forward to the object published at the
`0x00125080` equivalent:

| Hook | Forwards to |
|---|---|
| `qhNovodeXOffBegin` | `host->offBegin` |
| `qhNovodeXPoint3` | `host->point3` |
| `qhNovodeXFacet3Vertex` | `host->facet` |
| `qhNovodeXSize` | `host->size` |
| `qhNovodeXFprintf` | format into 0x2000 bytes, then `host->errexit(1)` |
| `qhNovodeXMalloc` | `host->malloc` |
| `qhNovodeXFree` | `host->free` |
| `qhNovodeXNarrowHull` | stays empty (`001583`) |
| `qhNovodeXErrexit` | `host->errexit` (release, then `longjmp` to the driver's `jmp_buf`) |

The OPCODE hooks in the same file are untouched.

**As written (Task 4a).** Band A is `Physics/src/QhullHost.cpp` with the classes in
`Physics/src/include/QhullHost.h` (`/arch:IA32`, `/EHs-c-`). The nine hooks moved out of
`ThirdPartyHost.cpp` into `QhullHost.cpp`, after the rows, so the targets that link
`ThirdPartyHost.cpp` without qhull do not pull the library in. Three slots carry different
descriptive names in the source, so that a member cannot shadow the CRT function a row calls
directly: +0x10 `print` (the table's `fprintf`), +0x14 `trackedMalloc` and +0x18 `trackedFree`.
`qhNovodeXFprintf` cannot forward `...` to the variadic slot, so it repeated `003263`'s body
(format into 0x2000 bytes, `errexit(1)` through the vtable) on the published object, until
Task 4e routed it to the slot itself.
`NxPhysicsThirdPartyTests` compiles `QhullHost.cpp` with the nine hooks renamed
(`tests/PhysicsThirdPartyHost.cpp`) and routes each call: to the product hook while
`gQhullHost` is non-NULL, else to the Task 1 sink, else to the pre-4a stand-in (untracked SDK
allocator, `abort` on errexit, silent prints) that every registered qhull family was measured
against. The global is never cleared, in the product as in the oracle, so a harness that runs
the candidate's `CreateConvexHull` resets it afterwards.

**As written (Task 4b).** The driver rows are in the same file, in address order. `003243`'s
position holds a marked placeholder `QhullHost::cleanupVertices` with no stable-ID line: it
copies the input and sets the scale to (1,1,1), so the driver links and runs until piece 4c
replaces it. A local check (not committed) against the pinned DLL at 0x0007ea10/0x0007e300,
with flags that keep the oracle's clean-up transparent (0x00, 0x10, 0x30, and 0x40/0x80 for
the dumps), gave identical `HullResult`s (every word and both arrays) and byte-identical
`QHULL_OK` files for the cube, tetrahedron, sphere(96) and a 200-point random cloud, and on a
diagonal-plane set the same FAIL dump sequence, `boxFallback` retry and OK result. The only
differences seen were in `cleanupVertices`' territory: the oracle's clean-up writes `+0.0`
where the input had `-0.0` and reorders one 30-point set.

**As written (Task 4c).** `QhullHost::cleanupVertices` replaces the placeholder in place, one
function under both stable-ID lines (`003245` begins inside the second bounding-box loop, reached
by the jump at 0x0007d953). It is Ratcliff's `CleanupVertices` with NovodeX's two arms: the weld
runs only when `weld` is set (0x0007d7f0), and a non-degenerate cleaned cloud larger than
`maxVertices` goes to band B when `reduce` is set (0x0007da0c-0x0007da38). Read from the listing:
- `*vcount = 0` and the `(1,1,1)` scale are written before the first box (0x0007d5d2-0x0007d5e4).
- The first degenerate box takes an extent as the running shortest when it is `> 1e-6f`
  (`test ah,0x41`); the second, after the weld, when it is `>= 1e-6f` (`test ah,1`). The first
  compare against the running shortest is folded to `FLT_MAX` from memory.
- The weld keeps the new point when `dist(stored) < dist(new)` (`fcompp`, false on NaN). Both
  squared distances are summed `(z*z + y*y) + x*x` (0x0007d880-0x0007d8a6). The stored vertex's
  y and z offsets from the centre are spilled to float (0x0007d871, 0x0007d87d); the other four
  offsets, the scaled `px` (0x0007d7d4) and the weld's `|v - p|` differences stay on the stack.
- The reduce call is `thiscall` on an object with no fields, built in the dead `weld` argument
  slot (`lea ecx,[esp+0x6c]`), with `(host+0x4048, vcount, vertices, &vcount, vertices,
  maxVertices)`; its result is not read. `QhullHost.h` declares it as
  `HullVertexReducer::reduceVertices`, and `QhullHost.cpp` holds a marked no-op placeholder for
  it (no stable-ID line) until piece 4d writes band B.

A local check (not committed) called 0x0007d5b0 in the pinned DLL and the candidate on 24 point
sets (tetrahedron, cube, lattice, sphere(96), 200 and 300 random, weld pairs, `-0.0`, flat,
collinear, axis line, all equal, two points, one point, zero points, sub-epsilon extents, sets
that weld to two points or to a flat cloud, 1e6 and 1e-4 scales, a diagonal plane, stride 20),
each with weld and scale on and off, under control words 0x027f, 0x037f, 0x007f and 0x0f7f:
every return, count, scale word and output word identical in 407 of 408 cases. The one difference was a
300-point set with `reduce` on, which reaches the quantizer placeholder.

**As written (Task 4d).** Band B is `Physics/src/Quantizer.cpp` (`/arch:IA32`, `/EHs-c-`), a
sibling of `QhullHost.cpp` because band B is a separate object in the image; the name sorts
between `qset.c` and `stat.c`. `QhullHost.cpp`'s placeholder is deleted. The Wu rows are
members of `WuQuantizer`, the 0xaf794-byte table, whose planes are, in order, `wt`, `mr`, `mg`,
`mb` (32-bit integers) and `m2` (float); `x`, `y`, `z` are Wu's `r`, `g`, `b`, and the box
struct is `{r0, r1, g0, g1, b0, b1, vol}` (0x1c bytes). Read from the listing:
- `M3d` is one function under 003347/003349/003351. It is `ret 0x14` with the five planes passed
  explicitly. `line2` stays on the FPU stack across the b loop, and the unrounded
  `line2 + area2[b]` is both stored to `area2[b]` and added to `m2[ind2]`.
- `Var` sums xx in Wu's order and the squares `(db*db + dg*dg) + dr*dr`, and returns unrounded.
  `Maximize` sums `(r*r + g*g) + b*b` per half, compares the unrounded `temp > max` and keeps
  the float-stored value. `Cut` compares the float maxima with `>=`, and, **unlike Wu's source,
  returns 0 on a negative cut in every direction**, not only red.
- `Quantize` returns the box count; `k` is capped at 256. `Bottom`/`Top` return 0 for an
  unknown direction.
- `reduceVertices` (003369/003371) **returns `this`** (`mov eax,[esp+0x10]`, the saved `ecx`);
  `QhullHost.h` now declares it `HullVertexReducer*`. The quantized coordinate is the listing's
  inline `fistp qword` (a naked helper `wuFistp255` in the file: `fld; fmul 255.0f; fistp`), and
  only its low dword is clamped.
- **Two oracle defects, reproduced:** the 0xaf794-byte moment table is never zeroed
  (0x00081178-0x000811d3; the oracle relies on a block that large coming from fresh pages), and
  the output count is `min(n, maxVertices)` whatever `Quantize` returns (0x000812db), so a short
  quantization (fewer occupied cells than boxes, or a failed `calloc`) dequantises uninitialised
  palette bytes. `maxVertices` above 256 does the same for the entries past 256.

A local check (not committed) called 0x00080e90 and the reduce arm of 0x0007d5b0 in the pinned
DLL against the candidate on ten sets over 256 points (300 and 1000 random, sphere(500), a
1000-point lattice, 1e6 and 1e-4 scales, five tight clusters, a thin slab, a 2000-point shell,
`-0.0` components) plus a flat set sent to `reduceVertices` directly. It compared the count,
every output word, the return value and the allocator's call sequence and sizes. What ran
(Task 4e reconciliation, from the harness source and its log): each of the ten sets in seven
cells -- `cleanupVertices` with the CRT at `maxVertices` 256 and 64 and with the zeroing
allocator at 256; `reduceVertices` with the zeroing allocator at 256, 20 and 300 and with the
CRT at 256 -- under all five control words 0x027f, 0x0f7f, 0x037f, 0x007f and 0x067f (350
cases), and the flat set's zeroing `reduceVertices` at 256 under 0x027f and 0x0f7f only (2): 352
cases, not the 420 a review estimated. 339 were identical. The other 13 are all the clustered
set with the CRT allocator, where `Quantize` finds fewer boxes than `k` and the uninitialised
palette tail differs between the two CRTs: its two CRT `cleanupVertices` cells under all five
words (10), and its CRT `reduceVertices` cell under 0x027f, 0x037f and 0x067f (3; under 0x0f7f
and 0x007f the two heaps' tails happened to agree), which is 13, not the 15 a review expected.
The same set is identical with the zeroing allocator, through both entries and under every
control word. The registered differential of Task 4e (`hull_create`, `hull_compute` and their
0x0f7f twins) supersedes this check.

**As written (Task 4e).** `002233`, `002235` and `002237` are in `Physics/src/TriangleMesh.cpp`,
after the writer `002162`, on a base declared in `Physics/src/include/TriangleMesh.h`:
`TriangleMeshHullAllocator : HullAllocator`, whose slots 0 and 1 are `002235` (the Foundation
allocator's `malloc(size, NX_MEMORY_PERSISTENT)`, slot +8) and `002237` (its `free` for a
non-null pointer, slot +0x14), and whose member `computeHull` is `002233`, handing `this` to the
library as the user allocator. `TriangleMesh` itself still carries its vtable as an opaque word,
so deriving it from that base (and so the public chain) stays with the deferred
TriangleMesh/ConvexHull unit. `002233` is transcribed from the listing: the descriptor
`{0xb7, n, points, stride, 1e-5f, 0x100, 0.8f}`, the library `{this, NULL}`, an uninitialised
result, the 13-dword copy of the input descriptor, then the counts, both strides 12, the two
`NX_MEMORY_TEMP` copies through `[[0x101041bc]]` +8 and `flags &= ~NX_MF_COMPUTE_CONVEX`, and
`ReleaseResult` on both paths.

Differentials A and B are registered in `NxPhysicsThirdPartyTests` (`nxDriveConvexCooking`;
evidence in `evidence/qhull-gap.md`, "Task 4e"). A calls `0x0007ea10`/`0x0007e300` directly: 20
point sets with `0xb7`, the tetrahedron, cube and sphere(96) also with `0xa7`, `0x97`, `0xb6`,
`0xb5`, `0xb3` and `0xf7`, the OK dump in polygon mode (`0xe7`), `0xf7` on the lattice and the
diagonal plane, no points with `0x37`, the `+4` interface on the cube (see below), and four runs
with no user allocator (the CRT arms): 52 runs. B calls `0x00054920` with `ecx` pointing at an
object whose vptr is `TriangleMesh`'s own table `.rdata:0x00108608`, so the oracle's own
`002235`/`002237` run, and points the Foundation allocator (`[[0x101041bc]]`, which in this
process is the same variable as the candidate's `nxFoundationSDKAllocator`) at a recording
allocator: 19 sets as `NxTriangleMeshDesc`s with `NX_MF_CONVEX|NX_MF_COMPUTE_CONVEX`, and the
cube and the sphere with `NX_MF_16_BIT_INDICES` as well. Both run under 0x027f and 0x0f7f. Each
side's `QHULL_*.obj` files are written in a temporary directory of its own and taped as text
tokens (Task 1's `nxQhTapeText`). The recording allocator zeroes what it returns (the 4d
defects) and tapes every call's size, memory type and block.

Results: `hull_create`, `hull_create_pc64`, `hull_compute` and `hull_compute_pc64` are exact.
Two inputs are divergent under 0x027f only, in families of their own (`hull_create_qhull`,
`hull_compute_qhull`): the set that welds to two points (its 8-corner box) and the five
clusters (a short quantization). qhull's input is the same on both sides, point for point
(measured with the harness's `NXHULL_PROBE` digest); qhull's own path over it differs (another
vertex order; for the clusters eight more tracked allocations, with the same counts). Under
0x0f7f both are exact. Attribution (corrected in Task 5): box: vendored qhull (reproduced by
`hull_qhull_direct`); clusters: not reproduced by qhull alone -- open (Task 5; candidates:
allocation pattern, `qh_gethash` address hashing). The four `_obj` families differ in one word each: the 2003 static CRT prints a float
`-0.0` as `0.000000000` and the UCRT as `-0.000000000` (the collinear set's FAIL dump of the
cleaned points).

Found by the differential and recorded, not fixed:
- **`002233`'s result is uninitialised when `CreateConvexHull` returns before `buildResult`**
  (only `cleanupVertices` refusing, `vcount == 0`): `ReleaseResult` then frees the two stack
  words at `E-0x14`/`E-0x04` through `002237`. The candidate does the same with its own stack.
  `isValid`'s `numVertices >= 3` keeps it unreachable from the public API; B does not drive it.
- **`HullResult +0x00` is a byte** (`mov byte ptr [ebp],1` at 0x0007e65d, `0` at 0x0007e6ea):
  the upper three bytes keep what the caller left there, on both sides.

**The `+4` interface is driven** with a test interface whose slots are the candidate's
`HullPolygonizer` virtuals, the same object serving both sides: `fromTriangles` (bit 3 with
triangles), `fromPolygons` then `finishPolygons` (bit 3 with polygons), a refusal at each, the
interface without bit 3 and bit 3 without the interface. All six runs are in `hull_create`
and `hull_create_pc64`, exact: the arguments the listing pushes (0x0007ebe6-0x0007ec22, the
float at `desc +0x18` as the second), the replacement of the result's vertices and indices, and
`+0x14` left as it was.

**Two slots no hull run reached in the candidate.** The size slot `003265` is called by qhull
only for the `FS` format (io.c NOVODEX [4]), and `CreateConvexHull` runs `o`: it is called
directly (`hull_host_size`, exact). The print slot `003263` was reached through the
`qhNovodeXFprintf` hook, which repeated its body (see "As written (Task 4a)"): the hook now
formats the message and calls the slot through the vtable with the text as `"%s"`, so the row's
own code formats it again, to the same text, and errexits. The harness's copy of the hook does
the same.

The cdb trace of the candidate over these families (`evidence/qhull-gap-trace-cooking.txt`)
shows every one of the 40 written rows executing. Four have no out-of-line call in the test exe
and are credited through the caller they are inlined into: `003257` and `003277` into `003279`,
`003274` into `003272` and `003365` into `003369`.

**Review follow-up (qhull-gap Task 5).**

- **The print slot's dispatch stays a hook.** The oracle's qhull calls slot +0x10 inline at
  every host-print site (`mov eax,[0x10125080]; mov ecx,[eax]`, the arguments pushed, `push eax;
  call [ecx+0x10]`), so `003263` formats qhull's own format once. The candidate's qhull calls
  `qhNovodeXFprintf`, which formats the message and calls the slot through the vtable with
  `"%s"`, so `003263` formats the same text a second time. Doing the dispatch inline in the
  `fprintf` redirect of `QhullNovodeXHost.h` was weighed and not done: it would change the
  ~593 host-print sites that NxQhull.lib compiles, which are the bodies the committed
  `vendored_match` classes and the qhull-gap Task 2 promotions describe (`vendored_match.py`
  normalises a call of a `qhNovodeX*` hook to `icall[host]+slot`; an inline vtable load from
  a C global is not normalised), and Task 1's print capture reaches the harness through the
  hook. The difference is unobservable: `003263`'s buffer is never read, and what it does is
  `errexit(1)` through slot +0x20, as the oracle's. `003263`'s proof records the reach.
- **The OBJ dumps are also compared as bytes**: a length and a digest of each file after the
  sign of a printed `-0.000...` is dropped (the two CRTs' one known difference), in four
  families of their own, all exact.
- **qhull alone over the two `_qhull` inputs** (`hull_qhull_direct`, `nxQhullRun` with `"o"`,
  the candidate `cleanupVertices`' output for each set): the box's hull differs in qhull
  itself (105 discrete words), so that divergence is qhull's, statement for statement. The
  clusters' hull is the same in qhull alone (21 doubles differ, the `qhull_hull_x87` class);
  inside `CreateConvexHull` it differs, so for the clusters the combinatorial difference is
  not reproduced by qhull alone (see `evidence/qhull-gap.md`, Task 5).
- The trace driver is committed as `tools/hull_trace.py`.

### Dependency closure

**write (Task 4): 40 rows, 12,493 B.** 34 of them are `discovered` (11,968 B). The other six
are the generic-shape rows, re-sourced.

| Piece | Rows | B | discovered B |
|---|---:|---:|---:|
| host object, tracked allocator and hooks | 14 | 1,334 | 809 |
| driver, result, release, fallback and OBJ writers | 8 | 2,690 | 2,690 |
| `cleanupVertices` | 2 | 2,264 | 2,264 |
| Wu quantizer and `reduceVertices` | 13 | 5,878 | 5,878 |
| TriangleMesh side: `002233`, `002235`, `002237` | 3 | 327 | 327 |

**reuse:**

- the vendored qhull as built in `NxQhull`. `qh_init_A` (`002585`), `qh_initflags` (`002587`),
  `qh_init_B` (`002678`), `qh_qhull` (`003234`), `qh_check_output` (`003198`) and
  `qh_produce_output` (`002866`) are all `reconstructed`, as is `qh_errexit` (`003413`);
- `QhullNovodeXHost.h` and the NOVODEX io.c/poly2.c dispatches;
- the Foundation allocator (`nxGetSdkAllocator`, `phys_fn_004803`) where the oracle uses
  `[0x101041bc]`;
- `phys_fn_001583` (the shared one-byte `ret`);
- the static-CRT rows: `_chkstk` 005695, `_setjmp3` 005779, `longjmp` 005776, `malloc` 005691,
  `free` 005692, `calloc` 005763, `fopen` 005671, `fclose` 005673, `fprintf` 005716,
  `sprintf` 005739, vsprintf 005780. These are phase 8, `classified`, and the candidate's CRT
  supplies them.

**defer:**

- the public chain `000242`, `000478`, `002251`, `002260` and `002253` (2,008 B). They are
  wired only once the `TriangleMesh` class is real.
- the NpTriangleMesh wrapper (`002140` and the table at `0x0010829c`);
- TriangleMesh's other vtable slots;
- `002256`, `002083`, `002164`/`002158`, `002255`;
- the ConvexHull, EdgeList and IceAdjacencies rows under them;
- the mass rows `002241`/`001397`.

That is the `createTriangleMesh` closure above (about 63 KB with its vtable edges). It is a
TriangleMesh/ConvexHull unit of its own, and Task 4 cannot absorb it at the ~12 KB piece size.

### Existing candidate code this replaces

As recorded before Task 4 (the first two items are replaced; see the "As written" notes):

- `Physics/src/ThirdPartyHost.cpp`, the qhull half: nine shims until Task 4a. `qhNovodeXFprintf`
  returned 0 where the oracle errexits, `qhNovodeXErrexit` called `abort()` where the oracle
  `longjmp`s, and malloc and free were untracked.
- `Physics/src/ObjectModel.cpp` generic shapes: `nxOwnVtableRelease3238`, `nxBatchAppend3268`,
  and the shape entries recorded as sources for `003257`, `003261`, `003265` and `003274`. Their
  inventory proofs were phase 8 shape drives, and `003268`'s carried a failing-differential note.
  They became the product class's members in Task 4a. **Decision (qhull-gap Task 5):** the
  helpers `nxBatchAppend3268` and `nxOwnVtableRelease3238` are kept, because the Phase 5
  NxPhysicsObjectLayoutTests' `batch3268` and `ownvtable3238` blocks still drive them; their
  comments now call them models superseded by `QhullHost.cpp` and no longer carry the
  `// phys_fn_` stable-ID line form. The rows' `source` is `QhullHost.cpp`. Whether the generic helpers are deleted or kept for
  their drive is Task 4's call; the rows' `source` moves either way.
- `Physics/src/NpPhysicsSDK.cpp:109` `createTriangleMesh` returns 0 ("needs TriangleMesh,
  Phase 4"). It is unchanged in Task 4 under the split below.
- `Physics/src/TriangleMesh.cpp`/`.h` model `TriangleMesh` without a C++ vtable ("the vtable
  pointer is carried as an opaque word"). So `002233`'s `ecx`-as-allocator cannot be expressed
  through `TriangleMesh` yet. Task 4 writes `002233` against an allocator interface pointer, and
  `002235`/`002237` as that interface's TriangleMesh implementation. It records the
  `TriangleMesh` base-class wiring as deferred.

### What the tests reach

As recorded before Task 4; Task 4e adds differentials A and B (see "As written (Task 4e)").

- **The candidate's public convex mesh API.** Nothing exists: `createTriangleMesh` returns NULL
  and there is no NpTriangleMesh.
- **The Phase 4 asset tests** (`tests/PhysicsAssetTests.cpp`) drive the mesh stream's header
  reader and writer on byte-built objects (`kMeshWriterRva` 0x000539d0, :111, :1007, :1049).
  They do not drive cooking.
- **`tests/PhysicsThirdPartyTests.cpp`** drives vendored qhull through `003236`'s call sequence
  with its own nine-slot host object (`gQhHostVtable`). Until Task 4e it never called
  `003279`/`002233`; `nxDriveConvexCooking` now does.

### The differential Task 4 should build

**A: `CreateConvexHull` direct.** This is a new family in `NxPhysicsThirdPartyTests`, oracle
RVAs `0x0007ea10`/`0x0007e300` against the candidate class.

- **Inputs.** A `HullLibrary` of `{test allocator with slots 0/1, NULL}` and a `HullDesc`. Run
  every input with `0xb7`, and the tetrahedron, the cube and a sphere also with `0xa7`
  (polygons), `0x97` (no reverse), `0xb6` (no weld), `0xb5` (no scale), `0xb3` (no reduce) and
  `0xf7` (OK dump).
- **Tape.** The return code; every `HullResult` word, with the float bits exact; the allocator's
  call sequence and sizes; and the bytes of any `QHULL_*.obj` written. Run each side in its own
  temporary directory: both DLLs keep their own counter.
- **Point sets:**
  - the tetrahedron, cube, lattice, sphere(96) and box(200) of `nxQhullPoints`;
  - more than 256 points, which reaches the quantizer;
  - duplicates within `1e-5` after normalisation (the weld);
  - a flat set with one extent 0 (the clean-up box);
  - a collinear set, and every point equal;
  - fewer than 3 distinct points after welding;
  - large coordinates (1e6) and tiny ones (1e-4);
  - a diagonal-plane set (the qhull failure, retry and fail dump);
  - a stride larger than 12.

**B: through `002233`.** Oracle RVA `0x00054920` with `ecx` = a fake owner whose vtable slots
0/1 are the test allocator, against the candidate function. Inputs are the same point sets as
`NxTriangleMeshDesc`s with `NX_MF_CONVEX|NX_MF_COMPUTE_CONVEX`, and one with
`NX_MF_16_BIT_INDICES` also set. Tape the output desc's 13 dwords and both arrays.

**C: public API, oracle side only for now.** This is the target the deferred
TriangleMesh/ConvexHull unit has to meet.

- Use `createTriangleMesh` with `NX_MF_CONVEX|NX_MF_COMPUTE_CONVEX` and `triangles = NULL`.
- Print `getSubmeshCount` and `getCount`/`getFormat`/`getStride`/`getBase` contents for
  `NX_ARRAY_VERTICES`, `TRIANGLES`, `HULL_VERTICES` and `HULL_POLYGONS`, and `saveToDesc`.
- The mass comparison needs an actor with an `NxTriangleMeshShapeDesc` and a density
  (`getMass`, `getMassSpaceInertiaTensor`, `getCMassLocalPosition`), and so the Phase 3 shape
  path through `001397`/`002241`.
- There are no plane getters in the public API.
- Until the candidate implements `createTriangleMesh` this family can only be recorded, not
  compared. Registering it as a failing candidate family would break the Phase 4 gate.
  **Controller decision:** either run C oracle-only (the `--self` pattern of the asset tests) in
  Task 4, or defer it with the public chain.

With these inputs, A and B are expected to execute every one of the 40 written rows except the
`+4` interface arm of `003279` (0x0007ebd6-0x0007ecd5), which needs an interface NovodeX never
passes. The cdb trace confirms which rows actually ran. Task 4 can
drive that arm with a test interface, or record it as statically reviewed.

### Task split (Task 4)

Each piece is at most about 12 KB of oracle bytes, in address order, with stable-ID lines.
Pieces 1 and 5 carry no x87.

| # | Piece | Rows | B | x87 |
|---|---|---:|---:|---|
| 4a | the host object (`QhullHost`: layout, vtable, tracked allocator, `offBegin`/`point3`/`facet`/`size`/`fprintf`/`errexit`, `releaseArrays`, `rawAlloc`/`rawFree`, destructor), the globals `0x00125080`/`0x00125040`, and the nine hooks re-pointed | 14 | 1,334 | none |
| 4b | `CreateConvexHull`, `runQhull`, `buildResult`, `ReleaseResult`, `boxFallback`, `writeOkObj`, `writeFailObj`, the counter `0x00125084` | 8 | 2,690 | light: the rescale, the widening, the writers; `boxFallback` has 69 |
| 4c | `cleanupVertices` (`003243`/`003245`) | 2 | 2,264 | **heavy** (330) |
| 4d | Wu quantizer and `reduceVertices` (band B), the square table and flag | 13 | 5,878 | **heavy** (`003371` 202, `Var`/`Maximize`/`Cut`/`Quantize` 106) |
| 4e | `002233`, `002235`, `002237` against the allocator interface; differentials A and B; the cdb trace, registry lines, ceilings, inventory and ledgers | 3 | 327 | none |

Order: 4a, 4c and 4d are independent. 4b needs 4a, and 4e needs all four.
