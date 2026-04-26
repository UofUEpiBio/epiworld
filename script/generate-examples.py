#!/usr/bin/env python3
"""
Generate docs/examples/*.md pages from the examples/ directory.

Run this script from the repository root:

    uv run python script/generate-examples.py

Each qualifying example (a directory containing only main.cpp as a source
file, requiring only the epiworld single header to build, and including a
README.md file) gets its own documentation page with:

  - A title and prose copied from the example README.md
  - An interactive Godbolt/Compiler Explorer playground where supported
"""

import re
import sys
from dataclasses import dataclass
from pathlib import Path

REPO_ROOT = Path(__file__).parent.parent
EXAMPLES_DIR = REPO_ROOT / "examples"
DOCS_EXAMPLES_DIR = REPO_ROOT / "docs" / "examples"
GITHUB_EXAMPLES_URL = "https://github.com/UofUEpiBio/epiworld/tree/master/examples"

FRONT_MATTER_RE = re.compile(
    r"\A---[ \t]*\n(?P<body>.*?)\n---[ \t]*(?:\n|\Z)",
    re.DOTALL,
)
H1_RE = re.compile(r"^# (?P<title>.+?)\s*$", re.MULTILINE)
REQUIRES_OMP_RE = re.compile(
    r"^requires_omp:\s*(?:true|yes|1)\s*$",
    re.IGNORECASE | re.MULTILINE,
)


@dataclass(frozen=True)
class ExampleDoc:
    name: str
    title: str
    body: str
    front_matter: str
    requires_omp: bool


def read_example_doc(example_dir: Path) -> ExampleDoc | None:
    """Read title, body, and page metadata from an example README."""
    readme_path = example_dir / "README.md"
    if not readme_path.exists():
        print(
            f"WARNING: {example_dir.name} has no README.md metadata — skipping.",
            file=sys.stderr,
        )
        return None

    raw = readme_path.read_text(encoding="utf-8")
    front_matter = ""
    content = raw

    front_matter_match = FRONT_MATTER_RE.match(raw)
    if front_matter_match:
        front_matter = front_matter_match.group("body").strip()
        content = raw[front_matter_match.end() :]

    heading_match = H1_RE.search(content)
    if not heading_match:
        print(
            f"WARNING: {readme_path.relative_to(REPO_ROOT)} has no H1 title — skipping.",
            file=sys.stderr,
        )
        return None

    title = heading_match.group("title").strip()
    body = content[heading_match.end() :].strip()
    requires_omp = bool(REQUIRES_OMP_RE.search(front_matter))

    return ExampleDoc(
        name=example_dir.name,
        title=title,
        body=body,
        front_matter=front_matter,
        requires_omp=requires_omp,
    )


def is_simple_example(example_dir: Path) -> bool:
    """Return True if the example has exactly one .cpp file (main.cpp)."""
    cpp_files = list(example_dir.glob("*.cpp"))
    return len(cpp_files) == 1 and (example_dir / "main.cpp").exists()


def prepare_source_for_playground(source: str) -> str:
    """Transform the source code for use in the Godbolt playground.

    - Replaces the relative include path with the single-header include.
    - Removes any extra epiworld sub-header includes (already in epiworld.hpp).
    """
    # Replace the relative include path
    source = source.replace(
        '#include "../../include/epiworld/epiworld.hpp"',
        '#include "epiworld.hpp"',
    )
    # Remove separate lfmcmc include (already bundled in epiworld.hpp)
    source = re.sub(
        r'#include\s+"../../include/epiworld/math/lfmcmc\.hpp"\s*\n',
        "",
        source,
    )
    return source


def playground_html(name: str, source: str) -> str:
    """Return the HTML block for an interactive playground."""
    # The textarea content does not need HTML escaping for <> because it is
    # inside a <textarea> element which browsers treat as raw text.  However,
    # bare ampersands can occasionally cause issues in some Markdown renderers,
    # so we escape & → &amp; only.
    safe_source = source.replace("&", "&amp;")
    # Guard against pathological C++ source that contains literal </textarea>
    # (unlikely, but would break the enclosing element if unescaped).
    safe_source = safe_source.replace("</textarea>", "&lt;/textarea&gt;")
    return (
        f'<div class="epiworld-playground" id="{name}-playground">\n'
        '  <div class="playground-toolbar">\n'
        '    <span class="playground-label">🧪 Interactive Example</span>\n'
        '    <span class="playground-status"></span>\n'
        '    <button class="playground-btn playground-reset">↺ Reset</button>\n'
        '    <button class="playground-btn playground-run">▶ Run</button>\n'
        "  </div>\n"
        f'  <textarea class="playground-editor" spellcheck="false">{safe_source}</textarea>\n'
        '  <div class="playground-output" style="display:none;">\n'
        '    <div class="playground-output-header">Output</div>\n'
        '    <pre class="playground-output-content"></pre>\n'
        "  </div>\n"
        "</div>\n"
    )


def page_front_matter(example: ExampleDoc) -> str:
    """Return MkDocs front matter for a generated example page."""
    lines = ["---", f"title: {example.title}"]
    if example.front_matter:
        lines.append(example.front_matter)
    lines.append("---")
    return "\n".join(lines) + "\n\n"


def first_paragraph(markdown: str) -> str:
    """Return a compact first paragraph for the examples index."""
    paragraphs = [p.strip() for p in re.split(r"\n\s*\n", markdown) if p.strip()]
    if not paragraphs:
        return ""
    return " ".join(paragraphs[0].split())


def generate_page(example: ExampleDoc, example_dir: Path) -> tuple[str, str]:
    """Generate a docs page for one example."""
    parts = []

    parts.append(page_front_matter(example))
    parts.append(f"# {example.title}\n\n")
    if example.body:
        parts.append(f"{example.body}\n\n")
    parts.append(
        f"[View this example on GitHub]({GITHUB_EXAMPLES_URL}/{example.name}).\n\n"
    )

    if example.requires_omp:
        parts.append(
            "!!! note\n"
            "    This example requires OpenMP support (`-fopenmp`). "
            "The interactive playground is not available for this example.\n\n"
        )
    else:
        raw_source = (example_dir / "main.cpp").read_text(encoding="utf-8")
        playground_source = prepare_source_for_playground(raw_source)
        parts.append("## Try It Live\n\n")
        parts.append(
            "Edit the code below and click **▶ Run** to compile and execute it "
            "in your browser via [Compiler Explorer](https://godbolt.org). "
            "No local setup needed!\n\n"
        )
        parts.append(playground_html(example.name, playground_source))

    return f"{example.name}.md", "".join(parts)


def generate_index(examples: list[ExampleDoc]) -> str:
    """Generate the examples/index.md overview page."""
    lines = [
        "---\ntitle: Examples\n---\n\n",
        "# Examples\n\n",
        "A collection of examples demonstrating epiworld's capabilities. "
        "Examples include an interactive playground powered by "
        "[Compiler Explorer](https://godbolt.org).\n\n",
        "| Example | Description |\n",
        "| ------- | ----------- |\n",
    ]
    for example in examples:
        description = first_paragraph(example.body)
        omp_badge = " *(requires OpenMP)*" if example.requires_omp else ""
        lines.append(
            f"| [{example.title}]({example.name}.md){omp_badge} | {description} |\n"
        )
    return "".join(lines)


def main() -> int:
    DOCS_EXAMPLES_DIR.mkdir(parents=True, exist_ok=True)

    # Collect qualifying examples in alphabetical / numeric order.
    examples: list[ExampleDoc] = []
    for entry in sorted(EXAMPLES_DIR.iterdir(), key=lambda p: p.name):
        if not entry.is_dir() or entry.name.startswith("."):
            continue
        if not (entry / "main.cpp").exists():
            continue
        if not is_simple_example(entry):
            continue
        example = read_example_doc(entry)
        if example is not None:
            examples.append(example)

    if not examples:
        print("ERROR: No qualifying examples found.", file=sys.stderr)
        return 1

    # Generate individual pages
    for example in examples:
        ex_dir = EXAMPLES_DIR / example.name
        filename, content = generate_page(example, ex_dir)
        out_path = DOCS_EXAMPLES_DIR / filename
        out_path.write_text(content, encoding="utf-8")
        print(f"  Generated {out_path.relative_to(REPO_ROOT)}")

    # Generate index page
    index_content = generate_index(examples)
    index_path = DOCS_EXAMPLES_DIR / "index.md"
    index_path.write_text(index_content, encoding="utf-8")
    print(f"  Generated {index_path.relative_to(REPO_ROOT)}")

    print(f"\nDone: generated {len(examples)} example pages + index.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
