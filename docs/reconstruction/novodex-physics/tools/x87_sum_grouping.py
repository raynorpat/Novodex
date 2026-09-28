#!/usr/bin/env python3
"""Record how every three-product x87 sum in the vendored rows is grouped, oracle and candidate.

The matcher compares which x87 operation classes a function executes, not the order in which it
adds. That order is not free: `(a+b)+c` and `(a+c)+b` round differently, and the 2003 compiler
that built the oracle reassociated float additions, where a 2026 /fp:precise build keeps the
source order. This tool makes that visible, as report-only evidence.

Method. Each matched group's code (every map row of the group on the oracle side, the candidate
symbol on the other) is read in address order by a small symbolic x87 evaluator: loads push
leaves, arithmetic builds trees, stores to memory remember the tree so a later load of the same
slot gets it back, and a call empties the stack. Branches are not followed, so a value merged
from two paths is only one of them; the evaluator is for recording sums, not for proof. Every
addition whose operands are `(product + product)` and `product` is a site. Its grouping names
which two products are summed first:

  (x+y)+z   the source order of `a.x*b.x + a.y*b.y + a.z*b.z`
  (y+z)+x   the last component first, then back to x
  (x+z)+y   the outer pair first

A product is named x, y or z by the memory slots its two operands were loaded from: the three
products must each read one slot of a common base at displacements d, d+4 and d+8 (a Point, or
a spilled Point), and the rank of the slot names the component. A site where no such base exists
is "?". The key column is a label-free identity of the three products (the displacements of
every non-stack leaf in each), used to pair an oracle site with a candidate site.

Output: evidence/phase4-third-party-map/sum_grouping.csv, one line per site. main() returns 0
on success and 2 when an input is missing.
"""

import argparse
import csv
import json
import sys
from collections import Counter
from pathlib import Path

import capstone
from capstone import x86

import vendored_match as vm

OUTPUT = vm.MAP_DIR / "sum_grouping.csv"
STACK_BASES = frozenset({"esp", "ebp"})


class Node:
    __slots__ = ("op", "kids", "leaf", "loc")

    def __init__(self, op, kids=(), leaf=None):
        self.op, self.kids, self.leaf = op, tuple(kids), leaf
        self.loc = leaf        # the memory slot the value was loaded from, if any


def _leaves(node, out, depth=0):
    if depth > 12:
        return out
    if node.leaf is not None:
        out.append(node.leaf)
    for kid in node.kids:
        _leaves(kid, out, depth + 1)
    return out


def _memkey(insn, op):
    base = insn.reg_name(op.mem.base) if op.mem.base else ""
    index = insn.reg_name(op.mem.index) if op.mem.index else ""
    return (base, index, op.mem.scale if index else 0, vm._signed(op.mem.disp, 4))


def _located(node, key):
    if node is None:
        return None
    copy = Node(node.op, node.kids, node.leaf)
    copy.loc = key
    return copy


def label_terms(products):
    """x/y/z for three product nodes from the slots of their direct operands, or None."""
    slots = [{k.loc for k in p.kids if k.loc is not None and k.loc[0] != "const"}
             for p in products]
    bases = set.intersection(*({s[:3] for s in group} for group in slots)) if all(slots) else set()
    for base in sorted(bases, key=str):
        options = [sorted(s[3] for s in group if s[:3] == base) for group in slots]
        for d0 in options[0]:
            for d1 in options[1]:
                for d2 in options[2]:
                    v = sorted((d0, d1, d2))
                    if v[1] - v[0] == 4 and v[2] - v[1] == 4:
                        rank = {v[0]: "x", v[1]: "y", v[2]: "z"}
                        return [rank[d0], rank[d1], rank[d2]]
    return None


def term_key(product):
    return ",".join(sorted(str(l[3]) + ("i" if l[1] else "")
                           for l in _leaves(product, []) if l[0] not in STACK_BASES | {"const"}))


def scan(image, start, end):
    """[(site rva, grouping, key)] for the code in [start, end)."""
    section = image.section_of(start)
    if section is None:
        return []
    data = section.data[start - section.rva:end - section.rva]
    cs = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    cs.detail = True
    stack, memory, found = [], {}, []

    def st(i):
        return stack[-1 - i] if i < len(stack) else Node("?")

    def put(i, value):
        if i < len(stack):
            stack[-1 - i] = value

    def push(value):
        stack.append(value)
        del stack[:-8]

    def pop():
        if stack:
            stack.pop()

    def combine(kind, a, b, rva):
        node = Node(kind, (a, b))
        if kind == "add":
            for pair, single in ((a, b), (b, a)):
                if (pair.op == "add" and single.op == "mul"
                        and all(k.op == "mul" for k in pair.kids)):
                    products = [pair.kids[0], pair.kids[1], single]
                    labels = label_terms(products)
                    if labels:
                        first = "".join(sorted(labels[:2]))
                        grouping = f"({first[0]}+{first[1]})+{labels[2]}"
                    else:
                        grouping = "?"
                    key = "|".join(sorted(term_key(p) for p in products[:2])) \
                        + "/" + term_key(products[2])
                    found.append((rva, grouping, key))
                    break
        return node

    for insn in cs.disasm(data, image.base + start):
        rva = insn.address - image.base
        m = insn.mnemonic
        if m == "call" or m.startswith("ret") or m == "int3":
            stack.clear()
            if m == "int3":
                break
            continue
        if not m.startswith("f"):
            continue
        ops = insn.operands
        mem = next((o for o in ops if o.type == x86.X86_OP_MEM), None)
        regs = [insn.reg_name(o.reg) for o in ops if o.type == x86.X86_OP_REG]
        sti = [int(r[3]) if r.startswith("st(") else 0 for r in regs]
        if m in ("fld", "fild"):
            if mem is not None:
                key = _memkey(insn, mem)
                if not key[0] and not key[1]:
                    push(Node("leaf", leaf=("const", "", 0, key[3])))
                else:
                    push(_located(memory.get(key), key) or Node("leaf", leaf=key))
            else:
                push(st(sti[0]))
        elif m in ("fldz", "fld1", "fldpi"):
            push(Node("leaf", leaf=("const", "", 0, m)))
        elif m in ("fst", "fstp", "fist", "fistp"):
            if mem is not None:
                memory[_memkey(insn, mem)] = st(0)
            else:
                put(sti[0], st(0))
            if m.endswith("p"):
                pop()
        elif m == "fxch":
            # capstone lists `fxch st(i)` as (st(0), st(i)): the exchanged register is the
            # non-zero index
            i = max(sti) if sti else 1
            a, b = st(0), st(i)
            put(0, b)
            put(i, a)
        elif m in ("fchs", "fabs", "fsqrt"):
            put(0, Node(m, (st(0),)))
        elif m.rstrip("p") in ("fadd", "fmul", "fsub", "fsubr", "fdiv", "fdivr"):
            kind = {"fadd": "add", "fmul": "mul"}.get(m.rstrip("p"), "other")
            if mem is not None:
                key = _memkey(insn, mem)
                other = _located(memory.get(key), key) or Node(
                    "leaf", leaf=key if (key[0] or key[1]) else ("const", "", 0, key[3]))
                put(0, combine(kind, st(0), other, rva))
            elif m.endswith("p"):
                i = sti[0] if sti else 1
                put(i, combine(kind, st(i), st(0), rva))
                pop()
            elif len(sti) == 2:
                put(sti[0], combine(kind, st(sti[0]), st(sti[1]), rva))
            elif len(sti) == 1:
                put(0, combine(kind, st(0), st(sti[0]), rva))
        elif m in ("fcomp", "fucomp", "ficomp", "fcomip", "fucomip"):
            pop()
        elif m in ("fcompp", "fucompp"):
            pop()
            pop()
        elif m in ("ffree", "fninit"):
            stack.clear()
    return found


def pair_sites(oracle_sites, candidate_sites):
    """(same, different) counts over oracle sites whose three products the candidate also sums."""
    by_terms = {}
    for _, grouping, key in candidate_sites:
        pair, single = key.split("/")
        terms = tuple(sorted(pair.split("|") + [single]))
        by_terms.setdefault(terms, set()).add(key)
    same = different = 0
    for _, grouping, key in oracle_sites:
        pair, single = key.split("/")
        terms = tuple(sorted(pair.split("|") + [single]))
        if "" in terms or terms not in by_terms:
            continue
        if key in by_terms[terms]:
            same += 1
        else:
            different += 1
    return same, different


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--oracle", default=str(vm.DEFAULT_ORACLE))
    parser.add_argument("--candidate",
                        default=str(vm.REPO_DIR / "build" / "Release" / "NxPhysics.dll"))
    parser.add_argument("--candidate-map",
                        default=str(vm.REPO_DIR / "build" / "Release" / "NxPhysics.map"))
    parser.add_argument("--map-dir", default=str(vm.MAP_DIR))
    parser.add_argument("--output", default=str(OUTPUT))
    args = parser.parse_args(argv)
    try:
        oracle = vm.Image.from_pe(args.oracle)
        candidate = vm.Image.from_pe(args.candidate)
        text = Path(args.candidate_map).read_text(encoding="utf-8", errors="replace")
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    index = vm.SymbolIndex(vm.parse_map(text, candidate.base, candidate))
    lines = []
    totals = {}
    for library in sorted(vm.LIBRARIES):
        rows = vm.load_map_rows(library, args.map_dir)
        vm.resolve_rows(rows, index)
        counts = {"oracle": Counter(), "candidate": Counter(), "same": 0, "different": 0}
        for key, group in vm.group_rows(rows).items():
            group.sort(key=lambda r: r.rva)
            head = group[0]
            o_sites = [s for r in group for s in scan(oracle, r.rva, r.rva + r.size)]
            c_sites = scan(candidate, head.symbol.rva, head.symbol.end) if head.symbol else []
            same, different = pair_sites(o_sites, c_sites)
            counts["same"] += same
            counts["different"] += different
            for side, sites in (("oracle", o_sites), ("candidate", c_sites)):
                for rva, grouping, term in sites:
                    counts[side][grouping] += 1
                    lines.append([library, f"0x{head.rva:08x}", head.source_function, side,
                                  f"0x{rva:08x}", grouping, term])
        totals[library] = counts
    with open(args.output, "w", encoding="utf-8", newline="") as handle:
        writer = csv.writer(handle, lineterminator="\n")
        writer.writerow(["library", "group_rva", "source_function", "side", "site_rva",
                         "grouping", "terms"])
        writer.writerows(lines)
    for library, counts in totals.items():
        print(f"{library}: oracle {dict(counts['oracle'])} candidate {dict(counts['candidate'])} "
              f"paired same={counts['same']} different={counts['different']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
