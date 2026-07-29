#!/usr/bin/env python3
"""Patch the one-shot code-review runner with the reviewed npm audit policy."""

from pathlib import Path


RUNNER = Path("scripts/code-review-fixes-runner.sh")


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected one anchor, found {count}")
    return text.replace(old, new, 1)


def main() -> None:
    text = RUNNER.read_text(encoding="utf-8")

    text = replace_once(
        text,
        '''dev.update(updates)
path.write_text(json.dumps(package, indent=2) + "\\n", encoding="utf-8")
PY
rm -rf webapp/node_modules
''',
        '''dev.update(updates)
package["scripts"]["audit:ci"] = "python3 ../scripts/check-npm-audit.py"
path.write_text(json.dumps(package, indent=2) + "\\n", encoding="utf-8")
PY
cat >scripts/check-npm-audit.py <<'PY'
#!/usr/bin/env python3
"""Enforce the repository's explicit npm audit acceptance policy."""

from __future__ import annotations

import argparse
import datetime as dt
import json
import subprocess
import sys
from pathlib import Path
from typing import Any

ACCEPTED_HIGH_FINDINGS = {
    "@eslint/config-array",
    "@eslint/eslintrc",
    "brace-expansion",
    "eslint",
    "minimatch",
}
ACCEPTED_ADVISORY_SOURCES = {1124334}
ACCEPTANCE_EXPIRES = dt.date(2026, 9, 30)


def load_report(input_path: Path | None) -> dict[str, Any]:
    if input_path is not None:
        return json.loads(input_path.read_text(encoding="utf-8"))
    completed = subprocess.run(
        ["npm", "audit", "--json"],
        check=False,
        capture_output=True,
        text=True,
    )
    if not completed.stdout.strip():
        print(completed.stderr, file=sys.stderr)
        raise SystemExit("npm audit produced no JSON report")
    return json.loads(completed.stdout)


def advisory_sources(finding: dict[str, Any]) -> set[int]:
    sources: set[int] = set()
    for entry in finding.get("via", []):
        if isinstance(entry, dict) and isinstance(entry.get("source"), int):
            sources.add(entry["source"])
    return sources


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path)
    parser.add_argument("--lockfile", type=Path, default=Path("package-lock.json"))
    args = parser.parse_args()

    report = load_report(args.input)
    findings = report.get("vulnerabilities", {})
    counts = report.get("metadata", {}).get("vulnerabilities", {})
    critical = int(counts.get("critical", 0))
    high_names = {
        name
        for name, finding in findings.items()
        if finding.get("severity") == "high"
    }

    if critical:
        raise SystemExit(f"npm audit contains {critical} critical finding(s)")
    if not high_names:
        print("npm audit policy: no high or critical findings")
        return 0
    if dt.date.today() > ACCEPTANCE_EXPIRES:
        raise SystemExit(
            f"npm audit acceptance expired on {ACCEPTANCE_EXPIRES.isoformat()}"
        )
    if high_names != ACCEPTED_HIGH_FINDINGS:
        unexpected = sorted(high_names - ACCEPTED_HIGH_FINDINGS)
        missing = sorted(ACCEPTED_HIGH_FINDINGS - high_names)
        raise SystemExit(
            f"npm audit high-finding set changed; unexpected={unexpected}, missing={missing}"
        )

    lock = json.loads(args.lockfile.read_text(encoding="utf-8"))
    packages = lock.get("packages", {})
    seen_sources: set[int] = set()
    for name in sorted(high_names):
        finding = findings[name]
        nodes = finding.get("nodes", [])
        if not nodes:
            raise SystemExit(f"accepted finding {name} has no dependency nodes")
        for node in nodes:
            package = packages.get(node)
            if not isinstance(package, dict) or package.get("dev") is not True:
                raise SystemExit(
                    f"accepted finding {name} is no longer exclusively dev-only at {node}"
                )
        seen_sources.update(advisory_sources(finding))

    if seen_sources != ACCEPTED_ADVISORY_SOURCES:
        raise SystemExit(
            "npm audit advisory source set changed; "
            f"expected={sorted(ACCEPTED_ADVISORY_SOURCES)}, actual={sorted(seen_sources)}"
        )

    print(
        "npm audit policy: accepted five dev-only ESLint toolchain findings "
        f"through {ACCEPTANCE_EXPIRES.isoformat()}; advisory sources "
        f"{sorted(seen_sources)}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
PY
rm -rf webapp/node_modules
''',
        "dependency policy injection",
    )

    text = replace_once(
        text,
        '''              "The pinned Quality CI runs `npm run audit:ci` and rejects high or critical",
              "advisories. The network-dependent audit is intentionally outside the normal",
              "offline-capable `scripts/check-all.sh` development path.",
              "",
''',
        '''              "The pinned Quality CI runs `npm run audit:ci`. It rejects every critical",
              "finding and every unexpected high finding. Five known high findings in the",
              "ESLint-only development graph are accepted through 2026-09-30 only when the",
              "finding names, advisory source, and dev-only lockfile classification all match",
              "the committed policy in `scripts/check-npm-audit.py`. The deployed ESP32 does",
              "not contain these packages. Any graph or advisory change fails closed and",
              "requires a new review. The network-dependent audit remains outside the normal",
              "offline-capable `scripts/check-all.sh` development path.",
              "",
''',
        "audit report policy",
    )

    text = replace_once(
        text,
        '''(
\tcd webapp
\tnpm run audit:ci
)
printf 'Raw npm audit status before threshold filtering: %s\\n' "${audit_status}"
''',
        '''python3 scripts/check-npm-audit.py \\
\t--input /tmp/npm-audit.json \\
\t--lockfile webapp/package-lock.json
printf 'Raw npm audit status before policy filtering: %s\\n' "${audit_status}"
''',
        "audit policy invocation",
    )

    text = replace_once(
        text,
        '''git rm scripts/code-review-fixes.payload.b64
git rm scripts/code-review-fixes-audit-policy-patch.py
git rm scripts/code-review-fixes-runner.sh
''',
        '''git rm scripts/code-review-fixes.payload.b64
git rm scripts/code-review-fixes-audit-policy-patch.py
git rm scripts/code_review_fixes_audit_policy_patch_impl.py
git rm scripts/code-review-fixes-runner.sh
''',
        "temporary patch cleanup",
    )

    RUNNER.write_text(text, encoding="utf-8")


if __name__ == "__main__":
    main()
