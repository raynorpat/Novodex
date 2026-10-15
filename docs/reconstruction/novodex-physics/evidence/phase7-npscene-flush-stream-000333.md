# NpScene flushStream no-op wrapper mapping

Phase 7 row `phys_fn_000333` is `NpScene::flushStream`. The oracle loads the
internal Scene pointer from `NpScene+0x24` and tail-jumps to `NxFluidAssert`;
the pinned helper is a one-byte `retn`, so the public slot has no observable
work. The registered simulation fixture observes zero error callbacks, rejects
an `NXE_DB_WARNING` mutation (`stdout_delta=554`, candidate exit 1), and the
restored candidate matches the oracle (`stdout_delta=0`, exact stderr).

The Win32 Release map resolves the candidate implementation to
`?flushStream@NpScene@@UAEXXZ` at `0x10042530` in `NpScene.obj`.
