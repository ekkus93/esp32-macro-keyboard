#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "app_error.h"
#include "storage.h"
#include "storage_atomic_recovery.h"
#include "storage_incidents.h"
#include "test_assert.h"
#include "test_temp_dir.h"

/*
 * Boot recovery is, in its entirety, "unlink every *.tmp under /data"
 * (SPEC 13.4). These tests pin that whole contract: what it removes, what it
 * must not remove, that it reaches nested set directories, and that it is safe
 * to run when there is nothing to do.
 *
 * The predecessor of this file tested a reconcile decision table over .tmp/.bak
 * pairs. That table is gone along with the .bak file it arbitrated, so those
 * cases are not ported -- there is nothing left to decide.
 */

static void make_directory(const char *path) {
    if (mkdir(path, 0700) != 0) {
        TEST_CHECK_EQ_INT(EEXIST, errno);
    }
}

static void write_text(const char *path, const char *text) {
    const int descriptor = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    TEST_CHECK(descriptor >= 0);
    const size_t length = strlen(text);
    const ssize_t count = write(descriptor, text, length);
    TEST_CHECK(count >= 0);
    TEST_CHECK_EQ_U64(length, (size_t)count);
    TEST_CHECK_EQ_INT(0, close(descriptor));
}

static bool path_exists(const char *path) {
    struct stat metadata;
    return stat(path, &metadata) == 0;
}

static void read_text(const char *path, char *output, size_t output_size) {
    TEST_CHECK(output_size > 0U);
    const int descriptor = open(path, O_RDONLY);
    TEST_CHECK(descriptor >= 0);
    const ssize_t count = read(descriptor, output, output_size - 1U);
    TEST_CHECK(count >= 0);
    output[(size_t)count] = '\0';
    TEST_CHECK_EQ_INT(0, close(descriptor));
}

static void reset_store(void) {
    test_temp_dir_remove_path(STORAGE_DATA_MOUNT);
    make_directory(STORAGE_DATA_MOUNT);
    make_directory(STORAGE_DATA_MOUNT "/sets");
}

/* SPEC 24.2 item: boot cleanup of stray `.tmp` files */
static void test_stray_temporary_is_removed_at_boot(void) {
    reset_store();
    storage_incidents_reset();
    write_text(STORAGE_DATA_MOUNT "/set-index.json", "{\"schema_version\":1,\"ids\":[]}");
    write_text(STORAGE_DATA_MOUNT "/set-index.json.tmp", "{\"partial\":");

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_atomic_recover_all());

    TEST_CHECK(!path_exists(STORAGE_DATA_MOUNT "/set-index.json.tmp"));
    char output[64U];
    read_text(STORAGE_DATA_MOUNT "/set-index.json", output, sizeof(output));
    TEST_CHECK_EQ_STRING("{\"schema_version\":1,\"ids\":[]}", output);

    /* SPEC 20.3: diagnostics reports how many interrupted writes boot recovery
     * cleaned up, so repeated power loss is visible rather than silent. */
    storage_incident_report_t report = {0};
    storage_incidents_snapshot(&report);
    TEST_CHECK_EQ_U64(1U, report.temporaries_removed);
}

/* An interrupted write that never reached its rename leaves the destination
 * absent. Recovery must not invent it from the temporary: an interrupted write
 * is indistinguishable from one that never started, and that is the correct
 * outcome (SPEC 13.4). */
static void test_temporary_without_destination_is_discarded_not_activated(void) {
    reset_store();
    write_text(STORAGE_DATA_MOUNT "/sets/orphan.json.tmp", "{\"never\":\"committed\"}");

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_atomic_recover_all());

    TEST_CHECK(!path_exists(STORAGE_DATA_MOUNT "/sets/orphan.json.tmp"));
    TEST_CHECK(!path_exists(STORAGE_DATA_MOUNT "/sets/orphan.json"));
}

static void test_nested_package_directories_are_swept(void) {
    reset_store();
    make_directory(STORAGE_DATA_MOUNT "/sets/aaaaaaaa-0000-4000-8000-000000000001");
    make_directory(STORAGE_DATA_MOUNT "/sets/aaaaaaaa-0000-4000-8000-000000000001/macros");
    write_text(STORAGE_DATA_MOUNT "/sets/aaaaaaaa-0000-4000-8000-000000000001/set.json", "{}");
    write_text(STORAGE_DATA_MOUNT "/sets/aaaaaaaa-0000-4000-8000-000000000001/set.json.tmp", "{");
    write_text(STORAGE_DATA_MOUNT "/sets/aaaaaaaa-0000-4000-8000-000000000001/macros/m.json.tmp",
               "{");

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_atomic_recover_all());

    TEST_CHECK(
        !path_exists(STORAGE_DATA_MOUNT "/sets/aaaaaaaa-0000-4000-8000-000000000001/set.json.tmp"));
    TEST_CHECK(!path_exists(STORAGE_DATA_MOUNT
                            "/sets/aaaaaaaa-0000-4000-8000-000000000001/macros/m.json.tmp"));
    TEST_CHECK(
        path_exists(STORAGE_DATA_MOUNT "/sets/aaaaaaaa-0000-4000-8000-000000000001/set.json"));
}

/* Only the exact `.tmp` suffix is debris. A file that merely contains "tmp", or
 * ends in something tmp-adjacent, is ordinary data and must survive. */
static void test_only_exact_tmp_suffix_is_removed(void) {
    reset_store();
    write_text(STORAGE_DATA_MOUNT "/sets/tmp.json", "keep");
    write_text(STORAGE_DATA_MOUNT "/sets/a.tmp.json", "keep");
    write_text(STORAGE_DATA_MOUNT "/sets/b.json.tmpx", "keep");
    write_text(STORAGE_DATA_MOUNT "/sets/c.json.temp", "keep");
    write_text(STORAGE_DATA_MOUNT "/sets/d.json.tmp", "remove");

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_atomic_recover_all());

    TEST_CHECK(path_exists(STORAGE_DATA_MOUNT "/sets/tmp.json"));
    TEST_CHECK(path_exists(STORAGE_DATA_MOUNT "/sets/a.tmp.json"));
    TEST_CHECK(path_exists(STORAGE_DATA_MOUNT "/sets/b.json.tmpx"));
    TEST_CHECK(path_exists(STORAGE_DATA_MOUNT "/sets/c.json.temp"));
    TEST_CHECK(!path_exists(STORAGE_DATA_MOUNT "/sets/d.json.tmp"));
}

/* A bare ".tmp" has no destination it could belong to, so it is not a staged
 * write and must be left alone rather than guessed at. */
static void test_bare_tmp_name_is_not_treated_as_an_artifact(void) {
    reset_store();
    write_text(STORAGE_DATA_MOUNT "/sets/.tmp", "keep");

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_atomic_recover_all());

    TEST_CHECK(path_exists(STORAGE_DATA_MOUNT "/sets/.tmp"));
}

static void test_clean_repository_is_a_no_op(void) {
    reset_store();
    write_text(STORAGE_DATA_MOUNT "/set-index.json", "{\"schema_version\":1,\"ids\":[]}");

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_atomic_recover_all());

    TEST_CHECK(path_exists(STORAGE_DATA_MOUNT "/set-index.json"));
}

/* A repository that has never been provisioned is the defined initial state,
 * not a recovery failure. */
static void test_missing_mount_is_not_an_error(void) {
    test_temp_dir_remove_path(STORAGE_DATA_MOUNT);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_atomic_recover_all());
}

static void test_invalid_operations_are_rejected(void) {
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_atomic_recover_all_with_ops(NULL));
}

int main(void) {
    test_stray_temporary_is_removed_at_boot();
    test_temporary_without_destination_is_discarded_not_activated();
    test_nested_package_directories_are_swept();
    test_only_exact_tmp_suffix_is_removed();
    test_bare_tmp_name_is_not_treated_as_an_artifact();
    test_clean_repository_is_a_no_op();
    test_missing_mount_is_not_an_error();
    test_invalid_operations_are_rejected();
    test_temp_dir_remove_path(STORAGE_DATA_MOUNT);
    puts("storage atomic recovery tests passed");
    return EXIT_SUCCESS;
}
