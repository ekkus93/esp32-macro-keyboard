# H12-120 — Clean checkout evidence

- **Date:** 2026-08-14
- **Task:** H12-120 — Clean checkout
- **Validated candidate SHA:** `a5474028044f056c92b8e43808be0ad62d1b72a9`
- **Validated candidate tree:** `e068f02cc99cdd794813bc43b4a63157485a50cd`
- **Quality run:** `31822644005`
- **Quality job:** `94839308592`
- **Runner:** `ubuntu-24.04`

## Scope and disposition

H12-120 requires a fresh checkout of the exact candidate, dependency installation
through the documented reproducible path, and proof that success does not depend
on generated or untracked source artifacts. All three requirements are satisfied
for candidate `a5474028044f056c92b8e43808be0ad62d1b72a9`.

No runtime behavior changed as part of this evidence pass. H12-121, H12-122, and
H12-123 remain open and are not inferred from H12-120.

## Exact source identity

The independently supplied clean source archive represented parent commit
`b40202bd58c39a71b2fdc545dc3937a0eb3bdb81`. Git object reconstruction produced
parent tree `3e4941b6b7057f425a39397a04e1a4704a399722`. The only candidate delta was
`docs/TODO_V2.md`; after applying that known documentation delta, its blob hashed
to `bdf040e8bf62e854ebb849b546feb70f4c1cab6e` and the complete reconstructed tree
hashed to `e068f02cc99cdd794813bc43b4a63157485a50cd`, exactly the tree referenced by
candidate commit `a5474028044f056c92b8e43808be0ad62d1b72a9`.

This establishes that the local clean-source audit used the exact candidate source
content rather than an approximate or stale working tree.

## Reproducible dependency-install proof

GitHub `Quality` run `31822644005`, job `94839308592`, executed successfully on
exact head SHA `a5474028044f056c92b8e43808be0ad62d1b72a9` using an Ubuntu 24.04 runner.
The workflow began with the repository checkout action and then used the
repository-pinned/documented setup path:

- `actions/checkout@v4` with recursive submodules and persisted credentials
  disabled;
- `actions/setup-node@v4` with `node-version-file: .nvmrc`, selecting Node.js
  `24.18.0`;
- `npm ci` in `webapp/` from the committed lockfile;
- `npx playwright install --with-deps chromium`;
- the reviewed npm audit policy;
- Ubuntu host lint dependencies plus the workflow-pinned Python, Go, actionlint,
  and shfmt tool versions;
- `./scripts/install-esp-idf.sh`, which fail-closes on anything other than the
  exact ESP-IDF `v5.5.5` tag and installs the ESP32-S3 plus esp-clang toolchains;
- `littlefs-python==0.15.0` with an explicit version assertion; and
- `./scripts/check-all.sh` after exporting ESP-IDF 5.5.5.

Every Quality job step completed successfully. No alternate Node version, relaxed
engine check, alternate ESP-IDF revision, ignored installer error, or fallback
bootstrap path was used.

## Independent generated/untracked-source audit

Before dependency installation, the fresh local source tree contained no
`node_modules`, build output, distribution output, coverage output, object/archive
files, Python caches, `sdkconfig`, or managed-component generated output.

The following offline validations were then executed from the clean source tree:

```text
python3 scripts/check-v2-limits.py
python3 scripts/generate-spec-traceability.py --check
python3 tests/scripts/test-generate-v2-macro-corpus.py
./scripts/check-v2-contracts.sh --native-only
```

Results:

- the v2 limits mirror check passed;
- specification traceability was current;
- the macro-conformance generator test suite passed 6/6 tests;
- the native v2 contract build generated its macro corpus include inside the
  build directory, compiled all six contract binaries, and passed 6/6 tests; and
- no required untracked source artifact appeared. Generated files remained build
  output rather than source prerequisites.

The package lock also matched the root dependency/devDependency declarations in
`webapp/package.json`, `.npmrc` retained `engine-strict=true`, and the committed
firmware dependency locks identify ESP-IDF 5.5.5.

## Local sandbox limitation and fail-closed handling

The local sandbox itself had no DNS/network access and only Node.js 22.16.0
cached. Its attempt to obtain the repository-pinned Node.js 24.18.0 therefore
could not complete. That limitation was not bypassed: Node 22 was not accepted as
an authoritative substitute and no dependency pin was weakened.

The exact-SHA Ubuntu 24.04 Quality job supplies the missing networked clean-
bootstrap proof while the independent local source audit supplies the separate
no-hidden-generated-source proof.

## H12-120 result

H12-120 is complete for
`a5474028044f056c92b8e43808be0ad62d1b72a9`:

1. exact fresh candidate source identity is proven;
2. documented reproducible dependency installation is proven on the exact SHA;
3. no generated or untracked source artifact is required for success.

This report and the tracker closure are documentation-only descendants of the
validated candidate. H12-121 must run its complete authoritative gate on the
resulting descendant separately; H12-120's Quality result is not reused as
H12-121 evidence. H12-122 and H12-123 likewise remain open.

No unchecked task is claimed complete by this report.
