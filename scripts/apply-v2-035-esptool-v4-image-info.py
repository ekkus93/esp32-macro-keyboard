#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def replace_once(path: str, old: str, new: str) -> None:
    target = ROOT / path
    text = target.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{path}: expected one match, found {count}: {old!r}")
    target.write_text(text.replace(old, new, 1), encoding="utf-8")


replace_once(
    "scripts/generate-flash-manifest.sh",
    'image_info="$(esptool.py image_info "${app_image}")"',
    'image_info="$(esptool.py image_info --version 2 "${app_image}")"',
)

replace_once(
    "scripts/run-v2-035-hardware.py",
    '[esptool, "image_info", str(app_image)],',
    '[esptool, "image_info", "--version", "2", str(app_image)],',
)

replace_once(
    "tests/scripts/fakes/esptool.py",
    '''if [ "${1:-}" = "image_info" ] && [ -n "${2:-}" ]; then
\tprintf 'Application Information\\n'
\tprintf 'ELF file SHA256: %s\\n' "${FAKE_ELF_SHA256:-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa}"
\texit 0
fi
''',
    '''if [ "$#" -eq 4 ] && [ "${1:-}" = "image_info" ] && \\
\t[ "${2:-}" = "--version" ] && [ "${3:-}" = "2" ] && [ -n "${4:-}" ]; then
\tprintf 'Application Information\\n'
\tprintf 'ELF file SHA256: %s\\n' "${FAKE_ELF_SHA256:-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa}"
\texit 0
fi
''',
)

replace_once(
    "tests/scripts/test-v2-035-hardware.py",
    '''        MODULE.shutil.which = lambda name: "/fake/esptool.py" if name == "esptool.py" else None
        MODULE.subprocess.run = lambda *args, **kwargs: subprocess.CompletedProcess(
            args=args[0], returncode=0, stdout=f"ELF file SHA256: {'a' * 64}\\n", stderr=""
        )
        try:
            manifest = MODULE.load_flash_manifest(manifest_path)
''',
    '''        MODULE.shutil.which = lambda name: "/fake/esptool.py" if name == "esptool.py" else None
        esptool_commands: list[list[str]] = []

        def fake_run(command, **kwargs):
            assert command == [
                "/fake/esptool.py",
                "image_info",
                "--version",
                "2",
                str(app_image),
            ]
            assert kwargs == {
                "check": False,
                "capture_output": True,
                "text": True,
            }
            esptool_commands.append(command)
            return subprocess.CompletedProcess(
                args=command,
                returncode=0,
                stdout=f"ELF file SHA256: {'a' * 64}\\n",
                stderr="",
            )

        MODULE.subprocess.run = fake_run
        try:
            manifest = MODULE.load_flash_manifest(manifest_path)
            assert len(esptool_commands) == 1
''',
)

print("V2-035 esptool v4 image-info fix applied")
