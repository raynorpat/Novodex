# Phase 4 PMap serialization and cell-run reconstruction

Date: 2026-10-02. Oracle: pinned `NxPhysics.dll` SHA-256
`4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c` from
`D:\FlamingEnt__\Unreal_3\Binaries`.

This closes the public triangle-mesh PMap size/data accessors and the cell-run
serializer/decoder path left open by the load-path evidence in
`phase4-pmap-reconstruction.md` §2 and §6. It does not close PMap computation or
the alternate file/stream creation arms.

## Reconstructed rows

- `phys_fn_002017` (`0x0004e1a0`, 915 bytes), implemented as
  `PenetrationMap::serialize` in `Physics/src/PMap.cpp`. It writes the `PMAP`
  tag, version, and resolution; masks off the filled/interior flags; sorts
  cells by value; encodes repeats or absolute 32-bit value ids; serializes each
  value's Morton-ordered cell run; writes the `0xffffffff` terminator; then
  writes one inverted filled bit per grid cell. The writer deliberately leaves
  trailing partial bits pending, matching `MemoryStream::getLength` and the
  image's observed buffer size.
- `phys_fn_001990` (`0x0004cc60`, 633 bytes), implemented as
  `PenetrationMap::encodeCellRun`. It writes the cell count, sorts coordinates
  by the same spread-table Morton key used by `finish`, and emits the oracle's
  5-bit 26-neighbor codebook. Codes 26–31 replace selected coordinates with
  resolution-width absolute values.
- `phys_fn_002008` (`0x0004dba0`, 909 bytes), implemented as
  `PenetrationMap::decodeCellRun`. It reads the 32-bit count and applies the
  26 relative moves or six absolute-coordinate forms before appending each
  reconstructed grid index.
- `phys_fn_002172` (`0x00053e20`, 84 bytes) and `phys_fn_002174`
  (`0x00053e80`, 148 bytes), implemented by `TriangleMesh::getPMapSize` and
  `TriangleMesh::getPMapData`. The size accessor returns zero without a map.
  Data export succeeds only when `NxPMap.dataSize` exactly matches the measured
  serialized size, then copies the collapsed stream into the caller's buffer.

The implementation uses the existing CRT allocation/sort path and adds no
runtime dependency. Public headers are unchanged.

## Oracle-backed fixtures

`NxPhysicsTriangleMeshApiTests` first failed on the candidate because both
accessors returned their old stub results. The pinned oracle and candidate now
match on these public mesh operations:

| Input | Oracle result | Candidate result |
| --- | --- | --- |
| Resolution-1 minimal map | size 24; bytes `504d415004000000010000001fffffff800000003fffffff` | exact match |
| Resolution-32 map with value id 1 at cells 0, 1, 32, and 63 | serialized size 4,124; FNV-1a `575faca3773bc417` | exact match |
| Export buffer one byte smaller or larger than `getPMapSize` | false | false |
| Malformed reload after a valid map | load false, `hasPMap` false, size 0, export false | exact match |

The resolution-32 payload exercises code 19 for the initial cell, codes 1 and 8
for local moves, and code 26 plus a five-bit absolute x value for the final
cell. The test asserts the oracle-pinned size and hash, not just successful
serialization.

Commands run from the current worktree:

```powershell
cmake --build build --config Release --target NxPhysics NxPhysicsTriangleMeshApiTests
build\Release\NxPhysicsTriangleMeshApiTests.exe (Resolve-Path build\Release).Path
build\Release\NxPhysicsTriangleMeshApiTests.exe D:\FlamingEnt__\Unreal_3\Binaries
```

Both executions exited 0. The candidate module audit reported two pair modules,
seven trusted system modules, and zero rejected modules. The standalone
simulation output retains the already tracked one-ULP mesh-settle difference;
the PMap serialization lines above match exactly.
