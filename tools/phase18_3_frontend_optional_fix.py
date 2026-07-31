from pathlib import Path

path = Path("webapp/src/features/settings/PackageOperationsPage.tsx")
text = path.read_text()
old = "    if (file === undefined || file === null) {\n      return;\n    }"
new = "    if (file === undefined) {\n      return;\n    }"
if text.count(old) != 1:
    raise SystemExit("replacement file guard anchor changed")
path.write_text(text.replace(old, new, 1))
