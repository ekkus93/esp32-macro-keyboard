from pathlib import Path

paths = [
    Path("docs/TODO_V2.md"),
    Path("docs/ESP32_MACRO_KEYBOARD_POST_V2_CORRECTNESS_HARDENING_TODO_2026-08-10.md"),
]

for path in paths:
    text = path.read_text(encoding="utf-8")
    while "\n\n\n" in text:
        text = text.replace("\n\n\n", "\n\n")
    if path.name.startswith("ESP32_MACRO_KEYBOARD_POST_V2"):
        text = text.replace(
            "\n---\n## Phase H3 — Crash-safe, resumable factory reset",
            "\n---\n\n## Phase H3 — Crash-safe, resumable factory reset",
        )
        text = text.replace(
            "recoverability.\n- Evidence: initial mechanical audit",
            "recoverability.\n\n- Evidence: initial mechanical audit",
        )
    path.write_text(text, encoding="utf-8")
