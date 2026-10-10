"""Tests for verify_vendored_sources.py.

Every test here is a way a vendoring rots, and every one of them was run against
the real trees before it was written down. The accepting case comes first,
because a checker that cannot pass is as useless as one that cannot fail.
"""

import io
import pathlib
import shutil
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))

import verify_vendored_sources as vendored  # noqa: E402


MARKER_BLOCK = "/*\n * NOVODEX LOCAL MODIFICATION\n * established at 0x000b4d93\n */\n"


def write(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="latin-1")


class VendoredTreeFixture(unittest.TestCase):
    """A miniature but complete pair of trees: pinned archives and a repository."""

    def setUp(self):
        self.tmp = pathlib.Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, self.tmp, True)
        self.pinned = self.tmp / "thirdparty"
        self.repo = self.tmp / "repo"

        write(self.pinned / "qhull-2003.1" / "src" / "mem.c",
              "/* see COPYING for copyright information */\nint qh_memalloc(void) { return 0; }\n")
        write(self.pinned / "qhull-2003.1" / "src" / "geom.c", "/* Copyright the Geometry Center */\n")
        write(self.pinned / "opcode13" / "Opcode" / "OPC_Settings.h", "#define OPC_RAYHIT_CALLBACK\n")
        write(self.pinned / "opcode13" / "Opcode" / "OPC_Model.cpp", "// stock\n")

        for library, upstream in (("qhull", "qhull-2003.1/src"),
                                  ("opcode", "opcode13/Opcode")):
            src = self.pinned / pathlib.Path(upstream)
            dst = self.repo / vendored.LIBRARIES[library][0]
            shutil.copytree(src, dst)

        write(self.repo / "External/qhull/novodex/mem.c",
              MARKER_BLOCK + "/* Copyright the Geometry Center */\nint qh_memalloc(void) { return 1; }\n")
        write(self.repo / "External/qhull/novodex/QhullNovodeXHost.h",
              MARKER_BLOCK + "void qhNovodeXFree(void*);\n")
        write(self.repo / "External/opcode/novodex/OPC_Settings.h",
              MARKER_BLOCK + "// #define OPC_RAYHIT_CALLBACK\n")
        write(self.repo / "External/opcode/novodex/OpcodeNovodeXHost.h",
              MARKER_BLOCK + "void opcNovodeXFree(void*);\n")

        for library, entries in vendored.LICENCE_FILES.items():
            for rel, needles in entries:
                if rel.startswith("External/qhull/upstream"):
                    continue
                write(self.repo / rel, "\n".join(needles) + "\n")
        write(self.repo / "External/qhull/upstream/COPYING.txt",
              "Qhull, Copyright (c) 1993-2003\nwww.qhull.org\n")
        # ...and its counterpart, so the byte-identity check has something to
        # compare against.
        write(self.pinned / "qhull-2003.1" / "src" / "COPYING.txt", "unused")

    def run_check(self):
        out = io.StringIO()
        return vendored.check(self.repo, self.pinned, out)


class TheAcceptingCase(VendoredTreeFixture):
    def test_a_well_formed_vendoring_passes(self):
        self.assertEqual(self.run_check(), [])


class UpstreamIsNeverEdited(VendoredTreeFixture):
    def test_a_changed_upstream_byte_fails(self):
        path = self.repo / vendored.LIBRARIES["qhull"][0] / "mem.c"
        path.write_text(path.read_text() + "/* tweak */\n", encoding="latin-1")
        failures = self.run_check()
        self.assertTrue(any("differs from the pinned archive" in f for f in failures), failures)

    def test_a_file_upstream_does_not_have_fails(self):
        write(self.repo / vendored.LIBRARIES["qhull"][0] / "invented.c", "int x;\n")
        failures = self.run_check()
        self.assertTrue(any("is not a file in the pinned tree" in f for f in failures), failures)

    def test_an_absent_pinned_tree_fails(self):
        shutil.rmtree(self.pinned / "opcode13")
        failures = self.run_check()
        self.assertTrue(any("the pinned upstream tree is missing" in f for f in failures), failures)


class NovodexIsExactlyTheModifications(VendoredTreeFixture):
    def test_an_overlay_that_drifted_back_to_stock_fails(self):
        """The failure mode that matters: a modification silently lost."""
        upstream = self.repo / vendored.LIBRARIES["opcode"][0] / "OPC_Settings.h"
        overlay = self.repo / "External/opcode/novodex/OPC_Settings.h"
        overlay.write_text(upstream.read_text(encoding="latin-1"), encoding="latin-1")
        failures = self.run_check()
        self.assertTrue(any("byte-identical to upstream" in f for f in failures), failures)

    def test_an_overlay_without_a_marker_block_fails(self):
        overlay = self.repo / "External/opcode/novodex/OPC_Settings.h"
        overlay.write_text("// nothing to see\n", encoding="latin-1")
        failures = self.run_check()
        self.assertTrue(any("carries no 'NOVODEX LOCAL MODIFICATION' block" in f
                            for f in failures), failures)

    def test_a_marker_block_with_no_address_fails(self):
        overlay = self.repo / "External/opcode/novodex/OPC_Settings.h"
        overlay.write_text("/* NOVODEX LOCAL MODIFICATION: trust me */\n// changed\n",
                           encoding="latin-1")
        failures = self.run_check()
        self.assertTrue(any("no oracle address in it" in f for f in failures), failures)

    def test_an_overlay_with_no_upstream_counterpart_fails(self):
        write(self.repo / "External/opcode/novodex/OPC_Invented.h",
              MARKER_BLOCK + "// nowhere upstream\n")
        failures = self.run_check()
        self.assertTrue(any("names no upstream counterpart" in f for f in failures), failures)

    def test_a_declared_addition_that_shadows_upstream_fails(self):
        write(self.repo / vendored.LIBRARIES["opcode"][0] / "OpcodeNovodeXHost.h", "// stock\n")
        write(self.pinned / "opcode13" / "Opcode" / "OpcodeNovodeXHost.h", "// stock\n")
        failures = self.run_check()
        self.assertTrue(any("declared an addition but upstream has a file" in f
                            for f in failures), failures)

    def test_portable_ray_tri_include_is_a_declared_addition(self):
        write(self.repo / "External/opcode/novodex/OPC_RayTriOverlapScalar.inl",
              MARKER_BLOCK + "// Scalar-only implementation.\n")
        self.assertEqual(self.run_check(), [])

    def test_dropping_the_qhull_copyright_header_fails(self):
        """Clause 1 of COPYING.txt forbids it, so the checker does too."""
        overlay = self.repo / "External/qhull/novodex/mem.c"
        overlay.write_text(MARKER_BLOCK + "int qh_memalloc(void) { return 1; }\n",
                           encoding="latin-1")
        failures = self.run_check()
        self.assertTrue(any("dropped the upstream copyright notice" in f
                            for f in failures), failures)


class TheLicenceObligations(VendoredTreeFixture):
    def test_a_missing_notice_fails(self):
        (self.repo / "External/qhull/NOTICE.txt").unlink()
        failures = self.run_check()
        self.assertTrue(any("the licence file External/qhull/NOTICE.txt is missing" in f
                            for f in failures), failures)

    def test_a_notice_that_stopped_saying_the_obligation_fails(self):
        path = self.repo / "External/qhull/NOTICE.txt"
        path.write_text(path.read_text(encoding="latin-1").replace("clause 3", "..."),
                        encoding="latin-1")
        failures = self.run_check()
        self.assertTrue(any("no longer says 'clause 3'" in f for f in failures), failures)

    def test_papering_over_the_opcode_discrepancy_fails(self):
        """The one licence statement this project must not lose is the awkward one."""
        path = self.repo / "External/opcode/NOTICE.md"
        path.write_text(path.read_text(encoding="latin-1")
                        .replace("carries no licence text at all", "is under ODE's terms"),
                        encoding="latin-1")
        failures = self.run_check()
        self.assertTrue(any("carries no licence text at all" in f for f in failures), failures)


class TrackedUpstreamDiscovery(unittest.TestCase):
    def test_clean_tree_uses_tracked_upstream_sources_without_analysis_snapshot(self):
        root = pathlib.Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, root, True)
        (root / "External/qhull/upstream/src").mkdir(parents=True)
        (root / "External/opcode/upstream/Opcode").mkdir(parents=True)

        pinned = vendored.find_pinned_sources(root / "docs/reconstruction/novodex-physics")

        self.assertEqual(pinned.resolve(), root.resolve())


class TheRealTrees(unittest.TestCase):
    """The checker against the actual repository, when both are on this disk."""

    REPO = pathlib.Path(__file__).resolve().parents[5]

    def test_the_vendored_trees_hold(self):
        pinned = vendored.find_pinned_sources(pathlib.Path(__file__).resolve().parent)
        if pinned is None or not self.REPO.is_dir():
            self.fail("tracked upstream trees are required for a reproducible checkout")
        failures = vendored.check(self.REPO, pinned, io.StringIO())
        self.assertEqual(failures, [])

    def test_the_tracked_manifest_rejects_a_changed_upstream_file(self):
        root = pathlib.Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, root, True)
        source = self.REPO / "External"
        target = root / "External"
        for library in vendored.UPSTREAM_MANIFEST_SHA256:
            (target / library).mkdir(parents=True, exist_ok=True)
            shutil.copy2(source / library / "UPSTREAM-MANIFEST.sha256",
                         target / library / "UPSTREAM-MANIFEST.sha256")
            shutil.copytree(source / library / "upstream", target / library / "upstream")

        self.assertEqual(vendored.verify_upstream_manifests(root), [])
        changed = target / "opcode/upstream/Opcode/OPC_Settings.h"
        changed.write_bytes(changed.read_bytes() + b"// altered\n")
        failures = vendored.verify_upstream_manifests(root)
        self.assertTrue(any("differs from pinned manifest" in failure for failure in failures),
                        failures)


if __name__ == "__main__":
    unittest.main()
