"""Generate stub implementations for BINARYNINJAUIAPI-decorated C++ classes.

Parses Binary Ninja UI headers to extract class/struct method declarations
marked with BINARYNINJAUIAPI, then generates a C++ source file with empty
stub implementations. The resulting stub library satisfies the Windows linker
(__declspec(dllimport) requires an import library) while the real
libbinaryninjaui provides implementations at runtime.

Follows the same pattern as binaryninja-api/stubs/generate_stubs.py for the
core C API.

Usage:
    python generate_ui_stubs.py <ui_headers_dir> <output_dir>
"""

import argparse
import os
import re
import sys
from pathlib import Path


def strip_comments(text: str) -> str:
    """Remove C and C++ comments."""
    # Remove block comments (non-greedy)
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
    # Remove line comments
    text = re.sub(r"//[^\n]*", "", text)
    return text


def find_matching_brace(text: str, start: int) -> int:
    """Find the position after the matching closing brace."""
    depth = 1
    i = start
    while i < len(text) and depth > 0:
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
        i += 1
    return i


def extract_classes(text: str, header_name: str) -> list:
    """Find all BINARYNINJAUIAPI class/struct declarations and their bodies."""
    classes = []
    pattern = re.compile(
        r"(class|struct)\s+BINARYNINJAUIAPI\s+(\w+)[^{]*\{"
    )
    for match in pattern.finditer(text):
        kind = match.group(1)
        name = match.group(2)
        body_start = match.end()
        body_end = find_matching_brace(text, body_start)
        body = text[body_start : body_end - 1]
        classes.append((name, kind, body, header_name))
    return classes


def strip_default_args(params: str) -> str:
    """Remove default argument values from a parameter list.

    Handles nested templates, parentheses, and braces in default values.
    """
    result = []
    depth = 0  # Track <, (, { nesting
    skip = False
    i = 0
    while i < len(params):
        ch = params[i]
        if ch in "<({":
            depth += 1
            if not skip:
                result.append(ch)
        elif ch in ">)}":
            depth -= 1
            if not skip:
                result.append(ch)
        elif ch == "," and depth == 0:
            skip = False
            result.append(ch)
        elif ch == "=" and depth == 0:
            skip = True
        elif not skip:
            result.append(ch)
        i += 1
    return "".join(result)


def parse_method_declaration(decl: str, class_name: str) -> str | None:
    """Transform a method declaration into an out-of-line stub definition.

    Returns None if the declaration should be skipped.
    """
    decl = decl.strip().rstrip(";").strip()

    # Skip pure virtual, default, delete
    if re.search(r"=\s*(0|default|delete)\s*$", decl):
        return None

    # Skip friend, using, typedef, Q_ macros, enum, static_assert
    skip_prefixes = (
        "friend ",
        "using ",
        "typedef ",
        "Q_",
        "enum ",
        "static_assert",
        "template",
    )
    stripped = decl.lstrip()
    for prefix in skip_prefixes:
        if stripped.startswith(prefix):
            return None

    # Must have parentheses at template depth 0 (function declaration).
    # Parentheses inside <...> (e.g. std::function<void(int)>) are template
    # args, not method parameter lists.
    has_toplevel_paren = False
    angle_depth = 0
    for ch in decl:
        if ch == "<":
            angle_depth += 1
        elif ch == ">":
            angle_depth -= 1
        elif ch == "(" and angle_depth == 0:
            has_toplevel_paren = True
            break
    if not has_toplevel_paren:
        return None

    # Strip leading qualifiers
    clean = decl
    for keyword in ("virtual ", "explicit "):
        clean = re.sub(r"^\s*" + keyword, "", clean)

    # Detect static (needs different handling — not repeated in definition)
    is_static = clean.lstrip().startswith("static ")
    if is_static:
        clean = re.sub(r"^\s*static\s+", "", clean)

    # Detect conversion operators — no trailing return type needed
    conv_match = re.match(r"^\s*(operator\s+.+?)\s*\(", clean)
    if conv_match:
        # e.g. "operator BinaryNinja::PluginCommandContext() const"
        is_const = bool(re.search(r"\)\s*const\s*$", clean))
        const_suffix = " const" if is_const else ""
        return f"{class_name}::{conv_match.group(1)}(){const_suffix} {{ std::abort(); }}"

    # Detect override/final and strip them (preserving const)
    clean = re.sub(r"\)\s*(const\s*)?(override|final)(\s*(override|final))*\s*$",
                    lambda m: ")" + (" const" if m.group(1) else ""), clean)

    # Detect const method (trailing const after the closing paren)
    # Must be checked AFTER stripping override/final
    is_const = bool(re.search(r"\)\s*const\s*$", clean))

    # Split into return-type+name and params
    # Find the last '(' at angle-bracket depth 0 that starts the parameter list
    paren_depth = 0
    angle_depth = 0
    paren_start = -1
    for i in range(len(clean) - 1, -1, -1):
        if clean[i] == ">":
            angle_depth += 1
        elif clean[i] == "<":
            angle_depth -= 1
        elif angle_depth == 0:
            if clean[i] == ")":
                paren_depth += 1
            elif clean[i] == "(":
                paren_depth -= 1
                if paren_depth == 0:
                    paren_start = i
                    break

    if paren_start < 0:
        return None

    before_paren = clean[:paren_start].strip()
    params_with_parens = clean[paren_start:]

    # Extract the parameter list between ( and the matching )
    # params_with_parens may end with ') const' if the method is const
    close_paren = params_with_parens.rfind(")")
    inner_params = params_with_parens[1:close_paren]

    inner_params = strip_default_args(inner_params)
    const_suffix = " const" if is_const else ""
    params_clean = f"({inner_params.strip()}){const_suffix}"

    # Determine if this is a constructor or destructor
    is_destructor = before_paren.strip() == f"~{class_name}"
    is_constructor = before_paren.strip() == class_name

    # For operator overloads, the "method name" includes "operator..."
    # For regular methods, it's the last identifier before '('

    if is_constructor:
        return f"{class_name}::{class_name}{params_clean} {{ std::abort(); }}"
    elif is_destructor:
        return f"{class_name}::~{class_name}{params_clean} {{ std::abort(); }}"
    else:
        # Split before_paren into return_type and method_name
        # Method name is after the last space (but handle operator overloads)
        operator_match = re.search(r"^(.*?)\b(operator\s*\S+)\s*$", before_paren)
        if operator_match:
            return_type = operator_match.group(1).strip()
            method_name = operator_match.group(2).strip()
        else:
            # Last token is the method name
            parts = before_paren.rsplit(None, 1)
            if len(parts) < 2:
                return None  # Can't determine return type
            return_type, method_name = parts[0].strip(), parts[1].strip()

        # Check if void return
        is_void = return_type in ("void",)

        qualified_name = f"{class_name}::{method_name}"
        if is_void:
            return f"void {qualified_name}{params_clean} {{ }}"
        else:
            # Use trailing return type so unqualified nested types (e.g. Group)
            # resolve in the class scope.
            return (
                f"auto {qualified_name}{params_clean}"
                f" -> {return_type} {{ std::abort(); }}"
            )


def parse_class_methods(class_name: str, body: str) -> list[str]:
    """Extract method declarations from a class body and generate stubs."""
    stubs = []
    depth = 0
    current_decl = ""

    for line in body.split("\n"):
        stripped = line.strip()

        # Track brace depth for nested scopes (inner classes, inline bodies)
        open_braces = stripped.count("{")
        close_braces = stripped.count("}")

        if depth > 0:
            depth += open_braces - close_braces
            continue

        # Skip lines with braces — either unbalanced (entering a scope) or
        # balanced (inline definition like `void foo() { ... }`)
        if open_braces > 0 or close_braces > 0:
            depth += open_braces - close_braces
            current_decl = ""  # Discard any accumulated partial declaration
            continue

        # Skip access specifiers (including Qt slot/signal variants)
        if re.match(
            r"^(public|protected|private)(\s+(slots|Q_SLOTS|Q_SIGNALS))?\s*:|^(signals|Q_SIGNALS|Q_SLOTS|slots)\s*:",
            stripped,
        ):
            after = re.sub(
                r"^(public|protected|private)(\s+(slots|Q_SLOTS|Q_SIGNALS))?\s*:\s*|^(signals|Q_SIGNALS|Q_SLOTS|slots)\s*:\s*",
                "",
                stripped,
            )
            if after:
                stripped = after
            else:
                continue

        # Skip empty lines, preprocessor
        if not stripped or stripped.startswith("#"):
            continue

        # Accumulate multi-line declarations
        current_decl += " " + stripped

        if ";" in current_decl:
            # May contain multiple ;-separated parts — take the first
            decl = current_decl.split(";")[0].strip()
            current_decl = ""

            stub = parse_method_declaration(decl + ";", class_name)
            if stub is not None:
                stubs.append(stub)
        elif "{" in current_decl:
            # Inline definition — skip
            current_decl = ""

    return stubs


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("ui_headers_dir", help="Path to binaryninja-api/ui/")
    parser.add_argument("output_dir", help="Build directory for generated source")
    args = parser.parse_args()

    ui_dir = Path(args.ui_headers_dir)
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    output_file = output_dir / "ui_stubs.cpp"

    print(f"GENERATE UI STUBS: {ui_dir} -> {output_file}")
    print(f"{sys.executable} {' '.join(sys.argv)}")

    all_classes = []
    headers_with_classes = set()

    for header in sorted(ui_dir.glob("*.h")):
        content = header.read_text(encoding="utf-8")
        content = strip_comments(content)
        classes = extract_classes(content, header.name)
        if classes:
            all_classes.extend(classes)
            headers_with_classes.add(header.name)

    with open(output_file, "w", encoding="utf-8") as f:
        f.write(
            "// Auto-generated UI stubs -- do not edit\n"
            "// Generated by generate_ui_stubs.py\n\n"
            "#define BINARYNINJAUI_LIBRARY\n"
        )

        for header_name in sorted(headers_with_classes):
            f.write(f'#include "{header_name}"\n')

        f.write(
            "\n#undef BINARYNINJAUI_LIBRARY\n\n"
            "#include <cstdlib>\n\n"
        )

        total_stubs = 0
        for class_name, kind, body, header_name in all_classes:
            stubs = parse_class_methods(class_name, body)
            if stubs:
                f.write(f"// {kind} {class_name} ({header_name})\n")
                for stub in stubs:
                    f.write(f"{stub}\n")
                f.write("\n")
                total_stubs += len(stubs)

    print(f"Generated {total_stubs} stubs for {len(all_classes)} classes")


if __name__ == "__main__":
    main()
