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
1385:gcovr: error: unrecognized arguments: --no-color

[ 67%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 67%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 67%] Building C object CMakeFiles/storage_atomic_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 68%] Building C object CMakeFiles/web_execution_submit_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 69%] Building C object CMakeFiles/storage_object_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 70%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 71%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 71%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 72%] Building C object CMakeFiles/web_execution_route_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_execution_route_policy.c.o
[ 72%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 73%] Building C object CMakeFiles/web_api_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 74%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 75%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 76%] Building C object CMakeFiles/web_api_admin_boundary_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c.o
[ 77%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 78%] Building C object CMakeFiles/storage_macro_repository_tests.dir/test_storage_macros.c.o
[ 72%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_origin.c.o
[ 79%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 81%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 82%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 83%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 84%] Building C object CMakeFiles/web_api_dispatch_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c.o
[ 84%] Building C object CMakeFiles/web_api_dispatch_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_dispatch.c.o
[ 85%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_macros.c.o
[ 86%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 87%] Building C object CMakeFiles/web_api_core_tests.dir/test_web_api_core.c.o
[ 88%] Building C object CMakeFiles/provisioning_settings_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/provisioning/provisioning_core.c.o
[ 85%] Linking C executable macro_model_tests
[ 89%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 67%] Building C object CMakeFiles/web_execution_route_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 67%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 82%] Building C object CMakeFiles/web_api_dispatch_tests.dir/test_web_api_dispatch.c.o
[ 67%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 67%] Building C object CMakeFiles/web_execution_submit_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 67%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 67%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 67%] Building C object CMakeFiles/web_api_admin_boundary_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_admin_boundary.c.o
[ 67%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 67%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 67%] Building C object CMakeFiles/storage_parent_sync_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 67%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 67%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 67%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_procedures.c.o
[ 67%] Building C object CMakeFiles/provisioning_settings_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 67%] Building C object CMakeFiles/storage_transaction_tests.dir/test_storage_transactions.c.o
[ 67%] Building C object CMakeFiles/web_setup_json_tests.dir/test_web_setup_json.c.o
[ 67%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 67%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 67%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/test_web_api_repository_handlers.c.o
[ 80%] Linking C executable app_operation_result_tests
[ 67%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 67%] Building C object CMakeFiles/web_api_admin_boundary_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 67%] Building C object CMakeFiles/web_api_response_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c.o
[ 67%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 67%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 67%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 67%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 67%] Building C object CMakeFiles/storage_object_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 67%] Building C object CMakeFiles/web_setup_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_setup_core.c.o
[ 67%] Building C object CMakeFiles/provisioning_bootstrap_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/provisioning/provisioning_bootstrap_core.c.o
[ 67%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 67%] Building C object CMakeFiles/storage_atomic_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 67%] Building C object CMakeFiles/web_api_admin_boundary_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 67%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 67%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 67%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 67%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 67%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 67%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 67%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic_validators.c.o
[ 89%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 67%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 67%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 67%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 89%] Building C object CMakeFiles/storage_repository_tests.dir/test_storage_repository.c.o
[ 89%] Building C object CMakeFiles/web_api_dispatch_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 89%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 89%] Linking C executable test_support_tests
[ 90%] Built target macro_model_tests
[ 90%] Linking C executable web_execution_route_policy_tests
[ 90%] Linking C executable web_security_tests
[ 90%] Linking C executable storage_mount_tests
[ 90%] Linking C executable macro_parser_tests
[ 91%] Linking C executable provisioning_bootstrap_tests
[ 91%] Linking C executable storage_repository_lock_tests
[ 91%] Built target app_operation_result_tests
[ 91%] Linking C executable web_api_response_tests
[ 92%] Linking C executable web_api_dispatch_tests
[ 92%] Linking C executable web_execution_submit_tests
[ 92%] Linking C executable web_request_policy_tests
[ 93%] Linking C executable storage_parent_sync_tests
[ 93%] Linking C executable web_api_admin_boundary_tests
[ 93%] Built target test_support_tests
[ 93%] Linking C executable usb_keyboard_tests
[ 93%] Linking C executable storage_atomic_tests
[ 93%] Built target web_security_tests
[ 93%] Linking C executable web_setup_json_tests
[ 94%] Linking C executable web_api_core_tests
[ 94%] Built target web_execution_route_policy_tests
[ 94%] Linking C executable web_server_adapter_tests
[ 94%] Linking C executable provisioning_tests
[ 95%] Built target storage_repository_lock_tests
[ 95%] Built target macro_parser_tests
[ 95%] Linking C executable web_setup_tests
[ 95%] Built target storage_mount_tests
[ 95%] Linking C executable storage_repository_io_tests
[ 95%] Linking C executable auth_tests
[ 95%] Built target provisioning_bootstrap_tests
[ 95%] Built target web_api_dispatch_tests
[ 95%] Linking C executable macro_executor_tests
[ 96%] Built target web_api_response_tests
[ 96%] Built target web_execution_submit_tests
[ 96%] Linking C executable web_api_json_tests
[ 96%] Linking C executable device_controls_tests
[ 97%] Linking C executable wifi_ap_tests
[ 97%] Built target web_request_policy_tests
[ 97%] Built target web_api_admin_boundary_tests
[ 97%] Built target storage_parent_sync_tests
[ 97%] Built target usb_keyboard_tests
[ 97%] Built target web_setup_json_tests
[ 97%] Built target storage_atomic_tests
[ 97%] Linking C executable storage_progress_repository_tests
[ 97%] Built target web_server_adapter_tests
[ 97%] Built target provisioning_tests
[ 97%] Linking C executable storage_active_set_delete_tests
[ 97%] Built target web_setup_tests
[ 97%] Built target auth_tests
[ 97%] Linking C executable storage_object_json_tests
[ 97%] Built target web_api_core_tests
[ 97%] Linking C executable provisioning_settings_tests
[ 97%] Built target web_api_json_tests
[ 97%] Built target storage_repository_io_tests
[ 97%] Built target wifi_ap_tests
[ 98%] Linking C executable storage_atomic_recovery_tests
[ 98%] Linking C executable storage_macro_repository_tests
[ 98%] Built target macro_executor_tests
[ 98%] Linking C executable storage_atomic_validators_tests
[ 98%] Built target device_controls_tests
[ 98%] Linking C executable web_api_repository_handlers_tests
[ 98%] Built target provisioning_settings_tests
[ 98%] Built target storage_object_json_tests
[ 98%] Built target storage_progress_repository_tests
[ 98%] Built target storage_atomic_validators_tests
[ 99%] Linking C executable app_core_tests
[ 99%] Built target storage_active_set_delete_tests
[ 99%] Linking C executable storage_procedure_repository_tests
[100%] Linking C executable storage_quarantine_tests
[100%] Built target storage_atomic_recovery_tests
[100%] Built target app_core_tests
[100%] Built target web_api_repository_handlers_tests
[100%] Built target storage_macro_repository_tests
[100%] Linking C executable storage_transaction_tests
[100%] Built target storage_procedure_repository_tests
[100%] Built target storage_quarantine_tests
[100%] Linking C executable storage_repository_tests
[100%] Built target storage_transaction_tests
[100%] Built target storage_repository_tests
Internal ctest changing into directory: /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/build-coverage
Test project /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/build-coverage
      Start  1: test_support
 1/41 Test  #1: test_support ........................   Passed    0.17 sec
      Start  2: app_operation_result
 2/41 Test  #2: app_operation_result ................   Passed    0.00 sec
      Start  3: macro_parser
 3/41 Test  #3: macro_parser ........................   Passed    0.02 sec
      Start  4: macro_model
 4/41 Test  #4: macro_model .........................   Passed    0.00 sec
      Start  5: macro_executor
 5/41 Test  #5: macro_executor ......................   Passed    0.00 sec
      Start  6: auth
 6/41 Test  #6: auth ................................   Passed    0.00 sec
      Start  7: web_security
 7/41 Test  #7: web_security ........................   Passed    0.00 sec
      Start  8: web_server_adapter
 8/41 Test  #8: web_server_adapter ..................   Passed    0.00 sec
      Start  9: app_core
 9/41 Test  #9: app_core ............................   Passed    0.00 sec
      Start 10: usb_keyboard
10/41 Test #10: usb_keyboard ........................   Passed    0.00 sec
      Start 11: device_controls
11/41 Test #11: device_controls .....................   Passed    0.00 sec
      Start 12: wifi_ap
12/41 Test #12: wifi_ap .............................   Passed    0.00 sec
      Start 13: provisioning
13/41 Test #13: provisioning ........................   Passed    0.00 sec
      Start 14: storage_mount
14/41 Test #14: storage_mount .......................   Passed    0.00 sec
      Start 15: storage_atomic_recovery
15/41 Test #15: storage_atomic_recovery .............   Passed    0.02 sec
      Start 16: storage_atomic_validators
16/41 Test #16: storage_atomic_validators ...........   Passed    0.00 sec
      Start 17: storage_atomic
17/41 Test #17: storage_atomic ......................   Passed    0.02 sec
      Start 18: storage_repository_lock
18/41 Test #18: storage_repository_lock .............   Passed    0.00 sec
      Start 19: storage_parent_sync
19/41 Test #19: storage_parent_sync .................   Passed    0.01 sec
      Start 20: storage_repository_io
20/41 Test #20: storage_repository_io ...............   Passed    0.00 sec
      Start 21: storage_transaction
21/41 Test #21: storage_transaction .................   Passed    0.07 sec
      Start 22: storage_quarantine
22/41 Test #22: storage_quarantine ..................   Passed    0.12 sec
      Start 23: storage_repository
23/41 Test #23: storage_repository ..................   Passed    0.34 sec
      Start 24: storage_progress_repository_tests
24/41 Test #24: storage_progress_repository_tests ...   Passed    0.08 sec
      Start 25: storage_active_set_delete_tests
25/41 Test #25: storage_active_set_delete_tests .....   Passed    0.04 sec
      Start 26: provisioning_settings
26/41 Test #26: provisioning_settings ...............   Passed    0.00 sec
      Start 27: web_api_core
27/41 Test #27: web_api_core ........................   Passed    0.00 sec
      Start 28: web_request_policy
28/41 Test #28: web_request_policy ..................   Passed    0.00 sec
      Start 29: web_execution_submit
29/41 Test #29: web_execution_submit ................   Passed    0.00 sec
      Start 30: web_execution_route_policy
30/41 Test #30: web_execution_route_policy ..........   Passed    0.00 sec
      Start 31: web_api_json
31/41 Test #31: web_api_json ........................   Passed    0.00 sec
      Start 32: web_api_response
32/41 Test #32: web_api_response ....................   Passed    0.00 sec
      Start 33: web_api_dispatch
33/41 Test #33: web_api_dispatch ....................   Passed    0.00 sec
      Start 34: web_api_admin_boundary
34/41 Test #34: web_api_admin_boundary ..............   Passed    0.00 sec
      Start 35: web_api_repository_handlers
35/41 Test #35: web_api_repository_handlers .........   Passed    0.03 sec
      Start 36: provisioning_bootstrap
36/41 Test #36: provisioning_bootstrap ..............   Passed    0.00 sec
      Start 37: web_setup
37/41 Test #37: web_setup ...........................   Passed    0.00 sec
      Start 38: web_setup_json
38/41 Test #38: web_setup_json ......................   Passed    0.00 sec
      Start 39: storage_object_json
39/41 Test #39: storage_object_json .................   Passed    0.00 sec
      Start 40: storage_macro_repository
40/41 Test #40: storage_macro_repository ............   Passed    0.10 sec
      Start 41: storage_procedure_repository
41/41 Test #41: storage_procedure_repository ........   Passed    0.07 sec

100% tests passed, 0 tests failed out of 41

Label Time Summary:
auth        =   0.00 sec*proc (1 test)
controls    =   0.00 sec*proc (1 test)
executor    =   0.00 sec*proc (1 test)
model       =   0.00 sec*proc (1 test)
parser      =   0.02 sec*proc (1 test)
startup     =   0.00 sec*proc (1 test)
storage     =   0.88 sec*proc (18 tests)
support     =   0.17 sec*proc (2 tests)
usb         =   0.00 sec*proc (1 test)
web         =   0.04 sec*proc (13 tests)
wifi        =   0.00 sec*proc (1 test)

Total Test time (real) =   1.14 sec
usage: gcovr [options] [search_paths...]
gcovr: error: unrecognized arguments: --no-color
```
