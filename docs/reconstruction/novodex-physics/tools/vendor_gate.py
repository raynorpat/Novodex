#!/usr/bin/env python3
"""The fast gate for the vendored qhull and OPCODE rows.

Their sources are merged (upstream + NovodeX overlay) into build/External/<lib>-tree at configure time and compiled from
there, so a mutation is made in the merged copy and the whole round (relink, run) takes seconds. The gate is
NxPhysicsThirdPartyTests, whose output is one `thirdparty name=<family> ...` line per family; a row is detected when a line
of one of its families differs from the unmutated run, which also lets rows with disjoint families share a build.
"""
import hashlib
import re
from pathlib import Path

ROOTS = (
    ('External/qhull/upstream/src/', 'build/External/qhull-tree/'),
    ('External/qhull/novodex/', 'build/External/qhull-tree/'),
    ('External/opcode/upstream/Opcode/', 'build/External/opcode-tree/'),
    ('External/opcode/novodex/', 'build/External/opcode-tree/'),
)
NAME = re.compile(r'\bname=([A-Za-z0-9_.]+)')
VOLATILE = ('pair_directory=', 'modules pair=', 'loaded module=')


def merged_path(rel):
    """The merged-tree copy of a vendored source path, or None when the path is not vendored source."""
    rel = (rel or '').replace(chr(92), '/')
    for src, dst in ROOTS:
        if rel.startswith(src):
            return dst + rel[len(src):]
    return None


def parse_lines(stdout):
    """{family: line} for the lines of the target's output that name a family."""
    out = {}
    for line in stdout.splitlines():
        if line.startswith(VOLATILE):
            continue
        m = NAME.search(line)
        if m:
            out[m.group(1)] = line
    return out


def changed_families(base, now):
    return {k for k in set(base) | set(now) if base.get(k) != now.get(k)}


def detection(base, now, families):
    """The number of changed families among the row's own, or None. A row with no named family sees every change."""
    changed = changed_families(base, now)
    if families:
        changed &= set(families)
    return len(changed) or None


def row_families(differential, names):
    """The families named in a coverage row's `differential` column that the target prints, sorted."""
    found = set(re.findall(r'(?:^|[:+;])([a-z0-9_]+)=(?:exact|discrete|divergent)', differential or ''))
    return sorted(found & set(names))


def pack(rows, size):
    """Batches of at most `size` rows whose family sets are pairwise disjoint; a row with no family goes alone."""
    packs = []
    alone = [r for r in rows if not r['families']]
    for r in sorted((r for r in rows if r['families']), key=lambda r: -len(r['families'])):
        for p in packs:
            used = set().union(*[set(x['families']) for x in p])
            if len(p) < size and not used & set(r['families']):
                p.append(r)
                break
        else:
            packs.append([r])
    return packs + [[r] for r in alone]


def _digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def dirty_tree_files(tree, upstream, overlay):
    """Files of a merged tree whose bytes differ from the source they were merged from (the overlay wins)."""
    tree, upstream, overlay = Path(tree), Path(upstream), Path(overlay)
    dirty = []
    for f in sorted(p for p in tree.rglob('*') if p.is_file()):
        rel = f.relative_to(tree)
        src = overlay / rel if (overlay / rel).is_file() else upstream / rel
        if src.is_file() and _digest(f) != _digest(src):
            dirty.append(str(rel).replace(chr(92), '/'))
    return dirty
