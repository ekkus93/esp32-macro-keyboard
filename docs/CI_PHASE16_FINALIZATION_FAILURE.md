# Phase 16 finalization failure

- Transform: success
- Cleanup: success
- Format: success
- Focused tests: failure
- Authoritative gate: skipped
- Coverage/device evidence: skipped

## phase16-finalization-transform.log

```text

Phase 16 finalization input normalized
Phase 16 request JSON hardening transform applied
Phase 16 finalization transform applied
```

## phase16-finalization-cleanup.log

```text

.github/workflows/phase16-http-integration-once.yml
.github/workflows/phase16-http-status-probe.yml
.github/workflows/inspect-phase16-http-failure.yml
scripts/run-phase16-policy-hardening.py
scripts/run-phase16-clang-tidy-cleanup.py
scripts/apply-phase16-execution-route-policy.py
scripts/run-phase16-execution-route-policy.py
scripts/apply-phase16-acceptance-registration.py
scripts/apply-phase16-http-integration.py
scripts/run-phase16-clang-tidy-cleanup-v2.py
scripts/apply-phase16-admin-boundary.py
scripts/run-phase16-clang-tidy-cleanup-v3.py
scripts/fix-phase16-clang-tidy.py
scripts/apply-phase16-finalization.py
scripts/apply-phase16-policy-hardening.py
scripts/apply-phase16-request-json.py
scripts/apply-phase16-handler-acceptance.py
scripts/patch-phase16-finalization-matcher.py
scripts/normalize-phase16-finalization-input.py
```

## phase16-finalization-format.log

```text

```

## phase16-finalization-focused.log

```text
328:/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_execution_route_policy.c:56:10: error: assignment of read-only variable ‘path’

[ 31%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 31%] Building C object CMakeFiles/web_api_dispatch_tests.dir/test_web_api_dispatch.c.o
[ 31%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 32%] Building C object CMakeFiles/web_execution_route_policy_tests.dir/test_web_execution_route_policy.c.o
[ 32%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_request_policy.c.o
[ 25%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 32%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 32%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 33%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 33%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 33%] Building C object CMakeFiles/web_execution_route_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 34%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 34%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 35%] Building C object CMakeFiles/web_api_response_tests.dir/test_web_api_response.c.o
[ 36%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 36%] Building C object CMakeFiles/web_execution_route_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 36%] Building C object CMakeFiles/web_setup_tests.dir/test_web_setup.c.o
[ 37%] Building C object CMakeFiles/web_api_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 37%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 37%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 37%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 38%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 39%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 39%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 39%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 39%] Building C object CMakeFiles/web_api_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 40%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 41%] Building C object CMakeFiles/storage_parent_sync_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 42%] Building C object CMakeFiles/web_execution_submit_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 42%] Building C object CMakeFiles/storage_object_json_tests.dir/test_storage_object_json.c.o
[ 43%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 44%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 44%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_set_operations.c.o
[ 44%] Building C object CMakeFiles/web_api_admin_boundary_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 44%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 44%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 44%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 45%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 46%] Building C object CMakeFiles/storage_object_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 36%] Building C object CMakeFiles/web_api_admin_boundary_tests.dir/test_web_api_admin_boundary.c.o
[ 38%] Building C object CMakeFiles/web_execution_route_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_execution_route_policy.c.o
[ 42%] Building C object CMakeFiles/web_setup_json_tests.dir/test_web_setup_json.c.o
[ 42%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 47%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 47%] Building C object CMakeFiles/provisioning_bootstrap_tests.dir/test_provisioning_bootstrap.c.o
[ 47%] Building C object CMakeFiles/web_api_dispatch_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c.o
[ 48%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 48%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/test_web_api_repository_handlers.c.o
[ 48%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 49%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 49%] Building C object CMakeFiles/web_api_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 50%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 30%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 30%] Building C object CMakeFiles/provisioning_settings_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/provisioning/provisioning_core.c.o
[ 46%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 30%] Building C object CMakeFiles/provisioning_settings_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 30%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 30%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 30%] Building C object CMakeFiles/web_api_core_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_core.c.o
[ 30%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 30%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 30%] Building C object CMakeFiles/storage_repository_lock_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 30%] Building C object CMakeFiles/storage_parent_sync_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 30%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 30%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 51%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 51%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 51%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_set_operations.c.o
[ 51%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_progress.c.o
[ 52%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_progress.c.o
[ 54%] Building C object CMakeFiles/storage_macro_repository_tests.dir/test_storage_macros.c.o
[ 54%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 54%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 55%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/test_storage_procedures.c.o
[ 55%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 55%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 56%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 56%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 57%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_core.c.o
[ 57%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 57%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 58%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 58%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 58%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 58%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 58%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 58%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 58%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 58%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 58%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 59%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 59%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 59%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 59%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 59%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 59%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 59%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 59%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 60%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 61%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 61%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 61%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 63%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 63%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 64%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 64%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 65%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 66%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 66%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 66%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 66%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 67%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 63%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 64%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 69%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 70%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 70%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_set_operations.c.o
[ 70%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 70%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 70%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_progress.c.o
[ 70%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 71%] Building C object CMakeFiles/web_api_admin_boundary_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c.o
[ 72%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 73%] Building C object CMakeFiles/web_api_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 74%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_origin.c.o
[ 75%] Building C object CMakeFiles/web_api_admin_boundary_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 76%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 76%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_keymap_us.c.o
[ 76%] Linking C executable app_operation_result_tests
[ 77%] Building C object CMakeFiles/web_setup_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_setup_json.c.o
[ 78%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 79%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c.o
[ 79%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_json.c.o
[ 79%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_parser.c.o
[ 80%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 80%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_core.c.o
[ 51%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_cookie.c.o
[ 51%] Building C object CMakeFiles/web_api_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 51%] Building C object CMakeFiles/storage_object_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 51%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 51%] Building C object CMakeFiles/web_api_admin_boundary_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_core.c.o
[ 51%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 51%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 51%] Building C object CMakeFiles/storage_object_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 51%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 51%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 51%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 51%] Building C object CMakeFiles/web_execution_submit_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 51%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 51%] Building C object CMakeFiles/provisioning_bootstrap_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/provisioning/provisioning_bootstrap_core.c.o
[ 51%] Building C object CMakeFiles/web_api_json_tests.dir/test_web_api_json.c.o
[ 51%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 51%] Building C object CMakeFiles/web_api_dispatch_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 51%] Building C object CMakeFiles/web_api_admin_boundary_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_admin_boundary.c.o
[ 51%] Building C object CMakeFiles/web_api_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 51%] Building C object CMakeFiles/storage_object_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 51%] Building C object CMakeFiles/web_execution_submit_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 51%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 51%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 51%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 51%] Building C object CMakeFiles/web_api_response_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 51%] Building C object CMakeFiles/web_api_response_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c.o
[ 51%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 51%] Building C object CMakeFiles/storage_progress_repository_tests.dir/test_storage_progress.c.o
[ 51%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 51%] Building C object CMakeFiles/web_execution_submit_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_execution_submit.c.o
[ 51%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 51%] Building C object CMakeFiles/storage_mount_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_mount_topology.c.o
[ 51%] Building C object CMakeFiles/web_setup_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_setup_core.c.o
[ 51%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 51%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic_validators.c.o
[ 51%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 81%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 81%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_sets.c.o
[ 82%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 83%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_macros.c.o
[ 84%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 84%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_procedures.c.o
[ 85%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 86%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 81%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 81%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 81%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 81%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 81%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 81%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 81%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 81%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 81%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 81%] Building C object CMakeFiles/web_api_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_json.c.o
[ 81%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 86%] Building C object CMakeFiles/web_api_dispatch_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_dispatch.c.o
[ 86%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_handler_common.c.o
/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_execution_route_policy.c: In function ‘test_ready’:
/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_execution_route_policy.c:56:10: error: assignment of read-only variable ‘path’
   56 |     path = current_path();
      |          ^
[ 87%] Linking C executable macro_model_tests
gmake[2]: *** [CMakeFiles/web_execution_route_policy_tests.dir/build.make:79: CMakeFiles/web_execution_route_policy_tests.dir/test_web_execution_route_policy.c.o] Error 1
gmake[1]: *** [CMakeFiles/Makefile2:1170: CMakeFiles/web_execution_route_policy_tests.dir/all] Error 2
gmake[1]: *** Waiting for unfinished jobs....
[ 87%] Built target app_operation_result_tests
[ 88%] Linking C executable web_security_tests
[ 88%] Linking C executable test_support_tests
[ 88%] Linking C executable web_setup_json_tests
[ 88%] Linking C executable storage_repository_lock_tests
[ 90%] Linking C executable macro_parser_tests
[ 90%] Linking C executable web_request_policy_tests
[ 90%] Linking C executable storage_atomic_tests
[ 90%] Linking C executable usb_keyboard_tests
[ 89%] Linking C executable web_api_core_tests
[ 90%] Linking C executable web_api_response_tests
[ 90%] Built target macro_model_tests
[ 90%] Linking C executable web_api_dispatch_tests
[ 90%] Linking C executable device_controls_tests
[ 90%] Linking C executable web_server_adapter_tests
[ 90%] Linking C executable storage_mount_tests
[ 90%] Linking C executable web_api_admin_boundary_tests
[ 90%] Built target web_security_tests
[ 90%] Linking C executable provisioning_tests
[ 90%] Built target test_support_tests
[ 90%] Linking C executable auth_tests
[ 91%] Linking C executable wifi_ap_tests
[ 91%] Linking C executable storage_repository_io_tests
[ 91%] Built target storage_repository_lock_tests
[ 91%] Linking C executable storage_parent_sync_tests
[ 92%] Linking C executable web_execution_submit_tests
[ 92%] Linking C executable storage_transaction_tests
[ 92%] Linking C executable macro_executor_tests
[ 93%] Linking C executable provisioning_bootstrap_tests
[ 94%] Linking C executable web_setup_tests
[ 94%] Built target usb_keyboard_tests
[ 94%] Built target web_api_core_tests
[ 94%] Built target web_api_response_tests
[ 94%] Built target web_setup_json_tests
[ 94%] Built target web_api_dispatch_tests
[ 94%] Built target storage_atomic_tests
[ 94%] Built target macro_parser_tests
[ 94%] Built target web_request_policy_tests
[ 94%] Built target device_controls_tests
[ 94%] Built target web_api_admin_boundary_tests
[ 94%] Linking C executable storage_atomic_recovery_tests
[ 95%] Linking C executable provisioning_settings_tests
[ 95%] Built target provisioning_tests
[ 95%] Built target auth_tests
[ 95%] Built target storage_mount_tests
[ 95%] Built target wifi_ap_tests
[ 96%] Linking C executable web_api_json_tests
[ 96%] Built target web_server_adapter_tests
[ 96%] Built target web_execution_submit_tests
[ 95%] Built target storage_parent_sync_tests
[ 96%] Built target storage_repository_io_tests
[ 96%] Built target macro_executor_tests
[ 96%] Built target storage_transaction_tests
[ 97%] Linking C executable storage_macro_repository_tests
[ 97%] Built target provisioning_settings_tests
[ 97%] Built target web_setup_tests
[ 97%] Built target provisioning_bootstrap_tests
[ 97%] Linking C executable storage_atomic_validators_tests
[ 97%] Built target storage_atomic_recovery_tests
[ 97%] Linking C executable web_api_repository_handlers_tests
[ 97%] Built target web_api_json_tests
[ 97%] Built target storage_atomic_validators_tests
[ 97%] Built target storage_macro_repository_tests
[ 97%] Linking C executable storage_repository_tests
[ 98%] Linking C executable storage_quarantine_tests
[ 99%] Linking C executable app_core_tests
[ 99%] Built target web_api_repository_handlers_tests
[ 99%] Linking C executable storage_progress_repository_tests
[ 99%] Linking C executable storage_procedure_repository_tests
[100%] Linking C executable storage_object_json_tests
[100%] Built target storage_quarantine_tests
[100%] Built target app_core_tests
[100%] Linking C executable storage_active_set_delete_tests
[100%] Built target storage_procedure_repository_tests
[100%] Built target storage_repository_tests
[100%] Built target storage_progress_repository_tests
[100%] Built target storage_active_set_delete_tests
[100%] Built target storage_object_json_tests
gmake: *** [Makefile:101: all] Error 2
```
