import json
import re
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[5]
MANIFEST = REPO_ROOT / "docs/reconstruction/novodex-physics/oracle/ghidra/manifest.json"
TABLE = REPO_ROOT / "Physics/src/FluidModelExports.inc"
IMAGE_BASE = 0x10000000


class FluidModelExportTableTests(unittest.TestCase):
    def test_table_matches_all_resolver_assignments_in_order(self):
        manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
        function = next(item for item in manifest["functions"] if item["rva"] == "0x0008d870")
        expected = [
            (int(address, 16) - IMAGE_BASE, name)
            for address, name in re.findall(
                r'DAT_(101[0-9a-fA-F]{5})\s*=\s*GetProcAddress\(DAT_10126654,"([^"]+)"\)',
                function["decompiler_c"],
            )
        ]
        table = [
            (int(address, 16), name)
            for name, address in re.findall(
                r'^NX_FLUID_MODEL_EXPORT\(([^,]+),\s*(0x[0-9a-fA-F]+)\)$',
                TABLE.read_text(encoding="utf-8"),
                re.MULTILINE,
            )
        ]
        self.assertEqual(169, len(expected))
        self.assertEqual(expected, table)
        self.assertEqual(len(table), len({name for _, name in table}))
        self.assertEqual(len(table), len({address for address, _ in table}))


if __name__ == "__main__":
    unittest.main()
