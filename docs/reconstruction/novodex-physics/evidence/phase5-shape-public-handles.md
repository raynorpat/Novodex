# Phase 5 actor-created public shape handles

The internal plane, sphere, box and capsule tables were installed in the
previous step, but non-box shapes still handed the caller a public `NxShape`
handle with a null vtable. The candidate now installs a public table for
each of plane, sphere and capsule. Their shared `NxShape` slots reuse the
reconstructed actor, group, material and type operations; geometry slots
remain family-specific. Sphere `getRadius` reads internal `+0xe0`. Capsule
`getRadius` reads `+0xe0` and `getHeight` doubles its stored `+0xe4`
half-height. Unsupported final operations retain explicit abort entries.

The public actor drive checks `getShapes()[0]`, shape type, owning actor,
group, material, sphere radius, and capsule radius/height. It also calls the
internal slot-10 center or extent method on each actor-created family:
box diagonal, sphere radius, capsule radius plus half-height, and plane's
max-float extent. All nine new lines match the pinned DLL and candidate,
raising Phase 5's floor from 759 to 768. Full public shape operations,
mesh and compound families, and the vtable-family gate marker remain open.
