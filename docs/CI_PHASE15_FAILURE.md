# FIX1 Phase 15 automation failure

Apply outcome: success
Apply status: 0
Validation outcome: failure
Validation status: 1
Evidence outcome: skipped

## Apply log

```text
```

## Validation log

```text
[  4%] Building C object CMakeFiles/test_support.dir/fakes/fake_fs_backend.c.o
[  5%] Linking C static library libtest_support.a
[  5%] Built target test_support
[  5%] Building C object CMakeFiles/test_support_tests.dir/test_support.c.o
[  5%] Building C object CMakeFiles/app_operation_result_tests.dir/test_app_operation_result.c.o
[  5%] Building C object CMakeFiles/macro_model_tests.dir/test_macro_model.c.o
[  6%] Building C object CMakeFiles/macro_executor_tests.dir/test_macro_executor.c.o
[  7%] Building C object CMakeFiles/app_operation_result_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/support/app_operation_result.c.o
[  8%] Building C object CMakeFiles/auth_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/auth/auth_core_common.c.o
[  8%] Building C object CMakeFiles/web_security_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_cookie.c.o
[  8%] Building C object CMakeFiles/auth_tests.dir/test_auth.c.o
[  8%] Building C object CMakeFiles/macro_model_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[  8%] Building C object CMakeFiles/web_security_tests.dir/test_web_security.c.o
[  8%] Building C object CMakeFiles/web_security_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_static_path.c.o
[  9%] Building C object CMakeFiles/web_security_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_origin.c.o
[  9%] Building C object CMakeFiles/auth_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/auth/auth_core_password.c.o
[  9%] Building C object CMakeFiles/auth_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/auth/auth_core_rate_limit.c.o
[ 10%] Building C object CMakeFiles/macro_parser_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_keymap_us.c.o
[ 10%] Building C object CMakeFiles/macro_parser_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[  8%] Building C object CMakeFiles/macro_parser_tests.dir/test_macro_parser.c.o
[  8%] Building C object CMakeFiles/macro_executor_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_executor/macro_executor_engine.c.o
[ 11%] Building C object CMakeFiles/macro_parser_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 11%] Building C object CMakeFiles/macro_parser_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 12%] Building C object CMakeFiles/auth_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/auth/auth_core_session.c.o
[ 12%] Building C object CMakeFiles/web_security_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_content.c.o
[ 12%] Building C object CMakeFiles/macro_parser_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_parser.c.o
[ 13%] Building C object CMakeFiles/device_controls_tests.dir/test_device_controls.c.o
[ 13%] Building C object CMakeFiles/device_controls_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/device_controls/device_controls_logic.c.o
[ 13%] Building C object CMakeFiles/storage_mount_tests.dir/test_storage_mount.c.o
[ 13%] Building C object CMakeFiles/web_server_adapter_tests.dir/test_web_server_adapter.c.o
[ 13%] Building C object CMakeFiles/provisioning_tests.dir/test_provisioning.c.o
[ 13%] Building C object CMakeFiles/wifi_ap_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/wifi_ap/wifi_ap_state.c.o
[ 14%] Building C object CMakeFiles/wifi_ap_tests.dir/test_wifi_ap.c.o
[ 14%] Building C object CMakeFiles/app_core_tests.dir/test_app_core.c.o
[ 14%] Building C object CMakeFiles/storage_parent_sync_tests.dir/test_storage_parent_sync.c.o
[ 15%] Building C object CMakeFiles/storage_mount_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 17%] Building C object CMakeFiles/storage_repository_io_tests.dir/test_storage_repository_io.c.o
[ 17%] Building C object CMakeFiles/app_core_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/app_core/app_core_sequence.c.o
[ 18%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_adapter_common.c.o
[ 18%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 18%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_adapter_body_auth.c.o
[ 18%] Building C object CMakeFiles/storage_mount_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_mount_topology.c.o
[ 19%] Building C object CMakeFiles/provisioning_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 13%] Building C object CMakeFiles/storage_mount_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_mount_core.c.o
[ 19%] Building C object CMakeFiles/storage_repository_lock_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 19%] Building C object CMakeFiles/usb_keyboard_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/usb_keyboard/usb_keyboard_state.c.o
[ 19%] Building C object CMakeFiles/storage_repository_lock_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 19%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_adapter_static_stream.c.o
[ 20%] Building C object CMakeFiles/storage_parent_sync_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 22%] Building C object CMakeFiles/storage_parent_sync_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 16%] Building C object CMakeFiles/app_core_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/support/app_operation_result.c.o
[ 22%] Building C object CMakeFiles/storage_atomic_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 23%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_origin.c.o
[ 23%] Building C object CMakeFiles/storage_repository_lock_tests.dir/test_storage_repository_lock.c.o
[ 23%] Building C object CMakeFiles/storage_atomic_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 24%] Building C object CMakeFiles/storage_parent_sync_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 25%] Building C object CMakeFiles/usb_keyboard_tests.dir/test_usb_keyboard.c.o
[ 25%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 19%] Building C object CMakeFiles/provisioning_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/provisioning/provisioning_core.c.o
[ 19%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_adapter_lifecycle.c.o
[ 21%] Building C object CMakeFiles/storage_atomic_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 25%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 25%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_cookie.c.o
[ 25%] Building C object CMakeFiles/storage_transaction_tests.dir/test_storage_transactions.c.o
[ 25%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 25%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 26%] Building C object CMakeFiles/provisioning_bootstrap_tests.dir/test_provisioning_bootstrap.c.o
[ 27%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 27%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 28%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 28%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 29%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 29%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic_recovery.c.o
[ 29%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 29%] Building C object CMakeFiles/storage_repository_tests.dir/test_storage_repository.c.o
[ 29%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_static_path.c.o
[ 29%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 30%] Building C object CMakeFiles/storage_atomic_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 30%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 31%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_content.c.o
[ 31%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic_validators.c.o
[ 31%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 31%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 33%] Building C object CMakeFiles/storage_quarantine_tests.dir/test_storage_quarantine.c.o
[ 33%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 33%] Building C object CMakeFiles/storage_object_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 33%] Building C object CMakeFiles/provisioning_settings_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/provisioning/provisioning_core.c.o
[ 33%] Building C object CMakeFiles/storage_object_json_tests.dir/test_storage_object_json.c.o
[ 33%] Building C object CMakeFiles/provisioning_settings_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 33%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 34%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 35%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 35%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 36%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 37%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 37%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 32%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 38%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 33%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 39%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 39%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 33%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 40%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/test_storage_procedures.c.o
[ 41%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_adapter_json.c.o
[ 42%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 42%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 42%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 35%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 42%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 42%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 42%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 42%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 43%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 43%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 44%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 44%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 44%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 44%] Building C object CMakeFiles/storage_macro_repository_tests.dir/test_storage_macros.c.o
[ 45%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 45%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 45%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 46%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 47%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 47%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 48%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 48%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 49%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 49%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 50%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 50%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 50%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 51%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 51%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 52%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 52%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 52%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 53%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 53%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_progress.c.o
[ 54%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 55%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 55%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 55%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 56%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/test_storage_atomic_validators.c.o
[ 57%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 58%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 58%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 48%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 59%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 60%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 61%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 62%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 63%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 63%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 64%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 64%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 53%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 64%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 56%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 66%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 67%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 67%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 67%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 68%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/test_storage_active_set_delete.c.o
[ 68%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 69%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 70%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 71%] Building C object CMakeFiles/provisioning_settings_tests.dir/test_provisioning_settings.c.o
[ 71%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 72%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 72%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 73%] Building C object CMakeFiles/storage_object_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 74%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 74%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 75%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 76%] Building C object CMakeFiles/web_setup_tests.dir/test_web_setup.c.o
[ 77%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic_validators.c.o
[ 78%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 79%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 38%] Building C object CMakeFiles/storage_atomic_tests.dir/test_storage_atomic.c.o
[ 38%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 38%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 38%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 38%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 38%] Building C object CMakeFiles/storage_parent_sync_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 38%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/test_storage_atomic_recovery.c.o
[ 38%] Building C object CMakeFiles/web_setup_json_tests.dir/test_web_setup_json.c.o
[ 38%] Building C object CMakeFiles/storage_object_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 38%] Building C object CMakeFiles/web_setup_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_setup_json.c.o
[ 38%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 38%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 38%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 38%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 80%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 81%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 82%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 82%] Linking C executable app_operation_result_tests
[ 83%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 84%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 85%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 86%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 87%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 88%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_progress.c.o
[ 89%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 90%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 80%] Building C object CMakeFiles/web_setup_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_setup_core.c.o
[ 80%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 80%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 80%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 80%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 80%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 80%] Building C object CMakeFiles/storage_object_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 80%] Building C object CMakeFiles/provisioning_bootstrap_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/provisioning/provisioning_bootstrap_core.c.o
[ 80%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 80%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 80%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 80%] Building C object CMakeFiles/storage_progress_repository_tests.dir/test_storage_progress.c.o
[ 80%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 80%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 91%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 80%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 80%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 80%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 80%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 91%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 80%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 80%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 80%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 80%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 80%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 80%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 91%] Linking C executable test_support_tests
[ 92%] Linking C executable storage_mount_tests
[ 93%] Built target app_operation_result_tests
[ 94%] Linking C executable macro_model_tests
[ 93%] Linking C executable web_security_tests
[ 95%] Linking C executable storage_repository_lock_tests
[ 95%] Linking C executable macro_parser_tests
[ 95%] Linking C executable provisioning_bootstrap_tests
[ 95%] Linking C executable storage_parent_sync_tests
[ 95%] Linking C executable web_setup_json_tests
[ 95%] Built target test_support_tests
[ 95%] Linking C executable device_controls_tests
[ 95%] Built target storage_mount_tests
[ 95%] Built target macro_model_tests
[ 96%] Linking C executable provisioning_settings_tests
[ 96%] Linking C executable web_server_adapter_tests
[ 97%] Linking C executable auth_tests
[ 97%] Built target web_security_tests
[ 97%] Built target storage_repository_lock_tests
[ 97%] Built target provisioning_bootstrap_tests
[ 97%] Linking C executable storage_repository_io_tests
[ 98%] Linking C executable wifi_ap_tests
[ 98%] Linking C executable provisioning_tests
[ 98%] Linking C executable usb_keyboard_tests
[ 98%] Built target macro_parser_tests
[ 98%] Built target storage_parent_sync_tests
[ 98%] Linking C executable storage_atomic_tests
[ 98%] Linking C executable web_setup_tests
[ 98%] Linking C executable storage_transaction_tests
[ 99%] Linking C executable macro_executor_tests
[ 99%] Built target web_setup_json_tests
[ 99%] Built target device_controls_tests
[ 99%] Built target auth_tests
[ 99%] Built target provisioning_tests
[ 99%] Built target provisioning_settings_tests
[ 99%] Built target storage_transaction_tests
[ 99%] Built target web_server_adapter_tests
[ 99%] Built target wifi_ap_tests
[ 99%] Linking C executable storage_object_json_tests
[ 99%] Built target storage_repository_io_tests
[ 99%] Built target usb_keyboard_tests
[ 99%] Linking C executable storage_atomic_validators_tests
[100%] Linking C executable storage_procedure_repository_tests
[100%] Built target web_setup_tests
[100%] Built target storage_atomic_tests
[100%] Built target macro_executor_tests
[100%] Linking C executable storage_active_set_delete_tests
[100%] Linking C executable storage_progress_repository_tests
[100%] Linking C executable storage_quarantine_tests
[100%] Linking C executable storage_macro_repository_tests
[100%] Linking C executable storage_repository_tests
[100%] Built target storage_object_json_tests
[100%] Linking C executable app_core_tests
[100%] Built target storage_procedure_repository_tests
[100%] Built target storage_progress_repository_tests
[100%] Built target storage_quarantine_tests
[100%] Built target storage_repository_tests
[100%] Linking C executable storage_atomic_recovery_tests
[100%] Built target storage_atomic_validators_tests
[100%] Built target app_core_tests
[100%] Built target storage_active_set_delete_tests
[100%] Built target storage_macro_repository_tests
[100%] Built target storage_atomic_recovery_tests
Internal ctest changing into directory: /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/build
Test project /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/build
      Start 13: provisioning
 1/18 Test #13: provisioning ........................   Passed    0.00 sec
      Start 14: storage_mount
 2/18 Test #14: storage_mount .......................   Passed    0.00 sec
      Start 15: storage_atomic_recovery
 3/18 Test #15: storage_atomic_recovery .............   Passed    0.03 sec
      Start 16: storage_atomic_validators
 4/18 Test #16: storage_atomic_validators ...........   Passed    0.00 sec
      Start 17: storage_atomic
 5/18 Test #17: storage_atomic ......................   Passed    0.03 sec
      Start 18: storage_repository_lock
 6/18 Test #18: storage_repository_lock .............   Passed    0.00 sec
      Start 19: storage_parent_sync
 7/18 Test #19: storage_parent_sync .................   Passed    0.01 sec
      Start 20: storage_repository_io
 8/18 Test #20: storage_repository_io ...............   Passed    0.00 sec
      Start 21: storage_transaction
 9/18 Test #21: storage_transaction .................   Passed    0.07 sec
      Start 22: storage_quarantine
10/18 Test #22: storage_quarantine ..................   Passed    0.13 sec
      Start 23: storage_repository
11/18 Test #23: storage_repository ..................   Passed    0.61 sec
      Start 24: storage_progress_repository_tests
12/18 Test #24: storage_progress_repository_tests ...   Passed    0.08 sec
      Start 25: storage_active_set_delete_tests
13/18 Test #25: storage_active_set_delete_tests .....   Passed    0.05 sec
      Start 26: provisioning_settings
14/18 Test #26: provisioning_settings ...............   Passed    0.00 sec
      Start 27: provisioning_bootstrap
15/18 Test #27: provisioning_bootstrap ..............   Passed    0.00 sec
      Start 30: storage_object_json
16/18 Test #30: storage_object_json .................   Passed    0.00 sec
      Start 31: storage_macro_repository
17/18 Test #31: storage_macro_repository ............   Passed    0.09 sec
      Start 32: storage_procedure_repository
18/18 Test #32: storage_procedure_repository ........   Passed    0.14 sec

100% tests passed, 0 tests failed out of 18

Label Time Summary:
storage    =   1.25 sec*proc (18 tests)

Total Test time (real) =   1.39 sec
-- The C compiler identification is GNU 13.3.0
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working C compiler: /usr/bin/cc - skipped
-- Detecting C compile features
-- Detecting C compile features - done
-- Found PkgConfig: /usr/bin/pkg-config (found version "1.8.1")
-- Checking for module 'libcjson'
--   Found libcjson, version 1.7.17
-- Configuring done (0.2s)
-- Generating done (0.1s)
-- Build files have been written to: /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/build-sanitizers
[  0%] Building C object CMakeFiles/test_support.dir/support/test_assert.c.o
[  1%] Building C object CMakeFiles/test_support.dir/support/test_memory.c.o
[  1%] Building C object CMakeFiles/test_support.dir/fakes/fake_call_log.c.o
[  1%] Building C object CMakeFiles/test_support.dir/support/test_temp_dir.c.o
[  2%] Building C object CMakeFiles/test_support.dir/fakes/fake_clock.c.o
[  2%] Building C object CMakeFiles/test_support.dir/fakes/fake_http_backend.c.o
[  3%] Building C object CMakeFiles/test_support.dir/fakes/fake_wifi_backend.c.o
[  3%] Building C object CMakeFiles/test_support.dir/fakes/fake_random.c.o
[  3%] Building C object CMakeFiles/test_support.dir/fakes/fake_fs_backend.c.o
[  3%] Building C object CMakeFiles/test_support.dir/fakes/fake_freertos.c.o
[  4%] Building C object CMakeFiles/test_support.dir/fakes/fake_usb_backend.c.o
[  4%] Building C object CMakeFiles/test_support.dir/fakes/fake_gpio_backend.c.o
[  5%] Linking C static library libtest_support.a
[  5%] Built target test_support
[  5%] Building C object CMakeFiles/test_support_tests.dir/test_support.c.o
[  5%] Building C object CMakeFiles/app_operation_result_tests.dir/test_app_operation_result.c.o
[  7%] Building C object CMakeFiles/macro_parser_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[  6%] Building C object CMakeFiles/app_operation_result_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/support/app_operation_result.c.o
[  7%] Building C object CMakeFiles/macro_parser_tests.dir/test_macro_parser.c.o
[  7%] Building C object CMakeFiles/macro_parser_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[  8%] Building C object CMakeFiles/auth_tests.dir/test_auth.c.o
[  8%] Building C object CMakeFiles/macro_parser_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[  8%] Building C object CMakeFiles/auth_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/auth/auth_core_rate_limit.c.o
[  9%] Building C object CMakeFiles/auth_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/auth/auth_core_common.c.o
[ 10%] Building C object CMakeFiles/macro_executor_tests.dir/test_macro_executor.c.o
[ 10%] Building C object CMakeFiles/web_security_tests.dir/test_web_security.c.o
[ 11%] Building C object CMakeFiles/usb_keyboard_tests.dir/test_usb_keyboard.c.o
[ 12%] Building C object CMakeFiles/auth_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/auth/auth_core_session.c.o
[  8%] Building C object CMakeFiles/macro_model_tests.dir/test_macro_model.c.o
[  8%] Building C object CMakeFiles/macro_parser_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_parser.c.o
[  8%] Building C object CMakeFiles/auth_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/auth/auth_core_password.c.o
[  8%] Building C object CMakeFiles/macro_model_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 12%] Building C object CMakeFiles/macro_executor_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_executor/macro_executor_engine.c.o
[ 12%] Building C object CMakeFiles/macro_parser_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_keymap_us.c.o
[ 12%] Building C object CMakeFiles/app_core_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/app_core/app_core_sequence.c.o
[ 12%] Building C object CMakeFiles/usb_keyboard_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/usb_keyboard/usb_keyboard_state.c.o
[ 13%] Building C object CMakeFiles/app_core_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/support/app_operation_result.c.o
[ 13%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 13%] Building C object CMakeFiles/app_core_tests.dir/test_app_core.c.o
[ 13%] Building C object CMakeFiles/storage_mount_tests.dir/test_storage_mount.c.o
[ 14%] Building C object CMakeFiles/storage_mount_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 14%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/test_storage_atomic_recovery.c.o
[ 14%] Building C object CMakeFiles/web_security_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_content.c.o
[ 14%] Building C object CMakeFiles/web_security_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_static_path.c.o
[ 14%] Building C object CMakeFiles/wifi_ap_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/wifi_ap/wifi_ap_state.c.o
[ 14%] Building C object CMakeFiles/storage_mount_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_mount_topology.c.o
[ 15%] Building C object CMakeFiles/storage_atomic_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 15%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_adapter_body_auth.c.o
[ 15%] Building C object CMakeFiles/storage_atomic_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 16%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 16%] Building C object CMakeFiles/web_security_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_cookie.c.o
[ 16%] Building C object CMakeFiles/storage_atomic_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 16%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 17%] Building C object CMakeFiles/storage_atomic_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 17%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_adapter_lifecycle.c.o
[ 18%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic_validators.c.o
[ 19%] Building C object CMakeFiles/device_controls_tests.dir/test_device_controls.c.o
[ 19%] Building C object CMakeFiles/device_controls_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/device_controls/device_controls_logic.c.o
[ 20%] Building C object CMakeFiles/web_security_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_origin.c.o
[ 20%] Building C object CMakeFiles/storage_mount_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_mount_core.c.o
[ 20%] Building C object CMakeFiles/web_server_adapter_tests.dir/test_web_server_adapter.c.o
[ 21%] Building C object CMakeFiles/wifi_ap_tests.dir/test_wifi_ap.c.o
[ 21%] Building C object CMakeFiles/storage_atomic_tests.dir/test_storage_atomic.c.o
[ 21%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 22%] Building C object CMakeFiles/storage_repository_io_tests.dir/test_storage_repository_io.c.o
[ 23%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 13%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 24%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 24%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_adapter_common.c.o
[ 24%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 16%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_adapter_static_stream.c.o
[ 22%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_origin.c.o
[ 24%] Building C object CMakeFiles/provisioning_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/provisioning/provisioning_core.c.o
[ 25%] Building C object CMakeFiles/provisioning_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 25%] Building C object CMakeFiles/storage_repository_lock_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 26%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/test_storage_atomic_validators.c.o
[ 26%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 27%] Building C object CMakeFiles/storage_parent_sync_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 27%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 24%] Building C object CMakeFiles/provisioning_tests.dir/test_provisioning.c.o
[ 28%] Building C object CMakeFiles/storage_repository_lock_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 29%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 29%] Building C object CMakeFiles/storage_parent_sync_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 29%] Building C object CMakeFiles/storage_progress_repository_tests.dir/test_storage_progress.c.o
[ 30%] Building C object CMakeFiles/storage_quarantine_tests.dir/test_storage_quarantine.c.o
[ 32%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 32%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 33%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 34%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 35%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 36%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 36%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 36%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 36%] Building C object CMakeFiles/storage_object_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 36%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 37%] Building C object CMakeFiles/storage_object_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 38%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/test_storage_active_set_delete.c.o
[ 39%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 40%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 41%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 42%] Building C object CMakeFiles/storage_object_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 43%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 43%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 44%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 44%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 45%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 45%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 46%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 46%] Building C object CMakeFiles/storage_macro_repository_tests.dir/test_storage_macros.c.o
[ 47%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 47%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 48%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 49%] Building C object CMakeFiles/storage_parent_sync_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 49%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 49%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 49%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 49%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 49%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 50%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 51%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_cookie.c.o
[ 51%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 51%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 52%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_content.c.o
[ 53%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/test_storage_procedures.c.o
[ 54%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 55%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 55%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 41%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 40%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 57%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 56%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 28%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 28%] Building C object CMakeFiles/storage_parent_sync_tests.dir/test_storage_parent_sync.c.o
[ 28%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic_recovery.c.o
[ 28%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 28%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 28%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 28%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 28%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 28%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_static_path.c.o
[ 31%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 59%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 59%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 60%] Building C object CMakeFiles/web_setup_json_tests.dir/test_web_setup_json.c.o
[ 61%] Building C object CMakeFiles/web_setup_tests.dir/test_web_setup.c.o
[ 62%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 63%] Building C object CMakeFiles/provisioning_settings_tests.dir/test_provisioning_settings.c.o
[ 64%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 65%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 66%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 67%] Building C object CMakeFiles/provisioning_bootstrap_tests.dir/test_provisioning_bootstrap.c.o
[ 68%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 69%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 70%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 71%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 72%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 73%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 74%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 75%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 76%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 77%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 78%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 79%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 80%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 81%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 82%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 83%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 84%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 85%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 86%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_progress.c.o
[ 59%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 59%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 59%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 59%] Building C object CMakeFiles/provisioning_settings_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 59%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 59%] Building C object CMakeFiles/storage_transaction_tests.dir/test_storage_transactions.c.o
[ 59%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 59%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 59%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 59%] Building C object CMakeFiles/provisioning_settings_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/provisioning/provisioning_core.c.o
[ 59%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 59%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 59%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 59%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 59%] Building C object CMakeFiles/web_setup_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_setup_json.c.o
[ 59%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 59%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 59%] Building C object CMakeFiles/storage_repository_tests.dir/test_storage_repository.c.o
[ 59%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 59%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 59%] Building C object CMakeFiles/storage_object_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 59%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 59%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 59%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 59%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 59%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 59%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 59%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 59%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 59%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 59%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 59%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 59%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 59%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 59%] Building C object CMakeFiles/web_setup_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_setup_core.c.o
[ 59%] Building C object CMakeFiles/provisioning_bootstrap_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/provisioning/provisioning_bootstrap_core.c.o
[ 59%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 59%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 59%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 59%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 59%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 59%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 59%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 59%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 59%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 59%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 59%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_adapter_json.c.o
[ 59%] Building C object CMakeFiles/storage_object_json_tests.dir/test_storage_object_json.c.o
[ 59%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 59%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 59%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 59%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 59%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic_validators.c.o
[ 59%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 59%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 59%] Building C object CMakeFiles/storage_parent_sync_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 59%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 59%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 59%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 87%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 88%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 89%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 90%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 91%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 87%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_progress.c.o
[ 87%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 87%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 87%] Building C object CMakeFiles/storage_repository_lock_tests.dir/test_storage_repository_lock.c.o
[ 87%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 91%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 91%] Linking C executable app_operation_result_tests
[ 92%] Linking C executable web_security_tests
[ 92%] Linking C executable test_support_tests
[ 93%] Linking C executable storage_repository_lock_tests
[ 94%] Linking C executable macro_model_tests
[ 94%] Linking C executable provisioning_bootstrap_tests
[ 94%] Built target app_operation_result_tests
[ 94%] Linking C executable web_setup_json_tests
[ 95%] Linking C executable storage_mount_tests
[ 95%] Linking C executable storage_atomic_tests
[ 95%] Built target web_security_tests
[ 95%] Built target test_support_tests
[ 95%] Built target macro_model_tests
[ 95%] Built target storage_repository_lock_tests
[ 95%] Built target provisioning_bootstrap_tests
[ 95%] Linking C executable storage_parent_sync_tests
[ 95%] Linking C executable macro_parser_tests
[ 95%] Built target web_setup_json_tests
[ 95%] Linking C executable storage_repository_io_tests
[ 95%] Built target storage_atomic_tests
[ 95%] Built target storage_mount_tests
[ 95%] Linking C executable device_controls_tests
[ 95%] Built target macro_parser_tests
[ 95%] Built target storage_parent_sync_tests
[ 95%] Linking C executable web_setup_tests
[ 95%] Linking C executable web_server_adapter_tests
[ 95%] Built target storage_repository_io_tests
[ 96%] Linking C executable wifi_ap_tests
[ 97%] Linking C executable usb_keyboard_tests
[ 97%] Linking C executable auth_tests
[ 97%] Built target device_controls_tests
[ 97%] Built target web_setup_tests
[ 97%] Built target usb_keyboard_tests
[ 97%] Built target auth_tests
[ 97%] Built target web_server_adapter_tests
[ 98%] Linking C executable macro_executor_tests
[ 98%] Built target wifi_ap_tests
[ 98%] Linking C executable storage_transaction_tests
[ 98%] Linking C executable storage_object_json_tests
[ 98%] Linking C executable provisioning_tests
[ 99%] Linking C executable provisioning_settings_tests
[ 99%] Built target storage_transaction_tests
[ 99%] Built target storage_object_json_tests
[ 99%] Built target provisioning_tests
[ 99%] Linking C executable storage_progress_repository_tests
[ 99%] Linking C executable app_core_tests
[ 99%] Built target macro_executor_tests
[ 99%] Linking C executable storage_atomic_validators_tests
[ 99%] Linking C executable storage_atomic_recovery_tests
[ 99%] Built target storage_progress_repository_tests
[ 99%] Built target storage_atomic_validators_tests
[ 99%] Built target storage_atomic_recovery_tests
[ 99%] Built target provisioning_settings_tests
[100%] Linking C executable storage_procedure_repository_tests
[100%] Linking C executable storage_quarantine_tests
[100%] Linking C executable storage_macro_repository_tests
[100%] Built target app_core_tests
[100%] Linking C executable storage_active_set_delete_tests
[100%] Built target storage_quarantine_tests
[100%] Linking C executable storage_repository_tests
[100%] Built target storage_procedure_repository_tests
[100%] Built target storage_macro_repository_tests
[100%] Built target storage_active_set_delete_tests
[100%] Built target storage_repository_tests
Internal ctest changing into directory: /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/build-sanitizers
Test project /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/build-sanitizers
      Start 13: provisioning
 1/18 Test #13: provisioning ........................   Passed    0.47 sec
      Start 14: storage_mount
 2/18 Test #14: storage_mount .......................   Passed    0.02 sec
      Start 15: storage_atomic_recovery
 3/18 Test #15: storage_atomic_recovery .............   Passed    0.05 sec
      Start 16: storage_atomic_validators
 4/18 Test #16: storage_atomic_validators ...........   Passed    0.01 sec
      Start 17: storage_atomic
 5/18 Test #17: storage_atomic ......................   Passed    0.04 sec
      Start 18: storage_repository_lock
 6/18 Test #18: storage_repository_lock .............   Passed    0.01 sec
      Start 19: storage_parent_sync
 7/18 Test #19: storage_parent_sync .................   Passed    0.01 sec
      Start 20: storage_repository_io
 8/18 Test #20: storage_repository_io ...............   Passed    0.01 sec
      Start 21: storage_transaction
 9/18 Test #21: storage_transaction .................   Passed    0.09 sec
      Start 22: storage_quarantine
10/18 Test #22: storage_quarantine ..................   Passed    0.13 sec
      Start 23: storage_repository
11/18 Test #23: storage_repository ..................   Passed    0.90 sec
      Start 24: storage_progress_repository_tests
12/18 Test #24: storage_progress_repository_tests ...   Passed    0.14 sec
      Start 25: storage_active_set_delete_tests
13/18 Test #25: storage_active_set_delete_tests .....   Passed    0.08 sec
      Start 26: provisioning_settings
14/18 Test #26: provisioning_settings ...............   Passed    0.01 sec
      Start 27: provisioning_bootstrap
15/18 Test #27: provisioning_bootstrap ..............   Passed    0.01 sec
      Start 30: storage_object_json
16/18 Test #30: storage_object_json .................   Passed    0.01 sec
      Start 31: storage_macro_repository
17/18 Test #31: storage_macro_repository ............   Passed    0.14 sec
      Start 32: storage_procedure_repository
18/18 Test #32: storage_procedure_repository ........   Passed    0.19 sec

100% tests passed, 0 tests failed out of 18

Label Time Summary:
storage    =   2.31 sec*proc (18 tests)

Total Test time (real) =   2.32 sec
gcovr is required for native coverage generation
```

## Evidence log

```text
```
