# Phase 16 cleanup failure

- Transform: success
- Format: success
- Focused tests: success
- Authoritative gate: failure

## phase16-transform.log

```text

Phase 16 clang-tidy cleanup applied
```

## phase16-format.log

```text

```

## phase16-focused.log

```text

[ 63%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 64%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 65%] Building C object CMakeFiles/storage_parent_sync_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 65%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 65%] Building C object CMakeFiles/storage_object_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 66%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 66%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 66%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 66%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 67%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 68%] Building C object CMakeFiles/storage_object_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 68%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 69%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 69%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 69%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 70%] Building C object CMakeFiles/storage_repository_lock_tests.dir/test_storage_repository_lock.c.o
[ 71%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 71%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 71%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 61%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 71%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 72%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 73%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 74%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 74%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 74%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 75%] Building C object CMakeFiles/provisioning_settings_tests.dir/test_provisioning_settings.c.o
[ 75%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 76%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/test_storage_atomic_validators.c.o
[ 76%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 77%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 78%] Building C object CMakeFiles/storage_transaction_tests.dir/test_storage_transactions.c.o
[ 78%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 78%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_progress.c.o
[ 79%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 79%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 80%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 82%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 83%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 84%] Building C object CMakeFiles/storage_mount_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_mount_core.c.o
[ 84%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 85%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 85%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 85%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 81%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 86%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 87%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 88%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 55%] Building C object CMakeFiles/storage_mount_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_mount_topology.c.o
[ 55%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 55%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 55%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic_recovery.c.o
[ 55%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic_validators.c.o
[ 55%] Building C object CMakeFiles/web_execution_submit_tests.dir/test_web_execution_submit.c.o
[ 55%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 55%] Building C object CMakeFiles/web_api_core_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 55%] Building C object CMakeFiles/web_api_core_tests.dir/test_web_api_core.c.o
[ 55%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 55%] Building C object CMakeFiles/provisioning_settings_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 55%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 55%] Building C object CMakeFiles/web_api_core_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_core.c.o
[ 55%] Building C object CMakeFiles/provisioning_settings_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/provisioning/provisioning_core.c.o
[ 55%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 55%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 55%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 55%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 55%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 55%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 55%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 55%] Building C object CMakeFiles/web_execution_submit_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 55%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 55%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic_validators.c.o
[ 55%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 55%] Building C object CMakeFiles/storage_atomic_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 55%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 55%] Building C object CMakeFiles/storage_progress_repository_tests.dir/test_storage_progress.c.o
[ 85%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 55%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 55%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 55%] Building C object CMakeFiles/storage_mount_tests.dir/test_storage_mount.c.o
[ 88%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 89%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 90%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 91%] Building C object CMakeFiles/web_setup_json_tests.dir/test_web_setup_json.c.o
[ 92%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 89%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 89%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 89%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 89%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 89%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 88%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 89%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_progress.c.o
[ 89%] Building C object CMakeFiles/web_setup_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_setup_core.c.o
[ 89%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 92%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 92%] Linking C executable app_operation_result_tests
[ 93%] Linking C executable web_api_response_tests
[ 94%] Linking C executable storage_repository_lock_tests
[ 94%] Linking C executable web_security_tests
[ 94%] Linking C executable macro_model_tests
[ 94%] Linking C executable provisioning_bootstrap_tests
[ 94%] Linking C executable test_support_tests
[ 94%] Built target app_operation_result_tests
[ 94%] Linking C executable web_setup_json_tests
[ 94%] Built target web_api_response_tests
[ 94%] Linking C executable web_api_json_tests
[ 94%] Built target storage_repository_lock_tests
[ 94%] Built target web_security_tests
[ 95%] Linking C executable web_execution_submit_tests
[ 95%] Linking C executable web_request_policy_tests
[ 95%] Built target macro_model_tests
[ 95%] Built target provisioning_bootstrap_tests
[ 95%] Linking C executable storage_atomic_tests
[ 95%] Linking C executable storage_parent_sync_tests
[ 95%] Built target web_setup_json_tests
[ 95%] Built target test_support_tests
[ 95%] Linking C executable storage_mount_tests
[ 95%] Linking C executable storage_repository_io_tests
[ 96%] Linking C executable web_api_core_tests
[ 96%] Linking C executable macro_parser_tests
[ 96%] Built target web_api_json_tests
[ 96%] Built target web_execution_submit_tests
[ 96%] Built target web_request_policy_tests
[ 96%] Built target storage_atomic_tests
[ 96%] Linking C executable device_controls_tests
[ 96%] Built target storage_parent_sync_tests
[ 96%] Linking C executable web_setup_tests
[ 96%] Built target web_api_core_tests
[ 96%] Built target storage_mount_tests
[ 96%] Linking C executable web_server_adapter_tests
[ 96%] Built target storage_repository_io_tests
[ 97%] Linking C executable wifi_ap_tests
[ 97%] Linking C executable auth_tests
[ 97%] Built target macro_parser_tests
[ 97%] Built target device_controls_tests
[ 97%] Linking C executable usb_keyboard_tests
[ 97%] Built target web_setup_tests
[ 97%] Built target auth_tests
[ 97%] Built target web_server_adapter_tests
[ 97%] Built target wifi_ap_tests
[ 97%] Linking C executable storage_transaction_tests
[ 97%] Linking C executable provisioning_tests
[ 98%] Linking C executable provisioning_settings_tests
[ 98%] Linking C executable storage_object_json_tests
[ 99%] Linking C executable storage_quarantine_tests
[ 98%] Linking C executable macro_executor_tests
[ 99%] Built target usb_keyboard_tests
[ 99%] Built target provisioning_tests
[ 99%] Built target storage_quarantine_tests
[ 99%] Built target storage_transaction_tests
[ 99%] Built target provisioning_settings_tests
[ 99%] Built target storage_object_json_tests
[ 99%] Linking C executable storage_progress_repository_tests
[ 99%] Linking C executable storage_repository_tests
[ 99%] Built target macro_executor_tests
[ 99%] Linking C executable app_core_tests
[ 99%] Linking C executable storage_atomic_recovery_tests
[ 99%] Linking C executable storage_atomic_validators_tests
[100%] Linking C executable storage_macro_repository_tests
[100%] Linking C executable storage_procedure_repository_tests
[100%] Built target storage_progress_repository_tests
[100%] Built target storage_atomic_validators_tests
[100%] Linking C executable storage_active_set_delete_tests
[100%] Built target storage_repository_tests
[100%] Built target app_core_tests
[100%] Built target storage_procedure_repository_tests
[100%] Built target storage_atomic_recovery_tests
[100%] Built target storage_macro_repository_tests
[100%] Built target storage_active_set_delete_tests
Internal ctest changing into directory: /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/build-sanitizers
Test project /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/build-sanitizers
    Start 5: macro_executor
1/1 Test #5: macro_executor ...................   Passed    0.01 sec

100% tests passed, 0 tests failed out of 1

Label Time Summary:
executor    =   0.01 sec*proc (1 test)

Total Test time (real) =   0.01 sec
```

## phase16-gate.log

```text
1623:    bugprone-shared-ptr-array-mismatch
1653:    bugprone-unique-ptr-array-mismatch
2096:/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_api.c:348:9: error: Value stored to 'result' is never read [clang-analyzer-deadcode.DeadStores,-warnings-as-errors]
2288:error: run-clang-tidy failed for /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware with status 1

Suppressed 1445 warnings (1445 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[45/80][0.8s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_executor/macro_executor.c
1308 warnings generated.
Suppressed 1308 warnings (1308 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[46/80][0.0s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c
[47/80][5.5s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c
1115 warnings generated.
Suppressed 1115 warnings (1115 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[48/80][0.1s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_mount_core.c
144 warnings generated.
Suppressed 144 warnings (144 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[49/80][5.4s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic_recovery.c
1042 warnings generated.
Suppressed 1042 warnings (1042 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[50/80][0.5s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_static_path.c
672 warnings generated.
Suppressed 672 warnings (672 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[51/80][0.2s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_adapter_lifecycle.c
687 warnings generated.
Suppressed 687 warnings (687 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[52/80][0.5s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_macros.c
998 warnings generated.
Suppressed 998 warnings (998 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[53/80][0.4s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_logout_execution.c
1442 warnings generated.
Suppressed 1442 warnings (1442 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[54/80][0.3s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c
1440 warnings generated.
Suppressed 1440 warnings (1440 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[55/80][0.1s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_mount_topology.c
816 warnings generated.
Suppressed 816 warnings (816 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[56/80][0.9s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_core.c
749 warnings generated.
Suppressed 749 warnings (749 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[57/80][0.2s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/provisioning/provisioning_bootstrap.c
874 warnings generated.
Suppressed 874 warnings (874 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[58/80][0.1s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c
739 warnings generated.
Suppressed 739 warnings (739 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[59/80][0.8s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c
1133 warnings generated.
Suppressed 1133 warnings (1133 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[60/80][3.6s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/provisioning/provisioning_core.c
692 warnings generated.
Suppressed 692 warnings (692 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[61/80][2.4s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_cookie.c
682 warnings generated.
Suppressed 682 warnings (682 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[62/80][0.3s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_static.c
1447 warnings generated.
Suppressed 1447 warnings (1447 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[63/80][0.1s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/auth/auth_core_rate_limit.c
311 warnings generated.
Suppressed 311 warnings (311 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[64/80][0.2s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c
745 warnings generated.
Suppressed 745 warnings (745 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[65/80][2.4s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c
1451 warnings generated.
Suppressed 1451 warnings (1451 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[66/80][0.1s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/main/app_main.c
745 warnings generated.
Suppressed 745 warnings (745 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[67/80][0.1s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/auth/auth_core_password.c
687 warnings generated.
Suppressed 687 warnings (687 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[68/80][0.4s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_progress.c
913 warnings generated.
Suppressed 913 warnings (913 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[69/80][0.3s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/auth/auth_core_common.c
687 warnings generated.
Suppressed 687 warnings (687 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[70/80][0.2s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_request_policy.c
712 warnings generated.
Suppressed 712 warnings (712 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[71/80][0.3s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/auth/auth.c
1479 warnings generated.
Suppressed 1479 warnings (1479 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[72/80][4.0s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c
1137 warnings generated.
Suppressed 1137 warnings (1137 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[73/80][0.3s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_cancel.c
1442 warnings generated.
Suppressed 1442 warnings (1442 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[74/80][0.4s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/wifi_ap/wifi_ap_state.c
682 warnings generated.
Suppressed 682 warnings (682 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[75/80][3.5s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_parser.c
866 warnings generated.
Suppressed 866 warnings (866 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[76/80][0.4s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/usb_keyboard/usb_keyboard.c
1454 warnings generated.
Suppressed 1454 warnings (1454 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[77/80][0.1s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/provisioning/provisioning_bootstrap_core.c
682 warnings generated.
Suppressed 682 warnings (682 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[78/80][3.9s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/app_core/app_core_sequence.c
687 warnings generated.
Suppressed 687 warnings (687 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[79/80][6.3s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c
1186 warnings generated.
Suppressed 1186 warnings (1186 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

[80/80][2.0s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_adapter_common.c
149 warnings generated.
Suppressed 149 warnings (149 in non-user code).
Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.

error: run-clang-tidy failed for /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware with status 1
```
