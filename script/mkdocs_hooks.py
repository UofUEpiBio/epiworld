"""
MkDocs hook: expand the API Reference nav with mkdoxy-generated pages.

mkdoxy generates individual class, struct, namespace, file, and directory
markdown files (e.g. ``epiworld/classAgent.md``) but does not register them in
the MkDocs nav.
Without nav membership the Material-for-MkDocs theme cannot:

* activate the correct top tab ("API Reference") when a user lands on a
  generated API page
* show the relevant left-sidebar navigation tree

This hook fires after the nav is initially built (``on_nav``) and appends
sub-sections under the existing "API Reference" section, populated with
orphaned pages that mkdoxy produced.
"""

from __future__ import annotations

import re
from mkdocs.structure.nav import Section
from mkdocs.structure.pages import Page


# Matches stems like "classAgent", "structFoo" (capital letter after prefix).
_API_CLASS_RE = re.compile(r"^(class|struct)[A-Z]")
_API_CLASS_INDEXES = {
    "classes",
    "hierarchy",
    "class_members",
    "class_member_functions",
    "class_member_variables",
    "class_member_typedefs",
    "class_member_enums",
}
_API_NAMESPACE_RE = re.compile(r"^namespace(?!_member)")
_API_NAMESPACE_INDEXES = {
    "namespace_members",
    "namespace_member_functions",
    "namespace_member_variables",
    "namespace_member_typedefs",
    "namespace_member_enums",
}
_API_FILE_INDEXES = {
    "functions",
    "macros",
    "variables",
}


def on_nav(nav, *, config, files):
    """Inject mkdoxy generated pages into the API Reference nav section."""

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
    # 3. Gather orphaned mkdoxy pages from epiworld/.                     #
    # ------------------------------------------------------------------ #
    class_pages: list[Page] = []
    class_index_pages: list[Page] = []
    namespace_pages: list[Page] = []
    namespace_index_pages: list[Page] = []
    file_pages: list[Page] = []
    file_source_pages: list[Page] = []
    file_index_pages: list[Page] = []

    for f in sorted(files, key=lambda f: f.src_path):
        if not f.src_path.startswith("epiworld/") or f.src_path in in_nav:
            continue
        stem = f.src_path.removeprefix("epiworld/").removesuffix(".md")

        if _API_CLASS_RE.match(stem):
            class_pages.append(Page(_display_class(stem), f, config))
        elif stem in _API_CLASS_INDEXES:
            class_index_pages.append(Page(_display_title(stem), f, config))
        elif stem in _API_NAMESPACE_INDEXES:
            namespace_index_pages.append(Page(_display_title(stem), f, config))
        elif _API_NAMESPACE_RE.match(stem):
            namespace_pages.append(Page(_display_namespace(stem), f, config))
        elif stem in _API_FILE_INDEXES:
            file_index_pages.append(Page(_display_title(stem), f, config))
        elif stem.endswith("_source"):
            file_source_pages.append(Page(_display_file(stem), f, config))
        elif stem.startswith("dir_") or "_8" in stem:
            file_pages.append(Page(_display_file(stem), f, config))

    # ------------------------------------------------------------------ #
    # 4. Append generated-page sections under API Reference.              #
    #    MkDocs' _add_parent_links() runs before on_nav, so we must set   #
    #    .parent manually for every new nav node we create.               #
    # ------------------------------------------------------------------ #
    _append_section(api_section, "Class Indexes", class_index_pages)
    _append_section(api_section, "All Classes", class_pages)
    _append_section(api_section, "Namespace Indexes", namespace_index_pages)
    _append_section(api_section, "All Namespaces", namespace_pages)
    _append_section(api_section, "File Indexes", file_index_pages)
    _append_section(api_section, "All Files", file_pages)
    _append_section(api_section, "Source Files", file_source_pages)

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


def _append_section(parent: Section, title: str, pages: list[Page]) -> None:
    """Append a nav section and wire parent links for Material navigation."""
    if not pages:
        return

    section = Section(title, pages)
    section.parent = parent
    for page in pages:
        page.parent = section
    parent.children.append(section)


def _display_class(stem: str) -> str:
    """Return a readable label for mkdoxy class/struct refids."""
    return stem[6:] if stem.startswith("struct") else stem[5:]


def _display_namespace(stem: str) -> str:
    """Return a readable label for mkdoxy namespace refids."""
    name = stem.removeprefix("namespace")
    return name.replace("_1_1", "::") or stem


def _display_file(stem: str) -> str:
    """Return a readable label for mkdoxy file/dir refids."""
    source = stem.endswith("_source")
    stem = stem.removesuffix("_source")

    if stem.startswith("dir_"):
        display = f"Directory {stem.removeprefix('dir_')}"
    else:
        display = stem.replace("_2", "/")
        display = re.sub(r"_8([A-Za-z0-9]+)$", r".\1", display)

    return f"{display} source" if source else display


def _display_title(stem: str) -> str:
    """Return title case for mkdoxy index filenames."""
    return stem.replace("_", " ").title()
