# Phase 16 repository handler acceptance failure

- Transform: success
- Format: success
- Focused tests: failure
- Authoritative gate: skipped

## phase16-handler-transform.log

```text

Phase 16 repository handler acceptance test registered
```

## phase16-handler-format.log

```text

```

## phase16-handler-focused.log

```text
399:collect2: error: ld returned 1 exit status

[ 44%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 45%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 46%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 46%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 46%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 46%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 47%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 46%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 49%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 49%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 49%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_progress.c.o
[ 50%] Building C object CMakeFiles/web_api_core_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 51%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 51%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 51%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 52%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_parser.c.o
[ 53%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 53%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_keymap_us.c.o
[ 54%] Building C object CMakeFiles/storage_repository_tests.dir/test_storage_repository.c.o
[ 54%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_core.c.o
[ 54%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 55%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 56%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c.o
[ 57%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 58%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 58%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 58%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_json.c.o
[ 58%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 58%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 49%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 58%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 59%] Building C object CMakeFiles/web_api_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 60%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 60%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 21%] Building C object CMakeFiles/provisioning_settings_tests.dir/test_provisioning_settings.c.o
[ 21%] Building C object CMakeFiles/storage_mount_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_mount_core.c.o
[ 21%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 21%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 21%] Building C object CMakeFiles/storage_parent_sync_tests.dir/test_storage_parent_sync.c.o
[ 21%] Building C object CMakeFiles/storage_atomic_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 21%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 31%] Building C object CMakeFiles/web_api_admin_boundary_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c.o
[ 21%] Building C object CMakeFiles/storage_parent_sync_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 21%] Building C object CMakeFiles/storage_atomic_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 21%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_cookie.c.o
[ 21%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 21%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 21%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 21%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 21%] Building C object CMakeFiles/storage_repository_lock_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 21%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/test_storage_atomic_validators.c.o
[ 21%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 21%] Building C object CMakeFiles/storage_parent_sync_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 21%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 21%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 21%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 21%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 21%] Building C object CMakeFiles/web_api_core_tests.dir/test_web_api_core.c.o
[ 21%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 21%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 61%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 63%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 63%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 64%] Building C object CMakeFiles/web_api_response_tests.dir/test_web_api_response.c.o
[ 62%] Linking C executable app_operation_result_tests
[ 65%] Building C object CMakeFiles/storage_progress_repository_tests.dir/test_storage_progress.c.o
[ 66%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/test_storage_active_set_delete.c.o
[ 67%] Building C object CMakeFiles/web_execution_submit_tests.dir/test_web_execution_submit.c.o
[ 68%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 69%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 70%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 71%] Building C object CMakeFiles/web_request_policy_tests.dir/test_web_request_policy.c.o
[ 72%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 73%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 74%] Building C object CMakeFiles/web_api_admin_boundary_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 75%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 76%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 77%] Building C object CMakeFiles/web_execution_submit_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 78%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 79%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_request_policy.c.o
[ 80%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 81%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 78%] Linking C executable macro_model_tests
[ 82%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 83%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 84%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 85%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 86%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 61%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 61%] Building C object CMakeFiles/provisioning_bootstrap_tests.dir/test_provisioning_bootstrap.c.o
[ 61%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 61%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/test_web_api_repository_handlers.c.o
[ 61%] Building C object CMakeFiles/web_api_dispatch_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c.o
[ 61%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 61%] Building C object CMakeFiles/web_api_dispatch_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 61%] Building C object CMakeFiles/web_setup_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_setup_core.c.o
[ 61%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 61%] Building C object CMakeFiles/web_api_response_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 61%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 61%] Building C object CMakeFiles/web_api_core_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_core.c.o
[ 61%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 61%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 61%] Building C object CMakeFiles/web_api_core_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 61%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 61%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 61%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 61%] Building C object CMakeFiles/web_execution_submit_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 61%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_sets.c.o
[ 61%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 61%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 61%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 61%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 61%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 61%] Building C object CMakeFiles/provisioning_settings_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 61%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 61%] Building C object CMakeFiles/web_api_admin_boundary_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_admin_boundary.c.o
[ 61%] Building C object CMakeFiles/storage_object_json_tests.dir/test_storage_object_json.c.o
[ 61%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 61%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 61%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 61%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 61%] Building C object CMakeFiles/web_api_admin_boundary_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_core.c.o
[ 61%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 61%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 61%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 61%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 61%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 61%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_core.c.o
[ 61%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_progress.c.o
[ 61%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 61%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 61%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 61%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 61%] Building C object CMakeFiles/web_api_dispatch_tests.dir/test_web_api_dispatch.c.o
[ 61%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 61%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic_recovery.c.o
[ 61%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 61%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 61%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 61%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 61%] Building C object CMakeFiles/web_api_json_tests.dir/test_web_api_json.c.o
[ 61%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 61%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 61%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic_validators.c.o
[ 61%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 61%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 61%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 61%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 61%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 61%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 61%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 86%] Linking C executable web_security_tests
[ 61%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 61%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 61%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 61%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic_validators.c.o
[ 61%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 61%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 61%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_adapter_body_auth.c.o
[ 61%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 87%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 88%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_macros.c.o
[ 87%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_procedures.c.o
[ 87%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_handler_common.c.o
[ 87%] Building C object CMakeFiles/web_api_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 87%] Building C object CMakeFiles/provisioning_settings_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/provisioning/provisioning_core.c.o
[ 87%] Building C object CMakeFiles/web_api_response_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c.o
[ 88%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 88%] Built target app_operation_result_tests
[ 88%] Linking C executable storage_repository_lock_tests
[ 89%] Linking C executable web_setup_json_tests
[ 89%] Linking C executable macro_parser_tests
[ 89%] Linking C executable storage_mount_tests
[ 89%] Built target macro_model_tests
[ 89%] Linking C executable usb_keyboard_tests
[ 89%] Linking C executable web_api_dispatch_tests
[ 89%] Linking C executable web_server_adapter_tests
[ 90%] Linking C executable storage_atomic_tests
[ 90%] Built target web_security_tests
[ 90%] Linking C executable storage_repository_io_tests
[ 90%] Built target storage_repository_lock_tests
[ 90%] Linking C executable device_controls_tests
[ 91%] Linking C executable storage_parent_sync_tests
[ 92%] Linking C executable wifi_ap_tests
[ 93%] Linking C executable web_setup_tests
[ 91%] Built target web_setup_json_tests
[ 93%] Built target storage_mount_tests
[ 93%] Linking C executable test_support_tests
[ 93%] Built target usb_keyboard_tests
[ 94%] Linking C executable web_request_policy_tests
[ 94%] Linking C executable web_execution_submit_tests
[ 94%] Linking C executable macro_executor_tests
[ 94%] Linking C executable auth_tests
[ 94%] Linking C executable web_api_admin_boundary_tests
[ 94%] Linking C executable provisioning_tests
[ 94%] Built target web_api_dispatch_tests
[ 94%] Built target device_controls_tests
[ 94%] Built target web_server_adapter_tests
[ 94%] Built target macro_parser_tests
[ 94%] Built target storage_atomic_tests
[ 95%] Linking C executable web_api_json_tests
[ 95%] Built target wifi_ap_tests
[ 96%] Linking C executable storage_macro_repository_tests
[ 97%] Linking C executable storage_transaction_tests
[ 97%] Built target storage_repository_io_tests
[ 97%] Built target storage_parent_sync_tests
[ 97%] Built target web_setup_tests
[ 97%] Built target test_support_tests
[ 97%] Linking C executable storage_atomic_validators_tests
[ 97%] Linking C executable web_api_response_tests
[ 97%] Built target web_request_policy_tests
[ 97%] Built target web_execution_submit_tests
[ 97%] Built target auth_tests
[ 97%] Linking C executable provisioning_bootstrap_tests
[ 97%] Built target provisioning_tests
[ 98%] Linking C executable web_api_core_tests
[ 98%] Built target web_api_admin_boundary_tests
[ 97%] Built target macro_executor_tests
[ 98%] Built target web_api_json_tests
[ 98%] Built target web_api_response_tests
[ 98%] Built target storage_atomic_validators_tests
[ 99%] Linking C executable app_core_tests
[ 99%] Built target storage_macro_repository_tests
[ 99%] Linking C executable provisioning_settings_tests
[ 99%] Built target storage_transaction_tests
[ 99%] Linking C executable storage_active_set_delete_tests
[ 99%] Linking C executable web_api_repository_handlers_tests
[ 99%] Built target web_api_core_tests
[ 99%] Built target provisioning_bootstrap_tests
[ 99%] Linking C executable storage_quarantine_tests
[ 99%] Linking C executable storage_repository_tests
[ 99%] Built target app_core_tests
[ 99%] Linking C executable storage_object_json_tests
[ 99%] Linking C executable storage_atomic_recovery_tests
[ 99%] Built target provisioning_settings_tests
[ 99%] Built target storage_quarantine_tests
/usr/bin/ld[ 99%] Built target storage_atomic_recovery_tests
: CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_sets.c.o: in function `send_set':
web_api_sets.c:(.text+0x9b): undefined reference to `web_api_handler_json_free'
/usr/bin/ld: CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_sets.c.o: in function `handle_set_collection':
web_api_sets.c:(.text+0x1eb): undefined reference to `web_api_handler_json_free'
/usr/bin/ld: CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_sets.c.o: in function `handle_select':
web_api_sets.c:(.text+0x876): undefined reference to `web_api_handler_json_free'
/usr/bin/ld: CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_macros.c.o: in function `send_macro':
web_api_macros.c:(.text+0x17d): undefined reference to `web_api_handler_json_free'
/usr/bin/ld: CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_macros.c.o: in function `handle_collection':
web_api_macros.c:(.text+0x396): undefined reference to `web_api_handler_json_free'
/usr/bin/ld: CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_procedures.c.o:web_api_procedures.c:(.text+0x9b): more undefined references to `web_api_handler_json_free' follow
[ 99%] Built target storage_object_json_tests
collect2: error: ld returned 1 exit status
gmake[2]: *** [CMakeFiles/web_api_repository_handlers_tests.dir/build.make:551: web_api_repository_handlers_tests] Error 1
gmake[1]: *** [CMakeFiles/Makefile2:1295: CMakeFiles/web_api_repository_handlers_tests.dir/all] Error 2
gmake[1]: *** Waiting for unfinished jobs....
[100%] Linking C executable storage_progress_repository_tests
[100%] Built target storage_repository_tests
[100%] Built target storage_active_set_delete_tests
[100%] Linking C executable storage_procedure_repository_tests
[100%] Built target storage_progress_repository_tests
[100%] Built target storage_procedure_repository_tests
gmake: *** [Makefile:101: all] Error 2
```
