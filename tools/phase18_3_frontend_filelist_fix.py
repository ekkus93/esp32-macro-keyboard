from pathlib import Path

path = Path("webapp/src/features/settings/PackageOperationsPage.tsx")
text = path.read_text()
old = "    const file = event.target.files?.item(0);"
new = "    const file = event.target.files?.[0];"
if text.count(old) != 1:
    raise SystemExit("replacement FileList access anchor changed")
path.write_text(text.replace(old, new, 1))
