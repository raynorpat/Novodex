# Phase 5 actor-created shape final wiring

The scene shape factory previously left the internal shape's first word zero.
It now installs the already reconstructed final method tables for plane,
sphere, box, and capsule objects. Actor-created shapes dispatch their
family-specific self-return slots like the pinned DLL: box and plane slots
14–16, sphere and capsule slots 16–18. This is a wiring step, not closure
of all table entries or constructors.

The same probes exposed missing descriptor fields. The factory now stores
the sphere radius at `+0xe0`, capsule radius and half-height at `+0xe0/+0xe4`,
and plane normal and negated distance at `+0xe0..+0xec`. The plane's default
zero distance becomes the oracle's negative-zero word. Scene registration,
unregistration, and actor release previously identified a compound shape by
testing whether `+0xe0` was nonzero. That field is the radius on sphere and
capsule shapes; scene code now checks the shape type tag at `+0xd0` for type
5. This prevents release of a sphere with nonzero radius from interpreting
its radius bits as a child-array pointer.

Seven added public actor lines compare box, sphere, capsule, and plane
vtable dispatch and the three descriptor field groups. All match the pinned
oracle, raising Phase 5's coverage floor from 752 to 759. Mesh and compound
final tables, constructor allocation semantics, and complete virtual
behavior remain open. The explicit vtable-family `CANDIDATE-MISSING` marker
therefore remains.
