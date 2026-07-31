# Phase 18.3 API integration failure

```text
-- The C compiler identification is GNU 13.3.0
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working C compiler: /usr/bin/cc - skipped
-- Detecting C compile features
-- Detecting C compile features - done
-- Found PkgConfig: /usr/bin/pkg-config (found version "1.8.1")
-- Checking for module 'libcjson'
--   Found libcjson, version 1.7.17
-- Configuring done (2.2s)
-- Generating done (0.1s)
-- Build files have been written to: /tmp/esp32-p183-api
[1/80] Building C object CMakeFiles/test_support.dir/support/test_assert.c.o
[2/80] Building C object CMakeFiles/test_support.dir/fakes/fake_clock.c.o
[3/80] Building C object CMakeFiles/test_support.dir/fakes/fake_random.c.o
[4/80] Building C object CMakeFiles/test_support.dir/support/test_memory.c.o
[5/80] Building C object CMakeFiles/test_support.dir/fakes/fake_freertos.c.o
[6/80] Building C object CMakeFiles/test_support.dir/fakes/fake_call_log.c.o
[7/80] Building C object CMakeFiles/test_support.dir/fakes/fake_gpio_backend.c.o
[8/80] Building C object CMakeFiles/test_support.dir/fakes/fake_usb_backend.c.o
[9/80] Building C object CMakeFiles/test_support.dir/fakes/fake_wifi_backend.c.o
[10/80] Building C object CMakeFiles/test_support.dir/fakes/fake_http_backend.c.o
[11/80] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[12/80] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[13/80] Building C object CMakeFiles/test_support.dir/fakes/fake_fs_backend.c.o
[14/80] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[15/80] Building C object CMakeFiles/test_support.dir/support/test_temp_dir.c.o
[16/80] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[17/80] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[18/80] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[19/80] Building C object CMakeFiles/storage_package_replace_tests.dir/test_storage_package_replace.c.o
[20/80] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[21/80] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[22/80] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction_replace.c.o
[23/80] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[24/80] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[25/80] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[26/80] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[27/80] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[28/80] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[29/80] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[30/80] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_set_operations.c.o
[31/80] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[32/80] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[33/80] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_progress.c.o
[34/80] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_parser.c.o
[35/80] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_keymap_us.c.o
[36/80] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[37/80] Building C object CMakeFiles/web_api_core_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[38/80] Building C object CMakeFiles/web_api_core_tests.dir/test_web_api_core.c.o
[39/80] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_package_replace.c.o
[40/80] Building C object CMakeFiles/web_api_core_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[41/80] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_set_tree.c.o
[42/80] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_package.c.o
[43/80] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[44/80] Building C object CMakeFiles/web_api_core_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_core.c.o
[45/80] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[46/80] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[47/80] Building C object CMakeFiles/web_api_set_export_tests.dir/test_web_api_set_export.c.o
[48/80] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[49/80] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[50/80] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[51/80] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[52/80] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[53/80] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction_replace.c.o
[54/80] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[55/80] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[56/80] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[57/80] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[58/80] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[59/80] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_set_operations.c.o
[60/80] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[61/80] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[62/80] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_progress.c.o
[63/80] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[64/80] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[65/80] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_keymap_us.c.o
[66/80] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_parser.c.o
[67/80] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[68/80] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_package_export.c.o
[69/80] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_package_replace.c.o
[70/80] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_package.c.o
[71/80] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_set_tree.c.o
[72/80] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c.o
[73/80] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_core.c.o
[74/80] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_json.c.o
[75/80] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_handler_common.c.o
[76/80] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_sets.c.o
[77/80] Linking C static library libtest_support.a
[78/80] Linking C executable web_api_core_tests
[79/80] Linking C executable web_api_set_export_tests
[80/80] Linking C executable storage_package_replace_tests
Internal ctest changing into directory: /tmp/esp32-p183-api
Test project /tmp/esp32-p183-api
    Start 25: storage_package_replace
1/3 Test #25: storage_package_replace ..........   Passed    0.04 sec
    Start 29: web_api_core
2/3 Test #29: web_api_core .....................   Passed    0.00 sec
    Start 46: web_api_set_export
3/3 Test #46: web_api_set_export ...............   Passed    0.04 sec

100% tests passed, 0 tests failed out of 3

Label Time Summary:
storage    =   0.08 sec*proc (2 tests)
web        =   0.04 sec*proc (2 tests)

Total Test time (real) =   0.15 sec
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
-- Build files have been written to: /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/build
[  0%] Building C object CMakeFiles/test_support.dir/support/test_assert.c.o
[  0%] Building C object CMakeFiles/test_support.dir/support/test_memory.c.o
[  1%] Building C object CMakeFiles/test_support.dir/support/test_temp_dir.c.o
[  1%] Building C object CMakeFiles/test_support.dir/fakes/fake_call_log.c.o
[  1%] Building C object CMakeFiles/test_support.dir/fakes/fake_clock.c.o
[  1%] Building C object CMakeFiles/test_support.dir/fakes/fake_gpio_backend.c.o
[  1%] Building C object CMakeFiles/test_support.dir/fakes/fake_random.c.o
[  2%] Building C object CMakeFiles/test_support.dir/fakes/fake_freertos.c.o
[  2%] Building C object CMakeFiles/test_support.dir/fakes/fake_usb_backend.c.o
[  2%] Building C object CMakeFiles/test_support.dir/fakes/fake_wifi_backend.c.o
[  3%] Building C object CMakeFiles/test_support.dir/fakes/fake_fs_backend.c.o
[  3%] Building C object CMakeFiles/test_support.dir/fakes/fake_http_backend.c.o
[  3%] Linking C static library libtest_support.a
[  3%] Built target test_support
[  4%] Building C object CMakeFiles/macro_parser_tests.dir/test_macro_parser.c.o
[  4%] Building C object CMakeFiles/app_operation_result_tests.dir/test_app_operation_result.c.o
[  4%] Building C object CMakeFiles/test_support_tests.dir/test_support.c.o
[  4%] Building C object CMakeFiles/macro_parser_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[  5%] Building C object CMakeFiles/macro_parser_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[  5%] Building C object CMakeFiles/macro_parser_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_parser.c.o
[  4%] Building C object CMakeFiles/macro_parser_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[  5%] Building C object CMakeFiles/macro_executor_tests.dir/test_macro_executor.c.o
[  6%] Building C object CMakeFiles/macro_parser_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_keymap_us.c.o
[  6%] Building C object CMakeFiles/app_operation_result_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/support/app_operation_result.c.o
[  6%] Building C object CMakeFiles/macro_model_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[  6%] Building C object CMakeFiles/macro_model_tests.dir/test_macro_model.c.o
[  6%] Building C object CMakeFiles/web_server_adapter_tests.dir/test_web_server_adapter.c.o
[  6%] Building C object CMakeFiles/macro_executor_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_executor/macro_executor_engine.c.o
[  6%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_adapter_body_auth.c.o
[  7%] Building C object CMakeFiles/auth_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/auth/auth_core_common.c.o
[  7%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[  7%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_adapter_json.c.o
[  7%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_adapter_lifecycle.c.o
[  7%] Building C object CMakeFiles/web_security_tests.dir/test_web_security.c.o
[  8%] Building C object CMakeFiles/web_security_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_static_path.c.o
[  9%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_cookie.c.o
[  9%] Building C object CMakeFiles/usb_keyboard_tests.dir/test_usb_keyboard.c.o
[  9%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_adapter_static_stream.c.o
[  9%] Building C object CMakeFiles/web_security_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_content.c.o
[  9%] Building C object CMakeFiles/web_security_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_cookie.c.o
[ 10%] Building C object CMakeFiles/app_core_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/support/app_operation_result.c.o
[ 11%] Building C object CMakeFiles/auth_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/auth/auth_core_rate_limit.c.o
[ 11%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_adapter_common.c.o
[ 11%] Building C object CMakeFiles/auth_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/auth/auth_core_password.c.o
[ 11%] Building C object CMakeFiles/auth_tests.dir/test_auth.c.o
[ 11%] Building C object CMakeFiles/app_core_tests.dir/test_app_core.c.o
[ 11%] Building C object CMakeFiles/auth_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/auth/auth_core_session.c.o
[  7%] Building C object CMakeFiles/web_security_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_origin.c.o
[ 11%] Building C object CMakeFiles/app_core_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/app_core/app_core_sequence.c.o
[ 11%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_content.c.o
[ 11%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_origin.c.o
[ 11%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_static_path.c.o
[ 11%] Building C object CMakeFiles/usb_keyboard_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/usb_keyboard/usb_keyboard_state.c.o
[ 11%] Building C object CMakeFiles/storage_set_tree_tests.dir/test_storage_set_tree.c.o
[ 12%] Building C object CMakeFiles/device_controls_tests.dir/test_device_controls.c.o
[ 12%] Building C object CMakeFiles/device_controls_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/device_controls/device_controls_logic.c.o
[ 13%] Building C object CMakeFiles/provisioning_tests.dir/test_provisioning.c.o
[ 13%] Building C object CMakeFiles/storage_repository_lock_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 13%] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 13%] Building C object CMakeFiles/wifi_ap_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/wifi_ap/wifi_ap_state.c.o
[ 13%] Building C object CMakeFiles/storage_repository_io_tests.dir/test_storage_repository_io.c.o
[ 13%] Building C object CMakeFiles/provisioning_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 14%] Building C object CMakeFiles/storage_repository_lock_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 14%] Building C object CMakeFiles/storage_repository_lock_tests.dir/test_storage_repository_lock.c.o
[ 14%] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 12%] Building C object CMakeFiles/wifi_ap_tests.dir/test_wifi_ap.c.o
[ 14%] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 14%] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_parser.c.o
[ 15%] Building C object CMakeFiles/web_api_core_tests.dir/test_web_api_core.c.o
[ 16%] Building C object CMakeFiles/storage_atomic_tests.dir/test_storage_atomic.c.o
[ 17%] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 17%] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 17%] Building C object CMakeFiles/storage_mount_tests.dir/test_storage_mount.c.o
[ 17%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/test_storage_atomic_validators.c.o
[ 17%] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_set_tree.c.o
[ 18%] Building C object CMakeFiles/provisioning_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/provisioning/provisioning_core.c.o
[ 18%] Building C object CMakeFiles/storage_mount_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 18%] Building C object CMakeFiles/web_execution_submit_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 19%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 19%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 19%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction_replace.c.o
[ 20%] Building C object CMakeFiles/storage_atomic_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 20%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 21%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 21%] Building C object CMakeFiles/storage_mount_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_mount_core.c.o
[ 22%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 23%] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 23%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 20%] Building C object CMakeFiles/storage_parent_sync_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 18%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 23%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 23%] Building C object CMakeFiles/provisioning_settings_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 23%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 23%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 23%] Building C object CMakeFiles/web_api_core_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_core.c.o
[ 23%] Building C object CMakeFiles/storage_package_replace_tests.dir/test_storage_package_replace.c.o
[ 23%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/test_storage_atomic_recovery.c.o
[ 23%] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 23%] Building C object CMakeFiles/storage_parent_sync_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 23%] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 23%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 23%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 23%] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 23%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 23%] Building C object CMakeFiles/storage_atomic_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 23%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 23%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 23%] Building C object CMakeFiles/web_api_core_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 23%] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_keymap_us.c.o
[ 24%] Building C object CMakeFiles/web_api_dispatch_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_dispatch.c.o
[ 25%] Building C object CMakeFiles/storage_object_json_tests.dir/test_storage_object_json.c.o
[ 25%] Building C object CMakeFiles/web_setup_json_tests.dir/test_web_setup_json.c.o
[ 26%] Building C object CMakeFiles/web_setup_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_setup_core.c.o
[ 26%] Building C object CMakeFiles/storage_atomic_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 27%] Building C object CMakeFiles/web_execution_submit_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_execution_submit.c.o
[ 27%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 28%] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 29%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 29%] Building C object CMakeFiles/storage_object_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 30%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 30%] Building C object CMakeFiles/storage_object_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 31%] Building C object CMakeFiles/storage_quarantine_tests.dir/test_storage_quarantine.c.o
[ 31%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 31%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/test_storage_active_set_delete.c.o
[ 31%] Building C object CMakeFiles/storage_object_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 31%] Building C object CMakeFiles/storage_object_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 32%] Building C object CMakeFiles/storage_progress_repository_tests.dir/test_storage_progress.c.o
[ 32%] Building C object CMakeFiles/web_setup_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_setup_json.c.o
[ 32%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 33%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 33%] Building C object CMakeFiles/storage_package_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 33%] Building C object CMakeFiles/web_api_admin_boundary_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 31%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 32%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 34%] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 34%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 34%] Building C object CMakeFiles/storage_package_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_package.c.o
[ 34%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 34%] Building C object CMakeFiles/storage_package_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 34%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 35%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 35%] Building C object CMakeFiles/storage_package_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 35%] Building C object CMakeFiles/storage_mount_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_mount_topology.c.o
[ 35%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction_replace.c.o
[ 35%] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 35%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 35%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 36%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 36%] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 36%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 36%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 36%] Building C object CMakeFiles/web_api_set_export_tests.dir/test_web_api_set_export.c.o
[ 37%] Building C object CMakeFiles/storage_atomic_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 37%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 37%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 38%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 38%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 38%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 38%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 36%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 36%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 39%] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 40%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 40%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 40%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 40%] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 41%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 41%] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 41%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 42%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 42%] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 41%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 44%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 44%] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_set_tree.c.o
[ 44%] Building C object CMakeFiles/provisioning_bootstrap_tests.dir/test_provisioning_bootstrap.c.o
[ 45%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 45%] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_core.c.o
[ 45%] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_parser.c.o
[ 45%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 45%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 45%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 46%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 46%] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_package_export.c.o
[ 46%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 46%] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_json.c.o
[ 46%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 46%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction_replace.c.o
[ 47%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic_validators.c.o
[ 47%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_set_operations.c.o
[ 48%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction_replace.c.o
[ 48%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 48%] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_sets.c.o
[ 49%] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c.o
[ 49%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 50%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 50%] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_handler_common.c.o
[ 50%] Building C object CMakeFiles/storage_repository_tests.dir/test_storage_repository.c.o
[ 51%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 52%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 52%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 47%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 52%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 53%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_parser.c.o
[ 53%] Building C object CMakeFiles/provisioning_bootstrap_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/provisioning/provisioning_bootstrap_core.c.o
[ 54%] Building C object CMakeFiles/web_api_dispatch_tests.dir/test_web_api_dispatch.c.o
[ 54%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 54%] Building C object CMakeFiles/web_request_policy_tests.dir/test_web_request_policy.c.o
[ 55%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 55%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_core.c.o
[ 56%] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 56%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_progress.c.o
[ 57%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 57%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c.o
[ 52%] Building C object CMakeFiles/web_api_core_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 57%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_origin.c.o
[ 57%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_core.c.o
[ 57%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_cookie.c.o
[ 58%] Building C object CMakeFiles/web_api_response_tests.dir/test_web_api_response.c.o
[ 58%] Building C object CMakeFiles/storage_transaction_tests.dir/test_storage_transactions.c.o
[ 59%] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_set_operations.c.o
[ 59%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_request_policy.c.o
[ 59%] Building C object CMakeFiles/web_execution_submit_tests.dir/test_web_execution_submit.c.o
[ 24%] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_progress.c.o
[ 24%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 24%] Building C object CMakeFiles/web_api_dispatch_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c.o
[ 33%] Building C object CMakeFiles/storage_package_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 43%] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_set_operations.c.o
[ 24%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_set_operations.c.o
[ 24%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 36%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 24%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 24%] Building C object CMakeFiles/web_api_dispatch_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 38%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 24%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 24%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 24%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 24%] Building C object CMakeFiles/web_execution_submit_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 24%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 24%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic_recovery.c.o
[ 24%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 24%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 24%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 24%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 24%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 24%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 24%] Building C object CMakeFiles/web_execution_submit_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 24%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 24%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 24%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 24%] Building C object CMakeFiles/storage_parent_sync_tests.dir/test_storage_parent_sync.c.o
[ 24%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 24%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 24%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 24%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 24%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 24%] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction_replace.c.o
[ 24%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction_replace.c.o
[ 24%] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 24%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 24%] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 24%] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 24%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 24%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic_validators.c.o
[ 24%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 24%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 24%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 24%] Building C object CMakeFiles/storage_parent_sync_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 24%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 24%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 24%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction_replace.c.o
[ 24%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 24%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 24%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 24%] Building C object CMakeFiles/provisioning_settings_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/provisioning/provisioning_core.c.o
[ 24%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 24%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 24%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 24%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 24%] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 24%] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 24%] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 24%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 24%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 24%] Building C object CMakeFiles/storage_parent_sync_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 24%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 24%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 24%] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 24%] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 24%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 24%] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 60%] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_set_tree.c.o
[ 61%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_package_export.c.o
[ 62%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 63%] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_package.c.o
[ 63%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_package_replace.c.o
[ 63%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_json.c.o
[ 63%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_macros.c.o
[ 63%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_set_tree.c.o
[ 63%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_sets.c.o
[ 63%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_package.c.o
[ 64%] Building C object CMakeFiles/storage_repository_io_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 65%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 66%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 67%] Building C object CMakeFiles/storage_package_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 64%] Linking C executable app_operation_result_tests
[ 68%] Building C object CMakeFiles/web_api_admin_boundary_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 68%] Building C object CMakeFiles/web_api_json_tests.dir/test_web_api_json.c.o
[ 68%] Building C object CMakeFiles/web_execution_route_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 67%] Building C object CMakeFiles/web_execution_route_policy_tests.dir/test_web_execution_route_policy.c.o
[ 68%] Building C object CMakeFiles/web_api_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 68%] Building C object CMakeFiles/web_execution_route_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 69%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 69%] Building C object CMakeFiles/web_api_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 69%] Building C object CMakeFiles/web_execution_route_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_execution_route_policy.c.o
[ 70%] Building C object CMakeFiles/storage_package_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_parser.c.o
[ 70%] Building C object CMakeFiles/web_api_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 69%] Building C object CMakeFiles/web_api_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 70%] Linking C executable macro_model_tests
[ 71%] Building C object CMakeFiles/storage_package_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_keymap_us.c.o
[ 71%] Building C object CMakeFiles/web_api_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_json.c.o
[ 72%] Building C object CMakeFiles/web_api_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 73%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 74%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 70%] Building C object CMakeFiles/web_api_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 75%] Building C object CMakeFiles/storage_package_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 76%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 77%] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 78%] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 79%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_set_operations.c.o
[ 79%] Linking C executable web_security_tests
[ 80%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_progress.c.o
[ 80%] Linking C executable test_support_tests
[ 81%] Building C object CMakeFiles/provisioning_settings_tests.dir/test_provisioning_settings.c.o
[ 82%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 83%] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 84%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 85%] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 85%] Linking C executable storage_mount_tests
[ 86%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 87%] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_package.c.o
[ 87%] Linking C executable macro_parser_tests
[ 60%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 60%] Building C object CMakeFiles/storage_package_tests.dir/test_storage_package.c.o
[ 60%] Building C object CMakeFiles/storage_package_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 60%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 60%] Building C object CMakeFiles/web_api_response_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c.o
[ 60%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 60%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_progress.c.o
[ 60%] Building C object CMakeFiles/storage_package_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_keymap_us.c.o
[ 60%] Building C object CMakeFiles/storage_package_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 60%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/test_storage_procedures.c.o
[ 60%] Building C object CMakeFiles/storage_package_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 60%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 60%] Building C object CMakeFiles/web_api_response_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 60%] Building C object CMakeFiles/storage_package_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 60%] Building C object CMakeFiles/web_api_admin_boundary_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_admin_boundary.c.o
[ 60%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 60%] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction_replace.c.o
[ 60%] Building C object CMakeFiles/web_setup_tests.dir/test_web_setup.c.o
[ 60%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 60%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 60%] Building C object CMakeFiles/web_api_admin_boundary_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_core.c.o
[ 60%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/test_web_api_repository_handlers.c.o
[ 60%] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 60%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 60%] Building C object CMakeFiles/web_api_admin_boundary_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c.o
[ 60%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 60%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 60%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 60%] Building C object CMakeFiles/storage_macro_repository_tests.dir/test_storage_macros.c.o
[ 60%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 60%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 60%] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 60%] Building C object CMakeFiles/storage_package_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 60%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 60%] Building C object CMakeFiles/storage_package_export_tests.dir/test_storage_package_export.c.o
[ 60%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 60%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_keymap_us.c.o
[ 60%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 60%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction_replace.c.o
[ 60%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 60%] Building C object CMakeFiles/web_api_admin_boundary_tests.dir/test_web_api_admin_boundary.c.o
[ 60%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 60%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 60%] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_keymap_us.c.o
[ 60%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 60%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 60%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 60%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 60%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 60%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 60%] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 60%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 60%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 60%] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 60%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 60%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 60%] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_package_replace.c.o
[ 60%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 60%] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 60%] Building C object CMakeFiles/storage_package_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_parser.c.o
[ 60%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 60%] Building C object CMakeFiles/storage_package_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 60%] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 60%] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_parser.c.o
[ 60%] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_progress.c.o
[ 60%] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 60%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction_replace.c.o
[ 60%] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 60%] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 60%] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 60%] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_keymap_us.c.o
[ 60%] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 60%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 60%] Building C object CMakeFiles/web_api_set_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_package_replace.c.o
[ 60%] Building C object CMakeFiles/storage_package_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_package_export.c.o
[ 60%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 60%] Building C object CMakeFiles/storage_package_export_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_package.c.o
[ 60%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 88%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_procedures.c.o
[ 89%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_handler_common.c.o
[ 88%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction_replace.c.o
[ 89%] Building C object CMakeFiles/web_api_repository_handlers_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 89%] Built target app_operation_result_tests
[ 89%] Linking C executable provisioning_bootstrap_tests
[ 90%] Linking C executable web_server_adapter_tests
[ 90%] Linking C executable usb_keyboard_tests
[ 90%] Built target macro_model_tests
[ 91%] Linking C executable web_api_dispatch_tests
[ 91%] Built target web_security_tests
[ 91%] Built target test_support_tests
[ 91%] Built target storage_mount_tests
[ 91%] Linking C executable web_api_response_tests
[ 91%] Built target macro_parser_tests
[ 92%] Linking C executable wifi_ap_tests
[ 94%] Linking C executable macro_executor_tests
[ 93%] Linking C executable auth_tests
[ 94%] Linking C executable storage_object_json_tests
[ 94%] Built target provisioning_bootstrap_tests
[ 94%] Linking C executable provisioning_tests
[ 94%] Built target usb_keyboard_tests
[ 94%] Built target web_server_adapter_tests
[ 94%] Built target web_api_dispatch_tests
[ 94%] Linking C executable storage_atomic_tests
[ 94%] Linking C executable web_api_core_tests
[ 94%] Linking C executable device_controls_tests
[ 94%] Linking C executable web_setup_json_tests
[ 94%] Linking C executable storage_repository_lock_tests
[ 94%] Linking C executable web_api_admin_boundary_tests
[ 94%] Built target web_api_response_tests
[ 95%] Linking C executable web_request_policy_tests
[ 95%] Linking C executable storage_parent_sync_tests
[ 94%] Linking C executable storage_set_tree_tests
[ 95%] Linking C executable web_execution_submit_tests
[ 95%] Linking C executable storage_repository_io_tests
[ 95%] Built target macro_executor_tests
[ 95%] Built target auth_tests
[ 95%] Built target provisioning_tests
[ 95%] Built target wifi_ap_tests
[ 96%] Linking C executable web_api_json_tests
[ 96%] Linking C executable web_setup_tests
[ 96%] Built target storage_object_json_tests
[ 96%] Built target web_request_policy_tests
[ 97%] Linking C executable web_execution_route_policy_tests
[ 97%] Linking C executable provisioning_settings_tests
[ 97%] Built target web_api_core_tests
[ 97%] Built target storage_atomic_tests
[ 97%] Linking C executable storage_quarantine_tests
[ 97%] Built target web_setup_json_tests
[ 97%] Built target web_execution_submit_tests
[ 97%] Built target device_controls_tests
[ 97%] Built target web_api_admin_boundary_tests
[ 97%] Linking C executable storage_progress_repository_tests
[ 98%] Linking C executable storage_package_tests
[ 98%] Built target storage_set_tree_tests
[ 98%] Built target storage_repository_io_tests
[ 98%] Built target storage_repository_lock_tests
[ 98%] Linking C executable storage_atomic_validators_tests
[ 98%] Built target storage_parent_sync_tests
[ 99%] Linking C executable web_api_set_export_tests
[ 99%] Built target provisioning_settings_tests
[ 99%] Linking C executable app_core_tests
[ 99%] Built target web_setup_tests
[ 99%] Linking C executable storage_transaction_tests
[ 99%] Linking C executable storage_package_replace_tests
[ 99%] Built target web_api_json_tests
[ 99%] Linking C executable storage_procedure_repository_tests
[ 99%] Linking C executable storage_macro_repository_tests
[ 99%] Linking C executable storage_active_set_delete_tests
[ 99%] Built target web_execution_route_policy_tests
[ 99%] Linking C executable storage_atomic_recovery_tests
[ 99%] Built target app_core_tests
[ 99%] Built target storage_atomic_validators_tests
[ 99%] Linking C executable storage_package_export_tests
[ 99%] Built target storage_quarantine_tests
[ 99%] Linking C executable web_api_repository_handlers_tests
[ 99%] Built target storage_progress_repository_tests
[ 99%] Built target storage_package_tests
[ 99%] Built target storage_atomic_recovery_tests
[ 99%] Built target storage_active_set_delete_tests
[ 99%] Built target storage_macro_repository_tests
[ 99%] Built target storage_transaction_tests
[100%] Linking C executable storage_repository_tests
[100%] Built target web_api_set_export_tests
[100%] Built target storage_package_replace_tests
[100%] Built target storage_package_export_tests
[100%] Built target storage_procedure_repository_tests
[100%] Built target storage_repository_tests
[100%] Built target web_api_repository_handlers_tests
Internal ctest changing into directory: /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/build
Test project /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/build
      Start  7: web_security
 1/14 Test  #7: web_security .....................   Passed    0.00 sec
      Start  8: web_server_adapter
 2/14 Test  #8: web_server_adapter ...............   Passed    0.00 sec
      Start 29: web_api_core
 3/14 Test #29: web_api_core .....................   Passed    0.00 sec
      Start 30: web_request_policy
 4/14 Test #30: web_request_policy ...............   Passed    0.00 sec
      Start 31: web_execution_submit
 5/14 Test #31: web_execution_submit .............   Passed    0.00 sec
      Start 32: web_execution_route_policy
 6/14 Test #32: web_execution_route_policy .......   Passed    0.00 sec
      Start 33: web_api_json
 7/14 Test #33: web_api_json .....................   Passed    0.00 sec
      Start 34: web_api_response
 8/14 Test #34: web_api_response .................   Passed    0.00 sec
      Start 35: web_api_dispatch
 9/14 Test #35: web_api_dispatch .................   Passed    0.00 sec
      Start 36: web_api_admin_boundary
10/14 Test #36: web_api_admin_boundary ...........   Passed    0.00 sec
      Start 37: web_api_repository_handlers
11/14 Test #37: web_api_repository_handlers ......***Failed    0.01 sec
test failure at /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_api_repository_handlers.c:251: response->status == status; expected=503, actual=422

      Start 39: web_setup
12/14 Test #39: web_setup ........................   Passed    0.00 sec
      Start 40: web_setup_json
13/14 Test #40: web_setup_json ...................   Passed    0.00 sec
      Start 46: web_api_set_export
14/14 Test #46: web_api_set_export ...............   Passed    0.03 sec

93% tests passed, 1 tests failed out of 14

Label Time Summary:
storage    =   0.03 sec*proc (1 test)
web        =   0.05 sec*proc (14 tests)

Total Test time (real) =   0.06 sec

The following tests FAILED:
	 37 - web_api_repository_handlers (Failed)              web
Errors while running CTest
```

## Pending diff

```diff
diff --git a/firmware/components/web_server/web_api_core.c b/firmware/components/web_server/web_api_core.c
index f54a2c3..c659e8e 100644
--- a/firmware/components/web_server/web_api_core.c
+++ b/firmware/components/web_server/web_api_core.c
@@ -414,7 +414,8 @@ bool web_api_physical_confirmation_required(web_api_route_t route,
     }
     return route == WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD ||
            route == WEB_API_ROUTE_DEVICE_RESTART || route == WEB_API_ROUTE_DEVICE_RESET_SETTINGS ||
-           route == WEB_API_ROUTE_DEVICE_FACTORY_RESET || route == WEB_API_ROUTE_RESTORE;
+           route == WEB_API_ROUTE_DEVICE_FACTORY_RESET || route == WEB_API_ROUTE_SET_IMPORT ||
+           route == WEB_API_ROUTE_RESTORE;
 }
 
 bool web_api_route_requires_physical_confirmation(web_api_route_t route) {
diff --git a/firmware/components/web_server/web_api_sets.c b/firmware/components/web_server/web_api_sets.c
index b706e45..99c75e9 100644
--- a/firmware/components/web_server/web_api_sets.c
+++ b/firmware/components/web_server/web_api_sets.c
@@ -261,6 +261,96 @@ static app_error_code_t handle_set_reorder(const web_api_call_t *call,
     return result;
 }
 
+
+typedef struct {
+    app_uuid_t target_set_id;
+    uint32_t expected_revision;
+    char *package_json;
+    size_t package_length;
+} web_set_import_request_t;
+
+static void free_set_import_request(web_set_import_request_t *request) {
+    if (request == NULL) {
+        return;
+    }
+    cJSON_free(request->package_json);
+    memset(request, 0, sizeof(*request));
+}
+
+static app_error_code_t parse_set_import(const web_api_call_t *call,
+                                         web_set_import_request_t *out_request) {
+    if (call == NULL || out_request == NULL || call->body == NULL || call->body_length == 0U) {
+        return APP_ERROR_INVALID_ARGUMENT;
+    }
+    memset(out_request, 0, sizeof(*out_request));
+    const char *parse_end = NULL;
+    cJSON *root = cJSON_ParseWithLengthOpts(call->body, call->body_length, &parse_end, false);
+    bool target_seen = false;
+    bool revision_seen = false;
+    bool package_seen = false;
+    app_error_code_t result =
+        root != NULL && parse_end == call->body + call->body_length && cJSON_IsObject(root)
+            ? APP_ERROR_NONE
+            : APP_ERROR_INVALID_ARGUMENT;
+    for (const cJSON *item = result == APP_ERROR_NONE ? root->child : NULL; item != NULL;
+         item = item->next) {
+        if (item->string != NULL && strcmp(item->string, "targetSetId") == 0 && !target_seen &&
+            cJSON_IsString(item) && item->valuestring != NULL &&
+            app_uuid_parse(item->valuestring, &out_request->target_set_id) == APP_ERROR_NONE) {
+            target_seen = true;
+        } else if (item->string != NULL && strcmp(item->string, "expectedRevision") == 0 &&
+                   !revision_seen && cJSON_IsNumber(item) && item->valuedouble >= 1.0 &&
+                   item->valuedouble <= (double)UINT32_MAX) {
+            const uint32_t revision = (uint32_t)item->valuedouble;
+            if ((double)revision != item->valuedouble) {
+                result = APP_ERROR_INVALID_ARGUMENT;
+                break;
+            }
+            out_request->expected_revision = revision;
+            revision_seen = true;
+        } else if (item->string != NULL && strcmp(item->string, "package") == 0 &&
+                   !package_seen && cJSON_IsObject(item)) {
+            out_request->package_json = cJSON_PrintUnformatted(item);
+            if (out_request->package_json == NULL) {
+                result = APP_ERROR_INTERNAL;
+                break;
+            }
+            out_request->package_length = strlen(out_request->package_json);
+            if (out_request->package_length == 0U ||
+                out_request->package_length > APP_IMPORT_PACKAGE_MAX_BYTES) {
+                result = APP_ERROR_INVALID_ARGUMENT;
+                break;
+            }
+            package_seen = true;
+        } else {
+            result = APP_ERROR_INVALID_ARGUMENT;
+            break;
+        }
+    }
+    cJSON_Delete(root);
+    if (result == APP_ERROR_NONE && (!target_seen || !revision_seen || !package_seen)) {
+        result = APP_ERROR_INVALID_ARGUMENT;
+    }
+    if (result != APP_ERROR_NONE) {
+        free_set_import_request(out_request);
+    }
+    return result;
+}
+
+static app_error_code_t handle_import(const web_api_call_t *call, web_api_response_t *response) {
+    web_set_import_request_t request = {0};
+    app_error_code_t result = parse_set_import(call, &request);
+    macro_set_t committed = {0};
+    if (result == APP_ERROR_NONE) {
+        result = storage_package_replace_set(&request.target_set_id, request.expected_revision,
+                                             request.package_json, request.package_length,
+                                             &committed);
+    }
+    free_set_import_request(&request);
+    return result == APP_ERROR_NONE ? send_set(response, WEB_HTTP_STATUS_OK, &committed)
+                                    : respond_result(response, result, "could not replace set");
+}
+
 static app_error_code_t handle_export(const web_api_call_t *call, web_api_response_t *response) {
     char *package_json = NULL;
     size_t package_length = 0U;
@@ -276,10 +366,6 @@ static app_error_code_t handle_export(const web_api_call_t *call, web_api_respon
     return result;
 }
 
-static app_error_code_t unavailable(web_api_response_t *response, const char *operation) {
-    return web_api_handler_error(response, APP_ERROR_STORAGE_UNAVAILABLE, operation, NULL);
-}
-
 app_error_code_t web_api_handle_sets(const web_api_call_t *call, web_api_response_t *response) {
     if (call == NULL || response == NULL) {
         return APP_ERROR_INVALID_ARGUMENT;
@@ -298,7 +384,7 @@ app_error_code_t web_api_handle_sets(const web_api_call_t *call, web_api_respons
     case WEB_API_ROUTE_SET_EXPORT:
         return handle_export(call, response);
     case WEB_API_ROUTE_SET_IMPORT:
-        return unavailable(response, "set import requires the Phase 18 package service");
+        return handle_import(call, response);
     default:
         return APP_ERROR_NOT_FOUND;
     }
diff --git a/firmware/components/web_server/web_server_api.c b/firmware/components/web_server/web_server_api.c
index 7ea542c..45f6189 100644
--- a/firmware/components/web_server/web_server_api.c
+++ b/firmware/components/web_server/web_server_api.c
@@ -113,6 +113,7 @@ static size_t route_body_limit(web_api_route_t route) {
     case WEB_API_ROUTE_PROCEDURE_PROGRESS:
         return STORAGE_PROGRESS_FILE_MAX_BYTES + WEB_API_WRAPPER_OVERHEAD_BYTES;
     case WEB_API_ROUTE_SET_IMPORT:
+        return APP_IMPORT_PACKAGE_MAX_BYTES + WEB_API_WRAPPER_OVERHEAD_BYTES;
     case WEB_API_ROUTE_RESTORE:
         return APP_IMPORT_PACKAGE_MAX_BYTES;
     default:
diff --git a/tests/host/cmake/extra_tests.cmake b/tests/host/cmake/extra_tests.cmake
index 62c968f..e0a9344 100644
--- a/tests/host/cmake/extra_tests.cmake
+++ b/tests/host/cmake/extra_tests.cmake
@@ -204,6 +204,8 @@ target_sources(
     web_api_repository_handlers_tests
     PRIVATE "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_package.c"
             "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_package_export.c"
+            "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_package_replace.c"
+            "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_set_tree.c"
 )
 
 add_executable(
@@ -214,6 +216,8 @@ add_executable(
     "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_parser/macro_keymap_us.c"
     "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_package.c"
     "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_package_export.c"
+    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_package_replace.c"
+    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_set_tree.c"
     "${CMAKE_SOURCE_DIR}/../../firmware/components/web_server/web_api_core.c"
     "${CMAKE_SOURCE_DIR}/../../firmware/components/web_server/web_api_response.c"
     "${CMAKE_SOURCE_DIR}/../../firmware/components/web_server/web_api_json.c"
@@ -234,7 +238,7 @@ target_include_directories(
 )
 target_compile_definitions(
     web_api_set_export_tests
-    PRIVATE STORAGE_DATA_MOUNT="${CMAKE_CURRENT_BINARY_DIR}/web-api-set-export-data"
+    PRIVATE STORAGE_DATA_MOUNT="/tmp/esp32-macro-keyboard-web-package"
 )
 target_link_libraries(web_api_set_export_tests PRIVATE PkgConfig::CJSON test_support)
 target_compile_options(web_api_set_export_tests PRIVATE ${STRICT_WARNINGS})
diff --git a/tests/host/test_web_api_core.c b/tests/host/test_web_api_core.c
index ca49d6a..c89f845 100644
--- a/tests/host/test_web_api_core.c
+++ b/tests/host/test_web_api_core.c
@@ -84,6 +84,7 @@ static void test_route_policy(void) {
     TEST_CHECK(web_api_route_requires_csrf(WEB_API_ROUTE_SETTINGS, WEB_API_METHOD_PUT));
     TEST_CHECK(!web_api_route_requires_csrf(WEB_API_ROUTE_SETTINGS, WEB_API_METHOD_GET));
     TEST_CHECK(web_api_route_requires_physical_confirmation(WEB_API_ROUTE_DEVICE_FACTORY_RESET));
+    TEST_CHECK(web_api_route_requires_physical_confirmation(WEB_API_ROUTE_SET_IMPORT));
     TEST_CHECK(web_api_route_requires_physical_confirmation(WEB_API_ROUTE_EXECUTIONS));
     TEST_CHECK(web_api_physical_confirmation_required(WEB_API_ROUTE_EXECUTIONS, true));
     TEST_CHECK(!web_api_physical_confirmation_required(WEB_API_ROUTE_EXECUTIONS, false));
diff --git a/tests/host/test_web_api_set_export.c b/tests/host/test_web_api_set_export.c
index d112b5a..0997355 100644
--- a/tests/host/test_web_api_set_export.c
+++ b/tests/host/test_web_api_set_export.c
@@ -9,6 +9,7 @@
 
 #include "app_error.h"
 #include "app_uuid.h"
+#include "cJSON.h"
 #include "macro_model.h"
 #include "provisioning.h"
 #include "storage_package.h"
@@ -261,6 +262,82 @@ static web_api_response_t invoke_export(const char *set_id) {
     return response;
 }
 
+
+static web_api_response_t invoke_import(const char *body) {
+    const web_api_call_t call = {
+        .method = WEB_API_METHOD_POST,
+        .path = {.route = WEB_API_ROUTE_SET_IMPORT},
+        .body = body,
+        .body_length = strlen(body),
+    };
+    web_api_response_t response = {0};
+    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_handle_sets(&call, &response));
+    TEST_CHECK(response.body != NULL);
+    return response;
+}
+
+static char *make_replacement_request(uint32_t expected_revision) {
+    web_api_response_t exported = invoke_export(SET_ID);
+    TEST_CHECK_EQ_U64(200U, exported.status);
+    const char *parse_end = NULL;
+    cJSON *package =
+        cJSON_ParseWithLengthOpts(exported.body, exported.body_length, &parse_end, false);
+    TEST_CHECK(package != NULL);
+    TEST_CHECK(parse_end == exported.body + exported.body_length);
+    cJSON *sets = cJSON_GetObjectItemCaseSensitive(package, "sets");
+    cJSON *set = cJSON_GetArrayItem(sets, 0);
+    TEST_CHECK(cJSON_IsObject(set));
+    cJSON *revision = cJSON_CreateNumber(2.0);
+    cJSON *name = cJSON_CreateString("Imported Replacement");
+    TEST_CHECK(revision != NULL);
+    TEST_CHECK(name != NULL);
+    TEST_CHECK(cJSON_ReplaceItemInObjectCaseSensitive(set, "revision", revision));
+    TEST_CHECK(cJSON_ReplaceItemInObjectCaseSensitive(set, "name", name));
+
+    cJSON *wrapper = cJSON_CreateObject();
+    TEST_CHECK(wrapper != NULL);
+    TEST_CHECK(cJSON_AddStringToObject(wrapper, "targetSetId", SET_ID) != NULL);
+    TEST_CHECK(cJSON_AddNumberToObject(wrapper, "expectedRevision", (double)expected_revision) !=
+               NULL);
+    TEST_CHECK(cJSON_AddItemToObject(wrapper, "package", package));
+    package = NULL;
+    char *request = cJSON_PrintUnformatted(wrapper);
+    TEST_CHECK(request != NULL);
+    cJSON_Delete(wrapper);
+    cJSON_Delete(package);
+    web_api_response_free(&exported);
+    return request;
+}
+
+static void test_import_route(void) {
+    char *request = make_replacement_request(1U);
+    web_api_response_t response = invoke_import(request);
+    TEST_CHECK_EQ_U64(200U, response.status);
+    TEST_CHECK(strstr(response.body, "\"ok\":true") != NULL);
+    TEST_CHECK(strstr(response.body, "Imported Replacement") != NULL);
+    TEST_CHECK(strstr(response.body, "\"revision\":2") != NULL);
+    macro_set_t committed = {0};
+    const app_uuid_t set_id = uuid(SET_ID);
+    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_read(&set_id, &committed));
+    TEST_CHECK_EQ_U64(2U, committed.revision);
+    TEST_CHECK_EQ_STRING("Imported Replacement", committed.name);
+    web_api_response_free(&response);
+    cJSON_free(request);
+
+    request = make_replacement_request(1U);
+    response = invoke_import(request);
+    TEST_CHECK_EQ_U64(409U, response.status);
+    TEST_CHECK(strstr(response.body, "\"ok\":false") != NULL);
+    TEST_CHECK(strstr(response.body, "could not replace set") != NULL);
+    web_api_response_free(&response);
+    cJSON_free(request);
+
+    response = invoke_import("{\"targetSetId\":\"" SET_ID
+                             "\",\"expectedRevision\":2,\"package\":{},\"extra\":true}");
+    TEST_CHECK_EQ_U64(422U, response.status);
+    web_api_response_free(&response);
+}
+
 static void test_export_route(void) {
     web_api_response_t response = invoke_export(SET_ID);
     TEST_CHECK_EQ_U64(200U, response.status);
@@ -301,6 +378,7 @@ int main(void) {
     populate_store();
     install_export_operations();
     test_export_route();
+    test_import_route();
     test_missing_set_error_envelope();
     storage_package_reset_export_ops_for_test();
     TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_deinit());
```
