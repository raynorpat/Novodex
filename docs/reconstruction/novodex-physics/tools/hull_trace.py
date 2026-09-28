#!/usr/bin/env python3
"""cdb execution trace of the NovodeX hull library in NxPhysicsThirdPartyTests (qhull-gap Task 4e).

The candidate side of the convex-cooking families (tests/PhysicsThirdPartyTests.cpp,
nxDriveConvexCooking and nxDriveConvexCookingBytes) is the hull library linked into the test
exe itself: Physics/src/QhullHost.cpp (through tests/PhysicsThirdPartyHost.cpp),
Physics/src/Quantizer.cpp and Physics/src/TriangleMesh.cpp. This tool traces that exe with one
counting breakpoint per hull-library function, reusing vendored_trace.py's script writer
(counter page at 0x60000000, a boundary at nxReport printing `SEG <family>` and dumping the
counters) and its log parser, over these functions instead of the matched vendored groups.

    python tools/hull_trace.py --repo <repo> --work <scratch directory> [--cap 2000]

It writes <work>/hull.cdb, <work>/hull-cdb.log and <work>/hull-trace.json, and the committed
excerpt evidence/qhull-gap-trace-cooking.txt. main() returns 0, or 1 when a traced row was not
hit in any hull_ family.
"""

import argparse
import hashlib
import json
import subprocess
import sys
from collections import OrderedDict
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import vendored_trace as vt  # noqa: E402

CDB = r"C:\Program Files (x86)\Windows Kits\10\Debuggers\x86\cdb.exe"
ORACLE_DIR = r"D:\FlamingEnt__\Unreal_3\Binaries"
ORACLE_SHA = "4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c"

# (row ids, candidate symbol in the exe's map, descriptive name). A continuation shares its
# entry's function and so its breakpoint.
ROWS = [
    (["003236"], "?runQhull@@YAHHPAPADHPBM@Z", "runQhull"),
    (["003238"], "?releaseArrays@QhullHost@@QAEXXZ", "QhullHost::releaseArrays"),
    (["003240"], "?rawAlloc@QhullHost@@QAEPAXI@Z", "QhullHost::rawAlloc"),
    (["003241"], "?rawFree@QhullHost@@QAEXPAX@Z", "QhullHost::rawFree"),
    (["003243", "003245"], "?cleanupVertices@QhullHost@@QAE_NIPBMIAAIPAMM2_N3I@Z", "QhullHost::cleanupVertices"),
    (["003247", "003249"], "?writeOkObj@QhullHost@@QAEXABUHullResult@@@Z", "QhullHost::writeOkObj"),
    (["003251"], "?writeFailObj@QhullHost@@QAEXIPBMI@Z", "QhullHost::writeFailObj"),
    (["003253"], "?boxFallback@QhullHost@@QAEXAAIPAM@Z", "QhullHost::boxFallback"),
    (["003255"], "?ReleaseResult@HullLibrary@@QAE?AW4HullError@@AAUHullResult@@@Z", "HullLibrary::ReleaseResult"),
    (["003259"], "?offBegin@QhullHost@@UAEXHIIH@Z", "QhullHost::offBegin"),
    (["003261"], "?point3@QhullHost@@UAEXMMM@Z", "QhullHost::point3"),
    (["003263"], "?print@QhullHost@@UAAHPAU_iobuf@@PBDZZ", "QhullHost::print"),
    (["003265"], "?size@QhullHost@@UAEXMM@Z", "QhullHost::size"),
    (["003267"], "?errexit@QhullHost@@UAEXH@Z", "QhullHost::errexit"),
    (["003268"], "?facet@QhullHost@@UAEXIPBI@Z", "QhullHost::facet"),
    (["003270"], "?buildResult@QhullHost@@QAE_NAAUHullResult@@_N1@Z", "QhullHost::buildResult"),
    (["003272"], "?trackedMalloc@QhullHost@@UAEPAXI@Z", "QhullHost::trackedMalloc"),
    (["003275"], "?trackedFree@QhullHost@@UAEXPAX@Z", "QhullHost::trackedFree"),
    (["003277"], "??1QhullHost@@QAE@XZ", "QhullHost::~QhullHost"),
    (["003279"], "?CreateConvexHull@HullLibrary@@QAE?AW4HullError@@ABUHullDesc@@AAUHullResult@@@Z",
     "HullLibrary::CreateConvexHull"),
    (["003347", "003349", "003351"], "?M3d@WuQuantizer@@QAEXPAH000PAM@Z", "WuQuantizer::M3d"),
    (["003353"], "?Vol@WuQuantizer@@QAEHPBUWuBox@@PBH@Z", "WuQuantizer::Vol"),
    (["003355"], "?Bottom@WuQuantizer@@QAEHPBUWuBox@@EPBH@Z", "WuQuantizer::Bottom"),
    (["003357"], "?Top@WuQuantizer@@QAEHPBUWuBox@@EHPBH@Z", "WuQuantizer::Top"),
    (["003359"], "?Var@WuQuantizer@@QAENPBUWuBox@@@Z", "WuQuantizer::Var"),
    (["003361"], "?Maximize@WuQuantizer@@QAENPBUWuBox@@EHHPAHHHHH@Z", "WuQuantizer::Maximize"),
    (["003363"], "?Cut@WuQuantizer@@QAEHPAUWuBox@@0@Z", "WuQuantizer::Cut"),
    (["003367"], "?Quantize@WuQuantizer@@QAEIPAEI@Z", "WuQuantizer::Quantize"),
    (["003369", "003371"], "?reduceVertices@HullVertexReducer@@QAEPAV1@PAVHullAllocator@@IPBMAAIPAMI@Z",
     "HullVertexReducer::reduceVertices"),
    (["002233"], "?computeHull@TriangleMeshHullAllocator@@QAE_NABVNxTriangleMeshDesc@@AAV2@@Z",
     "TriangleMeshHullAllocator::computeHull"),
    (["002235"], "?malloc@TriangleMeshHullAllocator@@UAEPAXI@Z", "TriangleMeshHullAllocator::malloc"),
    (["002237"], "?free@TriangleMeshHullAllocator@@UAEXPAX@Z", "TriangleMeshHullAllocator::free"),
]
# No out-of-line call in the hull runs: credited through the caller they are inlined into.
INLINED = OrderedDict([
    ("003257", "QhullHost::QhullHost       inlined into HullLibrary::CreateConvexHull (003279)"),
    ("003274", "BlockHeader::init          inlined into QhullHost::trackedMalloc (003272)"),
    ("003365", "WuQuantizer::Hist3d        inlined into HullVertexReducer::reduceVertices (003369)"),
    ("003277", "QhullHost::~QhullHost      inlined into 003279 in the hull runs; its out-of-line\n"
               "                                             copy runs in hull_host_size (the harness's host)"),
])
MARK = "?nxDriveConvexCooking@@YAXABUNxOracleRows@@_N@Z"


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def run(repo, work, cap):
    exe_path = repo / "build/Release/NxPhysicsThirdPartyTests.exe"
    exe = vt.Mapped.load(str(exe_path), str(repo / "build/Release/NxPhysicsThirdPartyTests.map"))
    at = OrderedDict()
    for rows, symbol, _ in ROWS:
        s = exe.function(symbol)
        if s is None:
            raise SystemExit(f"hull_trace: {symbol} is not in the exe's map")
        at.setdefault(s.rva, []).extend(rows)
    boundary = exe.function(vt.BOUNDARY_SYMBOL)
    marks = [(exe.function(MARK).rva, "cooking")]
    script = work / "hull.cdb"
    vt.write_script(at, exe.entry_rva, boundary.rva, cap, script, marks)
    log = work / "hull-cdb.log"
    with open(log, "w", encoding="utf-8", errors="replace") as f:
        subprocess.run([CDB, "-cf", str(script), str(exe_path), ORACLE_DIR, ORACLE_SHA],
                       stdout=f, stderr=subprocess.STDOUT, cwd=str(exe_path.parent))
    text = log.read_text(encoding="utf-8", errors="replace")
    segments = vt.parse_log(text, len(at))
    result = OrderedDict()
    for label, counts in segments:
        hits = result.setdefault(label, OrderedDict())
        for (rva, rows), count in zip(at.items(), counts):
            if count:
                for r in rows:
                    hits[r] = hits.get(r, 0) + count
    trace = {"exe_sha256": sha256(exe_path), "dll_sha256": sha256(repo / "build/Release/NxPhysics.dll"),
             "cap": cap, "breakpoints": [{"rva": f"0x{rva:08x}", "rows": rows} for rva, rows in at.items()],
             "segments": result, "families": vt.parse_families(text), "inlined": list(INLINED)}
    (work / "hull-trace.json").write_text(json.dumps(trace, indent=1) + "\n", encoding="utf-8")
    return trace, text, script.read_text(encoding="ascii").splitlines()


def excerpt(trace, log, script, out):
    names = {rows[0]: name for rows, _, name in ROWS}
    cap = trace["cap"]
    L = ["Convex cooking: cdb execution trace of the hull library (qhull-gap Tasks 4e and 5)",
         "=" * 84, "",
         "The execution evidence behind the dynamic_proof of the 40 hull-library rows written in",
         "qhull-gap Task 4 (phys_fn_002233/002235/002237 and 003236-003371; units/convex-cooking-",
         "contract.md). Families and their attribution: evidence/qhull-gap.md, \"Task 4e\" and \"Task 5\".",
         "Generated by tools/hull_trace.py.", "",
         "METHOD. The candidate side of NxPhysicsThirdPartyTests' convex-cooking families is the hull",
         "library linked into the test exe itself (Physics/src/QhullHost.cpp through",
         "tests/PhysicsThirdPartyHost.cpp, Physics/src/Quantizer.cpp, Physics/src/TriangleMesh.cpp),",
         "so the exe is what is traced. tools/vendored_trace.py's script writer and log parser are",
         "reused over these functions instead of the matched vendored groups: one counting",
         "breakpoint per hull-library function, taken from the exe's linker map",
         "(build/Release/NxPhysicsThirdPartyTests.map, relative to $exentry; the exe has no PDB), a",
         "counter page at 0x60000000, a boundary at nxReport that prints `SEG <family>` and dumps and",
         "clears the counters, and a `cooking` mark at nxDriveConvexCooking. The counts under a SEG",
         "line are the hits since the previous boundary, that is, the executions the named family",
         f"compares. A counter stops at the cap ({cap}) until the next boundary, so {cap} means \"at",
         f"least {cap}\". Rows sharing one candidate function (a continuation) share its breakpoint.", "",
         "Debugger:  Microsoft Windows Debugger 10.0.10586.567 (X86)",
         "Command:   python tools/hull_trace.py --repo <repo> --work <dir>, which runs",
         "           cdb -cf hull.cdb build\\Release\\NxPhysicsThirdPartyTests.exe <oracle dir> <sha256>",
         f"Exe:       build\\Release\\NxPhysicsThirdPartyTests.exe sha256={trace['exe_sha256']}",
         f"           (built with candidate build\\Release\\NxPhysics.dll sha256={trace['dll_sha256']};",
         "           the link timestamp changes both hashes on every rebuild, so they pin this build",
         "           only -- the families' oracle digests, registered in tools/gate_targets.ps1, are",
         "           the stable identity)",
         f"Oracle:    NxPhysics.dll sha256={ORACLE_SHA}", "",
         "Breakpoints (exe rva from the map; rows):"]
    for b in trace["breakpoints"]:
        L.append(f"  {b['rva']}  {' '.join('phys_fn_' + r for r in b['rows']):<44} {names.get(b['rows'][0], '')}")
    L += ["", "Credited through the caller they are inlined into (no out-of-line call in the hull runs):"]
    for row, text in INLINED.items():
        L.append(f"  phys_fn_{row}  {text}")
    L += ["", "Hits per family (family verdict from the harness's own line in the log):"]
    fams = trace["families"]
    for label, hits in trace["segments"].items():
        if not (label.startswith("hull_") or label == "cooking"):
            continue
        L.append(f"-- SEG {label} [{fams.get(label, {}).get('verdict', '-')}] --")
        if not hits:
            L.append("   (none: nothing of the hull library runs in this segment -- the family is filled by"
                     if label != "cooking" else
                     "   (none: nothing of the hull library runs between the last earlier family and the drive)")
            if label != "cooking":
                L.append("    the runs of the families reported just before it)")
        items = list(hits.items())
        for i in range(0, len(items), 6):
            L.append("   " + "  ".join(f"{r}:{c}" for r, c in items[i:i + 6]))
    L += ["", "The harness's lines for these families, from the same log (unedited):"]
    started = False
    for line in log.splitlines():
        line = line.strip()
        if line.startswith("HULL "):
            L.append("  " + line)
        if line.startswith("thirdparty name=hull_"):
            started = True
        if started and line.startswith("thirdparty "):
            L.append("  " + line)
    L += ["", "The cdb script (head):"] + ["  " + line for line in script[:6]] + ["  ..."]
    out.write_text("\n".join(L) + "\n", encoding="utf-8", newline="")


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    p.add_argument("--repo", required=True, type=Path)
    p.add_argument("--work", required=True, type=Path)
    p.add_argument("--cap", type=int, default=2000)
    args = p.parse_args(argv)
    args.work.mkdir(parents=True, exist_ok=True)
    trace, log, script = run(args.repo, args.work, args.cap)
    excerpt(trace, log, script,
            args.repo / "docs/reconstruction/novodex-physics/evidence/qhull-gap-trace-cooking.txt")
    every = {r for rows, _, _ in ROWS for r in rows}
    seen = {r for label, hits in trace["segments"].items() if label.startswith("hull_") for r in hits}
    print(f"hull_trace breakpoints={len(trace['breakpoints'])} rows={len(every)} hit={len(seen & every)}"
          f" exe={trace['exe_sha256']}")
    return 0 if every <= seen else 1


if __name__ == "__main__":
    sys.exit(main())
