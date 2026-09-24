# Phase 5 actor position and owned shape update

Actor dynamic vtable slot 2 points to RVA `0x8f60` (`phys_fn_000198`). The
static arm copies the requested translation into body `+0x44`. The dynamic
arm copies it into record `+0x50` and `+0x18`, sets record dirty bit 1,
refreshes the mass-frame center and inertia, then invokes the body shape's
slot-6 owner update with flag 1. The candidate now follows these branches
for a single shape and dispatches a two-child group to both children.

BASE shape slot 6 at RVA `0x266a0` (`phys_fn_001315`) had only the detached
no-op. Its owned pose-composition branch now reads the static cached body
pose or dynamic quaternion/translation, multiplies it by the local shape pose
at `+0x6c`, and stores the world pose at `+0x0c`. It also handles the scene
timestamp snapshot, pose mirror flag, flag-2 update, and pruner generation
increment. The owned function's optional `+0xa0` list and other pruner arms
remain open; the inventory row stays `discovered`.

The public setter probe compares static and dynamic positions, record current
and shadow translations, mass center, shape world pose, actor dirty bit,
pruner invalidation, and two-child group translation. A quarter-turn actor
with a rotated and translated local shape pose matches the oracle word for
word. These eleven new assertions raise the Phase 5 floor from 719 to 730.

The probe also exposed an independent dynamic broadphase initialization gap:
the oracle initializes new 0x18-byte entries with positive max-float minima
and negative max-float maxima, sets the shape pruning-section byte to 1, and
increments the manager generation. `nxSceneBroadphaseRegister` now does so.
The absolute generation later in the setter probe differs by one because an
earlier, unrelated mutation path still lacks a pruner invalidation; the probe
checks the two-generation delta caused by the position calls. The explicit
vtable-family `CANDIDATE-MISSING` gate marker remains in place.
