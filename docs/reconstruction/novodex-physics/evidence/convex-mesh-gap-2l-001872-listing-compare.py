"""Compare the Task 2l contact accumulator against its pinned x86 listing."""
import re
import sys

import capstone
import pefile

candidate_path, map_path, oracle_path = sys.argv[1:]
candidate = pefile.PE(candidate_path)
oracle = pefile.PE(oracle_path)
symbols = {}
for line in open(map_path, encoding="latin-1"):
    match = re.match(r"\s*000[1-9]:[0-9a-f]+\s+(\S+)\s+([0-9a-f]{8})\s", line)
    if match:
        symbols[match.group(1)] = int(match.group(2), 16)
symbol = next((name for name in symbols if name == "_nxMeshContactAccumulate@20"), None)
if symbol is None:
    raise SystemExit("candidate callback symbol is missing")

decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
row_rva, row_size = 0x466e0, 146


def instructions(image, address, size):
    raw = image.get_data(address - image.OPTIONAL_HEADER.ImageBase, size)
    return list(decoder.disasm(raw, address))


def normalized(items, base, end):
    addresses = {item.address: index for index, item in enumerate(items)}
    output = []
    for item in items:
        operands = item.op_str
        if item.mnemonic.startswith("j") and operands.startswith("0x"):
            operands = "@%s" % addresses.get(int(operands, 16), -1)
        else:
            operands = re.sub(r"\[0x[0-9a-f]+\]", "[absolute]", operands)
            operands = re.sub(r"\[([^]]*)\+ 0x[0-9a-f]+\]", r"[\1+ absolute]", operands)
        output.append((item.mnemonic, operands))
    return output


oracle_items = instructions(oracle, 0x10000000 + row_rva, row_size)
candidate_items = instructions(candidate, symbols[symbol], row_size)
if sum(item.size for item in oracle_items) != row_size or sum(item.size for item in candidate_items) != row_size:
    raise SystemExit("row extent does not decode to exactly 146 bytes")
left = normalized(oracle_items, 0x10000000 + row_rva, 0x10000000 + row_rva + row_size)
right = normalized(candidate_items, symbols[symbol], symbols[symbol] + row_size)
diffs = [(index, a, b) for index, (a, b) in enumerate(zip(left, right)) if a != b]
print("phys_fn_001872 instructions=%d differences=%d" % (len(left), len(diffs)))
for index, expected, actual in diffs:
    print("%d oracle=%s candidate=%s" % (index, expected, actual))
print("ALL EQUAL" if not diffs else "DIFFERENCES")
raise SystemExit(bool(diffs))
