# V2-035 — esptool v4 image-info production repair

**Status:** Hosted repair validated; physical V2-035 evidence still required  
**Task:** V2-035  
**Production repair commit:** `cd95fc5528f1608c77801c1bdcddc1c615cecf65`  
**Permanent-CI evidence commit:** `9779baace75fd395395fd55fca7813831c30f964`  
**Target hardware:** ESP32-S3R8  
**Required toolchain:** ESP-IDF v5.5.5

## Scope

This report records a hosted V2-035 harness defect discovered during the final
Ralph-loop audit. It does not claim any of the seven physical V2-035 checklist
items complete.

The earlier scoped repair path had corrected the temporary materializer that was
supposed to migrate V2-035 ELF metadata inspection to esptool v4 syntax. The
scoped workflow did not reach its publication step because an unrelated
repository-wide gate failed first. Later cleanup removed the temporary workflow
and materializer, leaving the intended production edits unapplied.

That state was unsafe because the repository looked cleaned up while the actual
hardware evidence collector and flash-manifest generator still invoked the
legacy command form:

```text
esptool.py image_info <application.bin>
```

ESP-IDF v5.5.5 supplies esptool v4, where V2-035 must request the v2 image format
explicitly in order to obtain the full ELF SHA-256 used to bind hardware evidence
to the exact application image.

## Permanent repair

Commit `cd95fc5528f1608c77801c1bdcddc1c615cecf65` applies the missing production
repair directly on `master`:

- `scripts/generate-flash-manifest.sh` now invokes
  `esptool.py image_info --version 2 <application.bin>`;
- `scripts/run-v2-035-hardware.py` independently verifies the application image
  with the same exact esptool v4 command;
- `tests/scripts/fakes/esptool.py` accepts only the required four-argument
  `image_info --version 2 <image>` form; and
- `tests/scripts/test-v2-035-hardware.py` asserts the exact command vector,
  subprocess options, and one-call provenance verification behavior.

The repair does not add a fallback to the legacy syntax. If esptool cannot
produce the required full ELF SHA-256 through the pinned command, the evidence
collector fails closed.

## Publication method and cleanup

A one-shot workflow was added directly to `master` solely because the connected
GitHub API exposes file writes but not arbitrary patch application or workflow
dispatch. The workflow:

1. checked out the exact trigger commit;
2. applied four exact `replace_once` edits;
3. ran the focused V2-035 regression suites before publication;
4. fetched `origin/master` and refused to publish if the branch had moved;
5. verified an exact five-path staged scope including its own deletion; and
6. committed the four permanent edits while removing itself.

The one-shot workflow is absent from the production repair commit.

Scoped publication evidence:

- trigger commit: `a163f93635c8292081e1a32017ce0d899f5a5101`;
- workflow run: `31145329093`;
- job: `92763446098`;
- result: success;
- focused Python compilation: pass;
- complete `tests/scripts/test-v2-035-hardware.py`: pass;
- `tests/scripts/test-generate-flash-manifest.sh`: pass;
- shell syntax checks: pass;
- `git diff --check`: pass;
- exact staged-path verification: pass;
- one-shot workflow removal: pass.

## Permanent CI evidence

A normal GitHub API commit, `9779baace75fd395395fd55fca7813831c30f964`,
was used to trigger every permanent workflow after the production repair. All
four permanent gates completed successfully on that exact SHA:

- Browser Tests — run `31145502545`, job `92763965174`: success;
- Host Tests — run `31145502514`: success across all five jobs:
  - Native Coverage `92763965184`;
  - Frontend Coverage `92763965190`;
  - Host Tests `92763965196`;
  - Host ASan and UBSan `92763965203`;
  - Frontend Tests `92763965209`;
- Device Test Build — run `31145502503`, job `92763992223`: success, including
  device-test source lint and the ESP32-S3 device-test firmware build; and
- Quality — run `31145502509`, job `92764007163`: success, including the reviewed
  npm audit policy, pinned ESP-IDF v5.5.5 setup, V2-034 LittleFS dependency, and
  the complete authoritative `./scripts/check-all.sh` gate.

The failed-log upload in Quality was skipped because the authoritative gate
passed.

This report update is itself a new evidence-only commit and therefore cannot
contain its own resulting commit SHA or future workflow run IDs without creating
an endless self-referential commit chain. The Ralph loop separately requires the
permanent Browser, Host, Device, and Quality workflows to pass on this final
report-update SHA before hosted V2-035 preparation is considered settled.

## Remaining physical gate

No physical ESP32-S3R8 observation is inferred from the hosted repair or CI.
V2-035 remains open until committed sanitized hardware evidence proves all seven
required scenarios:

1. power-cycle persistence;
2. numeric ordering;
3. deletion preservation;
4. interrupted upload without a partial final blob;
5. reboot cleanup of temporary files;
6. real HTTP 507 storage exhaustion preserving all committed blobs; and
7. mount failure without formatting userdata.

No unchecked V2-035 TODO item is claimed complete by this report.
