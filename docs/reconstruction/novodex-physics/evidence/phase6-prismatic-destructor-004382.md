# Phase 6 closure: `PrismaticJoint::~PrismaticJoint` (`phys_fn_004382`)

`phys_fn_004382` at RVA `0x000ad740` deletes the public `NpPrismaticJoint` wrapper when the internal joint is destroyed. The existing general joint transcript did not prove this cleanup: omitting the delete left that transcript unchanged.

The allocator-only fixture in `tests/PhysicsJointTests.cpp` now creates and releases a prismatic joint while Foundation allocator A and Physics allocator B are distinct. The oracle allocates the 0x17c-byte public wrapper and the 0x1c-byte internal joint through Foundation, then records two Foundation frees on release. The registered `NxPhysicsJointAllocatorTests` gate pins those oracle lines.

For row-specific falsification, made a throwaway `git archive` copy of the current commit, applied the fixture change, removed the destructor's public-wrapper delete, and rebuilt the Release DLL and allocator test with CMake. Both test processes exited 0, but the candidate recorded one Foundation free instead of two; the staged differential rejected it with `stdout_delta=2` and exact stderr. Mutant candidate SHA-256: `6b19f10bd74fb5436279c533404f239e6350dc54a76645370650f228e3e85cc2`.

Restored the destructor and rebuilt. The allocator transcript returned to exact equality (`stdout_delta=0`, both exits 0, exact stderr). Restored candidate DLL SHA-256: `4a9d81d19a53e91f34d54ff961482cfa54c176e13af4c913d6adde24ab5da4fe`. Pinned oracle DLL SHA-256: `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

The prismatic allocator fixture was also run alone against the pinned oracle and candidate after restoration. It reported the same 0x17c and 0x1c allocation sizes and two release frees for both; the differential passed with `stdout_delta=0`.
