# Phase 5: public box shape flags

The pinned oracle's public box vtable at RVA `0x106dc8` sends slot 5 to `0x230a0` and slot 6 to `0x236d0`. The setter delegates to internal leaf `0x26df0`, which sets or clears a 16-bit mask at shape+`0xde` and calls dirty-marker `0x26c90` with `0x10`. The getter delegates to `0x257d0`, returning the masked 16-bit value itself. The public signature is `NX_BOOL`, but that does not make the result a normalized C++ boolean.

The staged actor pair records `actor box flags_default=8.0.8`, `actor box flags_changed=0.20.20`, and `actor box flags_cleared=0.0.0` after enabling feature indices and clearing visualization, then clearing feature indices. A second box created from a descriptor with `shapeFlags=NX_SF_FEATURE_INDICES` records `actor box descriptor_flags=0.20.20` before any setter. Oracle and candidate transcripts match exactly. These four lines raise the Phase 5 coverage floor from 341 to 345.

A deliberate candidate mutation normalized `getFlag` to boolean. The oracle printed `8` and `0x20`, while the candidate printed `1` in both positions; the staged differential failed with `stdout_delta=6`. The mutation was reverted. Scene dirty-table queueing when the per-shape flag word starts at zero remains unproven by this drive, and the other box virtuals remain open.
