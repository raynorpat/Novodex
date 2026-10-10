# Scene retained event and root buffer teardown (`phys_fn_000663`)

The Scene deleting destructor retains three vector allocations that were not
released by the candidate: buffered contact reports at `+0x60c`, trigger events
at `+0x5fc`, and active simulation roots at `+0x57c`. The registered
`NxPhysicsPopulatedSceneTeardownTests` fixture now keeps each allocation live
until Scene release and checks the allocation tracker after the destructor.
The trigger fixture generates callbacks before teardown, while the contact
fixture leaves the final buffered report pending.

The test first failed against the candidate. With only the contact case added,
the oracle released its 264-byte buffer and the candidate retained it. After
adding the trigger and root cases, the oracle released 72 and 8 bytes
respectively while the candidate retained both. The candidate now frees each
buffer through `nxFoundationSDKAllocator` and clears its begin/end/capacity
fields. The final staged-pair differential passes with both exits zero,
`stdout_delta=0`, and exact stderr. The transcript records
`contact_report_buffer bytes=264 freed=1`, `trigger_buffer bytes=72 freed=1
callbacks=20`, and `active_root_buffer bytes=8 freed=1`.

The existing contact-pair fixture also checks the release delta for its own
Scene (`-54`) rather than comparing the allocator's cumulative process total.
The pinned oracle lazily makes a separate 24-byte process-wide allocation
through RVA `0x000b4530` while the blocking simulation wait completes; the
candidate does not take that path. That count is outside this destructor
ownership check and remains a separate reconstruction observation.

This evidence closes these three specific `phys_fn_000663` cleanup paths only.
The whole 906-byte destructor remains `dynamically_gated` with other teardown
branches still open. No public Physics headers changed.
