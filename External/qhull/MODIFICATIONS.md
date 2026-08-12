# qhull 2003.1 — what NovodeX changed

Every entry below is a file under `novodex/` that replaces the file of the same
name under `upstream/src/`. Diff the two to see the change; the file's own
`NOVODEX LOCAL MODIFICATION` header carries the addresses that establish it.

The pinned tree compiles unmodified under a 2026 MSVC. Everything here exists
because the shipped `NxPhysics.dll` does something stock 2003.1 does not.

| file | change | established at |
|---|---|---|
| `user.h` | `fprintf` redirected to the host object's slot `+0x10` | 587 of the 601 classified call sites through `.data:0x00125080` |
| `mem.c` | `malloc` and `free` routed to slots `+0x14` and `+0x18` | `0x0006dade`, `0x0006de24`, `0x0006dc74` |
| `user.c` | `qh_errexit`'s body replaced by a call on slot `+0x20` forwarding only the exit code; the stock body is kept under `#if 0` | `0x00084800` |
| `QhullNovodeXHost.h` | **added file**, no upstream counterpart: the four hooks | `0x0007ea51`, `.rdata:0x00113614` |

## One object, four slots

All four hooks are the same object: a NovodeX class with a nine-slot vtable at
`.rdata:0x00113614`, constructed at `0x0007e370` with a 16,384-byte inline
arena, held in a global at `.data:0x00125080` that is written **exactly once**
in the whole image, at `0x0007ea51`.

Recounted for this task over all 683 occurrences of that global in `.text`
(601 classified to a slot):

| slot | call sites | what it is |
|---|---:|---|
| `+0x10` | 587 | `fprintf` — `qh ferr` / `qh fout` |
| `+0x04` | 6 | emits three floats — a geometry dump |
| `+0x14` | 2 | `malloc`, both in `mem.c`'s band |
| `+0x18` | 1 | `free`, in `mem.c`'s band |
| `+0x20` | 1 | error exit, in `user.c` |
| `+0x00`, `+0x08`, `+0x0c`, `+0x1c` | 1 each | — |

So the object is an **I/O shim with allocation attached**, not an allocator: 587
of 601 classified sites print. An earlier pass reported 561 / 4 / 3 for the
first three slots. Those are close but are not what this pass measures, and the
difference is in the classifier rather than in the image — neither pass can
attribute a load that feeds two calls without ambiguity. What is unambiguous:
malloc and free survive in `mem.c` only, the error exit in `user.c` only, and
printing everywhere.

The object itself has no upstream counterpart and is **Task 2b's**, along with
the driver at `0x0007d420` that calls `qh_init_A` + `qh_initflags` the way
`unix.c`'s `main()` does, and the OBJ writers at `0x0007df20`/`0x0007dea0`.

## Claims checked and found to be STOCK — do not "restore" these

| claim | verdict |
|---|---|
| `qh_MAXnarrow` was changed from `-0.99999999` to `-0.999999999999999` | **stock, and a conflation of two constants.** Both are present at their stock values — `qh_MAXnarrow` at `.rdata:0x00111a80` (`user.h:726`) and `qh_WARNnarrow` at `.rdata:0x00111a68` (`user.h:738`) — and both are read from the same row `0x000793f0`, at `0x000795ed` and `0x00079630`, which is `poly2.c:1790` then `:1795` |
| `0x00084800` is the free path | **it is `qh_errexit`.** Seventeen bytes, cdecl, forwarding only its first argument although its callers pass three; `qh_memalloc` reaches it as `push edi; push edi; push 4` = `qh_errexit(qhmem_ERRmem, NULL, NULL)`. The free path is `qh_memfree` at `0x0006dc10` |
| the build's `#define`s were tuned | **all stock.** `qh_QHpointer` 0, `qh_KEEPstatistics` set, `qh_NOmerge`/`qh_NOmem`/`qh_NOtrace` unset, `realT` = `double`, `qh_MEMalign` 8, `qh_MEMbufsize` `0x10000`, `qh_MEMinitbuf` `0x20000`, 18 size classes, `qh_RANDOMmax` 2147483646 |

## Licence

The Geometry Center licence's clause 3 requires a notice naming who modified
qhull, when, and why, for **every** modification — NovodeX's and this project's.
That notice is `NOTICE.txt`, beside this file, and each modified file carries an
in-file marker below (never in place of) the copyright header clause 1 protects.
