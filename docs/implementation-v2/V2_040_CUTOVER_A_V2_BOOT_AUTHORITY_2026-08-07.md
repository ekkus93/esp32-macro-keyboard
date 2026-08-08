# V2-040 Cutover A — V2 settings boot authority

This intermediate implementation slice intentionally does not mark V2-040 complete.

It establishes the fail-closed runtime boundary required before the transactional setup submission can land:

- canonical `device_settings` is the only boot configuration authority;
- no legacy provisioning record is read during startup;
- normal-mode AP/station/password state is sourced from the validated V2 settings record;
- an unprovisioned boot generates a fresh eight-digit decimal setup code from ESP32 randomness;
- only that setup code is intentionally emitted on the trusted serial surface;
- setup mode exposes only `GET /api/v1/setup`, `POST /api/v1/setup`, and required static assets;
- `GET /api/v1/setup` returns only `provisioned:false` and `deviceName`;
- legacy normal-mode configuration mutations are explicitly unavailable rather than allowed to write state V2 startup would ignore.

`POST /api/v1/setup` remains deliberately fail-closed with `503` in this slice. The next slice replaces that temporary explicit failure with the already-reviewed V2 setup contract and transactional `device_settings_replace()` flow. No V1-to-V2 migration, conversion, alias, dual read, dual write, or fallback is introduced.

## Gate history

The runtime cutover first reached `master` at `371c51828a5d13b5a4c89d38faaa4b947ec7c225`.

On that exact SHA:

- Browser Tests passed;
- the complete Host Tests workflow passed;
- Device Test Build passed, including the ESP32-S3 device-test firmware build;
- Quality stopped before authoritative source checks because the live npm advisory set changed: the previous reviewed ESLint findings disappeared and a new `nanoid <3.3.17` high-severity finding appeared.

The audit drift was not allowlisted. A self-removing repair workflow ran `npm audit fix --package-lock-only` without `--force`, updated only `nanoid` from `3.3.16` to `3.3.18`, reinstalled from the resulting lockfile, and required a clean live `npm audit`. The permanent dependency repair is `69c74b5031718b9322f4432df5c8553cdb660108`.

Because the old reviewed audit exceptions were no longer needed, a second self-removing materializer replaced the exception-based audit policy with a strict-zero policy: any npm vulnerability finding at any severity now fails CI. Its regression suite covers info, low, moderate, high, critical, malformed reports, audit errors, and count mismatches. The permanent strict-policy commit is `11357f957cdfd98aa4722e2d386495999f0eac54`.

The next ordinary candidate, `a1b83b1bfcf69a190056653fed3fa9400a3a8664`, passed Browser Tests, all five Host Tests jobs, and Device Test Build. Its strict-zero npm audit also passed. Quality then reached the authoritative source checks and rejected three Cutover A files for repository `clang-format` conformance only: `firmware/components/app_core/app_core.c`, `firmware/components/app_core/app_core_ops.h`, and `tests/host/test_app_core.c`.

A self-removing diagnostic captured that exact failure in `docs/implementation-v2/V2_040_CUTOVER_A_QUALITY_DIAGNOSTIC.txt`. A separate self-removing formatter workflow then ran repository `clang-format` on exactly those three files, required `clang-format --dry-run --Werror`, ran the full host test suite, and required `git diff --check`. All of those checks passed. The permanent formatter repair is `5074c67e87203b86c22214729bcdd43c15389215`; it changes formatting only.

The documentation commit containing this updated evidence is the final Cutover A candidate and must pass Browser Tests, all Host Tests jobs, Device Test Build, and Quality on its own exact SHA before Cutover B begins.
