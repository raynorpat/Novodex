# Phase 5 actor quaternion orientation

Actor dynamic vtable slot 4 is RVA `0x9450` (`phys_fn_000202`). Its static
arm expands the input quaternion into nine row-major matrix words at body
`+0x20`. Its dynamic arm copies the four input words to record `+0x5c`,
mirrors them to shadow `+0x24`, sets dirty bit 2, refreshes the mass-frame
center and inertia, and calls the body shape's slot-6 owner update with flag
1. The candidate implements these arms without changing public headers.

The public actor setter probe compares the dirty bit, current and shadow
quaternion, mass-frame center, owned shape world pose, and static matrix.
All four registered orientation lines match the pinned oracle and candidate,
raising Phase 5's floor from 730 to 734. The probe writes an explicit
identity local shape pose before the quaternion call because candidate shape
creation currently leaves its default local rotation zero while the oracle
uses identity. That shape-creation gap remains open. The vtable-family
`CANDIDATE-MISSING` marker also remains, so Phase 5 is still red.
