#!/usr/bin/env python3
"""Derive a C-compatible ABI shim for Ghidra from the pinned Novodex public headers.

The shim carries no hand-authored type bodies: every enum, layout struct, base
embedding and exported prototype is derived from header text whose bytes are
pinned by the two public-header manifests. Constructs the derived subset cannot
express are rejected by name and line rather than skipped, because a silently
dropped member shifts the offset of every member after it.
"""

import argparse
import hashlib
import itertools
import json
import re
from pathlib import Path


GENERATOR_VERSION = 1

REQUIRED_INPUT_KEYS = (
    "schema_version", "generator_version", "physics_manifest", "foundation_manifest",
    "include_roots", "preprocessor_definitions", "forced_declarations", "type_selection_rules",
    "excluded_headers", "excluded_types",
)

# Layout-identical C spellings for the C++ and MSVC types the headers use. Each
# pair has the same size and alignment under Win32 MSVC, so substituting one for
# the other cannot move a member.
BUILTIN_ALIASES = {
    "bool": "unsigned char",
    "__int64": "long long",
}

BUILTIN_KEYWORDS = frozenset(
    ("void", "char", "short", "int", "long", "float", "double", "signed", "unsigned")
)

QUALIFIERS = frozenset(("const", "volatile", "mutable"))

# Widens an enum whose own values do not reach 32 bits to the 4-byte underlying
# type MSVC gives every enum. The suffix marks it as generator-added, so it can
# never be mistaken for an enumerator the SDK declared.
FORCE_32_BIT_SUFFIX = "__nx_force_32bit"
FORCE_32_BIT_VALUE = 0x7FFFFFFF

# Declarations carrying these markers define no storage, so they contribute no
# layout and are recognized rather than parsed.
NON_STORAGE_MARKERS = frozenset(("static", "friend", "typedef", "inline", "__forceinline"))

ACCESS_SPECIFIERS = frozenset(("public", "private", "protected"))

TOKEN_RE = re.compile(
    r"""(?P<space>[ \t]+)
      | (?P<newline>\n)
      | (?P<number>(?:0[xX][0-9a-fA-F]+|\d+)[uUlL]*)
      | (?P<name>[A-Za-z_~]\w*)
      | (?P<string>"(?:[^"\\\n]|\\.)*")
      | (?P<char>'(?:[^'\\\n]|\\.)*')
      | (?P<op><<|>>|<=|>=|==|!=|&&|\|\||::|->|\.\.\.|[-+*/%&|^~!<>=?:;,.(){}\[\]\\#])
      | (?P<other>\S)
    """,
    re.VERBOSE,
)

DIRECTIVE_RE = re.compile(r"^[ \t]*#[ \t]*(\w+)[ \t]*(.*)$")


class Token:
    __slots__ = ("value", "line")

    def __init__(self, value, line):
        self.value = value
        self.line = line

    def __repr__(self):
        return f"Token({self.value!r}, {self.line})"


class Declaration:
    """One emitted declaration plus the type spellings it names.

    ``references`` drives resolution (every spelling must be a builtin or a
    declared type) and ``dependencies`` drives emission order (only by-value
    members constrain it, since a forward declaration satisfies a pointer).
    """

    def __init__(self, kind, name, header, line, text=None, payload=None,
                 dependencies=(), references=()):
        self.kind = kind
        self.name = name
        self.header = header
        self.line = line
        self.text = text
        self.payload = payload
        self.dependencies = tuple(dependencies)
        self.references = tuple(references)

    @property
    def order_key(self):
        return (self.header, self.line, self.name)

    @property
    def signature(self):
        """What this declaration says, independent of where it was said."""
        return self.text if self.payload is None else repr(self.payload)

    def render(self, substitutions):
        if self.kind == "struct":
            return _render_struct(self.name, self.payload, substitutions)
        if self.kind == "function":
            return _render_function(self.payload, substitutions)
        return self.text


def strip_comments(text: str) -> str:
    """Remove C and C++ comments, preserving every newline so lines still count."""
    out = []
    index = 0
    length = len(text)
    while index < length:
        char = text[index]
        if char in '"\'':
            end = index + 1
            while end < length and text[end] != char:
                end += 2 if text[end] == "\\" else 1
            out.append(text[index:min(end + 1, length)])
            index = end + 1
        elif text.startswith("/*", index):
            end = text.find("*/", index + 2)
            end = length if end < 0 else end + 2
            out.append("\n" * text.count("\n", index, end))
            index = end
        elif text.startswith("//", index):
            end = text.find("\n", index)
            index = length if end < 0 else end
        else:
            out.append(char)
            index += 1
    return "".join(out)


def tokenize(text: str, first_line: int = 1) -> list:
    """Split text into tokens tagged with the physical line they came from."""
    tokens = []
    line = first_line
    for match in TOKEN_RE.finditer(text):
        kind = match.lastgroup
        if kind == "newline":
            line += 1
        elif kind != "space":
            tokens.append(Token(match.group(), line))
    return tokens


class ConstantExpression:
    """Evaluator for the integer constant expressions enums and #if lines use."""

    def __init__(self, tokens, resolve):
        self.tokens = tokens
        self.resolve = resolve
        self.index = 0

    def peek(self):
        return self.tokens[self.index].value if self.index < len(self.tokens) else None

    def take(self):
        token = self.tokens[self.index]
        self.index += 1
        return token.value

    def evaluate(self):
        value = self.ternary()
        if self.index != len(self.tokens):
            raise ValueError("trailing tokens")
        return value

    def ternary(self):
        condition = self.binary(0)
        if self.peek() != "?":
            return condition
        self.take()
        yes = self.ternary()
        if self.peek() != ":":
            raise ValueError("malformed conditional")
        self.take()
        return yes if condition else self.ternary()

    PRECEDENCE = (
        ("||",), ("&&",), ("|",), ("^",), ("&",), ("==", "!="),
        ("<", "<=", ">", ">="), ("<<", ">>"), ("+", "-"), ("*", "/", "%"),
    )

    def binary(self, level):
        if level >= len(self.PRECEDENCE):
            return self.unary()
        value = self.binary(level + 1)
        while self.peek() in self.PRECEDENCE[level]:
            operator = self.take()
            right = self.binary(level + 1)
            value = self.apply(operator, value, right)
        return value

    @staticmethod
    def apply(operator, left, right):
        if operator in ("/", "%") and right == 0:
            raise ValueError("division by zero")
        return {
            "||": lambda: int(bool(left) or bool(right)),
            "&&": lambda: int(bool(left) and bool(right)),
            "|": lambda: left | right, "^": lambda: left ^ right, "&": lambda: left & right,
            "==": lambda: int(left == right), "!=": lambda: int(left != right),
            "<": lambda: int(left < right), "<=": lambda: int(left <= right),
            ">": lambda: int(left > right), ">=": lambda: int(left >= right),
            "<<": lambda: left << right, ">>": lambda: left >> right,
            "+": lambda: left + right, "-": lambda: left - right,
            "*": lambda: left * right,
            "/": lambda: int(left / right) if left * right >= 0 else -(abs(left) // abs(right)),
            "%": lambda: left - right * (int(left / right) if left * right >= 0
                                         else -(abs(left) // abs(right))),
        }[operator]()

    def unary(self):
        token = self.peek()
        if token == "-":
            self.take()
            return -self.unary()
        if token == "+":
            self.take()
            return self.unary()
        if token == "~":
            self.take()
            return ~self.unary()
        if token == "!":
            self.take()
            return int(not self.unary())
        return self.primary()

    def primary(self):
        if self.peek() is None:
            raise ValueError("expression ended early")
        token = self.take()
        if token == "(":
            value = self.ternary()
            if self.peek() != ")":
                raise ValueError("unbalanced parenthesis")
            self.take()
            return value
        if token[0].isdigit():
            return int(token.rstrip("uUlL"), 0)
        if token[0].isalpha() or token[0] == "_":
            return self.resolve(token)
        raise ValueError(f"unexpected token {token!r}")


def evaluate_constant(text, resolve):
    return ConstantExpression(tokenize(text), resolve).evaluate()


class Preprocessor:
    """Evaluates conditionals and expands object-like macros for one header.

    Includes are never followed: the corpus is exactly the pinned file set, so a
    macro that decides a layout must come from the recorded definitions rather
    than from a header this pass did not read.
    """

    def __init__(self, header, definitions):
        self.header = header
        self.macros = dict(definitions)

    def run(self, text):
        lines = strip_comments(text).split("\n")
        lines = self._join_continuations(lines)
        kept = []
        # Each frame is (taken_any_branch, currently_active, parent_active).
        stack = []
        for number, line in enumerate(lines, start=1):
            match = DIRECTIVE_RE.match(line)
            active = all(frame[1] for frame in stack)
            if not match:
                kept.append(line if active else "")
                continue
            kept.append("")
            self._directive(match.group(1), match.group(2).strip(), stack, number, active)
        if stack:
            raise ValueError(f"{self.header}: {len(stack)} unterminated #if")
        return "\n".join(kept)

    @staticmethod
    def _join_continuations(lines):
        joined = []
        pending = ""
        for line in lines:
            if line.endswith("\\"):
                pending += line[:-1]
                joined.append("")
                continue
            joined.append(pending + line)
            pending = ""
        return joined

    def _directive(self, name, rest, stack, number, active):
        if name in ("ifdef", "ifndef", "if"):
            parent = all(frame[1] for frame in stack)
            taken = parent and self._condition(name, rest, number)
            stack.append([taken, taken, parent])
            return
        if name in ("elif", "else"):
            if not stack:
                raise ValueError(f"{self.header}:{number}: #{name} without a matching #if")
            frame = stack[-1]
            if frame[0]:
                frame[1] = False
                return
            taken = frame[2] and (name == "else" or self._condition("if", rest, number))
            frame[0] = taken
            frame[1] = taken
            return
        if name == "endif":
            if not stack:
                raise ValueError(f"{self.header}:{number}: #endif without a matching #if")
            stack.pop()
            return
        if not active:
            return
        if name == "define":
            key, _, value = rest.partition(" ")
            if "(" not in key:
                self.macros[key] = value.strip()
        elif name == "undef":
            self.macros.pop(rest.strip(), None)
        elif name == "error":
            raise ValueError(
                f"{self.header}:{number}: #error {rest} - the recorded preprocessor "
                "definitions selected an unsupported configuration")

    def _condition(self, name, rest, number):
        if name == "ifdef":
            return rest.split()[0] in self.macros if rest.split() else False
        if name == "ifndef":
            return rest.split()[0] not in self.macros if rest.split() else False
        expanded = self._expand_defined(rest)
        try:
            return bool(evaluate_constant(expanded, self._resolve_for_condition))
        except ValueError as error:
            raise ValueError(
                f"{self.header}:{number}: cannot evaluate #if {rest!r} ({error})") from error

    def _expand_defined(self, text):
        def replace(match):
            return "1" if (match.group(1) or match.group(2)) in self.macros else "0"

        return re.sub(r"defined[ \t]*\(([^)]*)\)|defined[ \t]+(\w+)", replace, text)

    def _resolve_for_condition(self, name, depth=0):
        # Undefined identifiers evaluate to zero, as the C preprocessor requires.
        if name not in self.macros or depth > 16:
            return 0
        value = self.macros[name].strip()
        if not value:
            return 0
        return ConstantExpression(
            tokenize(value), lambda inner: self._resolve_for_condition(inner, depth + 1)
        ).evaluate()

    def expand(self, tokens):
        """Replace object-like macro identifiers with their recorded expansions."""
        result = []
        for token in tokens:
            self._expand_token(token, result, 0)
        return result

    def _expand_token(self, token, result, depth):
        value = self.macros.get(token.value)
        if value is None or depth > 16 or token.value in BUILTIN_ALIASES:
            result.append(token)
            return
        for inner in tokenize(value, token.line):
            self._expand_token(inner, result, depth + 1)


def _strip_attributes(tokens):
    """Erase `__declspec(...)` groups wherever they appear.

    Removing them once, here, keeps every later consumer from having to know
    about them. Doing it per-consumer previously hid a whole class from
    `_has_body`, because `class NXF_DLL_EXPORT NxProfiler` reached the `(` of the
    expanded attribute before its `{`.
    """
    result = []
    index = 0
    while index < len(tokens):
        if tokens[index].value == "__declspec" and index + 1 < len(tokens) \
                and tokens[index + 1].value == "(":
            index = _matching(tokens, index + 1)
            continue
        result.append(tokens[index])
        index += 1
    return result


def _matching(tokens, index):
    """Return the index just past the bracket group that opens at index."""
    pairs = {"(": ")", "[": "]", "{": "}", "<": ">"}
    closer = pairs[tokens[index].value]
    opener = tokens[index].value
    depth = 0
    while index < len(tokens):
        value = tokens[index].value
        if value == opener:
            depth += 1
        elif value == closer:
            depth -= 1
            if depth == 0:
                return index + 1
        index += 1
    raise ValueError(f"unbalanced {opener!r}")


def _text_of(tokens):
    return " ".join(token.value for token in tokens)


class HeaderParser:
    """Extracts typedefs, enums, layout structs and exported prototypes."""

    def __init__(self, header, definitions, excluded_types=None):
        self.header = header
        self.preprocessor = Preprocessor(header, definitions)
        self.declarations = []
        # Maps an excluded type name to the reason it is withheld.
        self.excluded_types = dict(excluded_types or {})
        self.excluded_seen = set()

    def parse(self, text):
        tokens = _strip_attributes(
            self.preprocessor.expand(tokenize(self.preprocessor.run(text))))
        index = 0
        while index < len(tokens):
            index = self._top_level(tokens, index)
        return self.declarations

    def _top_level(self, tokens, index):
        value = tokens[index].value
        if value == ";":
            return index + 1
        if value == "template":
            # R6: templates are not instantiated; a member that names one resolves
            # through a recorded forced declaration instead.
            return self._skip_declaration(tokens, _matching(tokens, index + 1))
        if value == "typedef":
            return self._typedef(tokens, index)
        if value in ("class", "struct", "enum"):
            if self._has_body(tokens, index):
                return self._enum(tokens, index) if value == "enum" \
                    else self._record(tokens, index)
            if self._is_forward_declaration(tokens, index):
                return self._skip_declaration(tokens, index)
            # A tag declaration that is neither a definition nor a forward
            # declaration would otherwise be discarded whole, which is exactly
            # the silent skip R8 exists to prevent.
            raise ValueError(
                f"{self.header}:{tokens[index].line}: cannot parse {value} declaration "
                f"{_declaration_text(tokens[index:index + 6])!r}")
        if value == "extern" and index + 1 < len(tokens) and tokens[index + 1].value == '"C"':
            return self._exported_declaration(tokens, index)
        return self._skip_declaration(tokens, index)

    @staticmethod
    def _is_forward_declaration(tokens, index):
        return (index + 2 < len(tokens)
                and re.fullmatch(r"[A-Za-z_]\w*", tokens[index + 1].value)
                and tokens[index + 2].value == ";")

    @staticmethod
    def _has_body(tokens, index):
        scan = index + 1
        while scan < len(tokens) and tokens[scan].value not in (";", "{", "("):
            scan += 1
        return scan < len(tokens) and tokens[scan].value == "{"

    @staticmethod
    def _skip_declaration(tokens, index):
        """Advance past one top-level declaration or definition."""
        while index < len(tokens):
            value = tokens[index].value
            if value == ";":
                return index + 1
            if value == "{":
                index = _matching(tokens, index)
                if index < len(tokens) and tokens[index].value == ";":
                    index += 1
                return index
            index += 1
        return index

    def _typedef(self, tokens, index):
        end = index
        while end < len(tokens) and tokens[end].value != ";":
            if tokens[end].value == "{":
                end = _matching(tokens, end) - 1
            end += 1
        body = tokens[index + 1:end]
        if body and body[-1].value.replace("_", "").isalnum():
            name = body[-1].value
            line = tokens[index].line
            specifier = body[:-1]
            pointers = 0
            while specifier and specifier[-1].value in ("*", "&"):
                pointers += 1
                specifier = specifier[:-1]
            underlying = _normalize_type(specifier, self.header, line, name)
            reference = _type_reference(underlying)
            self.declarations.append(Declaration(
                "typedef", name, self.header, line,
                text=f"typedef {underlying} {'*' * pointers}{name};",
                # A pointer typedef needs only the forward declaration.
                dependencies=[reference] if reference and not pointers else (),
                references=[(underlying, name, line)] if reference else ()))
        return end + 1

    def _enum(self, tokens, index, name=None):
        name = name or tokens[index + 1].value
        open_brace = index + 2
        end = _matching(tokens, open_brace)
        entries = []
        values = {}
        next_value = 0
        cursor = open_brace + 1
        while cursor < end - 1:
            member = tokens[cursor].value
            cursor += 1
            if tokens[cursor].value == "=":
                cursor += 1
                start = cursor
                while cursor < end - 1 and tokens[cursor].value != ",":
                    cursor += 1
                expression = tokens[start:cursor]
                try:
                    next_value = ConstantExpression(
                        expression, lambda key: _lookup_enumerator(key, values)).evaluate()
                except ValueError as error:
                    raise ValueError(
                        f"{self.header}:{expression[0].line}: enumerator {member} has a "
                        f"non-constant initializer "
                        f"{''.join(token.value for token in expression)!r}") from error
            values[member] = next_value
            entries.append((member, next_value))
            next_value += 1
            if cursor < end - 1 and tokens[cursor].value == ",":
                cursor += 1
        # MSVC fixes an enum's underlying type at 4 bytes, but a C parser derives
        # the size from the value range, so a small enum would silently become
        # 1 or 2 bytes and move every member after it. The SDK pads the enums it
        # cared about; this pads the rest the same way.
        if entries and max(value for _, value in entries) < FORCE_32_BIT_VALUE:
            entries.append((f"{name}{FORCE_32_BIT_SUFFIX}", FORCE_32_BIT_VALUE))
        body = "\n".join(f"    {member} = {value}," for member, value in entries)
        text = f"enum {name} {{\n{body.rstrip(',')}\n}};"
        self.declarations.append(
            Declaration("enum", name, self.header, tokens[index].line, text))
        return end + 1 if end < len(tokens) and tokens[end].value == ";" else end

    def _withhold(self, name, line):
        """Record a withheld type and leave a marker where it would have appeared.

        The marker is what carries the exclusion into the generated header and,
        through ApplyPhysicsTypes.java, into the manifest; the inputs file is
        generator input that later phases never read.
        """
        self.excluded_seen.add(name)
        reason = "\n".join(f"   {text}" for text in _wrap(self.excluded_types[name], 86))
        self.declarations.append(Declaration(
            "excluded", name, self.header, line,
            text=f"/* excluded type: {name}\n{reason}\n*/"))

    def _base(self, clause, name, line):
        """Return the single base a tag declares, rejecting anything MSVC lays out
        by a rule this generator does not derive.

        Nested and top-level records share this, so a nested class cannot lose a
        base clause the top-level path would have rejected.
        """
        if not clause:
            return None
        if clause[0].value != ":":
            raise ValueError(
                f"{self.header}:{line}: cannot parse base clause of {name}: "
                f"{_text_of(clause)!r}")
        bases = [[]]
        virtual_base = False
        for token in clause[1:]:
            if token.value == "virtual":
                virtual_base = True
            elif token.value == ",":
                bases.append([])
            elif token.value not in ACCESS_SPECIFIERS:
                bases[-1].append(token)
        if virtual_base:
            raise ValueError(f"{self.header}:{line}: class {name} has a virtual base class")
        if len(bases) > 1:
            raise ValueError(
                f"{self.header}:{line}: class {name} has {len(bases)} base classes; "
                "only single inheritance is supported")
        return _text_of(bases[0]).replace(" ", "")

    def _record(self, tokens, index):
        name = tokens[index + 1].value
        if not re.fullmatch(r"[A-Za-z_]\w*", name):
            raise ValueError(
                f"{self.header}:{tokens[index].line}: {tokens[index].value} tag {name!r} "
                "is not an identifier")
        cursor = index + 2
        while tokens[cursor].value != "{":
            cursor += 1
        line = tokens[index].line
        end = _matching(tokens, cursor)
        if name in self.excluded_types:
            self._withhold(name, line)
            return end + 1 if end < len(tokens) and tokens[end].value == ";" else end
        base = self._base(tokens[index + 2:cursor], name, line)
        members, polymorphic = self._members(tokens, cursor + 1, end - 1, name)
        dependencies = _value_dependencies(members)
        references = _member_references(members)
        if base:
            dependencies.append(base)
            references.append((base, f"__base_{base}", line))
        self.declarations.append(Declaration(
            "struct", name, self.header, line,
            payload={"base": base, "members": members, "polymorphic": polymorphic},
            dependencies=dependencies, references=references))
        return end + 1 if end < len(tokens) and tokens[end].value == ";" else end

    def _members(self, tokens, start, stop, owner, aliases=None):
        members = []
        polymorphic = False
        aliases = {} if aliases is None else aliases
        cursor = start
        while cursor < stop:
            value = tokens[cursor].value
            if value in ACCESS_SPECIFIERS and tokens[cursor + 1].value == ":":
                cursor += 2
                continue
            if value == ";":
                cursor += 1
                continue
            declaration, cursor = self._one_member(tokens, cursor, stop)
            if declaration and declaration[0].value in ("class", "struct", "union", "enum") \
                    and any(token.value == "{" for token in declaration):
                members.extend(self._nested(declaration, owner, aliases))
                continue
            if declaration and declaration[0].value == "typedef":
                self._member_typedef(declaration, owner, aliases)
                continue
            # Only a declaration belonging to this class can make it polymorphic.
            # Scanning before the nested dispatch let a `virtual` inside a nested
            # type give its enclosing class a vftable pointer it does not have.
            if any(token.value == "virtual" for token in declaration):
                polymorphic = True
            members.extend(self._classify(declaration, owner, aliases))
        return members, polymorphic

    def _member_typedef(self, declaration, owner, aliases):
        """Record an in-class typedef; it declares no storage but does name a type."""
        body = declaration[1:]
        if len(body) < 2 or not (body[-1].value[0].isalpha() or body[-1].value[0] == "_"):
            raise ValueError(
                f"{self.header}:{declaration[0].line}: cannot parse typedef in {owner}: "
                f"{_declaration_text(declaration)!r}")
        spelling = _normalize_type(body[:-1], self.header, declaration[0].line, owner)
        aliases[body[-1].value] = aliases.get(spelling, spelling)

    def _nested(self, declaration, owner, aliases):
        """Hoist a tagged nested type, or inline an anonymous union or struct."""
        keyword = declaration[0].value
        opener = next(i for i, token in enumerate(declaration) if token.value == "{")
        closer = _matching(declaration, opener)
        tag = declaration[1].value if opener > 1 else None
        line = declaration[0].line
        trailing = declaration[closer:]

        if keyword == "enum":
            if tag is None:
                raise ValueError(
                    f"{self.header}:{line}: {owner} declares an anonymous enum; "
                    "an anonymous enum declares no storage the shim can name")
            self._enum(declaration, 0, name=f"{owner}_{tag}")
            aliases[tag] = f"{owner}_{tag}"
            return self._trailing_members(trailing, f"{owner}_{tag}", owner, line)

        if tag is not None and f"{owner}_{tag}" in self.excluded_types:
            self._withhold(f"{owner}_{tag}", line)
            if [token for token in trailing if token.value not in ("}", ";")]:
                raise ValueError(
                    f"{self.header}:{line}: excluded type {owner}_{tag} is declared with a "
                    "declarator, so its layout would still be needed")
            return []
        # A nested tag can carry a base clause too, so it goes through the same
        # guard the top-level path uses.
        base = self._base(declaration[2:opener], f"{owner}_{tag}", line) if tag else None
        members, polymorphic = self._members(declaration, opener + 1, closer - 1,
                                             f"{owner}_{tag}" if tag else owner, dict(aliases))
        if tag is None:
            # An anonymous union or struct occupies one slot in its owner; naming
            # that slot keeps the shim inside C89 without moving any member.
            declarators = [token.value for token in trailing if token.value not in (",", ";")]
            if len(declarators) > 1:
                raise ValueError(
                    f"{self.header}:{line}: {owner} declares an anonymous {keyword} with "
                    f"{len(declarators)} declarators; only one is supported")
            return [{"group": keyword, "members": members, "line": line,
                     "name": declarators[0] if declarators else None}]

        hoisted = f"{owner}_{tag}"
        dependencies = _value_dependencies(members)
        references = _member_references(members)
        if base:
            dependencies.append(base)
            references.append((base, f"__base_{base}", line))
        self.declarations.append(Declaration(
            "struct", hoisted, self.header, line,
            payload={"base": base, "members": members, "polymorphic": polymorphic},
            dependencies=dependencies, references=references))
        aliases[tag] = hoisted
        return self._trailing_members(trailing, hoisted, owner, line)

    def _trailing_members(self, trailing, spelling, owner, line):
        """Turn the declarators that follow a nested type body into members."""
        declarators = [token for token in trailing if token.value not in ("}", ";")]
        if not declarators:
            return []
        return _parse_declarators(declarators, spelling, self.header, owner, line)

    @staticmethod
    def _one_member(tokens, cursor, stop):
        start = cursor
        parameters = False
        while cursor < stop:
            value = tokens[cursor].value
            if value == ";":
                return tokens[start:cursor], cursor + 1
            if value == "{":
                end = _matching(tokens, cursor)
                if parameters:
                    # An inline function body ends its declaration; a nested type
                    # body may still be followed by declarators.
                    return tokens[start:end], end + 1 if end < stop \
                        and tokens[end].value == ";" else end
                cursor = end
                continue
            if value == "(":
                parameters = True
                cursor = _matching(tokens, cursor)
                continue
            if value == "[":
                cursor = _matching(tokens, cursor)
                continue
            cursor += 1
        return tokens[start:cursor], cursor

    def _classify(self, declaration, owner, aliases):
        """Return the storage a member declaration contributes, or reject it."""
        if not declaration:
            return []
        words = {token.value for token in declaration}
        if words & NON_STORAGE_MARKERS or "operator" in words:
            return []
        if any(token.value.startswith("~") for token in declaration):
            return []
        stripped = [token for token in declaration if token.value != "virtual"]
        opener = next((i for i, token in enumerate(stripped) if token.value == "("), None)
        if opener is not None:
            closer = _matching(stripped, opener)
            if closer < len(stripped) and stripped[closer].value == "(":
                raise ValueError(
                    f"{self.header}:{declaration[0].line}: cannot parse member declaration "
                    f"{_declaration_text(declaration)!r}")
            # A constructor or member function: named storage is not declared.
            return []
        if len(stripped) < 2:
            raise ValueError(
                f"{self.header}:{declaration[0].line}: cannot parse member declaration "
                f"{_declaration_text(declaration)!r}")
        return _parse_data_members(stripped, self.header, owner, aliases)

    def _exported_declaration(self, tokens, index):
        """Parse one `extern "C"` declaration: a function prototype or a variable."""
        end = index
        while end < len(tokens) and tokens[end].value != ";":
            if tokens[end].value == "(":
                end = _matching(tokens, end) - 1
            end += 1
        body = tokens[index + 2:end]
        opener = next((i for i, token in enumerate(body) if token.value == "("), None)
        if opener is None:
            # `extern "C" T name;` declares data, not a function, so it
            # contributes no prototype. Recognized by shape rather than by
            # falling off the end of the parser.
            name, _, _, type_tokens = _take_declarator(body)
            if name is None or not type_tokens:
                raise ValueError(
                    f"{self.header}:{tokens[index].line}: cannot parse extern \"C\" "
                    f"declaration {_declaration_text(body)!r}")
            return end + 1
        if opener < 2:
            raise ValueError(
                f"{self.header}:{tokens[index].line}: cannot parse extern \"C\" "
                f"declaration {_declaration_text(body)!r}")
        name = body[opener - 1].value
        convention = "__cdecl"
        head = body[:opener - 1]
        if head and head[-1].value.startswith("__"):
            convention = head[-1].value
            head = head[:-1]
        line = tokens[index].line
        return_pointers = 0
        while head and head[-1].value in ("*", "&"):
            return_pointers += 1
            head = head[:-1]
        return_type = _normalize_type(head, self.header, line, name)
        parameters = _parse_parameters(
            body[opener + 1:_matching(body, opener) - 1], self.header, name)
        references = [(return_type, "return value", line)] if _type_reference(return_type) else []
        references.extend((spelling, parameter, line)
                          for spelling, _, parameter, _ in parameters
                          if _type_reference(spelling))
        self.declarations.append(Declaration(
            "function", name, self.header, line,
            payload={"return_type": return_type, "return_pointers": return_pointers,
                     "convention": convention, "name": name, "parameters": parameters},
            references=references))
        return end + 1


def _word_like(text):
    return bool(text) and (text[-1].isalnum() or text[-1] == "_")


def _declaration_text(declaration):
    text = ""
    for token in declaration:
        if _word_like(text) and (_word_like(token.value) or token.value == "("):
            text += " "
        text += token.value
    return text + ";"


def _lookup_enumerator(name, values):
    if name in values:
        return values[name]
    raise ValueError(f"unknown enumerator {name}")


def _normalize_type(tokens, header, line, owner):
    """Render a type-specifier token run as a C type spelling."""
    words = [token.value for token in tokens if token.value not in QUALIFIERS]
    if not words:
        raise ValueError(f"{header}:{line}: {owner} has no type")
    if "<" in words:
        return "".join(words)
    words = [BUILTIN_ALIASES.get(word, word) for word in words]
    return " ".join(words)


def _split_top_level(tokens, separator=","):
    groups = [[]]
    depth = 0
    for token in tokens:
        if token.value in "([<":
            depth += 1
        elif token.value in ")]>":
            depth -= 1
        if token.value == separator and depth == 0:
            groups.append([])
            continue
        groups[-1].append(token)
    return groups


def _take_declarator(tokens):
    """Split a declarator's array dimensions, name and pointer depth off its type."""
    end = len(tokens)
    dimensions = ""
    while end > 0 and tokens[end - 1].value == "]":
        opener = end - 1
        depth = 0
        while opener >= 0:
            if tokens[opener].value == "]":
                depth += 1
            elif tokens[opener].value == "[":
                depth -= 1
                if depth == 0:
                    break
            opener -= 1
        dimensions = "[" + "".join(t.value for t in tokens[opener + 1:end - 1]) + "]" + dimensions
        end = opener
    if end == 0:
        return None, 0, "", tokens
    name = tokens[end - 1].value
    end -= 1
    pointers = 0
    while end > 0 and tokens[end - 1].value in ("*", "&"):
        pointers += 1
        end -= 1
    return name, pointers, dimensions, tokens[:end]


def _member(tokens, spelling, header, owner, line, allow_remainder=False):
    """Build one member from a declarator, given the type it shares."""
    name, pointers, dimensions, remainder = _take_declarator(tokens)
    if name is None or (remainder and not allow_remainder):
        raise ValueError(
            f"{header}:{line}: cannot parse member declaration of {owner}: "
            f"{_declaration_text(tokens)!r}")
    return {"spelling": spelling, "pointers": pointers, "name": name,
            "dimensions": dimensions, "line": line}


def _parse_data_members(tokens, header, owner, aliases=None):
    """Split one declaration's comma-separated declarators into members."""
    groups = _split_top_level(tokens)
    line = tokens[0].line
    _, _, _, type_tokens = _take_declarator(groups[0])
    if not type_tokens:
        raise ValueError(
            f"{header}:{line}: cannot parse member declaration "
            f"{_declaration_text(tokens)!r}")
    spelling = _normalize_type(type_tokens, header, line, owner)
    spelling = (aliases or {}).get(spelling, spelling)
    first = _member(groups[0], spelling, header, owner, line, allow_remainder=True)
    return [first] + [_member(group, spelling, header, owner, line) for group in groups[1:]]


def _parse_declarators(tokens, spelling, header, owner, line):
    return [_member(group, spelling, header, owner, line)
            for group in _split_top_level(tokens)]


def _walk_members(members):
    for member in members:
        if "group" in member:
            yield from _walk_members(member["members"])
        else:
            yield member


def _value_dependencies(members):
    # A by-value member must see a complete type, so it constrains emission
    # order; a pointer member is satisfied by the forward declaration block.
    return [member["spelling"] for member in _walk_members(members) if member["pointers"] == 0]


def _member_references(members):
    return [(member["spelling"], member["name"], member["line"])
            for member in _walk_members(members) if _type_reference(member["spelling"])]


def _parse_parameters(tokens, header, owner):
    parameters = []
    for position, group in enumerate(_split_top_level(tokens)):
        group = _split_top_level(group, "=")[0]
        if not group or _text_of(group) == "void":
            continue
        name, pointers, dimensions, type_tokens = _take_declarator(group)
        if not type_tokens:
            type_tokens, name, dimensions = group, f"param_{position}", ""
            pointers = 0
        spelling = _normalize_type(type_tokens, header, group[0].line, owner)
        parameters.append((spelling, pointers, name, dimensions))
    return parameters


def _wrap(text, width):
    lines = [""]
    for word in text.split():
        if lines[-1] and len(lines[-1]) + 1 + len(word) > width:
            lines.append("")
        lines[-1] += (" " if lines[-1] else "") + word
    return lines


def _type_reference(spelling):
    """Return the spelling when it names a declared type, or None when builtin."""
    if all(word in BUILTIN_KEYWORDS for word in spelling.split()):
        return None
    return spelling


def _declarator(spelling, pointers, name, dimensions, substitutions):
    return f"{substitutions.get(spelling, spelling)} {'*' * pointers}{name}{dimensions}"


def _render_members(members, substitutions, indent, anonymous):
    lines = []
    for member in members:
        if "group" in member:
            # A named slot for an anonymous union or struct keeps the shim inside
            # C89; the slot's offset and size are those of the group itself.
            name = member["name"] or f"__anon_{next(anonymous)}"
            lines.append(f"{indent}{member['group']} {{")
            lines.extend(_render_members(member["members"], substitutions,
                                         indent + "    ", anonymous))
            lines.append(f"{indent}}} {name};")
            continue
        lines.append(indent + _declarator(
            member["spelling"], member["pointers"], member["name"],
            member["dimensions"], substitutions) + ";")
    return lines


def _resolve_vftables(types):
    """Decide which structs carry their own vftable pointer.

    MSVC gives a class a new vfptr at offset 0 only when it introduces virtual
    functions and does not already inherit one. A class that introduces virtuals
    over a base with no vftable puts the pointer *before* the base subobject,
    which reorders the whole layout; that case is rejected rather than guessed.
    """
    structs = {declaration.name: declaration
               for declaration in types if declaration.kind == "struct"}

    def inherits_vftable(name, seen):
        declaration = structs.get(name)
        if declaration is None or name in seen:
            return None
        if declaration.payload["polymorphic"]:
            return True
        base = declaration.payload["base"]
        return False if base is None else inherits_vftable(base, seen | {name})

    for declaration in structs.values():
        base = declaration.payload["base"]
        if not declaration.payload["polymorphic"]:
            declaration.payload["vftable"] = False
            continue
        if base is None:
            declaration.payload["vftable"] = True
            continue
        if inherits_vftable(base, set()) is not True:
            raise ValueError(
                f"{declaration.header}:{declaration.line}: class {declaration.name} adds "
                f"virtual members over base {base}, which has no vftable of its own; MSVC "
                "would place the vftable pointer before the base subobject")
        declaration.payload["vftable"] = False


def _render_struct(name, payload, substitutions):
    lines = []
    if payload["vftable"]:
        lines.append("    void **__vftable;")
    if payload["base"]:
        base = substitutions.get(payload["base"], payload["base"])
        lines.append(f"    {base} __base_{base};")
    lines.extend(_render_members(payload["members"], substitutions, "    ", itertools.count()))
    body = "\n".join(lines) or "    char __empty;"
    return f"struct {name} {{\n{body}\n}};"


def _render_function(payload, substitutions):
    signature = ", ".join(
        _declarator(spelling, pointers, parameter, dimensions, substitutions)
        for spelling, pointers, parameter, dimensions in payload["parameters"]) or "void"
    returns = substitutions.get(payload["return_type"], payload["return_type"])
    return (f"{returns} {'*' * payload['return_pointers']}{payload['convention']} "
            f"{payload['name']}({signature});")


def _load_json(path, description):
    try:
        return json.loads(Path(path).read_text(encoding="utf-8"))
    except OSError as error:
        raise ValueError(f"cannot read {description}: {error}") from error
    except json.JSONDecodeError as error:
        raise ValueError(f"{description} is not valid JSON: {error}") from error


def _verify_manifest_file(path, expected, description):
    digest = hashlib.sha256(Path(path).read_bytes()).hexdigest()
    if digest != expected:
        raise ValueError(
            f"{Path(path).name} sha256 {digest} does not match the pinned {expected}")
    return _load_json(path, description)


def load_corpus(headers_root, inputs_path):
    """Return the pinned header files, verified byte for byte, in a stable order."""
    inputs = _load_json(inputs_path, "inputs file")
    for key in REQUIRED_INPUT_KEYS:
        if inputs.get(key) is None:
            raise ValueError(f"inputs file is missing {key!r}")
    if inputs["generator_version"] != GENERATOR_VERSION:
        raise ValueError(
            f"inputs declare generator_version {inputs['generator_version']} but this "
            f"generator is version {GENERATOR_VERSION}")

    base = Path(inputs_path).resolve().parent
    physics = _verify_manifest_file(
        base / inputs["physics_manifest"]["path"], inputs["physics_manifest"]["sha256"],
        "physics header manifest")
    foundation = _verify_manifest_file(
        base / inputs["foundation_manifest"]["path"], inputs["foundation_manifest"]["sha256"],
        "foundation header manifest")

    pinned = {}
    for entry in physics["files"]:
        pinned[f"Physics/include/{entry['path']}"] = entry["sha256"]
    for entry in foundation["files"]:
        if entry["path"].startswith("Foundation/include/"):
            pinned[entry["path"]] = entry["raw_oracle_sha256"]

    roots = tuple(inputs["include_roots"])
    excluded = {entry["path"] for entry in inputs["excluded_headers"]}
    root = Path(headers_root)
    corpus = []
    for relative in sorted(pinned):
        if not relative.startswith(roots) or not relative.endswith(".h"):
            continue
        path = root / relative
        if not path.is_file():
            raise ValueError(f"missing pinned header {relative}")
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        if digest != pinned[relative]:
            raise ValueError(
                f"{relative} sha256 {digest} does not match the pinned {pinned[relative]}")
        if relative in excluded:
            continue
        corpus.append((relative, path.read_text(encoding="utf-8", errors="strict")))
    unknown = sorted(excluded - set(pinned))
    if unknown:
        raise ValueError(f"excluded header {unknown[0]} is not pinned by either manifest")
    return inputs, corpus


def _order(declarations, known):
    """Emit each declaration after the declarations its definition depends on."""
    pending = sorted(declarations, key=lambda declaration: declaration.order_key)
    emitted = set()
    ordered = []
    while pending:
        ready = [d for d in pending
                 if all(dep in emitted or dep not in known for dep in d.dependencies)]
        if not ready:
            names = ", ".join(sorted(d.name for d in pending))
            raise ValueError(f"type definitions form a dependency cycle: {names}")
        chosen = ready[0]
        ordered.append(chosen)
        emitted.add(chosen.name)
        pending.remove(chosen)
    return ordered


def generate(headers_root, inputs_path) -> str:
    """Derive the ABI shim text from the pinned headers and recorded inputs."""
    inputs, corpus = load_corpus(headers_root, inputs_path)

    declarations = []
    origins = {}
    signatures = {}
    excluded_types = {entry["name"]: entry["reason"] for entry in inputs["excluded_types"]}
    excluded_seen = set()
    for relative, text in corpus:
        parser = HeaderParser(relative, inputs["preprocessor_definitions"], excluded_types)
        parsed = parser.parse(text)
        excluded_seen |= parser.excluded_seen
        for declaration in parsed:
            key = (declaration.kind == "function", declaration.name)
            if key in signatures:
                # Headers legitimately repeat a declaration verbatim; only a
                # repeat that says something different can move a member.
                if signatures[key] == declaration.signature:
                    continue
                first, second = sorted((origins[declaration.name], declaration.header))
                raise ValueError(
                    f"{declaration.name} is declared in both {first} and {second}")
            signatures[key] = declaration.signature
            if declaration.kind == "function":
                origins.setdefault(declaration.name, declaration.header)
            elif declaration.kind != "excluded":
                origins[declaration.name] = declaration.header
            declarations.append(declaration)

    stale = sorted(set(excluded_types) - excluded_seen)
    if stale:
        raise ValueError(f"excluded type {stale[0]} is not declared by any pinned header")

    forced = []
    for entry in inputs["forced_declarations"]:
        if entry["emit_as"] in origins:
            raise ValueError(
                f"forced declaration {entry['emit_as']!r} supplies a layout for a type "
                f"declared in {origins[entry['emit_as']]}")
        # The reason travels into the shim so its caveats reach Ghidra and Task 5
        # instead of living only in the inputs file.
        reason = "\n".join(f"   {line}" for line in _wrap(entry["reason"], 86))
        forced.append(Declaration(
            "forced", entry["emit_as"], "", 0,
            f"/* forced declaration: {entry['name']}\n{reason}\n*/\n{entry['declaration']}"))
    substitutions = {entry["name"].replace(" ", ""): entry["emit_as"]
                     for entry in inputs["forced_declarations"]}

    known = set(origins) | {entry.name for entry in forced}
    types = [d for d in declarations if d.kind not in ("function", "excluded")]
    functions = [d for d in declarations if d.kind == "function"]
    withheld = [d for d in declarations if d.kind == "excluded"]
    for declaration in types:
        declaration.dependencies = tuple(
            substitutions.get(dep, dep) for dep in declaration.dependencies)
    _check_reachability(types + functions, excluded_types, substitutions)
    _check_forced_declarations(forced, inputs["forced_declarations"], excluded_types)
    _check_resolvable(types + functions, known, substitutions)
    _resolve_vftables(types)

    structs = sorted(d.name for d in types if d.kind == "struct")
    lines = [
        "/* Generated by generate_ghidra_types.py; do not edit.",
        f"   generator_version {GENERATOR_VERSION}",
        f"   derived from {inputs['physics_manifest']['path']} and "
        f"{inputs['foundation_manifest']['path']},",
        "   whose sha256 physics_type_inputs.json pins and this generator verifies"
        " before parsing.",
        "   C++ spellings are replaced by layout-identical C ones: "
        + ", ".join(f"{key} -> {value}" for key, value in sorted(BUILTIN_ALIASES.items()))
        + ", references -> pointers.",
        "*/",
        "",
    ]
    lines.extend(f"typedef struct {name} {name};" for name in structs)
    lines.append("")
    for declaration in _order(types + forced + withheld, known):
        lines.append(declaration.render(substitutions))
        lines.append("")
    for declaration in sorted(functions, key=lambda d: d.order_key):
        lines.append(declaration.render(substitutions))
    return "\n".join(lines).rstrip("\n") + "\n"


def _check_reachability(declarations, excluded_types, substitutions):
    """Refuse to withhold a type anything emitted still needs.

    Every type that is not withheld is emitted, so a withheld type is reachable
    exactly when some emitted declaration names it - whether that is an export's
    signature, a vtable-bearing class, or a member of any other emitted type.
    Unreachability is a fact about today's corpus, not a property of the hatch,
    so it is checked rather than assumed.
    """
    for declaration in declarations:
        for spelling, member, line in declaration.references:
            resolved = substitutions.get(spelling, spelling)
            if resolved in excluded_types:
                raise ValueError(
                    f"{declaration.header}:{line}: excluded type {resolved} is reachable "
                    f"from {declaration.name}.{member}, so it cannot be withheld")


def _check_forced_declarations(forced, entries, excluded_types):
    """Close the one hole in the reachability bound.

    A forced declaration's body is hand-written text with no parsed references,
    so a withheld type named inside one would pass both the reachability and the
    resolvability checks. Its identifiers are scanned directly instead.
    """
    for declaration, entry in zip(forced, entries):
        for word in re.findall(r"[A-Za-z_]\w*", entry["declaration"]):
            if word in excluded_types:
                raise ValueError(
                    f"forced declaration {declaration.name!r} names excluded type {word}, "
                    "so that type cannot be withheld")


def _check_resolvable(declarations, known, substitutions):
    """Reject any member or parameter whose type no pinned header declares."""
    for declaration in declarations:
        for spelling, member, line in declaration.references:
            resolved = substitutions.get(spelling, spelling)
            if resolved in known:
                continue
            raise ValueError(
                f"{declaration.header}:{line}: member {member!r} of {declaration.name} "
                f"has unresolved type {resolved!r}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--headers", required=True, help="pinned Novodex header tree root")
    parser.add_argument("--inputs", required=True, help="physics_type_inputs.json to obey")
    parser.add_argument("--output", required=True, help="ABI shim header to write")
    args = parser.parse_args()

    try:
        text = generate(args.headers, args.inputs)
        output = Path(args.output)
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(text, encoding="utf-8", newline="\n")
    except (OSError, ValueError, KeyError) as error:
        parser.exit(2, f"error: {error}\n")

    print("types=written")
    print(f"enums={text.count(chr(10) + 'enum ')}")
    print(f"structs={text.count(chr(10) + 'struct ')}")
    print(f"functions={len([line for line in text.split(chr(10)) if line.endswith(');')])}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
