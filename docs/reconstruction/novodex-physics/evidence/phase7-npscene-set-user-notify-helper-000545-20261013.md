# NpScene joint-break notifier store `phys_fn_000545`

`phys_fn_000545` is the private callback-store helper called by `NpScene::setUserNotify`. The candidate now models it as `NxSceneInternal::setUserNotify` in `Physics/src/Scene.cpp`; it stores the pointer at Scene+0x6ac and returns it, and the wrapper calls it. Public SDK headers are unchanged.

Test-first verification used the registered joint-break notifier fixture. A no-op store in the helper is caught (`oracle_exit=0`, candidate exit 1, `stdout_delta=831`; failed-candidate stderr differs). Restoring the store returns an exact staged differential (`stdout_delta=0`, both exits zero, exact stderr). The fresh Phase 7 gate passed at `1,770/1,770` registered coverage assertions and the immutable public-header check. Raw logs, including the test-first red control, are in [`evidence/raw/phase7-npscene-set-user-notify-helper-000545-20261013/`](raw/phase7-npscene-set-user-notify-helper-000545-20261013/).
