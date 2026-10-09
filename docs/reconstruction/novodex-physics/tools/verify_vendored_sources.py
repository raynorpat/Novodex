"""Check the vendored third-party trees in the implementation repository.

Phase 4 vendors qhull 2003.1 and OPCODE 1.3 rather than reconstructing them:
722 census rows and 377,842 bytes. Vendoring is only worth anything if two
things stay true, and neither of them stays true on its own.

    1. `External/<library>/upstream/` is the archive, byte for byte, forever.
       It is the reference every local modification is measured against, and a
       tree that has quietly drifted from the archive measures nothing. The
       committed file manifests under `External/<library>/` pin every
       imported source byte, so clean checkouts need no ignored archive copy.

    2. `External/<library>/novodex/` is exactly the local modifications, each
       carrying the oracle address that establishes it. Three ways that decays,
       and this rejects all three:
         - a novodex/ file that no longer differs from its upstream counterpart
           is a modification somebody lost;
         - a novodex/ file that differs but carries no NOVODEX LOCAL
           MODIFICATION block, or a block with no address in it, is a change
           nobody can check;
         - a novodex/ file naming an upstream counterpart that does not exist,
           other than the declared additions, is a file the build silently
           ignores.

The licence files are checked here too, for the same reason: they are the part
of a vendoring that is easiest to lose and hardest to notice missing.
"""

import argparse
import hashlib
import pathlib
import re
import sys

THIRD_PARTY_SOURCE_DIR = pathlib.Path("External")

UPSTREAM_MANIFEST_SHA256 = {
    "opcode": "e8fa2ad3cdeee2bc48d043683ecdc50271b5f3da170dc67a4697fbb46b4cb0dc",
    "qhull": "c6cb967e5b2d5f8e06c7ef074dd9b7b2133926441ffb8625347c212835ce9ee1",
}

# library -> (repo upstream root, pinned tree relative to the thirdparty dir)
LIBRARIES = {
    "qhull": ("External/qhull/upstream/src", "qhull-2003.1/src"),
    "opcode": ("External/opcode/upstream/Opcode", "opcode13/Opcode"),
}

# Files under novodex/ that deliberately have no upstream counterpart: the two
# host seam headers. Anything else without one is a mistake.
DECLARED_ADDITIONS = {
    "qhull": {"QhullNovodeXHost.h"},
    "opcode": {"OpcodeNovodeXHost.h"},
}

MARKER = "NOVODEX LOCAL MODIFICATION"
ADDRESS = re.compile(r"0x[0-9a-f]{8}")

# Licence obligations, per library. Each entry is (path, required substrings).
LICENCE_FILES = {
    "qhull": [
        ("External/qhull/upstream/COPYING.txt", ["Qhull, Copyright", "www.qhull.org"]),
        ("External/qhull/NOTICE.txt", ["MODIFICATIONS", "clause 3", "clause 4",
                                       "www.qhull.org", "Modified by:", "Date:", "Reason:"]),
        ("External/qhull/MODIFICATIONS.md", ["user.h", "mem.c", "user.c"]),
    ],
    "opcode": [
        ("External/opcode/COPYING.from-ode-0.13.1.txt", ["LGPLv2.1+ and BSD"]),
        ("External/opcode/NOTICE.md", ["carries no licence text at all",
                                       "COPYING.from-ode-0.13.1.txt",
                                       "standalone 1.3 distribution"]),
        ("External/opcode/MODIFICATIONS.md", ["OPC_Settings.h", "IceContainer.cpp"]),
    ],
}

# The qhull licence's clause 1 forbids removing the copyright header, so a
# modified qhull file has to carry it still.
QHULL_COPYRIGHT = "copyright"


def find_pinned_sources(start):
    """The repository root with the tracked upstream trees, or None."""
    root = pathlib.Path(start).resolve()
    for base in [root] + list(root.parents):
        if all((base / upstream_rel).is_dir()
               for upstream_rel, _ in LIBRARIES.values()):
            return base
    return None


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify_upstream_manifests(repo):
    """Verify all committed upstream files against their pinned SHA-256 lists."""
    repo = pathlib.Path(repo)
    failures = []
    external = repo / "External"
    for library, expected_manifest_digest in sorted(UPSTREAM_MANIFEST_SHA256.items()):
        manifest = external / library / "UPSTREAM-MANIFEST.sha256"
        if not manifest.is_file():
            failures.append("%s: upstream file manifest is missing: %s"
                            % (library, manifest))
            continue
        actual_manifest_digest = digest(manifest)
        if actual_manifest_digest != expected_manifest_digest:
            failures.append("%s: UPSTREAM-MANIFEST.sha256 digest is %s, expected %s"
                            % (library, actual_manifest_digest, expected_manifest_digest))
            continue

        prefix = library + "/upstream/"
        listed = set()
        for line_number, line in enumerate(manifest.read_text(encoding="ascii").splitlines(), 1):
            fields = line.split(None, 1)
            if len(fields) != 2 or len(fields[0]) != 64:
                failures.append("%s: malformed upstream manifest line %d"
                                % (library, line_number))
                continue
            expected_digest, relative = fields[0].lower(), fields[1].strip()
            if relative.startswith("*"):
                relative = relative[1:]
            if not relative.startswith(prefix) or ".." in pathlib.PurePosixPath(relative).parts:
                failures.append("%s: manifest path is outside its upstream tree: %s"
                                % (library, relative))
                continue
            path = external / pathlib.Path(relative)
            listed.add(path.resolve())
            if not path.is_file():
                failures.append("%s: upstream manifest file is missing: %s"
                                % (library, relative))
            elif digest(path) != expected_digest:
                failures.append("%s: upstream file differs from pinned manifest: %s"
                                % (library, relative))

        upstream_root = external / library / "upstream"
        actual = {path.resolve() for path in upstream_root.rglob("*") if path.is_file()}
        for path in sorted(actual - listed):
            failures.append("%s: upstream file is absent from its pinned manifest: %s"
                            % (library, path.relative_to(external).as_posix()))
        for path in sorted(listed - actual):
            failures.append("%s: manifest contains an untracked upstream path: %s"
                            % (library, path))
    return failures


def check(repo, pinned, out):
    """Returns a list of failure strings. Empty means the vendoring holds."""
    repo = pathlib.Path(repo).resolve()
    pinned = pathlib.Path(pinned).resolve()
    failures = []
    checked = 0
    modified = 0
    tracked_sources = pinned == repo
    if tracked_sources:
        failures.extend(verify_upstream_manifests(repo))

    for library, (upstream_rel, pinned_rel) in sorted(LIBRARIES.items()):
        upstream_root = repo / upstream_rel
        pinned_root = (repo / upstream_rel if tracked_sources
                       else pinned / pinned_rel)
        novodex_root = repo / "External" / library / "novodex"

        if not upstream_root.is_dir():
            failures.append("%s: the vendored upstream tree is missing: %s"
                            % (library, upstream_root))
            continue
        if not pinned_root.is_dir():
            failures.append("%s: the pinned upstream tree is missing: %s"
                            % (library, pinned_root))
            continue
        if not novodex_root.is_dir():
            failures.append("%s: the local-modification directory is missing: %s"
                            % (library, novodex_root))
            continue

        # 1. upstream/ is pinned, either against the separate extracted archive
        # used by older callers, or against the committed manifest on clean trees.
        if not tracked_sources:
            for path in sorted(p for p in upstream_root.rglob("*") if p.is_file()):
                rel = path.relative_to(upstream_root)
                counterpart = pinned_root / rel
                checked += 1
                if not counterpart.is_file():
                    failures.append("%s: External/.../upstream/%s is not a file in the pinned "
                                    "tree %s, so it is not upstream at all"
                                    % (library, rel.as_posix(), pinned_rel))
                elif digest(path) != digest(counterpart):
                    failures.append("%s: External/.../upstream/%s differs from the pinned "
                                    "archive; upstream/ is never edited"
                                    % (library, rel.as_posix()))

        # 2. novodex/ is exactly the local modifications, each with an address.
        for path in sorted(p for p in novodex_root.rglob("*") if p.is_file()):
            rel = path.relative_to(novodex_root)
            name = rel.as_posix()
            text = path.read_text(encoding="latin-1")
            counterpart = upstream_root / rel

            if name in DECLARED_ADDITIONS[library]:
                if counterpart.is_file():
                    failures.append("%s: novodex/%s is declared an addition but upstream "
                                    "has a file of that name" % (library, name))
            elif not counterpart.is_file():
                failures.append("%s: novodex/%s names no upstream counterpart and is not a "
                                "declared addition, so the build ignores it"
                                % (library, name))
            elif digest(path) == digest(counterpart):
                failures.append("%s: novodex/%s is byte-identical to upstream, which means "
                                "a local modification was lost" % (library, name))
            else:
                modified += 1

            if MARKER not in text:
                failures.append("%s: novodex/%s carries no '%s' block, so what changed and "
                                "why cannot be read off the file"
                                % (library, name, MARKER))
            elif not ADDRESS.search(text):
                failures.append("%s: novodex/%s carries a modification block with no oracle "
                                "address in it, which makes it unfalsifiable"
                                % (library, name))

            if library == "qhull" and counterpart.is_file():
                if QHULL_COPYRIGHT in counterpart.read_text(encoding="latin-1").lower() \
                        and QHULL_COPYRIGHT not in text.lower():
                    failures.append("%s: novodex/%s dropped the upstream copyright notice, "
                                    "which clause 1 of COPYING.txt forbids" % (library, name))

        # 3. the licence obligations.
        for rel_path, needles in LICENCE_FILES[library]:
            path = repo / rel_path
            if not path.is_file():
                failures.append("%s: the licence file %s is missing" % (library, rel_path))
                continue
            text = path.read_text(encoding="latin-1")
            for needle in needles:
                if needle not in text:
                    failures.append("%s: %s no longer says %r"
                                    % (library, rel_path, needle))

    if tracked_sources:
        checked = sum(1 for library in UPSTREAM_MANIFEST_SHA256
                      for path in (repo / "External" / library / "upstream").rglob("*")
                      if path.is_file())
    out.write("vendored upstream files checked=%d locally modified=%d\n" % (checked, modified))
    return failures


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", required=True,
                        help="the implementation repository holding External/")
    parser.add_argument("--thirdparty-root", default=None,
                        help="the staged pinned archives; searched for upward by default")
    args = parser.parse_args(argv)

    pinned = pathlib.Path(args.thirdparty_root) if args.thirdparty_root \
        else find_pinned_sources(pathlib.Path(__file__).resolve().parent)
    if pinned is None or not pinned.is_dir():
        sys.stderr.write("the tracked upstream source trees are missing under External/; "
                         "the vendored trees cannot be checked against anything\n")
        return 1

    sys.stdout.write("vendored repo=%s pinned=%s\n" % (args.repo, pinned))
    failures = check(args.repo, pinned, sys.stdout)
    for failure in failures:
        sys.stderr.write("vendored FAILED %s\n" % failure)
    sys.stdout.write("vendored status=%s failures=%d\n"
                     % ("pass" if not failures else "fail", len(failures)))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
