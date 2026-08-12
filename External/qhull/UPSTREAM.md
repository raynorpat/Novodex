# qhull 2003.1 — upstream import

| | |
|---|---|
| archive | `qhull-2003.1.tar.gz` |
| archive SHA-256 | `c97d1982e6f5423379bf0ad6dd7293886ac69cae1596b59a305614aec4ae54f8` |
| obtained from | http://www.qhull.org |
| version literal | `src/global.c:45` — `char *qh_version = "2003.1 2003/12/30";` |

## Why this release and not another

`src/global.c:45`'s version literal is byte-identical to the string at
`.rdata:0x00109f90` in the pinned oracle `NxPhysics.dll`
(SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`).
The negative control, qhull 2002.1, carries a different literal.

## What was imported

`upstream/src/` is the archive's `src/` directory, complete — all 46 files,
including the eight command-line programs (`qconvex.c`, `qdelaun.c`, `qhalf.c`,
`qvoronoi.c`, `unix.c`, `rbox.c`, `user_eg.c`, `user_eg2.c`) that the build does
**not** compile and whose absence from the oracle is itself evidence: none of
their `"qhull internal warning (main): ..."` literals occurs in the image.

`upstream/COPYING.txt`, `README.txt`, `Announce.txt` and `REGISTER.txt` are the
archive's root files. The `html/` documentation tree, `eg/`, `index.htm` and
`QHULL-GO.pif` were not imported; they are documentation and a DOS shortcut, and
nothing in the build or the evidence refers to them.

`UPSTREAM-MANIFEST.sha256` lists the SHA-256 of every imported file.

## What the oracle says about the build that produced it

Read out of the image rather than assumed, and all stock:

`qh_QHpointer` = 0, `qh_KEEPstatistics` set, `qh_NOmerge` / `qh_NOmem` /
`qh_NOtrace` not set, `realT` = `double`, `qh_MEMalign` = 8,
`qh_MEMbufsize` = `0x10000`, `qh_MEMinitbuf` = `0x20000`, 18 size classes,
`qh_RANDOMmax` = 2147483646. `qh_MAXnarrow` and `qh_WARNnarrow` are at their
stock values too — an earlier pass claimed `qh_MAXnarrow` had been changed and
was wrong; both constants are in `.rdata` at `0x00111a80` and `0x00111a68` and
both are read from the same row.

Link order in the image is strictly alphabetical over the twelve library
translation units, which is how the span was tiled.
