"""convex-mesh gap Task 2d: compare the FPU instruction sequence of a built candidate
function with the oracle listing's.

Usage (from the repository root, after building NxPhysics Release Win32):

    dumpbin /disasm build\\NxPhysics.dir\\Release\\IceMeshBuilder2.obj > mb2obj.txt
    python docs/reconstruction/novodex-physics/evidence/convex-mesh-gap-2d-fpu-opcodes.py mb2obj.txt

For each row below it takes every x87 instruction (opcode bytes 0xd8..0xdf) of the
Capstone listing (oracle/capstone/manifest.json) inside the row's extent, and every
x87 instruction of the candidate function of that name in the dumpbin listing, in
address order, and compares them: a register form by its two bytes, a memory form
by its opcode byte and the ModRM reg field (the operation and operand size; the
addressing of the operand differs between the two builds by construction). It
prints EQUAL or the difference for each.
"""

import difflib
import json
import os
import re
import sys

ROWS = (
    ("?AddFace@MeshBuilder2@", 0x0002EDA0, 0x0002F290),        # 001597
    ("?ComputeNormals@MeshBuilder2@", 0x0002F5C0, 0x0002F9C0), # 001603 + 001605
    ("?SaveStreams@MeshBuilder2@", 0x0002F9C0, 0x0002FB40),    # 001607
    ("?OutputRun@MeshBuilder2@", 0x00030620, 0x00030D10),      # 001627
)


def fpu_key(hexbytes):
    b = bytes.fromhex(hexbytes)
    if not b or not 0xD8 <= b[0] <= 0xDF:
        return None
    if b[1] >= 0xC0:
        return "%02x%02x" % (b[0], b[1])
    return "%02x/%d" % (b[0], (b[1] >> 3) & 7)


def listing(manifest, lo, hi):
    keys = []
    for rva, ins in sorted(manifest.items()):
        if lo <= rva < hi:
            k = fpu_key(ins)
            if k:
                keys.append(k)
    return keys


def candidate(lines, prefix):
    keys, inside = [], False
    for line in lines:
        if re.match(r"^[?_].*:$", line):
            inside = line.startswith(prefix)
            continue
        if inside:
            m = re.match(r"^\s+[0-9A-F]+: ((?:[0-9A-F]{2} )+)", line)
            if m:
                k = fpu_key(m.group(1).replace(" ", ""))
                if k:
                    keys.append(k)
    return keys


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    with open(os.path.join(here, "..", "oracle", "capstone", "manifest.json"), encoding="utf-8") as f:
        m = json.load(f)
    manifest = {int(i["rva"], 16): i["bytes"] for i in m["instructions"]}
    with open(sys.argv[1], encoding="utf-8", errors="replace") as f:
        lines = f.read().split("\n")
    for prefix, lo, hi in ROWS:
        a, b = listing(manifest, lo, hi), candidate(lines, prefix)
        print("%s listing=%d candidate=%d %s" % (prefix, len(a), len(b), "EQUAL" if a == b else "DIFF"))
        if a != b:
            for d in difflib.unified_diff(a, b, lineterm="", n=2):
                print("  " + d)


if __name__ == "__main__":
    main()
