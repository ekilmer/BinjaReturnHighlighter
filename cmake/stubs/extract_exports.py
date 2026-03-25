"""Extract exported symbol names from MSVC object files and write a .def file.

Parses `dumpbin /SYMBOLS` output to find all externally-defined function
symbols, then writes a module-definition (.def) file that `lib /DEF` can
consume to create an import library (.lib) without building a real DLL.

Usage:
    dumpbin /SYMBOLS ui_stubs.obj > symbols.txt
    python extract_exports.py symbols.txt binaryninjaui.def binaryninjaui
"""

import argparse
import re
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("symbols_file", help="Output of dumpbin /SYMBOLS")
    parser.add_argument("def_file", help="Output .def file path")
    parser.add_argument("library_name", help="DLL name for LIBRARY directive")
    args = parser.parse_args()

    # Parse dumpbin /SYMBOLS output for externally-defined function symbols.
    # Format: "NNN XXXXXXXX SECT<n>  notype ()  External  | ?decorated_name"
    # We want symbols that are:
    #   - In a SECT (defined, not UNDEF)
    #   - External visibility
    #   - Function type: notype () — the () indicates function
    pattern = re.compile(
        r"^\s*[0-9A-F]+\s+[0-9A-F]+\s+SECT[0-9A-F]+\s+notype\s+\(\)\s+External\s+\|\s+(\S+)"
    )

    exports = []
    with open(args.symbols_file, encoding="utf-8", errors="replace") as f:
        for line in f:
            m = pattern.match(line)
            if m:
                symbol = m.group(1)
                # Skip compiler-generated symbols that start with __
                # but keep C++ mangled names that start with ?
                if symbol.startswith("?") or not symbol.startswith("_"):
                    exports.append(symbol)

    with open(args.def_file, "w", encoding="utf-8") as f:
        f.write(f"LIBRARY {args.library_name}\n")
        f.write("EXPORTS\n")
        for sym in sorted(exports):
            f.write(f"    {sym}\n")

    print(f"Extracted {len(exports)} exports -> {args.def_file}")


if __name__ == "__main__":
    main()
