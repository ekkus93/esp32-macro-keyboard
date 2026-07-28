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
409:test failure at /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_api_json.c:277: revision == 0U; expected=0, actual=4
431:The following tests FAILED:

[ 52%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 53%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 53%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 53%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 54%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 54%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 54%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 54%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 43%] Building C object CMakeFiles/web_setup_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_setup_json.c.o
[ 54%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 55%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 55%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 55%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 55%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 56%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 56%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 45%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 56%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 56%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 56%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 57%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 57%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 58%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 59%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 59%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 60%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 61%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 61%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 63%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 64%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 64%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 64%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_progress.c.o
[ 54%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 64%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 64%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 64%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 65%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 66%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 66%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 67%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 67%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 67%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_progress.c.o
[ 55%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 68%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 68%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 68%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 68%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 68%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 69%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 60%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 34%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 34%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 34%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 34%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 34%] Building C object CMakeFiles/storage_atomic_tests.dir/test_storage_atomic.c.o
[ 34%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic_recovery.c.o
[ 34%] Building C object CMakeFiles/web_api_dispatch_tests.dir/test_web_api_dispatch.c.o
[ 34%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic_validators.c.o
[ 34%] Building C object CMakeFiles/storage_parent_sync_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 34%] Building C object CMakeFiles/storage_repository_lock_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 35%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 34%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 62%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 34%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic_validators.c.o
[ 34%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 34%] Building C object CMakeFiles/storage_parent_sync_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 34%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 34%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 34%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 34%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 34%] Building C object CMakeFiles/storage_parent_sync_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 68%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 70%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 70%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 69%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 70%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_keymap_us.c.o
[ 71%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_macros.c.o
[ 72%] Building C object CMakeFiles/web_execution_submit_tests.dir/test_web_execution_submit.c.o
[ 73%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 74%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 75%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 76%] Linking C executable app_operation_result_tests
[ 77%] Building C object CMakeFiles/provisioning_bootstrap_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/provisioning/provisioning_bootstrap_core.c.o
[ 78%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 79%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 79%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_procedures.c.o
[ 80%] Building C object CMakeFiles/web_api_core_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 80%] Linking C executable macro_model_tests
[ 81%] Building C object CMakeFiles/web_api_response_tests.dir/test_web_api_response.c.o
[ 82%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/test_storage_active_set_delete.c.o
[ 83%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_request_policy.c.o
[ 84%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 70%] Building C object CMakeFiles/web_api_core_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 70%] Building C object CMakeFiles/storage_atomic_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 70%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 70%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 70%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 70%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 70%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 70%] Building C object CMakeFiles/web_api_core_tests.dir/test_web_api_core.c.o
[ 70%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 70%] Building C object CMakeFiles/web_setup_tests.dir/test_web_setup.c.o
[ 70%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 70%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 70%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 70%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 70%] Building C object CMakeFiles/web_execution_submit_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 70%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 70%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 70%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_progress.c.o
[ 70%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 70%] Building C object CMakeFiles/storage_object_json_tests.dir/test_storage_object_json.c.o
[ 70%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 70%] Building C object CMakeFiles/web_execution_submit_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 70%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/test_storage_procedures.c.o
[ 70%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 70%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 70%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 70%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_origin.c.o
[ 70%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 70%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 70%] Building C object CMakeFiles/web_api_response_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c.o
[ 70%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_cookie.c.o
[ 70%] Building C object CMakeFiles/web_setup_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_setup_core.c.o
[ 70%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_core.c.o
[ 70%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 85%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 86%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_parser.c.o
[ 87%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c.o
[ 88%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 85%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 85%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 85%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 85%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 85%] Building C object CMakeFiles/storage_repository_tests.dir/test_storage_repository.c.o
[ 85%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_core.c.o
[ 85%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_json.c.o
[ 85%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_handler_common.c.o
[ 85%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_sets.c.o
[ 85%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 85%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 88%] Building C object CMakeFiles/web_execution_submit_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 88%] Linking C executable web_security_tests
[ 88%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 88%] Linking C executable storage_mount_tests
[ 88%] Linking C executable test_support_tests
[ 88%] Built target app_operation_result_tests
[ 88%] Linking C executable provisioning_bootstrap_tests
[ 88%] Linking C executable usb_keyboard_tests
[ 88%] Built target macro_model_tests
[ 89%] Linking C executable web_api_json_tests
[ 89%] Linking C executable provisioning_settings_tests
[ 89%] Linking C executable web_api_response_tests
[ 89%] Linking C executable device_controls_tests
[ 89%] Built target web_security_tests
[ 89%] Built target storage_mount_tests
[ 89%] Linking C executable web_server_adapter_tests
[ 90%] Linking C executable storage_transaction_tests
[ 91%] Linking C executable web_api_core_tests
[ 92%] Linking C executable wifi_ap_tests
[ 92%] Built target usb_keyboard_tests
[ 93%] Linking C executable storage_atomic_tests
[ 93%] Built target test_support_tests
[ 93%] Linking C executable storage_repository_lock_tests
[ 93%] Linking C executable auth_tests
[ 94%] Linking C executable storage_parent_sync_tests
[ 94%] Linking C executable macro_executor_tests
[ 95%] Linking C executable web_request_policy_tests
[ 95%] Linking C executable macro_parser_tests
[ 94%] Built target provisioning_bootstrap_tests
[ 95%] Built target web_api_json_tests
[ 95%] Linking C executable provisioning_tests
[ 95%] Built target device_controls_tests
[ 95%] Linking C executable web_api_dispatch_tests
[ 96%] Linking C executable web_setup_json_tests
[ 96%] Built target provisioning_settings_tests
[ 96%] Linking C executable storage_repository_io_tests
[ 96%] Built target web_api_response_tests
[ 96%] Linking C executable web_api_admin_boundary_tests
[ 96%] Built target storage_transaction_tests
[ 96%] Built target storage_atomic_tests
[ 97%] Linking C executable web_setup_tests
[ 97%] Built target wifi_ap_tests
[ 97%] Built target web_server_adapter_tests
[ 97%] Built target web_api_core_tests
[ 97%] Built target storage_repository_lock_tests
[ 97%] Linking C executable web_execution_submit_tests
[ 97%] Built target macro_parser_tests
[ 97%] Built target macro_executor_tests
[ 97%] Built target web_api_dispatch_tests
[ 97%] Built target auth_tests
[ 97%] Linking C executable storage_atomic_validators_tests
[ 97%] Built target web_request_policy_tests
[ 97%] Built target storage_parent_sync_tests
[ 97%] Built target provisioning_tests
[ 97%] Built target web_setup_json_tests
[ 97%] Built target web_setup_tests
[ 97%] Built target web_api_admin_boundary_tests
[ 98%] Linking C executable app_core_tests
[ 98%] Linking C executable storage_procedure_repository_tests
[ 98%] Built target storage_repository_io_tests
[ 98%] Linking C executable storage_object_json_tests
[ 98%] Linking C executable storage_repository_tests
[ 98%] Linking C executable storage_atomic_recovery_tests
[ 99%] Linking C executable storage_progress_repository_tests
[ 99%] Built target web_execution_submit_tests
[ 99%] Built target storage_procedure_repository_tests
[ 99%] Linking C executable web_api_repository_handlers_tests
[ 99%] Built target storage_atomic_recovery_tests
[ 99%] Built target storage_repository_tests
[ 99%] Built target storage_atomic_validators_tests
[ 99%] Built target app_core_tests
[ 99%] Built target storage_progress_repository_tests
[ 99%] Built target storage_object_json_tests
[ 99%] Built target web_api_repository_handlers_tests
[ 99%] Linking C executable storage_quarantine_tests
[100%] Linking C executable storage_macro_repository_tests
[100%] Linking C executable storage_active_set_delete_tests
[100%] Built target storage_quarantine_tests
[100%] Built target storage_macro_repository_tests
[100%] Built target storage_active_set_delete_tests
Internal ctest changing into directory: /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/build
Test project /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/build
      Start  7: web_security
 1/12 Test  #7: web_security .....................   Passed    0.00 sec
      Start  8: web_server_adapter
 2/12 Test  #8: web_server_adapter ...............   Passed    0.00 sec
      Start 27: web_api_core
 3/12 Test #27: web_api_core .....................   Passed    0.00 sec
      Start 28: web_request_policy
 4/12 Test #28: web_request_policy ...............   Passed    0.00 sec
      Start 29: web_execution_submit
 5/12 Test #29: web_execution_submit .............   Passed    0.00 sec
      Start 30: web_api_json
 6/12 Test #30: web_api_json .....................***Failed    0.00 sec
test failure at /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_api_json.c:277: revision == 0U; expected=0, actual=4

      Start 31: web_api_response
 7/12 Test #31: web_api_response .................   Passed    0.00 sec
      Start 32: web_api_dispatch
 8/12 Test #32: web_api_dispatch .................   Passed    0.00 sec
      Start 33: web_api_admin_boundary
 9/12 Test #33: web_api_admin_boundary ...........   Passed    0.00 sec
      Start 34: web_api_repository_handlers
10/12 Test #34: web_api_repository_handlers ......   Passed    0.09 sec
      Start 36: web_setup
11/12 Test #36: web_setup ........................   Passed    0.00 sec
      Start 37: web_setup_json
12/12 Test #37: web_setup_json ...................   Passed    0.00 sec

92% tests passed, 1 tests failed out of 12

Label Time Summary:
web    =   0.10 sec*proc (12 tests)

Total Test time (real) =   0.10 sec

The following tests FAILED:
	 30 - web_api_json (Failed)                             web
Errors while running CTest
```
