import hashlib
import json
import pathlib
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


TOOLS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS_DIR))

import generate_ghidra_types  # noqa: E402


SIMPLE_TYPES_H = """\
#ifndef NX_SIMPLE_TYPES
#define NX_SIMPLE_TYPES
typedef unsigned int NxU32;
typedef float NxReal;
#endif
"""

VEC3_H = """\
#ifndef NX_VEC3
#define NX_VEC3
#include "NxSimpleTypes.h"

/**
Doc comment that must not confuse the parser: class NxNotAType { };
*/
class NxVec3
\t{
\tpublic:
\tNX_INLINE NxVec3();
\tNX_INLINE NxVec3(NxReal _x, NxReal _y, NxReal _z);
\tNX_INLINE void set(NxReal _x, NxReal _y, NxReal _z)
\t\t{
\t\tx = _x; y = _y; z = _z;
\t\t}
\tNX_INLINE NxVec3& operator=(const NxVec3& v);
\tNX_INLINE bool isFinite() const;
\tstatic NxReal tolerance;

\tNxReal x, y, z;
\t};
#endif
"""

MINI_SHAPE_H = """\
#ifndef MINI_SHAPE
#define MINI_SHAPE
#include "NxVec3.h"

enum MiniFlag
\t{
\tMINI_A = 1 << 0,
\tMINI_B = 1 << 2,
\tMINI_C = MINI_B,
\tMINI_PAD = 0x7fffffff
\t};

class MiniShape
\t{
\tpublic:
\tNxVec3\t\t\tcenter;
\tNxU32\t\t\tflags;
\tvoid*\t\t\tuserData;
\tconst char*\t\tname;
#ifdef MINI_EXTRA
\tNxU32\t\t\textra;
#endif
#if MINI_LEVEL > 1
\tNxU32\t\t\tleveled;
#endif
\tbool\t\t\tenabled;
\tvirtual void release() = 0;
\tvirtual ~MiniShape();
\tprotected:
\tNX_INLINE MiniShape();
\t};

class MiniBox : public MiniShape
\t{
\tpublic:
\tNxVec3\tdimensions;
\tNxReal\tmargin[3];
\t};

NX_C_EXPORT NXP_DLL_EXPORT NxReal NX_CALL_CONV MiniComputeVolume(const NxVec3& extents, NxReal density);
NX_C_EXPORT NXP_DLL_EXPORT void NX_CALL_CONV MiniReset(MiniShape* shape, bool hard);
NX_C_EXPORT NXP_DLL_EXPORT const NxU32* NX_CALL_CONV MiniGetTable();
NX_C_EXPORT NXP_DLL_EXPORT NxU32 miniGlobalCounter;
#endif
"""

MINI_MATRIX_H = """\
#ifndef MINI_MATRIX
#define MINI_MATRIX
#include "NxVec3.h"

typedef NxU32* MiniHandle;

class MiniCell
\t{
\tpublic:
\tNxU32 tag;
\t};

class MiniMatrix
\t{
\tpublic:
\ttypedef NxU32 IndexType;
\tunion
\t\t{
\t\tstruct Row : public MiniCell
\t\t\t{
\t\t\tNxReal a, b;
\t\t\t} r;
\t\tNxReal cells[2];
\t\t};
\tIndexType index;
\t};
#endif
"""

MINI_LIST_H = """\
#ifndef MINI_LIST
#define MINI_LIST
#include "MiniShape.h"

typedef NxU32* MiniHandle;

class NXP_DLL_EXPORT MiniList
\t{
\tpublic:
\tNxArray<MiniShape*>\tshapes;
\tNxU32\t\t\t\tcount;
\t};
#endif
"""

FORCED_NX_ARRAY = {
    "name": "NxArray<MiniShape*>",
    "emit_as": "NxArray_MiniShape_ptr",
    "declaration": "typedef struct NxArray_MiniShape_ptr { void *data; unsigned int size; "
                   "unsigned int capacity; } NxArray_MiniShape_ptr;",
    "reason": "Class template instantiation; the generator does not instantiate templates.",
}


BASE_DEFINITIONS = {
    "_WIN32": "1", "NX_USE_SDK_DLLS": "1", "MINI_LEVEL": "1",
    "NX_C_EXPORT": "extern \"C\"", "NXP_DLL_EXPORT": "__declspec(dllexport)",
    "NX_CALL_CONV": "__cdecl", "NX_INLINE": "inline",
}


def sha256_text(text):
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


class Corpus:
    """A synthesized Novodex-shaped header tree with its two pinned manifests."""

    def __init__(self, root):
        self.root = Path(root)
        self.headers = self.root / "Novodex"
        self.evidence = self.root / "evidence"
        self.foundation_files = {"Foundation/include/NxSimpleTypes.h": SIMPLE_TYPES_H,
                                 "Foundation/include/NxVec3.h": VEC3_H}
        self.physics_files = {"MiniShape.h": MINI_SHAPE_H, "MiniList.h": MINI_LIST_H,
                              "MiniMatrix.h": MINI_MATRIX_H}
        self.inputs_overrides = {}
        self.omit_from_tree = set()
        self.physics_manifest_override = None
        self.foundation_manifest_override = None

    def write(self):
        for relative, text in self.foundation_files.items():
            self._write(self.headers / relative, text)
        for relative, text in self.physics_files.items():
            if relative not in self.omit_from_tree:
                self._write(self.headers / "Physics" / "include" / relative, text)

        physics_manifest = self.physics_manifest_override or {
            "schema_version": 1, "algorithm": "SHA-256",
            "root": (self.headers / "Physics/include").as_posix(),
            "file_count": len(self.physics_files),
            "files": [{"path": name, "size": len(text.encode("utf-8")),
                       "sha256": sha256_text(text)}
                      for name, text in sorted(self.physics_files.items())],
        }
        foundation_manifest = self.foundation_manifest_override or {
            "schema_version": 2, "algorithm": "SHA-256",
            "files": [{"path": name,
                       "raw_worktree_sha256": sha256_text(text),
                       "raw_oracle_sha256": sha256_text(text)}
                      for name, text in sorted(self.foundation_files.items())],
        }
        physics_path = self.evidence / "public_header_hashes.json"
        foundation_path = self.evidence / "dumps" / "immutable_public_hashes.json"
        self._write(physics_path, json.dumps(physics_manifest, indent=2) + "\n")
        self._write(foundation_path, json.dumps(foundation_manifest, indent=2) + "\n")

        inputs = {
            "schema_version": 1,
            "generator_version": generate_ghidra_types.GENERATOR_VERSION,
            "physics_manifest": {
                "path": "../public_header_hashes.json",
                "sha256": hashlib.sha256(physics_path.read_bytes()).hexdigest(),
            },
            "foundation_manifest": {
                "path": "../dumps/immutable_public_hashes.json",
                "sha256": hashlib.sha256(foundation_path.read_bytes()).hexdigest(),
            },
            "include_roots": ["Physics/include", "Foundation/include"],
            "excluded_headers": [],
            "excluded_types": [],
            "preprocessor_definitions": dict(BASE_DEFINITIONS),
            "forced_declarations": [dict(FORCED_NX_ARRAY)],
            "type_selection_rules": ["R1 enums", "R2 layout structs"],
        }
        inputs.update(self.inputs_overrides)
        inputs_path = self.evidence / "ghidra" / "physics_type_inputs.json"
        self._write(inputs_path, json.dumps(inputs, indent=2) + "\n")
        return self.headers, inputs_path

    @staticmethod
    def _write(path, text):
        path.parent.mkdir(parents=True, exist_ok=True)
        # newline="" keeps the bytes on disk equal to the text the manifests hash.
        path.write_text(text, encoding="utf-8", newline="")


class GenerateGhidraTypesTests(unittest.TestCase):
    def generate(self, configure=None):
        with tempfile.TemporaryDirectory() as directory:
            corpus = Corpus(directory)
            if configure:
                configure(corpus)
            headers, inputs = corpus.write()
            return generate_ghidra_types.generate(headers, inputs)

    def assertRejects(self, configure, pattern):
        with self.assertRaisesRegex(ValueError, pattern):
            self.generate(configure)

    def body(self, text, name):
        """Return the emitted definition block for one type."""
        for block in text.split("\n\n"):
            if block.strip().startswith((f"struct {name} {{", f"enum {name} {{")):
                return block.strip()
        raise AssertionError(f"{name} was not emitted:\n{text}")

    def test_emits_file_scope_typedefs_from_the_headers(self):
        text = self.generate()
        self.assertIn("typedef unsigned int NxU32;", text)
        self.assertIn("typedef float NxReal;", text)

    def test_evaluates_enumerator_expressions_to_literal_integers(self):
        self.assertEqual(
            self.body(self.generate(), "MiniFlag"),
            "enum MiniFlag {\n"
            "    MINI_A = 1,\n"
            "    MINI_B = 4,\n"
            "    MINI_C = 4,\n"
            "    MINI_PAD = 2147483647\n"
            "};",
        )

    def test_emits_only_data_members_and_expands_multiple_declarators(self):
        # NxVec3's ctors, inline body, operator=, const method and static member
        # occupy no storage, so only x/y/z may survive.
        self.assertEqual(
            self.body(self.generate(), "NxVec3"),
            "struct NxVec3 {\n"
            "    NxReal x;\n"
            "    NxReal y;\n"
            "    NxReal z;\n"
            "};",
        )

    def test_gives_a_polymorphic_class_a_leading_vftable_pointer(self):
        self.assertEqual(
            self.body(self.generate(), "MiniShape"),
            "struct MiniShape {\n"
            "    void **__vftable;\n"
            "    NxVec3 center;\n"
            "    NxU32 flags;\n"
            "    void *userData;\n"
            "    char *name;\n"
            "    unsigned char enabled;\n"
            "};",
        )

    def test_embeds_a_base_class_as_the_first_member(self):
        self.assertEqual(
            self.body(self.generate(), "MiniBox"),
            "struct MiniBox {\n"
            "    MiniShape __base_MiniShape;\n"
            "    NxVec3 dimensions;\n"
            "    NxReal margin[3];\n"
            "};",
        )

    def test_honours_preprocessor_definitions_when_selecting_members(self):
        text = self.generate()
        # MINI_EXTRA is undefined and MINI_LEVEL is 1, so neither guarded member
        # may appear; a wrong answer here silently shifts every later offset.
        self.assertNotIn("extra", text)
        self.assertNotIn("leveled", text)

    def test_includes_a_member_whose_guard_the_definitions_satisfy(self):
        def configure(corpus):
            corpus.inputs_overrides["preprocessor_definitions"] = dict(
                BASE_DEFINITIONS, MINI_LEVEL="4")
        self.assertIn("    NxU32 leveled;", self.generate(configure))

    def test_emits_exported_functions_as_c_prototypes_with_pointers_for_references(self):
        text = self.generate()
        self.assertIn("NxReal __cdecl MiniComputeVolume(NxVec3 *extents, NxReal density);", text)
        self.assertIn("void __cdecl MiniReset(MiniShape *shape, unsigned char hard);", text)

    def test_resolves_a_template_member_through_a_forced_declaration(self):
        text = self.generate()
        self.assertIn(FORCED_NX_ARRAY["declaration"], text)
        self.assertEqual(
            self.body(text, "MiniList"),
            "struct MiniList {\n"
            "    NxArray_MiniShape_ptr shapes;\n"
            "    NxU32 count;\n"
            "};",
        )

    def test_orders_declarations_so_every_type_precedes_its_users(self):
        text = self.generate()
        self.assertLess(text.index("typedef float NxReal;"), text.index("struct NxVec3 {"))
        self.assertLess(text.index("struct NxVec3 {"), text.index("struct MiniShape {"))
        self.assertLess(text.index("struct MiniShape {"), text.index("struct MiniBox {"))
        self.assertLess(text.index("struct MiniBox {"), text.index("MiniComputeVolume"))

    def test_names_the_manifests_it_derived_from_without_quoting_their_hashes(self):
        # Quoting the hashes coupled the shim to the pin: re-pinning rewrote the
        # shim, whose own hash the Ghidra manifest records, so a one-line pin
        # change cascaded through a 27 MB oracle. The banner names what the shim
        # came from; the hashes stay in the inputs file, where they are checked.
        with tempfile.TemporaryDirectory() as directory:
            corpus = Corpus(directory)
            headers, inputs = corpus.write()
            declared = json.loads(inputs.read_text(encoding="utf-8"))
            text = generate_ghidra_types.generate(headers, inputs)
        self.assertIn(f"generator_version {generate_ghidra_types.GENERATOR_VERSION}", text)
        for manifest in ("physics_manifest", "foundation_manifest"):
            self.assertIn(declared[manifest]["path"], text)
            self.assertNotIn(declared[manifest]["sha256"], text)

    def test_a_re_pin_leaves_the_generated_shim_unchanged(self):
        # The point of naming the manifests rather than quoting their hashes.
        with tempfile.TemporaryDirectory() as directory:
            corpus = Corpus(directory)
            headers, inputs = corpus.write()
            before = generate_ghidra_types.generate(headers, inputs)
            declared = json.loads(inputs.read_text(encoding="utf-8"))
            manifest = pathlib.Path(inputs).parent / declared["physics_manifest"]["path"]
            manifest.write_bytes(manifest.read_bytes() + b"\n")
            declared["physics_manifest"]["sha256"] = hashlib.sha256(
                manifest.read_bytes()).hexdigest()
            inputs.write_text(json.dumps(declared), encoding="utf-8")
            self.assertEqual(generate_ghidra_types.generate(headers, inputs), before)

    def test_generating_twice_from_one_corpus_produces_identical_text(self):
        with tempfile.TemporaryDirectory() as directory:
            headers, inputs = Corpus(directory).write()
            first = generate_ghidra_types.generate(headers, inputs)
            second = generate_ghidra_types.generate(headers, inputs)
        self.assertEqual(first, second)

    def test_hoists_a_nested_type_and_names_the_anonymous_union_slot(self):
        text = self.generate()
        # The nested path must embed a base clause exactly as the top-level path
        # does; it silently dropped them until the NxProfiler::DefineZone defect.
        self.assertEqual(
            self.body(text, "MiniMatrix_Row"),
            "struct MiniMatrix_Row {\n"
            "    MiniCell __base_MiniCell;\n"
            "    NxReal a;\n"
            "    NxReal b;\n"
            "};",
        )

    def test_rejects_multiple_inheritance_declared_by_a_nested_class(self):
        # The top-level guard was proved by test_rejects_multiple_inheritance; this
        # locks in that the nested path reaches the same guard rather than
        # discarding the base clause.
        self.assertRejects(
            lambda corpus: corpus.physics_files.__setitem__(
                "MiniMatrix.h", MINI_MATRIX_H.replace(
                    "struct Row : public MiniCell",
                    "struct Row : public MiniCell, public NxVec3")),
            r"MiniMatrix\.h:\d+: class MiniMatrix_Row has 2 base classes; "
            "only single inheritance is supported",
        )

    def test_rejects_a_virtual_base_declared_by_a_nested_class(self):
        self.assertRejects(
            lambda corpus: corpus.physics_files.__setitem__(
                "MiniMatrix.h", MINI_MATRIX_H.replace(
                    "struct Row : public MiniCell",
                    "struct Row : virtual public MiniCell")),
            r"MiniMatrix\.h:\d+: class MiniMatrix_Row has a virtual base class",
        )

    def test_a_nested_types_virtual_member_does_not_make_its_owner_polymorphic(self):
        # NxProfiler shipped with a phantom vftable pointer because the `virtual`
        # scan ran over nested declarations too.
        def configure(corpus):
            corpus.physics_files["MiniMatrix.h"] = (
                MINI_MATRIX_H
                .replace("struct Row : public MiniCell", "struct Row")
                .replace("\t\t\tNxReal a, b;",
                         "\t\t\tNxReal a, b;\n\t\t\tvirtual void release();"))
        text = self.generate(configure)
        self.assertIn("void **__vftable;", self.body(text, "MiniMatrix_Row"))
        self.assertNotIn("__vftable", self.body(text, "MiniMatrix"))
        # The union slot must be named so the shim stays inside C89, and the
        # nested tag must resolve to its hoisted name.
        self.assertEqual(
            self.body(text, "MiniMatrix"),
            "struct MiniMatrix {\n"
            "    union {\n"
            "        MiniMatrix_Row r;\n"
            "        NxReal cells[2];\n"
            "    } __anon_0;\n"
            "    NxU32 index;\n"
            "};",
        )

    def test_resolves_an_in_class_typedef_to_its_underlying_type(self):
        # MiniMatrix declares `typedef NxU32 IndexType;` then uses IndexType.
        self.assertIn("    NxU32 index;", self.generate())

    def test_emits_a_pointer_typedef_without_making_it_an_ordering_dependency(self):
        self.assertIn("typedef NxU32 *MiniHandle;", self.generate())

    def test_allows_one_declaration_repeated_verbatim_in_two_headers(self):
        # MiniList.h and MiniMatrix.h both declare `typedef NxU32* MiniHandle;`.
        # An identical repeat cannot move a member, so it must not be an error.
        self.assertEqual(self.generate().count("typedef NxU32 *MiniHandle;"), 1)

    def test_skips_a_header_the_inputs_exclude(self):
        def configure(corpus):
            corpus.inputs_overrides["excluded_headers"] = [
                {"path": "Physics/include/MiniList.h", "reason": "not part of this ABI"}]
        text = self.generate(configure)
        self.assertNotIn("struct MiniList {", text)
        self.assertIn("struct MiniShape {", text)

    def test_rejects_an_excluded_header_that_no_manifest_pins(self):
        self.assertRejects(
            lambda corpus: corpus.inputs_overrides.__setitem__(
                "excluded_headers", [{"path": "Physics/include/Ghost.h", "reason": "typo"}]),
            "excluded header Physics/include/Ghost.h is not pinned by either manifest",
        )

    def test_rejects_a_class_tag_that_is_not_an_identifier(self):
        self.assertRejects(
            lambda corpus: corpus.physics_files.__setitem__(
                "MiniList.h", MINI_LIST_H.replace("class NXP_DLL_EXPORT MiniList", "class ?")),
            r"MiniList\.h:\d+: class tag '\?' is not an identifier",
        )

    def test_emits_a_zero_parameter_export_rather_than_dropping_it(self):
        # An empty parameter list must not be mistaken for the empty parentheses
        # an erased attribute macro leaves behind.
        self.assertIn("NxU32 *__cdecl MiniGetTable(void);", self.generate())

    def test_parses_a_class_whose_tag_is_preceded_by_an_attribute_macro(self):
        # `class NXP_DLL_EXPORT MiniList` expands to `class __declspec(...) MiniList`;
        # the attribute must not hide the class body from the parser.
        self.assertIn("struct MiniList {", self.generate())

    def test_recognizes_an_extern_c_variable_without_emitting_a_prototype(self):
        text = self.generate()
        self.assertNotIn("miniGlobalCounter(", text)
        self.assertNotIn("miniGlobalCounter;", text)

    def test_widens_an_enum_to_the_four_byte_msvc_underlying_type(self):
        # Without the SDK's own pad the values stop at 4, so a C parser would
        # size the enum at one byte and move every member after it.
        def configure(corpus):
            corpus.physics_files["MiniShape.h"] = MINI_SHAPE_H.replace(
                ",\n\tMINI_PAD = 0x7fffffff", "")
        self.assertIn(
            f"    MiniFlag{generate_ghidra_types.FORCE_32_BIT_SUFFIX} = 2147483647",
            self.generate(configure))

    def test_leaves_an_enum_that_already_spans_32_bits_alone(self):
        # MiniFlag carries the SDK's own NX_..._FORCE_DWORD-style pad.
        text = self.generate()
        self.assertIn("MINI_PAD = 2147483647", text)
        self.assertNotIn(generate_ghidra_types.FORCE_32_BIT_SUFFIX, text)

    def test_carries_a_forced_declarations_reason_into_the_generated_header(self):
        # The caveat has to travel with the artifact, not stay in the inputs file.
        self.assertIn("forced declaration: NxArray<MiniShape*>", self.generate())
        self.assertIn("does not instantiate templates", self.generate())

    def test_skips_a_type_the_inputs_exclude_and_says_so_in_the_header(self):
        def configure(corpus):
            corpus.inputs_overrides["excluded_types"] = [
                {"name": "MiniBox", "reason": "underivable under the target ABI"}]
        text = self.generate(configure)
        self.assertNotIn("struct MiniBox {", text)
        self.assertIn("struct MiniShape {", text)
        # The withheld type must leave a marker where it would have appeared, so
        # the caveat travels with the artifact rather than staying in the inputs.
        self.assertIn("/* excluded type: MiniBox", text)
        self.assertIn("underivable under the target ABI", text)

    def test_rejects_withholding_a_type_an_emitted_member_still_needs(self):
        # MiniShape is reachable: MiniBox derives from it and MiniReset takes one.
        self.assertRejects(
            lambda corpus: corpus.inputs_overrides.__setitem__(
                "excluded_types", [{"name": "MiniShape", "reason": "would leave a hole"}]),
            "excluded type MiniShape is reachable from MiniBox.__base_MiniShape, "
            "so it cannot be withheld",
        )

    def test_inherits_a_vftable_rather_than_adding_a_second_one(self):
        # MiniBox derives from polymorphic MiniShape, so the vftable pointer comes
        # from the base subobject and must not be repeated.
        box = self.body(self.generate(), "MiniBox")
        self.assertNotIn("__vftable", box)
        self.assertIn("MiniShape __base_MiniShape;", box)

    def test_rejects_virtual_members_added_over_a_base_with_no_vftable(self):
        # MSVC puts the new vftable pointer *before* the base subobject here,
        # which reorders the whole layout; the generator will not guess it.
        self.assertRejects(
            lambda corpus: corpus.physics_files.__setitem__(
                "MiniMatrix.h", MINI_MATRIX_H.replace(
                    "\t\t\tNxReal a, b;",
                    "\t\t\tNxReal a, b;\n\t\t\tvirtual void release();")),
            "class MiniMatrix_Row adds virtual members over base MiniCell, which has no "
            "vftable of its own",
        )

    def test_rejects_a_forced_declaration_naming_a_withheld_type(self):
        # A forced body is hand-written text with no parsed references, so it is
        # the one place a withheld type could slip past the reachability bound.
        def configure(corpus):
            corpus.inputs_overrides["excluded_types"] = [
                {"name": "MiniList", "reason": "underivable under the target ABI"}]
            corpus.inputs_overrides["forced_declarations"] = [
                dict(FORCED_NX_ARRAY,
                     declaration="typedef struct NxArray_MiniShape_ptr { "
                                 "MiniList *first; } NxArray_MiniShape_ptr;")]
        self.assertRejects(
            configure,
            "forced declaration 'NxArray_MiniShape_ptr' names excluded type MiniList",
        )

    def test_rejects_withholding_a_type_an_export_signature_names(self):
        self.assertRejects(
            lambda corpus: corpus.inputs_overrides.__setitem__(
                "excluded_types", [{"name": "NxVec3", "reason": "would leave a hole"}]),
            "excluded type NxVec3 is reachable from",
        )

    def test_rejects_an_excluded_type_that_no_pinned_header_declares(self):
        self.assertRejects(
            lambda corpus: corpus.inputs_overrides.__setitem__(
                "excluded_types", [{"name": "MiniGhost", "reason": "typo"}]),
            "excluded type MiniGhost is not declared by any pinned header",
        )

    def test_rejects_a_live_error_directive(self):
        # The #error guards are what prove the recorded macro table selects a
        # configuration the SDK actually supports.
        self.assertRejects(
            lambda corpus: corpus.physics_files.__setitem__(
                "MiniList.h", MINI_LIST_H.replace(
                    "#include \"MiniShape.h\"",
                    "#ifndef MINI_LEVEL\n#error unsupported configuration\n#endif\n"
                    "#include \"MiniShape.h\"").replace("MINI_LEVEL", "MINI_ABSENT", 1)),
            r"MiniList\.h:\d+: #error unsupported configuration",
        )

    def test_rejects_an_anonymous_enum_inside_a_class(self):
        self.assertRejects(
            lambda corpus: corpus.physics_files.__setitem__(
                "MiniList.h", MINI_LIST_H.replace(
                    "\tNxU32\t\t\t\tcount;", "\tenum { MINI_ONE = 1 };")),
            "MiniList declares an anonymous enum",
        )

    def test_rejects_an_in_class_typedef_it_cannot_parse(self):
        self.assertRejects(
            lambda corpus: corpus.physics_files.__setitem__(
                "MiniMatrix.h", MINI_MATRIX_H.replace(
                    "\ttypedef NxU32 IndexType;", "\ttypedef NxU32;")),
            "cannot parse typedef in MiniMatrix",
        )

    def test_rejects_an_unbalanced_bracket(self):
        self.assertRejects(
            lambda corpus: corpus.physics_files.__setitem__(
                "MiniList.h", MINI_LIST_H.replace("NxU32\t\t\t\tcount;", "NxU32 count[4;")),
            r"unbalanced '\['",
        )

    def test_rejects_an_if_expression_it_cannot_evaluate(self):
        self.assertRejects(
            lambda corpus: corpus.physics_files.__setitem__(
                "MiniList.h", MINI_LIST_H.replace(
                    "#include \"MiniShape.h\"", "#if MINI_LEVEL >\n#endif\n"
                    "#include \"MiniShape.h\"")),
            r"MiniList\.h:\d+: cannot evaluate #if",
        )

    def test_rejects_a_tag_that_is_neither_a_definition_nor_a_forward_declaration(self):
        self.assertRejects(
            lambda corpus: corpus.physics_files.__setitem__(
                "MiniList.h", MINI_LIST_H.replace("#endif", "class Broken extra;\n#endif")),
            "cannot parse class declaration",
        )

    def test_rejects_an_anonymous_union_with_several_declarators(self):
        self.assertRejects(
            lambda corpus: corpus.physics_files.__setitem__(
                "MiniMatrix.h", MINI_MATRIX_H.replace("\t\t};", "\t\t} u, v;")),
            "declares an anonymous union with 2 declarators",
        )

    def test_rejects_an_extern_c_declaration_it_cannot_parse(self):
        self.assertRejects(
            lambda corpus: corpus.physics_files.__setitem__(
                "MiniShape.h", MINI_SHAPE_H.replace(
                    "NX_C_EXPORT NXP_DLL_EXPORT NxU32 miniGlobalCounter;",
                    "NX_C_EXPORT NXP_DLL_EXPORT miniGlobalCounter;")),
            r'cannot parse extern "C" declaration',
        )

    def test_rejects_an_inputs_file_missing_a_required_key(self):
        self.assertRejects(
            lambda corpus: corpus.inputs_overrides.update({"include_roots": None}),
            "inputs file is missing 'include_roots'",
        )

    def test_rejects_an_inputs_file_written_for_another_generator_version(self):
        self.assertRejects(
            lambda corpus: corpus.inputs_overrides.update({"generator_version": 99}),
            "inputs declare generator_version 99 but this generator is version 1",
        )

    def test_rejects_a_physics_manifest_whose_bytes_changed(self):
        self.assertRejects(
            lambda corpus: corpus.inputs_overrides.update(
                {"physics_manifest": {"path": "../public_header_hashes.json", "sha256": "b" * 64}}),
            "public_header_hashes.json sha256 [0-9a-f]{64} does not match the pinned b{64}",
        )

    def test_rejects_a_foundation_manifest_whose_bytes_changed(self):
        self.assertRejects(
            lambda corpus: corpus.inputs_overrides.update(
                {"foundation_manifest": {"path": "../dumps/immutable_public_hashes.json",
                                         "sha256": "c" * 64}}),
            "immutable_public_hashes.json sha256 [0-9a-f]{64} does not match the pinned c{64}",
        )

    def test_rejects_a_header_the_manifest_pins_but_the_tree_lacks(self):
        self.assertRejects(
            lambda corpus: corpus.omit_from_tree.add("MiniList.h"),
            "missing pinned header Physics/include/MiniList.h",
        )

    def test_rejects_a_header_whose_content_drifted_from_the_pin(self):
        def configure(corpus):
            corpus.physics_manifest_override = {
                "schema_version": 1, "algorithm": "SHA-256", "root": "x", "file_count": 2,
                "files": [{"path": "MiniList.h", "size": 1, "sha256": "d" * 64},
                          {"path": "MiniShape.h", "size": len(MINI_SHAPE_H.encode()),
                           "sha256": sha256_text(MINI_SHAPE_H)}],
            }
        self.assertRejects(
            configure,
            r"Physics/include/MiniList\.h sha256 [0-9a-f]{64} does not match the pinned d{64}",
        )

    def test_rejects_a_construct_the_parser_cannot_derive_a_layout_from(self):
        self.assertRejects(
            lambda corpus: corpus.physics_files.__setitem__(
                "MiniShape.h", MINI_SHAPE_H.replace(
                    "\tNxU32\t\t\tflags;", "\tNxU32 (*callback)(NxU32);")),
            r"MiniShape\.h:\d+: cannot parse member declaration 'NxU32 \(\*callback\)\(NxU32\);'",
        )

    def test_rejects_multiple_inheritance(self):
        self.assertRejects(
            lambda corpus: corpus.physics_files.__setitem__(
                "MiniShape.h", MINI_SHAPE_H.replace(
                    "class MiniBox : public MiniShape",
                    "class MiniBox : public MiniShape, public NxVec3")),
            r"MiniShape\.h:\d+: class MiniBox has 2 base classes; only single inheritance is supported",
        )

    def test_rejects_virtual_inheritance(self):
        self.assertRejects(
            lambda corpus: corpus.physics_files.__setitem__(
                "MiniShape.h", MINI_SHAPE_H.replace(
                    "class MiniBox : public MiniShape",
                    "class MiniBox : virtual public MiniShape")),
            r"MiniShape\.h:\d+: class MiniBox has a virtual base class",
        )

    def test_rejects_a_member_whose_type_no_header_declares(self):
        self.assertRejects(
            lambda corpus: corpus.physics_files.__setitem__(
                "MiniShape.h", MINI_SHAPE_H.replace("\tNxU32\t\t\tflags;", "\tNxMystery\tflags;")),
            r"MiniShape\.h:\d+: member 'flags' of MiniShape has unresolved type 'NxMystery'",
        )

    def test_rejects_a_forced_declaration_for_a_type_the_headers_define(self):
        def configure(corpus):
            corpus.inputs_overrides["forced_declarations"] = [
                dict(FORCED_NX_ARRAY),
                {"name": "NxVec3", "emit_as": "NxVec3",
                 "declaration": "typedef struct NxVec3 { float x; float y; float z; } NxVec3;",
                 "reason": "hand-written layout"},
            ]
        self.assertRejects(
            configure,
            "forced declaration 'NxVec3' supplies a layout for a type declared in "
            r"Foundation/include/NxVec3\.h",
        )

    def test_rejects_an_unbalanced_preprocessor_conditional(self):
        self.assertRejects(
            lambda corpus: corpus.physics_files.__setitem__(
                "MiniShape.h", MINI_SHAPE_H.replace("#ifdef MINI_EXTRA\n", "")),
            r"MiniShape\.h:\d+: #endif without a matching #if",
        )

    def test_rejects_one_type_declared_by_two_headers(self):
        self.assertRejects(
            lambda corpus: corpus.physics_files.__setitem__(
                "MiniList.h", MINI_LIST_H.replace("class NXP_DLL_EXPORT MiniList", "class MiniBox")),
            r"MiniBox is declared in both Physics/include/MiniList\.h and "
            r"Physics/include/MiniShape\.h",
        )

    def test_rejects_an_enumerator_whose_initializer_is_not_constant(self):
        self.assertRejects(
            lambda corpus: corpus.physics_files.__setitem__(
                "MiniShape.h", MINI_SHAPE_H.replace("MINI_B = 1 << 2", "MINI_B = sizeof(NxVec3)")),
            r"MiniShape\.h:\d+: enumerator MINI_B has a non-constant initializer 'sizeof\(NxVec3\)'",
        )


class GenerateGhidraTypesCliTests(unittest.TestCase):
    def test_writes_the_shim_and_reports_what_it_emitted(self):
        with tempfile.TemporaryDirectory() as directory:
            headers, inputs = Corpus(directory).write()
            output = Path(directory) / "ghidra" / "physics_x86_msvc.h"
            result = subprocess.run(
                [sys.executable, str(TOOLS_DIR / "generate_ghidra_types.py"),
                 "--headers", str(headers), "--inputs", str(inputs), "--output", str(output)],
                capture_output=True, text=True, check=False,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            text = output.read_text(encoding="utf-8")
        self.assertIn("types=written", result.stdout)
        self.assertIn("enums=1", result.stdout)
        self.assertIn("structs=7", result.stdout)
        self.assertIn("functions=3", result.stdout)
        self.assertIn("struct MiniBox {", text)

    def test_unreadable_inputs_report_cli_error_without_traceback(self):
        with tempfile.TemporaryDirectory() as directory:
            headers, _ = Corpus(directory).write()
            result = subprocess.run(
                [sys.executable, str(TOOLS_DIR / "generate_ghidra_types.py"),
                 "--headers", str(headers),
                 "--inputs", str(Path(directory) / "missing.json"),
                 "--output", str(Path(directory) / "out.h")],
                capture_output=True, text=True, check=False,
            )
        self.assertEqual(result.returncode, 2)
        self.assertIn("error:", result.stderr)
        self.assertNotIn("Traceback", result.stderr)


if __name__ == "__main__":
    unittest.main()
