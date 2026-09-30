"""Compare Task 2l's implemented callback and helper rows against pinned x86 listings."""
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

rows = [
    ("phys_fn_001859", "_nxMeshHeightfieldTriangleContact", 0x44b70, 2892, None),
    ("phys_fn_001861", "_nxMeshHeightfieldAabbPass", 0x456c0, 1693, None),
    ("phys_fn_001865", "_nxMeshHeightfieldObbPass", 0x45d70, 1937, None),
    ("phys_fn_001869", "_nxMeshHeightfieldContact", 0x46510, 64, None),
    ("phys_fn_001874", "_nxMeshMeshSphereCallback", 0x46780, 801, None),
    ("phys_fn_001857", "_nxMeshTriangleEdgeNormal", 0x44860, 774, None),
    ("phys_fn_001872", "_nxMeshContactAccumulate@20", 0x466e0, 146, None),
    ("phys_fn_001870", "_nxOverlapMeshMesh", 0x46550, 394,
        (0x100d13c0, "_nxMeshMeshCallAabbTreeCollide", "AABBTreeCollider::Collide")),
]
CALL_MAPPINGS_001859 = {
    0x543d0: "_nxTask2lCallCreateAdjacencies",
    0x54460: "_nxTask2lCallCreateEdgeList",
    0x44860: "_nxMeshTriangleEdgeNormal",
    0x44510: "?NxSegmentTriangleEdge@@",
    0x345b0: "?NxLineLineClosestPoints@@",
    0xb4de0: "_nxTask2lCallContainerResize",
    0x0deb0: "_nxTask2lCallGetDebugRenderable",
    0x1d610: "?NxEmitContact@@",
}
decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)


def instructions(image, address, size):
    raw = image.get_data(address - image.OPTIONAL_HEADER.ImageBase, size)
    return list(decoder.disasm(raw, address))


def fragmented_001861(image):
    return (instructions(image, 0x100456c0, 538)
            + instructions(image, 0x100458e0, 1155))


def fragmented_001865(image):
    return (instructions(image, 0x10045d70, 1498)
            + instructions(image, 0x10046350, 439))


def normalized(items, call_targets=None, address_values=None):
    addresses = {item.address: index for index, item in enumerate(items)}
    call_targets = call_targets or {}
    address_values = address_values or {}
    output = []
    for item in items:
        operands = item.op_str
        if item.mnemonic.startswith("j") and operands.startswith("0x"):
            operands = "@%s" % addresses.get(int(operands, 16), -1)
        elif item.mnemonic == "call" and operands.startswith("0x"):
            operands = call_targets.get(int(operands, 16), "external-call")
        else:
            for address, label in address_values.items():
                operands = operands.replace("0x%08x" % address, label)
            operands = re.sub(r"\[0x[0-9a-f]+\]", "[absolute]", operands)
            operands = re.sub(r"\[([^]]*)\+ 0x[0-9a-f]+\]", r"[\1+ absolute]", operands)
        output.append((item.mnemonic, operands))
    return output


all_equal = True
for stable_id, symbol, row_rva, row_size, external_call in rows:
    if symbol not in symbols:
        raise SystemExit("candidate symbol is missing: " + symbol)
    oracle_items = (fragmented_001861(oracle) if stable_id == "phys_fn_001861"
                    else fragmented_001865(oracle) if stable_id == "phys_fn_001865"
                    else instructions(oracle, 0x10000000 + row_rva, row_size))
    candidate_items = instructions(candidate, symbols[symbol], row_size)
    if sum(item.size for item in oracle_items) != row_size or sum(item.size for item in candidate_items) != row_size:
        raise SystemExit(stable_id + " does not decode to its full row extent")
    oracle_calls = {}
    candidate_calls = {}
    if external_call:
        oracle_target, candidate_symbol, label = external_call
        if candidate_symbol not in symbols:
            raise SystemExit("candidate call target is missing: " + candidate_symbol)
        oracle_calls[oracle_target] = label
        candidate_calls[symbols[candidate_symbol]] = label
    if stable_id == "phys_fn_001859":
        call_count = 0
        for expected, actual in zip(oracle_items, candidate_items):
            if expected.mnemonic != "call" or not expected.op_str.startswith("0x"):
                continue
            oracle_target = int(expected.op_str, 16) - 0x10000000
            candidate_target = int(actual.op_str, 16) if actual.op_str.startswith("0x") else -1
            wanted = CALL_MAPPINGS_001859.get(oracle_target)
            matched = wanted is not None and any(
                wanted in name and address == candidate_target for name, address in symbols.items()
            )
            print("  call oracle_rva=0x%08x candidate=%s%s" % (
                oracle_target, next((name for name, address in symbols.items() if address == candidate_target), "?"),
                "" if matched else " UNEXPECTED"))
            call_count += 1
            all_equal &= matched
        if call_count != 14:
            print("001859 direct call count=%d expected=14" % call_count)
            all_equal = False
        print("001859 direct calls checked=%d mappings=14" % call_count)
    if stable_id == "phys_fn_001861":
        call_map = {
            0xb4d70: "??0Container@IceCore@@QAE@XZ",
            0xf4669: "_atexit",
            0xd13c0: "?Collide@AABBTreeCollider@Opcode@@QAE_NAAUBVTCache@2@PBVMatrix4x4@IceMaths@@1@Z",
            0x44b70: "_nxMeshHeightfieldTriangleContact",
        }
        call_count = 0
        for expected, actual in zip(oracle_items, candidate_items):
            if expected.mnemonic != "call" or not expected.op_str.startswith("0x"):
                continue
            oracle_target = int(expected.op_str, 16) - 0x10000000
            candidate_target = int(actual.op_str, 16) if actual.op_str.startswith("0x") else -1
            wanted = call_map.get(oracle_target)
            matched = wanted is not None and any(
                name == wanted and address == candidate_target for name, address in symbols.items()
            )
            print("001861 direct call oracle_rva=0x%08x candidate=%s%s" % (
                oracle_target,
                next((name for name, address in symbols.items() if address == candidate_target), "?"),
                "" if matched else " UNEXPECTED"))
            call_count += 1
            all_equal &= matched
        if call_count != 4:
            print("001861 direct call count=%d expected=4" % call_count)
            all_equal = False
    if stable_id == "phys_fn_001865":
        call_map = {
            0xde0d0: "?Collide@OBBCollider@Opcode@@QAE_NAAUOBBCache@2@ABVOBB@IceMaths@@ABVModel@2@PBVMatrix4x4@5@3@Z",
            0x10190: "?nxScratchStamp@@YIIPAX@Z",
            0x52240: "?nxMeshComputeVertexNormals@@YAXXZ",
            0xf47b0: "__alloca_probe",
            0x3c160: "?NxTrianglePlane@@YIPAVNxPlane@@PAV1@PAXPBVNxVec3@@22@Z",
            0x1d610: "?NxEmitContact@@YAXPAUNxContactSink@@PAX1IPBVNxVec3@@2GG@Z",
        }
        call_count = 0
        for expected, actual in zip(oracle_items, candidate_items):
            if expected.mnemonic != "call" or not expected.op_str.startswith("0x"):
                continue
            oracle_target = int(expected.op_str, 16) - 0x10000000
            candidate_target = int(actual.op_str, 16) if actual.op_str.startswith("0x") else -1
            wanted = call_map.get(oracle_target)
            matched = wanted is not None and any(
                name == wanted and address == candidate_target for name, address in symbols.items()
            )
            print("001865 direct call oracle_rva=0x%08x candidate=%s%s" % (
                oracle_target,
                next((name for name, address in symbols.items() if address == candidate_target), "?"),
                "" if matched else " UNEXPECTED"))
            call_count += 1
            all_equal &= matched
        if call_count != 7:
            print("001865 direct call count=%d expected=7" % call_count)
            all_equal = False
    if stable_id == "phys_fn_001869":
        call_map = {
            0x456c0: "_nxMeshHeightfieldAabbPass",
            0x45d70: "_nxMeshHeightfieldObbPass",
        }
        for expected, actual in zip(oracle_items, candidate_items):
            if expected.mnemonic != "call" or not expected.op_str.startswith("0x"):
                continue
            oracle_target = int(expected.op_str, 16) - 0x10000000
            candidate_target = int(actual.op_str, 16) if actual.op_str.startswith("0x") else -1
            wanted = call_map.get(oracle_target)
            matched = wanted is not None and any(
                name == wanted and address == candidate_target for name, address in symbols.items()
            )
            print("001869 direct call oracle_rva=0x%08x candidate=%s%s" % (
                oracle_target,
                next((name for name, address in symbols.items() if address == candidate_target), "?"),
                "" if matched else " UNEXPECTED"))
            all_equal &= matched
    if stable_id == "phys_fn_001874":
        call_map = {
            0x277c0: "??0SphereShape@@QAE@PAXI@Z",
            0x278c0: "?nxSphereSetRadius@SphereShape@@QAEXM@Z",
            0x27820: "?nxSphereCallbackDtor@SphereShape@@QAEXXZ",
            0x466e0: "_nxMeshContactAccumulate@20",
        }
        call_count = 0
        for expected, actual in zip(oracle_items, candidate_items):
            if expected.mnemonic != "call" or not expected.op_str.startswith("0x"):
                continue
            oracle_target = int(expected.op_str, 16) - 0x10000000
            candidate_target = int(actual.op_str, 16) if actual.op_str.startswith("0x") else -1
            wanted = call_map.get(oracle_target)
            matched = wanted is not None and symbols.get(wanted) == candidate_target
            print("001874 direct call oracle_rva=0x%08x candidate=%s%s" % (
                oracle_target,
                next((name for name, address in symbols.items() if address == candidate_target), "?"),
                "" if matched else " UNEXPECTED"))
            call_count += 1
            all_equal &= matched
        if call_count != 9:
            print("001874 direct call count=%d expected=9" % call_count)
            all_equal = False
    oracle_addresses = {}
    candidate_addresses = {}
    if stable_id == "phys_fn_001861":
        oracle_addresses = {0x10123ce4: "shared-container", 0x10103040: "container-cleanup"}
        candidate_addresses = {
            symbols["?nxTask2lTrianglePairContainer@@3PAEA"]: "shared-container",
            symbols["_nxTask2lTrianglePairContainerCleanup"]: "container-cleanup",
        }
    if stable_id == "phys_fn_001865":
        oracle_addresses = {
            0x101043cc: "mesh-epsilon", 0x101041f0: "zero",
            0x10107bc4: "opcode-error", 0x10107d00: "source-file",
            0x101041b0: "foundation-instance-slot", 0x101041b4: "foundation-error-slot",
        }
        candidate_addresses = {
            symbols["?nxTask2lEpsilon@@3MB"]: "mesh-epsilon",
            symbols["?nxTask2lZero@@3MB"]: "zero",
            symbols["?nxTask2lOpcodeError@@3QBDB"]: "opcode-error",
            symbols["?nxTask2lSourceFile@@3QBDB"]: "source-file",
            symbols["_nxConvexMeshFoundationInstanceSlot"]: "foundation-instance-slot",
            symbols["_nxConvexMeshFoundationErrorSlot"]: "foundation-error-slot",
        }
    if stable_id == "phys_fn_001874":
        oracle_addresses = {
            0x10123d78: "matrix-a-slot", 0x10123d7c: "matrix-b-slot",
            0x10107a08: "distance-epsilon", 0x101041ec: "one",
        }
        candidate_addresses = {
            symbols["_nxTask2lSphereMatrixA"]: "matrix-a-slot",
            symbols["_nxTask2lSphereMatrixB"]: "matrix-b-slot",
            symbols["?nxTask2lSphereEpsilon@@3MB"]: "distance-epsilon",
            symbols["?nxTask2lSphereOne@@3MB"]: "one",
        }
    left = normalized(oracle_items, oracle_calls, oracle_addresses)
    right = normalized(candidate_items, candidate_calls, candidate_addresses)
    diffs = [(index, a, b) for index, (a, b) in enumerate(zip(left, right)) if a != b]
    print("%s instructions=%d differences=%d" % (stable_id, len(left), len(diffs)))
    for index, expected, actual in diffs:
        print("%d oracle=%s candidate=%s" % (index, expected, actual))
    all_equal &= not diffs
print("ALL EQUAL" if all_equal else "DIFFERENCES")
raise SystemExit(not all_equal)
