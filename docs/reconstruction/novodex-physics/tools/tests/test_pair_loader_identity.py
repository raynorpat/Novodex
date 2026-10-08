"""The differential harness must identify staged modules before test code runs.

An oracle-sensitive test may crash before reaching its final identity report.
The loader therefore reports the loaded pair as part of opening it, while the
test's existing final report remains responsible for auditing late loads.
"""

import re
import unittest
from pathlib import Path


LOADER = Path(__file__).resolve().parents[5] / "Tests" / "PhysicsPairLoader.h"


def function_body(source: str, name: str) -> str:
    match = re.search(rf"static int {name}\([^)]*\)\s*\{{", source)
    if not match:
        raise AssertionError(f"{name} declaration not found")
    start = match.end()
    depth = 1
    for index in range(start, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[start:index]
    raise AssertionError(f"{name} body is unterminated")


class PairLoaderIdentityTests(unittest.TestCase):
    def test_open_pair_reports_identity_before_returning_to_test_code(self):
        source = LOADER.read_text(encoding="utf-8")
        open_pair = function_body(source, "nxOpenPair")
        self.assertIn("nxReportPairIdentity(pairDirectory)", open_pair)
        self.assertLess(
            open_pair.index("nxLoadPhysics(pairDirectory)"),
            open_pair.index("nxReportPairIdentity(pairDirectory)"),
        )

    def test_end_of_run_identity_audit_is_retained(self):
        source = LOADER.read_text(encoding="utf-8")
        report = function_body(source, "nxReportPairIdentity")
        self.assertIn("nxAuditModules(pairDirectory)", report)
        self.assertIn("loaded module=%S path=%S sha256=%s", report)
        self.assertIn("fflush(stdout)", report)


if __name__ == "__main__":
    unittest.main()
