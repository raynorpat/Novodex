# Phase 5 public shape pose setters

The common public shape table now implements all six pose setters:
local and global pose, position, and orientation. Local setters update the
shape's pose at `+0x6c`, then run `ShapeBase::nxApplyOwnerUpdate(1)` to
compose the world pose and update the pruner. Global setters invert the
owner actor's pose to derive the local value before using the same path.

The posed-box drive uses a nonidentity actor orientation, a nonidentity
initial local orientation, and successive pose mutations. It compares
the local and world pose words, update flags, and pruner generation after
each setter. All six new lines match the pinned DLL bit for bit, raising
the Phase 5 coverage floor from 794 to 800. This establishes the ordinary
unlocked actor-created box path; scene-lock and error branches and other
shape-family operations remain open.
