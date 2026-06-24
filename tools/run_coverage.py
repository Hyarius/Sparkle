#!/usr/bin/env python3
"""Execute Sparkle's unit tests under coverage instrumentation and enforce thresholds."""

from __future__ import annotations

import argparse
import html as html_lib
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path
from typing import List, NamedTuple, Sequence


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
    input_list = output.with_suffix(".profraw-files.txt")
    input_list.write_text("\n".join(str(p) for p in profraw_files) + "\n", encoding="utf-8")

    command: List[str] = [
        str(llvm_profdata),
        "merge",
        "-sparse",
        f"--input-files={input_list}",
        "-o",
        str(output),
    ]

    try:
        _run(command)
    finally:
        try:
            input_list.unlink()
        except FileNotFoundError:
            pass


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


class _SourcePage(NamedTuple):
    path: str
    href: str
    fragment: str


def _read_source_page(output_dir: Path, page: Path, index: int) -> _SourcePage | None:
    content = page.read_text(encoding="utf-8")
    title_match = re.search(r"<div class='source-name-title'><pre>(.*?)</pre></div>", content, re.DOTALL)
    body_match = re.search(r"<body>(.*?)</body>", content, re.DOTALL)
    if not title_match or not body_match:
        return None

    body = body_match.group(1)
    source_start = body.find("<div class='centered'>")
    if source_start < 0:
        return None

    prefix = f"file-{index}"
    fragment = body[source_start:]
    fragment = re.sub(r"name='L(\d+)'", rf"name='{prefix}-L\1'", fragment)
    fragment = re.sub(r"href='#L(\d+)'", rf"href='#{prefix}-L\1'", fragment)

    return _SourcePage(
        path=html_lib.unescape(title_match.group(1)),
        href=page.relative_to(output_dir).as_posix(),
        fragment=fragment,
    )


def _find_directory_index(output_dir: Path) -> Path | None:
    index_files = [path for path in output_dir.rglob("index.html") if path != output_dir / "index.html"]
    if not index_files:
        return None
    return min(index_files, key=lambda path: len(path.relative_to(output_dir).parts))


def _write_consolidated_html_index(output_dir: Path, project_name: str) -> None:
    source_pages: List[_SourcePage] = []
    for index, page in enumerate(sorted(output_dir.rglob("*.html"))):
        if page.name == "index.html":
            continue
        source_page = _read_source_page(output_dir, page, index)
        if source_page:
            source_pages.append(source_page)

    if not source_pages:
        return

    source_pages.sort(key=lambda page: page.path.lower())
    directory_index = _find_directory_index(output_dir)
    directory_link = ""
    if directory_index:
        href = directory_index.relative_to(output_dir).as_posix()
        directory_link = f"<a href='{html_lib.escape(href, quote=True)}'>Open directory summary</a>"

    toc_rows = "\n".join(
        "<tr class='light-row'>"
        f"<td><pre><a href='#source-{index}'>{html_lib.escape(page.path)}</a></pre></td>"
        f"<td><pre><a href='{html_lib.escape(page.href, quote=True)}'>file page</a></pre></td>"
        "</tr>"
        for index, page in enumerate(source_pages)
    )

    source_sections = "\n".join(
        "<section class='source-file' "
        f"id='source-{index}'>"
        f"<h2>{html_lib.escape(page.path)}</h2>"
        f"<p><a href='{html_lib.escape(page.href, quote=True)}'>Open this file on its own</a></p>"
        f"{page.fragment}"
        "</section>"
        for index, page in enumerate(source_pages)
    )

    index_html = f"""<!doctype html>
<html>
<head>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<meta charset='UTF-8'>
<link rel='stylesheet' type='text/css' href='style.css'>
<script src='control.js'></script>
<style>
body {{ margin: 24px; }}
.report-links {{ margin-bottom: 1rem; }}
.source-file {{ margin-top: 3rem; }}
.source-file > h2 {{ font-size: 1.1rem; margin-bottom: 0.25rem; }}
.source-file > p {{ margin-top: 0; }}
.source-file .centered {{ max-width: 100%; overflow-x: auto; }}
.file-list {{ max-height: 24rem; overflow: auto; }}
</style>
</head>
<body>
<h1>{html_lib.escape(project_name)} Coverage</h1>
<h2>All Files</h2>
<p class='report-links'>{directory_link}</p>
<span class='control'><a href='javascript:next_line()'>next uncovered line (L)</a>, <a href='javascript:next_region()'>next uncovered region (R)</a>, <a href='javascript:next_branch()'>next uncovered branch (B)</a></span>
<div class='centered file-list'>
<table>
<tr><td class='column-entry-bold'>File</td><td class='column-entry-bold'>Original report</td></tr>
{toc_rows}
</table>
</div>
{source_sections}
<h5>Generated by llvm-cov with Sparkle consolidated index</h5>
</body>
</html>
"""
    (output_dir / "index.html").write_text(index_html, encoding="utf-8")


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

    _write_consolidated_html_index(output_dir, project_name)


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

    profraw_files = sorted(coverage_dir.glob("sparkle-*.profraw"))
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
