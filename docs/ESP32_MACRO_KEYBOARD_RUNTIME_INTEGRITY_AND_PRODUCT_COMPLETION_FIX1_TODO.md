# ESP32 Macro Keyboard Runtime Integrity and Product Completion FIX1 TODO

**Document type:** Ordered implementation checklist
**Repository:** `ekkus93/esp32-macro-keyboard`
**Target branch:** `master`
**Review baseline:** `992f2a018aff97e5b167c98d6a0d469d6a7c84ff`
**Required specification:**
`docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_SPEC.md`

## 0. Instructions for Claude Code

Read the FIX1 specification completely before changing code.

Implement tasks in order. Do not skip an earlier phase because a later phase is
more visible.

For every task:

1. inspect the current implementation and tests;
2. preserve intentional good behavior;
3. add or update tests before marking the task complete;
4. run the smallest relevant test group during development;
5. run all required quality checks at the phase gate;
6. update documentation evidence;
7. commit only when the phase is internally consistent.

Do not:

- use `|| true` to hide a failed first-party check;
- ignore an error with `(void)` unless the called operation is explicitly
  infallible and documented as such;
- return success after partial cleanup;
- replace a primary failure with a cleanup failure;
- keep mock product buttons enabled;
- invent fixed credentials;
- format LittleFS automatically after failure;
- add an open AP fallback;
- accept arbitrary macro source over the execution API;
- force-push or rewrite existing history;
- reference files that are not committed at the named paths.

The code snippets below define required implementation shapes. Use them directly
where they match the current interfaces. When a snippet introduces a new type,
add the matching header declaration, CMake source entry, host fake, and tests.

## 1. Establish the FIX1 baseline

### 1.1 Confirm repository state

- [x] Confirm `master` contains baseline commit
      `992f2a018aff97e5b167c98d6a0d469d6a7c84ff` or a documented descendant.
- [x] Confirm both FIX1 documents exist under `docs/`.
- [x] Create a dedicated implementation branch unless the operator explicitly
      instructs direct work on `master`.
- [x] Record the actual starting commit in the first implementation commit.

### 1.2 Add a FIX1 progress document

Create:

```text
docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_PROGRESS.md
```

The progress file must contain:

- starting commit;
- current phase;
- completed tasks with commit SHAs;
- tests run;
- CI links or run IDs;
- unresolved blockers;
- hardware evidence;
- deviations from this TODO and why.

Do not mark a checkbox in this TODO without corresponding evidence.

### 1.3 Baseline validation

Run:

```bash
./scripts/check-all.sh
./scripts/run-tests.sh --sanitizers
./scripts/generate-native-coverage.sh
./scripts/generate-frontend-coverage.sh
./scripts/build-device-tests.sh
```

- [x] Record all baseline results without changing them to green by suppression.
- [x] Record the current firmware binary size, webapp build size, and test
      counts.
- [x] Record any check that cannot run locally and the exact missing dependency.

## 2. Make the quality gate fail closed

### 2.1 Replace ignored clang-tidy exit status

**Primary file:**

```text
scripts/check-firmware.sh
```

Replace the current command substitution that ends in `|| true`.

Use this implementation shape:

```bash
run_first_party_clang_tidy() {
    local project_dir="$1"
    local build_dir="${project_dir}/build-clang"
    local compile_database="${build_dir}/compile_commands.json"

    if [[ ! -f "${compile_database}" ]]; then
        printf 'error: missing compile database: %s\n' \
            "${compile_database}" >&2
        return 1
    fi

    if ! jq empty "${compile_database}" >/dev/null; then
        printf 'error: invalid compile database: %s\n' \
            "${compile_database}" >&2
        return 1
    fi

    local translation_units
    translation_units="$(
        jq --arg pattern '/firmware/(main/|components/|test_app/main/)' \
            '[.[] | select(.file | test($pattern))] | length' \
            "${compile_database}"
    )"
    if [[ ! "${translation_units}" =~ ^[0-9]+$ ]] ||
        ((translation_units == 0)); then
        printf 'error: no first-party translation units selected in %s\n' \
            "${compile_database}" >&2
        return 1
    fi

    local report_file
    report_file="$(mktemp)"
    local status
    set +e
    run-clang-tidy \
        -p "${build_dir}" \
        -header-filter="${first_party}" \
        "${first_party}" >"${report_file}" 2>&1
    status=$?
    set -e

    cat -- "${report_file}"

    if ((status != 0)); then
        printf 'error: run-clang-tidy failed for %s with status %d\n' \
            "${project_dir}" "${status}" >&2
        rm -f -- "${report_file}"
        return 1
    fi

    local findings
    findings="$(
        grep -E ':[0-9]+:[0-9]+: (warning|error):' "${report_file}" |
            grep -E "${first_party}" || :
    )"
    rm -f -- "${report_file}"

    if [[ -n "${findings}" ]]; then
        printf '%s\n' "${findings}"
        printf 'error: first-party clang-tidy findings in %s\n' \
            "${project_dir}" >&2
        return 1
    fi
}
```

- [x] Call `run_first_party_clang_tidy` for both firmware projects.
- [x] Add cleanup through `trap` or explicit removal for temporary report files.

**Implemented (RESPONSES Q1):** the snippet above is refined so `run-clang-tidy`'s
exit status is trustworthy without post-filtering. Third-party header diagnostics
are excluded *before* emission: `run-clang-tidy -exclude-header-filter='(esp-idf|managed_components)'`
for most checks, plus `misc-header-include-cycle.IgnoredFilesList` in `.clang-tidy`
for the FreeRTOS include cycle in `idf_additions.h` (which `-exclude-header-filter`
does not cover). Both options are verified supported on the pinned esp-clang
LLVM 19.1.2. The grep of the report is only an additional assertion after a
successful analyzer run. See `scripts/check-firmware.sh`.

### 2.2 Add script regression tests

Add shell tests under a new first-party script-test location, for example:

```text
tests/scripts/test-check-firmware.sh
tests/scripts/fakes/run-clang-tidy
tests/scripts/fakes/idf.py
```

Test at least:

- [x] analyzer executable missing;
- [x] analyzer exits nonzero with no warning-shaped output;
- [x] analyzer exits zero with a first-party warning;
- [x] analyzer exits zero with only third-party warnings;
- [x] compile database missing;
- [x] compile database invalid;
- [x] zero selected first-party translation units;
- [x] valid clean run.

Update:

```text
scripts/check-scripts.sh
scripts/check-all.sh
```

so these tests run automatically.

### 2.3 Remove first-party formatting suppression

Refactor:

```text
firmware/components/auth/auth_core.c
firmware/components/web_server/web_server.c
firmware/components/web_server/web_server_adapter.c
```

Preferred change:

- convert each included implementation fragment into a normal `.c` translation
  unit;
- move shared private declarations into an `*_internal.h` header;
- list every new `.c` file in the component `CMakeLists.txt`;
- remove all first-party `clang-format off` and `clang-format on` comments.

- [x] Ensure no duplicate non-static symbol is introduced.
- [x] Ensure coverage includes the new source files.
- [x] Ensure host tests compile the same production translation units.

### 2.4 Phase 2 gate

Run:

```bash
./scripts/check-format.sh
./scripts/check-firmware.sh
./scripts/check-scripts.sh
./scripts/run-tests.sh
```

- [x] Deliberately break the analyzer command and verify the gate fails.
- [x] Deliberately emit one first-party warning and verify the gate fails.
- [x] Restore the tree and verify all checks pass.

## 3. Introduce structured failure and ownership reporting

### 3.1 Add a common operation result

Create:

```text
firmware/components/support/include/app_operation_result.h
firmware/components/support/app_operation_result.c
```

Use:

```c
#ifndef APP_OPERATION_RESULT_H
#define APP_OPERATION_RESULT_H

#include <stdbool.h>

#include "app_error.h"

typedef struct {
    app_error_code_t primary_error;
    app_error_code_t cleanup_error;
    bool cleanup_incomplete;
} app_operation_result_t;

static inline app_operation_result_t app_operation_success(void) {
    return (app_operation_result_t){
        .primary_error = APP_ERROR_NONE,
        .cleanup_error = APP_ERROR_NONE,
        .cleanup_incomplete = false,
    };
}

static inline bool app_operation_result_ok(app_operation_result_t result) {
    return result.primary_error == APP_ERROR_NONE &&
           result.cleanup_error == APP_ERROR_NONE &&
           !result.cleanup_incomplete;
}

void app_operation_record_primary(app_operation_result_t *result,
                                  app_error_code_t error);
void app_operation_record_cleanup(app_operation_result_t *result,
                                  app_error_code_t error);

#endif
```

Implementation:

```c
#include "app_operation_result.h"

void app_operation_record_primary(app_operation_result_t *result,
                                  app_error_code_t error) {
    if (result != NULL && error != APP_ERROR_NONE &&
        result->primary_error == APP_ERROR_NONE) {
        result->primary_error = error;
    }
}

void app_operation_record_cleanup(app_operation_result_t *result,
                                  app_error_code_t error) {
    if (result != NULL && error != APP_ERROR_NONE) {
        if (result->cleanup_error == APP_ERROR_NONE) {
            result->cleanup_error = error;
        }
        result->cleanup_incomplete = true;
    }
}
```

- [x] Add host unit tests.
- [x] Add support component CMake entries.
- [x] Do not use this as an excuse to collapse stable API errors.

### 3.2 Extend application log events

Update:

```text
firmware/components/app_core/app_core_ops.h
firmware/components/app_core/app_core.c
firmware/components/app_core/app_core_sequence.c
```

Add log fields for:

- subsystem;
- primary error;
- cleanup error;
- cleanup incomplete;
- operation ID when available.

- [x] Ensure logs never include credentials, tokens, cookies, or macro source.
- [x] Add exact host assertions for event ordering and contents.

## 4. Correct application lifecycle ownership

### 4.1 Add subsystem deinitialization APIs

Add declarations and implementations:

```c
app_error_code_t auth_deinit(void);
app_error_code_t usb_keyboard_deinit(void);
app_error_code_t macro_executor_deinit(void);
app_error_code_t device_controls_deinit(void);
app_error_code_t storage_repository_deinit(void);
```

Update each public component header and CMake dependency.

### 4.2 Extend `app_core_ops_t`

Add operations:

```c
app_error_code_t (*repository_deinit)(void *context);
app_error_code_t (*auth_deinit)(void *context);
app_error_code_t (*usb_deinit)(void *context);
app_error_code_t (*executor_deinit)(void *context);
app_error_code_t (*controls_deinit)(void *context);
app_error_code_t (*nvs_deinit)(void *context);
bool (*http_owns_resources)(void *context);
bool (*wifi_owns_resources)(void *context);
```

Update `operations_valid()` so every required callback is checked.

### 4.3 Track every completed stage

Add a lifecycle structure in `app_core_sequence.c`:

```c
typedef struct {
    bool nvs_initialized;
    bool storage_mounted;
    bool repository_initialized;
    bool auth_initialized;
    bool usb_initialized;
    bool executor_initialized;
    bool controls_initialized;
    bool wifi_started;
    bool http_started;
} app_core_owned_t;
```

Do not infer ownership only from a function's final return code.

### 4.4 Implement exhaustive reverse cleanup

Use this implementation shape:

```c
static void record_cleanup(app_operation_result_t *operation,
                           app_error_code_t result) {
    app_operation_record_cleanup(operation, result);
}

static app_operation_result_t cleanup_after_failure(
    const app_core_ops_t *operations,
    app_core_owned_t *owned,
    app_error_code_t primary_error) {
    app_operation_result_t result = app_operation_success();
    app_operation_record_primary(&result, primary_error);

    if (owned->http_started ||
        operations->http_owns_resources(operations->context)) {
        const app_error_code_t cleanup =
            operations->http_stop(operations->context);
        record_cleanup(&result, cleanup);
        if (cleanup == APP_ERROR_NONE) {
            owned->http_started = false;
        }
    }

    if (owned->wifi_started ||
        operations->wifi_owns_resources(operations->context)) {
        const app_error_code_t cleanup =
            operations->wifi_stop(operations->context);
        record_cleanup(&result, cleanup);
        if (cleanup == APP_ERROR_NONE) {
            owned->wifi_started = false;
        }
    }

    if (owned->controls_initialized) {
        const app_error_code_t cleanup =
            operations->controls_deinit(operations->context);
        record_cleanup(&result, cleanup);
        if (cleanup == APP_ERROR_NONE) {
            owned->controls_initialized = false;
        }
    }

    if (owned->executor_initialized) {
        const app_error_code_t cleanup =
            operations->executor_deinit(operations->context);
        record_cleanup(&result, cleanup);
        if (cleanup == APP_ERROR_NONE) {
            owned->executor_initialized = false;
        }
    }

    if (owned->usb_initialized) {
        const app_error_code_t cleanup =
            operations->usb_deinit(operations->context);
        record_cleanup(&result, cleanup);
        if (cleanup == APP_ERROR_NONE) {
            owned->usb_initialized = false;
        }
    }

    if (owned->auth_initialized) {
        const app_error_code_t cleanup =
            operations->auth_deinit(operations->context);
        record_cleanup(&result, cleanup);
        if (cleanup == APP_ERROR_NONE) {
            owned->auth_initialized = false;
        }
    }

    if (owned->repository_initialized) {
        const app_error_code_t cleanup =
            operations->repository_deinit(operations->context);
        record_cleanup(&result, cleanup);
        if (cleanup == APP_ERROR_NONE) {
            owned->repository_initialized = false;
        }
    }

    if (owned->storage_mounted) {
        const app_error_code_t cleanup =
            operations->storage_unmount(operations->context);
        record_cleanup(&result, cleanup);
        if (cleanup == APP_ERROR_NONE) {
            owned->storage_mounted = false;
        }
    }

    if (owned->nvs_initialized) {
        const app_error_code_t cleanup =
            operations->nvs_deinit(operations->context);
        record_cleanup(&result, cleanup);
        if (cleanup == APP_ERROR_NONE) {
            owned->nvs_initialized = false;
        }
    }

    const app_error_code_t indicator =
        operations->set_indicator(operations->context,
                                  DEVICE_INDICATOR_FATAL);
    record_cleanup(&result, indicator);
    return result;
}
```

Do not return from the middle of this cleanup function.

### 4.5 Reorder startup around provisioning

Before initializing USB, executor, or controls:

- [x] initialize NVS;
- [x] mount and recover storage;
- [x] initialize repositories;
- [x] initialize authentication;
- [ ] load and validate persistent provisioning state.
      (Ordering established — the provisioning decision now runs here, before
      USB/executor/controls. The persistent encrypted-NVS load/validate itself
      lands in Phase 14; only its position is fixed in Phase 4.)

If production provisioning is incomplete:

- [ ] enter the explicit setup mode; or
      (Explicit setup mode is implemented in Phase 14.)
- [x] cleanly stop and report provisioning required.

Do not initialize normal-operation tasks and then return
`APP_ERROR_AUTH_REQUIRED`.

### 4.6 Add failure-injection tests

Update:

```text
tests/host/test_app_core.c
```

For every startup stage:

- [x] inject primary failure;
- [x] assert exact cleanup order;
- [x] inject cleanup failure at each later stage;
- [x] assert all remaining stages are still attempted;
- [x] assert primary and cleanup errors are both retained;
- [x] assert no owned resource remains when cleanup succeeds;
- [x] assert residual ownership is visible when cleanup fails.

### 4.7 Phase 4 gate

Run:

```bash
./scripts/run-tests.sh startup
./scripts/run-tests.sh --sanitizers startup
./scripts/check-firmware.sh
```

## 5. Correct HTTP partial-start lifecycle

### 5.1 Add resource ownership query

Update:

```text
firmware/components/web_server/include/web_server.h
firmware/components/web_server/web_server.c
firmware/components/web_server/web_server_adapter_lifecycle.c
```

Add:

```c
bool web_server_owns_resources(void);
```

Implementation shape:

```c
bool web_server_owns_resources(void) {
    return server_lifecycle.handle != NULL;
}
```

### 5.2 Preserve partial-start state

When route registration fails and `httpd_stop()` also fails:

- [x] keep the handle;
- [x] retain the registered-route count;
- [x] retain cleanup error;
- [x] return a structured or stable failure;
- [x] ensure `app_core` sees ownership and calls stop again.

### 5.3 Make stop idempotent

- [x] `web_server_stop()` returns success when no handle exists.
- [x] A successful stop clears configuration and lifecycle state.
- [x] A failed stop retains all state needed for retry.
- [x] No later start is allowed while a residual handle exists.

### 5.4 Add tests

Test:

- [x] start failure before handle creation;
- [x] registration failure plus successful stop;
- [x] registration failure plus failed stop;
- [x] retry stop after partial start;
- [x] successful retry clears state;
- [x] start rejected while residual state remains.

## 6. Correct filesystem mount ownership and topology

### 6.1 Return mount ownership

Replace the two loose mount booleans with explicit state:

```c
typedef struct {
    bool web_mounted;
    bool data_mounted;
} storage_mount_state_t;
```

Add:

```c
storage_mount_state_t storage_mount_state(void);
```

`app_core` must check this state during cleanup even when `storage_mount_all()`
returns failure.

### 6.2 Validate existing directories

Replace `mkdir_checked()` with:

```c
static app_error_code_t ensure_directory(const char *path) {
    if (mkdir(path, STORAGE_DIR_MODE) == 0) {
        return APP_ERROR_NONE;
    }

    const int create_error = errno;
    if (create_error != EEXIST) {
        return create_error == ENOSPC ? APP_ERROR_STORAGE_FULL
                                      : APP_ERROR_IO;
    }

    struct stat metadata;
    if (stat(path, &metadata) != 0) {
        return errno == ENOSPC ? APP_ERROR_STORAGE_FULL
                               : APP_ERROR_IO;
    }
    return S_ISDIR(metadata.st_mode) ? APP_ERROR_NONE
                                     : APP_ERROR_STORAGE_CORRUPT;
}
```

- [ ] Verify permissions where supported by LittleFS.
      (Directories are created 0750; LittleFS permission support is limited, so a
      full permission assertion is a device-observable item. The host filesystem
      does not preserve the mode identically, so it is not asserted on host.)
- [x] Reject symlink-like or unsupported types in host tests.

### 6.3 Add mount rollback tests

Test:

- [x] web mount fails;
- [x] data mount fails and web unmount succeeds;
- [x] data mount fails and web unmount fails;
- [x] directory creation fails after both mounts;
- [x] unmount continues for both partitions after one failure;
- [x] regular file collides with every required directory.

## 7. Add atomic-write artifact recovery

### 7.1 Define artifact parsing

Create:

```text
firmware/components/storage/storage_atomic_recovery.c
firmware/components/storage/storage_atomic_recovery.h
```

Define:

```c
typedef enum {
    STORAGE_ATOMIC_ARTIFACT_TEMPORARY = 0,
    STORAGE_ATOMIC_ARTIFACT_BACKUP,
} storage_atomic_artifact_kind_t;

typedef struct {
    char destination[APP_PATH_MAX_BYTES];
    app_uuid_t operation_id;
    storage_atomic_artifact_kind_t kind;
    char artifact_path[APP_PATH_MAX_BYTES];
} storage_atomic_artifact_t;
```

Parse only suffixes of the exact form:

```text
.tmp.<lowercase-rfc4122-v4-uuid>
.bak.<lowercase-rfc4122-v4-uuid>
```

- [x] Reject path traversal.
- [x] Reject empty destination.
- [x] Reject duplicate artifact paths.
- [x] Reject cross-directory destination reconstruction.

### 7.2 Add validator callbacks

Define:

```c
typedef app_error_code_t (*storage_atomic_validate_fn)(
    void *context,
    const char *destination,
    const char *candidate_path);
```

Recovery must never activate a candidate without an object-specific validator.

Provide validators for:

- [x] transaction manifest;
- [x] schema marker;
- [x] set index;
- [x] global macro index;
- [x] set metadata;
- [ ] macro object;
- [ ] procedure object;
- [ ] progress object;
- [ ] settings object;
- [x] quarantine record.

The macro / procedure / progress / settings object validators are deferred to
Phase 15 (their object repositories are not yet implemented — settings has no
storage representation at all). Until then the dispatch classifies those
destinations but has **no validator**, so recovery refuses to activate their
candidates — the fail-closed behavior the invariant requires.

The FIX1 §7.2 validator typedef sketch used two separate `const char *` parameters
(destination, candidate_path). Those are folded into one `storage_atomic_candidate_t`
struct to satisfy the first-party `bugprone-easily-swappable-parameters` policy
(which forbids exempting parameters we control); the semantics are unchanged.

### 7.3 Implement reconciliation rules

Add deterministic tests for all canonical/temporary/backup combinations.

Required conservative behavior:

- [x] restore a valid backup when canonical is absent;
- [x] preserve the old committed state when operation intent is ambiguous;
- [x] activate a temporary only when the owning transaction proves roll-forward;
- [x] quarantine malformed or conflicting artifacts;
- [x] check every rename, unlink, close, and parent sync;
- [x] retain recovery evidence when cleanup fails.

Note: the executor scans the mount root, `global/`, `transactions/`, and every
`sets/` and `staging/` subdirectory. `quarantine/` is intentionally not scanned
for reconciliation (its own atomic artifacts cannot be re-quarantined, since
`safe_source_path` excludes the quarantine root); handling quarantine-record
artifacts is tracked as a small residual. At the atomic layer roll-forward is
never proven (the barrier only publishes a canonical name once fully written), so
the executor always rolls interrupted writes back; transaction-level completion
is handled by transaction recovery, which runs after (§7.4).

### 7.4 Run atomic recovery before transaction recovery

Update the startup order:

```text
mount
atomic artifact recovery
transaction manifest recovery
quarantine staging recovery
repository initialization
```

Do not enumerate strict transaction manifest filenames before their own atomic
artifacts are reconciled.

### 7.5 Add fault-injection tests

Extend:

```text
tests/host/test_storage_atomic.c
tests/host/test_storage_transactions.c
```

Inject interruption/failure after:

- [x] temporary open;
- [x] partial write;
- [x] file sync;
- [x] close;
- [x] readback;
- [x] destination-to-backup rename;
- [x] first parent sync;
- [x] temporary-to-destination rename;
- [x] second parent sync;
- [x] backup removal;
- [x] final parent sync.

Assert old or new complete state, never ambiguous active state.

Implemented as a deterministic crash-consistency matrix in
`tests/host/test_storage_atomic_recovery.c`: the exact on-disk state a crash after
each of the eleven steps would leave is constructed and reconciled, asserting the
destination ends OLD-complete or NEW-complete with no leftover artifacts. (This
uses a constructed post-step state rather than an aborted live write, because a
mid-write fault triggers the write's own graceful rollback rather than the crash
state recovery must handle; the file location differs from the §7.5 sketch, which
named test_storage_atomic.c / test_storage_transactions.c.)

## 8. Make quarantine recoverable

### 8.1 Change quarantine layout

Use:

```text
/data/staging/quarantine-<transaction-id>/
  record.json
  evidence

/data/quarantine/<quarantine-id>/
  record.json
  evidence
```

Update path helpers and limits.

### 8.2 Implement staged quarantine creation

Required sequence:

1. create unique staging directory;
2. copy or rename source into staged evidence;
3. create bounded record;
4. sync evidence and record;
5. validate record and evidence;
6. sync staging directory;
7. rename staging directory into quarantine;
8. sync quarantine parent;
9. return committed entry.

If the source must remain available until commit, copy it into staging and
remove the source only after activation. If LittleFS rename semantics are used,
document the exact rollback behavior.

### 8.3 Add quarantine recovery

At startup:

- [x] finish a provably complete staged quarantine;
- [x] restore the source when activation never occurred and restoration is safe;
- [x] preserve ambiguous staging as evidence;
- [x] never delete an unmatched evidence file;
- [x] never make the entire quarantine list unreadable because one entry is
      damaged; return valid entries plus a health error.

### 8.4 Add tests

Test power loss after every quarantine phase and corruption of:

- [x] record only;
- [x] evidence only;
- [x] directory name;
- [x] record ID;
- [x] source path;
- [x] reason;
- [x] duplicate quarantine ID.

## 9. Serialize repository operations

### 9.1 Add repository lock abstraction

Create:

```text
firmware/components/storage/storage_repository_lock.c
firmware/components/storage/storage_repository_lock.h
```

Production implementation:

```c
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static SemaphoreHandle_t repository_mutex;

app_error_code_t storage_repository_lock_init(void) {
    if (repository_mutex != NULL) {
        return APP_ERROR_CONFLICT;
    }
    repository_mutex = xSemaphoreCreateMutex();
    return repository_mutex != NULL ? APP_ERROR_NONE
                                    : APP_ERROR_INTERNAL;
}

app_error_code_t storage_repository_lock_take(void) {
    if (repository_mutex == NULL) {
        return APP_ERROR_INTERNAL;
    }
    return xSemaphoreTake(repository_mutex, portMAX_DELAY) == pdTRUE
               ? APP_ERROR_NONE
               : APP_ERROR_INTERNAL;
}

app_error_code_t storage_repository_lock_give(void) {
    if (repository_mutex == NULL) {
        return APP_ERROR_INTERNAL;
    }
    return xSemaphoreGive(repository_mutex) == pdTRUE
               ? APP_ERROR_NONE
               : APP_ERROR_INTERNAL;
}

app_error_code_t storage_repository_lock_deinit(void) {
    if (repository_mutex == NULL) {
        return APP_ERROR_NONE;
    }
    vSemaphoreDelete(repository_mutex);
    repository_mutex = NULL;
    return APP_ERROR_NONE;
}
```

For host tests, provide an operations seam rather than compiling FreeRTOS
directly.

### 9.2 Split public and locked helpers

Pattern:

```c
static app_error_code_t storage_set_update_locked(
    const macro_set_t *replacement,
    uint32_t expected_revision,
    macro_set_t *out_updated);

app_error_code_t storage_set_update(const macro_set_t *replacement,
                                    uint32_t expected_revision,
                                    macro_set_t *out_updated) {
    app_error_code_t result = storage_repository_lock_take();
    if (result != APP_ERROR_NONE) {
        return result;
    }

    result = storage_set_update_locked(replacement,
                                       expected_revision,
                                       out_updated);
    const app_error_code_t unlock_result =
        storage_repository_lock_give();
    return unlock_result == APP_ERROR_NONE ? result
                                           : APP_ERROR_INTERNAL;
}
```

Do not call another public locking repository function while the lock is held.
Use internal `_locked` helpers.

### 9.3 Add concurrency tests

Use host threads or deterministic fake scheduling to prove:

- [x] two updates with the same expected revision cannot both succeed;
- [x] create and delete cannot race the same index;
- [x] recovery cannot race an API mutation;
- [ ] import/restore excludes all other mutations (deferred with the
      import/restore feature, Phase 18; it will acquire the same lock, so the
      exclusion proven for create/delete/recovery covers it once implemented);
- [x] unlock failure is visible and does not report mutation success.

## 10. Separate password mismatch from crypto failure

### 10.1 Change the authentication API

Update:

```text
firmware/components/auth/include/auth.h
firmware/components/auth/auth.c
firmware/components/auth/auth_core.h
firmware/components/auth/auth_core_password.c
```

Use:

```c
app_error_code_t auth_password_verify(
    const char *password,
    size_t password_length,
    const auth_password_record_t *record,
    bool *out_matches);
```

Core implementation shape:

```c
app_error_code_t auth_core_password_verify(
    auth_core_t *core,
    const char *password,
    size_t password_length,
    const auth_password_record_t *record,
    bool *out_matches) {
    if (out_matches != NULL) {
        *out_matches = false;
    }
    if (core == NULL || password == NULL || record == NULL ||
        out_matches == NULL ||
        password_length < AUTH_CORE_PASSWORD_MIN_BYTES ||
        password_length > AUTH_CORE_PASSWORD_MAX_BYTES ||
        record->iterations < AUTH_CORE_PBKDF2_ITERATIONS ||
        memchr(password, '\0', password_length) != NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    uint8_t derived[AUTH_HASH_BYTES] = {0};
    const int derive_result = core->ops.derive(
        core->ops.context,
        password,
        password_length,
        record->salt,
        record->iterations,
        derived);
    if (derive_result != 0) {
        core->ops.secure_zero(core->ops.context,
                              derived,
                              sizeof(derived));
        return APP_ERROR_INTERNAL;
    }

    *out_matches = auth_core_constant_time_equal(
        derived,
        record->hash,
        sizeof(derived));
    core->ops.secure_zero(core->ops.context,
                          derived,
                          sizeof(derived));
    return APP_ERROR_NONE;
}
```

### 10.2 Correct login behavior

Use:

```c
bool password_matches = false;
const app_error_code_t verify_result =
    auth_password_verify(password->valuestring,
                         strlen(password->valuestring),
                         &server_configuration.password_record,
                         &password_matches);
if (verify_result != APP_ERROR_NONE) {
    return send_error(request,
                      "500 Internal Server Error",
                      verify_result,
                      "authentication subsystem unavailable");
}
if (!password_matches) {
    const app_error_code_t record_result =
        auth_login_record_failure();
    if (record_result != APP_ERROR_NONE) {
        return send_error(request,
                          "500 Internal Server Error",
                          record_result,
                          "could not record login failure");
    }
    return send_error(request,
                      "401 Unauthorized",
                      APP_ERROR_AUTH_FAILED,
                      "invalid credentials");
}
```

- [x] Do not increment failure count on PBKDF2 failure.
- [x] Do not return 401 on password-record corruption.
- [x] Add tests for all result combinations.

## 11. Fix Wi-Fi cleanup

### 11.1 Continue after cleanup errors

Replace early-return cleanup with accumulation:

```c
static void record_first_error(app_error_code_t candidate,
                               app_error_code_t *first_error) {
    if (candidate != APP_ERROR_NONE &&
        *first_error == APP_ERROR_NONE) {
        *first_error = candidate;
    }
}

static app_error_code_t cleanup_resources(wifi_ap_engine_t *engine) {
    app_error_code_t first_error = APP_ERROR_NONE;

    if (engine->wifi_started) {
        const app_error_code_t result =
            engine->operations.wifi_stop(
                engine->operations.context);
        record_first_error(result, &first_error);
        if (result == APP_ERROR_NONE) {
            engine->wifi_started = false;
        }
    }

    if (engine->handler_registered) {
        const app_error_code_t result =
            engine->operations.handler_unregister(
                engine->operations.context);
        record_first_error(result, &first_error);
        if (result == APP_ERROR_NONE) {
            engine->handler_registered = false;
        }
    }

    if (engine->wifi_initialized) {
        const app_error_code_t result =
            engine->operations.wifi_deinit(
                engine->operations.context);
        record_first_error(result, &first_error);
        if (result == APP_ERROR_NONE) {
            engine->wifi_initialized = false;
        }
    }

    if (engine->netif_created) {
        const app_error_code_t result =
            engine->operations.netif_destroy(
                engine->operations.context);
        record_first_error(result, &first_error);
        if (result == APP_ERROR_NONE) {
            engine->netif_created = false;
        }
    }

    return first_error;
}
```

### 11.2 Add ownership query

Add:

```c
bool wifi_ap_owns_resources(void);
```

Return true when any engine resource flag is set.

### 11.3 Add tests

For each cleanup operation:

- [x] fail it;
- [x] assert later cleanup operations still run;
- [x] assert only successful ownership flags clear;
- [x] assert retry cleans residual ownership;
- [x] assert original start error remains visible.

## 12. Fix executor shutdown and terminal integrity

### 12.1 Add cooperative stop message

Change the queue payload from raw execution request to a tagged message:

```c
typedef enum {
    MACRO_EXECUTOR_MESSAGE_EXECUTE = 0,
    MACRO_EXECUTOR_MESSAGE_STOP,
} macro_executor_message_type_t;

typedef struct {
    macro_executor_message_type_t type;
    macro_execution_request_t request;
} macro_executor_message_t;
```

Create a binary semaphore:

```c
static SemaphoreHandle_t executor_stopped;
```

Task loop shape:

```c
static void executor_task(void *context) {
    (void)context;
    macro_executor_message_t message = {0};

    for (;;) {
        if (xQueueReceive(request_queue,
                          &message,
                          portMAX_DELAY) != pdTRUE) {
            continue;
        }

        if (message.type == MACRO_EXECUTOR_MESSAGE_STOP) {
            break;
        }

        const app_error_code_t result =
            macro_executor_engine_execute(
                &engine,
                &message.request);
        if (result != APP_ERROR_NONE) {
            ESP_LOGE(TAG,
                     "execution ended with %s",
                     app_error_code_string(result));
        }
    }

    executor_task_handle = NULL;
    (void)xSemaphoreGive(executor_stopped);
    vTaskDelete(NULL);
}
```

Do not leave the semaphore result ignored in final code. If the give can fail,
record it in executor health before task exit.

### 12.2 Implement deinit

`macro_executor_deinit()` must:

- [x] reject new submissions;
- [x] request cancellation;
- [x] enqueue or notify stop;
- [x] wait with a bounded timeout;
- [x] release all USB keys;
- [x] free any owned plan;
- [x] delete queue and semaphores after task exit;
- [x] clear handles and engine state;
- [x] retain release and shutdown errors.

### 12.3 Remove ignored `finish_execution`

Replace:

```c
(void)finish_execution(...);
```

with explicit result handling. If terminal publication or reset fails:

- [x] return that failure;
- [x] retain the primary execution failure in status/health;
- [x] leave executor unavailable rather than falsely idle;
- [x] require deinit/restart or explicit recovery.

### 12.4 Add terminal states

Add `EXECUTION_TIMED_OUT` or retain `EXECUTION_FAILED` with a required
`APP_ERROR_TIMEOUT` mapping. The API and frontend must distinguish timeout.

- [ ] Add execution ID and object identity to status. (execution_id already
      present; set_id/macro_id identity deferred to the API/diagnostics phases,
      16/19, where it is consumed and its JSON shape is designed.)
- [ ] Add accepted, started, and completed timestamps. (Observability metadata
      deferred to Phase 16/19 with its consumer.)
- [ ] Add current action summary. (Observability metadata deferred to Phase 16/19
      with its consumer.)
- [x] Add tests for key-release failure after otherwise successful execution.

## 13. Fix device-controls shutdown and failure visibility

### 13.1 Track controls task handle and stop request

Add:

```c
static TaskHandle_t controls_task_handle;
static SemaphoreHandle_t controls_stopped;
static volatile bool controls_stop_requested;
```

The task must break only after observing the stop request, set a health result,
signal completion, and delete itself.

### 13.2 Add controls health

Define:

```c
typedef struct {
    app_error_code_t last_error;
    app_error_code_t cleanup_error;
    bool task_running;
    bool indicator_output_failed;
    bool confirmation_signal_failed;
    bool cancel_request_failed;
} device_controls_health_t;
```

Add:

```c
device_controls_health_t device_controls_get_health(void);
```

- [x] Update health atomically.
- [x] Log failures through ESP logging.
- [ ] Expose redacted health through diagnostics. (Remains open for Phase 19.)

### 13.3 Implement deinit

- [x] request task stop;
- [x] wait with bounded timeout;
- [x] configure outputs to a documented safe state;
- [x] delete semaphores after task exit;
- [x] clear handles;
- [x] return cleanup failure if any step fails.

### 13.4 Add tests

Test:

- [x] semaphore give failure;
- [x] cancel failure;
- [x] GPIO output failure;
- [x] task stop timeout;
- [x] second deinit call;
- [x] no use-after-free after deinit.

## 14. Implement encrypted persistent provisioning

### 14.1 Update partition table

Update:

```text
firmware/partitions.csv
scripts/check-partitions.sh
```

Add a 4 KiB `nvs_keys` partition with encrypted flag. Recalculate offsets and
verify both OTA slots still satisfy the firmware budget.

Example row:

```csv
nvs_keys,    data, nvs_keys, ,         0x1000,   encrypted
```

- [x] Verify total flash size.
- [x] Verify application slots remain aligned.
- [x] Add partition tests that require exactly one `nvs_keys` partition.

### 14.2 Enable NVS encryption configuration

Update:

```text
firmware/sdkconfig.defaults
```

Enable the chosen IDF `v5.5.5` NVS encryption scheme.

- [x] Document whether release uses flash-encryption-based or HMAC-based key
      protection.
- [x] Do not claim physical confidentiality until the matching eFuse/flash
      workflow is tested.
- [x] Add release checks that reject an unencrypted production configuration.

### 14.3 Add provisioning repository

Create a dedicated component or storage module:

```text
firmware/components/provisioning/
```

Suggested public API:

```c
typedef struct {
    uint32_t schema_version;
    uint32_t revision;
    bool provisioned;
    char ap_ssid[WIFI_AP_SSID_MAX_BYTES + 1U];
    char ap_passphrase[WIFI_AP_PASSPHRASE_MAX_BYTES + 1U];
    auth_password_record_t password_record;
    bool require_physical_confirmation;
    bool always_select_set;
    bool has_active_set;
    app_uuid_t active_set_id;
} provisioning_config_t;

app_error_code_t provisioning_init(void);
app_error_code_t provisioning_load(
    provisioning_config_t *out_config);
app_error_code_t provisioning_commit(
    const provisioning_config_t *replacement,
    uint32_t expected_revision,
    provisioning_config_t *out_committed);
app_error_code_t provisioning_clear_credentials(void);
app_error_code_t provisioning_factory_reset(void);
app_error_code_t provisioning_deinit(void);
```

Requirements:

- [x] strict schema and bounds;
- [x] NVS transaction with `nvs_commit()`;
- [x] readback validation;
- [x] no plaintext administrator password;
- [x] secure zero of temporary credential buffers;
- [x] sessions remain RAM-only;
- [x] revision conflict behavior.

### 14.4 Remove ordinary plaintext credential logs

Update:

```text
firmware/components/app_core/app_core.c
firmware/main/Kconfig.projbuild
scripts/check-firmware.sh
```

- [x] Remove AP and web password printing from ordinary development mode.
- [x] Add a separate manufacturing-only option if still required.
- [x] Make the release check fail when that option is enabled.
- [x] Add source scanning for credential log format strings.

### 14.5 Implement setup flow

Add setup routes that exist only when unprovisioned:

- [x] setup-state read;
- [x] setup credential submission;
- [x] setup completion;
- [x] restart.

Require physical confirmation or the explicitly gated manufacturing mode.

- [x] Test interruption before and after every NVS commit/readback step.

## 15. Complete storage object repositories

### 15.1 Macro repository

Implement files under:

```text
/data/sets/<set-id>/macros/<macro-id>.json
/data/sets/<set-id>/macro-order.json
/data/global/macros/<macro-id>.json
/data/global/macro-order.json
```

Add:

- [x] list;
- [x] create;
- [x] read;
- [x] update with expected revision;
- [x] delete;
- [x] duplicate;
- [x] reorder;
- [x] validate references;
- [x] quarantine corrupt objects.

### 15.2 Procedure repository

Implement:

```text
/data/sets/<set-id>/procedures/<procedure-id>.json
/data/sets/<set-id>/procedure-order.json
```

Validate:

- [x] step IDs unique;
- [x] macro references exist and match scope;
- [x] no duplicate completion/skip IDs;
- [x] step count and text bounds;
- [x] exact fields;
- [x] revision conflicts.

### 15.3 Progress repository

Implement:

```text
/data/sets/<set-id>/progress/<procedure-id>.json
```

- [x] current step must belong to the referenced procedure revision;
- [x] completed and skipped arrays must contain only existing step IDs;
- [x] no ID may be both completed and skipped;
- [x] procedure revision changes must produce visible stale-progress handling;
- [x] reset is atomic.

### 15.4 Settings and active-set repository

Persist non-secret user settings in the chosen protected configuration store.

- [x] always ask for set;
- [x] optional active set;
- [x] physical confirmation requirement;
- [x] UI preferences only when specified by `docs/SPEC.md`.

### 15.5 Delete/reference behavior

Before deleting a macro:

- [x] scan procedure references under the repository lock;
- [x] return `APP_ERROR_CONFLICT` when referenced;
- [x] include bounded referencing IDs in API details;
- [x] never silently rewrite a procedure.

Before deleting a set:

- [x] move the set to transaction-owned trash;
- [x] remove it from index;
- [x] preserve global macros;
- [x] clear active-set selection if it points to the deleted set;
- [x] recover deterministically after interruption.

Phase 15 implementation notes:

- Macro and procedure CRUD/order/reference/quarantine suites are registered as real CTest targets;
  the previously unregistered source-only tests no longer provide false confidence.
- Progress reads return an explicit `CURRENT` or `STALE` status and the current procedure
  revision; stale records remain visible and can only be reset against the current revision.
- Non-secret settings and active-set selection remain in the encrypted provisioning record
  through a redacted settings API. Set deletion clears a matching active set before the
  LittleFS transaction, preventing a power loss from leaving a dangling active-set ID.

## 16. Complete the HTTP API

### 16.1 Add centralized request policy

Create a reusable request-policy adapter that checks:

- [x] Content-Type;
- [x] body limit;
- [x] Host;
- [x] Origin;
- [x] cookie;
- [x] CSRF;
- [x] session;
- [x] request ID;
- [x] route-specific physical confirmation.

Do not duplicate security checks across every route.

### 16.2 Add path parameter parsing

- [x] strict UUID only;
- [x] no decoded slash;
- [x] no path traversal;
- [x] bounded route buffers;
- [x] unknown fields rejected in JSON bodies.

### 16.3 Implement set routes

Implement all set routes from the FIX1 specification.

For every mutation:

- [x] require expected revision;
- [x] map conflict to HTTP 409;
- [x] map storage full to HTTP 507;
- [x] return committed object readback;
- [x] test partial body and malformed JSON.

### 16.4 Implement macro routes

- [x] set-local CRUD;
- [x] global CRUD;
- [x] ordering;
- [x] validation without execution;
- [x] reference-conflict details;
- [x] exact action count and duration.

### 16.5 Implement procedure/progress routes

- [x] CRUD and order;
- [x] progress read;
- [x] complete step;
- [x] skip with explicit confirmation;
- [x] reset progress;
- [x] stale revision handling.

### 16.6 Implement server-owned execution submission

Add:

```text
POST /api/v1/executions
```

Request type:

```c
typedef struct {
    app_uuid_t set_id;
    app_uuid_t macro_id;
    uint32_t macro_revision;
    bool has_procedure_context;
    app_uuid_t procedure_id;
    app_uuid_t step_id;
} web_execution_submit_request_t;
```

Handler sequence:

1. authorize mutation;
2. parse and validate identity;
3. acquire repository read/mutation boundary;
4. load persisted macro;
5. verify set/scope and exact revision;
6. compile full source;
7. release repository lock after plan ownership is independent;
8. generate execution ID;
9. submit plan to executor;
10. clear local ownership only after executor accepts it;
11. return `202` with execution identity.

Ownership pattern:

```c
macro_plan_t plan = {0};
bool plan_owned = false;

app_error_code_t result =
    macro_compile(macro.source,
                  macro.source_length,
                  &options,
                  &plan,
                  &parse_error);
if (result == APP_ERROR_NONE) {
    plan_owned = true;
}

macro_execution_request_t execution = {
    .plan = plan,
    .execution_id = execution_id,
    .set_id = request.set_id,
    .macro_id = request.macro_id,
    .macro_revision = request.macro_revision,
    .key_press_ms = macro.key_press_ms,
    .inter_key_ms = macro.inter_key_ms,
};

if (result == APP_ERROR_NONE) {
    result = macro_executor_submit(&execution);
    if (result == APP_ERROR_NONE) {
        plan_owned = false;
    }
}

if (plan_owned) {
    macro_plan_free(&plan);
}
```

- [x] Do not accept macro source in the request.
- [x] Return `202` only after executor ownership transfer.
- [x] Add tests for stale revision, missing object, USB unavailable, busy
      executor, compile failure, queue failure, and ownership cleanup.

### 16.7 Correct cancellation status mapping

Map:

- no current execution: `404`;
- terminal execution: `409`;
- cancellation already requested: `409`;
- internal executor failure: `500`;
- unavailable executor: `503`;
- accepted cancellation: `202`.

### 16.8 Add administration routes

Implement settings, storage health, quarantine, the Phase 19 diagnostics boundary,
the Phase 18 export/import/backup/restore boundaries, restart, credential reset,
and factory reset. Package operations must remain explicit 503 responses until
Phase 18 owns their validated transaction.

Destructive operations must enforce reauthentication or physical confirmation
as specified.

### 16.9 Expand API tests

Every documented route needs:

- [x] success test;
- [x] authentication failure;
- [x] CSRF failure;
- [x] Origin failure;
- [x] Host failure;
- [x] Content-Type failure;
- [x] oversized body;
- [x] malformed JSON;
- [x] unknown field;
- [x] storage failure;
- [x] revision conflict;
- [x] redaction assertion.

Phase 16 completion evidence:

- centralized path, body, Host, Origin, cookie, session, CSRF, request-ID, and
  physical-confirmation policy is shared by wildcard API routes;
- repository-backed acceptance tests exercise set, macro, procedure, and progress
  success, malformed/unknown input, revision conflict, reference conflict, stale
  progress, redaction, and durable readback;
- execution tests cover stale/missing objects, procedure mismatch, compile failure,
  UUID/queue failure, USB unavailable, busy executor, ownership transfer, and exact
  cancellation admission/status mapping;
- import/export/backup/restore are intentionally Phase 18 boundaries and return 503
  rather than false success; full diagnostics aggregation remains Phase 19.

## 17. Replace frontend mock behavior

### 17.1 Split `App.tsx`

Move route-specific logic into:

```text
webapp/src/features/auth/
webapp/src/features/execution/
webapp/src/features/macros/
webapp/src/features/procedures/
webapp/src/features/sets/
webapp/src/features/settings/
webapp/src/pages/
```

Keep `App.tsx` responsible for routing, session boundary, and global shell only.

**Implemented foundation:** feature-owned setup/authentication, set selection, settings, and execution views now sit behind a small routing and shell coordinator.

### 17.2 Runtime-validate API responses

Add explicit guards or a small pinned schema library.

Example guard:

```ts
export function isExecutionStatus(
  value: unknown,
): value is ExecutionStatus {
  if (!isRecord(value)) {
    return false;
  }

  const states = new Set([
    "idle",
    "running",
    "completed",
    "cancelled",
    "failed",
    "timed_out",
  ]);

  return (
    typeof value.executionId === "string" &&
    states.has(value.state) &&
    typeof value.error === "string" &&
    typeof value.releaseError === "string" &&
    typeof value.actionIndex === "number" &&
    Number.isInteger(value.actionIndex) &&
    typeof value.actionCount === "number" &&
    Number.isInteger(value.actionCount)
  );
}
```

Do not use `value.data as T` without route-specific validation.

**Implemented foundation:** `apiRequest` requires a route validator; exact guards cover setup, status, session, settings, sets, cancellation, and execution payloads. Invalid 2xx payloads fail closed as `invalid_response`.

### 17.3 Implement real session boundary

- [x] load status/session on startup;
- [x] redirect unprovisioned devices to setup;
- [x] redirect expired sessions to login;
- [x] clear CSRF token on logout or 401;
- [x] display rate-limit retry time;
- [x] stop authenticated polling after session expiry.

### 17.4 Implement real set selection

- [x] load sets;
- [x] recents;
- [x] search;
- [x] metadata;
- [x] explicit active set;
- [x] no hardcoded HP model;
- [x] active set shown in the header from server state.

### 17.5 Implement real macro editor

- [x] load/create/update;
- [x] source byte count;
- [x] live server validation;
- [x] action count;
- [x] estimated duration;
- [x] exact parse location;
- [x] directive insertion;
- [x] disable Save when invalid;
- [x] stale-revision conflict UI.

**Implemented:** the active-set macro library and editor use exact runtime-validated
firmware resources, expected-revision PUTs, UTF-8 byte counts, and a debounced
compile-only validation request. Metrics and parser coordinates are server-owned;
Save remains disabled unless the successful validation fingerprint matches the
current draft. A 409 never overwrites local source and requires an explicit reload
of the latest server revision.

### 17.6 Implement real procedure workflow

- [x] list and progress;
- [x] current step;
- [x] previous/next;
- [x] instruction completion;
- [x] checkpoint confirmation;
- [x] resend;
- [x] skip confirmation;
- [x] reset;
- [x] no automatic next execution.

**Implemented:** the active-set procedure library loads persisted summaries and
independent progress, with a missing progress resource represented explicitly as
not started. The workflow loads exact procedure and progress resources, validates
their identities and revisions, keeps future steps visible, and supports
previous/next review. Instruction completion, checkpoint confirmation, confirmed
skip, and reset use the dedicated progress endpoints. Stale progress is never
silently reconciled. Macro Send/Resend stops at the Phase 17.7 confirmation route,
and no progress action submits or automatically starts an execution.

### 17.7 Implement execution confirmation and submission

The confirmation page must use live data and disable Send unless:

- [x] active set is loaded;
- [x] macro is loaded and validated;
- [x] revision is current;
- [x] USB state is `ready`;
- [x] executor is not busy;
- [x] physical confirmation policy can be satisfied.

Send handler:

```ts
async function submitExecution(
  request: ExecutionSubmitRequest,
): Promise<ExecutionAccepted> {
  return apiRequest(
    "/api/v1/executions",
    {
      method: "POST",
      body: JSON.stringify(request),
    },
    isExecutionAccepted,
  );
}
```

Update `apiRequest` to accept a validator:

```ts
export async function apiRequest<T>(
  path: string,
  init: RequestInit,
  validate: (value: unknown) => value is T,
): Promise<T> {
  // Existing same-origin, timeout, and envelope handling.
  const envelope = parseEnvelope(response.status, value);
  if (!response.ok || !envelope.ok) {
    // Existing error handling.
  }
  if (!validate(envelope.data)) {
    throw invalidResponse(
      response.status,
      "The device returned an invalid response payload.",
    );
  }
  return envelope.data;
}
```

**Implemented:** confirmation routes accept either exactly one macro ID or a
complete procedure/step context. The page loads and validates the persisted
set-local or global macro, verifies the exact procedure macro step, and shows
the server-owned source, revision, action count, duration, and key timings.
Navigation never executes. Explicit Send performs a second preflight of the
active set, settings, macro, validation result, procedure context, USB state,
and executor state. Drift fails closed. When configured, the page visibly waits
for the device button using a bounded 25-second client timeout around the
firmware's 20-second confirmation window. Submission uses nested
`sourceContext`; execution polling retains the accepted execution ID and refuses
to display or cancel a different current execution. Tests cover malformed and
partial routes, preview-only navigation, USB blocking, global-macro fallback,
preflight drift, physical-confirmation state, exact nested submission, timeout,
and execution-identity mismatch.

### 17.8 Correct result labeling

Use an exhaustive function:

```ts
function executionResultTitle(
  execution: ExecutionStatus,
): string {
  if (execution.releaseError.length > 0) {
    return "Macro ended with a key-release error";
  }

  switch (execution.state) {
    case "completed":
      return "Macro completed";
    case "cancelled":
      return "Macro cancelled";
    case "failed":
      return "Macro failed";
    case "timed_out":
      return "Macro timed out";
    case "idle":
    case "running":
      throw new Error(
        "Execution result requested for a non-terminal state.",
      );
  }
}
```

Update `webapp/tests/app-execution.test.tsx` so cancellation expects
`Macro cancelled`, not `Macro finished`.

**Implemented:** terminal labels are exhaustive for completed, cancelled, failed, timed out, and key-release failure. Polling stops on terminal state, route exit, unmount, or session expiry.

### 17.9 Implement real management screens

- [x] create/edit/duplicate/reorder/delete sets;
- [x] import-as-new boundary screen, disabled until Phase 18;
- [x] transactional-replace boundary screen, disabled until Phase 18;
- [x] export boundary screen, disabled until Phase 18;
- [x] settings;
- [x] backup/restore boundary screens, disabled until Phase 18;
- [x] diagnostics;
- [x] quarantine view;
- [x] restart/reset workflows.

No enabled button may be inert.

**Implemented:** every enabled management control performs a real request or
navigation. Set create, edit, duplicate, keyboard-accessible reorder, and
guarded delete use live revisioned APIs. Settings, storage health, redacted
quarantine evidence, restart, settings reset, and factory reset use strict
response guards and visible physical-confirmation waits. Deterministic set
export, transactional replacement, full backup, and all-or-nothing restore now
use the completed Phase 18 services. Import-as-new remains an honest disabled
boundary until its identity-rewrite transaction is implemented; the frontend
never simulates success or sends an unsupported mutation.

### 17.10 Add accessibility and browser tests

- [x] keyboard-only navigation;
- [x] focus trap and restoration;
- [x] screen-reader live regions;
- [x] reorder alternatives;
- [x] touch target checks;
- [x] no color-only status;
- [x] offline/reconnect;
- [x] full execution workflow.

**Implemented:** the permanent read-only Browser Tests workflow builds the
production bundle and drives Chrome through the DevTools Protocol. It proves
native keyboard activation, modal focus wrapping and restoration, visible
status text, 44 by 44 CSS-pixel targets, explicit reorder controls, offline and
reconnect announcements with live refresh, and the complete persisted-macro
preview, submit, poll, and terminal-result workflow. The gate fails if Chrome
is unavailable rather than silently falling back to a DOM simulator.

## 18. Complete import, export, backup, and restore

### 18.1 Implement bounded package reader

- [x] enforce `APP_IMPORT_PACKAGE_MAX_BYTES`;
- [x] stream or use bounded allocation;
- [x] reject trailing data;
- [x] reject unknown schema fields;
- [x] validate all objects before mutation.

Implemented: `storage_package_validate()` is a zero-copy package reader over the
bounded request buffer. It rejects packages above `APP_IMPORT_PACKAGE_MAX_BYTES`
before parsing; scans JSON with an iterative, depth-bounded state machine; allocates
only count-bounded validation metadata; requires the complete input to contain one
package document with no non-whitespace trailing bytes; rejects duplicate, unknown,
or future fields; and validates every set, macro, procedure, progress object, and
cross-object reference before returning success. The validation component has no
repository mutation dependency, so Phase 18.3 remains the first code permitted to
activate imported state.

### 18.2 Implement export

- [x] include referenced set-local and global macros;
- [x] include procedures and optional progress;
- [x] exclude every secret;
- [x] deterministic ordering;
- [x] stable schema version;
- [x] exact content length or bounded chunked response.

Implemented: the set export route builds a bounded, versioned package from one
repository-locked snapshot; includes all set-local macros and only referenced
global macros; includes procedures and optional current progress; excludes
provisioning, session, credential, and encryption stores by construction;
revalidates the serialized package; and transfers the exact byte length to the
HTTP response.

### 18.3 Implement transactional replace

Add a transaction type and recovery code that handles every phase.

- [x] stage complete replacement;
- [x] validate readback;
- [x] back up current set;
- [x] activate replacement;
- [x] update index;
- [x] validate active set;
- [x] remove backup and manifest;
- [x] recover after every interrupted phase.

### 18.4 Implement full backup and restore

- [x] backup all sets, global macros, procedures, and optional progress;
- [x] exclude credentials and sessions;
- [x] restore all-or-nothing;
- [x] require physical/admin confirmation;
- [x] test storage full during staging;
- [x] test power loss after every phase.

Implemented: full backup serializes every logical repository object from one
locked snapshot, remains bounded by `APP_IMPORT_PACKAGE_MAX_BYTES`, and excludes
provisioning, credentials, sessions, CSRF material, encryption keys, quarantine,
schema markers, and transaction evidence by construction. Restore validates the
complete package before mutation, stages and validates a full replacement tree,
and activates only `set-index.json`, `sets/`, and `global/` through a durable
six-phase restore transaction. Startup resolves restore manifests before ordinary
set transactions. Host tests cover every durable phase, partial renames,
idempotent recovery, contradictory evidence, deterministic `STORAGE_FULL` during
staging, and preservation of the complete old repository. The API requires an
authenticated administrator session, CSRF, same-origin policy, and physical
confirmation. The frontend adds strict file validation, the exact typed phrase
`RESTORE FULL BACKUP`, visible device-confirmation state, and a mandatory reload
after success.

### 18.5 Secret scanner tests

Generate known sentinel secrets and assert they do not occur in:

- [ ] set export;
- [ ] full backup;
- [ ] diagnostics;
- [ ] logs;
- [ ] frontend persisted state.

## 19. Diagnostics and observability

### 19.1 Add subsystem health records

Add stable health snapshots for:

- [ ] app lifecycle;
- [ ] storage mount/recovery;
- [ ] repository;
- [ ] authentication;
- [ ] USB;
- [ ] executor;
- [ ] controls;
- [ ] Wi-Fi;
- [ ] HTTP.

Retain primary and cleanup errors separately.

### 19.2 Add redacted diagnostics route

Include:

- [ ] build ID;
- [ ] firmware and schema versions;
- [ ] reset reason and uptime;
- [ ] heap;
- [ ] task stack high-water marks;
- [ ] webfs/userdata capacity;
- [ ] quarantine count;
- [ ] current execution state;
- [ ] subsystem health.

Exclude all secret material and raw macro source.

### 19.3 Add diagnostic tests

- [ ] exact allowed fields;
- [ ] secret sentinels absent;
- [ ] bounded output;
- [ ] behavior when a subsystem health query fails;
- [ ] no false healthy state when cleanup is incomplete.

## 20. Hardware and integration validation

### 20.1 Clean production build

Run from a clean checkout:

```bash
./scripts/check-all.sh
idf.py -C firmware fullclean
idf.py -C firmware build
./scripts/build-device-tests.sh
```

Record:

- [ ] exact commit;
- [ ] IDF version;
- [ ] component lock hash;
- [ ] application binary size;
- [ ] partition headroom;
- [ ] webfs image size;
- [ ] static RAM;
- [ ] peak heap;
- [ ] task stack high-water marks.

### 20.2 USB host matrix

Run and record Linux and ChromeOS:

- [ ] enumeration;
- [ ] disconnect/reconnect;
- [ ] suspend/resume;
- [ ] printable text;
- [ ] chords;
- [ ] delay cancellation;
- [ ] rapid typing cancellation;
- [ ] disconnect during execution;
- [ ] release-all observation.

### 20.3 SoftAP/browser integration

Test:

- [ ] first-run setup;
- [ ] encrypted persistence after reboot;
- [ ] login;
- [ ] rate limiting;
- [ ] Host/Origin behavior;
- [ ] session expiry;
- [ ] CRUD;
- [ ] execution;
- [ ] cancellation;
- [ ] offline/reconnect.

### 20.4 Power interruption

Interrupt real hardware after each storage transaction phase.

- [ ] old or new complete state;
- [ ] no automatic format;
- [ ] no mixed active set;
- [ ] recoverable diagnostics;
- [ ] evidence preserved for ambiguous corruption.

### 20.5 Physical controls

Measure:

- [ ] cancellation latency during 10-second delay;
- [ ] cancellation latency during rapid typing;
- [ ] confirmation timeout;
- [ ] reset gesture duration;
- [ ] accidental short-press rejection.

## 21. Release budgets and immutable CI

### 21.1 Add size gates

Create a script such as:

```text
scripts/check-release-budgets.sh
```

Fail when:

- [ ] application exceeds configured OTA slot budget;
- [ ] webfs image exceeds partition budget;
- [ ] minimum userdata free-space requirement is impossible;
- [ ] static RAM exceeds budget;
- [ ] measured task stack margin is below threshold.

### 21.2 Pin GitHub Actions

- [ ] replace action tags with full commit SHAs;
- [ ] comment the human-readable version;
- [ ] document runner/tool versions;
- [ ] retain least-privilege permissions;
- [ ] keep concurrency cancellation.

### 21.3 Add production-configuration gate

Fail release when:

- [ ] manufacturing credential logging enabled;
- [ ] NVS encryption disabled;
- [ ] required security configuration absent;
- [ ] development-only setup bypass enabled;
- [ ] debug server or remote assets enabled.

## 22. Synchronize documentation

Update:

```text
README.md
docs/API.md
docs/DEVELOPMENT.md
docs/IMPLEMENTATION_STATUS.md
docs/SECURITY_REVIEW.md
docs/RECOVERY.md
docs/HARDWARE_TEST_PLAN.md
docs/RELEASE_NOTES.md
docs/TODO.md
docs/UNIT_TESTS1_TODO.md
```

- [ ] Remove stale claim that `firmware/dependencies.lock` is missing.
- [ ] Correct stale claim that set CRUD is not implemented.
- [ ] Clearly distinguish implemented, host-tested, device-tested, and
      release-ready behavior.
- [ ] Include exact evidence commit/run for each completed release gate.
- [ ] Do not reference missing companion files.
- [ ] Keep historical findings but mark when and how they were fixed.

## 23. Final regression and acceptance gate

Run from a clean checkout:

```bash
./scripts/check-all.sh
./scripts/run-tests.sh --sanitizers
./scripts/generate-native-coverage.sh
./scripts/generate-frontend-coverage.sh
./scripts/build-device-tests.sh
./scripts/check-release-budgets.sh
```

Then run all hardware and browser integration tests.

The final acceptance checklist is:

### Quality

- [ ] analyzer infrastructure failure makes CI fail;
- [ ] no first-party warning or suppression;
- [ ] formatting, lint, typecheck, and tests clean;
- [ ] sanitizer and leak checks clean;
- [ ] coverage gates pass without exclusions.

### Lifecycle

- [ ] startup failure after every stage cleans all owned resources;
- [ ] cleanup continues after a cleanup error;
- [ ] residual ownership is visible;
- [ ] production provisioning check occurs before normal-operation tasks.

### Storage

- [ ] atomic artifacts recover deterministically;
- [ ] transaction manifests recover their own interrupted writes;
- [ ] quarantine is recoverable;
- [ ] repository mutations are serialized;
- [ ] concurrent stale revisions cannot both succeed;
- [ ] power loss yields old or new complete state.

### Security

- [ ] encrypted persistent provisioning works;
- [ ] no plaintext credentials in ordinary logs;
- [ ] crypto failure is not reported as bad password;
- [ ] mutations require auth, CSRF, Host, Origin, and Content-Type;
- [ ] exports, backups, and diagnostics contain no secrets.

### Execution

- [ ] server loads persisted macro by ID and revision;
- [ ] client cannot submit arbitrary source for execution;
- [ ] `202` occurs only after executor ownership transfer;
- [ ] cancellation is not labeled success;
- [ ] release error is visible;
- [ ] no next step executes automatically.

### Product workflows

- [ ] setup;
- [ ] login/logout/session expiry;
- [ ] set selection;
- [ ] set CRUD and ordering;
- [ ] macro CRUD, validation, and ordering;
- [ ] procedure CRUD and progress;
- [ ] execution and cancellation;
- [ ] import/export;
- [ ] backup/restore;
- [ ] settings;
- [ ] diagnostics and quarantine.

### Hardware

- [ ] Linux USB matrix;
- [ ] ChromeOS USB matrix;
- [ ] SoftAP/browser integration;
- [ ] encrypted NVS reboot persistence;
- [ ] power interruption;
- [ ] storage full;
- [ ] cancellation latency;
- [ ] release-all observation.

### Release

- [ ] firmware/webfs/heap/stack budgets enforced;
- [ ] CI actions pinned;
- [ ] production configuration rejects development bypasses;
- [ ] documentation matches implementation;
- [ ] every FIX1 checkbox has evidence.

## 24. Completion rule

Do not declare FIX1 complete while any checkbox above remains open.

Do not replace an open checkbox with a note that the behavior is "acceptable
for now." Any intentional deferral requires a new explicit specification,
release-scope decision, and visible product limitation. It must not be hidden
behind a passing test or static UI.
