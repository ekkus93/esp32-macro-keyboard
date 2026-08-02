---
name: lint-n-test
description: Lint all files and run the full test suite for this ESP32 macro-keyboard repo. Use when the user wants to lint and test everything (not just changed files) before committing or merging.
model: haiku
---

# lint-n-test

Lint every file and run all tests using the repo's authoritative scripts. Do NOT
invent commands — these enforce the "no failure-hiding" policy (no `|| true`, no
warning suppression, no first-party exclusions).

## Step 1 — set up the environment FIRST

`check-all.sh` fails immediately, and misleadingly, without all three of these.
Run them in one shell before the gate; shell state does not persist between
commands, so put the setup and the gate in the SAME invocation.

```bash
export PATH="$HOME/go/bin:$PATH"                 # actionlint, shfmt
export NVM_DIR="$HOME/.nvm"; . "$NVM_DIR/nvm.sh" # nvm
nvm use 24.18.0                                  # Node must be EXACTLY this
. "$HOME/esp/esp-idf-v5.5.5/export.sh"           # ESP-IDF + esp-clang
```

Known failures when a step is missed, so they are recognized rather than
debugged from scratch:

- Missing `$HOME/go/bin` on PATH → `check-scripts.sh` exits **127** (`actionlint`
  not found). The error does not name the tool.
- Wrong Node → `error: Node.js v24.18.0 is required; active version is vX`. The
  system default is not 24.18.0.
- ESP-IDF not sourced → `check-firmware.sh` cannot find `idf.py`, and clang-tidy
  falls back to the apt build, which cannot parse xtensa targets
  (`clang: error: unsupported option '-mcpu='`).

## Step 2 — run the gate

```bash
./scripts/check-all.sh
```

It runs, in order: `verify-toolchain.sh`, `check-format.sh`,
`check-static-analysis-policy.sh`, `check-partitions.sh`,
`check-production-config.sh`, `check-credential-logging.sh`,
`check-frontend-persisted-state.sh`, `check-setup-route-isolation.sh`,
`check-firmware.sh` (build + clang-tidy), `check-stack-usage.sh`,
`build-webfs-image.sh`, `generate-flash-manifest.sh`,
`check-release-budgets.sh`, `check-webapp.sh` (typecheck / eslint / stylelint /
vitest / build), `check-scripts.sh`, `check-docs.sh`, and `run-tests.sh`
(host C tests).

It is slow (several minutes) and prints a lot. Capture to a file and grep, rather
than scrolling:

```bash
./scripts/check-all.sh > /tmp/lnt.log 2>&1; echo "EXIT=$?"
grep -E '^/home.*error:|^error:|grew beyond|no longer match' -A3 /tmp/lnt.log
```

**Only `EXIT=0` is a pass.** clang-tidy prints thousands of suppressed
third-party warnings on a clean run — a wall of "warnings generated" is normal
and is not a failure. Check the exit code, not the volume of output.

## Step 3 — targeted subsets

Use these while iterating on a fix; finish with the full gate before reporting a
pass.

| Scope | Command |
| --- | --- |
| Host C tests only | `./scripts/run-tests.sh` |
| One host test label | `./scripts/run-tests.sh storage` (also: `support parser executor auth web startup usb controls wifi model`) |
| Frontend | `./scripts/check-webapp.sh` |
| Firmware build + clang-tidy | `./scripts/check-firmware.sh` |
| Formatting (check only) | `./scripts/check-format.sh` |
| Shell + workflows | `./scripts/check-scripts.sh` |
| Markdown + YAML | `./scripts/check-docs.sh` |

## Fixing what it finds

- **Formatting** is the only auto-fixable class:
  `git diff --name-only --diff-filter=d | grep -E '\.(c|h)$' | xargs clang-format -i`
  and `npm --prefix webapp run format:write`. Everything else is a real defect.
- **clang-tidy** findings must be fixed at the source. Do not suppress, do not
  add `// NOLINT`, do not exclude first-party files. Approved exceptions live in
  `docs/STATIC_ANALYSIS_EXCEPTIONS.md` and adding one is a deliberate,
  documented act — not a way to make the gate pass.
- **Stack-usage ratchet** (`scripts/stack-usage-allowlist.txt`) has three failure
  modes: an unlisted frame over 4096 bytes, a listed frame that grew, and a
  listed frame that no longer exists. The fix for the first two is to
  heap-allocate the large local — **never to record a bigger number**. The fix
  for the third is to delete the stale entry.

## Reporting

State which commands ran and whether each passed. Paste failing output verbatim;
do not summarize it away. If a step could not run at all, say which and why
rather than reporting a partial run as a pass.
