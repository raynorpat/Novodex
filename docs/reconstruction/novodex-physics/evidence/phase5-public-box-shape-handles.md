# Phase 5 public box shape handles — 2026-09-24

The staged actor lifecycle now calls `NxActor::getNbShapes()` and
`NxActor::getShapes()` for static and dynamic single-box actors and a dynamic
two-box actor. It follows the returned public `NxShape*` handles through
`getActor()`, `getType()`, `isBox()`, and `NxBoxShape::getDimensions()`.
Thirty-seven registered lines raise the Phase 5 floor from 252 to 289.
The rebuilt DLL and pinned oracle return identical ordinary transcripts.

The oracle actor vtable slots 15 and 16 at RVAs `0x2d00` and `0x2d30`
delegate to body functions at `0x14f0` and `0x1540`. A single internal box
shape has kind 2 at `+0xd0` and returns one handle via the address of its
`+0x9c` pointer. A kind-5 group counts children in `+0xe0/+0xe4` and returns
the parallel handle array at `+0xf0`. Its two children are ordinary boxes.

Each public box handle is a separate 0x1c-byte allocation. Its `+0x8` and
`+0x18` fields point to the 0x228-byte internal shape; internal shape `+0x4`
points to the owning 0x50-byte body, whose first word points to the public
actor. The box dimensions live at internal shape `+0xe4`; the second group
child retains its distinct `2,1,1` dimensions. The descriptor list's
single-shape path must dereference its first `NxShapeDesc*` before reading
the shape kind; treating the list buffer itself as a descriptor produced a
pointer-valued kind in the candidate.

The drive checks the negative `isSphere()` branch for every box and confirms
that `getDimensions()` returns a reference into internal shape `+0xe4`, not
just equal copied values.

The measured box-handle table spans 35 x86 slots. Four are currently wired
to recovered behavior: slot 1 `getActor` (oracle RVA `0x24870`), slot 27
`getType` (`0x248a0`), slot 28 `is` (`0x23a90`), and slot 32
`getDimensions` (`0x23520`, delegating to `0x20480`). The other slots
fail explicitly rather than returning invented values. Separate mutations
of the four implemented methods changed the staged transcript and failed
the differential: wrong actor (`stdout_delta=8`), wrong type (320), failed
`isBox` (320), and dimensions shifted by four bytes (8). Each mutation was
rebuilt before measurement and restored afterward.

This proves only the driven box-handle methods and ownership graph. Other
shape kinds and the remaining box virtuals are still open. The Phase 5
object-layout gate remains red on its explicit final-vtable placeholder;
this packet does not remove or weaken it. Public headers are unchanged.
