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

- [ ] Ensure no duplicate non-static symbol is introduced.
- [ ] Ensure coverage includes the new source files.
- [ ] Ensure host tests compile the same production translation units.

### 2.4 Phase 2 gate

Run:

```bash
./scripts/check-format.sh
./scripts/check-firmware.sh
./scripts/check-scripts.sh
./scripts/run-tests.sh
```

- [ ] Deliberately break the analyzer command and verify the gate fails.
- [ ] Deliberately emit one first-party warning and verify the gate fails.
- [ ] Restore the tree and verify all checks pass.

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

- [ ] Add host unit tests.
- [ ] Add support component CMake entries.
- [ ] Do not use this as an excuse to collapse stable API errors.

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

- [ ] Ensure logs never include credentials, tokens, cookies, or macro source.
- [ ] Add exact host assertions for event ordering and contents.

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

- [ ] initialize NVS;
- [ ] mount and recover storage;
- [ ] initialize repositories;
- [ ] initialize authentication;
- [ ] load and validate persistent provisioning state.

If production provisioning is incomplete:

- [ ] enter the explicit setup mode; or
- [ ] cleanly stop and report provisioning required.

Do not initialize normal-operation tasks and then return
`APP_ERROR_AUTH_REQUIRED`.

### 4.6 Add failure-injection tests

Update:

```text
tests/host/test_app_core.c
```

For every startup stage:

- [ ] inject primary failure;
- [ ] assert exact cleanup order;
- [ ] inject cleanup failure at each later stage;
- [ ] assert all remaining stages are still attempted;
- [ ] assert primary and cleanup errors are both retained;
- [ ] assert no owned resource remains when cleanup succeeds;
- [ ] assert residual ownership is visible when cleanup fails.

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

- [ ] keep the handle;
- [ ] retain the registered-route count;
- [ ] retain cleanup error;
- [ ] return a structured or stable failure;
- [ ] ensure `app_core` sees ownership and calls stop again.

### 5.3 Make stop idempotent

- [ ] `web_server_stop()` returns success when no handle exists.
- [ ] A successful stop clears configuration and lifecycle state.
- [ ] A failed stop retains all state needed for retry.
- [ ] No later start is allowed while a residual handle exists.

### 5.4 Add tests

Test:

- [ ] start failure before handle creation;
- [ ] registration failure plus successful stop;
- [ ] registration failure plus failed stop;
- [ ] retry stop after partial start;
- [ ] successful retry clears state;
- [ ] start rejected while residual state remains.

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
- [ ] Reject symlink-like or unsupported types in host tests.

### 6.3 Add mount rollback tests

Test:

- [ ] web mount fails;
- [ ] data mount fails and web unmount succeeds;
- [ ] data mount fails and web unmount fails;
- [ ] directory creation fails after both mounts;
- [ ] unmount continues for both partitions after one failure;
- [ ] regular file collides with every required directory.

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

- [ ] Reject path traversal.
- [ ] Reject empty destination.
- [ ] Reject duplicate artifact paths.
- [ ] Reject cross-directory destination reconstruction.

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

- [ ] transaction manifest;
- [ ] schema marker;
- [ ] set index;
- [ ] global macro index;
- [ ] set metadata;
- [ ] macro object;
- [ ] procedure object;
- [ ] progress object;
- [ ] settings object;
- [ ] quarantine record.

### 7.3 Implement reconciliation rules

Add deterministic tests for all canonical/temporary/backup combinations.

Required conservative behavior:

- [ ] restore a valid backup when canonical is absent;
- [ ] preserve the old committed state when operation intent is ambiguous;
- [ ] activate a temporary only when the owning transaction proves roll-forward;
- [ ] quarantine malformed or conflicting artifacts;
- [ ] check every rename, unlink, close, and parent sync;
- [ ] retain recovery evidence when cleanup fails.

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

- [ ] temporary open;
- [ ] partial write;
- [ ] file sync;
- [ ] close;
- [ ] readback;
- [ ] destination-to-backup rename;
- [ ] first parent sync;
- [ ] temporary-to-destination rename;
- [ ] second parent sync;
- [ ] backup removal;
- [ ] final parent sync.

Assert old or new complete state, never ambiguous active state.

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

- [ ] finish a provably complete staged quarantine;
- [ ] restore the source when activation never occurred and restoration is safe;
- [ ] preserve ambiguous staging as evidence;
- [ ] never delete an unmatched evidence file;
- [ ] never make the entire quarantine list unreadable because one entry is
      damaged; return valid entries plus a health error.

### 8.4 Add tests

Test power loss after every quarantine phase and corruption of:

- [ ] record only;
- [ ] evidence only;
- [ ] directory name;
- [ ] record ID;
- [ ] source path;
- [ ] reason;
- [ ] duplicate quarantine ID.

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

- [ ] two updates with the same expected revision cannot both succeed;
- [ ] create and delete cannot race the same index;
- [ ] recovery cannot race an API mutation;
- [ ] import/restore excludes all other mutations;
- [ ] unlock failure is visible and does not report mutation success.

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

- [ ] Do not increment failure count on PBKDF2 failure.
- [ ] Do not return 401 on password-record corruption.
- [ ] Add tests for all result combinations.

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

- [ ] fail it;
- [ ] assert later cleanup operations still run;
- [ ] assert only successful ownership flags clear;
- [ ] assert retry cleans residual ownership;
- [ ] assert original start error remains visible.

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

- [ ] reject new submissions;
- [ ] request cancellation;
- [ ] enqueue or notify stop;
- [ ] wait with a bounded timeout;
- [ ] release all USB keys;
- [ ] free any owned plan;
- [ ] delete queue and semaphores after task exit;
- [ ] clear handles and engine state;
- [ ] retain release and shutdown errors.

### 12.3 Remove ignored `finish_execution`

Replace:

```c
(void)finish_execution(...);
```

with explicit result handling. If terminal publication or reset fails:

- [ ] return that failure;
- [ ] retain the primary execution failure in status/health;
- [ ] leave executor unavailable rather than falsely idle;
- [ ] require deinit/restart or explicit recovery.

### 12.4 Add terminal states

Add `EXECUTION_TIMED_OUT` or retain `EXECUTION_FAILED` with a required
`APP_ERROR_TIMEOUT` mapping. The API and frontend must distinguish timeout.

- [ ] Add execution ID and object identity to status.
- [ ] Add accepted, started, and completed timestamps.
- [ ] Add current action summary.
- [ ] Add tests for key-release failure after otherwise successful execution.

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

- [ ] Update health atomically.
- [ ] Log failures through ESP logging.
- [ ] Expose redacted health through diagnostics.

### 13.3 Implement deinit

- [ ] request task stop;
- [ ] wait with bounded timeout;
- [ ] configure outputs to a documented safe state;
- [ ] delete semaphores after task exit;
- [ ] clear handles;
- [ ] return cleanup failure if any step fails.

### 13.4 Add tests

Test:

- [ ] semaphore give failure;
- [ ] cancel failure;
- [ ] GPIO output failure;
- [ ] task stop timeout;
- [ ] second deinit call;
- [ ] no use-after-free after deinit.

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

- [ ] Verify total flash size.
- [ ] Verify application slots remain aligned.
- [ ] Add partition tests that require exactly one `nvs_keys` partition.

### 14.2 Enable NVS encryption configuration

Update:

```text
firmware/sdkconfig.defaults
```

Enable the chosen IDF `v5.5.5` NVS encryption scheme.

- [ ] Document whether release uses flash-encryption-based or HMAC-based key
      protection.
- [ ] Do not claim physical confidentiality until the matching eFuse/flash
      workflow is tested.
- [ ] Add release checks that reject an unencrypted production configuration.

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

- [ ] strict schema and bounds;
- [ ] NVS transaction with `nvs_commit()`;
- [ ] readback validation;
- [ ] no plaintext administrator password;
- [ ] secure zero of temporary credential buffers;
- [ ] sessions remain RAM-only;
- [ ] revision conflict behavior.

### 14.4 Remove ordinary plaintext credential logs

Update:

```text
firmware/components/app_core/app_core.c
firmware/main/Kconfig.projbuild
scripts/check-firmware.sh
```

- [ ] Remove AP and web password printing from ordinary development mode.
- [ ] Add a separate manufacturing-only option if still required.
- [ ] Make the release check fail when that option is enabled.
- [ ] Add source scanning for credential log format strings.

### 14.5 Implement setup flow

Add setup routes that exist only when unprovisioned:

- [ ] setup-state read;
- [ ] setup credential submission;
- [ ] setup completion;
- [ ] restart.

Require physical confirmation or the explicitly gated manufacturing mode.

Test interruption before and after every NVS commit/readback step.

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

- [ ] list;
- [ ] create;
- [ ] read;
- [ ] update with expected revision;
- [ ] delete;
- [ ] duplicate;
- [ ] reorder;
- [ ] validate references;
- [ ] quarantine corrupt objects.

### 15.2 Procedure repository

Implement:

```text
/data/sets/<set-id>/procedures/<procedure-id>.json
/data/sets/<set-id>/procedure-order.json
```

Validate:

- [ ] step IDs unique;
- [ ] macro references exist and match scope;
- [ ] no duplicate completion/skip IDs;
- [ ] step count and text bounds;
- [ ] exact fields;
- [ ] revision conflicts.

### 15.3 Progress repository

Implement:

```text
/data/sets/<set-id>/progress/<procedure-id>.json
```

- [ ] current step must belong to the referenced procedure revision;
- [ ] completed and skipped arrays must contain only existing step IDs;
- [ ] no ID may be both completed and skipped;
- [ ] procedure revision changes must produce visible stale-progress handling;
- [ ] reset is atomic.

### 15.4 Settings and active-set repository

Persist non-secret user settings in the chosen protected configuration store.

- [ ] always ask for set;
- [ ] optional active set;
- [ ] physical confirmation requirement;
- [ ] UI preferences only when specified by `docs/SPEC.md`.

### 15.5 Delete/reference behavior

Before deleting a macro:

- [ ] scan procedure references under the repository lock;
- [ ] return `APP_ERROR_CONFLICT` when referenced;
- [ ] include bounded referencing IDs in API details;
- [ ] never silently rewrite a procedure.

Before deleting a set:

- [ ] move the set to transaction-owned trash;
- [ ] remove it from index;
- [ ] preserve global macros;
- [ ] clear active-set selection if it points to the deleted set;
- [ ] recover deterministically after interruption.

## 16. Complete the HTTP API

### 16.1 Add centralized request policy

Create a reusable request-policy adapter that checks:

- [ ] Content-Type;
- [ ] body limit;
- [ ] Host;
- [ ] Origin;
- [ ] cookie;
- [ ] CSRF;
- [ ] session;
- [ ] request ID;
- [ ] route-specific physical confirmation.

Do not duplicate security checks across every route.

### 16.2 Add path parameter parsing

- [ ] strict UUID only;
- [ ] no decoded slash;
- [ ] no path traversal;
- [ ] bounded route buffers;
- [ ] unknown fields rejected in JSON bodies.

### 16.3 Implement set routes

Implement all set routes from the FIX1 specification.

For every mutation:

- [ ] require expected revision;
- [ ] map conflict to HTTP 409;
- [ ] map storage full to HTTP 507;
- [ ] return committed object readback;
- [ ] test partial body and malformed JSON.

### 16.4 Implement macro routes

- [ ] set-local CRUD;
- [ ] global CRUD;
- [ ] ordering;
- [ ] validation without execution;
- [ ] reference-conflict details;
- [ ] exact action count and duration.

### 16.5 Implement procedure/progress routes

- [ ] CRUD and order;
- [ ] progress read;
- [ ] complete step;
- [ ] skip with explicit confirmation;
- [ ] reset progress;
- [ ] stale revision handling.

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

- [ ] Do not accept macro source in the request.
- [ ] Return `202` only after executor ownership transfer.
- [ ] Add tests for stale revision, missing object, USB unavailable, busy
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

Implement settings, storage health, quarantine, diagnostics, export, import,
backup, restore, restart, credential reset, and factory reset.

Destructive operations must enforce reauthentication or physical confirmation
as specified.

### 16.9 Expand API tests

Every documented route needs:

- [ ] success test;
- [ ] authentication failure;
- [ ] CSRF failure;
- [ ] Origin failure;
- [ ] Host failure;
- [ ] Content-Type failure;
- [ ] oversized body;
- [ ] malformed JSON;
- [ ] unknown field;
- [ ] storage failure;
- [ ] revision conflict;
- [ ] redaction assertion.

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

### 17.3 Implement real session boundary

- [ ] load status/session on startup;
- [ ] redirect unprovisioned devices to setup;
- [ ] redirect expired sessions to login;
- [ ] clear CSRF token on logout or 401;
- [ ] display rate-limit retry time;
- [ ] stop authenticated polling after session expiry.

### 17.4 Implement real set selection

- [ ] load sets;
- [ ] recents;
- [ ] search;
- [ ] metadata;
- [ ] explicit active set;
- [ ] no hardcoded HP model;
- [ ] active set shown in the header from server state.

### 17.5 Implement real macro editor

- [ ] load/create/update;
- [ ] source byte count;
- [ ] live server validation;
- [ ] action count;
- [ ] estimated duration;
- [ ] exact parse location;
- [ ] directive insertion;
- [ ] disable Save when invalid;
- [ ] stale-revision conflict UI.

### 17.6 Implement real procedure workflow

- [ ] list and progress;
- [ ] current step;
- [ ] previous/next;
- [ ] instruction completion;
- [ ] checkpoint confirmation;
- [ ] resend;
- [ ] skip confirmation;
- [ ] reset;
- [ ] no automatic next execution.

### 17.7 Implement execution confirmation and submission

The confirmation page must use live data and disable Send unless:

- [ ] active set is loaded;
- [ ] macro is loaded and validated;
- [ ] revision is current;
- [ ] USB state is `ready`;
- [ ] executor is not busy;
- [ ] physical confirmation policy can be satisfied.

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

### 17.9 Implement real management screens

- [ ] create/edit/duplicate/reorder/delete sets;
- [ ] import as new;
- [ ] transactional replace;
- [ ] export;
- [ ] settings;
- [ ] backup/restore;
- [ ] diagnostics;
- [ ] quarantine view;
- [ ] restart/reset workflows.

No enabled button may be inert.

### 17.10 Add accessibility and browser tests

- [ ] keyboard-only navigation;
- [ ] focus trap and restoration;
- [ ] screen-reader live regions;
- [ ] reorder alternatives;
- [ ] touch target checks;
- [ ] no color-only status;
- [ ] offline/reconnect;
- [ ] full execution workflow.

## 18. Complete import, export, backup, and restore

### 18.1 Implement bounded package reader

- [ ] enforce `APP_IMPORT_PACKAGE_MAX_BYTES`;
- [ ] stream or use bounded allocation;
- [ ] reject trailing data;
- [ ] reject unknown schema fields;
- [ ] validate all objects before mutation.

### 18.2 Implement export

- [ ] include referenced set-local and global macros;
- [ ] include procedures and optional progress;
- [ ] exclude every secret;
- [ ] deterministic ordering;
- [ ] stable schema version;
- [ ] exact content length or bounded chunked response.

### 18.3 Implement transactional replace

Add a transaction type and recovery code that handles every phase.

- [ ] stage complete replacement;
- [ ] validate readback;
- [ ] back up current set;
- [ ] activate replacement;
- [ ] update index;
- [ ] validate active set;
- [ ] remove backup and manifest;
- [ ] recover after every interrupted phase.

### 18.4 Implement full backup and restore

- [ ] backup all sets, global macros, procedures, and optional progress;
- [ ] exclude credentials and sessions;
- [ ] restore all-or-nothing;
- [ ] require physical/admin confirmation;
- [ ] test storage full during staging;
- [ ] test power loss after every phase.

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
