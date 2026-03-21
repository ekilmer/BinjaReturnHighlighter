#!/usr/bin/env python3
"""Update the binaryninja-api submodule to match the installed Binary Ninja."""

import argparse
import platform
import subprocess
import sys
from pathlib import Path

REVISION_FILENAME = "api_REVISION.txt"
SUBMODULE_REL_PATH = Path("external", "binaryninja-api")


def default_install_paths() -> list[Path]:
    system = platform.system()
    if system == "Darwin":
        return [Path("/Applications/Binary Ninja.app")]
    if system == "Linux":
        return [Path.home() / "binaryninja"]
    if system == "Windows":
        return [Path(r"C:\Program Files\Vector35\BinaryNinja")]
    return []


def find_revision_file(install_path: Path) -> Path:
    # macOS app bundle stores resources in Contents/Resources/
    candidates = [
        install_path / "Contents" / "Resources" / REVISION_FILENAME,
        install_path / REVISION_FILENAME,
    ]
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    raise FileNotFoundError(
        f"Could not find {REVISION_FILENAME} in {install_path}\n"
        f"Searched: {', '.join(str(c) for c in candidates)}"
    )


def read_api_commit(revision_file: Path) -> str:
    lines = revision_file.read_text().strip().splitlines()
    if not lines:
        raise ValueError(f"{revision_file} is empty")
    commit = lines[-1].strip().lower()
    if len(commit) != 40 or not all(c in "0123456789abcdef" for c in commit):
        raise ValueError(f"Invalid commit hash in {revision_file}: {commit}")
    return commit


def git(*args: str, cwd: Path | None = None) -> str:
    result = subprocess.run(
        ["git", *args],
        cwd=cwd,
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        raise RuntimeError(
            f"git {' '.join(args)} failed (exit {result.returncode}):\n{result.stderr}"
        )
    return result.stdout.strip()


def get_tag_for_commit(commit: str, cwd: Path) -> str | None:
    output = git("tag", "--points-at", commit, cwd=cwd)
    if not output:
        return None
    return output.splitlines()[0]


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Update binaryninja-api submodule to match installed Binary Ninja."
    )
    parser.add_argument(
        "path",
        nargs="?",
        type=Path,
        help="Path to Binary Ninja installation root (auto-detected if omitted)",
    )
    args = parser.parse_args()

    if args.path:
        install_path = args.path.expanduser().resolve()
        if not install_path.is_dir():
            print(f"Error: {install_path} is not a directory", file=sys.stderr)
            return 1
    else:
        candidates = default_install_paths()
        install_path = None
        for candidate in candidates:
            if candidate.is_dir():
                install_path = candidate
                break
        if install_path is None:
            print(
                "Error: Could not find Binary Ninja installation.\n"
                f"Searched: {', '.join(str(c) for c in candidates)}\n"
                "Specify the path explicitly as a positional argument.",
                file=sys.stderr,
            )
            return 1

    print(f"Binary Ninja install: {install_path}")

    try:
        revision_file = find_revision_file(install_path)
    except FileNotFoundError as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1

    try:
        target_commit = read_api_commit(revision_file)
    except ValueError as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1

    print(f"Target API commit:   {target_commit}")

    repo_root = Path(git("rev-parse", "--show-toplevel"))
    submodule_path = repo_root / SUBMODULE_REL_PATH
    if not (submodule_path / ".git").exists():
        print(f"Error: Submodule not found at {submodule_path}", file=sys.stderr)
        return 1

    current_commit = git("rev-parse", "HEAD", cwd=submodule_path)
    print(f"Current API commit:  {current_commit}")

    if current_commit == target_commit:
        tag = get_tag_for_commit(target_commit, submodule_path)
        if tag:
            print(f"API tag:             {tag}")
        print("Already up to date.")
        return 0

    print("Fetching from origin...")
    git("fetch", "origin", cwd=submodule_path)

    print(f"Checking out {target_commit[:12]}...")
    git("checkout", target_commit, cwd=submodule_path)

    print("Updating vendor/fmt submodule...")
    git("submodule", "update", "--init", "vendor/fmt", cwd=submodule_path)

    tag = get_tag_for_commit(target_commit, submodule_path)
    if tag:
        print(f"API tag:             {tag}")
    print(f"Updated: {current_commit[:12]} -> {target_commit[:12]}")
    label = tag if tag else target_commit[:12]
    print(f"\nCommit message:      Bump binaryninja-api to {label}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
