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
[  5%] Building C object CMakeFiles/app_operation_result_tests.dir/test_app_operation_result.c.o
[  6%] Building C object CMakeFiles/app_operation_result_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/support/app_operation_result.c.o
[  6%] Building C object CMakeFiles/macro_parser_tests.dir/test_macro_parser.c.o
[  6%] Building C object CMakeFiles/macro_parser_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[  7%] Building C object CMakeFiles/macro_parser_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[  7%] Building C object CMakeFiles/test_support_tests.dir/test_support.c.o
[  7%] Building C object CMakeFiles/macro_parser_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[  7%] Building C object CMakeFiles/macro_model_tests.dir/test_macro_model.c.o
[  8%] Building C object CMakeFiles/macro_executor_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_executor/macro_executor_engine.c.o
[  8%] Building C object CMakeFiles/auth_tests.dir/test_auth.c.o
[  8%] Building C object CMakeFiles/macro_executor_tests.dir/test_macro_executor.c.o
[  9%] Building C object CMakeFiles/auth_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/auth/auth_core_rate_limit.c.o
[  9%] Building C object CMakeFiles/macro_model_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[  9%] Building C object CMakeFiles/web_security_tests.dir/test_web_security.c.o
[  9%] Building C object CMakeFiles/auth_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/auth/auth_core_common.c.o
[  9%] Building C object CMakeFiles/web_security_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_cookie.c.o
[ 10%] Building C object CMakeFiles/web_security_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_origin.c.o
[ 10%] Building C object CMakeFiles/macro_parser_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_parser.c.o
[ 10%] Building C object CMakeFiles/web_security_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_static_path.c.o
[ 10%] Building C object CMakeFiles/auth_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/auth/auth_core_password.c.o
[ 11%] Building C object CMakeFiles/auth_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/auth/auth_core_session.c.o
[ 12%] Building C object CMakeFiles/macro_parser_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_keymap_us.c.o
[ 12%] Building C object CMakeFiles/web_security_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_content.c.o
[ 12%] Building C object CMakeFiles/web_server_adapter_tests.dir/test_web_server_adapter.c.o
[ 13%] Building C object CMakeFiles/wifi_ap_tests.dir/test_wifi_ap.c.o
[ 13%] Building C object CMakeFiles/app_core_tests.dir/test_app_core.c.o
[ 14%] Building C object CMakeFiles/app_core_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/support/app_operation_result.c.o
[ 14%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_adapter_body_auth.c.o
[ 13%] Building C object CMakeFiles/app_core_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/app_core/app_core_sequence.c.o
[ 14%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 15%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_adapter_common.c.o
[ 15%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_adapter_static_stream.c.o
[ 15%] Building C object CMakeFiles/wifi_ap_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/wifi_ap/wifi_ap_state.c.o
[ 15%] Building C object CMakeFiles/storage_atomic_tests.dir/test_storage_atomic.c.o
[ 16%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_cookie.c.o
[ 16%] Building C object CMakeFiles/storage_repository_lock_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 17%] Building C object CMakeFiles/provisioning_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 17%] Building C object CMakeFiles/usb_keyboard_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/usb_keyboard/usb_keyboard_state.c.o
[ 18%] Building C object CMakeFiles/storage_repository_lock_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 19%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_adapter_json.c.o
[ 20%] Building C object CMakeFiles/storage_repository_io_tests.dir/test_storage_repository_io.c.o
[ 21%] Building C object CMakeFiles/device_controls_tests.dir/test_device_controls.c.o
[ 21%] Building C object CMakeFiles/storage_transaction_tests.dir/test_storage_transactions.c.o
[ 21%] Building C object CMakeFiles/storage_atomic_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 21%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 22%] Building C object CMakeFiles/storage_mount_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 23%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/test_storage_atomic_validators.c.o
[ 23%] Building C object CMakeFiles/storage_mount_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_mount_core.c.o
[ 23%] Building C object CMakeFiles/provisioning_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/provisioning/provisioning_core.c.o
[ 23%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 15%] Building C object CMakeFiles/provisioning_tests.dir/test_provisioning.c.o
[ 21%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 21%] Building C object CMakeFiles/storage_atomic_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 23%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 25%] Building C object CMakeFiles/storage_progress_repository_tests.dir/test_storage_progress.c.o
[ 26%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 26%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 27%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 27%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 28%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic_validators.c.o
[ 28%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 28%] Building C object CMakeFiles/storage_macro_repository_tests.dir/test_storage_macros.c.o
[ 29%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 29%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 29%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 29%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 29%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 30%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 31%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 31%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 31%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 32%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 32%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 32%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 33%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 34%] Building C object CMakeFiles/usb_keyboard_tests.dir/test_usb_keyboard.c.o
[ 34%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 34%] Building C object CMakeFiles/storage_object_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 34%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 35%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 35%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 36%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 36%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 37%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 37%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 38%] Building C object CMakeFiles/storage_object_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 38%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 38%] Building C object CMakeFiles/storage_object_json_tests.dir/test_storage_object_json.c.o
[ 39%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 40%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 40%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 41%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 42%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/test_storage_procedures.c.o
[ 42%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 42%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 42%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 43%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_content.c.o
[ 43%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 43%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 43%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 44%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 45%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 45%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 45%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 45%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 45%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 46%] Building C object CMakeFiles/web_setup_tests.dir/test_web_setup.c.o
[ 47%] Building C object CMakeFiles/storage_object_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 47%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 47%] Building C object CMakeFiles/storage_object_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 47%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 48%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 49%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 50%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 34%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 50%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 51%] Building C object CMakeFiles/storage_quarantine_tests.dir/test_storage_quarantine.c.o
[ 52%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 53%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 54%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 55%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 55%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 56%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 56%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 57%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 58%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 58%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 58%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 58%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 58%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 59%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 59%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 59%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_progress.c.o
[ 59%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 58%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 59%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 60%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 60%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 61%] Building C object CMakeFiles/web_setup_json_tests.dir/test_web_setup_json.c.o
[ 62%] Building C object CMakeFiles/storage_parent_sync_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 25%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_origin.c.o
[ 25%] Building C object CMakeFiles/provisioning_settings_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 50%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 25%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 25%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 25%] Building C object CMakeFiles/storage_atomic_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 25%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 25%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_adapter_lifecycle.c.o
[ 25%] Building C object CMakeFiles/storage_mount_tests.dir/test_storage_mount.c.o
[ 25%] Building C object CMakeFiles/device_controls_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/device_controls/device_controls_logic.c.o
[ 25%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_static_path.c.o
[ 25%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 25%] Building C object CMakeFiles/storage_mount_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_mount_topology.c.o
[ 25%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 25%] Building C object CMakeFiles/storage_repository_tests.dir/test_storage_repository.c.o
[ 25%] Building C object CMakeFiles/storage_atomic_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 25%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/test_storage_atomic_recovery.c.o
[ 25%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 25%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 25%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 25%] Building C object CMakeFiles/provisioning_settings_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/provisioning/provisioning_core.c.o
[ 63%] Building C object CMakeFiles/storage_parent_sync_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 64%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 65%] Building C object CMakeFiles/provisioning_bootstrap_tests.dir/test_provisioning_bootstrap.c.o
[ 66%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 67%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 68%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 69%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 70%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 71%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 72%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 73%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 74%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/test_storage_active_set_delete.c.o
[ 75%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 76%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 77%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 78%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 79%] Building C object CMakeFiles/storage_parent_sync_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 80%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 81%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 82%] Building C object CMakeFiles/provisioning_settings_tests.dir/test_provisioning_settings.c.o
[ 83%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 84%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 85%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_progress.c.o
[ 85%] Linking C executable app_operation_result_tests
[ 86%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 87%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 88%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 89%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 63%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 63%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 63%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 63%] Building C object CMakeFiles/provisioning_bootstrap_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/provisioning/provisioning_bootstrap_core.c.o
[ 63%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 63%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 63%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic_recovery.c.o
[ 63%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic_validators.c.o
[ 63%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 63%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 63%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 63%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 63%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 63%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 63%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 63%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 63%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 63%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 63%] Building C object CMakeFiles/storage_parent_sync_tests.dir/test_storage_parent_sync.c.o
[ 63%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 63%] Building C object CMakeFiles/storage_repository_lock_tests.dir/test_storage_repository_lock.c.o
[ 63%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 63%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 63%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 63%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 63%] Building C object CMakeFiles/web_setup_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_setup_core.c.o
[ 63%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 63%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 63%] Building C object CMakeFiles/web_setup_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_setup_json.c.o
[ 63%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 63%] Building C object CMakeFiles/storage_parent_sync_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 63%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 63%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 63%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 90%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 91%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 90%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 91%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 92%] Linking C executable macro_model_tests
[ 92%] Linking C executable test_support_tests
[ 93%] Linking C executable web_security_tests
[ 93%] Built target app_operation_result_tests
[ 93%] Linking C executable provisioning_bootstrap_tests
[ 93%] Linking C executable macro_parser_tests
[ 93%] Built target macro_model_tests
[ 94%] Linking C executable storage_repository_lock_tests
[ 94%] Built target test_support_tests
[ 94%] Linking C executable storage_repository_io_tests
[ 94%] Built target provisioning_bootstrap_tests
[ 94%] Linking C executable web_server_adapter_tests
[ 94%] Built target web_security_tests
[ 95%] Linking C executable usb_keyboard_tests
[ 95%] Linking C executable auth_tests
[ 96%] Linking C executable storage_mount_tests
[ 95%] Built target macro_parser_tests
[ 96%] Linking C executable provisioning_tests
[ 96%] Built target storage_repository_lock_tests
[ 96%] Linking C executable storage_parent_sync_tests
[ 96%] Linking C executable web_setup_json_tests
[ 96%] Linking C executable web_setup_tests
[ 97%] Linking C executable wifi_ap_tests
[ 97%] Linking C executable storage_atomic_tests
[ 97%] Built target storage_repository_io_tests
[ 97%] Built target web_server_adapter_tests
[ 97%] Built target storage_mount_tests
[ 98%] Linking C executable macro_executor_tests
[ 98%] Built target usb_keyboard_tests
[ 98%] Linking C executable storage_object_json_tests
[ 98%] Linking C executable device_controls_tests
[ 98%] Built target provisioning_tests
[ 98%] Linking C executable storage_atomic_recovery_tests
[ 98%] Built target auth_tests
[ 99%] Linking C executable provisioning_settings_tests
[ 99%] Built target wifi_ap_tests
[ 99%] Built target web_setup_tests
[ 99%] Built target web_setup_json_tests
[ 99%] Linking C executable storage_macro_repository_tests
[ 99%] Built target storage_parent_sync_tests
[ 99%] Built target storage_atomic_tests
[ 99%] Built target macro_executor_tests
[ 99%] Linking C executable storage_transaction_tests
[ 99%] Built target device_controls_tests
[ 99%] Linking C executable storage_atomic_validators_tests
[ 99%] Built target storage_object_json_tests
[ 99%] Built target provisioning_settings_tests
[ 99%] Linking C executable storage_progress_repository_tests
[ 99%] Built target storage_macro_repository_tests
[ 99%] Linking C executable storage_active_set_delete_tests
[ 99%] Linking C executable storage_quarantine_tests
[ 99%] Built target storage_atomic_recovery_tests
[100%] Linking C executable storage_procedure_repository_tests
[100%] Built target storage_transaction_tests
[100%] Linking C executable app_core_tests
[100%] Built target storage_quarantine_tests
[100%] Built target storage_procedure_repository_tests
[100%] Built target storage_atomic_validators_tests
[100%] Built target storage_active_set_delete_tests
[100%] Built target storage_progress_repository_tests
[100%] Built target app_core_tests
[100%] Linking C executable storage_repository_tests
[100%] Built target storage_repository_tests
Internal ctest changing into directory: /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/build-coverage
Test project /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/build-coverage
      Start  1: test_support
 1/32 Test  #1: test_support ........................   Passed    0.15 sec
      Start  2: app_operation_result
 2/32 Test  #2: app_operation_result ................   Passed    0.00 sec
      Start  3: macro_parser
 3/32 Test  #3: macro_parser ........................   Passed    0.01 sec
      Start  4: macro_model
 4/32 Test  #4: macro_model .........................   Passed    0.00 sec
      Start  5: macro_executor
 5/32 Test  #5: macro_executor ......................   Passed    0.00 sec
      Start  6: auth
 6/32 Test  #6: auth ................................   Passed    0.00 sec
      Start  7: web_security
 7/32 Test  #7: web_security ........................   Passed    0.00 sec
      Start  8: web_server_adapter
 8/32 Test  #8: web_server_adapter ..................   Passed    0.00 sec
      Start  9: app_core
 9/32 Test  #9: app_core ............................   Passed    0.00 sec
      Start 10: usb_keyboard
10/32 Test #10: usb_keyboard ........................   Passed    0.00 sec
      Start 11: device_controls
11/32 Test #11: device_controls .....................   Passed    0.00 sec
      Start 12: wifi_ap
12/32 Test #12: wifi_ap .............................   Passed    0.00 sec
      Start 13: provisioning
13/32 Test #13: provisioning ........................   Passed    0.00 sec
      Start 14: storage_mount
14/32 Test #14: storage_mount .......................   Passed    0.00 sec
      Start 15: storage_atomic_recovery
15/32 Test #15: storage_atomic_recovery .............   Passed    0.03 sec
      Start 16: storage_atomic_validators
16/32 Test #16: storage_atomic_validators ...........   Passed    0.01 sec
      Start 17: storage_atomic
17/32 Test #17: storage_atomic ......................   Passed    0.04 sec
      Start 18: storage_repository_lock
18/32 Test #18: storage_repository_lock .............   Passed    0.00 sec
      Start 19: storage_parent_sync
19/32 Test #19: storage_parent_sync .................   Passed    0.01 sec
      Start 20: storage_repository_io
20/32 Test #20: storage_repository_io ...............   Passed    0.00 sec
      Start 21: storage_transaction
21/32 Test #21: storage_transaction .................   Passed    0.07 sec
      Start 22: storage_quarantine
22/32 Test #22: storage_quarantine ..................   Passed    0.11 sec
      Start 23: storage_repository
23/32 Test #23: storage_repository ..................   Passed    0.51 sec
      Start 24: storage_progress_repository_tests
24/32 Test #24: storage_progress_repository_tests ...   Passed    0.07 sec
      Start 25: storage_active_set_delete_tests
25/32 Test #25: storage_active_set_delete_tests .....   Passed    0.05 sec
      Start 26: provisioning_settings
26/32 Test #26: provisioning_settings ...............   Passed    0.00 sec
      Start 27: provisioning_bootstrap
27/32 Test #27: provisioning_bootstrap ..............   Passed    0.00 sec
      Start 28: web_setup
28/32 Test #28: web_setup ...........................   Passed    0.00 sec
      Start 29: web_setup_json
29/32 Test #29: web_setup_json ......................   Passed    0.00 sec
      Start 30: storage_object_json
30/32 Test #30: storage_object_json .................   Passed    0.00 sec
      Start 31: storage_macro_repository
31/32 Test #31: storage_macro_repository ............   Passed    0.08 sec
      Start 32: storage_procedure_repository
32/32 Test #32: storage_procedure_repository ........   Passed    0.12 sec

100% tests passed, 0 tests failed out of 32

Label Time Summary:
auth        =   0.00 sec*proc (1 test)
controls    =   0.00 sec*proc (1 test)
executor    =   0.00 sec*proc (1 test)
model       =   0.00 sec*proc (1 test)
parser      =   0.01 sec*proc (1 test)
startup     =   0.00 sec*proc (1 test)
storage     =   1.10 sec*proc (18 tests)
support     =   0.15 sec*proc (2 tests)
usb         =   0.00 sec*proc (1 test)
web         =   0.00 sec*proc (4 tests)
wifi        =   0.00 sec*proc (1 test)

Total Test time (real) =   1.29 sec
(INFO) Reading coverage data...

(INFO) Writing coverage report...

lines: 86.7% (5871 out of 6769)
functions: 98.2% (532 out of 542)
branches: 69.7% (4083 out of 5857)
(INFO) Reading coverage data...

(INFO) Writing coverage report...

(INFO) Reading coverage data...

(INFO) Writing coverage report...

lines: 95.9% (1548 out of 1615)
functions: 100.0% (134 out of 134)
branches: 83.2% (1171 out of 1408)
------------------------------------------------------------------------------
                           GCC Code Coverage Report
Directory: .
------------------------------------------------------------------------------
File                                       Lines     Exec  Cover   Missing
------------------------------------------------------------------------------
firmware/components/app_core/app_core_sequence.c
                                             214      214   100%
firmware/components/auth/auth_core_common.c
                                              90       80    88%   51,67,72-74,90,107,116-118
firmware/components/auth/auth_core_password.c
                                              31       31   100%
firmware/components/auth/auth_core_rate_limit.c
                                              63       54    85%   21,46-48,55,59,68,83,87
firmware/components/auth/auth_core_session.c
                                              83       78    94%   40,47,67,91,113
firmware/components/device_controls/device_controls_logic.c
                                             279      260    93%   125,133,140,144,152,156,169,273-275,294,324,396,410,414,438,444-446
firmware/components/macro_executor/macro_executor_engine.c
                                             188      171    91%   43,47,52,60,144,186,198,214-215,226,246,258,260-261,277,284-285
firmware/components/macro_model/app_error.c
                                              40        4    10%   5-36,39-42
firmware/components/macro_model/app_uuid.c
                                              65       46    70%   43,58,63-64,71,80,86-87,89-91,93,96-98,100,105,119-120
firmware/components/macro_model/macro_model.c
                                              25       25   100%
firmware/components/macro_parser/macro_keymap_us.c
                                             143      139    97%   176-177,207,223
firmware/components/macro_parser/macro_parser.c
                                             220      209    95%   56,64,74,81,110,271,352,378-380,392
firmware/components/provisioning/provisioning_bootstrap_core.c
                                              41       41   100%
firmware/components/provisioning/provisioning_core.c
                                             307      296    96%   69,137,151,156,208,210,290,324-325,370,384
firmware/components/storage/storage_atomic.c
                                             188      151    80%   35,40,51-52,66,72-73,82-83,85,89-90,99-100,134,144,149-151,153,157-159,173-174,194-195,204,214-215,228,236,263-264,318-319,333
firmware/components/storage/storage_atomic_recovery.c
                                             263      200    76%   60,101,112,180-181,186,197-199,207-208,219-220,224-225,231,242-243,251-252,261-264,266-268,272,304,327,333-335,351,360,371,383,387-388,390,393-395,397-398,407-408,416-417,426-427,470,476,490,498,500,503-506,509-511
firmware/components/storage/storage_atomic_validators.c
                                             219      159    72%   40,52,70,80,93,120,123,155,174-177,187,207,228,230,238,242,245-247,253,255-258,260-261,263-266,268-271,273-274,277,279-282,284-285,287-290,292-295,297,306,326-329,343
firmware/components/storage/storage_fs_ops.c
                                              97       77    79%   40-41,48,56-57,82,89-90,94-97,100,140-141,149,156-157,166-167
firmware/components/storage/storage_json.c
                                             129      109    84%   20,33,55,84-85,107,112,116,132,136,140,144,155,170,177,185,190,202,207-208
firmware/components/storage/storage_mount_core.c
                                              40       36    90%   18,32-33,45
firmware/components/storage/storage_mount_topology.c
                                              15       13    86%   23,28
firmware/components/storage/storage_paths.c
                                              58       45    77%   11,15-16,25,30-31,44-45,53,63,75,86,98
firmware/components/storage/storage_quarantine.c
                                             554      461    83%   43,64,73,83,95,107,118,138,146-147,152,156-157,176,182,190,195,199,212,251-252,268,274,291,297-298,306-307,309,336,341,398-399,409-410,449-450,470-471,497-498,513-514,534,544,569-570,573,582,607,614,639,641-644,646,648-650,686-687,696-697,717,782-783,785,807,843-844,855-857,859-860,862,883,890,897,929-930,944-945,952,955,991-994,996-998
firmware/components/storage/storage_repository_index.c
                                             165      132    80%   30-31,35-36,42-44,66,89-91,96-98,104,122,127,138,141,157,160,169,185-186,188,195,204,210,219,238,243,257,265
firmware/components/storage/storage_repository_io.c
                                             209      179    85%   28,33-34,53,69,86,92-93,101,103,133,179,183,187,203,214,225,231,236-237,247,277,281,283,302,314,355,359,377-378
firmware/components/storage/storage_repository_json.c
                                             100       87    87%   19,23,46,56,64,92,104,129,146-147,153,157-158
firmware/components/storage/storage_repository_lock.c
                                              39       39   100%
firmware/components/storage/storage_repository_macros.c
                                             367      307    83%   50,63,77,82,101,113-115,153,157,163,167-168,170-171,206,214,218,221,225,229,233,249,276,301,326-327,329,338,363,367,373,377,394-398,402,404,418,427,441,446,464,468-469,478-479,487,501,513,525,534,547,558,570,583,597,609
firmware/components/storage/storage_repository_objects_json.c
                                             466      397    85%   54,58,63,77,81,93,104,116,169,173,190,194-195,211,215,235,244,254,271,278,286,316,320,324,331,351,359,364,372,375,387,431-432,455-456,483-485,493,506,510,516,519-520,535,539,542,544,556,559,561,574,616-617,622-623,648-649,671,683,687,693-694,698-699,724,728,739-740
firmware/components/storage/storage_repository_order.c
                                              64       51    79%   18,25,30,32-33,41,60,77,80,83,93,97,113
firmware/components/storage/storage_repository_procedures.c
                                             321      284    88%   36,41,107,156,173-175,199,251,255,261,266-267,269-270,312,320,324,327,331,337,341,358,390,395,409,418,433,447,459,471,480,492,503,514,526,538
firmware/components/storage/storage_repository_progress.c
                                             145      129    89%   48,52-53,62,77,92-93,121,124,135-136,163,205,226,238,250
firmware/components/storage/storage_repository_sets.c
                                             270      219    81%   46-47,89,99,135,171-172,185,189,197,201,209,214-215,220,228,232,242,254,258,261,280,285-288,291,307,311-312,315,320,326-327,329,334,348,363,397,407,413,418,424,439,443,446,451,460-461,463,468
firmware/components/storage/storage_transaction.c
                                             355      300    84%   28,31,57,72,83,88,111-112,136,161,188,194-195,220-221,223,234,248,252,264,293,317,320,341,348,368,382,385,389-390,408,411,415,427,433,441,447,454,461,475,502-503,509-510,533,537-538,546-548,566-567,570,621,632
firmware/components/support/app_operation_result.c
                                              10       10   100%
firmware/components/support/include/app_operation_result.h
                                               5        5   100%
firmware/components/usb_keyboard/usb_keyboard_state.c
                                              84       78    92%   50-51,53,60,98,107
firmware/components/web_server/web_content.c
                                              72       68    94%   20,28,57-58
firmware/components/web_server/web_cookie.c
                                              39       38    97%   47
firmware/components/web_server/web_origin.c
                                              14       14   100%
firmware/components/web_server/web_server_adapter_body_auth.c
                                              41       38    92%   52,68-69
firmware/components/web_server/web_server_adapter_common.c
                                              54       30    55%   25-26,28,38-39,41,48-56,60-65,68-70
firmware/components/web_server/web_server_adapter_json.c
                                              52       45    86%   16,40,46,64,74,87-88
firmware/components/web_server/web_server_adapter_lifecycle.c
                                              35       31    88%   47,50,53-54
firmware/components/web_server/web_server_adapter_static_stream.c
                                              49       45    91%   21,31-32,62
firmware/components/web_server/web_setup_core.c
                                             154      149    96%   34,61,81,177,227
firmware/components/web_server/web_setup_json.c
                                             120      111    92%   28,43,55,61,84,92,144,182-183
firmware/components/web_server/web_static_path.c
                                              24       24   100%
firmware/components/wifi_ap/wifi_ap_state.c
                                             163      162    99%   49
------------------------------------------------------------------------------
TOTAL                                       6769     5871    86%
------------------------------------------------------------------------------
------------------------------------------------------------------------------
                           GCC Code Coverage Report
Directory: .
------------------------------------------------------------------------------
File                                    Branches    Taken  Cover   Missing
------------------------------------------------------------------------------
firmware/components/app_core/app_core_sequence.c
                                             135      135   100%
firmware/components/auth/auth_core_common.c
                                              70       52    74%   15,16,17,29,33,50,65,66,71,87,97,102,106,109,112
firmware/components/auth/auth_core_password.c
                                              36       36   100%
firmware/components/auth/auth_core_rate_limit.c
                                              36       26    72%   20,45,54,58,65,66,67,82,86
firmware/components/auth/auth_core_session.c
                                              64       52    81%   25,27,39,42,61,62,66,73,90,112,119
firmware/components/device_controls/device_controls_logic.c
                                             216      178    82%   75,121,124,132,139,143,151,155,168,171,199,225,254,262,272,293,323,331,332,336,395,399,409,410,413,414,424,437,443,446
firmware/components/macro_executor/macro_executor_engine.c
                                             158      126    79%   17,18,19,20,42,46,51,59,90,118,143,165,172,176,185,193,197,213,225,245,257,261,271,276,283
firmware/components/macro_model/app_error.c
                                              19        1     5%   4
firmware/components/macro_model/app_uuid.c
                                              64       42    65%   28,35,42,53,57,62,70,79,85,86,90,95,97,104,118,126
firmware/components/macro_model/macro_model.c
                                              18       18   100%
firmware/components/macro_parser/macro_keymap_us.c
                                              72       67    93%   76,206,222
firmware/components/macro_parser/macro_parser.c
                                             156      131    84%   21,48,55,63,73,80,85,98,99,109,115,156,168,189,256,270,293,318,351,377,383,391
firmware/components/provisioning/provisioning_bootstrap_core.c
                                              26       25    96%   36
firmware/components/provisioning/provisioning_core.c
                                             246      193    78%   68,91,92,118,136,145,146,150,154,177,178,205,207,289,306,309,323,330,369,380,383,394,397,398,399,414,428,429,432,442,449,470,487
firmware/components/storage/storage_atomic.c
                                             150       98    65%   34,39,50,65,71,81,84,88,96,100,132,133,141,148,150,156,159,172,177,193,203,206,213,227,235,238,258,262,271,289,298,312,319,332,343,344
firmware/components/storage/storage_atomic_recovery.c
                                             207      133    64%   33,59,100,111,181,185,196,198,199,205,218,223,230,241,243,249,257,262,267,271,280,283,286,289,303,326,332,334,350,359,370,376,380,387,394,398,406,412,421,459,469,475,482,489,505,511
firmware/components/storage/storage_atomic_validators.c
                                             158       87    55%   39,46,47,51,64,69,74,75,79,87,92,96,119,122,126,154,161,168,186,193,205,214,227,228,237,241,244,245,246,257,265,269,270,281,289,293,294,304,314,342
firmware/components/storage/storage_fs_ops.c
                                              68       40    58%   36,39,47,49,55,81,88,93,99,139,148,155,165,199,200,201,205,206,207
firmware/components/storage/storage_json.c
                                             148       89    60%   19,32,37,50,53,54,83,106,111,115,125,128,131,135,139,143,154,159,161,169,174,175,176,184,189,198,201,206,214
firmware/components/storage/storage_mount_core.c
                                              40       24    60%   10,11,12,17,21,29,31,32,44,70
firmware/components/storage/storage_mount_topology.c
                                              16       10    62%   22,23,27,28
firmware/components/storage/storage_paths.c
                                              58       29    50%   9,10,14,23,24,29,38,43,52,62,73,74,85,96,97
firmware/components/storage/storage_quarantine.c
                                             518      337    65%   42,58,59,63,72,78,82,89,94,99,106,114,117,126,131,134,137,142,143,144,151,155,173,175,179,181,189,194,198,207,210,211,226,227,228,230,231,232,233,234,248,249,267,273,290,296,305,308,312,335,340,353,381,387,397,404,408,431,447,452,468,496,498,512,530,533,540,543,546,558,561,562,563,568,572,580,581,598,602,606,613,642,643,650,660,661,665,682,683,693,695,713,719,730,733,741,766,781,782,790,806,810,814,823,825,837,842,854,856,859,877,879,881,889,895,921,927,943,950,954,984,993,998
firmware/components/storage/storage_repository_index.c
                                             136       78    57%   24,25,28,29,34,40,65,74,86,87,95,103,110,121,126,133,137,140,144,147,156,158,168,175,178,179,180,182,188,194,203,209,218,223,237,242,247,256,264
firmware/components/storage/storage_repository_io.c
                                             168      115    68%   27,45,48,51,52,61,62,68,85,91,100,102,106,129,132,153,158,177,182,186,202,207,212,213,219,224,230,233,246,254,276,280,283,300,313,354,358
firmware/components/storage/storage_repository_json.c
                                             110       69    62%   18,22,32,33,34,43,44,45,55,61,63,91,103,109,125,126,127,128,135,136,137,138,139,140,141,142,143,144,152,156
firmware/components/storage/storage_repository_lock.c
                                              28       23    82%   82,113,120,127,134
firmware/components/storage/storage_repository_macros.c
                                             302      178    58%   32,35,36,40,41,44,49,55,62,76,81,88,93,94,100,111,112,121,132,141,152,156,162,166,167,184,187,196,205,213,217,220,224,228,232,241,248,251,260,267,272,294,300,315,319,323,329,333,334,346,351,362,366,367,372,376,386,396,410,417,426,429,434,437,440,445,455,456,457,463,467,477,484,494,500,504,512,524,529,533,546,551,557,562,569,575,582,588,596,602,608,613
firmware/components/storage/storage_repository_objects_json.c
                                             534      335    62%   53,57,62,71,72,73,74,75,76,80,84,86,92,103,112,115,155,158,165,166,167,171,175,176,177,179,183,189,193,204,207,210,214,218,231,232,234,243,247,253,258,263,270,275,285,289,293,296,315,319,323,330,334,338,339,340,341,347,348,349,356,357,358,362,363,367,368,369,370,371,383,386,420,423,430,442,445,446,447,451,454,463,466,469,474,476,477,478,479,480,481,488,491,495,505,509,514,518,519,529,530,531,532,533,534,538,541,542,555,558,559,570,573,605,607,615,621,631,634,637,641,642,643,644,646,653,657,667,670,682,686,691,697,716,719,722,723,727,737,743
firmware/components/storage/storage_repository_order.c
                                              60       36    60%   17,24,29,33,40,47,59,75,76,79,82,92,96,108,112
firmware/components/storage/storage_repository_procedures.c
                                             242      158    65%   28,35,40,48,68,77,90,103,106,147,153,154,155,163,171,172,179,198,203,208,229,241,247,250,254,260,265,266,284,287,296,311,319,323,326,330,336,340,350,357,360,366,378,389,392,401,402,403,408,417,422,425,429,432,440,441,446,450,458,470,475,479,491,496,502,507,513,519,525,531,537,542
firmware/components/storage/storage_repository_progress.c
                                             112       75    67%   21,27,28,47,51,52,61,70,76,83,91,107,120,123,134,149,152,155,162,179,182,186,195,198,204,225,230,237,242,249,255
firmware/components/storage/storage_repository_sets.c
                                             198      127    64%   45,83,88,93,98,103,125,129,134,153,170,184,188,196,200,208,213,219,227,231,241,253,257,260,279,284,285,287,305,306,310,312,314,319,325,326,333,347,350,362,367,371,396,400,406,412,417,423,437,438,442,445,450,459,460,467
firmware/components/storage/storage_transaction.c
                                             327      206    63%   27,30,50,56,62,63,68,71,78,82,87,95,96,97,101,104,106,117,122,129,135,156,159,160,187,193,199,219,220,230,233,242,247,251,256,263,279,289,292,316,319,338,339,340,347,364,366,378,381,384,387,404,407,410,414,426,432,439,440,444,446,450,453,457,459,471,474,481,498,501,507,508,514,529,532,536,544,557,558,564,569,612,620,631,638
firmware/components/support/app_operation_result.c
                                              12       12   100%
firmware/components/support/include/app_operation_result.h
                                               6        6   100%
firmware/components/usb_keyboard/usb_keyboard_state.c
                                              80       72    90%   48,50,59,78,97,101,106
firmware/components/web_server/web_content.c
                                              90       74    82%   11,15,19,27,35,43,50,53,57,70
firmware/components/web_server/web_cookie.c
                                              54       45    83%   12,16,17,38,39,45,46,62
firmware/components/web_server/web_origin.c
                                              30       27    90%   9,14,28
firmware/components/web_server/web_server_adapter_body_auth.c
                                              50       33    66%   13,16,33,47,50,58,61,67
firmware/components/web_server/web_server_adapter_common.c
                                              44       24    54%   12,24,25,37,38,44,67,80,84
firmware/components/web_server/web_server_adapter_json.c
                                              48       25    52%   12,15,35,38,39,45,63,70,73,86
firmware/components/web_server/web_server_adapter_lifecycle.c
                                              32       23    71%   11,19,24,46,49,52
firmware/components/web_server/web_server_adapter_static_stream.c
                                              62       45    72%   17,20,30,39,60,61,70,79
firmware/components/web_server/web_setup_core.c
                                             140      103    73%   16,17,18,19,33,52,60,68,69,71,74,80,88,89,91,94,97,126,132,143,172,176,226
firmware/components/web_server/web_setup_json.c
                                             108       84    77%   23,27,42,54,60,83,91,112,114,116,117,132,143,162,163,178,181
firmware/components/web_server/web_static_path.c
                                              50       46    92%   6,12,33
firmware/components/wifi_ap/wifi_ap_state.c
                                             155      135    87%   34,48,71,136,140,141,201,205,206,216,222,227,233,244,250
------------------------------------------------------------------------------
TOTAL                                       5857     4083    69%
------------------------------------------------------------------------------
------------------------------------------------------------------------------
                           GCC Code Coverage Report
Directory: .
------------------------------------------------------------------------------
File                                       Lines     Exec  Cover   Missing
------------------------------------------------------------------------------
firmware/components/app_core/app_core_sequence.c
                                             214      214   100%
firmware/components/device_controls/device_controls_logic.c
                                             279      260    93%   125,133,140,144,152,156,169,273-275,294,324,396,410,414,438,444-446
firmware/components/macro_executor/macro_executor_engine.c
                                             188      171    91%   43,47,52,60,144,186,198,214-215,226,246,258,260-261,277,284-285
firmware/components/provisioning/provisioning_bootstrap_core.c
                                              41       41   100%
firmware/components/provisioning/provisioning_core.c
                                             307      296    96%   69,137,151,156,208,210,290,324-325,370,384
firmware/components/web_server/web_content.c
                                              72       68    94%   20,28,57-58
firmware/components/web_server/web_cookie.c
                                              39       38    97%   47
firmware/components/web_server/web_origin.c
                                              14       14   100%
firmware/components/web_server/web_setup_core.c
                                             154      149    96%   34,61,81,177,227
firmware/components/web_server/web_setup_json.c
                                             120      111    92%   28,43,55,61,84,92,144,182-183
firmware/components/web_server/web_static_path.c
                                              24       24   100%
firmware/components/wifi_ap/wifi_ap_state.c
                                             163      162    99%   49
------------------------------------------------------------------------------
TOTAL                                       1615     1548    95%
------------------------------------------------------------------------------
Toolchain verified: ESP-IDF v5.5.5, target esp32s3, Node.js v24.18.0
firmware/CMakeLists.txt
=======================

firmware/components/app_core/CMakeLists.txt
===========================================

firmware/components/auth/CMakeLists.txt
=======================================

firmware/components/device_controls/CMakeLists.txt
==================================================

firmware/components/macro_executor/CMakeLists.txt
=================================================

firmware/components/macro_model/CMakeLists.txt
==============================================

firmware/components/macro_parser/CMakeLists.txt
===============================================

firmware/components/provisioning/CMakeLists.txt
===============================================

firmware/components/storage/CMakeLists.txt
==========================================

firmware/components/support/CMakeLists.txt
==========================================

firmware/components/usb_keyboard/CMakeLists.txt
===============================================

firmware/components/web_server/CMakeLists.txt
=============================================

firmware/components/wifi_ap/CMakeLists.txt
==========================================

firmware/main/CMakeLists.txt
============================

firmware/test_app/CMakeLists.txt
================================

firmware/test_app/main/CMakeLists.txt
=====================================

tests/host/CMakeLists.txt
=========================
tests/host/CMakeLists.txt:504,00: [C0111] Missing docstring on function or macro declaration

Summary
=======
files scanned: 17
found lint:
  Convention: 1

```

## Evidence log

```text
```
