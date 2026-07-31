from pathlib import Path

source = Path("firmware/components/storage/storage_transaction_replace.c")
text = source.read_text()
anchor = '''static app_error_code_t recover_staged(
    storage_transaction_manifest_t *manifest, const storage_fs_ops_t *operations,
'''
prepared = '''static app_error_code_t recover_prepared(
    const storage_transaction_manifest_t *manifest, const storage_fs_ops_t *operations,
    storage_transaction_set_index_presence_fn set_index_presence, void *index_context,
    storage_transaction_validate_set_fn validate_set, void *validation_context,
    storage_transaction_remove_tree_fn remove_tree, void *remove_context,
    const app_uuid_t *set_id) {
    bool staging_exists = false;
    bool destination_exists = false;
    bool backup_exists = false;
    app_error_code_t result = path_is_directory(manifest->staging, operations, &staging_exists);
    if (result == APP_ERROR_NONE) {
        result = path_is_directory(manifest->destination, operations, &destination_exists);
    }
    if (result == APP_ERROR_NONE) {
        result = path_is_directory(manifest->backup, operations, &backup_exists);
    }
    if (result != APP_ERROR_NONE || !destination_exists || backup_exists) {
        return result == APP_ERROR_NONE ? APP_ERROR_STORAGE_CORRUPT : result;
    }
    result = validate_tree(validate_set, validation_context, manifest->destination, set_id,
                           manifest->expected_revision);
    if (result == APP_ERROR_NONE) {
        result = set_index_presence(index_context, set_id, true);
    }
    if (result == APP_ERROR_NONE && staging_exists) {
        result = remove_tree(remove_context, manifest->staging);
        if (result == APP_ERROR_NONE &&
            storage_fs_sync_parent_path(operations->context, manifest->staging) != 0) {
            result = map_error_number(errno);
        }
    }
    return result == APP_ERROR_NONE ? remove_manifest(manifest, operations) : result;
}

'''
if text.count(anchor) != 1:
    raise SystemExit("replace prepared insertion point changed")
text = text.replace(anchor, prepared + anchor, 1)
old = '''    if (manifest->phase == STORAGE_TRANSACTION_STAGED) {
        result = recover_staged(manifest, operations, generate_uuid, uuid_context, validate_set,
                                validation_context, &set_id);
    }
'''
new = '''    if (manifest->phase == STORAGE_TRANSACTION_PREPARED) {
        return recover_prepared(manifest, operations, set_index_presence, index_context,
                                validate_set, validation_context, remove_tree, remove_context,
                                &set_id);
    }
    if (manifest->phase == STORAGE_TRANSACTION_STAGED) {
        result = recover_staged(manifest, operations, generate_uuid, uuid_context, validate_set,
                                validation_context, &set_id);
    }
'''
if text.count(old) != 1:
    raise SystemExit("replace phase dispatch point changed")
source.write_text(text.replace(old, new, 1))

tests = Path("tests/host/test_storage_transactions.c")
text = tests.read_text()
anchor = '''static void test_replace_recovery_is_idempotent(void)
{
'''
prepared_test = '''static void test_replace_prepared_rolls_back_incomplete_staging(void)
{
    reset_storage();
    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    storage_fs_ops_t operations = make_operations(&filesystem);
    uuid_sequence_t uuids = {0};
    index_fixture_t index = {.failure = APP_ERROR_IO};
    storage_transaction_manifest_t manifest = make_replace_manifest(
        "00000000-0000-4000-8000-000000000099",
        "10000000-0000-4000-8000-000000000099",
        STORAGE_TRANSACTION_PREPARED);
    create_directory(manifest.destination);
    create_directory(manifest.staging);
    write_manifest(&manifest, &operations, &uuids);
    fake_fs_backend_reset(&filesystem);

    TEST_CHECK_EQ_INT(APP_ERROR_NONE, recover(&operations, &uuids, &index));
    TEST_CHECK(path_exists(manifest.destination));
    TEST_CHECK(!path_exists(manifest.staging));
    TEST_CHECK(!path_exists(manifest.backup));
    char path[APP_PATH_MAX_BYTES];
    transaction_path(path, sizeof(path), &manifest.id);
    TEST_CHECK(!path_exists(path));
    TEST_CHECK_EQ_U64(1U, index.count);
    TEST_CHECK(index.presence[0]);

    reset_storage();
    fake_fs_backend_reset(&filesystem);
    operations = make_operations(&filesystem);
    uuids = (uuid_sequence_t){0};
    index = (index_fixture_t){.failure = APP_ERROR_IO};
    manifest = make_replace_manifest(
        "00000000-0000-4000-8000-000000000098",
        "10000000-0000-4000-8000-000000000098",
        STORAGE_TRANSACTION_PREPARED);
    create_directory(manifest.destination);
    create_directory(manifest.staging);
    write_manifest(&manifest, &operations, &uuids);
    remove_fixture.fail_on_call = 1U;
    fake_fs_backend_reset(&filesystem);
    TEST_CHECK_EQ_INT(APP_ERROR_IO, recover(&operations, &uuids, &index));
    TEST_CHECK(path_exists(manifest.staging));
    remove_fixture.fail_on_call = 0U;
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, recover(&operations, &uuids, &index));
}

'''
if text.count(anchor) != 1:
    raise SystemExit("prepared recovery test insertion point changed")
text = text.replace(anchor, prepared_test + anchor, 1)
old_calls = '''    test_delete_recovery_is_idempotent();
    test_replace_recovery_is_idempotent();
'''
new_calls = '''    test_delete_recovery_is_idempotent();
    test_replace_prepared_rolls_back_incomplete_staging();
    test_replace_recovery_is_idempotent();
'''
if text.count(old_calls) != 1:
    raise SystemExit("prepared recovery test call point changed")
tests.write_text(text.replace(old_calls, new_calls, 1))
