"""Regression tests for decoding row-local x86 stack cleanup."""

import importlib.util
from pathlib import Path

import capstone


TOOL = Path(__file__).resolve().parents[1] / "audit_call_conventions.py"
SPEC = importlib.util.spec_from_file_location("audit_call_conventions", TOOL)
AUDIT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(AUDIT)


MD = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)


def test_row_cleanup_reads_bare_and_argument_popping_returns():
    assert AUDIT.row_cleanup(b"\xc3", MD, 0) == 0
    assert AUDIT.row_cleanup(b"\xc2\x08\x00", MD, 0) == 8


def test_row_cleanup_stops_at_indirect_tail_jump():
    # jmp dword ptr [0x10]; ret 4. The ret belongs to the target function.
    code = b"\xff\x25\x10\x00\x00\x00\xc2\x04\x00"
    assert AUDIT.row_cleanup(code, MD, 0) is None


def test_audited_indirect_tail_rows_require_exact_pinned_bytes():
    for rva, (expected, cleanup) in AUDIT.AUDITED_INDIRECT_TAILS.items():
        assert AUDIT.audited_indirect_tail_cleanup(expected, rva, len(expected)) == cleanup
        mutated = bytes([expected[0] ^ 1]) + expected[1:]
        assert AUDIT.audited_indirect_tail_cleanup(mutated, rva, len(mutated)) is None
    assert AUDIT.audited_indirect_tail_cleanup(b"\xff\x25\x00\x00\x00\x00", 0x12345) is None


def test_row_cleanup_stops_at_register_tail_jump():
    # jmp eax; ret. The ret is unreachable in this row.
    assert AUDIT.row_cleanup(b"\xff\xe0\xc3", MD, 0) is None


def test_row_cleanup_follows_direct_jumps_inside_the_row():
    # jmp +2 skips a decoy `ret 4`; the reachable return pops eight bytes.
    code = b"\xeb\x03\xc2\x04\x00\xc2\x08\x00"
    assert AUDIT.row_cleanup(code, MD, 0) == 8


def test_row_cleanup_checks_every_conditional_return_path():
    # Both branches return with the same cleanup even though a linear scan
    # would stop at the first return.
    same_cleanup = b"\x74\x03\xc2\x08\x00\xc2\x08\x00"
    assert AUDIT.row_cleanup(same_cleanup, MD, 0) == 8

    different_cleanup = b"\x74\x03\xc2\x04\x00\xc2\x08\x00"
    assert AUDIT.row_cleanup(different_cleanup, MD, 0) is None


def test_row_cleanup_keeps_out_of_row_direct_jumps_undecidable():
    # A direct tail jump leaves the function row; the target's ret is outside
    # the bounded function bytes and cannot establish this row's cleanup.
    assert AUDIT.row_cleanup(b"\xeb\x01\x90\xc2\x04\x00", MD, 0,
                             function_size=2) is None


def test_row_cleanup_uses_only_explicitly_resolved_indirect_switch_targets():
    # cmp eax,1; ja default; jmp [eax*4+table]; two in-row returns.
    code = (b"\x83\xf8\x01\x77\x0a\xff\x24\x85\x20\x00\x00\x10"
            b"\xc2\x04\x00\xc2\x04\x00")
    start = 0x10000000
    reader = lambda _ins: [start + 12, start + 15]
    assert AUDIT.row_cleanup(code, MD, 0, jump_table_reader=reader) == 4

    external_target = lambda _ins: [start + 0x100]
    assert AUDIT.row_cleanup(code, MD, 0,
                             jump_table_reader=external_target) is None


def test_bounded_switch_resolver_checks_limit_table_section_and_targets():
    code = (b"\x83\xf8\x01\x77\x0a\xff\x24\x85\x20\x00\x00\x10"
            b"\xc2\x04\x00\xc2\x04\x00")
    start = 0x10000000
    MD.detail = True
    jump = next(ins for ins in MD.disasm(code, start) if ins.mnemonic == "jmp")
    good_table = (start + 12).to_bytes(4, "little") + (start + 15).to_bytes(4, "little")
    section = [{"rva": 0x20, "raw_size": len(good_table), "raw_offset": 0}]
    assert AUDIT.bounded_switch_targets(
        jump, code, MD, 0, good_table, section, 0x10000000) == [start + 12, start + 15]

    bad_table = (start + 12).to_bytes(4, "little") + (start + 0x100).to_bytes(4, "little")
    assert AUDIT.bounded_switch_targets(
        jump, code, MD, 0, bad_table, section, 0x10000000) is None


def test_fastcall_callee_pops_stack_arguments_after_ecx_and_edx():
    assert AUDIT.expected_cleanup("__fastcall", 0) == 0
    assert AUDIT.expected_cleanup("__fastcall", 2) == 0
    assert AUDIT.expected_cleanup("__fastcall", 4) == 8


def test_all_x86_cleanup_conventions_use_their_register_argument_counts():
    assert AUDIT.expected_cleanup("__cdecl", 3) == 0
    assert AUDIT.expected_cleanup("__stdcall", 3) == 12
    assert AUDIT.expected_cleanup("__thiscall", 3) == 8


def test_audit_inputs_follow_the_selected_repository_and_oracle_roots(tmp_path):
    harness, dll, pe = AUDIT.resolve_inputs(tmp_path, tmp_path / "oracle")
    assert harness == tmp_path / "tests" / "PhysicsObjectLayoutTests.cpp"
    assert dll == tmp_path / "oracle" / "NxPhysics.dll"
    assert pe == tmp_path / "docs" / "reconstruction" / "novodex-physics" / "oracle" / "pe.json"
