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


def test_row_cleanup_stops_at_register_tail_jump():
    # jmp eax; ret. The ret is unreachable in this row.
    assert AUDIT.row_cleanup(b"\xff\xe0\xc3", MD, 0) is None
