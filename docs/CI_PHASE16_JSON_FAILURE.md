# Phase 16 request JSON hardening failure

- Transform: success
- Format: success
- Focused tests: failure
- Authoritative gate: skipped

## phase16-json-transform.log

```text

Phase 16 request JSON hardening transform applied
```

## phase16-json-format.log

```text

```

## phase16-json-focused.log

```text

[ 48%] Building C object CMakeFiles/web_api_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 48%] Building C object CMakeFiles/storage_object_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 49%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 50%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 50%] Building C object CMakeFiles/web_api_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_json.c.o
[ 50%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 50%] Building C object CMakeFiles/web_api_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 50%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 50%] Building C object CMakeFiles/web_api_admin_boundary_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_admin_boundary.c.o
[ 51%] Building C object CMakeFiles/storage_object_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 51%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 51%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 52%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 52%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 43%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 52%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 40%] Building C object CMakeFiles/web_api_dispatch_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_dispatch.c.o
[ 53%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 54%] Building C object CMakeFiles/web_setup_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_setup_json.c.o
[ 55%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 48%] Building C object CMakeFiles/web_api_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 55%] Building C object CMakeFiles/web_api_admin_boundary_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_core.c.o
[ 56%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 57%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 57%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 57%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 57%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 58%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 59%] Building C object CMakeFiles/web_api_admin_boundary_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c.o
[ 60%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 60%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 60%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 60%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 60%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 60%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 61%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 61%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 62%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 63%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 53%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 63%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 63%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 63%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 63%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 63%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 63%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 64%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_progress.c.o
[ 65%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 66%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 66%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 66%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/test_storage_procedures.c.o
[ 65%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 67%] Building C object CMakeFiles/web_execution_route_policy_tests.dir/test_web_execution_route_policy.c.o
[ 67%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 67%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 67%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_progress.c.o
[ 66%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 33%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 33%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 33%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 33%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic_validators.c.o
[ 33%] Building C object CMakeFiles/storage_parent_sync_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 33%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 33%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 33%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/test_storage_atomic_validators.c.o
[ 33%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 33%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 33%] Building C object CMakeFiles/web_execution_submit_tests.dir/test_web_execution_submit.c.o
[ 33%] Building C object CMakeFiles/provisioning_settings_tests.dir/test_provisioning_settings.c.o
[ 33%] Building C object CMakeFiles/web_api_core_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_core.c.o
[ 33%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 33%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/test_storage_active_set_delete.c.o
[ 33%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 33%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 33%] Building C object CMakeFiles/web_api_core_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 33%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 68%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 68%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 69%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 70%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_core.c.o
[ 70%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 70%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 71%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 72%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 72%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_request_policy.c.o
[ 72%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_json.c.o
[ 72%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_handler_common.c.o
[ 72%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 72%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_sets.c.o
[ 72%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_cookie.c.o
[ 72%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 73%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 74%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_origin.c.o
[ 75%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_macros.c.o
[ 69%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 68%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 75%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 76%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c.o
[ 76%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 77%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 78%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 79%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 80%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 81%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 81%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_procedures.c.o
[ 81%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 82%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 83%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 84%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 85%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 68%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 68%] Building C object CMakeFiles/web_execution_submit_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_execution_submit.c.o
[ 68%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 68%] Building C object CMakeFiles/provisioning_bootstrap_tests.dir/test_provisioning_bootstrap.c.o
[ 68%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 68%] Building C object CMakeFiles/web_api_json_tests.dir/test_web_api_json.c.o
[ 68%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 68%] Building C object CMakeFiles/web_api_dispatch_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 68%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 68%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 68%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 68%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 68%] Building C object CMakeFiles/web_request_policy_tests.dir/test_web_request_policy.c.o
[ 68%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 68%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 68%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 68%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 68%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 68%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 68%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 68%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 68%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 86%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_keymap_us.c.o
[ 87%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_parser.c.o
[ 88%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 86%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 86%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_core.c.o
[ 86%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 86%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 86%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 86%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 88%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 88%] Linking C executable web_api_response_tests
[ 89%] Linking C executable app_operation_result_tests
[ 89%] Linking C executable web_api_dispatch_tests
[ 89%] Linking C executable storage_repository_lock_tests
[ 89%] Linking C executable macro_model_tests
[ 89%] Linking C executable test_support_tests
[ 90%] Linking C executable web_security_tests
[ 90%] Linking C executable web_execution_route_policy_tests
[ 91%] Linking C executable provisioning_bootstrap_tests
[ 90%] Linking C executable web_setup_json_tests
[ 91%] Built target web_api_response_tests
[ 91%] Built target app_operation_result_tests
[ 91%] Built target web_api_dispatch_tests
[ 91%] Built target storage_repository_lock_tests
[ 91%] Linking C executable storage_mount_tests
[ 91%] Linking C executable web_api_admin_boundary_tests
[ 91%] Built target macro_model_tests
[ 91%] Built target web_security_tests
[ 91%] Linking C executable storage_atomic_tests
[ 91%] Built target test_support_tests
[ 92%] Linking C executable web_api_core_tests
[ 92%] Built target provisioning_bootstrap_tests
[ 92%] Built target web_execution_route_policy_tests
[ 92%] Built target web_setup_json_tests
[ 92%] Linking C executable storage_repository_io_tests
[ 93%] Linking C executable macro_parser_tests
[ 93%] Linking C executable storage_parent_sync_tests
[ 93%] Built target storage_mount_tests
[ 94%] Linking C executable web_execution_submit_tests
[ 94%] Built target web_api_admin_boundary_tests
[ 94%] Built target web_api_core_tests
[ 94%] Built target storage_atomic_tests
[ 95%] Linking C executable web_setup_tests
[ 95%] Linking C executable web_request_policy_tests
[ 95%] Linking C executable device_controls_tests
[ 95%] Linking C executable web_server_adapter_tests
[ 95%] Built target macro_parser_tests
[ 95%] Built target storage_repository_io_tests
[ 96%] Linking C executable wifi_ap_tests
[ 95%] Built target storage_parent_sync_tests
[ 96%] Built target web_execution_submit_tests
[ 96%] Linking C executable usb_keyboard_tests
[ 96%] Built target web_setup_tests
[ 96%] Linking C executable auth_tests
[ 96%] Built target web_request_policy_tests
[ 96%] Linking C executable provisioning_tests
[ 96%] Built target device_controls_tests
[ 96%] Built target web_server_adapter_tests
[ 96%] Built target auth_tests
[ 97%] Built target wifi_ap_tests
[ 97%] Linking C executable storage_transaction_tests
[ 97%] Linking C executable web_api_json_tests
[ 97%] Built target usb_keyboard_tests
[ 97%] Linking C executable provisioning_settings_tests
[ 97%] Linking C executable macro_executor_tests
[ 97%] Linking C executable storage_object_json_tests
[ 97%] Built target provisioning_tests
[ 97%] Built target web_api_json_tests
[ 97%] Built target provisioning_settings_tests
[ 97%] Linking C executable storage_atomic_recovery_tests
[ 98%] Linking C executable storage_macro_repository_tests
[ 98%] Built target storage_transaction_tests
[ 99%] Linking C executable storage_quarantine_tests
[ 99%] Built target storage_object_json_tests
[ 99%] Linking C executable storage_atomic_validators_tests
[ 99%] Linking C executable storage_active_set_delete_tests
[ 99%] Built target macro_executor_tests
[ 99%] Linking C executable storage_procedure_repository_tests
[ 99%] Built target storage_macro_repository_tests
[ 99%] Linking C executable storage_repository_tests
[100%] Linking C executable app_core_tests
[100%] Linking C executable storage_progress_repository_tests
[100%] Built target storage_repository_tests
[100%] Built target storage_active_set_delete_tests
[100%] Built target storage_atomic_validators_tests
[100%] Built target storage_atomic_recovery_tests
[100%] Linking C executable web_api_repository_handlers_tests
[100%] Built target app_core_tests
[100%] Built target storage_quarantine_tests
[100%] Built target storage_procedure_repository_tests
[100%] Built target storage_progress_repository_tests
[100%] Built target web_api_repository_handlers_tests
Internal ctest changing into directory: /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/build-sanitizers
Test project /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/build-sanitizers
      Start  7: web_security
 1/13 Test  #7: web_security .....................   Passed    0.20 sec
      Start  8: web_server_adapter
 2/13 Test  #8: web_server_adapter ...............   Passed    0.01 sec
      Start 27: web_api_core
 3/13 Test #27: web_api_core .....................   Passed    0.01 sec
      Start 28: web_request_policy
 4/13 Test #28: web_request_policy ...............   Passed    0.01 sec
      Start 29: web_execution_submit
 5/13 Test #29: web_execution_submit .............   Passed    0.01 sec
      Start 30: web_execution_route_policy
 6/13 Test #30: web_execution_route_policy .......   Passed    0.01 sec
      Start 31: web_api_json
 7/13 Test #31: web_api_json .....................   Passed    0.01 sec
      Start 32: web_api_response
 8/13 Test #32: web_api_response .................   Passed    0.01 sec
      Start 33: web_api_dispatch
 9/13 Test #33: web_api_dispatch .................   Passed    0.01 sec
      Start 34: web_api_admin_boundary
10/13 Test #34: web_api_admin_boundary ...........   Passed    0.01 sec
      Start 35: web_api_repository_handlers
11/13 Test #35: web_api_repository_handlers ......   Passed    0.06 sec
      Start 37: web_setup
12/13 Test #37: web_setup ........................   Passed    0.01 sec
      Start 38: web_setup_json
13/13 Test #38: web_setup_json ...................   Passed    0.01 sec

100% tests passed, 0 tests failed out of 13

Label Time Summary:
web    =   0.35 sec*proc (13 tests)

Total Test time (real) =   0.36 sec
gcovr is required for native coverage generation
```
