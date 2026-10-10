# Phase 6 closure: Revolute joint-level setters

The four-case staged Revolute fixture now directly exercises the public anchor, axis, breakability, limit-point, limit-plane, iterator-reset, purge, and name wrappers, with distinct values and readbacks. Each row was mutation-tested separately by omitting its wrapper call. Every mutation changed the registered transcript with `stdout_delta=2`; both processes exited 0 and stderr matched exactly. Every restored control returned `stdout_delta=0` with exact stderr.

| Row | Wrapper | Mutant SHA-256 | Restored SHA-256 |
| --- | --- | --- | --- |
| `phys_fn_004681` | `setGlobalAnchor` | `eb6ff4bbc7aade057d1abb2295f426871e9490ff4ed75e7bd4162f82dbe62066` | `68078ff51d86ec7d6e609a58f9d85423e34a83fc012e79b7ac437554b27a060d` |
| `phys_fn_004683` | `setGlobalAxis` | `ec78034d62e6aa8f5437967a78d442ce2ebb15425c3a44c9fad5d1e1c6eb08fc` | `dfaee7b8cb83fa760d06cd34960c96ba8bf2f70c8635e0c12f0ebb5c1fb1858f` |
| `phys_fn_004685` | `setBreakable` | `e345e033bd670f329865e6a53e4e5947e89f392ab2e6562696fe24946e0ecc59` | `fdbb0df2a0460e00296ea1b8c9a0715d885cf3d8381dc33a4657255c8b3ed003` |
| `phys_fn_004687` | `setLimitPoint` | `6c5115667fa7a9723765ef5c1ec78e20e7f3f40287c62e97c18fd7733498463b` | `30d4957c5cb63063392836678628d8c8e1f2fc8349a22fdc2ece87c384d0488f` |
| `phys_fn_004689` | `addLimitPlane` | `05d39980291f065744c6adf77d5ea36dc6cbc854179afc0e5946f485ea6fdca9` | `08510039b894f1467aec0b4a03adb3badfc8bc5a67efdc9dc3654b3f693ad83a` |
| `phys_fn_004691` | `resetLimitPlaneIterator` | `28da348f28b07d723fafb8e05fbe4ed3233906651d442f8d013cd56b2fdbe89e` | `51e0b33d659cc37d21b726ce4c55a8032876a2255b780ecb83ada7cf800ff7c5` |
| `phys_fn_004693` | `setName` | `0700be3f2bf99b3a437133a6f956dda73bb0d65c28c0ac84d135ad911708ce58` | `1a9279a3c5ff14ca6f37c63e799c380cb9103cf1b9002fe52841d709b5361fa6` |
| `phys_fn_004695` | `purgeLimitPlanes` | `7ad7b6afdb72e98d21fa42afaefbcb6434e6687e2d426ae3e35ee9c44c51446b` | `523175f602a88a46fda3acd4c590afbc43ea4dfd4fdb6f949b761ebfcb214ec5` |

The refreshed oracle-only joint matrix pins fixture source SHA-256 `dd394438da373c2dbae9f0df04220e8cabf3ef82e1035accc1c9114a7a1c349e`, 3,233 output lines, and digest `ebb13c0e366b7dbe57a9232b7116d989706e7bc38b275a19872e3fb2105392f2`.
