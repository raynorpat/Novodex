#!/usr/bin/env python3
"""Attribute every qhull print site in the oracle to a traceN macro or a plain fprintf.

The oracle's qhull span prints two ways: a direct call to the static CRT's fprintf
(0x000f4d5a) and a virtual call through the NovodeX host object at slot +0x10. For each such
call this tool takes the format string pushed before it (the last push, within 14
instructions, of an address in qhull's .rdata literal pool), finds that string in the upstream
qhull 2003.1 sources, and asks whether one of the four source lines ending at the string is
inside a `traceN((...))` macro. A string found in no source file, or a call with no pushed
pool address, is "unattributed".

Result on the pinned oracle (evidence/vendored-correspondence.md): of 217 CRT sites, 208 are in a
trace macro, 2 are not and 7 are unattributed; of 616 host sites, 8 are, 555 are not and 53 are
unattributed.
"""

import argparse
import re
import sys
from pathlib import Path

import capstone
from capstone import x86

import vendored_match as vm

QHULL_SPAN = (0x0005c5c0, 0x00084a50)
QHULL_POOL = (0x001088dc, 0x00115a8c)       # phase4-third-party.md section 3.1
CRT_FPRINTF = 0x000f4d5a
DEFAULT_UPSTREAM = vm.REPO_DIR / "External" / "qhull" / "upstream" / "src"
TRACE_MACRO = re.compile(r"\btrace\d\s*\(")


def c_literal(text):
    """The text as it is spelled inside a C string literal."""
    return (text.replace("\\", "\\\\").replace("\n", "\\n").replace("\r", "\\r")
            .replace("\t", "\\t").replace('"', '\\"'))


def in_trace_macro(sources, fmt, prefix=40, lines=4):
    """True/False when the format string is found in a source, None when it is not."""
    key = c_literal(fmt[:prefix])
    for text in sources.values():
        at = text.find(key)
        if at >= 0:
            context = text[:at].split("\n")[-lines:]
            return any(TRACE_MACRO.search(line) for line in context)
    return None


def print_sites(image, span=QHULL_SPAN):
    """[(call rva, "crt"|"host", format string or None)] over the span."""
    section = image.section_of(span[0])
    code = section.data[span[0] - section.rva:span[1] - section.rva]
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    insns = list(md.disasm(code, span[0]))
    sites = []
    for k, insn in enumerate(insns):
        if insn.mnemonic != "call":
            continue
        if insn.op_str == f"{CRT_FPRINTF:#x}":
            kind = "crt"
        elif insn.op_str.endswith("+ 0x10]"):
            kind = "host"
        else:
            continue
        fmt = None
        for prior in reversed(insns[max(0, k - 14):k]):
            m = re.fullmatch(r"0x([0-9a-f]+)", prior.op_str)
            if prior.mnemonic == "push" and m:
                rva = int(m.group(1), 16) - image.base
                if QHULL_POOL[0] <= rva < QHULL_POOL[1]:
                    fmt = image.string_at(rva)
                    break
        sites.append((insn.address, kind, fmt))
    return sites


def tally(sites, sources):
    counts = {(kind, verdict): 0 for kind in ("crt", "host")
              for verdict in ("trace", "plain", "unattributed")}
    for _, kind, fmt in sites:
        verdict = in_trace_macro(sources, fmt) if fmt else None
        counts[(kind, {True: "trace", False: "plain", None: "unattributed"}[verdict])] += 1
    return counts


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--oracle", default=str(vm.DEFAULT_ORACLE))
    parser.add_argument("--upstream", default=str(DEFAULT_UPSTREAM))
    args = parser.parse_args(argv)
    try:
        image = vm.Image.from_pe(args.oracle)
        sources = {p.name: p.read_text(encoding="latin-1")
                   for p in sorted(Path(args.upstream).glob("*.c"))}
    except OSError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    if not sources:
        print(f"error: no qhull sources under {args.upstream}", file=sys.stderr)
        return 2
    sites = print_sites(image)
    counts = tally(sites, sources)
    for kind in ("crt", "host"):
        total = sum(v for (k, _), v in counts.items() if k == kind)
        print(f"{kind}: sites={total} trace={counts[(kind, 'trace')]} "
              f"plain={counts[(kind, 'plain')]} unattributed={counts[(kind, 'unattributed')]}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
