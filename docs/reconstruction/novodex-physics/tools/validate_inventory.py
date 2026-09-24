#!/usr/bin/env python3
"""Validate the Novodex Physics oracle census inventory, its label ledger, its
phase closure ledgers, and the programme record that quotes their counts."""

import argparse
import collections
from bisect import bisect_left, bisect_right
import csv
import io
import json
import os
import re
import sys
from pathlib import Path


SCHEMA_VERSION = 1

TOP_LEVEL_KEYS = (
    "schema_version",
    "pins",
    "sections",
    "functions",
    "data_objects",
    "exports",
    "imports",
    "coverage",
    "phases",
    "gates",
)

PIN_KEYS = ("oracle", "link_oracle", "public_headers", "analysis_toolchain", "labels")
ORACLE_PIN_KEYS = ("path", "sha256", "size", "named_exports")
LINK_ORACLE_PIN_KEYS = ("path", "sha256")
HEADER_PIN_KEYS = ("root", "sdk_version", "physics_manifest", "foundation_manifest")

SECTION_KEYS = (
    "name",
    "rva",
    "virtual_size",
    "raw_offset",
    "raw_size",
    "characteristics",
    "executable",
)
FUNCTION_KEYS = (
    "id",
    "rva",
    "size",
    "kind",
    "label",
    "label_confidence",
    "section",
    "phase",
    "phase_provenance",
    "state",
    "source",
    "ghidra_ref",
    "capstone_ref",
    "static_proof",
    "dynamic_proof",
    "notes",
)
# The library a row was emitted from, when a per-row source correspondence says
# so. Optional because absence is the common case and the common case must stay
# cheap to read; it is not a free-text note, and validate_third_party checks it
# against the correspondence map in both directions.
# `implementation` is the reconstruction's OWN file, as distinct from `source`,
# which is the oracle's __FILE__ attribution (7g). Optional because a row with no
# implementation has nothing to record, and absence is the common case.
#
# It is the field a path check belongs on. `source` is never expected to resolve:
# it names a file the oracle was built from, and the reconstruction deliberately
# does not recreate the oracle's directory layout -- 49 of the 51 unresolvable
# source paths have basenames that exist nowhere in the repository.
# `implementation_symbol` is the symbol a row was reconstructed INTO, which
# `implementation` cannot express: it names a file, and a file is not a function
# (13c). Round 42 showed the candidate's map carries symbols with addresses and the
# census could not be joined to it; round 46 recovered the value for the rows whose
# symbol the harness names -- its dispatch tables carry the row's oracle RVA and its
# abbreviated stable ID, and one drive names the candidate function outright.
FUNCTION_OPTIONAL_KEYS = ("third_party", "implementation",
                         "implementation_symbol")
DATA_KEYS = (
    "id",
    "rva",
    "size",
    "type",
    "owner",
    "references",
    "section",
    "phase",
    "phase_provenance",
    "state",
    "source",
    "label",
    "label_confidence",
    "structural_proof",
    "notes",
)
EXPORT_KEYS = ("ordinal", "name", "rva", "function_id")
IMPORT_KEYS = ("dll", "name", "ordinal", "iat_rva")
PHASE_KEYS = ("phase", "name", "status")
LEDGER_KEYS = ("schema_version", "labels")
LABEL_KEYS = ("id", "label", "confidence", "evidence", "reason")

STATES = (
    "discovered",
    "typed",
    "decompiled",
    "reconstructed",
    "statically_reviewed",
    "dynamically_gated",
    "classified",
    "closed",
)
# The states are a ladder, and the rung a row stands on is a claim about the kind
# of evidence behind it. Only the two dynamic rungs assert that something ran and
# noticed: that is the distinction the closure ledger is checked against below.
STATE_RANK = {name: index for index, name in enumerate(STATES)}
# `classified` shares the terminal rank with `closed`. They are not ordered against each other
# because they are not the same kind of claim: `closed` says a mutation was aimed at a code row and
# a gate caught it, `classified` says an artifact's form has been established and it has no
# behaviour to mutate. Ordering them would invite reading one as incomplete relative to the other.
STATE_RANK["classified"] = STATE_RANK["closed"]
DYNAMIC_STATES = ("dynamically_gated", "closed")
# The rungs a row reaches when its evidence is complete: `closed` for a code row a gate caught,
# `classified` for a row with no behaviour to mutate. A row here owes its phase nothing further, so
# it need not appear in a closure ledger at all -- which is what stops the data half of the census
# being carried as a debt now that it has somewhere to arrive.
TERMINAL_STATES = ("closed", "classified")
# The highest rung a row may stand on with nothing but a reconstruction behind
# it. Everything above is a claim that a gate ran and caught something, which
# only a closure ledger entry can establish.
UNGATED_CEILING = "reconstructed"
# `closed` is the terminal rung and it is the full-census audit's to grant: its
# gate is "entire census closed", and no earlier phase's ledger can establish
# that a row is finished rather than merely entered. It is the last declared
# phase by construction, and validate_row_states says so if that stops holding.
FULL_CENSUS_AUDIT_PHASE = 8
KINDS = ("code", "compiler_artifact")
# The rule that decided a row's phase. A phase worker holding one row reads this
# to tell evidence from propagation without re-deriving the whole assignment.
PHASE_PROVENANCE = (
    "runtime_artifact",
    "runtime_tail",
    "translation_unit",
    "enclosed_by_one_phase",
    "callers",
    "layout_adjacency",
    "shared_by_callers",
    "export_pin",
    "padding",
    "pe_structure",
    "reading_sites",
    # The shape-class vtable ruling in shape_slot_ruling.json placed the row. Unlike the
    # propagation rules above it is recomputed here, from that file and the oracle, so a
    # row carrying it cannot be moved or relabelled without the check saying so.
    "slot_ruling",
)
LABEL_CONFIDENCES = ("stable-id", "semantic")
# A phase closes by proving each of its rows or by deferring it in writing. The
# ledger below is what makes the second half auditable: a deferred-proof list
# nobody validates is how rows quietly become "closed" three phases later.
CLOSURE_KEYS = ("schema_version", "phase", "note", "proof_kinds", "deferred_reasons",
                "measured_from", "counts", "closed", "deferred")
# Both entry shapes are closed vocabularies. An entry carrying a key nobody reads
# is an entry whose evidence moved somewhere the validator cannot see it: the
# deleted "observed" kind left `observed_in` fields behind that still parsed.
CLOSED_KEYS = ("id", "rva", "proof", "gate", "falsification")
# A deferral discharged by a later phase. Phase 2 deferred 1,155 rows, each
# recording why and which phases could discharge it, and nothing anywhere
# recorded that one *had*: `stray` forbids phase N's ledger from naming another
# phase's row, and `unaccounted` forbids the owning phase's ledger from dropping
# it, so the two records would have to disagree.
#
# The discharge lands on the owning phase's ledger, moving the row from
# `deferred` to `closed`. Ownership is a census fact and `stray` exists to keep
# one row owned by exactly one phase, so the row stays where the census put it;
# what moved is the proof, and that belongs where the original "no falsifiable
# proof was available" claim was made. `counts` is recomputed from the ledger,
# so the owning phase's totals correct themselves rather than going stale.
#
# The original `driving_phases` is transcribed onto the row at the close. Closing
# the row deletes the deferred entry it came from, so nothing holds the original
# list to compare against and BOTH FIELDS ARE SELF-DECLARED: what `_check_discharge`
# can hold them to is that they agree with each other, that the discharging phase
# is not the owning one, and that every phase named is later than the owning one --
# the same rule `blocked_on_later_phase` deferrals already obey. That last part is
# the one with teeth, because a discharged row may name the DISCHARGING phase's
# gates, so without it two self-declared fields buy a gate this phase never runs.
#
# `note` is what a deferred entry has always had and a closed one did not: room
# to say something about the entry that is not the mutation. A closure that meets
# the standard and is nonetheless thin -- a seven-byte row whose delta is 1 -- can
# now say so on the row instead of only in a document nobody diffs against it.
CLOSED_OPTIONAL_KEYS = ("discharged_by_phase", "driving_phases", "note")
FALSIFICATION_KEYS = ("mutation", "detected")
DEFERRED_KEYS = ("id", "rva", "size", "reason", "phase_provenance", "driving_phases")
DEFERRED_OPTIONAL_KEYS = ("type", "translation_unit_zone", "reachability", "callers",
                          "note", "blocked_on", "blocked_on_type", "vtable_slot")
# There is one way to close a row: aim a mutation at it and have the gate catch
# it. An earlier schema had an "observed" kind resting on a free-text field
# nobody could check, and every error found in review was one of those rows, so
# the kind is gone. A token can be real and the row still not entered; only a
# mutation measures entry.
PROOF_KINDS = ("differential_falsified", "static_proof_falsified",
               "oracle_differential_falsified")
DIFFERENTIAL_KINDS = ("differential_falsified",)
STATIC_PROOF_KINDS = ("static_proof_falsified",)
# Phase 3 Task 3 added a third gate class, for rows that are not exported and so
# cannot be reached by a staged-pair differential at all. Those targets load the
# pinned oracle themselves and call the recorded internal addresses, so they do
# have an oracle side -- which is what separates them from a static proof -- but
# they run once rather than once per pair, so there is no transcript to diff and
# `stdout_delta` is a claim they cannot make. What they report is a mismatch
# count against the pinned DLL.
ORACLE_DIFFERENTIAL_KINDS = ("oracle_differential_falsified",)
# A data object has no body to mutate and no transcript of its own, so it cannot
# carry a code proof kind.
DATA_PROOF_KINDS = ()
# Phase 3 needed two reasons Phase 2's vocabulary could not spell. Phase 2 owned
# 163 rows and reached 125 of them, so every row it did not close was blocked,
# homeless or measured unreachable. Phase 3 owns 441 and closed 61; the rest is
# mostly a phase that ran out of dispatches, which is a debt rather than an
# obstruction, and one row that was reconstructed and driven but never had a
# mutation aimed at it. Calling either of those "unreachable" would be a false
# reachability claim, and calling them "blocked_on_later_phase" would name a
# phase that owes nothing.
#
# Phase 4 needed two more, and both exist to stop the same misreading. 722 of its
# 1,137 function rows were not reconstructed at all: they were vendored from pinned
# upstream archives, qhull 2003.1 and OPCODE 1.3, and compiled into the DLL. Saying
# `not_reconstructed_in_phase` about them would be false -- a candidate for those
# bytes exists and builds -- and saying nothing would let present bytes read as
# finished ones. `vendored_not_falsified` is the honest middle: the source is there
# and no mutation this schema can spend is aimed at the row. It deliberately makes
# no reachability claim, because "nothing runs it" is not checkable per row when
# the harness drives library entry points that call onward.
# `vendored_driven_divergent` is separate because collapsing it into the first
# would read as "nobody got to it" about the one row that was got to and did not
# reproduce the image.
DEFERRED_REASONS = ("blocked_on_later_phase", "unreachable_in_phase_2",
                    "not_independently_falsifiable", "homeless_shared_code",
                    "not_reconstructed_in_phase", "reconstructed_not_falsified",
                    "vendored_not_falsified", "vendored_driven_divergent",
                    "data_object_not_dispositioned")
# The one reason that belongs to the data half of the census, and the only one a
# data object may give. Policed in both directions: a function row borrowing it
# would shrink the code half of the split, and a data object borrowing a code
# reason would claim a reachability argument nobody made about it.
DATA_DEFERRAL_REASONS = ("data_object_not_dispositioned",)
# The two reasons that say a row's candidate is vendored upstream source. The split
# they draw was enforced by prose alone: a NovodeX row at `discovered` relabelled
# `vendored_not_falsified`, or a vendored row relabelled `not_reconstructed_in_phase`,
# validated with the counts adjusted. Both are policed now, in both directions:
# either reason needs a `third_party` row, `vendored_not_falsified` is used only
# where the census leaves that row at `discovered`, and a third-party row the
# census leaves at `discovered` has to give one of the two.
VENDORED_DEFERRAL_REASONS = ("vendored_not_falsified", "vendored_driven_divergent")
# From this phase on a closure ledger that closes a row must name the evidence file
# its counts are published in. Phases 2 and 3 published before stable IDs were
# written into their evidence; every later ledger has no such excuse, and an
# optional key would let the binding be switched off from inside the file it guards.
EVIDENCE_FILE_FROM_PHASE = 4
CLOSURE_NAME = re.compile(r"^phase([0-9]+)-closure[.]json$")
PHASE_RECORD_NAME = re.compile(r"^phase([0-9]+)[.]json$")
# Canonical decimal only: `stdout_delta=007` used to parse as 7 and pass, so a
# delta could be written in a form no runner ever emits.
STDOUT_DELTA = re.compile(r"^stdout_delta=(0|[1-9][0-9]*)$")
# What a staged-pair differential may report. `stdout_delta` is what
# run_differential.ps1 prints for a whole target; `cases` and `digests` are what
# a per-row mutation probe reports when it is compared against the committed
# oracle transcript instead, which is the only comparison fine enough to say
# which row moved. NxPhysicsGeometryTests prints one line per case and
# NxPhysicsKernelFuzzTests one digest per export, so a count in those units is a
# reading; converting it to a `stdout_delta` would be arithmetic on
# Compare-Object's semantics presented as a measurement. Non-zero is still
# required, and the unit is part of the record so a reader knows which
# comparison produced it.
DIFFERENTIAL_DETECTION = re.compile(r"^(?:stdout_delta|cases|digests)=(0|[1-9][0-9]*)$")
# Recording the unit is only worth doing if the unit has to be one the named
# gate can actually print. Before the widening a differential had exactly one
# legal detection shape; it now has three, and two of them are gate-specific:
# `cases` is a line of NxPhysicsGeometryTests' 299-case matrix and `digests` is
# one of NxPhysicsKernelFuzzTests' per-export folds. A row claiming `cases` on
# the fuzz target is claiming a comparison that target never performed, and
# nothing checked that. `stdout_delta` stays unbound because it is
# run_differential.ps1's whole-target number and every staged-pair target has
# one.
DETECTION_UNIT_GATES = {
    "cases": ("NxPhysicsGeometryTests",),
    "digests": ("NxPhysicsKernelFuzzTests",),
}
# The same shape for an oracle differential, which compares in process and so
# reports a mismatch count rather than a transcript delta.
MISMATCHES = re.compile(r"^mismatches=(0|[1-9][0-9]*)$")
# The number a closed row spends, in whichever unit its gate prints it, so that
# `validate_closure_evidence` can look for it in the evidence file. A static
# proof reports `check_failed <name>` and matches nothing here, which is right:
# there is no count to bind.
DETECTED_COUNT = re.compile(r"^[a-z_]+=([0-9]+)$")
# A registry ends at a closing brace in the first column and may not contain
# another registry: indenting one brace by a space used to fold the static-proof
# list into the differential one. A block whose brace is indented now matches
# nothing, so it registers no target rather than the wrong ones. Comments are
# stripped first, so a commented-out target cannot register as live.
PS_TARGET_BLOCK = re.compile(r"^[$](NxPhaseTestTargets|NxPhaseStaticProofTargets"
                             r"|NxPhaseOracleDifferentialTargets)"
                             r"[^=\n]*=[^{\n]*[{][^\n]*\n"
                             r"((?:^(?![}$]).*\n)*)"
                             r"^[}]", re.M)
PS_COMMENT = re.compile(r"#[^\n]*")
# The one line in run_differential.ps1 that drops output before comparing it.
DIFFERENTIAL_EXCLUSION = re.compile(r"-notmatch\s*'\^\((?P<alternatives>[^)]*)\)'")
# How a phase plan takes on an escalation raised by an earlier phase's record.
PLAN_ESCALATION = re.compile(r"`([a-z0-9_]+)` escalation in `gates/phase([0-9]+)[.]json`")
CENSUS_STATUSES = ("pending", "pass", "fail")
GATE_STATUSES = ("pending", "pass", "fail")
PHASE_STATUSES = ("pending", "closed")

# program.json is the document the next phase reads to decide what is already
# done, and every number in it is a claim about inventory.json and the closure
# ledgers. Nothing compared the two, so every phase's counters went stale at once
# and the staleness was invisible until someone recomputed them by hand.
PROGRAM_KEYS = ("schema_version", "oracle", "link_oracle", "header_root",
                "foundation_commit", "evidence_commit", "phases", "global_gates")
PROGRAM_PHASE_KEYS = ("phase", "name", "plan", "gate", "status",
                      "implementation_commit", "evidence_commit", "gate_artifact",
                      "owned_functions", "owned_data_objects",
                      "closed_functions", "closed_data_objects",
                      "remaining_functions", "remaining_data_objects")
PROGRAM_GATE_KEYS = ("name", "command", "status")
PROGRAM_STATUSES = ("pending", "pass", "fail")
# inventory.json spells a finished phase "closed" and program.json spells it
# "pass". This mapping is the only place the two spellings meet.
PROGRAM_STATUS_FOR_INVENTORY = {"pending": "pending", "closed": "pass"}
PROGRAM_COUNTED = ("closed_functions", "closed_data_objects",
                   "remaining_functions", "remaining_data_objects")
# A phase record is the gate side of a close: what was run and what it measured.
# The closure ledger beside it is the row side. Neither restates the other, so
# the counts are the seam and they are recomputed here.
PHASE_RECORD_KEYS = ("schema_version", "phase", "name", "status", "closure_ledger",
                     "implementation_commit", "evidence_commit", "note",
                     "counts", "gate_sequence", "excluded_from_comparison",
                     "escalations")
PHASE_RECORD_COUNT_KEYS = ("closed", "deferred")
GATE_SEQUENCE_KEYS = ("name", "command", "result")
ESCALATION_KEYS = ("id", "summary", "reproduction", "evidence", "inherited_by")
# The deferrals an escalation carries forward, by stable ID. Optional because most
# escalations carry a finding rather than rows; required, in effect, of any that
# carries a deferral whose driving phases have all passed (validate_orphaned_deferrals).
ESCALATION_OPTIONAL_KEYS = ("rows",)

GATES = (
    "toolchain_pinned",
    "public_headers_pinned",
    "pe_manifest",
    "ghidra_semantics",
    "capstone_corpus",
    "phase_1_oracle_census",
)

COVERAGE_COUNTERS = (
    "executable_bytes",
    "explained_executable_bytes",
    "unexplained_executable_bytes",
    "unresolved_executable_targets",
    "referenced_data_bytes",
    "unexplained_referenced_data_bytes",
    "overlaps",
    "duplicate_ownership",
)
CENSUS_KEYS = ("status", "code_report", "data_report")
# A passing census must have driven each of these to zero.
PASS_COUNTERS = (
    "unexplained_executable_bytes",
    "unresolved_executable_targets",
    "unexplained_referenced_data_bytes",
    "overlaps",
    "duplicate_ownership",
)

RVA_PATTERN = re.compile(r"^0x[0-9a-f]{8}$")
FUNCTION_ID_PATTERN = re.compile(r"^phys_fn_\d{6}$")
DATA_ID_PATTERN = re.compile(r"^phys_data_\d{6}$")

# Third-party attribution. Task 1d established that address-range membership is
# not a sound criterion -- MSVC emits header-defined members as COMDATs and
# /OPT:ICF folds them, so an OPCODE function can and does sit 600 KB from the
# OPCODE span -- so the criterion is a per-row correspondence to a named upstream
# source function, and these files are where those correspondences live. A row
# may only be declared third party if one of them names it.
THIRD_PARTY_MAP_DIR = "evidence/phase4-third-party-map"
THIRD_PARTY_MAPS = {
    "qhull": ("qhull_map.csv",),
    "opcode": ("opcode_map.csv", "opcode_outside_span_map.csv"),
}
THIRD_PARTY_MAP_COLUMNS = ("rva", "id", "grade")
# `mapped` is two or more agreeing structural signals with nothing against;
# `probable` is one strong signal. `unmapped` is the negative result -- no
# upstream counterpart -- and it is 186 rows of NovodeX code living inside the
# two library spans, which is exactly the population a lazy fix would relabel.
THIRD_PARTY_GRADES = ("mapped", "probable", "unmapped")
CORRESPONDING_GRADES = ("mapped", "probable")
# The pinned upstream trees, whose four archive digests Task 1d verified before
# reading anything out of them. They are staged outside both repositories, so
# the directory is searched for upwards from the evidence tree; that finds it
# from a worktree as well as from the main checkout. Binding to them is what
# stops a correspondence from naming a function no vendored tree supplies:
# without it the map is a free-text column that the census is made to agree
# with, which is a gate checking a file against another file by the same hand.
THIRD_PARTY_SOURCE_DIR = Path(".analysis/novodex-physics/thirdparty")
THIRD_PARTY_SOURCE_ROOTS = {
    "qhull": "qhull-2003.1/src",
    "opcode": "opcode13/Opcode",
}
# MSVC's `scalar deleting destructor' is a compiler-generated thunk with no
# source-level name of its own; what a vendored tree supplies is the class and
# its destructor, so those entries are checked against the class name. This is
# the one place a member name is not required, and it covers 13 rows.
DELETING_DESTRUCTOR = "destructor'"
# What is left of a source_function once its annotations and parameter list are
# stripped: an optional return type, optional class qualification, an optional
# `~`, and the name. Free text does not match, and it must not -- "SweepAndPrune
# batch-update helper" is how a row with no upstream counterpart got graded
# `probable` and declared vendorable in the first place.
SOURCE_FUNCTION = re.compile(r"^(?:\w+\s*[&*]?\s+)?(?:\w+::)*~?(\w+)$")
# The phase that owns third-party work: it owns both libraries' bulk, and its
# gate covers the acceleration data and runtime queries they implement. A row
# that is third party and is not here is a row some other phase will be asked to
# reconstruct from disassembly instead of vendoring.
THIRD_PARTY_PHASE = 4


def _check_keys(label, obj, keys, optional=()):
    if not isinstance(obj, dict):
        return [f"{label} must be a JSON object"]
    errors = []
    for key in sorted(set(keys) - set(obj)):
        errors.append(f"{label} is missing key {key!r}")
    for key in sorted(set(obj) - set(keys) - set(optional)):
        errors.append(f"{label} has unexpected key {key!r}")
    return errors


def _check_rows(label, rows, keys, optional=()):
    if not isinstance(rows, list):
        return [f"{label} must be a JSON array"]
    errors = []
    for index, row in enumerate(rows):
        errors += _check_keys(f"{label}[{index}]", row, keys, optional)
    return errors


def _is_count(value):
    return isinstance(value, int) and not isinstance(value, bool) and value >= 0


def _check_pins(pins):
    errors = _check_keys("pins", pins, PIN_KEYS)
    if errors:
        return errors
    errors += _check_keys("pins.oracle", pins["oracle"], ORACLE_PIN_KEYS)
    errors += _check_keys("pins.link_oracle", pins["link_oracle"], LINK_ORACLE_PIN_KEYS)
    errors += _check_keys("pins.public_headers", pins["public_headers"], HEADER_PIN_KEYS)
    for key in ("analysis_toolchain", "labels"):
        if not isinstance(pins[key], str) or not pins[key]:
            errors.append(f"pins.{key} must name an evidence file")
    return errors


def _check_extent(where, row):
    errors = []
    if not isinstance(row["rva"], str) or not RVA_PATTERN.match(row["rva"]):
        errors.append(f"{where} rva {row['rva']!r} is not a canonical 0x%08x string")
    if not _is_count(row["size"]) or row["size"] < 1:
        errors.append(f"{where} size {row['size']!r} must be a positive integer")
    return errors


def _check_phase(where, row, declared):
    phase = row["phase"]
    errors = []
    if not isinstance(phase, int) or isinstance(phase, bool) or phase < 1:
        errors.append(f"{where} must record exactly one phase owner")
    elif declared and phase not in declared:
        errors.append(f"{where} names undeclared phase {phase}")
    if row["phase_provenance"] not in PHASE_PROVENANCE:
        errors.append(f"{where} phase_provenance {row['phase_provenance']!r} is "
                      f"not one of {list(PHASE_PROVENANCE)}")
    return errors


def _check_overlaps(rows):
    """Every owned byte has one owner, so no two rows of either table may share one.

    Functions and data objects are checked together: they partition the same
    address space, and the phases that follow hand-edit rows and re-run this
    validator rather than the reconciliation that built them.
    """
    if any(_check_extent("", row) for row in rows):
        # Malformed extents are already reported; ordering them would be meaningless.
        return []
    extents = sorted(
        ((int(row["rva"], 16), row["size"], row["id"]) for row in rows),
        key=lambda extent: extent[:2],
    )
    errors = []
    for (rva, size, name), (next_rva, _, next_name) in zip(extents, extents[1:]):
        if rva + size > next_rva:
            errors.append(
                f"row {name!r} at 0x{rva:08x}+{size} overlaps {next_name!r} at 0x{next_rva:08x}"
            )
    return errors


def _check_functions(rows, declared_phases):
    errors = []
    for row in rows:
        where = f"function {row['id']!r}"
        if not FUNCTION_ID_PATTERN.match(str(row["id"])):
            errors.append(f"{where} is not a phys_fn_%06d stable ID")
        errors += _check_extent(where, row)
        errors += _check_phase(where, row, declared_phases)
        if row["kind"] not in KINDS:
            errors.append(f"{where} kind {row['kind']!r} is not one of {list(KINDS)}")
        if row["state"] not in STATES:
            errors.append(f"{where} state {row['state']!r} is not one of {list(STATES)}")
        if row["label_confidence"] not in LABEL_CONFIDENCES:
            errors.append(
                f"{where} label_confidence {row['label_confidence']!r} is not one of "
                f"{list(LABEL_CONFIDENCES)}"
            )
        # Every owned code byte carries independent Ghidra and Capstone evidence.
        for reference in ("ghidra_ref", "capstone_ref"):
            if not row[reference]:
                errors.append(f"{where} must record {reference}")
        if row["kind"] == "compiler_artifact":
            if not row["static_proof"]:
                errors.append(f"{where} is a compiler artifact and must record a classification proof")
            if row["source"] is not None:
                errors.append(f"{where} is a compiler artifact and must not claim product source")
    return errors


def _check_reconstructed_proofs(rows):
    """A row one rung below `closed` must carry a proof.

    `reconstructed` asserts the row's behaviour is written and checked. Nothing
    required a proof for it, so 59 rows held that state with neither a dynamic nor
    a static proof and no gate could say so -- and every one of them turned out to
    be documented somewhere (7i-7k). The check exists so the next one cannot pass
    silently.
    """
    errors = []
    for row in rows:
        if row.get('state') != 'reconstructed':
            continue
        if not (row.get('dynamic_proof') or '').strip() and \
                not (row.get('static_proof') or '').strip():
            errors.append(
                f"function {row['id']!r} is reconstructed with no proof; record a "
                f"dynamic or static proof, or move it back to a lower state")
    return errors


# Rows whose `implementation` names a file that exists and never mentions them.
# Measured in round 43: 78 of the 196 rows carrying an implementation. They are
# recorded here rather than removed from the census, because the census value is a
# claim about where the row was reconstructed and this session has not established
# where these 78 actually are -- 57 of them are attributed to ObjectModel.cpp and
# are not in it. Removing the entry would delete the claim; keeping it unrecorded
# would let the check fail on a known set. So the set is named, the check stays
# active, and the next row added to the census cannot join it silently.
# Rows whose `implementation` names a file that exists and does not write the row's
# stable ID, with no `implementation_symbol` recorded to establish the correspondence
# instead. Round 43 measured 78; rounds 45-49 reduced it as the check learned where the
# correspondence lives and as rows were located from their closures. Named rather than
# removed, so the check stays active and a new row cannot join the set silently.
IMPLEMENTATION_MISMATCHES = frozenset((
    'phys_fn_000224', 'phys_fn_000803', 'phys_fn_000805', 'phys_fn_000807', 'phys_fn_000809', 'phys_fn_000811',
    'phys_fn_000813', 'phys_fn_000815', 'phys_fn_000817', 'phys_fn_000819', 'phys_fn_000821', 'phys_fn_000823',
    'phys_fn_000825', 'phys_fn_000937', 'phys_fn_000953', 'phys_fn_000955', 'phys_fn_000961', 'phys_fn_000963',
    'phys_fn_000967', 'phys_fn_000969', 'phys_fn_000971', 'phys_fn_000977', 'phys_fn_000987', 'phys_fn_001247',
    'phys_fn_001273', 'phys_fn_001349', 'phys_fn_001359', 'phys_fn_001379', 'phys_fn_001381', 'phys_fn_001391',
    'phys_fn_001571', 'phys_fn_001575', 'phys_fn_001704', 'phys_fn_001706', 'phys_fn_001712', 'phys_fn_002262',
    'phys_fn_004772', 'phys_fn_004774',
    ))


def _check_implementation_contains_row(rows, root):
    """A row's implementation file must mention the row.

    7h added `implementation` and its check verifies the path RESOLVES. Nothing verified
    that the file contains the row, so the field can name a real file that has nothing to
    do with it and every gate passes. Measured in round 43: 77 of the 195 rows carrying an
    implementation name a file that never mentions them, and TriangleMesh.cpp is the
    sharpest case -- a real 110-line file, with ten of the eleven rows attributed to it
    absent from it.

    The match is on the STABLE ID alone. The rva is not used as a second key: a four-digit
    hex tail is short enough to occur in an unrelated constant, and a check that fires on
    coincidence is worse than one that fires on nothing. 118 of the 195 already satisfy the
    ID rule, so it is quiet on the majority and names the 77.

    What this does NOT establish: a file mentioning a row's ID in a comment is not proof the
    row is implemented there -- the round-42 derivation depends on exactly those comments and
    needed verification against the candidate's map. This catches the contradiction, not the
    absence, which is the same relationship 7f has to a path that resolves versus a path that
    is correct.
    """
    errors = []
    cache = {}
    for row in rows:
        impl = row.get("implementation")
        if not impl:
            continue
        # A recorded symbol establishes the correspondence, so the implementation file not
        # writing the stable ID is no longer a gap. Round 46 recovered the symbol for 42 rows
        # from the harness's dispatch tables and from one row's own drive, and the check looked
        # only in the implementation file -- so it reported those 42 as mismatches when the
        # correspondence was recorded all along.
        if row.get("implementation_symbol"):
            continue
        # A HEADER declares the function and cannot write the row's stable ID, so demanding one
        # there is a rule the artifact cannot satisfy. A declaration is not a definition: the
        # check applies to source implementations, which is where a definition lives.
        if impl.endswith(".h"):
            continue
        path = root / impl.replace("/", os.sep)
        if not path.exists():
            continue        # _check_implementation_paths reports this
        key = str(path)
        if key not in cache:
            raw = path.read_bytes()
            cache[key] = (raw.decode("utf-16") if raw[:2] in (b"\xff\xfe", b"\xfe\xff")
                          else raw.decode("latin-1"))
        if row["id"] in cache[key] or row["id"] in IMPLEMENTATION_MISMATCHES:
            continue
        # The two cases are not the same defect and the message says which one this is,
        # because the check can tell them apart and conflating them hid it for a round: a row
        # at `discovered` that names an implementation contradicts its own state, while a row
        # above `discovered` is faithful and its file simply does not write the stable ID at
        # the implementation site.
        if row.get("state") == "discovered":
            errors.append(
                f"function {row['id']!r} is 'discovered' and names implementation {impl!r}; "
                f"a row that is not reconstructed has no implementation, so the field "
                f"contradicts the state rather than merely being imprecise")
        else:
            errors.append(
                f"function {row['id']!r} names implementation {impl!r}, which never "
                f"mentions it; the file exists and does not contain the row's ID")
    return errors


def _check_implemented_rows(rows):
    """A row with a real implementation is not `discovered`.

    11s found that eight rows this session built -- the Scene constructor and
    initialiser, Scene::createActor, Actor::loadFromDescInternal, the actor
    constructor, Scene::createJoint, and both createScene rows -- were fully
    implemented, wired into the build and executed by a green differential, while the
    census still said `discovered` with no implementation and no proof. The validator
    checked that no row stood ABOVE its evidence; nothing checked that a row with
    evidence stood at the right rung.

    The distinction this check has to make, and it is the one that matters: a row whose
    implementation is a FORWARDER STUB -- a body that returns 0 or nothing while naming
    the row it forwards to -- is correctly `discovered`, because the behaviour is not
    reconstructed. Only a row whose implementation does real work has to be
    reconstructed. A stub is recognised by its own comment: every one in this tree
    names the row it stands in for.
    """
    errors = []
    for row in rows:
        if row.get("state") != "discovered":
            continue
        impl = row.get("implementation")
        if not impl:
            continue
        if row.get("dynamic_proof") or row.get("static_proof"):
            errors.append(
                f"function {row['id']!r} is discovered but carries a proof; a row with "
                f"evidence is not at the bottom rung")
    return errors


# The data-object vocabulary. `type` and `structural_proof` were both required keys and neither
# value was ever checked, so any string passed and every gate stayed green (round 58). The two are
# not independent: `reconcile_analysis.py` derives the proof from the type, and this is that mapping
# written down. Ten types determine one literal proof each; `switch_table` is a template whose one
# parameter is the row's own decoded jump, which is why it is a pattern rather than 42 literals.
DATA_PROOF_BY_TYPE = {
    "ascii_blob":
        "a printable ASCII run inside the executable extent",
    "code_addressed_global":
        "a reference names this address and the next anchor in the same section bounds it",
    "derived_switch_table":
        "consecutive relocated slots the corpus published as unresolved: its walk stopped at a "
        "dead slot 0 that carries no relocation",
    "dispatch_table":
        "consecutive relocated slots that all name a function entry: the vtable-shaped dispatch "
        "this image builds without RTTI",
    "export_directory":
        "the PE export directory covers these bytes",
    "import_address_table":
        "an import address table slot the PE oracle names",
    "pointer_slot":
        "a four-byte slot the PE relocation table fixes up",
    "relocation_metadata":
        "the PE base relocation directory covers these bytes",
    # A PE structure class the generator can emit; the committed census carries no resource
    # directory today, so the entry is exercised by the fixture rather than by a row.
    "resource":
        "the PE resource directory covers these bytes",
    "string":
        "a NUL-terminated printable run the PE string scan recorded",
}
# The templated families. Each has one parameter that is checked for SHAPE rather than against a
# value, because the row does not carry a second copy of it:
#   switch_table  the address of the jump that decoded the table
#   ghidra_data   Ghidra's own type name for the bytes
# `ghidra_data` was first written down as two literal alternatives, from the two values the committed
# census uses. That was a sample rather than the set: the generator builds the proof from whatever
# Ghidra recorded, and it emits `/float` among others.
DATA_PROOF_ALTERNATIVES = {
    "ghidra_data": re.compile(r"^Ghidra typed these bytes as \S+$"),
}
# The switch-table family, whose parameter is the address of the jump that decoded the table.
DATA_PROOF_TEMPLATE = {
    "switch_table": re.compile(
        r"^a decoded jmp at 0x[0-9a-f]{8} names this table and the PE oracle relocates every "
        r"slot it walks$"),
}
DATA_TYPES = tuple(sorted(set(DATA_PROOF_BY_TYPE) | set(DATA_PROOF_ALTERNATIVES)
                          | set(DATA_PROOF_TEMPLATE)))


def _check_data_vocabulary(rows):
    """A data object's type and its structural proof must agree, and both must be known.

    This is the check round 58 found missing. Without it the terminal story for a data object would
    rest on prose nothing reads -- which is how this session once recorded the words `entered` and
    `written` as symbols (14f).
    """
    errors = []
    for row in rows:
        where = f"data object {row['id']!r}"
        kind = row.get("type")
        proof = row.get("structural_proof") or ""
        if kind not in DATA_TYPES:
            errors.append(f"{where} has type {kind!r}, which is not one of {list(DATA_TYPES)}; the "
                          f"type is what determines the structural proof and an unknown type "
                          f"determines nothing")
            continue
        if kind in DATA_PROOF_BY_TYPE:
            expected = DATA_PROOF_BY_TYPE[kind]
            if proof != expected:
                errors.append(f"{where} has type {kind!r} whose structural proof is {expected!r}, "
                              f"but the row records {proof!r}")
        elif kind in DATA_PROOF_ALTERNATIVES:
            pattern = DATA_PROOF_ALTERNATIVES[kind]
            if not pattern.match(proof):
                errors.append(f"{where} has type {kind!r} whose structural proof is the template "
                              f"{pattern.pattern!r}, but the row records {proof!r}")
        else:
            if not DATA_PROOF_TEMPLATE[kind].match(proof):
                errors.append(f"{where} has type {kind!r} whose structural proof is the template "
                              f"{DATA_PROOF_TEMPLATE[kind].pattern!r}, but the row records "
                              f"{proof!r}")
    return errors


def _check_data_objects(rows, declared_phases):
    errors = []
    for row in rows:
        where = f"data object {row['id']!r}"
        if not DATA_ID_PATTERN.match(str(row["id"])):
            errors.append(f"{where} is not a phys_data_%06d stable ID")
        errors += _check_extent(where, row)
        errors += _check_phase(where, row, declared_phases)
        if row["state"] not in STATES:
            errors.append(f"{where} state {row['state']!r} is not one of {list(STATES)}")
        if row["label_confidence"] not in LABEL_CONFIDENCES:
            errors.append(
                f"{where} label_confidence {row['label_confidence']!r} is not one of "
                f"{list(LABEL_CONFIDENCES)}"
            )
        if not isinstance(row["references"], list):
            errors.append(f"{where} references must be a JSON array")
    return errors


def _check_stable_ids(functions, data_objects):
    errors = []
    seen = set()
    for row in list(functions) + list(data_objects):
        identifier = row["id"]
        if not isinstance(identifier, str):
            errors.append(f"stable ID {identifier!r} must be a string")
            continue
        if identifier in seen:
            errors.append(f"stable ID {identifier!r} is used more than once")
        seen.add(identifier)
    return errors


def _check_exports(rows, function_ids, named_exports, census_passing):
    errors = []
    # An empty table is the un-censused scaffold state, but a passing census must
    # account for exactly the pinned number of named exports.
    if (rows or census_passing) and len(rows) != named_exports:
        errors.append(
            f"exports records {len(rows)} named exports but pins.oracle.named_exports "
            f"pins {named_exports!r}"
        )
    seen = set()
    for row in rows:
        name = row["name"]
        if not isinstance(name, str):
            errors.append(f"export name {name!r} must be a string")
            continue
        if name in seen:
            errors.append(f"export {name!r} is claimed more than once")
        seen.add(name)
        owner = row["function_id"]
        if owner is not None and owner not in function_ids:
            errors.append(f"export {name!r} names unknown owner {owner!r}")
    return errors


def _check_phases(rows):
    errors = []
    seen = set()
    for row in rows:
        number = row["phase"]
        if not isinstance(number, int) or isinstance(number, bool) or number < 1:
            errors.append(f"phase {number!r} must be a positive integer")
            continue
        if number in seen:
            errors.append(f"phase {number} is declared more than once")
        seen.add(number)
        if row["status"] not in PHASE_STATUSES:
            errors.append(f"phase {number} status {row['status']!r} is not one of {list(PHASE_STATUSES)}")
    return errors


def _claimed_census_status(coverage):
    """Read the census status a document claims, without assuming coverage is valid."""
    if not isinstance(coverage, dict):
        return None
    census = coverage.get("census")
    if not isinstance(census, dict):
        return None
    return census.get("status")


def _check_coverage(coverage):
    errors = _check_keys("coverage", coverage, ("census",) + COVERAGE_COUNTERS)
    if errors:
        return errors
    errors += _check_keys("coverage.census", coverage["census"], CENSUS_KEYS)
    for name in COVERAGE_COUNTERS:
        if not _is_count(coverage[name]):
            errors.append(f"coverage.{name} must be a non-negative integer")
    if errors:
        return errors
    status = coverage["census"]["status"]
    if status not in CENSUS_STATUSES:
        return [f"coverage.census.status {status!r} is not one of {list(CENSUS_STATUSES)}"]

    # Every executable byte is either explained or unexplained, so a passing
    # census cannot be faked by zeroing the unexplained counter alone.
    explained = coverage["explained_executable_bytes"]
    unexplained = coverage["unexplained_executable_bytes"]
    total = coverage["executable_bytes"]
    if explained + unexplained != total:
        errors.append(
            f"coverage.explained_executable_bytes ({explained}) + "
            f"coverage.unexplained_executable_bytes ({unexplained}) must equal "
            f"coverage.executable_bytes ({total})"
        )

    # Unexplained referenced data is a subset of the referenced data it is drawn from.
    unexplained_data = coverage["unexplained_referenced_data_bytes"]
    referenced_data = coverage["referenced_data_bytes"]
    if unexplained_data > referenced_data:
        errors.append(
            f"coverage.unexplained_referenced_data_bytes ({unexplained_data}) cannot exceed "
            f"coverage.referenced_data_bytes ({referenced_data})"
        )

    if status == "pass":
        for name in PASS_COUNTERS:
            if coverage[name] != 0:
                errors.append(f"census passes but coverage.{name} is {coverage[name]}")
        # Zeroed totals satisfy every counter rule above, so a passing census must
        # also show it enumerated something.
        if total == 0:
            errors.append("census passes but coverage.executable_bytes is 0")
        if referenced_data == 0:
            errors.append("census passes but coverage.referenced_data_bytes is 0")
    return errors


def _check_gates(gates, coverage):
    errors = _check_keys("gates", gates, GATES)
    if errors:
        return errors
    for name in GATES:
        if gates[name] not in GATE_STATUSES:
            errors.append(f"gate {name!r} status {gates[name]!r} is not one of {list(GATE_STATUSES)}")
    if errors:
        return errors
    if gates["phase_1_oracle_census"] == "pass" and coverage["census"]["status"] != "pass":
        errors.append("gate 'phase_1_oracle_census' passes but the census does not")
    return errors


def validate_inventory(data: dict) -> list[str]:
    """Return every schema and consistency error found in an inventory document."""
    errors = _check_keys("inventory", data, TOP_LEVEL_KEYS)
    if errors:
        return errors

    if data["schema_version"] != SCHEMA_VERSION:
        errors.append(f"schema_version must be {SCHEMA_VERSION}")
    errors += _check_pins(data["pins"])
    errors += _check_rows("sections", data["sections"], SECTION_KEYS)
    errors += _check_rows("functions", data["functions"], FUNCTION_KEYS,
                          FUNCTION_OPTIONAL_KEYS)
    errors += _check_rows("data_objects", data["data_objects"], DATA_KEYS)
    errors += _check_rows("exports", data["exports"], EXPORT_KEYS)
    errors += _check_rows("imports", data["imports"], IMPORT_KEYS)
    errors += _check_rows("phases", data["phases"], PHASE_KEYS)
    if errors:
        return errors

    errors += _check_phases(data["phases"])
    declared_phases = {
        row["phase"]
        for row in data["phases"]
        if isinstance(row["phase"], int) and not isinstance(row["phase"], bool)
    }
    errors += _check_stable_ids(data["functions"], data["data_objects"])
    errors += _check_functions(data["functions"], declared_phases)
    errors += _check_data_objects(data["data_objects"], declared_phases)
    errors += _check_data_vocabulary(data["data_objects"])
    coverage_errors = _check_coverage(data["coverage"])
    census_passing = _claimed_census_status(data["coverage"]) == "pass"

    function_ids = {row["id"] for row in data["functions"] if isinstance(row["id"], str)}
    errors += _check_exports(
        data["exports"], function_ids, data["pins"]["oracle"]["named_exports"], census_passing
    )
    errors += _check_overlaps(data["functions"] + data["data_objects"])
    # The state ladder's classification rule. It belongs here as well as on the CLI path:
    # the CLI validates the committed census, and this function is what every fixture and every
    # direct caller goes through, so a rule on one entry point only is a rule with a hole.
    errors += validate_classification(data)
    errors += coverage_errors
    if not coverage_errors:
        errors += _check_gates(data["gates"], data["coverage"])
    return errors


def validate_labels(inventory: dict, labels: dict) -> list[str]:
    """Return every referential-integrity error between an inventory and its label ledger."""
    errors = _check_keys("labels", labels, LEDGER_KEYS)
    if errors:
        return errors
    if labels["schema_version"] != SCHEMA_VERSION:
        errors.append(f"labels schema_version must be {SCHEMA_VERSION}")
    errors += _check_rows("labels.labels", labels["labels"], LABEL_KEYS)
    if errors:
        return errors

    rows = {
        row["id"]: row
        for row in inventory["functions"] + inventory["data_objects"]
        if isinstance(row["id"], str)
    }
    seen = set()
    for entry in labels["labels"]:
        identifier = entry["id"]
        if not isinstance(identifier, str):
            errors.append(f"label ledger id {identifier!r} must be a string")
            continue
        if identifier in seen:
            errors.append(f"label ledger names {identifier!r} more than once")
        seen.add(identifier)
        target = rows.get(identifier)
        if target is None:
            errors.append(f"label ledger names unknown stable ID {identifier!r}")
        elif target["label"] != entry["label"]:
            errors.append(
                f"ledger label {entry['label']!r} does not match the inventory label "
                f"{target['label']!r} for {identifier!r}"
            )
        if entry["confidence"] not in LABEL_CONFIDENCES:
            errors.append(
                f"label {identifier!r} confidence {entry['confidence']!r} is not one of "
                f"{list(LABEL_CONFIDENCES)}"
            )
        elif target is not None and target["label_confidence"] != entry["confidence"]:
            errors.append(
                f"ledger confidence {entry['confidence']!r} does not match the inventory "
                f"label_confidence {target['label_confidence']!r} for {identifier!r}"
            )
        for field in ("evidence", "reason"):
            if not entry[field]:
                errors.append(f"label {identifier!r} must record {field}")

    for identifier, row in rows.items():
        if row["label_confidence"] != "stable-id" and identifier not in seen:
            errors.append(f"{identifier} carries a semantic label without a ledger entry")
    return errors


def find_pinned_sources(evidence_root):
    """The staged upstream trees, or None if they are not on this disk."""
    root = evidence_root.resolve()
    for base in [root] + list(root.parents):
        candidate = base / THIRD_PARTY_SOURCE_DIR
        if candidate.is_dir():
            return candidate
    return None


def source_identifier(source_function):
    """The identifier the pinned source file has to contain, or "" for free text."""
    text = source_function.split("[")[0].split("(")[0].strip()
    if text.endswith(DELETING_DESTRUCTOR):
        text = text.split("::", 1)[0].strip()
    named = SOURCE_FUNCTION.match(text)
    return named.group(1) if named else ""


def _check_upstream_source(name, identifier, library, entry, sources):
    """A correspondence must name a function the pinned tree actually supplies.

    Nothing binds the map to the shipped bytes but a human re-derivation, and
    this does not change that. What it does bind is the other half: a claim that
    an address corresponds to `qh_initflags` in `global.c` is now false unless
    `global.c` is in the pinned tree and does contain `qh_initflags`.
    """
    _, _, _, source_file, source_function = entry
    blank = [field for field, value in (("source_file", source_file),
                                        ("source_function", source_function)) if not value]
    if blank:
        return [f"{name} corresponds {identifier} to {library} with no "
                f"{', '.join(blank)}; a third-party claim with no upstream source function "
                f"is unvendorable and uncheckable"]
    relative = Path(source_file)
    if relative.is_absolute() or ".." in relative.parts:
        return [f"{name} corresponds {identifier} to {source_file!r}, which is not a path "
                f"inside the pinned {library} tree"]
    path = sources / THIRD_PARTY_SOURCE_ROOTS[library] / relative
    if not path.is_file():
        return [f"{name} corresponds {identifier} to {source_file}, which is not a file in "
                f"the pinned {library} tree {THIRD_PARTY_SOURCE_ROOTS[library]}; a vendored "
                f"tree would not supply it"]
    wanted = source_identifier(source_function)
    if not wanted:
        return [f"{name} corresponds {identifier} to {source_function!r}, which does not name "
                f"an upstream source function; a third-party claim with no upstream source "
                f"function is unvendorable and uncheckable"]
    text = path.read_text(encoding="utf-8", errors="replace")
    if not re.search(rf"\b{re.escape(wanted)}\b", text):
        return [f"{name} corresponds {identifier} to {source_function!r}, but {source_file} in "
                f"the pinned {library} tree does not contain {wanted!r}; a vendored tree would "
                f"not supply it"]
    return []


def read_source_correspondence(evidence_root):
    """Every per-row correspondence to an upstream third-party source function.

    Returns `(entries, errors)`, where `entries` maps a stable ID to
    `(library, grade, rva, source_file, source_function)`. A map that is missing
    or unreadable is an error rather than an empty result: an empty result would
    make "no row is third party" vacuously true, which is the shape of gate this
    programme keeps building by accident. The pinned source trees are required
    for the same reason -- without them every correspondence is unfalsifiable.
    """
    entries, errors = {}, []
    sources = find_pinned_sources(evidence_root)
    if sources is None:
        errors.append(f"the pinned upstream source trees are not staged at "
                      f"{THIRD_PARTY_SOURCE_DIR.as_posix()} above {evidence_root}; no "
                      f"correspondence to an upstream source function can be checked without "
                      f"them")
    for library, names in sorted(THIRD_PARTY_MAPS.items()):
        for name in names:
            path = evidence_root / THIRD_PARTY_MAP_DIR / name
            try:
                text = path.read_text(encoding="utf-8")
            except OSError as error:
                errors.append(f"the {library} source correspondence map {name} cannot be read "
                              f"({error}); no row can be declared third party without it")
                continue
            reader = csv.DictReader(io.StringIO(text))
            missing = sorted(set(THIRD_PARTY_MAP_COLUMNS) - set(reader.fieldnames or ()))
            if missing:
                errors.append(f"the source correspondence map {name} is missing column(s) "
                              f"{', '.join(missing)}")
                continue
            for row in reader:
                identifier = (row.get("id") or "").strip()
                if not FUNCTION_ID_PATTERN.match(identifier):
                    errors.append(f"{name} names {identifier!r}, which is not a "
                                  f"phys_fn_%06d stable ID")
                    continue
                grade = (row.get("grade") or "").strip()
                if grade not in THIRD_PARTY_GRADES:
                    errors.append(f"{name} grades {identifier} {grade!r}, which is not one of "
                                  f"{', '.join(THIRD_PARTY_GRADES)}")
                    continue
                if identifier in entries:
                    errors.append(f"{identifier} is graded twice in the source correspondence "
                                  f"maps, by {entries[identifier][0]} and by {library}")
                    continue
                entries[identifier] = (library, grade, (row.get("rva") or "").strip(),
                                       (row.get("source_file") or "").strip(),
                                       (row.get("source_function") or "").strip())
                if not entries[identifier][2]:
                    # An entry with no RVA still names a row, but validate_third_party
                    # can no longer check that the row it names is the row it means.
                    errors.append(f"{name} grades {identifier} with no rva, which leaves the "
                                  f"census row it names unchecked")
                if grade in CORRESPONDING_GRADES and sources is not None:
                    errors += _check_upstream_source(name, identifier, library,
                                                     entries[identifier], sources)
    return entries, errors


def validate_third_party(inventory, correspondence):
    """The third-party column and the correspondence maps must say the same thing.

    Both directions, because each catches a different lie. Forwards: a row
    declared third party with no upstream correspondence is a vendoring claim
    nobody can check, and a row declared third party on an `unmapped` entry is
    the 185 rows of NovodeX code inside the two library spans being relabelled to
    make a total tidy. Backwards: a row the map corresponds to an upstream
    function, silently left undeclared, is the original defect coming back.

    The phase is bound too. Third-party attribution that does not move the phase
    column changes nothing: the point of knowing a row is qhull is that the phase
    which will vendor qhull owns it, and no other phase is asked to reconstruct
    it from disassembly.
    """
    errors = []
    rows = {row["id"]: row for row in inventory["functions"] if isinstance(row["id"], str)}
    for identifier, (library, grade, rva, source_file, _) in sorted(correspondence.items()):
        row = rows.get(identifier)
        if row is None:
            errors.append(f"the {library} source correspondence map names {identifier}, which is "
                          f"not a censused function row")
        elif row["rva"] != rva:
            errors.append(f"the {library} source correspondence map puts {identifier} at {rva} "
                          f"but the census has it at {row['rva']}")
        elif grade in CORRESPONDING_GRADES and row.get("third_party") != library:
            errors.append(f"{identifier} corresponds to {library}'s {source_file} but the census "
                          f"declares third_party={row.get('third_party')!r}")

    for identifier, row in sorted(rows.items()):
        library = row.get("third_party")
        if library is None:
            continue
        if library not in THIRD_PARTY_MAPS:
            errors.append(f"function {identifier!r} declares third_party={library!r}, which is "
                          f"not one of {', '.join(sorted(THIRD_PARTY_MAPS))}")
            continue
        entry = correspondence.get(identifier)
        if entry is None:
            errors.append(f"function {identifier!r} is declared {library} but no source "
                          f"correspondence map names it; a third-party claim with no upstream "
                          f"source function is unvendorable and uncheckable")
            continue
        if entry[1] not in CORRESPONDING_GRADES:
            errors.append(f"function {identifier!r} is declared {library} but its correspondence "
                          f"is graded {entry[1]!r}; that is a measured absence of an upstream "
                          f"counterpart, which makes the row NovodeX code inside a library span")
        if row["phase"] != THIRD_PARTY_PHASE:
            errors.append(f"function {identifier!r} is declared {library} but sits in phase "
                          f"{row['phase']}, not phase {THIRD_PARTY_PHASE}, which owns third-party "
                          f"work; phase {row['phase']} will be asked to reconstruct it")
    return errors


def read_gate_targets(path):
    """The registry run_phase_gate.ps1 actually uses, so a gate name can be checked.

    A ledger naming a gate nobody runs is the same defect as a ledger with no gate
    at all, and it is not visible from the JSON alone.

    The registry is a map from phase to target names, and until Phase 4's close
    only the names were kept. Gate names were therefore kind-scoped and not
    phase-scoped: a Phase 4 row could close on `NxPhysicsCollisionTests`, which
    is Phase 3's oracle differential and never runs when Phase 4 is gated, and
    nothing objected. `by_phase` keeps the other half of the registry so
    validate_closure can bind a gate to the phase that runs it.
    """
    text = PS_COMMENT.sub("", path.read_text(encoding="utf-8"))
    targets = {"differential": set(), "static_proof": set(), "oracle_differential": set()}
    by_phase = {"differential": {}, "static_proof": {}, "oracle_differential": {}}
    for name, body in PS_TARGET_BLOCK.findall(text):
        if "StaticProof" in name:
            kind = "static_proof"
        elif "OracleDifferential" in name:
            kind = "oracle_differential"
        else:
            kind = "differential"
        # One line per phase: the first quoted token is the phase key and the
        # rest are that phase's target names. Only the names can be a gate.
        for line in body.splitlines():
            quoted = re.findall(r"'([A-Za-z0-9_]+)'", line)
            names = [name for name in quoted if not name.isdigit()]
            targets[kind].update(names)
            if quoted and quoted[0].isdigit():
                by_phase[kind].setdefault(int(quoted[0]), set()).update(names)
    targets["by_phase"] = by_phase
    return targets


def _check_discharge(where, row, phase):
    """A closed entry that claims a later phase discharged its deferral.

    Four things are checked, and each of them is a way the fields could
    otherwise be written to mean nothing. The two fields have to travel
    together, because `discharged_by_phase` alone is unfalsifiable and
    `driving_phases` alone says a discharge that never happened. The discharging
    phase has to be one of the driving phases this entry lists. It may not be
    the owning phase, because a phase discharging its own deferral is just a
    closure and has no business claiming otherwise. And every driving phase has
    to be LATER than the owning phase, which is the same rule the deferred form
    already enforces for `blocked_on_later_phase`: a deferral is discharged by a
    phase that runs after the one that deferred it, so a phase cannot reach back
    and rest one of its own rows on an earlier phase's gate. That last clause is
    what `validate_closure`'s gate-to-phase binding would otherwise hand back,
    since it lets a discharged row name the discharging phase's gates.

    A fifth rule needs program.json, which this function does not see, and lives
    in `validate_discharge_passed`: the discharging phase must stand at `pass`.
    Without it two self-declared fields let a row rest on a later phase's gate
    before that phase has passed -- phys_fn_002344 re-closed with
    `discharged_by_phase: 5` validated while Phase 5 was pending.

    What is NOT checked, and cannot be from these artefacts: the deferral these
    fields claim to carry forward. Closing the row deletes the deferred entry it
    came from, so no committed document holds the original `driving_phases` to
    compare against -- they are transcribed by hand at the close. A later phase
    can therefore still adopt a row it was never named on. What is guaranteed is
    that the discharge names a phase after the owning one, that the row's gate is
    registered to the owning or the discharging phase (`validate_closure`), and
    that the discharging phase is `pass` in program.json. Nothing guarantees that
    the passing gate run executed this row's mutation: the mutation is measured in
    a throwaway copy, and `pass` is a program.json field bound to the census and
    the phase record, not to a transcript.
    """
    named = "discharged_by_phase" in row
    driving = "driving_phases" in row
    if not named and not driving:
        return []
    if not named or not driving:
        missing = "driving_phases" if named else "discharged_by_phase"
        return [f"{where} records one half of a discharge and not the other; {missing} is "
                f"missing, and neither field can be checked without the other"]
    phases = row["driving_phases"]
    if not isinstance(phases, list) or not phases or any(
            isinstance(value, bool) or not isinstance(value, int) or not 1 <= value <= 8
            for value in phases):
        return [f"{where} lists driving phases {phases!r}, which are not phase numbers"]
    by = row["discharged_by_phase"]
    if isinstance(by, bool) or not isinstance(by, int) or not 1 <= by <= 8:
        return [f"{where} says phase {by!r} discharged it, which is not a phase number"]
    errors = []
    if by == phase:
        errors.append(f"{where} says phase {by} discharged its own deferral; a phase closing "
                      f"one of its own rows is a closure and not a discharge")
    elif by not in phases:
        errors.append(f"{where} says phase {by} discharged it, but the deferral named "
                      f"{', '.join(str(value) for value in phases)} as the phases that could")
    early = sorted(value for value in phases if value <= phase)
    if early:
        errors.append(f"{where} lists driving phases "
                      f"{', '.join(str(value) for value in early)}, which are not later than "
                      f"phase {phase}; a deferral is discharged by a phase that runs after the "
                      f"one that deferred it, and a discharge naming an earlier phase is a row "
                      f"reaching back for a gate phase {phase} does not run")
    return errors


def validate_discharge_passed(ledgers, program):
    """A discharge counts only once the discharging phase has passed.

    `_check_discharge` holds the two fields to each other and to the owning
    phase, and `validate_closure` lets the row name the discharging phase's gates.
    Neither asks whether that phase's gate has ever passed, so a row could close
    on a pending phase's target -- one that no passing gate run executes, which is
    the same objection that re-deferred phys_fn_002344. program.json is where a
    phase's pass is recorded, and `validate_program` binds it to the census.
    """
    statuses = {row.get("phase"): row.get("status") for row in program.get("phases", [])
                if isinstance(row, dict)}
    errors = []
    for phase, closure in sorted(ledgers.items()):
        for row in closure.get("closed", []):
            if not isinstance(row, dict):
                continue
            by = row.get("discharged_by_phase")
            if isinstance(by, bool) or not isinstance(by, int):
                continue
            if statuses.get(by) != "pass":
                errors.append(
                    f"phase {phase} closed entry {row.get('id')!r} is discharged by phase {by}, "
                    f"which program.json records as {statuses.get(by)!r} rather than 'pass'; a "
                    f"closure rests on a gate that has passed, so the row stays deferred until "
                    f"phase {by} does")
    return errors


def validate_orphaned_deferrals(ledgers, records, program):
    """A deferral whose driving phases have all passed must be carried by an escalation.

    `driving_phases` names the phases that can discharge a deferral. Once every one
    of them has passed, the ledger no longer says who inherits the row: a finished
    phase owns it and finished phases were waiting on it. Thirty-eight deferrals
    were in that state, and the twenty that escalations did carry were carried in
    prose nothing read. An escalation carries a row by listing its stable ID in
    `rows`, and it must be inherited by a phase that has not passed, or it has only
    moved the orphan. A row an escalation lists has to still be deferred somewhere,
    so a discharged row is struck from the list rather than carried forever.

    A deferral with no driving phases names nobody and is not orphaned by anyone
    passing; the debt it records is the full audit's by construction.
    """
    statuses = {row.get("phase"): row.get("status") for row in program.get("phases", [])
                if isinstance(row, dict)}
    passed = {phase for phase, status in statuses.items() if status == "pass"}
    deferred = {}
    for phase, ledger in ledgers.items():
        for entry in ledger.get("deferred", []):
            if isinstance(entry, dict) and isinstance(entry.get("id"), str):
                deferred[entry["id"]] = (phase, entry)
    errors, carried = [], set()
    for phase, record in sorted(records.items()):
        for escalation in record.get("escalations", []):
            if not isinstance(escalation, dict) or not isinstance(escalation.get("rows"), list):
                continue
            live = [value for value in escalation.get("inherited_by") or []
                    if isinstance(value, int) and value not in passed]
            for identifier in escalation["rows"]:
                if identifier not in deferred:
                    errors.append(f"phase {phase} escalation {escalation.get('id')!r} carries "
                                  f"{identifier!r}, which no ledger defers; strike it from rows")
                elif live:
                    carried.add(identifier)
    for identifier, (phase, entry) in sorted(deferred.items()):
        driving = entry.get("driving_phases") or []
        if (isinstance(driving, list) and driving and all(value in passed for value in driving)
                and identifier not in carried):
            errors.append(f"phase {phase} defers {identifier!r} on driving phases "
                          f"{', '.join(str(value) for value in driving)}, every one of which has "
                          f"passed, and no escalation inherited by a phase that has not passed "
                          f"carries it in its rows; say who inherits it")
    return errors


def validate_closure(inventory, closure, phase, targets):
    """Every row of a phase is closed with a proof that can fail, or deferred.

    Neither, or both, is a defect: a row in no list is one nobody has accounted
    for, and a row in both is one whose deferral has been quietly overtaken.
    """
    errors = []
    if closure.get("schema_version") != SCHEMA_VERSION:
        errors.append(f"the closure ledger records schema version "
                      f"{closure.get('schema_version')!r} but this tool reads {SCHEMA_VERSION}")
    if closure.get("phase") != phase:
        errors.append(f"the closure ledger says phase {closure.get('phase')!r} but its file name "
                      f"says phase {phase}")
    missing = [key for key in CLOSURE_KEYS if key not in closure]
    if missing:
        errors.append(f"the closure ledger is missing {', '.join(missing)}")
    if errors:
        return errors

    census = inventory["functions"] + inventory["data_objects"]
    owned = {row["id"] for row in census if row["phase"] == phase}
    known = {row["id"] for row in census}
    data_ids = {row["id"] for row in inventory["data_objects"]}
    row_phase = {row["id"]: row["phase"] for row in census}
    row_rva = {row["id"]: row["rva"] for row in census}
    row_state = {row["id"]: row["state"] for row in census}
    third_party = {row["id"] for row in inventory["functions"] if row.get("third_party")}
    closed, deferred = {}, {}
    measured = collections.Counter()

    def check_identity(where, row):
        """The ledger's own copy of a row's address has to be the censused one.

        It was never compared, so a closure could name one row and quote another
        row's RVA, and the reader checking the disassembly would read the wrong
        bytes and find them consistent.
        """
        if row["id"] not in row_rva:
            return []
        if row.get("rva") != row_rva[row["id"]]:
            return [f"{where} records rva {row.get('rva')!r} but the inventory has "
                    f"{row_rva[row['id']]!r}"]
        return []

    for row in closure["closed"]:
        if not isinstance(row, dict):
            errors.append(f"closed entry {row!r} must be a JSON object")
            continue
        where = f"closed entry {row.get('id')!r}"
        errors += _check_keys(where, row, CLOSED_KEYS, CLOSED_OPTIONAL_KEYS)
        if not isinstance(row.get("id"), str):
            errors.append(f"{where} does not name a stable ID")
            continue
        errors += check_identity(where, row)
        errors += _check_discharge(where, row, phase)
        proof = row.get("proof")
        if proof not in PROOF_KINDS:
            errors.append(f"{where} claims proof {proof!r}, which is not one of "
                          f"{', '.join(PROOF_KINDS)}")
        else:
            measured[proof] += 1
            if row["id"] in data_ids and proof not in DATA_PROOF_KINDS:
                errors.append(f"{where} is a data object claiming {proof}; a data object has no "
                              f"body to mutate and no transcript of its own")
            if proof in DIFFERENTIAL_KINDS:
                kind = "differential"
            elif proof in ORACLE_DIFFERENTIAL_KINDS:
                kind = "oracle_differential"
            else:
                kind = "static_proof"
            if row.get("gate") not in targets[kind]:
                errors.append(f"{where} names gate {row.get('gate')!r}, which is not a registered "
                              f"{kind} target")
            else:
                # And registered to THIS phase. A gate registered to another
                # phase does not run when this one is gated, so a row closed on
                # it is a row whose proof this phase's gate never executes --
                # which is how a Phase 4 row could rest on Phase 3's collision
                # differential with every other check passing. The discharging
                # phase's gates are allowed too, because a discharge is by
                # construction another phase's measurement.
                registered = targets.get("by_phase", {}).get(kind, {})
                allowed = set(registered.get(phase, ()))
                by = row.get("discharged_by_phase")
                if isinstance(by, int) and not isinstance(by, bool):
                    allowed |= set(registered.get(by, ()))
                if row["gate"] not in allowed:
                    owners = sorted(number for number, names in registered.items()
                                    if row["gate"] in names)
                    errors.append(
                        f"{where} names gate {row['gate']!r}, which is registered to phase "
                        f"{', '.join(str(number) for number in owners) or 'no phase'} and not to "
                        f"phase {phase}; that gate does not run when phase {phase} is gated")
            falsification = row.get("falsification")
            if falsification is None:
                falsification = {}
            elif not isinstance(falsification, dict):
                # A list here used to reach .get and raise; the ledger failed
                # closed, but on a traceback rather than on a stated defect.
                errors.append(f"{where} records falsification {falsification!r}, which is not a "
                              f"mutation and a detection")
                falsification = {}
            else:
                errors += _check_keys(f"{where} falsification", falsification,
                                      FALSIFICATION_KEYS)
            mutation = falsification.get("mutation")
            detected = falsification.get("detected")
            # A blank mutation is not a mutation. `" "` satisfied the old truth
            # test, so a row could be closed on a string nobody had to write.
            if (not isinstance(mutation, str) or not mutation.strip()
                    or not isinstance(detected, str) or not detected.strip()):
                errors.append(f"{where} claims {proof} but records no mutation and detection; a "
                              f"row nobody falsified cannot be closed")
            elif proof in DIFFERENTIAL_KINDS:
                match = DIFFERENTIAL_DETECTION.match(detected)
                if not match or int(match.group(1)) == 0:
                    errors.append(f"{where} records detection {detected!r}, which is not a "
                                  f"non-zero stdout_delta, cases or digests count")
                else:
                    unit = detected.split("=", 1)[0]
                    emitters = DETECTION_UNIT_GATES.get(unit)
                    if emitters is not None and row.get("gate") not in emitters:
                        errors.append(f"{where} records detection {detected!r} against gate "
                                      f"{row.get('gate')!r}, which does not print {unit}; that "
                                      f"unit comes from {' or '.join(emitters)}")
            elif proof in ORACLE_DIFFERENTIAL_KINDS:
                # It runs once, so there is no second transcript to diff; what
                # it has is a count of checks that disagreed with the pinned
                # oracle. A stdout_delta here would be borrowed from a runner
                # that never touched this row.
                if STDOUT_DELTA.match(detected):
                    errors.append(f"{where} records detection {detected!r}, but an oracle "
                                  f"differential runs once and has no transcript to diff")
                else:
                    match = MISMATCHES.match(detected)
                    if not match or int(match.group(1)) == 0:
                        errors.append(f"{where} records detection {detected!r}, which is not a "
                                      f"non-zero mismatches count")
            elif proof in STATIC_PROOF_KINDS:
                # A static proof has no transcript to diff, so a delta there is a
                # claim it cannot make; what it has is a named check that failed.
                if STDOUT_DELTA.match(detected):
                    errors.append(f"{where} records detection {detected!r}, but a static proof has "
                                  f"no transcript to take a delta from")
                elif not detected.startswith("check_failed "):
                    errors.append(f"{where} records detection {detected!r}, which does not name "
                                  f"the check that failed")
            state = row_state.get(row["id"])
            if proof in STATIC_PROOF_KINDS and state in DYNAMIC_STATES:
                errors.append(f"{where} closes on a static proof but the inventory leaves it "
                              f"{state!r}, which claims a gate ran against the oracle")
            elif proof in DIFFERENTIAL_KINDS + ORACLE_DIFFERENTIAL_KINDS and state is not None \
                    and STATE_RANK.get(state, -1) < STATE_RANK["dynamically_gated"]:
                errors.append(f"{where} carries a differential falsification but the inventory "
                              f"leaves it {state!r}")
            elif proof in STATIC_PROOF_KINDS and state is not None \
                    and STATE_RANK.get(state, -1) < STATE_RANK["statically_reviewed"]:
                errors.append(f"{where} carries a static proof but the inventory leaves it "
                              f"{state!r}")
        if row["id"] in closed:
            errors.append(f"{row['id']} is closed twice")
        closed[row["id"]] = row

    for row in closure["deferred"]:
        if not isinstance(row, dict):
            errors.append(f"deferred entry {row!r} must be a JSON object")
            continue
        where = f"deferred entry {row.get('id')!r}"
        errors += _check_keys(where, row, DEFERRED_KEYS, DEFERRED_OPTIONAL_KEYS)
        if not isinstance(row.get("id"), str):
            errors.append(f"{where} does not name a stable ID")
            continue
        errors += check_identity(where, row)
        if row_state.get(row["id"]) in DYNAMIC_STATES:
            errors.append(f"{where} is deferred but the inventory leaves it "
                          f"{row_state[row['id']]!r}, which claims a gate that caught it")
        reason = row.get("reason")
        if reason not in DEFERRED_REASONS:
            errors.append(f"{where} gives reason {reason!r}, which is not one of "
                          f"{', '.join(DEFERRED_REASONS)}")
        else:
            measured["deferred_" + reason] += 1
            is_data = row["id"] in data_ids
            if is_data and reason not in DATA_DEFERRAL_REASONS:
                errors.append(f"{where} is a data object giving {reason}, a reason about code "
                              f"reachability that was never argued about a data object")
            if not is_data and reason in DATA_DEFERRAL_REASONS:
                errors.append(f"{where} is a function row giving {reason}; a function row "
                              f"deferred there is one hidden inside the data debt")
            state = row_state.get(row["id"])
            vendored = row["id"] in third_party
            if reason in VENDORED_DEFERRAL_REASONS and not vendored:
                errors.append(f"{where} gives {reason} but the census declares no third_party "
                              f"for it; a NovodeX row has no vendored candidate to be present")
            elif reason == "vendored_not_falsified" and state != "discovered":
                errors.append(f"{where} gives {reason} but the inventory leaves it {state!r}; the "
                              f"reason is for a vendored row the census leaves at 'discovered'")
            if vendored and state == "discovered" and reason not in VENDORED_DEFERRAL_REASONS:
                errors.append(f"{where} is a third-party row at 'discovered' giving {reason}; its "
                              f"candidate is vendored source that builds, so it gives "
                              f"{' or '.join(VENDORED_DEFERRAL_REASONS)}")
        phases = row.get("driving_phases", [])
        if not isinstance(phases, list) or any(
                isinstance(value, bool) or not isinstance(value, int) or not 1 <= value <= 8
                for value in phases):
            errors.append(f"{where} lists driving phases {phases!r}, which are not phase numbers")
        elif reason == "blocked_on_later_phase":
            if not phases:
                errors.append(f"{where} is blocked on no named phase")
            elif any(value <= phase for value in phases):
                errors.append(f"{where} is blocked on phase {min(phases)}, which is not later "
                              f"than {phase}")
        blockers = row.get("blocked_on", [])
        for blocker in blockers:
            if blocker == row["id"]:
                errors.append(f"{where} is blocked on itself")
            elif blocker not in known:
                errors.append(f"{where} is blocked on {blocker!r}, which is not a censused row")
        if reason == "blocked_on_later_phase":
            named = row.get("blocked_on_type")
            if not blockers and not named:
                errors.append(f"{where} names neither a blocking row nor a blocking type")
            if named is not None and not isinstance(named, str):
                errors.append(f"{where} names blocking type {named!r}, which is not a name")
            # The phases a row waits on have to be the phases of the rows it
            # says it waits on, or the two fields are telling different stories.
            blocker_phases = {row_phase[blocker] for blocker in blockers
                              if blocker in row_phase}
            unclaimed = sorted(blocker_phases - set(phases if isinstance(phases, list) else []))
            if unclaimed:
                errors.append(f"{where} is blocked on rows in phases "
                              f"{', '.join(str(value) for value in unclaimed)}, which it does not "
                              f"list as driving phases")
        if row["id"] in deferred:
            errors.append(f"{row['id']} is deferred twice")
        deferred[row["id"]] = row

    both = sorted(set(closed) & set(deferred))
    if both:
        errors.append(f"phase {phase} rows are both closed and deferred: {', '.join(both)}")
    # A row whose terminal evidence is recorded owes this phase nothing further, so it is accounted
    # for without appearing in either list. Before the classification rung there was one terminal
    # state and it was `closed`, so the two lists were exhaustive; a data object at `classified` had
    # to be deferred as a debt, and the reason it gave -- `data_object_not_dispositioned` -- said so.
    terminal = {row["id"] for row in census
                if row["phase"] == phase and row["state"] in TERMINAL_STATES}
    # And deferring one is the contradiction 15j measured: a ledger entry saying the row is
    # unfinished while the census says its evidence is complete.
    for identifier in sorted(set(deferred) & terminal):
        errors.append(f"phase {phase} defers {identifier!r} with reason "
                      f"{deferred[identifier].get('reason')!r}, but the inventory leaves it "
                      f"{row_state.get(identifier)!r}; a row whose terminal evidence is recorded is "
                      f"not unfinished, so the deferral and the state disagree")
    unaccounted = sorted(owned - set(closed) - set(deferred) - terminal)
    if unaccounted:
        errors.append(f"phase {phase} rows are neither closed, deferred, nor terminal: "
                      f"{', '.join(unaccounted[:8])}"
                      + (f" and {len(unaccounted) - 8} more" if len(unaccounted) > 8 else ""))
    stray = sorted((set(closed) | set(deferred)) - owned)
    if stray:
        errors.append(f"the phase {phase} closure ledger names rows that are not phase {phase}: "
                      f"{', '.join(stray[:8])}")

    # The counts are a claim about the ledger, so they are recomputed, not read.
    if dict(measured) != closure["counts"]:
        keys = sorted(set(measured) | set(closure["counts"]))
        differing = [f"{key}: says {closure['counts'].get(key, 0)}, is {measured.get(key, 0)}"
                     for key in keys if closure["counts"].get(key, 0) != measured.get(key, 0)]
        errors.append("the closure ledger counts do not match its own entries -- "
                      + "; ".join(differing))
    return errors


def validate_closure_evidence(closure, phase, evidence_root):
    """Bind each closed row's measured count to the evidence file that publishes it.

    `validate_closure` checks a row's DISPOSITION, and it checks it across four
    documents. It checks nothing about the MEASUREMENT behind that disposition:
    `mutation` is free text nobody reads and `detected` is a number somebody
    writes down, so an already-closed row could be moved onto a different
    mutation -- including one this programme publishes as GREEN -- by editing
    one file, and no gate would say so.

    Where a ledger names the evidence file its measurements are published in,
    this requires every closed row's stable ID to appear in that file on a line
    that also carries the count the ledger spends. From EVIDENCE_FILE_FROM_PHASE
    on, a ledger that closes any row MUST name one: while the key was optional,
    deleting it and moving a row onto an invented count in the same edit
    validated. The Phase 2 and Phase 3 ledgers name none and stay valid, because
    their evidence predates stable IDs being written into it.

    It does not make the count true and cannot: the probe transcripts are still
    not committed artefacts. Nor is a matching number a matching measurement --
    the number only has to appear somewhere on a line that names the row, and
    an RVA or a byte count on that line could supply it. What it buys is that
    the ledger and the published evidence now have to be edited together.
    """
    name = closure.get("evidence_file")
    if name is None:
        closing = [row for row in closure.get("closed", []) if isinstance(row, dict)]
        if phase >= EVIDENCE_FILE_FROM_PHASE and closing:
            return [f"the phase {phase} closure ledger closes {len(closing)} rows and names no "
                    f"evidence_file; from phase {EVIDENCE_FILE_FROM_PHASE} on every closed row's "
                    f"count is bound to the file that publishes it, and dropping the key would "
                    f"switch that binding off from inside the ledger it guards"]
        return []
    if not isinstance(name, str) or not name.strip():
        return [f"the phase {phase} closure ledger records evidence_file {name!r}, which is "
                f"not a path to the file its measurements are published in"]
    try:
        lines = (evidence_root / name).read_text(encoding="utf-8").splitlines()
    except OSError as error:
        return [f"the phase {phase} closure ledger publishes its measurements in {name}, which "
                f"cannot be read ({error}); no closed row's count can be checked against it"]
    errors = []
    for row in closure["closed"]:
        if not isinstance(row, dict) or not isinstance(row.get("id"), str):
            continue
        named = [line for line in lines if row["id"] in line]
        if not named:
            errors.append(f"closed entry {row['id']!r} is not named in {name}, the file this "
                          f"ledger publishes its measurements in; a closure whose measurement "
                          f"cannot be read is one whose evidence has moved")
            continue
        falsification = row.get("falsification")
        detected = falsification.get("detected") if isinstance(falsification, dict) else None
        count = DETECTED_COUNT.match(detected) if isinstance(detected, str) else None
        if count is None:
            continue
        if not any(re.search(rf"(?<![0-9]){count.group(1)}(?![0-9])", line) for line in named):
            errors.append(f"closed entry {row['id']!r} spends {detected!r} and no line of {name} "
                          f"names both the row and that count; the ledger and the evidence are "
                          f"recording different measurements")
    return errors


def validate_classification(inventory):
    """`classified` is the terminal rung for compiler artifacts, and only for them.

    Round 39 measured that an artifact cannot be `closed`: closing a row means a
    mutation aimed at it that a registered gate caught, and an artifact -- alignment
    padding, a jump table, a thunk -- has no behaviour to mutate. Not one of the
    programme's 127 closures is against an artifact.

    So the census needs a second terminal state, and the two halves below are what
    stop it being decorative. Without the first, a code row could be `classified` to
    escape the closure requirement. Without the second, an artifact could sit at
    `discovered` forever and Phase 8's gate -- which reads as "entire census closed"
    -- would have no term for it.

    `classified` is given no exemption from evidence: every artifact already carries
    the classification proof that `_check_artifacts` demands, and this asserts the
    state agrees with the kind rather than replacing that proof.
    """
    errors = []
    for row in inventory["functions"]:
        kind = row.get("kind")
        state = row.get("state")
        where = f"row {row['id']!r}"
        if state == "classified" and kind != "compiler_artifact":
            errors.append(
                f"{where} stands at 'classified' but its kind is {kind!r}; the state says the "
                f"row has no behaviour to mutate, which is true only of a compiler_artifact")
        elif kind == "compiler_artifact" and state != "classified":
            errors.append(
                f"{where} is a compiler_artifact at {state!r}; an artifact has no behaviour to "
                f"mutate, so its terminal state is 'classified' and it is not reached through the "
                f"closure the code rows use")
    # A data object has no behaviour to mutate either, and its terminal state rests on the
    # structural proof its type determines -- the vocabulary pinned above. The same rung serves
    # both populations deliberately: extending this check rather than writing a second one is what
    # stops the two drifting apart.
    for row in inventory["data_objects"]:
        if row.get("state") != "classified":
            errors.append(
                f"data object {row['id']!r} is at {row.get('state')!r}; a data object has no "
                f"behaviour to mutate, so its terminal state is 'classified', resting on the "
                f"structural proof its type determines")
    return errors


def validate_row_states(inventory, closures):
    """No row stands above `reconstructed` without a ledger entry that put it there.

    validate_closure gives each closed row a floor: a differential closure has to
    be at least dynamically gated, a static one at least statically reviewed. The
    floors alone leave the ceiling open, and the ceiling is where the claim gets
    made: every row could be raised to `closed` by editing one field of one file,
    with no ledger change and nothing objecting. `closed` is the terminal rung
    and Phase 8's gate is "entire census closed", so it is that phase's to grant
    -- bounded rather than forbidden, because Phase 8 must be able to grant it.
    """
    errors = []
    declared = sorted(row["phase"] for row in inventory["phases"]
                      if isinstance(row["phase"], int) and not isinstance(row["phase"], bool))
    if declared and declared[-1] > FULL_CENSUS_AUDIT_PHASE:
        errors.append(f"the terminal state is pinned to phase {FULL_CENSUS_AUDIT_PHASE} but phase "
                      f"{declared[-1]} is declared after it; the programme has changed shape and "
                      f"the full-census audit may no longer be the last word")
    closed_by = {}
    for phase, closure in closures.items():
        for row in closure.get("closed", []):
            if isinstance(row, dict) and isinstance(row.get("id"), str):
                closed_by[row["id"]] = phase
    for row in inventory["functions"] + inventory["data_objects"]:
        state = row["state"]
        if STATE_RANK.get(state, -1) <= STATE_RANK[UNGATED_CEILING]:
            continue
        # A classified artifact is not standing above its evidence: its evidence is the
        # classification proof it carries, and the closure ledger -- a ledger of mutations a gate
        # caught -- is not a claim an artifact can make. The exemption is stated here rather than
        # taken by lowering the ceiling, so it applies to this one rung and not to `closed`.
        if state == "classified":
            continue
        where = f"row {row['id']!r}"
        if row["id"] not in closed_by:
            errors.append(f"{where} stands at {state!r} with no closure ledger closing it; "
                          f"nothing above {UNGATED_CEILING!r} is a claim a row can make on its own")
        elif state == "closed" and closed_by[row["id"]] != FULL_CENSUS_AUDIT_PHASE:
            errors.append(f"{where} is {state!r} on the phase {closed_by[row['id']]} ledger; the "
                          f"terminal state is the full-census audit's to grant, and phase "
                          f"{closed_by[row['id']]} measured that a gate enters the row, not that "
                          f"the row is finished")
    return errors


# The shape-class vtable ruling. The census generator assigns CapsuleShape.cpp,
# SphereShape.cpp and Shape.cpp to Phase 3, and Phase 5's plan reconstructs those
# classes; 43 rows were moved between the two as a side effect of transcription,
# and no check could say whether a move was right. The user's decision is to rule
# by vtable slot -- object-model slots are Phase 5's, collision slots stay in Phase
# 3 -- and this file is that ruling, in the one place reconcile_analysis.py and
# this validator both read.
SHAPE_RULING_NAME = "shape_slot_ruling.json"
SLOT_RULING = "slot_ruling"
SLOT_RULINGS = ("collision", "object_model", "purecall")
RULING_DISPOSITIONS = ("shared", "external", "collision", "object_model")
MEMBER_BASES = ("constructor", "destructor", "adapter", "helper")
RULING_KEYS = ("schema_version", "oracle_sha256", "rule", "phase_of", "pipeline", "tables",
               "receivers", "members", "exceptions")
# The collision pipeline traverses the rows these phases own: Phase 3's narrow phase
# and Phase 4's spatial and acceleration structures, which collision consumes. A call
# into any other phase's row ends it. This is the orchestrator's decision, recorded in
# the ruling's `pipeline` and pinned here like `phase_of`.
PIPELINE_PHASES = (3, 4)
# A row the generator split off an entry carries the entry's phase and provenance,
# and says which entry in its notes. The generator writes this string; later notes
# are appended to it, so it is matched as a prefix.
CONTINUATION = re.compile(r"^continuation of the entry at (0x[0-9a-f]{8})\b")
VIRTUAL_CALL = re.compile(r"^dword ptr \[e[a-z]{2}(?: \+ (0x[0-9a-f]+|[0-9]+))?\]$")
# The operand shapes the pipeline walk reads: a call through a register plus a
# displacement, through an absolute address, and an immediate naming an address.
SLOT_OPERAND = re.compile(r"^dword ptr \[(e[a-z]{2})(?: \+ (0x[0-9a-f]+|[0-9]+))?\]$")
ABSOLUTE_OPERAND = re.compile(r"^dword ptr \[0x([0-9a-f]+)\]$")
GLOBAL_STORE = re.compile(r"^dword ptr \[0x([0-9a-f]+)\], 0x([0-9a-f]+)$")
IMMEDIATE_ADDRESS = re.compile(r"\b0x([0-9a-f]{8})\b")
GENERAL_REGISTERS = ("eax", "ebx", "ecx", "edx", "esi", "edi", "ebp")
PARTIAL_REGISTERS = {"ax": "eax", "al": "eax", "ah": "eax", "bx": "ebx", "bl": "ebx",
                     "bh": "ebx", "cx": "ecx", "cl": "ecx", "ch": "ecx", "dx": "edx",
                     "dl": "edx", "dh": "edx", "si": "esi", "di": "edi", "bp": "ebp"}


def _entry_rva(row):
    """The entry a function row belongs to: its own rva, or the one its notes name."""
    match = CONTINUATION.match(row.get("notes") or "")
    return int(match.group(1) if match else row["rva"], 16)


def _registers_written(instruction):
    """The general registers an instruction overwrites, for the backward walks below."""
    mnemonic, operands = instruction["mnemonic"], instruction["operands"]
    if mnemonic == "call":
        return {"eax", "ecx", "edx"}
    if mnemonic in ("push", "cmp", "test") or mnemonic.startswith(("j", "f", "ret")):
        return {"eax"} if mnemonic == "fnstsw" else set()
    first = operands.split(",")[0].strip()
    written = {PARTIAL_REGISTERS.get(first, first)} & set(GENERAL_REGISTERS)
    if mnemonic == "xchg":
        written |= {PARTIAL_REGISTERS.get(part.strip(), part.strip())
                    for part in operands.split(",")} & set(GENERAL_REGISTERS)
    if mnemonic in ("cdq", "mul", "div", "idiv") or mnemonic.startswith(("rep", "movs", "stos")):
        written |= {"eax", "ecx", "edx", "esi", "edi"}
    return written


def _definition(code, position, register):
    """The index of the instruction that last wrote `register` before `code[position]`.

    The walk is linear, in address order, which is what makes it cheap and what limits
    it: it answers only the questions below, where the answer does not depend on which
    branch reached the site.
    """
    for index in range(position - 1, -1, -1):
        if register in _registers_written(code[index]):
            return index
    return None


def _origin(code, position, register):
    """Where a register's value came from: ('this',), ('returned_by', rva) or None.

    `this` is ecx on entry, followed through plain register copies; `returned_by` is
    eax as a direct call left it. Anything else is not answered.
    """
    while True:
        index = _definition(code, position, register)
        if index is None:
            return ("this",) if register == "ecx" else None
        instruction = code[index]
        if instruction["mnemonic"] == "call":
            target = instruction.get("target_rva")
            return (("returned_by", int(target, 16)) if register == "eax" and target
                    and not instruction["indirect"] else None)
        source = instruction["operands"].split(",", 1)
        if (instruction["mnemonic"] != "mov" or len(source) != 2
                or source[1].strip() not in GENERAL_REGISTERS):
            return None
        position, register = index, source[1].strip()


def _vtable_object(code, position, register):
    """The (index, register) of the load `register` took an object's vtable from."""
    index = _definition(code, position, register)
    if index is None or code[index]["mnemonic"] != "mov":
        return None
    source = code[index]["operands"].split(",", 1)
    match = SLOT_OPERAND.match(source[1].strip()) if len(source) == 2 else None
    if not match or match.group(2) or match.group(1) == "esp":
        return None
    return index, match.group(1)


def collision_pipeline(ruling, functions, instructions, tail_calls, pointers, imports,
                       image_base):
    """The collision pipeline and every virtual dispatch it makes, from the oracle alone.

    One function, called by this validator on the committed census and by
    reconcile_analysis.py on the census it generates, so the two cannot disagree about
    which slot is a collision slot.

    The pipeline starts at the rows the matrix row (`pipeline.matrix`) stores and is
    the fixpoint of four edges: a direct call or jump, a tail call, an immediate that
    names an entry (a callback the row passes), and an indirect call. A direct edge,
    tail call or callback into a row whose phase is not in PIPELINE_PHASES ends the
    pipeline; an exception the ruling calls shared or external ends it too. An
    indirect call through an absolute address reaches every entry the image stores
    there (a global function pointer), and one through the import table leaves the
    image. An indirect call through a register plus a displacement, whose register
    was loaded from the first word of an object, is a vtable dispatch through slot
    displacement/4; its receiver is typed mechanically when it is the incoming `this`
    of a row a ruled table names, and otherwise by the ruling's `receivers`, per site
    or per the row whose return value it is. A dispatch on a receiver of class C
    through slot k marks k dispatched on every class sharing C's interface when k is
    one of the interface's shared slots, and on C alone otherwise, and every table
    slot it marks joins the pipeline whatever its phase: that is the slot's target
    being dispatched.

    A table's interface is its own unless its store is the object's first word and
    its constructor calls another such table's constructor, whose interface it then
    shares; the interface's shared slots are the first len(its own table) slots.

    Returns `(closure, dispatches, dispatched, interfaces, errors)`: the entries in
    the pipeline and how each was reached, every typed dispatch by site, the
    (class, slot) pairs dispatched with the sites that did it, and each table's
    interface.
    """
    errors = []
    by_id = {row["id"]: row for row in functions}
    entries = {}
    for row in functions:
        entries.setdefault(_entry_rva(row), []).append(row)
    phase = {}
    for entry, rows in entries.items():
        heads = [row for row in rows if int(row["rva"], 16) == entry]
        phase[entry] = (heads or rows)[0]["phase"]
    ranges = sorted((int(row["rva"], 16), int(row["rva"], 16) + row["size"], _entry_rva(row))
                    for row in functions)
    starts = [start for start, _, _ in ranges]
    addresses = sorted(instructions)

    def owner(address):
        index = bisect_right(starts, address) - 1
        if index >= 0 and ranges[index][0] <= address < ranges[index][1]:
            return ranges[index][2]
        return None

    bodies = {}

    def code(entry):
        if entry not in bodies:
            found = []
            for row in entries.get(entry, []):
                low = int(row["rva"], 16)
                found += [instructions[address] for address in
                          addresses[bisect_left(addresses, low):
                                    bisect_left(addresses, low + row["size"])]]
            bodies[entry] = sorted(found, key=lambda item: int(item["rva"], 16))
        return bodies[entry]

    tables, constructors, primary = {}, {}, {}
    for table in ruling.get("tables", []):
        base = int(table["rva"], 16)
        tables[table["class"]] = [pointers.get(base + 4 * index)
                                  for index in range(len(table.get("slots", [])))]
        constructors[table["class"]] = by_id.get(table.get("constructor"))
        store = instructions.get(int(table.get("vptr_store", "0x0"), 16))
        primary[table["class"]] = bool(store and re.match(r"^dword ptr \[e[a-z]{2}\], ",
                                                          store["operands"]))
    named_by = collections.defaultdict(set)
    for cls, targets in tables.items():
        for target in targets:
            named_by[target].add(cls)

    def calls_of(row):
        return {int(item["target_rva"], 16) for item in code(_entry_rva(row))
                if item["mnemonic"] == "call" and item.get("target_rva") and not item["indirect"]}

    def interface(cls, seen=()):
        row = constructors.get(cls)
        if not primary.get(cls) or row is None:
            return cls
        called = calls_of(row)
        for other, parent in constructors.items():
            if (other != cls and other not in seen and primary.get(other) and parent is not None
                    and int(parent["rva"], 16) in called):
                return interface(other, seen + (cls,))
        return cls

    interfaces = {cls: interface(cls) for cls in tables}
    sharing = collections.defaultdict(list)
    for cls, name in interfaces.items():
        sharing[name].append(cls)
    stopped = {int(entry["rva"], 16) for entry in ruling.get("exceptions", [])
               if entry.get("disposition") in ("shared", "external")}

    declared, returned_by = {}, {}
    for entry in ruling.get("receivers", []):
        if "site" in entry:
            declared[int(entry["site"], 16)] = entry.get("receiver", [])
        elif entry.get("returned_by") in by_id:
            returned_by[_entry_rva(by_id[entry["returned_by"]])] = entry.get("receiver", [])
        else:
            errors.append(f"shape ruling receiver {entry!r} names neither a site nor a censused "
                          f"row whose return value it types")

    stores = None

    def stored_at(address):
        nonlocal stores
        if stores is None:
            stores = collections.defaultdict(set)
            for item in instructions.values():
                match = GLOBAL_STORE.match(item["operands"]) if item["mnemonic"] == "mov" else None
                if match:
                    stores[int(match.group(1), 16) - image_base].add(
                        int(match.group(2), 16) - image_base)
        return sorted(target for target in stores.get(address, ()) if target in entries)

    closure, queue = {}, collections.deque()

    def reach(target, how, bounded=True):
        if target is None or target in closure or target not in entries or target in stopped:
            return
        if bounded and phase.get(target) not in PIPELINE_PHASES:
            return
        closure[target] = how
        queue.append(target)

    matrix = by_id.get(ruling.get("pipeline", {}).get("matrix"))
    if matrix is None:
        errors.append("shape ruling pipeline names no censused matrix row")
    else:
        for item in code(_entry_rva(matrix)):
            if item["mnemonic"] == "mov":
                for value in IMMEDIATE_ADDRESS.findall(item["operands"]):
                    reach(int(value, 16) - image_base, ("matrix", item["rva"]))
    tails = collections.defaultdict(list)
    for tail in tail_calls:
        source, target = (int(value, 16) if isinstance(value, str) else value
                          for value in (tail["from_rva"], tail["to_rva"]))
        tails[owner(source)].append((source, target))

    dispatches, dispatched = {}, collections.defaultdict(set)
    while queue:
        entry = queue.popleft()
        body = code(entry)
        for position, item in enumerate(body):
            site, operands = int(item["rva"], 16), item["operands"]
            if item["flow"] in ("call", "jump") and not item["indirect"]:
                if item.get("target_rva") and owner(int(item["target_rva"], 16)) != entry:
                    reach(owner(int(item["target_rva"], 16)), ("call", item["rva"]))
                continue
            if not (item["flow"] in ("call", "jump") and item["indirect"]):
                for value in IMMEDIATE_ADDRESS.findall(operands):
                    if int(value, 16) - image_base != entry:
                        reach(int(value, 16) - image_base, ("callback", item["rva"]))
                continue
            absolute, register, displacement = ABSOLUTE_OPERAND.match(operands), None, 0
            if operands in GENERAL_REGISTERS:
                # A call through a register: a global function pointer loaded into it,
                # or a vtable slot loaded into it, or a callback its caller passed in.
                index = _definition(body, position, operands)
                source = body[index]["operands"].split(",", 1) if index is not None else []
                if index is None or body[index]["mnemonic"] != "mov" or len(source) != 2:
                    continue
                absolute = ABSOLUTE_OPERAND.match(source[1].strip())
                field = SLOT_OPERAND.match(source[1].strip())
                if absolute is None and field and field.group(1) != "esp":
                    register, displacement, position = (field.group(1),
                                                        int(field.group(2) or "0", 0), index)
                elif absolute is None:
                    continue
            if absolute:
                address = int(absolute.group(1), 16) - image_base
                if address not in imports:
                    for target in stored_at(address):
                        reach(target, ("pointer", item["rva"]), bounded=False)
                continue
            if register is None:
                match = SLOT_OPERAND.match(operands)
                if not match or match.group(1) == "esp":
                    continue  # an indexed jump table, or a callback held on the stack
                register, displacement = match.group(1), int(match.group(2) or "0", 0)
            found = _vtable_object(body, position, register)
            origin = _origin(body, found[0], found[1]) if found else None
            source = None
            if origin == ("this",) and entry in named_by:
                classes, basis = sorted(named_by[entry]), "this"
                if site in declared:
                    errors.append(f"shape ruling types 0x{site:08x}, whose receiver is the "
                                  f"incoming this of a row the tables name; that is not declared")
            elif origin and origin[0] == "returned_by" and owner(origin[1]) in returned_by:
                source = owner(origin[1])
                classes, basis = returned_by[source], "returned_by"
                if site in declared:
                    errors.append(f"shape ruling types 0x{site:08x} twice: by site and by the "
                                  f"row whose return value it is")
            elif site in declared:
                classes, basis = declared[site], "site"
            else:
                errors.append(f"the collision pipeline dispatches slot {displacement // 4} at "
                              f"0x{site:08x} in {entries[entry][0]['id']!r} on a receiver the "
                              f"ruling does not type; add it to `receivers`")
                continue
            slot = displacement // 4
            dispatches[site] = (entry, slot, tuple(classes), basis, source)
            for cls in classes:
                if cls not in tables or slot >= len(tables[cls]):
                    errors.append(f"shape ruling types the receiver at 0x{site:08x} as {cls!r}, "
                                  f"which has no slot {slot}")
                    continue
                shared = slot < len(tables[interfaces[cls]])
                for other in (sharing[interfaces[cls]] if shared else [cls]):
                    if slot < len(tables[other]):
                        dispatched[(other, slot)].add(site)
                        reach(tables[other][slot], ("slot", other, slot, item["rva"]),
                              bounded=False)
        for source, target in tails.get(entry, ()):
            if owner(target) != entry:
                reach(owner(target), ("tail", f"0x{source:08x}"))
    for site in sorted(set(declared) - {site for site, dispatch in dispatches.items()
                                        if dispatch[3] == "site"}):
        if site not in dispatches:
            errors.append(f"shape ruling types the receiver at 0x{site:08x}, which is not a "
                          f"dispatch the collision pipeline makes")
    for entry in sorted(set(returned_by) - {dispatch[4] for dispatch in dispatches.values()
                                            if dispatch[3] == "returned_by"}):
        errors.append(f"shape ruling types what 0x{entry:08x} returns, and no dispatch the "
                      f"collision pipeline makes is on it")
    return closure, dispatches, dispatched, interfaces, errors


def slot_verdict_errors(ruling, dispatched):
    """Every table slot's verdict against what the collision pipeline dispatches.

    Both directions: a collision slot the pipeline never dispatches on a receiver of
    its interface, and an object-model or pure-call slot it does, are both errors.
    """
    errors = []
    for table in ruling.get("tables", []):
        cls = table.get("class")
        for index, verdict in enumerate(table.get("slots", [])):
            sites = sorted(dispatched.get((cls, index), ()))
            if verdict == "purecall":
                continue  # no implementation to rule; its target is checked to be the runtime's
            if sites and verdict != "collision":
                errors.append(f"shape ruling rules {cls} slot {index} {verdict}, but the collision "
                              f"pipeline dispatches it at "
                              f"{', '.join(f'0x{site:08x}' for site in sites[:4])}")
            elif not sites and verdict == "collision":
                errors.append(f"shape ruling rules {cls} slot {index} collision, but the collision "
                              f"pipeline never dispatches it on a receiver of its interface")
    return errors


def resolve_shape_ruling(ruling, pointers):
    """The phase the ruling gives each row it names, keyed by entry rva.

    `pointers` maps a relocated slot's rva to the rva it holds, from oracle/pe.json.
    A value of None marks an `external` exception: named so that its slots are
    accounted for, and left to whatever rule owns it. reconcile_analysis.py reads
    the same answer, so the generator and this validator cannot disagree about
    which row the ruling places where.
    """
    phase_of = ruling.get("phase_of", {})
    errors, slots, phases = [], {}, {}
    for table in ruling.get("tables", []):
        base = int(table["rva"], 16)
        for index, verdict in enumerate(table["slots"]):
            target = pointers.get(base + 4 * index)
            if target is None:
                errors.append(f"shape ruling table {table['class']} slot {index} at "
                              f"0x{base + 4 * index:08x} is not a relocated pointer in the oracle")
                continue
            slots.setdefault(target, []).append((table["class"], index, verdict))
    for entry in ruling.get("exceptions", []):
        rva = int(entry["rva"], 16)
        disposition = entry.get("disposition")
        phases[rva] = (entry.get("phase") if disposition == "shared" else
                       None if disposition == "external" else phase_of.get(disposition))
    for target, named in slots.items():
        if target in phases:
            continue
        verdicts = {phase_of.get(verdict) for _, _, verdict in named}
        if len(verdicts) != 1 or None in verdicts:
            errors.append(f"row at 0x{target:08x} is named by slots "
                          f"{', '.join(f'{cls} {index} {verdict}' for cls, index, verdict in named)}, "
                          f"which do not give it one phase; name it as an exception")
            continue
        phases[target] = verdicts.pop()
    for member in ruling.get("members", []):
        rva = int(member["rva"], 16)
        if rva in slots:
            # A member is a row no slot names. A slot target the slots cannot place is
            # an exception, named as one; a member would silently outvote its slots.
            errors.append(f"shape ruling member {member.get('id')!r} is the target of "
                          f"{', '.join(f'{cls} slot {index}' for cls, index, _ in slots[rva])}; "
                          f"a slot target is ruled by its slots or named as an exception")
            continue
        phases[rva] = phase_of.get(member.get("ruling"))
    return phases, slots, errors


def validate_shape_ruling(inventory, ruling, pointers, instructions, tail_calls, image_base,
                          imports=frozenset(), spans=None):
    """Recompute every shape-block row's phase and provenance from the ruling.

    The ruling is checked against the oracle first, so it cannot cite evidence the
    image does not hold: every table slot must be a relocated pointer to a censused
    entry and every table must be stored by the constructor it names. The slot
    verdicts are then derived, not read: collision_pipeline walks the pipeline from
    the matrix and every table's verdicts must be exactly what it dispatches, in both
    directions -- a collision slot the pipeline never dispatches and an object-model
    slot it does are both errors. Every constructor and destructor must store a
    shape table; every helper must be called, only by ruled rows of its phase, from
    exactly the sites it records, and be neither inside a translation-unit span, nor
    an export the plan pins, nor a row that stores a table or has its address taken.
    Then the census is checked against the ruling: a ruled row -- and every
    continuation of one -- must carry the ruled phase and `slot_ruling`, and no other
    row may claim it.

    `imports` is the set of import-table slot rvas and `spans` the translation-unit
    spans (translation_unit_spans_of); without spans the span exclusion is not read.
    """
    errors = [f"shape ruling is missing {key!r}" for key in RULING_KEYS if key not in ruling]
    if errors:
        return errors
    if ruling["schema_version"] != SCHEMA_VERSION:
        errors.append(f"shape ruling schema_version must be {SCHEMA_VERSION}")
    if ruling["oracle_sha256"] != inventory["pins"]["oracle"]["sha256"]:
        errors.append("shape ruling names an oracle the inventory does not pin")
    if ruling["phase_of"] != {"collision": 3, "object_model": 5}:
        errors.append(f"shape ruling phase_of {ruling['phase_of']!r} is not the user's decision: "
                      f"collision slots stay in Phase 3, object-model slots are Phase 5's")
    if ruling["pipeline"].get("phases") != list(PIPELINE_PHASES):
        errors.append(f"shape ruling pipeline phases {ruling['pipeline'].get('phases')!r} are not "
                      f"the decision: the collision pipeline traverses the rows Phases 3 and 4 own")
    functions = inventory["functions"]
    by_rva = {int(row["rva"], 16): row for row in functions}
    by_id = {row["id"]: row for row in functions}
    ranges = sorted((int(row["rva"], 16), int(row["rva"], 16) + row["size"], row["id"])
                    for row in functions)
    starts = [start for start, _, _ in ranges]

    def owner(address):
        index = bisect_right(starts, address) - 1
        if index >= 0 and ranges[index][0] <= address < ranges[index][1]:
            return ranges[index][2]
        return None

    def instruction(address, where):
        found = instructions.get(address)
        if found is None:
            errors.append(f"{where} cites 0x{address:08x}, where the capstone corpus decodes no "
                          f"instruction")
        return found

    def named_row(entry, where):
        row = by_id.get(entry.get("id"))
        if row is None:
            errors.append(f"{where} names {entry.get('id')!r}, which is not a censused function")
        elif row["rva"] != entry.get("rva"):
            errors.append(f"{where} records rva {entry.get('rva')!r} for {row['id']!r} but the "
                          f"inventory has {row['rva']!r}")
        return row

    tables = {}
    for table in ruling["tables"]:
        where = f"shape ruling table {table.get('class')!r}"
        if not all(verdict in SLOT_RULINGS for verdict in table.get("slots", [])):
            errors.append(f"{where} rules a slot outside {list(SLOT_RULINGS)}")
            continue
        tables[table["class"]] = table
        store = instruction(int(table["vptr_store"], 16), where)
        value = f", 0x{image_base + int(table['rva'], 16):08x}"
        if store is not None and not (store["mnemonic"] == "mov"
                                      and store["operands"].endswith(value)):
            errors.append(f"{where} says {table['vptr_store']} stores its table, and the "
                          f"instruction there is {store['mnemonic']} {store['operands']}")
        elif store is not None and owner(int(table["vptr_store"], 16)) != table.get("constructor"):
            errors.append(f"{where} says {table.get('constructor')!r} stores it, but "
                          f"{table['vptr_store']} is inside {owner(int(table['vptr_store'], 16))!r}")

    phases, slots, resolve_errors = resolve_shape_ruling(ruling, pointers)
    errors += resolve_errors
    for target, named in sorted(slots.items()):
        if target not in by_rva:
            errors.append(f"shape ruling slot {named[0][0]} {named[0][1]} holds 0x{target:08x}, "
                          f"which is not a censused function entry")

    # The verdicts are derived from the image, not read: every slot the pipeline
    # dispatches is a collision slot and no other slot is.
    closure, dispatches, dispatched, interfaces, pipeline_errors = collision_pipeline(
        ruling, functions, instructions, tail_calls, pointers, imports, image_base)
    errors += pipeline_errors + slot_verdict_errors(ruling, dispatched)

    def function_of(address):
        # The function a site belongs to: the entry row of the row that holds it.
        row = by_id.get(owner(address))
        return by_rva[_entry_rva(row)]["id"] if row and _entry_rva(row) in by_rva else None

    for example in ruling["pipeline"].get("worked_examples", []):
        cls, slot, verdict = example.get("class"), example.get("slot"), example.get("verdict")
        where = f"shape ruling worked example for {cls} slot {slot}"
        if (cls not in tables or not isinstance(slot, int) or slot >= len(tables[cls]["slots"])
                or tables[cls]["slots"][slot] != verdict):
            errors.append(f"{where} concludes {verdict!r}, which is not the table's verdict")
        for hop in example.get("path", []):
            found = instruction(int(hop["site"], 16), where)
            row = by_id.get(hop.get("row"))
            if found is not None and function_of(int(hop["site"], 16)) != hop.get("row"):
                errors.append(f"{where} says {hop['site']} is in {hop.get('row')!r}, but it is in "
                              f"{function_of(int(hop['site'], 16))!r}")
            elif row is not None and (_entry_rva(row) in closure) != hop.get("in_pipeline"):
                errors.append(f"{where} says {hop['row']!r} is "
                              f"{'' if hop.get('in_pipeline') else 'not '}in the collision "
                              f"pipeline, and it is{'' if _entry_rva(row) in closure else ' not'}")
    for entry in ruling["receivers"]:
        where = f"shape ruling receiver {entry.get('site') or entry.get('returned_by')!r}"
        if "site" in entry and function_of(int(entry["site"], 16)) != entry.get("row"):
            errors.append(f"{where} says it is in {entry.get('row')!r}, but it is in "
                          f"{function_of(int(entry['site'], 16))!r}")
        if not isinstance(entry.get("note"), str) or not entry["note"].strip():
            errors.append(f"{where} records no evidence for the receiver it types")

    direct = {}
    for found in instructions.values():
        target = found.get("target_rva")
        if target and not found["indirect"] and found["flow"] in ("call", "jump"):
            direct.setdefault(int(target, 16) if isinstance(target, str) else target,
                              set()).add(int(found["rva"], 16))
    for tail in tail_calls:
        source, target = (int(value, 16) if isinstance(value, str) else value
                          for value in (tail["from_rva"], tail["to_rva"]))
        direct.setdefault(target, set()).add(source)
    taken = collections.defaultdict(list)
    for slot, target in sorted(pointers.items()):
        taken[target].append(slot)
    import reconcile_analysis  # the generator owns the plan's export pins
    pinned_exports = {entry.get("function_id") for entry in inventory.get("exports", [])
                      if entry.get("name") in reconcile_analysis.EXPORT_PHASES}
    table_values = {f", 0x{image_base + int(table['rva'], 16):08x}" for table in tables.values()}

    seen = set()
    for member in ruling["members"]:
        where = f"shape ruling member {member.get('id')!r}"
        row = named_row(member, where)
        if row is None:
            continue
        if row["id"] in seen:
            errors.append(f"{where} is named twice")
        seen.add(row["id"])
        basis, verdict = member.get("basis"), member.get("ruling")
        if basis not in MEMBER_BASES or verdict not in ("collision", "object_model"):
            errors.append(f"{where} gives basis {basis!r} and ruling {verdict!r}")
            continue
        if basis in ("constructor", "destructor"):
            store = instruction(int(member["site"], 16), where)
            stored = {f", 0x{image_base + int(table['rva'], 16):08x}" for table in tables.values()}
            if store is not None and not (store["mnemonic"] == "mov"
                                          and any(store["operands"].endswith(value) for value in stored)):
                errors.append(f"{where} is a {basis} because {member['site']} stores a shape table, "
                              f"and the instruction there is {store['mnemonic']} {store['operands']}")
            elif store is not None and owner(int(member["site"], 16)) != row["id"]:
                errors.append(f"{where} cites {member['site']}, which is in "
                              f"{owner(int(member['site'], 16))!r}")
            if verdict != "object_model":
                errors.append(f"{where} is a {basis}, and constructing or destroying a shape is "
                              f"object model")
        elif basis == "adapter":
            install = instruction(int(member["site"], 16), where)
            if install is not None and not (install["mnemonic"] == "mov" and install["operands"]
                                            .endswith(f", 0x{image_base + int(row['rva'], 16):08x}")):
                errors.append(f"{where} says {member['site']} installs it, and the instruction there "
                              f"is {install['mnemonic']} {install['operands']}")
            call = instruction(int(member["call_site"], 16), where)
            match = VIRTUAL_CALL.match(call["operands"]) if call is not None else None
            if call is not None and not (match and call["indirect"]
                                         and int(match.group(1) or "0", 0) == 4 * member["slot"]
                                         and owner(int(member["call_site"], 16)) == row["id"]):
                errors.append(f"{where} says its body calls slot {member['slot']} at "
                              f"{member['call_site']}, and it does not")
            # The adapter thunks a slot of the interface whose constructor installs it.
            installer = next((cls for cls, table in tables.items()
                              if table.get("constructor") == owner(int(member["site"], 16))), None)
            thunked = {table["slots"][member["slot"]] for cls, table in tables.items()
                       if installer is not None and interfaces.get(cls) == interfaces.get(installer)
                       and member["slot"] < len(table["slots"])
                       and table["slots"][member["slot"]] != "purecall"}
            if thunked != {verdict}:
                errors.append(f"{where} thunks slot {member['slot']}, which the tables rule "
                              f"{sorted(thunked)}, but the member is ruled {verdict}")
        else:
            rva = int(row["rva"], 16)
            listed = {int(caller["site"], 16): caller.get("row")
                      for caller in member.get("callers", [])}
            actual = direct.get(rva, set())
            if not listed:
                errors.append(f"{where} is a helper that records no caller; a helper is a row "
                              f"only ruled rows call, so it names them")
            elif set(listed) != actual:
                errors.append(f"{where} is a helper whose recorded call sites "
                              f"{sorted(f'0x{a:08x}' for a in listed)} are not its direct callers "
                              f"{sorted(f'0x{a:08x}' for a in actual)}")
            for address, caller in sorted(listed.items()):
                if owner(address) != caller:
                    errors.append(f"{where} says 0x{address:08x} is in {caller!r}, but it is in "
                                  f"{owner(address)!r}")
                elif caller not in by_id or _entry_rva(by_id[caller]) not in phases:
                    errors.append(f"{where} is called from {caller!r}, which the ruling does not "
                                  f"place; a helper is a row only ruled rows call")
                elif phases[_entry_rva(by_id[caller])] != ruling["phase_of"].get(verdict):
                    errors.append(f"{where} is ruled {verdict}, but its caller {caller!r} is ruled to "
                                  f"phase {phases[_entry_rva(by_id[caller])]}")
            # What a helper is not: a row whose address is taken (a pointer or an
            # immediate names it, and the relocation table records both), an export the
            # plan pins, a row inside a translation-unit span, or a row that stores a
            # shape table of its own, which makes it a constructor or destructor.
            if taken.get(rva):
                errors.append(f"{where} has its address taken at "
                              f"{', '.join(f'0x{slot:08x}' for slot in taken[rva][:4])}; a helper "
                              f"is only ever called")
            if row["id"] in pinned_exports:
                errors.append(f"{where} is an export the plan pins, and a pin is not the ruling's "
                              f"to move")
            unit = next((name for low, high, name in spans or () if low <= rva <= high), None)
            if unit is not None:
                errors.append(f"{where} is inside {unit}'s span, which places it; a helper has no "
                              f"translation unit of its own")
            for address in range(rva, rva + row["size"]):
                found = instructions.get(address)
                if found and found["mnemonic"] == "mov" and any(
                        found["operands"].endswith(value) for value in table_values):
                    errors.append(f"{where} stores a shape table at 0x{address:08x}, so it is a "
                                  f"constructor or destructor, not a helper")
    for entry in ruling["exceptions"]:
        where = f"shape ruling exception {entry.get('id')!r}"
        row = named_row(entry, where)
        if row is None:
            continue
        if row["id"] in seen:
            errors.append(f"{where} is also a member; a row is ruled once")
        seen.add(row["id"])
        disposition = entry.get("disposition")
        if disposition not in RULING_DISPOSITIONS:
            errors.append(f"{where} has disposition {disposition!r}")
        elif disposition == "shared" and entry.get("phase") != 2:
            errors.append(f"{where} is shared runtime, which is Phase 2")
        elif disposition in ("collision", "object_model"):
            # A slot target ruled against its slot rests on who calls it directly: every
            # direct caller must be a ruled row of the phase it is given.
            rva = int(row["rva"], 16)
            callers = sorted(direct.get(rva, ()))
            placed = {phases.get(_entry_rva(by_id[owner(site)])) if owner(site) in by_id else None
                      for site in callers}
            if rva not in slots:
                errors.append(f"{where} is no slot's target; rule it as a member")
            elif not callers or placed != {ruling["phase_of"].get(disposition)}:
                errors.append(f"{where} is ruled {disposition}, and its direct callers "
                              f"{', '.join(f'0x{site:08x} in {owner(site)!r}' for site in callers) or 'none'} "
                              f"are not all ruled rows of that phase")
    for target, named in sorted(slots.items()):
        if any(verdict == "purecall" for _, _, verdict in named) and phases.get(target, 0) is not None:
            errors.append(f"row at 0x{target:08x} fills a pure-call slot, so it is the runtime's and "
                          f"must be an external exception")
        if (any(verdict == "purecall" for _, _, verdict in named)
                and by_rva.get(target, {}).get("kind") != "compiler_artifact"):
            errors.append(f"row at 0x{target:08x} fills a pure-call slot and is not a compiler "
                          f"artifact; a pure-call slot holds the runtime's filler")

    ruled = {rva: phase for rva, phase in phases.items() if phase is not None}
    for row in functions:
        entry = _entry_rva(row)
        where = f"function {row['id']!r}"
        if entry in ruled:
            if row["phase"] != ruled[entry] or row["phase_provenance"] != SLOT_RULING:
                errors.append(f"{where} is placed by the shape ruling at phase {ruled[entry]} with "
                              f"provenance {SLOT_RULING!r}, but the census records phase "
                              f"{row['phase']} {row['phase_provenance']!r}")
        elif row["phase_provenance"] == SLOT_RULING:
            errors.append(f"{where} claims {SLOT_RULING!r}, but the shape ruling does not place it")
    return errors


# The census rows the generator cannot derive, each pinned to the phase and provenance
# the census carries, with the reason and the commit that decided it. reconcile_analysis.py
# applies them last, to the row alone, and records what it would have emitted, so the
# validator can hold the list exact: a pin the generator now agrees with is struck.
PHASE_PINS_NAME = "phase_pins.json"
PHASE_PIN_KEYS = ("id", "rva", "phase", "phase_provenance", "reason", "decided_by")
COMMIT_PREFIX = re.compile(r"^[0-9a-f]{7,40}(?![0-9a-z])")


def read_phase_pins(document):
    """The pins in phase_pins.json, by stable ID, and whatever is wrong with them."""
    if not isinstance(document, dict) or not isinstance(document.get("pins"), list):
        return {}, [f"{PHASE_PINS_NAME} must hold a `pins` list"]
    pins, errors = {}, []
    for index, entry in enumerate(document["pins"]):
        where = f"{PHASE_PINS_NAME} entry {index}"
        found = _check_keys(where, entry, PHASE_PIN_KEYS)
        errors += found
        if found:
            continue
        identifier = entry["id"]
        if not isinstance(identifier, str) or not FUNCTION_ID_PATTERN.match(identifier):
            errors.append(f"{where} pins {identifier!r}, which is not a function stable ID")
            continue
        if identifier in pins:
            errors.append(f"{where} pins {identifier} a second time")
        if entry["phase_provenance"] not in PHASE_PROVENANCE or not isinstance(entry["phase"], int):
            errors.append(f"{where} pins {identifier} to phase {entry['phase']!r} "
                          f"{entry['phase_provenance']!r}, which the census cannot record")
        if not isinstance(entry["reason"], str) or not entry["reason"].strip():
            errors.append(f"{where} pins {identifier} with no reason")
        if not isinstance(entry["decided_by"], str) or not COMMIT_PREFIX.match(entry["decided_by"]):
            errors.append(f"{where} pins {identifier} without naming the commit that decided it")
        pins[identifier] = entry
    return pins, errors


def validate_regeneration(inventory, regenerated, pins, unpinned):
    """The committed phase column is the generator's output, row for row.

    Every function row and data object must carry the phase and provenance
    reconcile_analysis.py gives it from the oracle, the shape ruling, the third-party
    correspondence maps and the pins. A pin must hold what the census carries and
    must differ from what the generator would emit without it (`unpinned`), or it is
    struck: the list is exact both ways, like every other exception list here.
    """
    errors = []
    for kind in ("functions", "data_objects"):
        mine = {row["id"]: row for row in inventory[kind]}
        theirs = {row["id"]: row for row in regenerated[kind]}
        if set(mine) != set(theirs):
            errors.append(f"the generator emits {len(theirs)} {kind} and the census holds "
                          f"{len(mine)}; the census is not the generator's output")
            continue
        for identifier in sorted(mine):
            ours, generated = mine[identifier], theirs[identifier]
            if ours["rva"] != generated["rva"]:
                errors.append(f"{identifier} is at {ours['rva']} in the census and at "
                              f"{generated['rva']} in the generator's output")
            elif (ours["phase"], ours["phase_provenance"]) != (generated["phase"],
                                                                generated["phase_provenance"]):
                errors.append(f"{identifier} is phase {ours['phase']} {ours['phase_provenance']!r} in "
                              f"the census, and the generator places it at phase "
                              f"{generated['phase']} {generated['phase_provenance']!r}"
                              + (", which is its pin" if identifier in pins else
                                 f"; take the generator's value or pin it in {PHASE_PINS_NAME}"))
    rows = {row["id"]: row for row in inventory["functions"]}
    for identifier, pin in sorted(pins.items()):
        if identifier not in rows or rows[identifier]["rva"] != pin["rva"]:
            errors.append(f"{PHASE_PINS_NAME} pins {identifier} at {pin['rva']}, which is not a "
                          f"census row there")
        elif unpinned.get(identifier) == (pin["phase"], pin["phase_provenance"]):
            errors.append(f"{PHASE_PINS_NAME} pins {identifier} to what the generator already "
                          f"gives it; strike the pin")
    return errors


# Rules a span says nothing about: an artifact, the runtime tail, a plan's export pin,
# padding, and the shape ruling, which is the one rule that outranks a span on purpose.
SPAN_INDEPENDENT_PROVENANCE = ("runtime_artifact", "runtime_tail", "export_pin", "padding",
                               SLOT_RULING)


def translation_unit_spans_of(functions, strings, references):
    """The __FILE__ spans the generator reads, recomputed from the census's own rows.

    reconcile_analysis.py seeds a translation unit from every function that
    references an NX_ASSERT __FILE__ string and takes each unit's span from its
    seeds; this does the same over the committed rows, so a span is evidence the
    validator reads from the oracle and not from the census it is checking.
    """
    import reconcile_analysis  # the generator owns SOURCE_PHASES and the span rule
    ranges = sorted((int(row["rva"], 16), int(row["rva"], 16) + row["size"], _entry_rva(row))
                    for row in functions)
    starts = [start for start, _, _ in ranges]
    files = {}
    for entry in strings:
        index = entry["value"].find(reconcile_analysis.SOURCE_MARK)
        if index >= 0:
            files[int(entry["rva"], 16)] = entry["value"][index + len(reconcile_analysis.SOURCE_MARK):]
    seeds = {}
    for reference in references:
        if not reference["to_rva"]:
            continue
        name = files.get(int(reference["to_rva"], 16))
        if name is None or name not in reconcile_analysis.SOURCE_PHASES:
            continue
        address = int(reference["from_rva"], 16)
        index = bisect_right(starts, address) - 1
        if index >= 0 and ranges[index][0] <= address < ranges[index][1]:
            seeds[ranges[index][2]] = name
    return reconcile_analysis.translation_unit_spans(seeds), reconcile_analysis.SOURCE_PHASES


def validate_translation_unit_spans(functions, spans, source_phases, without_span=frozenset()):
    """A row's translation-unit claim must be the span its entry sits in.

    `translation_unit` says the entry is inside a named unit's span and takes that
    unit's phase; `enclosed_by_one_phase` says it sits between two spans one phase
    owns. Both were free text: rows claimed a unit no span encloses, and a row
    inside ContactConvexHeightfield.cpp could be moved to Phase 5 with nothing
    objecting. And an entry inside a span may be placed by nothing weaker than the
    span, because a unit that names an entry outranks every propagation rule.

    `without_span` names the rows phase_pins.json pins to `translation_unit`: 52 Phase
    4 rows the third-party correction gave that provenance with no span around them.
    Their phase is held by the pin, so only their label is excused here, and a pinned
    row that no longer lacks a span has its pin struck.
    """
    lows = [low for low, _, _ in spans]

    def enclosing(address):
        index = bisect_right(lows, address) - 1
        if index >= 0 and spans[index][0] <= address <= spans[index][1]:
            return spans[index][2]
        return None

    def neighbours(address):
        index = bisect_right(lows, address) - 1
        return (spans[index] if index >= 0 else None,
                spans[index + 1] if index + 1 < len(spans) else None)

    errors, gaps = [], set()
    for row in functions:
        entry, why, where = _entry_rva(row), row["phase_provenance"], f"function {row['id']!r}"
        unit = enclosing(entry)
        if why == "translation_unit":
            if row.get("third_party"):
                continue
            if unit is None or source_phases[unit] != row["phase"]:
                if row["id"] in without_span:
                    gaps.add(row["id"])
                    continue
                errors.append(f"{where} claims translation_unit at phase {row['phase']}, but its "
                              f"entry 0x{entry:08x} is "
                              + (f"inside {unit}, a phase {source_phases[unit]} unit" if unit else
                                 "inside no __FILE__ span"))
        elif why == "enclosed_by_one_phase":
            below, above = neighbours(entry)
            if unit is not None or not (below and above and source_phases[below[2]]
                                        == source_phases[above[2]] == row["phase"]):
                errors.append(f"{where} claims enclosed_by_one_phase at phase {row['phase']}, but "
                              f"its entry 0x{entry:08x} is "
                              + (f"inside {unit}" if unit else
                                 f"between {below and below[2]} and {above and above[2]}"))
        elif unit is not None and why not in SPAN_INDEPENDENT_PROVENANCE:
            errors.append(f"{where} sits inside {unit}, a phase {source_phases[unit]} unit, but is "
                          f"placed by {why!r} at phase {row['phase']}")
    for identifier in sorted(set(without_span) - gaps):
        errors.append(f"{identifier!r} is pinned as claiming translation_unit without a span, "
                      f"and no longer does; strike its pin from {PHASE_PINS_NAME}")
    return errors


def read_differential_exclusions(path):
    """The line prefixes run_differential.ps1 drops before it compares.

    A phase record naming what its differential left out of the comparison is a
    claim about that script, so it is read from the script rather than believed.
    """
    found = DIFFERENTIAL_EXCLUSION.findall(path.read_text(encoding="utf-8"))
    return found[0].split("|") if len(found) == 1 else None


def read_plan_escalations(path):
    """The (escalation, phase record) pairs a phase plan takes responsibility for."""
    return {(name, int(phase))
            for name, phase in PLAN_ESCALATION.findall(path.read_text(encoding="utf-8"))}


def validate_escalations(records, plans):
    """Each escalation is named by every plan that inherits it, and the reverse.

    A gate record is the only place an escalation lives, so nothing pins the set:
    deleting one deletes the programme's knowledge of it. The inheriting phase's
    plan is the other end of the obligation, so the two are pinned to each other.
    """
    errors = []
    claimed = set()
    for phase, record in sorted(records.items()):
        for escalation in record.get("escalations", []):
            if not isinstance(escalation, dict) or not isinstance(escalation.get("id"), str):
                continue
            for inheritor in escalation.get("inherited_by") or []:
                if not isinstance(inheritor, int) or isinstance(inheritor, bool):
                    continue
                claimed.add((escalation["id"], phase, inheritor))
                if inheritor not in plans:
                    errors.append(f"phase {phase} escalation {escalation['id']!r} is inherited by "
                                  f"phase {inheritor}, which has no plan to record it in")
                elif (escalation["id"], phase) not in plans[inheritor]:
                    errors.append(f"phase {phase} escalation {escalation['id']!r} is inherited by "
                                  f"phase {inheritor}, whose plan does not name it")
    for inheritor, named in sorted(plans.items()):
        for name, phase in sorted(named):
            if (name, phase, inheritor) not in claimed:
                errors.append(f"the phase {inheritor} plan takes on escalation {name!r} from "
                              f"gates/phase{phase}.json, which does not record it")
    return errors


def _ledger_totals(closure):
    """Closed and deferred totals recomputed from a ledger's own entries."""
    counts = collections.Counter()
    for row in closure.get("closed", []):
        if isinstance(row, dict):
            counts["closed_data" if DATA_ID_PATTERN.match(str(row.get("id")))
                   else "closed_functions"] += 1
    for row in closure.get("deferred", []):
        if isinstance(row, dict):
            counts["deferred_data" if DATA_ID_PATTERN.match(str(row.get("id")))
                   else "deferred_functions"] += 1
    return counts


def validate_phase_record(record, phase, program_phase, closure_totals, exclusions=None):
    """A phase gate record may not disagree with the ledger or the runner.

    It exists to say what was run and what came out. The counts are where it
    restates the closure ledger and the exclusion list is where it restates the
    differential runner; both are recomputed from the source rather than read,
    because everything else in it is prose only this file carries.
    """
    errors = _check_keys(f"phase {phase} record", record, PHASE_RECORD_KEYS)
    if errors:
        return errors
    if record["schema_version"] != SCHEMA_VERSION:
        errors.append(f"the phase {phase} record records schema version "
                      f"{record['schema_version']!r} but this tool reads {SCHEMA_VERSION}")
    if record["phase"] != phase:
        errors.append(f"the phase record says phase {record['phase']!r} but its file name says "
                      f"phase {phase}")
    if record["status"] not in PROGRAM_STATUSES:
        errors.append(f"the phase {phase} record status {record['status']!r} is not one of "
                      f"{list(PROGRAM_STATUSES)}")
    # The record and program.json describe one close. Every field they both carry
    # is cross-checked, or the two drift and each vouches for the other.
    for field in ("status", "name", "implementation_commit", "evidence_commit"):
        stated = (program_phase or {}).get(field)
        if program_phase is not None and record[field] != stated:
            errors.append(f"the phase {phase} record says {field}={record[field]!r} but "
                          f"program.json says {stated!r}")
    if closure_totals is None:
        errors.append(f"the phase {phase} record has no phase{phase}-closure.json beside it, so "
                      f"nothing accounts for the rows it reports on")
    else:
        if record["closure_ledger"] != f"gates/phase{phase}-closure.json":
            errors.append(f"the phase {phase} record names closure ledger "
                          f"{record['closure_ledger']!r} rather than its own phase's")
        errors += _check_keys(f"phase {phase} record counts", record["counts"],
                              PHASE_RECORD_COUNT_KEYS)
        if not errors:
            recomputed = {
                "closed": closure_totals["closed_functions"] + closure_totals["closed_data"],
                "deferred": (closure_totals["deferred_functions"]
                             + closure_totals["deferred_data"]),
            }
            if record["counts"] != recomputed:
                errors.append(f"the phase {phase} record counts {record['counts']} do not match "
                              f"its closure ledger {recomputed}")
    errors += _check_rows(f"phase {phase} record gate_sequence", record["gate_sequence"],
                          GATE_SEQUENCE_KEYS)
    # A record whose gate side is blank is a file with the right two numbers in
    # it, which is not a gate. This half is prose, so what is checkable is that
    # it is there.
    if not record["gate_sequence"]:
        errors.append(f"the phase {phase} record runs no command; a gate that reports nothing "
                      f"cannot have measured anything")
    for index, step in enumerate(record["gate_sequence"]):
        if not isinstance(step, dict):
            continue
        for field in GATE_SEQUENCE_KEYS:
            if not isinstance(step.get(field), str) or not step[field].strip():
                errors.append(f"phase {phase} record gate_sequence[{index}] records no {field}")
    if exclusions is None:
        errors.append(f"the phase {phase} record names what its differential excluded, but the "
                      f"exclusion list could not be read back out of run_differential.ps1")
    elif record["excluded_from_comparison"] != exclusions:
        errors.append(f"the phase {phase} record says the differential excluded "
                      f"{record['excluded_from_comparison']!r} but run_differential.ps1 excludes "
                      f"{exclusions!r}")
    errors += _check_rows(f"phase {phase} record escalations", record["escalations"],
                          ESCALATION_KEYS, ESCALATION_OPTIONAL_KEYS)
    for index, escalation in enumerate(record["escalations"]):
        if not isinstance(escalation, dict):
            continue
        where = f"phase {phase} record escalation {escalation.get('id')!r}"
        # An escalation whose text is a conclusion is one the inheriting phase
        # cannot re-run; the reproduction is the part that carries over.
        for field in ("reproduction", "evidence"):
            if not isinstance(escalation.get(field), str) or not escalation[field].strip():
                errors.append(f"{where} records no {field}")
        rows = escalation.get("rows", [])
        if not isinstance(rows, list) or not all(
                isinstance(value, str) and FUNCTION_ID_PATTERN.match(value) for value in rows):
            errors.append(f"{where} carries rows {rows!r}, which are not function stable IDs")
        phases = escalation.get("inherited_by")
        if not isinstance(phases, list) or not phases or any(
                isinstance(value, bool) or not isinstance(value, int) or not 1 <= value <= 8
                for value in phases):
            errors.append(f"{where} is inherited by {phases!r}, which are not phase numbers")
        elif any(value <= phase for value in phases):
            errors.append(f"{where} is inherited by phase {min(phases)}, which is not later "
                          f"than {phase}")
    return errors


def plan_path(evidence_root, plan):
    """A phase plan is recorded from the repository root, three levels up.

    The root is resolved first. Run as `validate_inventory.py inventory.json`
    from the evidence directory itself, the root was `.` and `parents[2]` had
    nothing to index, so the tool died on an IndexError traceback -- which reads
    exactly like a validation failure and is not one. The production runner
    always passes an absolute path, which is why it was never seen.
    """
    root = Path(evidence_root).resolve()
    if len(root.parents) < 3:
        return None
    return root.parents[2] / plan


# A `source` naming a path is a claim that the reconstruction lives there. Nothing
# resolved those paths, so a row could name a file that has never existed and the
# census still passed -- a check that cannot fail, which is the shape of gate this
# programme has been caught building before.
#
# The paths below do not exist. They are listed rather than silently skipped so
# the gap is visible to every run. Each is a production file a reader would expect
# the reconstruction to live in; the rows naming them are proven by their
# differentials but their candidate is a shared helper in ObjectModel.cpp, so the
# path records where the row belongs rather than where its implementation is.
#
# Removing an entry from this list is the act of creating the file or repointing
# the row. Adding one is a regression that has to be argued.
UNRESOLVED_SOURCE_PATHS = (
    'Physics/src/Actor.cpp',                         # 3 rows
    'Physics/src/CapsuleShape.cpp',                  # 1 rows
    'Physics/src/ContactConvexHeightfield.cpp',      # 2 rows
    'Physics/src/ContactMeshMesh.cpp',               # 1 rows
    'Physics/src/ContactPlaneMesh.cpp',              # 2 rows
    'Physics/src/Controller.cpp',                    # 2 rows
    'Physics/src/ConvexHull.cpp',                    # 1 rows
    'Physics/src/D6Joint.cpp',                       # 3 rows
    'Physics/src/EdgeList.cpp',                      # 3 rows
    'Physics/src/IceAdjacencies.cpp',                # 2 rows
    'Physics/src/InternalTriangleMesh.cpp',          # 1 rows
    'Physics/src/Joint.cpp',                         # 6 rows
    # 'Physics/src/NpActor.cpp' was on this list with 66 rows against it, and is
    # REMOVED: the concrete actor class now exists, so the path resolves and the entry
    # would be a claim that a real file is missing. The check said so itself --
    # "is on the allowlist but no longer unresolved; remove the entry".
    'Physics/src/NpBoxShape.cpp',                    # 12 rows
    'Physics/src/NpCapsuleShape.cpp',                # 14 rows
    'Physics/src/NpPlaneShape.cpp',                  # 12 rows
    # 'Physics/src/NpScene.cpp' was here with 37 rows against it, and was REMOVED
    # when the file was created for the Scene reconstruction. The check said so
    # itself: "is on the allowlist but no longer unresolved; remove the entry".
    'Physics/src/NpSphereShape.cpp',                 # 12 rows
    'Physics/src/NpSpringAndDamperEffector.cpp',     # 3 rows
    'Physics/src/NpTriangleMesh.cpp',                # 6 rows
    'Physics/src/NpTriangleMeshShape.cpp',           # 11 rows
    # 'Physics/src/Scene.cpp' was here with 14 rows against it. It is REMOVED
    # rather than kept: the file now exists, so the row-level `implementation`
    # field resolves and the allowlist entry would be a claim that a real file is
    # missing. The validator says so itself -- "is on the allowlist but no longer
    # unresolved; remove the entry" -- which is the check working.
    'Physics/src/SceneRaycast.cpp',                  # 6 rows
    'Physics/src/Shape.cpp',                         # 6 rows
    'Physics/src/core/CylindricalJoint.cpp',         # 2 rows
    'Physics/src/core/DistanceJoint.cpp',            # 2 rows
    'Physics/src/core/FixedJoint.cpp',               # 2 rows
    'Physics/src/core/NpCylindricalJoint.cpp',       # 10 rows
    'Physics/src/core/NpD6Joint.cpp',                # 14 rows
    'Physics/src/core/NpDistanceJoint.cpp',          # 10 rows
    'Physics/src/core/NpFixedJoint.cpp',             # 10 rows
    'Physics/src/core/NpPointInPlaneJoint.cpp',      # 10 rows
    'Physics/src/core/NpPointOnLineJoint.cpp',       # 10 rows
    'Physics/src/core/NpPrismaticJoint.cpp',         # 10 rows
    'Physics/src/core/NpPulleyJoint.cpp',            # 10 rows
    'Physics/src/core/NpRevoluteJoint.cpp',          # 14 rows
    'Physics/src/core/NpSphericalJoint.cpp',         # 12 rows
    'Physics/src/core/PointInPlaneJoint.cpp',        # 2 rows
    'Physics/src/core/PointOnLineJoint.cpp',         # 2 rows
    'Physics/src/core/PrismaticJoint.cpp',           # 2 rows
    'Physics/src/core/PulleyJoint.cpp',              # 2 rows
    'Physics/src/core/RevoluteJoint.cpp',            # 7 rows
    'Physics/src/core/SphericalJoint.cpp',           # 4 rows
    'Physics/src/fluids/Fluid.cpp',                  # 3 rows
    'Physics/src/fluids/FluidManager.cpp',           # 7 rows
    'Physics/src/fluids/ImplicitMesh.cpp',           # 1 rows
    'Physics/src/fluids/NpFluid.cpp',                # 32 rows
    'Physics/src/fluids/NpFluidEmitter.cpp',         # 14 rows
    'Physics/src/fluids/NpImplicitMesh.cpp',         # 9 rows
    'Physics/src/opcode/OPC_MeshInterface.cpp',      # 1 rows
    'Physics/src/opcode/OPC_Model.cpp',              # 1 rows
)


def _check_implementation_paths(functions, evidence_root):
    """Every `implementation` resolves, or the row has none.

    This is the check `source` cannot carry. A row that names an implementation
    file is claiming the reconstruction lives there, and that claim is either true
    or it is not.
    """
    repo = Path(__file__).resolve().parents[4]
    if not (repo / 'Physics' / 'src').is_dir():
        return []
    errors = []
    for row in functions:
        impl = row.get('implementation')
        if impl is None:
            continue
        if not isinstance(impl, str) or not impl:
            errors.append(f"function {row['id']!r} implementation is not a path")
            continue
        if not (repo / impl).is_file():
            errors.append(
                f"function {row['id']!r} implementation {impl!r} does not exist")
    return errors


def _check_source_paths(functions, evidence_root):
    """Every path-shaped `source` either resolves or is on the allowlist.

    A path that resolves is evidence. A path on the allowlist is a recorded gap.
    A path that does neither is a new claim nothing backs, and fails.

    The repository root is derived from THIS FILE's location, not from the
    inventory it was handed. A test builds a synthetic inventory in a temporary
    directory, and resolving against that directory would report every real path
    as unresolved -- which is what an earlier version of this check did.
    """
    repo = Path(__file__).resolve().parents[4]
    if not (repo / 'Physics' / 'src').is_dir():
        return []
    errors = []
    unresolved = sorted({row['source'] for row in functions
                         if row.get('source') and row['source'].endswith(('.cpp', '.h'))
                         and not (repo / row['source']).exists()})
    for path in unresolved:
        if path not in UNRESOLVED_SOURCE_PATHS:
            errors.append(
                f"function source {path!r} does not exist and is not a recorded "
                f"unresolved path; create the file or record the gap")
    stale = sorted(set(UNRESOLVED_SOURCE_PATHS) - set(unresolved))
    for path in stale:
        errors.append(
            f"unresolved source path {path!r} is on the allowlist but no longer "
            f"unresolved; remove the entry")
    return errors


def validate_program(inventory, program, closures, evidence_root):
    """Recompute every count program.json states about a phase from the census.

    program.json is written by hand at each close and read by every later phase.
    Nothing compared it to inventory.json, so the two could disagree about how
    much of the census is done and no gate would say so.
    """
    errors = _check_keys("program", program, PROGRAM_KEYS)
    if errors:
        return errors
    if program["schema_version"] != SCHEMA_VERSION:
        errors.append(f"program schema_version must be {SCHEMA_VERSION}")
    errors += _check_rows("program.phases", program["phases"], PROGRAM_PHASE_KEYS)
    errors += _check_rows("program.global_gates", program["global_gates"], PROGRAM_GATE_KEYS)
    # PROGRAM_STATUSES was defined and never applied here, so a global gate could
    # hold any string and the vocabulary nothing checked was decorative. The phase
    # statuses beside these are checked against the same tuple.
    for index, gate in enumerate(program["global_gates"]):
        if isinstance(gate, dict) and gate.get("status") not in PROGRAM_STATUSES:
            errors.append(
                f"program.global_gates[{index}] status {gate.get('status')!r} is not "
                f"one of {list(PROGRAM_STATUSES)}")
    # The pins are the identity of the thing being reconstructed. Two documents
    # naming two oracles is the one disagreement that invalidates everything else.
    for key, pin, fields in (("oracle", "oracle", ORACLE_PIN_KEYS),
                             ("link_oracle", "link_oracle", LINK_ORACLE_PIN_KEYS),
                             ("header_root", "public_headers", HEADER_PIN_KEYS)):
        stated = program[key]
        if not isinstance(stated, dict):
            errors.append(f"program.{key} must be a JSON object")
            continue
        differing = {field: stated.get(field) for field in fields
                     if stated.get(field) != inventory["pins"][pin][field]}
        if differing:
            errors.append(f"program.{key} does not pin what inventory.pins.{pin} pins: "
                          f"{differing}")
    if errors:
        return errors

    declared = {row["phase"]: row for row in inventory["phases"]}
    stated = {row["phase"]: row for row in program["phases"]}
    if sorted(stated) != sorted(declared):
        errors.append(f"program.json declares phases {sorted(stated)} but inventory.json declares "
                      f"{sorted(declared)}")
        return errors

    owned = collections.Counter()
    for row in inventory["functions"]:
        owned[(row["phase"], "functions")] += 1
    for row in inventory["data_objects"]:
        owned[(row["phase"], "data_objects")] += 1

    for phase in sorted(stated):
        row, inventory_row = stated[phase], declared[phase]
        where = f"program.json phase {phase}"
        if row["name"] != inventory_row["name"]:
            errors.append(f"{where} is named {row['name']!r} but inventory.json names it "
                          f"{inventory_row['name']!r}")
        # A phase whose plan file is not there is a phase nobody can execute, and
        # the plan is where its escalations are inherited.
        plan = plan_path(evidence_root, row["plan"]) if isinstance(row["plan"], str) else None
        if plan is None or not plan.is_file():
            errors.append(f"{where} names plan {row['plan']!r}, which is not a file")
        if not isinstance(row["gate"], str) or not row["gate"].strip():
            errors.append(f"{where} states no gate")
        if row["status"] not in PROGRAM_STATUSES:
            errors.append(f"{where} status {row['status']!r} is not one of "
                          f"{list(PROGRAM_STATUSES)}")
            continue
        expected_status = PROGRAM_STATUS_FOR_INVENTORY.get(inventory_row["status"])
        if row["status"] != expected_status:
            errors.append(f"{where} says {row['status']!r} but inventory.json says the phase is "
                          f"{inventory_row['status']!r}, which is {expected_status!r} here")
        for key, table in (("owned_functions", "functions"),
                           ("owned_data_objects", "data_objects")):
            if row[key] != owned[(phase, table)]:
                errors.append(f"{where} claims {key}={row[key]!r} but the census assigns it "
                              f"{owned[(phase, table)]}")

        # No ledger means nothing has closed a row, so the counters are the
        # census alone. A phase that has not started at all leaves them null;
        # stating some but not others is the shape staleness came in as.
        totals = _ledger_totals(closures[phase]) if phase in closures else collections.Counter()
        recomputed = {
            "closed_functions": totals["closed_functions"],
            "closed_data_objects": totals["closed_data"],
            "remaining_functions": owned[(phase, "functions")] - totals["closed_functions"],
            "remaining_data_objects": owned[(phase, "data_objects")] - totals["closed_data"],
        }
        if phase in closures or any(row[key] is not None for key in PROGRAM_COUNTED):
            for key in PROGRAM_COUNTED:
                if row[key] != recomputed[key]:
                    errors.append(f"{where} claims {key}={row[key]!r} but the census and its "
                                  f"closure ledger give {recomputed[key]}")
        if phase not in closures and row["status"] == "pass" and (
                owned[(phase, "functions")] or owned[(phase, "data_objects")]):
            errors.append(f"{where} passes while owning {owned[(phase, 'functions')]} functions "
                          f"and {owned[(phase, 'data_objects')]} data objects that no closure "
                          f"ledger accounts for")

        artifact = row["gate_artifact"]
        if row["status"] == "pass" or artifact is not None:
            if not isinstance(artifact, str) or not artifact.strip():
                errors.append(f"{where} passes without naming a gate artifact")
            elif not (evidence_root / artifact).is_file():
                errors.append(f"{where} names gate artifact {artifact!r}, which is not a file")
            elif (evidence_root / "gates" / f"phase{phase}.json").is_file() \
                    and artifact != f"gates/phase{phase}.json":
                errors.append(f"{where} names gate artifact {artifact!r} while a "
                              f"gates/phase{phase}.json record sits unread beside it")
        if row["status"] == "pass":
            if not isinstance(row["evidence_commit"], str) or not row["evidence_commit"].strip():
                errors.append(f"{where} passes without recording evidence_commit")
            # An evidence-only phase legitimately has no implementation commit;
            # a phase that closed a code row cannot, because closing one means
            # a mutation was aimed at a reconstruction that has to exist.
            if row["closed_functions"] and (not isinstance(row["implementation_commit"], str)
                                            or not row["implementation_commit"].strip()):
                errors.append(f"{where} closes {row['closed_functions']} function rows without "
                              f"recording the implementation_commit they were measured against")
    return errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("inventory", help="inventory JSON to validate")
    args = parser.parse_args()

    path = Path(args.inventory)
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, ValueError) as error:
        parser.exit(2, f"error: {error}\n")

    errors = validate_inventory(data)
    if not errors:
        ledger = path.parent / data["pins"]["labels"]
        try:
            errors = validate_labels(data, json.loads(ledger.read_text(encoding="utf-8")))
        except (OSError, ValueError) as error:
            parser.exit(2, f"error: {error}\n")

        # The third-party column is a claim about upstream source, so it is
        # checked against the per-row correspondence maps rather than believed.
        correspondence, map_errors = read_source_correspondence(path.parent)
        errors += map_errors
        if not map_errors:
            errors += validate_third_party(data, correspondence)

    # Each phase that has published a closure ledger must account for every row
    # it owns: closed with a named proof, or deferred with a named reason.
    # Every ledger is checked; a broken one must not hide the next.
    closures = sorted((path.parent / "gates").glob("phase*-closure.json"))
    targets = read_gate_targets(Path(__file__).resolve().parent / "gate_targets.ps1")
    ledgers = {}
    for closure_path in closures:
        named = CLOSURE_NAME.match(closure_path.name)
        if not named:
            errors.append(f"{closure_path.name} is not named phase<N>-closure.json")
            continue
        try:
            closure = json.loads(closure_path.read_text(encoding="utf-8"))
        except (OSError, ValueError) as error:
            parser.exit(2, f"error: {error}\n")
        ledgers[int(named.group(1))] = closure
        errors += validate_closure(data, closure, int(named.group(1)), targets)
        errors += validate_closure_evidence(closure, int(named.group(1)), path.parent)

    # Every phase that owns a row must publish a ledger. The loop above checks
    # each ledger it finds and none it does not, so a phase without one had rows in
    # no list -- neither closed nor deferred -- and nothing said so. Phases 4, 5
    # and 7 were in exactly that state and owned 5,340 rows between them.
    #
    # The full-census audit phase is exempt: its gate is "entire census closed" and
    # its rows are that audit's own subject rather than a phase's slate.
    # Scoped to the committed census, like the other path checks: a test builds a
    # synthetic inventory in a temporary directory with no gates beside it, and
    # would otherwise report every phase as unaccounted for.
    repo_root = Path(__file__).resolve().parents[4]
    if path.resolve() == (repo_root / 'docs' / 'reconstruction' / 'novodex-physics'
                          / 'inventory.json').resolve():
        owning = {row["phase"] for row in data["functions"] + data["data_objects"]}
        for phase in sorted(owning - set(ledgers)):
            if phase >= FULL_CENSUS_AUDIT_PHASE:
                continue
            errors.append(
                f"phase {phase} owns rows but publishes no closure ledger; every row "
                f"it owns is in neither a closed nor a deferred list, and a row in no "
                f"list is one nobody has accounted for")

    # program.json quotes the census and the ledgers back at the next phase, so
    # it is recomputed from both rather than read.
    try:
        program = json.loads((path.parent / "program.json").read_text(encoding="utf-8"))
    except (OSError, ValueError) as error:
        parser.exit(2, f"error: {error}\n")
    # The check applies to the committed census only. A test builds a synthetic
    # inventory whose rows name real-looking paths, and the allowlist describes
    # this census, so running it over a fixture reports the fixture's own gaps.
    repo_root = Path(__file__).resolve().parents[4]
    if path.resolve() == (repo_root / 'docs' / 'reconstruction' / 'novodex-physics'
                          / 'inventory.json').resolve():
        errors += _check_implemented_rows(data['functions'])
        errors += _check_reconstructed_proofs(data['functions'])
        errors += _check_source_paths(data['functions'], path.parent)
        errors += _check_implementation_paths(data['functions'], path.parent)
        errors += _check_implementation_contains_row(data['functions'], repo_root)

    # The shape ruling is recomputed wherever it sits beside an inventory, and its
    # absence is an error for the committed census: without it, the rows it places
    # would be back to claiming whatever they were last written as. The spans read the
    # Ghidra manifest's __FILE__ strings and references; like the path checks above,
    # the span check itself is scoped to the committed census, because a fixture has
    # no oracle beside it and its spans would be the fixture's own.
    committed = path.resolve() == (repo_root / 'docs' / 'reconstruction' / 'novodex-physics'
                                   / 'inventory.json').resolve()
    ruling_path = path.parent / SHAPE_RULING_NAME
    pins_path = path.parent / PHASE_PINS_NAME
    ruling = pe_oracle = corpus = ghidra = spans = None
    pins = {}
    try:
        if ruling_path.is_file() or committed:
            ghidra_path = path.parent / "oracle" / "ghidra" / "manifest.json"
            if ghidra_path.is_file() or committed:
                ghidra = json.loads(ghidra_path.read_text(encoding="utf-8"))
        if ruling_path.is_file():
            ruling = json.loads(ruling_path.read_text(encoding="utf-8"))
            pe_oracle = json.loads((path.parent / "oracle" / "pe.json").read_text(encoding="utf-8"))
            corpus = json.loads((path.parent / "oracle" / "capstone" / "manifest.json")
                                .read_text(encoding="utf-8"))
        if pins_path.is_file():
            pins, pin_errors = read_phase_pins(json.loads(pins_path.read_text(encoding="utf-8")))
            errors += pin_errors
        elif committed:
            errors.append(f"the committed census has no {PHASE_PINS_NAME}")
    except (OSError, ValueError) as error:
        parser.exit(2, f"error: {error}\n")
    if ghidra is not None:
        spans, source_phases = translation_unit_spans_of(data["functions"], ghidra["strings"],
                                                         ghidra["references"])
    if ruling is not None:
        pointers = {slot["rva"]: slot["target_rva"] for slot in pe_oracle["pointers"]
                    if slot["relocated"] and slot["target_rva"] is not None}
        errors += validate_shape_ruling(
            data, ruling, pointers, {int(entry["rva"], 16): entry for entry in corpus["instructions"]},
            corpus["tail_calls"], pe_oracle["image"]["image_base"],
            {entry["iat_rva"] for entry in pe_oracle["imports"]}, spans)
    else:
        errors += [f"function {row['id']!r} claims {SLOT_RULING!r}, but there is no "
                   f"{SHAPE_RULING_NAME} beside the inventory to recompute it from"
                   for row in data["functions"] if row["phase_provenance"] == SLOT_RULING]
        if committed:
            errors.append(f"the committed census has no {SHAPE_RULING_NAME}")
    if committed:
        errors += validate_translation_unit_spans(
            data["functions"], spans, source_phases,
            frozenset(identifier for identifier, pin in pins.items()
                      if pin.get("phase_provenance") == "translation_unit"))
        # The phase column is regenerated, not read: the generator runs over the
        # committed oracle with the ruling, the third-party correspondence maps and the
        # pins, and every row must come out as the census has it.
        if ruling is not None:
            import reconcile_analysis
            correspondence, _ = read_source_correspondence(path.parent)
            try:
                result = reconcile_analysis.reconcile(
                    pe_oracle, ghidra, corpus, data, ruling,
                    reconcile_analysis.third_party_rvas(correspondence), pins)
            except ValueError as error:
                errors.append(f"the generator cannot regenerate the committed census: {error}")
            else:
                errors += validate_regeneration(data, result["inventory"], pins, result["unpinned"])
    errors += validate_program(data, program, ledgers, path.parent)
    errors += validate_discharge_passed(ledgers, program)
    errors += _check_data_vocabulary(data['data_objects'])
    errors += validate_classification(data)
    errors += validate_row_states(data, ledgers)
    stated = {row.get("phase"): row for row in program.get("phases", [])
              if isinstance(row, dict)}
    exclusions = read_differential_exclusions(
        Path(__file__).resolve().parent / "run_differential.ps1")
    records = {}
    for record_path in sorted((path.parent / "gates").glob("phase*.json")):
        named = PHASE_RECORD_NAME.match(record_path.name)
        if not named:
            continue
        try:
            record = json.loads(record_path.read_text(encoding="utf-8"))
        except (OSError, ValueError) as error:
            parser.exit(2, f"error: {error}\n")
        phase = int(named.group(1))
        records[phase] = record
        totals = _ledger_totals(ledgers[phase]) if phase in ledgers else None
        errors += validate_phase_record(record, phase, stated.get(phase), totals, exclusions)

    # An escalation and the plan that inherits it pin each other; neither can be
    # deleted without the other saying so.
    plans = {}
    for phase, row in stated.items():
        plan = plan_path(path.parent, row["plan"]) if isinstance(row.get("plan"), str) else None
        if plan is not None and plan.is_file():
            plans[phase] = read_plan_escalations(plan)
    errors += validate_escalations(records, plans)
    errors += validate_orphaned_deferrals(ledgers, records, program)

    if errors:
        for error in errors:
            print(f"error: {error}", file=sys.stderr)
        return 1

    coverage = data["coverage"]
    print("inventory=pass")
    for closure_path in closures:
        closure = json.loads(closure_path.read_text(encoding="utf-8"))
        counts = closure["counts"]
        deferred = sum(v for k, v in counts.items() if k.startswith("deferred_"))
        print(f"closure phase={closure['phase']}"
              f" closed={sum(counts.values()) - deferred}"
              f" deferred={deferred}")
    print(f"functions={len(data['functions'])}")
    print(f"data_objects={len(data['data_objects'])}")
    print(
        "unexplained="
        f"{coverage['unexplained_executable_bytes'] + coverage['unexplained_referenced_data_bytes']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
