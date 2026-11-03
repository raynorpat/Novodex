#!/usr/bin/env python3
"""The fast gate for the vendored qhull and OPCODE rows.

Their sources are merged (upstream + NovodeX overlay) into build/External/<lib>-tree at configure time and compiled from
there, so a mutation is made in the merged copy and the whole round (relink, run) takes seconds. The gate is
NxPhysicsThirdPartyTests, whose output is one `thirdparty name=<family> ...` line per family; a row is detected when a line
of one of its families differs from the unmutated run, which also lets rows with disjoint families share a build.
"""
import hashlib
import os
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


def build_dir():
    """The CMake build directory the merged trees live in (NX_BUILD_DIR, so several workers can each have their own)."""
    return os.environ.get('NX_BUILD_DIR', 'build')


def merged_path(rel):
    """The merged-tree copy of a vendored source path, or None when the path is not vendored source."""
    rel = (rel or '').replace(chr(92), '/')
    for src, dst in ROOTS:
        if rel.startswith(src):
            return build_dir() + dst[len('build'):] + rel[len(src):]
    return None


def _remainder(rel):
    rel = (rel or '').replace(chr(92), '/')
    for src, dst in ROOTS:
        if rel.startswith(src):
            return dst, rel[len(src):]
    return None, None


def same_source(a, b):
    """True when two repository paths are the same vendored file: one upstream, one the overlay that shadows it."""
    da, ra = _remainder(a)
    db, rb = _remainder(b)
    return da is not None and da == db and ra == rb


def source_file(row, repo):
    """The repository file a vendored row's mutation actually lands in: the overlay when one shadows the upstream file."""
    rel = (row.get('implementation') or row.get('source') or '').replace(chr(92), '/')
    dst, rest = _remainder(rel)
    if dst is None:
        return rel
    for src, d in ROOTS:
        if d == dst and 'novodex' in src and (Path(repo) / (src + rest)).is_file():
            return src + rest
    return rel


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
    found = set(re.findall(r'(?:^|[:+;])([a-z0-9_.]+)=(?:exact|discrete|divergent|lastbit)', differential or ''))
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


def restore_tree(tree, upstream, overlay):
    """Copy the source back over every merged-tree file that differs from it (after a killed run); the names restored."""
    import shutil
    tree, upstream, overlay = Path(tree), Path(upstream), Path(overlay)
    dirty = dirty_tree_files(tree, upstream, overlay)
    for rel in dirty:
        src = overlay / rel if (overlay / rel).is_file() else upstream / rel
        shutil.copyfile(src, tree / rel)
    return dirty


def check_trees(repo, build=None):
    """Restore both merged trees of a build directory from their sources; the files that had to be restored."""
    build = build or build_dir()
    repo = Path(repo)
    out = []
    for lib, up, ov in (('qhull', 'External/qhull/upstream/src', 'External/qhull/novodex'),
                        ('opcode', 'External/opcode/upstream/Opcode', 'External/opcode/novodex')):
        tree = repo / build / 'External' / ('%s-tree' % lib)
        if tree.is_dir():
            out += [lib + '/' + f for f in restore_tree(tree, repo / up, repo / ov)]
    return out
