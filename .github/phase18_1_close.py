from pathlib import Path

TODO_PATH = Path(
    "docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_TODO.md"
)
PROGRESS_PATH = Path(
    "docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_PROGRESS.md"
)
API_PATH = Path("docs/API.md")


def replace_once(text: str, old: str, new: str, description: str) -> str:
    if text.count(old) != 1:
        raise RuntimeError(f"expected one {description}, found {text.count(old)}")
    return text.replace(old, new, 1)


def update_todo() -> None:
    text = TODO_PATH.read_text(encoding="utf-8")
    replacements = (
        (
            "- [ ] enforce `APP_IMPORT_PACKAGE_MAX_BYTES`;",
            "- [x] enforce `APP_IMPORT_PACKAGE_MAX_BYTES`;",
        ),
        (
            "- [ ] stream or use bounded allocation;",
            "- [x] stream or use bounded allocation;",
        ),
        ("- [ ] reject trailing data;", "- [x] reject trailing data;"),
        (
            "- [ ] reject unknown schema fields;",
            "- [x] reject unknown schema fields;",
        ),
        (
            "- [ ] validate all objects before mutation.",
            "- [x] validate all objects before mutation.",
        ),
    )
    for old, new in replacements:
        text = replace_once(text, old, new, old)

    marker = "- [x] validate all objects before mutation.\n"
    evidence = """

Implemented: `storage_package_validate()` is a zero-copy package reader over the
bounded request buffer. It rejects packages above `APP_IMPORT_PACKAGE_MAX_BYTES`
before parsing; scans JSON with an iterative, depth-bounded state machine; allocates
only count-bounded validation metadata; requires the complete input to contain one
package document with no non-whitespace trailing bytes; rejects duplicate, unknown,
or future fields; and validates every set, macro, procedure, progress object, and
cross-object reference before returning success. The validation component has no
repository mutation dependency, so Phase 18.3 remains the first code permitted to
activate imported state.
"""
    text = replace_once(text, marker, marker + evidence, "Phase 18.1 evidence marker")
    TODO_PATH.write_text(text, encoding="utf-8")


def update_progress() -> None:
    text = PROGRESS_PATH.read_text(encoding="utf-8")
    text = replace_once(
        text,
        "| 18 | Import / export / backup / restore | in progress (§18.2 complete; §18.1 and §18.3–18.5 remain) |",
        "| 18 | Import / export / backup / restore | in progress (§18.1–18.2 complete; §18.3–18.5 remain) |",
        "Phase 18 progress row",
    )
    anchor = "## Completed tasks (commit evidence)\n\n"
    entry = """- Phase 18.1 bounded package reader — complete in `39b264e`, `974d339`,
  `e262137`, `3e03294`, `60856a8`, and `ce81a76`, with iterative parser hardening
  in `3670557`. `storage_package_validate()` enforces the import byte ceiling before
  parsing, scans without recursion, uses only count-bounded metadata allocations,
  rejects trailing data and unknown/duplicate fields, validates every object and
  reference, and remains structurally isolated from repository mutation APIs. The
  dedicated `tests/scripts/test-storage-package.sh` gate compiles and runs the
  production parser and its host regression matrix automatically.

"""
    text = replace_once(text, anchor, anchor + entry, "progress evidence anchor")
    PROGRESS_PATH.write_text(text, encoding="utf-8")


def update_api() -> None:
    text = API_PATH.read_text(encoding="utf-8")
    old = """Export and import currently return explicit `503 Service Unavailable`;
they cannot report false success before the Phase 18 package service exists.
"""
    new = """Set export returns the raw, validated Phase 18 package with its exact byte
length. Set import remains an explicit `503 Service Unavailable` boundary until
Phase 18.3 supplies transactional activation; the Phase 18.1 reader and validator
never mutate repository state.
"""
    text = replace_once(text, old, new, "stale export/import API text")
    API_PATH.write_text(text, encoding="utf-8")


update_todo()
update_progress()
update_api()
