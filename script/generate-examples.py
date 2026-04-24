#!/usr/bin/env python3
"""
Generate docs/examples/*.md pages from the examples/ directory.

Run this script from the repository root:

    python3 script/generate-examples.py

Each qualifying example (a directory containing only main.cpp as a source
file, requiring only the epiworld single header to build) gets its own
documentation page with:

  - A title and description
  - The full source code in a syntax-highlighted block
  - An interactive Godbolt/Compiler Explorer playground (where supported)
"""

import os
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).parent.parent
EXAMPLES_DIR = REPO_ROOT / "examples"
DOCS_EXAMPLES_DIR = REPO_ROOT / "docs" / "examples"

# ---------------------------------------------------------------------------
# Metadata: title and description for each example directory.
# requires_omp=True examples show source code only (no interactive playground)
# because OpenMP is not enabled in the shared Godbolt integration.
# ---------------------------------------------------------------------------
EXAMPLE_META = {
    "00-hello-world": {
        "title": "Hello World",
        "description": (
            "Demonstrates the basic usage of epiworld by creating a custom model "
            "with custom states, a virus, and a tool on a small-world network."
        ),
    },
    "01-seir": {
        "title": "SEIR Model",
        "description": (
            "A basic SEIR (Susceptible-Exposed-Infected-Recovered) epidemic model "
            "using the built-in `ModelSEIR` class."
        ),
    },
    "01-sir": {
        "title": "SIR Model",
        "description": (
            "A classic SIR (Susceptible-Infected-Recovered) epidemic model "
            "using the built-in `ModelSIR` class on a small-world network."
        ),
    },
    "01-sis": {
        "title": "SIS Model",
        "description": (
            "A Susceptible-Infected-Susceptible (SIS) endemic model where "
            "recovered agents can be re-infected."
        ),
    },
    "02-sir_multiple_runs": {
        "title": "SIR: Multiple Runs",
        "description": (
            "Runs the SIR model multiple times using `run_multiple` and "
            "collects summary statistics across replicates."
        ),
    },
    "02b-sir_multiple_runs": {
        "title": "SIR: Multiple Runs with Plotting",
        "description": (
            "Extends the multiple-run SIR example with post-processing resources "
            "for R-based visualization of replicate results."
        ),
    },
    "03-simple-sir": {
        "title": "Simple SIR",
        "description": (
            "A minimal SIR simulation built step-by-step from scratch, "
            "illustrating state definitions, virus setup, and network generation."
        ),
    },
    "05-user-data": {
        "title": "User Data",
        "description": (
            "Shows how to attach arbitrary user-defined data to agents and "
            "retrieve it during and after the simulation."
        ),
    },
    "06-sir-omp": {
        "title": "SIR with OpenMP",
        "description": (
            "Runs multiple independent SIR simulations in parallel using OpenMP, "
            "demonstrating thread-level parallelism for large-scale studies."
        ),
        "requires_omp": True,
    },
    "06b-sir-omp": {
        "title": "SIR with OpenMP (Callbacks)",
        "description": (
            "Extends the OpenMP parallel SIR example with custom per-replicate "
            "callbacks for collecting results across threads."
        ),
        "requires_omp": True,
    },
    "07-surveillance": {
        "title": "Surveillance Model",
        "description": (
            "Uses the built-in `ModelSURV` surveillance model to simulate "
            "disease spread with active case detection and vaccination."
        ),
    },
    "10-likelihood-free-mcmc": {
        "title": "Likelihood-Free MCMC",
        "description": (
            "Demonstrates likelihood-free Markov Chain Monte Carlo (LFMCMC) "
            "for Bayesian parameter estimation of an SIR model."
        ),
    },
    "11-entities": {
        "title": "Entities",
        "description": (
            "Illustrates how to create structured populations using entities "
            "(e.g., groups, households) with contact matrices."
        ),
    },
    "13-genint": {
        "title": "Generation Interval",
        "description": (
            "Calculates and displays the generation interval and reproductive "
            "number for an SIR-like model."
        ),
    },
    "14-community-hosp": {
        "title": "Community and Hospital",
        "description": (
            "Models disease transmission in two linked populations — a community "
            "and a hospital — with admission and discharge events."
        ),
    },
    "15-seird-transitions": {
        "title": "SEIRD Transitions",
        "description": (
            "Builds a SEIRD model using the `new_state_update_transition` factory "
            "function instead of hand-written lambda update functions."
        ),
    },
    "16-sbm": {
        "title": "Stochastic Block Model",
        "description": (
            "Generates contact networks using the stochastic block model (SBM) "
            "and runs an SIR simulation on it."
        ),
    },
    "17-sbm-scalability": {
        "title": "SBM Scalability Benchmark",
        "description": (
            "Benchmarks `rgraph_sbm`, `rgraph_bernoulli`, and `rgraph_smallworld` "
            "across varying population sizes to compare network generation performance."
        ),
    },
    "18-seir-network-quarantine-benchmark": {
        "title": "SEIR Quarantine Benchmark",
        "description": (
            "Benchmarks the `ModelSEIRNetworkQuarantine` model with contact tracing "
            "and isolation for varying population sizes."
        ),
    },
    "19-poisson-approximation-binomial": {
        "title": "Poisson vs. Binomial Sampling",
        "description": (
            "Compares epiworld's `rbinom` and `rpoiss` random number generators "
            "to verify the Poisson approximation to the Binomial distribution."
        ),
    },
}


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


def generate_page(name: str, example_dir: Path) -> tuple:
    """Generate a docs page for one example.

    Returns (filename, markdown_content) or (None, None) if not in EXAMPLE_META.
    """
    meta = EXAMPLE_META.get(name)
    if meta is None:
        return None, None

    title = meta["title"]
    description = meta["description"]
    requires_omp = meta.get("requires_omp", False)

    raw_source = (example_dir / "main.cpp").read_text()
    playground_source = prepare_source_for_playground(raw_source)

    parts = []

    # YAML front matter
    parts.append(f"---\ntitle: {title}\n---\n\n")

    # Page heading and description
    parts.append(f"# {title}\n\n")
    parts.append(f"{description}\n\n")

    # OpenMP note (no playground)
    if requires_omp:
        parts.append(
            "!!! note\n"
            "    This example requires OpenMP support (`-fopenmp`). "
            "The interactive playground is not available for this example.\n\n"
        )

    # Source code block
    parts.append("## Source Code\n\n")
    parts.append(f"```cpp\n{playground_source}```\n\n")

    # Interactive playground (only for non-OpenMP examples)
    if not requires_omp:
        parts.append("## Try It Live\n\n")
        parts.append(
            "Edit the code below and click **▶ Run** to compile and execute it "
            "in your browser via [Compiler Explorer](https://godbolt.org). "
            "No local setup needed!\n\n"
        )
        parts.append(playground_html(name, playground_source))

    return f"{name}.md", "".join(parts)


def generate_index(examples: list) -> str:
    """Generate the examples/index.md overview page."""
    lines = [
        "---\ntitle: Examples\n---\n\n",
        "# Examples\n\n",
        "A collection of examples demonstrating epiworld's capabilities. "
        "Each example includes full source code and, where supported, an "
        "interactive playground powered by "
        "[Compiler Explorer](https://godbolt.org).\n\n",
        "| Example | Description |\n",
        "| ------- | ----------- |\n",
    ]
    for name in examples:
        meta = EXAMPLE_META.get(name)
        if meta is None:
            continue
        title = meta["title"]
        description = meta["description"]
        omp_badge = " *(requires OpenMP)*" if meta.get("requires_omp") else ""
        lines.append(f"| [{title}]({name}.md){omp_badge} | {description} |\n")
    return "".join(lines)


def main() -> int:
    DOCS_EXAMPLES_DIR.mkdir(parents=True, exist_ok=True)

    # Collect qualifying examples in alphabetical / numeric order
    examples = []
    for entry in sorted(EXAMPLES_DIR.iterdir(), key=lambda p: p.name):
        if not entry.is_dir() or entry.name.startswith("."):
            continue
        if not (entry / "main.cpp").exists():
            continue
        if not is_simple_example(entry):
            continue
        if entry.name in EXAMPLE_META:
            examples.append(entry.name)
        else:
            print(
                f"WARNING: {entry.name} has no metadata in EXAMPLE_META — skipping.",
                file=sys.stderr,
            )

    if not examples:
        print("ERROR: No qualifying examples found.", file=sys.stderr)
        return 1

    # Generate individual pages
    for ex_name in examples:
        ex_dir = EXAMPLES_DIR / ex_name
        filename, content = generate_page(ex_name, ex_dir)
        if filename and content:
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
