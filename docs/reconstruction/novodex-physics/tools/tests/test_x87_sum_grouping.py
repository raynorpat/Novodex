import sys
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS_DIR))

import vendored_match as vm  # noqa: E402
import x87_sum_grouping as sg  # noqa: E402

BASE, TEXT = 0x10000000, 0x1000
X = b"\xd9\x01\xd8\x0a"            # fld [ecx]; fmul [edx]
Y = b"\xd9\x41\x04\xd8\x4a\x04"    # fld [ecx+4]; fmul [edx+4]
Z = b"\xd9\x41\x08\xd8\x4a\x08"    # fld [ecx+8]; fmul [edx+8]
FADDP = b"\xde\xc1"                # faddp st(1)


def image(code):
    text = bytes(code).ljust(0x100, b"\xcc")
    return vm.Image(BASE, [vm.Section(".text", TEXT, text, True, False)], [], {})


class GroupingTest(unittest.TestCase):
    def grouping(self, code):
        sites = sg.scan(image(code + b"\xc3"), TEXT, TEXT + 0x100)
        self.assertEqual(len(sites), 1)
        return sites[0][1]

    def test_source_order(self):
        self.assertEqual(self.grouping(X + Y + FADDP + Z + FADDP), "(x+y)+z")

    def test_last_component_first(self):
        self.assertEqual(self.grouping(Z + Y + FADDP + X + FADDP), "(y+z)+x")

    def test_outer_pair_first(self):
        self.assertEqual(self.grouping(X + Z + FADDP + Y + FADDP), "(x+z)+y")

    def test_spilled_operand_keeps_its_slot(self):
        # z computed, spilled to [esp+8] and reloaded: the product still reads a named slot
        spill = Z[:3] + b"\xd9\x5c\x24\x08" + b"\xd9\x44\x24\x08" + Z[3:]
        self.assertEqual(self.grouping(spill + Y + FADDP + X + FADDP), "(y+z)+x")

    def test_pairing_counts_same_and_different(self):
        o = sg.scan(image(Z + Y + FADDP + X + FADDP + b"\xc3"), TEXT, TEXT + 0x100)
        c = sg.scan(image(X + Y + FADDP + Z + FADDP + b"\xc3"), TEXT, TEXT + 0x100)
        self.assertEqual(sg.pair_sites(o, c), (0, 1))
        self.assertEqual(sg.pair_sites(o, o), (1, 0))


if __name__ == "__main__":
    unittest.main()
