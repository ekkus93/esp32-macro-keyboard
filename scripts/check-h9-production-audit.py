#!/usr/bin/env python3
"""Fail closed on newly introduced H9 silent/fallback patterns.

H9 is a classification audit, so the important regression property is that a
new ignored-result/fallback marker cannot silently join the production tree.
Known reviewed occurrences are an exact, count-bounded allowlist. Any wording
or statement change intentionally forces a fresh classification.
"""

from __future__ import annotations

import argparse
from collections import Counter
from pathlib import Path
import re
import sys

REPO_ROOT = Path(__file__).resolve().parents[1]
SOURCE_SUFFIXES = {".c", ".h", ".ts", ".tsx"}
PRODUCTION_ROOTS = (
    Path("firmware/components"),
    Path("firmware/main"),
    Path("webapp/src"),
)

EMPTY_CATCH = re.compile(r"\bcatch\s*(?:\([^)]*\))?\s*\{\s*\}", re.DOTALL)
EMPTY_PROMISE_CATCH = re.compile(
    r"\.catch\s*\(\s*\(\s*\)\s*=>\s*(?:\{\s*\}|undefined|null)\s*\)",
    re.DOTALL,
)
BEST_EFFORT = re.compile(r"\bbest[- ]effort\b", re.IGNORECASE)
FALLBACK = re.compile(r"\bfallbacks?\b|\b(?:fall|falls|fell|falling) back\b", re.IGNORECASE)
DISCARDED_C_CALL = re.compile(r"(?m)^\s*\(void\)\s*[A-Za-z_]\w*\s*\(")

# Every entry below was explicitly classified by H9. Counts matter: adding a
# second copy of an otherwise allowed statement still requires review.
ALLOWED_BEST_EFFORT_LINES = Counter(
    {
        (
            "firmware/components/web_server/web_settings.c",
            "* a best-effort cache refresh. Login remains fail-closed behind the",
        ): 1,
    }
)

ALLOWED_FALLBACK_LINES = Counter(
    {
        (
            "firmware/components/provisioning/include/provisioning.h",
            "* device joins this network at boot and falls back to AP-only if it cannot",
        ): 1,
        ("firmware/components/web_server/web_server_async.c",
         "/* Confirmation-gated work must never fall back to the httpd task:"): 1,
        ("firmware/components/device_controls/device_controls.c",
         "* failure therefore falls back to immediate reboot. If esp_restart()"): 1,
        # Classified 2026-08-16. POST /api/v1/setup commits settings before it
        # reboots, so a device that cannot schedule the reboot must still leave
        # setup mode -- the same trade device_controls.c makes one entry above.
        # The fallback loses the 202 response, which is why it is reached only
        # when the controls task is absent, never on the normal path.
        ("firmware/components/web_server/web_server_setup.c",
         "* so staying in setup mode is the worse outcome: fall back to an"): 1,
        ("firmware/components/web_server/http_health.c",
         "* runtime fallback. */"): 1,
        ("webapp/src/v2/deviceActionsClient.ts",
         "* fall back to Sign In, so this deliberately does not suppress the default"): 1,
        ("webapp/src/v2/gzip.ts", "* uncompressed fallback."): 1,
        ("webapp/src/v2/routingV2.ts",
         'export function routeFromHashV2(fallback: ScreenV2 = "macros"): ScreenV2 {'): 1,
        ("webapp/src/v2/routingV2.ts",
         "return isScreenV2(route) ? route : fallback;"): 1,
        ("webapp/src/v2/routingV2.ts",
         '* preview route with a missing/malformed macro ID" so a caller can fall back'): 1,
        ("webapp/src/v2/routingV2.ts",
         "* caller can fall back rather than editing the wrong macro."): 1,
        ("webapp/src/features/macros/v2/MacrosPage.tsx",
         "// handled above. Fall back to idle rather than mislabeling a"): 1,
        (
            "webapp/src/v2/snapshotClient.ts",
            "* React chooses which blob to load, and never falls back to storing",
        ): 1,
        (
            "webapp/src/v2/snapshotClient.ts",
            "* silently falling back (SPEC_V2 §9).",
        ): 1,
    }
)

ALLOWED_DISCARDED_C_CALLS = Counter(
    {
        ("firmware/components/macro_parser/macro_parser.c",
         "(void)macro_keymap_us_printable('{', &key);"): 1,
        ("firmware/components/macro_parser/macro_parser.c",
         "(void)macro_keymap_us_printable('}', &key);"): 1,
        ("firmware/components/wifi_ap/wifi_ap.c", "(void)esp_wifi_disconnect();"): 1,
        ("firmware/components/wifi_ap/wifi_ap.c",
         "(void)xSemaphoreTake(sta_outcome_semaphore, 0);"): 1,
        ("firmware/components/storage/storage_fs_ops.c", "(void)close(descriptor);"): 1,
        ("firmware/components/device_settings/device_settings.c",
         "(void)reopen_settings_handle();"): 1,
        ("firmware/components/app_contracts_v2/device_settings_v2.c",
         "(void)bounded_length(text, destination_length, &length);"): 1,
        ("firmware/components/app_contracts_v2/device_settings_v2.c",
         '(void)memcpy(settings->device_name, "ESP32 Macro Keyboard", sizeof("ESP32 Macro Keyboard"));'): 2,
        ("firmware/components/macro_executor/macro_executor_engine.c",
         "(void)atomic_compare_exchange_strong_explicit(&engine->release_fault_error, &expected, (int)release_error, memory_order_acq_rel, memory_order_acquire);"): 1,
        ("firmware/components/macro_executor/macro_executor.c",
         "(void)ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(milliseconds));"): 1,
        ("firmware/components/web_server/web_server_status_limits.c",
         "(void)esp_app_get_elf_sha256(build_id, sizeof(build_id));"): 1,
        ("firmware/components/web_server/web_server_status_limits.c",
         '(void)snprintf(firmware_version, sizeof(firmware_version), "%s", esp_app_get_description()->version);'): 1,
        ("firmware/components/web_server/web_server_static.c", "(void)fclose(file.handle);"): 1,
        ("firmware/components/web_server/web_server_diagnostics.c",
         "(void)esp_app_get_elf_sha256(out_snapshot->build_id, sizeof(out_snapshot->build_id));"): 1,
        ("firmware/components/web_server/web_server_diagnostics.c",
         '(void)snprintf(out_snapshot->firmware_version, sizeof(out_snapshot->firmware_version), "%s", description->version);'): 1,
        ("firmware/components/web_server/web_server_blob.c",
         "(void)storage_blob_reader_close(&reader);"): 1,
        ("firmware/components/web_server/web_server_api.c",
         '(void)set_error_response(&response, WEB_HTTP_STATUS_INTERNAL_SERVER_ERROR, APP_ERROR_INTERNAL, "response encoding failed");'): 2,
    }
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=REPO_ROOT)
    return parser.parse_args()


def normalize_statement(statement: str) -> str:
    return " ".join(statement.split())


def line_number(text: str, position: int) -> int:
    return text.count("\n", 0, position) + 1


def fail(path: Path, line: int, message: str) -> None:
    print(f"error: {path}:{line}: {message}", file=sys.stderr)
    raise SystemExit(1)


def source_files(root: Path):
    for relative_root in PRODUCTION_ROOTS:
        directory = root / relative_root
        if not directory.is_dir():
            continue
        for path in sorted(directory.rglob("*")):
            if path.is_file() and path.suffix in SOURCE_SUFFIXES:
                yield path


def main() -> int:
    args = parse_args()
    root = args.root.resolve()
    best_effort_seen: Counter[tuple[str, str]] = Counter()
    fallback_seen: Counter[tuple[str, str]] = Counter()
    discarded_seen: Counter[tuple[str, str]] = Counter()

    for path in source_files(root):
        relative = path.relative_to(root).as_posix()
        text = path.read_text(encoding="utf-8", errors="replace")

        match = EMPTY_CATCH.search(text)
        if match is not None:
            fail(path.relative_to(root), line_number(text, match.start()), "empty catch is forbidden")
        match = EMPTY_PROMISE_CATCH.search(text)
        if match is not None:
            fail(
                path.relative_to(root),
                line_number(text, match.start()),
                "empty Promise rejection handler is forbidden",
            )
        lines = text.splitlines()
        for match in BEST_EFFORT.finditer(text):
            line = line_number(text, match.start())
            line_text = lines[line - 1].strip()
            best_effort_seen[(relative, line_text)] += 1
            if (
                best_effort_seen[(relative, line_text)]
                > ALLOWED_BEST_EFFORT_LINES[(relative, line_text)]
            ):
                fail(
                    path.relative_to(root),
                    line,
                    "best-effort marker requires explicit H9 classification",
                )

        for match in FALLBACK.finditer(text):
            line = line_number(text, match.start())
            line_text = lines[line - 1].strip()
            fallback_seen[(relative, line_text)] += 1
            if fallback_seen[(relative, line_text)] > ALLOWED_FALLBACK_LINES[(relative, line_text)]:
                fail(
                    path.relative_to(root),
                    line,
                    "new or changed fallback occurrence requires H9 classification",
                )

        if path.suffix != ".c":
            continue
        for match in DISCARDED_C_CALL.finditer(text):
            end = text.find(";", match.start())
            if end < 0:
                fail(
                    path.relative_to(root),
                    line_number(text, match.start()),
                    "could not bound explicit discarded C call",
                )
            statement = normalize_statement(text[match.start() : end + 1])
            key = (relative, statement)
            discarded_seen[key] += 1
            if discarded_seen[key] > ALLOWED_DISCARDED_C_CALLS[key]:
                fail(
                    path.relative_to(root),
                    line_number(text, match.start()),
                    "new or changed explicit discarded C call requires H9 classification",
                )

    print(
        "H9 production audit guard passed "
        f"({sum(best_effort_seen.values())} best-effort occurrences, "
        f"{sum(fallback_seen.values())} fallback occurrences, "
        f"{sum(discarded_seen.values())} explicit discarded C calls)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
