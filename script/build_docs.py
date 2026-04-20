#!/usr/bin/env python3

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
import textwrap
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DOCS_DIR = ROOT / "docs"
SITE_DIR = ROOT / "site"
BUILD_DIR = ROOT / "build" / "docs"
MRDOCS_CONFIG = DOCS_DIR / "mrdocs.yml"
MRDOCS_OVERRIDES = DOCS_DIR / "mrdocs-overrides"
MODELS_SOURCE = DOCS_DIR / "models" / "index.md"

MRDOCS_WRAPPER = MRDOCS_OVERRIDES / "generator" / "html" / "layouts" / "wrapper.html.hbs"


@dataclass(frozen=True)
class Page:
    title: str
    source: Path
    output: Path
    section: str


PAGES = [
    Page("Home", DOCS_DIR / "README.md", Path("index.html"), "home"),
    Page("Models", DOCS_DIR / "models" / "index.md", Path("models/index.html"), "models"),
    Page("API Overview", DOCS_DIR / "api" / "index.md", Path("api/index.html"), "api"),
    Page("Implementation", DOCS_DIR / "impl" / "index.md", Path("impl/index.html"), "impl"),
    Page(
        "Library Architecture",
        DOCS_DIR / "impl" / "library-architecture.md",
        Path("impl/library-architecture.html"),
        "impl",
    ),
    Page(
        "Performance Optimization",
        DOCS_DIR / "impl" / "performance-optimization.md",
        Path("impl/performance-optimization.html"),
        "impl",
    ),
    Page(
        "Extending the Library",
        DOCS_DIR / "impl" / "extending-the-library.md",
        Path("impl/extending-the-library.html"),
        "impl",
    ),
    Page(
        "Extending the Library with Models",
        DOCS_DIR / "impl" / "extending-the-library-with-models.md",
        Path("impl/extending-the-library-with-models.html"),
        "impl",
    ),
    Page(
        "Core Simulation Models",
        DOCS_DIR / "impl" / "core-simulation-models.md",
        Path("impl/core-simulation-models.html"),
        "impl",
    ),
    Page(
        "Queueing System",
        DOCS_DIR / "impl" / "queueing-system.md",
        Path("impl/queueing-system.html"),
        "impl",
    ),
    Page(
        "Data Collection and Analysis",
        DOCS_DIR / "impl" / "data-collection-and-analysis.md",
        Path("impl/data-collection-and-analysis.html"),
        "impl",
    ),
    Page(
        "Random Number Generation and Statistical Functions",
        DOCS_DIR / "impl" / "random-number-generation.md",
        Path("impl/random-number-generation.html"),
        "impl",
    ),
    Page(
        "Reproducibility and run_multiple",
        DOCS_DIR / "impl" / "reproducibility-and-run-multiple.md",
        Path("impl/reproducibility-and-run-multiple.html"),
        "impl",
    ),
    Page(
        "Mixing and Entity Distribution",
        DOCS_DIR / "impl" / "mixing-and-entity-distribution.md",
        Path("impl/mixing-and-entity-distribution.html"),
        "impl",
    ),
    Page(
        "Virus and Tool Distribution Functions",
        DOCS_DIR / "impl" / "virus-and-tool-distribution.md",
        Path("impl/virus-and-tool-distribution.html"),
        "impl",
    ),
    Page(
        "Generation Interval and Reproductive Number Calculation",
        DOCS_DIR / "impl" / "generation-interval-and-reproductive-number.md",
        Path("impl/generation-interval-and-reproductive-number.html"),
        "impl",
    ),
    Page(
        "Events and Multi-Event Handling",
        DOCS_DIR / "impl" / "events-and-multi-event-handling.md",
        Path("impl/events-and-multi-event-handling.html"),
        "impl",
    ),
    Page(
        "Quarantine, Isolation, and Contact Tracing",
        DOCS_DIR / "impl" / "quarantine-isolation-and-contact-tracing.md",
        Path("impl/quarantine-isolation-and-contact-tracing.html"),
        "impl",
    ),
    Page(
        "Sampling Contacts",
        DOCS_DIR / "impl" / "sampling-contacts.md",
        Path("impl/sampling-contacts.html"),
        "impl",
    ),
]

SOURCE_TO_OUTPUT = {page.source.resolve(): page.output for page in PAGES}
IMPL_PAGES = [page for page in PAGES if page.section == "impl" and page.source.name != "index.md"]


def run(cmd: list[str], *, cwd: Path | None = None, input_text: str | None = None) -> str:
    result = subprocess.run(
        cmd,
        cwd=cwd or ROOT,
        input=input_text,
        text=True,
        capture_output=True,
        check=False,
    )
    if result.returncode != 0:
        sys.stderr.write(result.stdout)
        sys.stderr.write(result.stderr)
        raise SystemExit(f"command failed: {' '.join(cmd)}")
    return result.stdout


def require_command(name: str) -> str:
    found = shutil.which(name)
    if not found:
        raise SystemExit(f"required command not found in PATH: {name}")
    return found


def parse_markdown(source: Path) -> tuple[dict, str]:
    raw = source.read_text(encoding="utf-8")
    if raw.startswith("---\n"):
        parts = raw.split("---\n", 2)
        if len(parts) == 3:
            _, frontmatter, body = parts
            metadata: dict[str, str] = {}
            for line in frontmatter.splitlines():
                if ":" not in line:
                    continue
                key, value = line.split(":", 1)
                metadata[key.strip()] = value.strip()
            return metadata, body.lstrip()
    return {}, raw


def infer_title(markdown: str, fallback: str) -> str:
    for line in markdown.splitlines():
        if line.startswith("# "):
            return line[2:].strip()
    return fallback


def relative_href(current_output: Path, target_output: Path) -> str:
    return os.path.relpath(target_output, current_output.parent).replace(os.sep, "/")


def rewrite_relative_links(markdown: str, source: Path, output: Path) -> str:
    link_pattern = re.compile(r"(?P<prefix>\]\()(?P<target>[^)#?]+?\.md)(?P<suffix>[#?][^)]+)?\)")
    html_pattern = re.compile(r'(?P<attr>href=")(?P<target>[^"#?]+?\.md)(?P<suffix>[#?][^"]+)?(")')

    def replace_md(match: re.Match[str]) -> str:
        target = match.group("target")
        suffix = match.group("suffix") or ""
        resolved = (source.parent / target).resolve()
        output_target = SOURCE_TO_OUTPUT.get(resolved)
        if not output_target:
            return match.group(0)
        return f"{match.group('prefix')}{relative_href(output, output_target)}{suffix})"

    def replace_html(match: re.Match[str]) -> str:
        target = match.group("target")
        suffix = match.group("suffix") or ""
        resolved = (source.parent / target).resolve()
        output_target = SOURCE_TO_OUTPUT.get(resolved)
        if not output_target:
            return match.group(0)
        return f'{match.group("attr")}{relative_href(output, output_target)}{suffix}"'

    markdown = link_pattern.sub(replace_md, markdown)
    markdown = html_pattern.sub(replace_html, markdown)
    return markdown


def nav_items(current_output: Path, current_section: str) -> str:
    def item(label: str, href: str, *, active: bool = False) -> str:
        cls = ' class="is-active"' if active else ""
        return f'<li{cls}><a href="{href}">{label}</a></li>'

    top = [
        ("Home", Path("index.html"), current_section == "home"),
        ("Models", Path("models/index.html"), current_section == "models"),
        ("API Reference", Path("api/index.html"), current_section == "api"),
        ("Implementation", Path("impl/index.html"), current_section == "impl"),
    ]
    top_list = "".join(item(label, relative_href(current_output, target), active=active) for label, target, active in top)

    api_list = "".join(
        [
            item("Overview", relative_href(current_output, Path("api/index.html")), active=current_output == Path("api/index.html")),
            item("Generated Reference", relative_href(current_output, Path("api/reference/index.html"))),
        ]
    )

    impl_list = "".join(
        item(page.title, relative_href(current_output, page.output), active=current_output == page.output) for page in IMPL_PAGES
    )

    return textwrap.dedent(
        f"""
        <nav class="site-sidebar-nav" aria-label="Documentation">
          <div class="site-sidebar-group">
            <p class="site-sidebar-heading">Sections</p>
            <ul>{top_list}</ul>
          </div>
          <div class="site-sidebar-group">
            <p class="site-sidebar-heading">API</p>
            <ul>{api_list}</ul>
          </div>
          <div class="site-sidebar-group">
            <p class="site-sidebar-heading">Implementation</p>
            <ul>{impl_list}</ul>
          </div>
        </nav>
        """
    ).strip()


def render_shell(*, page_title: str, body: str, output: Path, section: str) -> str:
    assets = lambda target: relative_href(output, Path(target))
    body_class = "page-home" if output == Path("index.html") else f"page-{section}"
    sidebar = nav_items(output, section)

    return textwrap.dedent(
        f"""\
        <!doctype html>
        <html lang="en">
        <head>
          <meta charset="utf-8">
          <meta name="viewport" content="width=device-width, initial-scale=1">
          <title>{page_title} | Epiworld</title>
          <meta name="description" content="Documentation for the epiworld C++ simulation framework.">
          <link rel="icon" href="{assets('assets/branding/epiworld.png')}">
          <link rel="stylesheet" href="{assets('stylesheets/extra.css')}">
          <script defer src="{assets('javascripts/site.js')}"></script>
          <script defer src="{assets('javascripts/mathjax.js')}"></script>
          <script defer src="{assets('javascripts/network-hero.js')}"></script>
          <script defer src="{assets('javascripts/compiler.js')}"></script>
          <script defer src="https://unpkg.com/mathjax@3/es5/tex-mml-chtml.js"></script>
        </head>
        <body class="epiworld-docs {body_class}">
          <header class="site-header">
            <div class="site-header-inner">
              <a class="site-brand" href="{assets('index.html')}">
                <img src="{assets('assets/branding/epiworld.png')}" alt="Epiworld logo">
                <span>Epiworld Docs</span>
              </a>
              <button class="site-nav-toggle" type="button" aria-expanded="false" aria-controls="site-sidebar">Menu</button>
              <nav class="site-topnav" aria-label="Primary">
                <a href="{assets('index.html')}">Home</a>
                <a href="{assets('models/index.html')}">Models</a>
                <a href="{assets('api/index.html')}">API</a>
                <a href="{assets('impl/index.html')}">Implementation</a>
                <a href="https://github.com/UofUEpiBio/epiworld">GitHub</a>
              </nav>
            </div>
          </header>
          <div class="site-layout">
            <aside class="site-sidebar" id="site-sidebar">
              {sidebar}
            </aside>
            <main class="site-main">
              <article class="content-card">
                {body}
              </article>
            </main>
          </div>
          <footer class="site-footer">
            <div class="site-footer-inner">
              <p>Built with MrDocs for the C++ reference and a lightweight static site pipeline for the authored guides.</p>
              <a href="https://github.com/UofUEpiBio/epiworld">UofUEpiBio/epiworld</a>
            </div>
          </footer>
        </body>
        </html>
        """
    )


def convert_markdown(page: Page) -> None:
    metadata, body = parse_markdown(page.source)
    rewritten = rewrite_relative_links(body, page.source, page.output)
    title = metadata.get("title") or infer_title(rewritten, page.title)
    fragment = run(
        [
            "pandoc",
            "--from=gfm+yaml_metadata_block+definition_lists+pipe_tables+raw_html+raw_attribute",
            "--to=html5",
            "--mathjax",
        ],
        input_text=rewritten,
    )
    output_path = SITE_DIR / page.output
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(render_shell(page_title=title, body=fragment, output=page.output, section=page.section), encoding="utf-8")


def copy_tree(source: Path, destination: Path) -> None:
    if not source.exists():
        return
    shutil.copytree(source, destination, dirs_exist_ok=True)


def build_static_site() -> None:
    SITE_DIR.mkdir(parents=True, exist_ok=True)
    copy_tree(DOCS_DIR / "assets", SITE_DIR / "assets")
    copy_tree(DOCS_DIR / "stylesheets", SITE_DIR / "stylesheets")
    copy_tree(DOCS_DIR / "javascripts", SITE_DIR / "javascripts")
    for page in PAGES:
        convert_markdown(page)


def prepare_compile_database(cxx: str) -> Path:
    build_input = BUILD_DIR / "mrdocs-input"
    build_input.mkdir(parents=True, exist_ok=True)

    headers = sorted((ROOT / "include" / "epiworld").rglob("*.hpp"))
    if not headers:
        raise SystemExit("no headers found under include/epiworld")

    include_file = build_input / "all_headers.cpp"
    include_lines = ['// Generated for MrDocs\n']
    for header in headers:
        rel = header.relative_to(ROOT / "include").as_posix()
        include_lines.append(f'#include "{rel}"')
    include_file.write_text("\n".join(include_lines) + "\n", encoding="utf-8")

    command = " ".join(
        [
            cxx,
            "-std=c++20",
            f"-I{(ROOT / 'include').as_posix()}",
            f"-I{ROOT.as_posix()}",
            "-c",
            include_file.as_posix(),
        ]
    )

    compile_commands = build_input / "compile_commands.json"
    compile_commands.write_text(
        textwrap.dedent(
            f"""\
            [
              {{
                "directory": "{ROOT.as_posix()}",
                "command": "{command}",
                "file": "{include_file.as_posix()}"
              }}
            ]
            """
        ),
        encoding="utf-8",
    )
    return compile_commands


def locate_default_addons(mrdocs: Path) -> Path:
    candidates = [
        mrdocs.resolve().parent.parent / "share" / "mrdocs" / "addons",
        mrdocs.resolve().parent / ".." / "share" / "mrdocs" / "addons",
    ]
    for candidate in candidates:
        candidate = candidate.resolve()
        if candidate.exists():
            return candidate
    raise SystemExit(f"unable to locate MrDocs addons next to {mrdocs}")


def prepare_addons(mrdocs: Path) -> Path:
    addons_target = BUILD_DIR / "mrdocs-addons"
    if addons_target.exists():
        shutil.rmtree(addons_target)
    copy_tree(locate_default_addons(mrdocs), addons_target)
    wrapper_target = addons_target / "generator" / "html" / "layouts" / "wrapper.html.hbs"
    wrapper_target.parent.mkdir(parents=True, exist_ok=True)
    wrapper_target.write_text(MRDOCS_WRAPPER.read_text(encoding="utf-8"), encoding="utf-8")
    return addons_target


def build_api_reference(mrdocs_exe: str, cxx: str) -> None:
    compile_database = prepare_compile_database(cxx)
    addons = prepare_addons(Path(mrdocs_exe))
    output = SITE_DIR / "api" / "reference"
    output.mkdir(parents=True, exist_ok=True)

    run(
        [
            mrdocs_exe,
            MRDOCS_CONFIG.as_posix(),
            f"--compilation-database={compile_database.as_posix()}",
            f"--output={output.as_posix()}",
            f"--addons={addons.as_posix()}",
        ]
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Build the epiworld documentation site.")
    parser.add_argument("--skip-api", action="store_true", help="Skip the MrDocs API generation step.")
    parser.add_argument("--mrdocs", default=shutil.which("mrdocs"), help="Path to the mrdocs executable.")
    parser.add_argument("--cxx", default=os.environ.get("CXX", shutil.which("clang++") or shutil.which("c++")), help="C++ compiler used in the synthetic compile_commands.json.")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    require_command("pandoc")

    if SITE_DIR.exists():
        shutil.rmtree(SITE_DIR)
    BUILD_DIR.mkdir(parents=True, exist_ok=True)

    build_static_site()

    if not args.skip_api:
        if not args.mrdocs:
            raise SystemExit("mrdocs was not found in PATH. Install MrDocs or rerun with --skip-api.")
        if not args.cxx:
            raise SystemExit("no C++ compiler found. Set CXX or install clang++.")
        build_api_reference(args.mrdocs, args.cxx)


if __name__ == "__main__":
    main()
