"""
MkDocs hook: expand the API Reference nav with mkdoxy-generated class pages.

mkdoxy generates individual class/struct markdown files (e.g.
``epiworld/classAgent.md``) but does not register them in the MkDocs nav.
Without nav membership the Material-for-MkDocs theme cannot:

* activate the correct top tab ("API Reference") when a user lands on a
  class page
* show the relevant left-sidebar navigation tree

This hook fires after the nav is initially built (``on_nav``) and appends an
"All Classes" sub-section under the existing "API Reference" section,
populated with every orphaned ``epiworld/class*.md`` and
``epiworld/struct*.md`` file that mkdoxy produced.
"""

from __future__ import annotations

import re
from mkdocs.structure.nav import Section
from mkdocs.structure.pages import Page


# Matches stems like "classAgent", "structFoo" (capital letter after prefix).
_API_CLASS_RE = re.compile(r"^(class|struct)[A-Z]")


def on_nav(nav, *, config, files):
    """Inject mkdoxy class/struct pages into the API Reference nav section."""

    # ------------------------------------------------------------------ #
    # 1. Collect file paths already present in the nav (no duplicates).   #
    # ------------------------------------------------------------------ #
    in_nav: set[str] = set()
    for item in _walk(nav.items):
        if isinstance(item, Page) and item.file:
            in_nav.add(item.file.src_path)

    # ------------------------------------------------------------------ #
    # 2. Locate the top-level "API Reference" section.                    #
    # ------------------------------------------------------------------ #
    api_section: Section | None = next(
        (
            s
            for s in nav.items
            if isinstance(s, Section) and s.title == "API Reference"
        ),
        None,
    )
    if api_section is None:
        return nav  # Nothing to do if the section is absent.

    # ------------------------------------------------------------------ #
    # 3. Gather orphaned class / struct pages from epiworld/.             #
    # ------------------------------------------------------------------ #
    class_pages: list[Page] = []
    for f in sorted(files, key=lambda f: f.src_path):
        if not f.src_path.startswith("epiworld/") or f.src_path in in_nav:
            continue
        stem = f.src_path.removeprefix("epiworld/").removesuffix(".md")
        if not _API_CLASS_RE.match(stem):
            continue
        # Strip "class" (5 chars) or "struct" (6 chars) prefix for display.
        display = stem[6:] if stem.startswith("struct") else stem[5:]
        class_pages.append(Page(display, f, config))

    if not class_pages:
        return nav  # mkdoxy not run yet or no classes found.

    # ------------------------------------------------------------------ #
    # 4. Append "All Classes" as a new child of API Reference.            #
    #    MkDocs' _add_parent_links() runs before on_nav, so we must set   #
    #    .parent manually for every new nav node we create.               #
    # ------------------------------------------------------------------ #
    all_classes = Section("All Classes", class_pages)
    all_classes.parent = api_section
    for page in class_pages:
        page.parent = all_classes
    api_section.children.append(all_classes)

    return nav


# --------------------------------------------------------------------------- #
# Helper                                                                       #
# --------------------------------------------------------------------------- #

def _walk(items):
    """Recursively yield every item in the nav tree."""
    for item in items:
        yield item
        if hasattr(item, "children") and item.children:
            yield from _walk(item.children)
