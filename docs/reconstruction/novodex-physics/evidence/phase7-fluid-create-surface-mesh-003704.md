# Public fluid surface-mesh creation — row 003704

IDA identifies `phys_fn_003704` at RVA `0x8b250` as the public `NxFluid::createSurfaceMesh` wrapper. It checks the fluid feature license bit, obtains the Scene write lock, blocks API reentry, validates the descriptor, releases any prior surface manager, creates the backend manager and public mesh, attaches Scene lock links to the returned object, and releases the guard and lock.

The candidate implementation is `nxFluidCreateSurfaceMeshPublic` in `Physics/src/Scene.cpp`. The registered licensed simulation fixture creates a fluid, invokes explicit public surface-mesh creation, checks the returned mesh's `userData`, verifies the backend callback arguments, and exercises the resulting surface properties. Its row-specific coverage line is:

```text
simulation fluid licensed-surface-mesh properties row=003704 matched=1
```

Replacing the wrapper with an immediate null return is mutation-rejected by the staged oracle/candidate run: the candidate exits 1, `stdout_delta=77`, and the row marker reports `matched=0`. After restoring the wrapper and rebuilding `NxPhysics.dll`, the oracle and candidate both exit 0 with `stdout_delta=0` and exact stderr.

The registered row passes the fresh Phase 5/6/7 gates at 2,815/2,815, 1,704/1,704, and 2,945/2,945 assertions. The reconstruction-tool suite passes all 816 tests. Raw build, mutation, restored-control, gate, and suite transcripts are in [`evidence/raw/phase7-fluid-create-surface-003704-20261016/`](raw/phase7-fluid-create-surface-003704-20261016/).
