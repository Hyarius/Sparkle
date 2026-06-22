#!/usr/bin/env python3
"""Execute Sparkle's unit tests under coverage instrumentation and enforce thresholds."""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path
from typing import List, Sequence


def _parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mode", choices=["llvm"], required=True, help="Coverage backend to use.")
    parser.add_argument("--build-dir", required=True, help="CMake build directory where ctest runs.")
    parser.add_argument("--config", default="", help="CMake configuration (Debug/Release).")
    parser.add_argument("--threshold", type=float, default=0.0, help="Minimum accepted line coverage percentage.")
    parser.add_argument("--output", required=True, help="Path to write the human-readable summary.")
    parser.add_argument("--html-dir", help="Directory where the HTML coverage report will be generated.")
    parser.add_argument("--docs-dir", help="Directory where the HTML report will be copied for documentation.")
    parser.add_argument("--project-name", default="Sparkle", help="Project name to display inside the HTML report.")
    parser.add_argument("--binary", required=True, help="Path to the test executable.")
    parser.add_argument("--library", action="append", required=True, help="Library/object to include in coverage analysis.")
    parser.add_argument("--source-root", required=True, help="Project source root for reporting.")
    parser.add_argument("--llvm-profdata", required=True, help="Path to llvm-profdata executable.")
    parser.add_argument("--llvm-cov", required=True, help="Path to llvm-cov executable.")
    parser.add_argument(
        "--ignore-regex",
        action="append",
        default=[],
        help="Regular expression describing paths to ignore.",
    )
    return parser.parse_args()


def _run(command: Sequence[str], *, cwd: Path | None = None, env: dict[str, str] | None = None) -> subprocess.CompletedProcess:
    """Run a command with helpful logging."""
    print(f"[coverage] {' '.join(command)}", flush=True)
    return subprocess.run(command, cwd=cwd, env=env, check=True, capture_output=False)


def _clean_existing_profiles(profile_dir: Path, pattern: str) -> None:
    for file in profile_dir.glob(pattern):
        try:
            file.unlink()
        except OSError as exc:
            print(f"[coverage] Warning: unable to remove {file}: {exc}", file=sys.stderr)


def _run_ctests(build_dir: Path, config: str, env: dict[str, str]) -> None:
    ctest = shutil.which("ctest")
    if not ctest:
        raise RuntimeError("ctest executable not found in PATH.")

    command: List[str] = [ctest, "--output-on-failure"]
    cfg = config.strip()
    if "<" in cfg or ">" in cfg:
        cfg = ""
    if cfg and cfg != ".":
        command.extend(["-C", cfg])

    _run(command, cwd=build_dir, env=env)


def _merge_profiles(llvm_profdata: Path, profraw_files: Sequence[Path], output: Path) -> None:
    command: List[str] = [str(llvm_profdata), "merge", "-sparse"]
    command.extend(str(p) for p in profraw_files)
    command.extend(["-o", str(output)])
    _run(command)


def _export_summary(llvm_cov: Path, binary: Path, libraries: Sequence[Path], profdata: Path, ignore_patterns: Sequence[str]) -> dict:
    command: List[str] = [
        str(llvm_cov),
        "export",
        "--summary-only",
        str(binary),
        f"--instr-profile={profdata}",
    ]

    for library in libraries:
        command.extend(["--object", str(library)])

    for pattern in ignore_patterns:
        command.extend(["--ignore-filename-regex", pattern])

    print(f"[coverage] {' '.join(command)}", flush=True)
    try:
        completed = subprocess.run(command, check=True, text=True, capture_output=True)
    except subprocess.CalledProcessError as exc:
        print("[coverage] llvm-cov export failed.", file=sys.stderr)
        if exc.stdout:
            print(exc.stdout, file=sys.stderr)
        if exc.stderr:
            print(exc.stderr, file=sys.stderr)
        raise

    return json.loads(completed.stdout)


def _generate_html_report(
    llvm_cov: Path,
    binary: Path,
    libraries: Sequence[Path],
    profdata: Path,
    ignore_patterns: Sequence[str],
    output_dir: Path,
    project_name: str,
) -> None:
    if output_dir.exists():
        shutil.rmtree(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    command: List[str] = [
        str(llvm_cov),
        "show",
        str(binary),
        f"--instr-profile={profdata}",
        "--format=html",
        f"--output-dir={output_dir}",
        f"--project-title={project_name} Coverage",
        "--show-regions",
        "--show-region-summary",
        "--show-branch-summary",
        "--show-directory-coverage",
    ]

    for library in libraries:
        command.extend(["--object", str(library)])

    for pattern in ignore_patterns:
        command.extend(["--ignore-filename-regex", pattern])

    print(f"[coverage] {' '.join(command)}", flush=True)
    try:
        subprocess.run(command, check=True, text=True, capture_output=True)
    except subprocess.CalledProcessError as exc:
        print("[coverage] llvm-cov show failed.", file=sys.stderr)
        if exc.stdout:
            print(exc.stdout, file=sys.stderr)
        if exc.stderr:
            print(exc.stderr, file=sys.stderr)
        raise


def _write_report(output_path: Path, line_pct: float, func_pct: float, region_pct: float) -> None:
    lines = [
        "Sparkle coverage summary",
        f"Line coverage    : {line_pct:.2f}%",
        f"Function coverage: {func_pct:.2f}%",
        f"Region coverage  : {region_pct:.2f}%",
    ]
    output_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    args = _parse_args()

    build_dir = Path(args.build_dir).resolve()
    source_root = Path(args.source_root).resolve()
    coverage_dir = Path(args.output).resolve().parent
    coverage_dir.mkdir(parents=True, exist_ok=True)

    binary = Path(args.binary).resolve()
    libraries = [Path(lib).resolve() for lib in args.library]
    llvm_profdata = Path(args.llvm_profdata).resolve()
    llvm_cov = Path(args.llvm_cov).resolve()
    html_dir = Path(args.html_dir).resolve() if args.html_dir else None
    docs_dir = Path(args.docs_dir).resolve() if args.docs_dir else None

    prof_pattern = coverage_dir / "sparkle-%p.profraw"
    _clean_existing_profiles(coverage_dir, "sparkle-*.profraw")

    env = os.environ.copy()
    env["LLVM_PROFILE_FILE"] = str(prof_pattern)
    env["SPARKLE_SOURCE_ROOT"] = str(source_root)

    _run_ctests(build_dir, args.config, env)

    profraw_files = list(coverage_dir.glob("sparkle-*.profraw"))
    if not profraw_files:
        print("[coverage] No .profraw files were generated; check instrumentation settings.", file=sys.stderr)
        return 1

    profdata_path = coverage_dir / "sparkle.profdata"
    if profdata_path.exists():
        profdata_path.unlink()

    _merge_profiles(llvm_profdata, profraw_files, profdata_path)

    summary = _export_summary(llvm_cov, binary, libraries, profdata_path, args.ignore_regex)

    data_root = summary["data"][0]["totals"]
    line_pct = float(data_root["lines"]["percent"])
    func_pct = float(data_root["functions"]["percent"])
    region_pct = float(data_root["regions"]["percent"])

    print(f"[coverage] Line coverage    : {line_pct:.2f}%")
    print(f"[coverage] Function coverage: {func_pct:.2f}%")
    print(f"[coverage] Region coverage  : {region_pct:.2f}%")

    _write_report(Path(args.output), line_pct, func_pct, region_pct)

    if html_dir:
        _generate_html_report(
            llvm_cov,
            binary,
            libraries,
            profdata_path,
            args.ignore_regex,
            html_dir,
            args.project_name,
        )
        print(f"[coverage] HTML report generated at: {html_dir}")

        if docs_dir:
            if docs_dir.exists():
                shutil.rmtree(docs_dir)
            docs_dir.mkdir(parents=True, exist_ok=True)
            for item in html_dir.iterdir():
                target = docs_dir / item.name
                if item.is_dir():
                    shutil.copytree(item, target)
                else:
                    shutil.copy2(item, target)
            print(f"[coverage] Documentation copy stored at: {docs_dir}")
    elif docs_dir:
        print("[coverage] docs-dir specified without html-dir; skipping docs copy.", file=sys.stderr)

    threshold = float(args.threshold)
    if threshold > 0.0 and line_pct + 1e-6 < threshold:
        print(
            f"[coverage] Coverage threshold violation: {line_pct:.2f}% < required {threshold:.2f}%",
            file=sys.stderr,
        )
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
