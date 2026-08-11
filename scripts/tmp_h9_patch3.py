from pathlib import Path

path = Path("webapp/tests/v2-browser-storage-prohibition.test.tsx")
text = path.read_text()
old = " * and every `src/features/**/v2/` production feature directory. This includes\n"
new = " * and every production V2 feature directory under `src/features/`. This includes\n"
if text.count(old) != 1:
    raise SystemExit(f"frontend audit comment anchor count: {text.count(old)}")
path.write_text(text.replace(old, new, 1))
