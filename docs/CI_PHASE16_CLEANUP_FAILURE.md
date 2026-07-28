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

[ 68%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 69%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 70%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 71%] Building C object CMakeFiles/storage_atomic_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 72%] Building C object CMakeFiles/web_api_core_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 73%] Building C object CMakeFiles/storage_object_json_tests.dir/test_storage_object_json.c.o
[ 74%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 75%] Building C object CMakeFiles/provisioning_bootstrap_tests.dir/test_provisioning_bootstrap.c.o
[ 76%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 77%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_cookie.c.o
[ 78%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 79%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 80%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 81%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 82%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 83%] Building C object CMakeFiles/web_execution_submit_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 84%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 85%] Building C object CMakeFiles/web_api_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 86%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 87%] Building C object CMakeFiles/storage_object_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 88%] Building C object CMakeFiles/web_setup_json_tests.dir/test_web_setup_json.c.o
[ 89%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 90%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 91%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 65%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 92%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 65%] Building C object CMakeFiles/provisioning_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/provisioning/provisioning_core.c.o
[ 65%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 65%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 65%] Building C object CMakeFiles/storage_parent_sync_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 65%] Building C object CMakeFiles/web_api_core_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_core.c.o
[ 65%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 65%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 65%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 65%] Building C object CMakeFiles/web_api_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 65%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 65%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 65%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 65%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[ 65%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 65%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 65%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 65%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_request_policy.c.o
[ 65%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 65%] Building C object CMakeFiles/storage_quarantine_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 65%] Building C object CMakeFiles/web_api_response_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c.o
[ 65%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/test_storage_active_set_delete.c.o
[ 65%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[ 65%] Building C object CMakeFiles/web_api_core_tests.dir/test_web_api_core.c.o
[ 65%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 65%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 65%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_core.c.o
[ 65%] Building C object CMakeFiles/web_api_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_json.c.o
[ 65%] Building C object CMakeFiles/storage_object_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 65%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 65%] Building C object CMakeFiles/storage_object_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 65%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 65%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 65%] Building C object CMakeFiles/web_execution_submit_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_execution_submit.c.o
[ 65%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 65%] Building C object CMakeFiles/web_execution_submit_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 65%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 65%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 65%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[ 65%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 65%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[ 65%] Building C object CMakeFiles/web_api_json_tests.dir/test_web_api_json.c.o
[ 65%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[ 65%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_origin.c.o
[ 65%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 65%] Building C object CMakeFiles/web_api_core_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 65%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[ 65%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 65%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 65%] Building C object CMakeFiles/web_setup_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_setup_core.c.o
[ 65%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[ 65%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 65%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[ 65%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 65%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_progress.c.o
[ 65%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[ 65%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[ 65%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[ 65%] Building C object CMakeFiles/provisioning_bootstrap_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/provisioning/provisioning_bootstrap_core.c.o
[ 65%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 65%] Building C object CMakeFiles/provisioning_settings_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[ 65%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[ 65%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[ 65%] Building C object CMakeFiles/web_execution_submit_tests.dir/test_web_execution_submit.c.o
[ 65%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[ 65%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic_validators.c.o
[ 65%] Building C object CMakeFiles/storage_repository_tests.dir/test_storage_repository.c.o
[ 65%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[ 65%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[ 92%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[ 92%] Linking C executable app_operation_result_tests
[ 93%] Linking C executable storage_repository_lock_tests
[ 93%] Linking C executable web_security_tests
[ 93%] Linking C executable macro_model_tests
[ 93%] Linking C executable test_support_tests
[ 93%] Linking C executable web_setup_json_tests
[ 93%] Built target app_operation_result_tests
[ 94%] Linking C executable web_api_response_tests
[ 94%] Built target storage_repository_lock_tests
[ 94%] Linking C executable provisioning_bootstrap_tests
[ 94%] Linking C executable storage_mount_tests
[ 94%] Built target web_security_tests
[ 94%] Built target macro_model_tests
[ 94%] Built target test_support_tests
[ 94%] Built target web_setup_json_tests
[ 94%] Linking C executable storage_atomic_tests
[ 94%] Built target web_api_response_tests
[ 94%] Linking C executable storage_parent_sync_tests
[ 95%] Linking C executable web_api_json_tests
[ 95%] Linking C executable web_execution_submit_tests
[ 95%] Built target provisioning_bootstrap_tests
[ 96%] Linking C executable web_api_core_tests
[ 96%] Linking C executable storage_repository_io_tests
[ 96%] Linking C executable macro_parser_tests
[ 96%] Built target storage_mount_tests
[ 96%] Linking C executable web_request_policy_tests
[ 96%] Built target storage_atomic_tests
[ 96%] Built target web_execution_submit_tests
[ 96%] Built target storage_parent_sync_tests
[ 96%] Linking C executable device_controls_tests
[ 96%] Built target web_api_core_tests
[ 96%] Linking C executable web_setup_tests
[ 96%] Built target web_api_json_tests
[ 97%] Linking C executable wifi_ap_tests
[ 97%] Built target storage_repository_io_tests
[ 97%] Built target macro_parser_tests
[ 97%] Linking C executable auth_tests
[ 97%] Linking C executable web_server_adapter_tests
[ 97%] Linking C executable usb_keyboard_tests
[ 97%] Built target web_request_policy_tests
[ 97%] Built target web_setup_tests
[ 97%] Linking C executable storage_transaction_tests
[ 97%] Built target device_controls_tests
[ 97%] Built target auth_tests
[ 97%] Built target wifi_ap_tests
[ 98%] Linking C executable provisioning_settings_tests
[ 98%] Built target web_server_adapter_tests
[ 98%] Built target usb_keyboard_tests
[ 98%] Linking C executable macro_executor_tests
[ 98%] Built target storage_transaction_tests
[ 98%] Linking C executable storage_atomic_recovery_tests
[ 98%] Linking C executable provisioning_tests
[ 98%] Built target provisioning_settings_tests
[ 98%] Linking C executable storage_object_json_tests
[ 99%] Linking C executable storage_quarantine_tests
[ 99%] Linking C executable storage_procedure_repository_tests
[ 99%] Built target storage_object_json_tests
[ 99%] Built target macro_executor_tests
[ 99%] Linking C executable storage_repository_tests
[ 99%] Built target provisioning_tests
[ 99%] Built target storage_quarantine_tests
[ 99%] Built target storage_atomic_recovery_tests
[100%] Linking C executable storage_macro_repository_tests
[100%] Linking C executable app_core_tests
[100%] Linking C executable storage_atomic_validators_tests
[100%] Built target storage_procedure_repository_tests
[100%] Built target storage_repository_tests
[100%] Built target app_core_tests
[100%] Linking C executable storage_active_set_delete_tests
[100%] Linking C executable storage_progress_repository_tests
[100%] Built target storage_atomic_validators_tests
[100%] Built target storage_macro_repository_tests
[100%] Built target storage_active_set_delete_tests
[100%] Built target storage_progress_repository_tests
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
1282:FAILED: [code=1] esp-idf/web_server/CMakeFiles/__idf_web_server.dir/web_api_sets.c.obj
1285:/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_sets.c:121:15: error: 'WEB_SET_DELETE_RESPONSE_BYTES' undeclared (first use in this function)
1292:/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_sets.c:128:1: error: control reaches end of non-void function [-Werror=return-type]

[949/1200] Building C object esp-idf/espressif__tinyusb/CMakeFiles/__idf_espressif__tinyusb.dir/lib/networking/rndis_reports.c.obj
[950/1200] Building C object esp-idf/espressif__tinyusb/CMakeFiles/__idf_espressif__tinyusb.dir/src/portable/synopsys/dwc2/dwc2_common.c.obj
[951/1200] Linking C static library esp-idf/fatfs/libfatfs.a
[952/1200] Performing configure step for 'bootloader'
-- Found Git: /usr/bin/git (found version "2.54.0")
-- Component directory /home/runner/esp/esp-idf-v5.5.5/components/esp_blockdev does not contain a CMakeLists.txt file. No component will be added
-- Minimal build - OFF
-- The C compiler identification is GNU 14.2.0
-- The CXX compiler identification is GNU 14.2.0
-- The ASM compiler identification is GNU
-- Found assembler: /home/runner/.espressif/tools/xtensa-esp-elf/esp-14.2.0_20260121/xtensa-esp-elf/bin/xtensa-esp32s3-elf-gcc
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working C compiler: /home/runner/.espressif/tools/xtensa-esp-elf/esp-14.2.0_20260121/xtensa-esp-elf/bin/xtensa-esp32s3-elf-gcc - skipped
-- Detecting C compile features
-- Detecting C compile features - done
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: /home/runner/.espressif/tools/xtensa-esp-elf/esp-14.2.0_20260121/xtensa-esp-elf/bin/xtensa-esp32s3-elf-g++ - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Building ESP-IDF components for target esp32s3
-- ESP-TEE is currently supported only on the esp32c6;esp32h2;esp32c5 SoCs
-- Project sdkconfig file /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/sdkconfig
-- Compiler supported targets: xtensa-esp-elf
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Adding linker script /home/runner/esp/esp-idf-v5.5.5/components/soc/esp32s3/ld/esp32s3.peripherals.ld
-- Bootloader project name: "bootloader" version: 1
-- Adding linker script /home/runner/esp/esp-idf-v5.5.5/components/esp_rom/esp32s3/ld/esp32s3.rom.ld
-- Adding linker script /home/runner/esp/esp-idf-v5.5.5/components/esp_rom/esp32s3/ld/esp32s3.rom.api.ld
-- Adding linker script /home/runner/esp/esp-idf-v5.5.5/components/esp_rom/esp32s3/ld/esp32s3.rom.bt_funcs.ld
-- Adding linker script /home/runner/esp/esp-idf-v5.5.5/components/esp_rom/esp32s3/ld/esp32s3.rom.libgcc.ld
-- Adding linker script /home/runner/esp/esp-idf-v5.5.5/components/esp_rom/esp32s3/ld/esp32s3.rom.wdt.ld
-- Adding linker script /home/runner/esp/esp-idf-v5.5.5/components/esp_rom/esp32s3/ld/esp32s3.rom.version.ld
-- Adding linker script /home/runner/esp/esp-idf-v5.5.5/components/esp_rom/esp32s3/ld/esp32s3.rom.libc.ld
-- Adding linker script /home/runner/esp/esp-idf-v5.5.5/components/esp_rom/esp32s3/ld/esp32s3.rom.newlib.ld
-- Adding linker script /home/runner/esp/esp-idf-v5.5.5/components/bootloader/subproject/main/ld/esp32s3/bootloader.rom.ld
-- Components: bootloader bootloader_support efuse esp_app_format esp_bootloader_format esp_common esp_hw_support esp_rom esp_security esp_system esptool_py freertos hal log main micro-ecc newlib partition_table soc spi_flash xtensa
-- Component paths: /home/runner/esp/esp-idf-v5.5.5/components/bootloader /home/runner/esp/esp-idf-v5.5.5/components/bootloader_support /home/runner/esp/esp-idf-v5.5.5/components/efuse /home/runner/esp/esp-idf-v5.5.5/components/esp_app_format /home/runner/esp/esp-idf-v5.5.5/components/esp_bootloader_format /home/runner/esp/esp-idf-v5.5.5/components/esp_common /home/runner/esp/esp-idf-v5.5.5/components/esp_hw_support /home/runner/esp/esp-idf-v5.5.5/components/esp_rom /home/runner/esp/esp-idf-v5.5.5/components/esp_security /home/runner/esp/esp-idf-v5.5.5/components/esp_system /home/runner/esp/esp-idf-v5.5.5/components/esptool_py /home/runner/esp/esp-idf-v5.5.5/components/freertos /home/runner/esp/esp-idf-v5.5.5/components/hal /home/runner/esp/esp-idf-v5.5.5/components/log /home/runner/esp/esp-idf-v5.5.5/components/bootloader/subproject/main /home/runner/esp/esp-idf-v5.5.5/components/bootloader/subproject/components/micro-ecc /home/runner/esp/esp-idf-v5.5.5/components/newlib /home/runner/esp/esp-idf-v5.5.5/components/partition_table /home/runner/esp/esp-idf-v5.5.5/components/soc /home/runner/esp/esp-idf-v5.5.5/components/spi_flash /home/runner/esp/esp-idf-v5.5.5/components/xtensa
-- Adding linker script /home/runner/esp/esp-idf-v5.5.5/components/bootloader/subproject/main/ld/esp32s3/bootloader.ld
-- Configuring done (4.4s)
-- Generating done (0.1s)
-- Build files have been written to: /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build/bootloader
[953/1200] Linking C static library esp-idf/usb/libusb.a
[954/1200] Building C object esp-idf/macro_model/CMakeFiles/__idf_macro_model.dir/app_error.c.obj
[955/1200] Building C object esp-idf/macro_model/CMakeFiles/__idf_macro_model.dir/macro_model.c.obj
[956/1200] Building C object esp-idf/macro_model/CMakeFiles/__idf_macro_model.dir/app_uuid.c.obj
[957/1200] Building C object esp-idf/espressif__tinyusb/CMakeFiles/__idf_espressif__tinyusb.dir/src/tusb.c.obj
[958/1200] Building C object esp-idf/espressif__tinyusb/CMakeFiles/__idf_espressif__tinyusb.dir/src/portable/synopsys/dwc2/dcd_dwc2.c.obj
[959/1200] Building C object esp-idf/espressif__esp_tinyusb/CMakeFiles/__idf_espressif__esp_tinyusb.dir/tinyusb.c.obj
[960/1200] Linking C static library esp-idf/macro_model/libmacro_model.a
[961/1200] Building C object esp-idf/espressif__esp_tinyusb/CMakeFiles/__idf_espressif__esp_tinyusb.dir/usb_descriptors.c.obj
[962/1200] Building C object esp-idf/espressif__tinyusb/CMakeFiles/__idf_espressif__tinyusb.dir/src/common/tusb_fifo.c.obj
[963/1200] Building C object esp-idf/espressif__esp_tinyusb/CMakeFiles/__idf_espressif__esp_tinyusb.dir/descriptors_control.c.obj
[964/1200] Building C object esp-idf/espressif__esp_tinyusb/CMakeFiles/__idf_espressif__esp_tinyusb.dir/tinyusb_task.c.obj
[965/1200] Building C object esp-idf/espressif__tinyusb/CMakeFiles/__idf_espressif__tinyusb.dir/src/device/usbd.c.obj
[966/1200] Building C object esp-idf/auth/CMakeFiles/__idf_auth.dir/auth_core_password.c.obj
[967/1200] Linking C static library esp-idf/espressif__tinyusb/libespressif__tinyusb.a
[968/1200] Building C object esp-idf/auth/CMakeFiles/__idf_auth.dir/auth.c.obj
[969/1200] Building C object esp-idf/auth/CMakeFiles/__idf_auth.dir/auth_core_common.c.obj
[970/1200] Building C object esp-idf/auth/CMakeFiles/__idf_auth.dir/auth_core_rate_limit.c.obj
[971/1200] Linking C static library esp-idf/espressif__esp_tinyusb/libespressif__esp_tinyusb.a
[972/1200] Building C object esp-idf/auth/CMakeFiles/__idf_auth.dir/auth_core_session.c.obj
[973/1200] Building C object esp-idf/usb_keyboard/CMakeFiles/__idf_usb_keyboard.dir/usb_keyboard_state.c.obj
[974/1200] Building C object esp-idf/wifi_ap/CMakeFiles/__idf_wifi_ap.dir/wifi_ap.c.obj
[975/1200] Building C object esp-idf/wifi_ap/CMakeFiles/__idf_wifi_ap.dir/wifi_ap_state.c.obj
[976/1200] Building C object esp-idf/macro_parser/CMakeFiles/__idf_macro_parser.dir/macro_keymap_us.c.obj
[977/1200] Building C object esp-idf/usb_keyboard/CMakeFiles/__idf_usb_keyboard.dir/usb_descriptors.c.obj
[978/1200] Linking C static library esp-idf/wifi_ap/libwifi_ap.a
[979/1200] Linking C static library esp-idf/auth/libauth.a
[980/1200] Building C object esp-idf/usb_keyboard/CMakeFiles/__idf_usb_keyboard.dir/usb_keyboard.c.obj
[981/1200] Building C object esp-idf/provisioning/CMakeFiles/__idf_provisioning.dir/provisioning_bootstrap.c.obj
[982/1200] Building C object esp-idf/provisioning/CMakeFiles/__idf_provisioning.dir/provisioning.c.obj
[983/1200] Building C object esp-idf/provisioning/CMakeFiles/__idf_provisioning.dir/provisioning_bootstrap_core.c.obj
[984/1200] Building C object esp-idf/joltwallet__littlefs/CMakeFiles/__idf_joltwallet__littlefs.dir/src/littlefs/lfs_util.c.obj
[985/1200] Building C object esp-idf/json/CMakeFiles/__idf_json.dir/cJSON/cJSON_Utils.c.obj
[986/1200] Building C object esp-idf/macro_parser/CMakeFiles/__idf_macro_parser.dir/macro_parser.c.obj
[987/1200] Building C object esp-idf/joltwallet__littlefs/CMakeFiles/__idf_joltwallet__littlefs.dir/src/littlefs_esp_part.c.obj
[988/1200] Building C object esp-idf/joltwallet__littlefs/CMakeFiles/__idf_joltwallet__littlefs.dir/src/lfs_config.c.obj
[989/1200] Linking C static library esp-idf/macro_parser/libmacro_parser.a
[990/1200] Linking C static library esp-idf/usb_keyboard/libusb_keyboard.a
[991/1200] Building C object esp-idf/provisioning/CMakeFiles/__idf_provisioning.dir/provisioning_core.c.obj
[992/1200] Linking C static library esp-idf/provisioning/libprovisioning.a
[993/1200] Building C object esp-idf/storage/CMakeFiles/__idf_storage.dir/storage_mount.c.obj
[994/1200] Building C object esp-idf/macro_executor/CMakeFiles/__idf_macro_executor.dir/macro_executor_engine.c.obj
[995/1200] Building C object esp-idf/macro_executor/CMakeFiles/__idf_macro_executor.dir/macro_executor.c.obj
[996/1200] Building C object esp-idf/storage/CMakeFiles/__idf_storage.dir/storage_mount_core.c.obj
[997/1200] Building C object esp-idf/storage/CMakeFiles/__idf_storage.dir/storage_mount_topology.c.obj
[998/1200] Building C object esp-idf/storage/CMakeFiles/__idf_storage.dir/storage_paths.c.obj
[999/1200] Building C object esp-idf/storage/CMakeFiles/__idf_storage.dir/storage_atomic_validators.c.obj
[1000/1200] Building C object esp-idf/storage/CMakeFiles/__idf_storage.dir/storage_atomic_recovery.c.obj
[1001/1200] Building C object esp-idf/storage/CMakeFiles/__idf_storage.dir/storage_fs_ops.c.obj
[1002/1200] Building C object esp-idf/json/CMakeFiles/__idf_json.dir/cJSON/cJSON.c.obj
[1003/1200] Linking C static library esp-idf/json/libjson.a
[1004/1200] Building C object esp-idf/storage/CMakeFiles/__idf_storage.dir/storage_repository_json.c.obj
[1005/1200] Building C object esp-idf/joltwallet__littlefs/CMakeFiles/__idf_joltwallet__littlefs.dir/src/esp_littlefs.c.obj
[1006/1200] Building C object esp-idf/storage/CMakeFiles/__idf_storage.dir/storage_repository_io.c.obj
[1007/1200] Building C object esp-idf/storage/CMakeFiles/__idf_storage.dir/storage_json.c.obj
[1008/1200] Building C object esp-idf/storage/CMakeFiles/__idf_storage.dir/storage_atomic.c.obj
[1009/1200] Building C object esp-idf/storage/CMakeFiles/__idf_storage.dir/storage_repository_lock.c.obj
[1010/1200] Building C object esp-idf/storage/CMakeFiles/__idf_storage.dir/storage_repository_index.c.obj
[1011/1200] Building C object esp-idf/storage/CMakeFiles/__idf_storage.dir/storage_transaction.c.obj
[1012/1200] Building C object esp-idf/storage/CMakeFiles/__idf_storage.dir/storage_repository_order.c.obj
[1013/1200] Building C object esp-idf/storage/CMakeFiles/__idf_storage.dir/storage_repository_sets.c.obj
[1014/1200] Building C object esp-idf/storage/CMakeFiles/__idf_storage.dir/storage_repository_objects_json.c.obj
[1015/1200] Linking C static library esp-idf/macro_executor/libmacro_executor.a
[1016/1200] Building C object esp-idf/storage/CMakeFiles/__idf_storage.dir/storage_repository_procedures.c.obj
[1017/1200] Building C object esp-idf/storage/CMakeFiles/__idf_storage.dir/storage_repository_progress.c.obj
[1018/1200] Building C object esp-idf/storage/CMakeFiles/__idf_storage.dir/storage_repository_macros.c.obj
[1019/1200] Building C object esp-idf/device_controls/CMakeFiles/__idf_device_controls.dir/device_controls.c.obj
[1020/1200] Building C object esp-idf/console/CMakeFiles/__idf_console.dir/esp_console_common.c.obj
[1021/1200] Building C object esp-idf/console/CMakeFiles/__idf_console.dir/split_argv.c.obj
[1022/1200] Building C object esp-idf/device_controls/CMakeFiles/__idf_device_controls.dir/device_controls_logic.c.obj
[1023/1200] Building C object esp-idf/console/CMakeFiles/__idf_console.dir/commands.c.obj
[1024/1200] Building C object esp-idf/console/CMakeFiles/__idf_console.dir/esp_console_repl_internal.c.obj
[1025/1200] Building C object esp-idf/storage/CMakeFiles/__idf_storage.dir/storage_quarantine.c.obj
[1026/1200] Building C object esp-idf/console/CMakeFiles/__idf_console.dir/esp_console_repl_chip.c.obj
[1027/1200] Building C object esp-idf/console/CMakeFiles/__idf_console.dir/argtable3/arg_cmd.c.obj
[1028/1200] Building C object esp-idf/console/CMakeFiles/__idf_console.dir/argtable3/arg_dbl.c.obj
[1029/1200] Building C object esp-idf/console/CMakeFiles/__idf_console.dir/argtable3/arg_end.c.obj
[1030/1200] Building C object esp-idf/console/CMakeFiles/__idf_console.dir/argtable3/arg_dstr.c.obj
[1031/1200] Building C object esp-idf/joltwallet__littlefs/CMakeFiles/__idf_joltwallet__littlefs.dir/src/littlefs/lfs.c.obj
[1032/1200] Building C object esp-idf/console/CMakeFiles/__idf_console.dir/argtable3/arg_file.c.obj
[1033/1200] Building C object esp-idf/console/CMakeFiles/__idf_console.dir/argtable3/arg_date.c.obj
[1034/1200] Linking C static library esp-idf/joltwallet__littlefs/libjoltwallet__littlefs.a
[1035/1200] Building C object esp-idf/console/CMakeFiles/__idf_console.dir/argtable3/arg_lit.c.obj
[1036/1200] Building C object esp-idf/console/CMakeFiles/__idf_console.dir/argtable3/arg_rem.c.obj
[1037/1200] Building C object esp-idf/console/CMakeFiles/__idf_console.dir/argtable3/arg_int.c.obj
[1038/1200] Building C object esp-idf/console/CMakeFiles/__idf_console.dir/argtable3/arg_hashtable.c.obj
[1039/1200] Building C object esp-idf/console/CMakeFiles/__idf_console.dir/argtable3/arg_str.c.obj
[1040/1200] Building C object esp-idf/console/CMakeFiles/__idf_console.dir/argtable3/arg_utils.c.obj
[1041/1200] Building C object esp-idf/support/CMakeFiles/__idf_support.dir/app_operation_result.c.obj
[1042/1200] Linking C static library esp-idf/device_controls/libdevice_controls.a
[1043/1200] Linking C static library esp-idf/storage/libstorage.a
[1044/1200] Building C object esp-idf/console/CMakeFiles/__idf_console.dir/linenoise/linenoise.c.obj
[1045/1200] Building C object esp-idf/web_server/CMakeFiles/__idf_web_server.dir/web_server_status_limits.c.obj
[1046/1200] Building C object esp-idf/console/CMakeFiles/__idf_console.dir/argtable3/arg_rex.c.obj
[1047/1200] Building C object esp-idf/web_server/CMakeFiles/__idf_web_server.dir/web_server_common.c.obj
[1048/1200] Building C object esp-idf/web_server/CMakeFiles/__idf_web_server.dir/web_server_login.c.obj
[1049/1200] Building C object esp-idf/web_server/CMakeFiles/__idf_web_server.dir/web_server_logout_execution.c.obj
[1050/1200] Building C object esp-idf/web_server/CMakeFiles/__idf_web_server.dir/web_server_cancel.c.obj
[1051/1200] Building C object esp-idf/web_server/CMakeFiles/__idf_web_server.dir/web_server_static.c.obj
[1052/1200] Building C object esp-idf/web_server/CMakeFiles/__idf_web_server.dir/web_server_adapter_common.c.obj
[1053/1200] Building C object esp-idf/web_server/CMakeFiles/__idf_web_server.dir/web_server_lifecycle.c.obj
[1054/1200] Building C object esp-idf/web_server/CMakeFiles/__idf_web_server.dir/web_server_setup.c.obj
[1055/1200] Building C object esp-idf/web_server/CMakeFiles/__idf_web_server.dir/web_server_adapter_body_auth.c.obj
[1056/1200] Building C object esp-idf/console/CMakeFiles/__idf_console.dir/argtable3/argtable3.c.obj
[1057/1200] Building C object esp-idf/web_server/CMakeFiles/__idf_web_server.dir/web_server_adapter_json.c.obj
[1058/1200] Building C object esp-idf/web_server/CMakeFiles/__idf_web_server.dir/web_server_adapter_lifecycle.c.obj
[1059/1200] Building C object esp-idf/web_server/CMakeFiles/__idf_web_server.dir/web_server_api.c.obj
[1060/1200] Building C object esp-idf/web_server/CMakeFiles/__idf_web_server.dir/web_server_adapter_static_stream.c.obj
[1061/1200] Building C object esp-idf/web_server/CMakeFiles/__idf_web_server.dir/web_api_sets.c.obj
FAILED: [code=1] esp-idf/web_server/CMakeFiles/__idf_web_server.dir/web_api_sets.c.obj
/home/runner/.espressif/tools/xtensa-esp-elf/esp-14.2.0_20260121/xtensa-esp-elf/bin/xtensa-esp32s3-elf-gcc -DESP_PLATFORM -DIDF_VER=\"v5.5.5\" -DMBEDTLS_CONFIG_FILE=\"mbedtls/esp_config.h\" -DSOC_MMU_PAGE_SIZE=CONFIG_MMU_PAGE_SIZE -DSOC_XTAL_FREQ_MHZ=CONFIG_XTAL_FREQ -D_GLIBCXX_HAVE_POSIX_SEMAPHORE -D_GLIBCXX_USE_POSIX_SEMAPHORE -D_GNU_SOURCE -D_POSIX_READER_WRITER_LOCKS -I/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build/config -I/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/include -I/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server -I/home/runner/esp/esp-idf-v5.5.5/components/newlib/platform_include -I/home/runner/esp/esp-idf-v5.5.5/components/freertos/config/include -I/home/runner/esp/esp-idf-v5.5.5/components/freertos/config/include/freertos -I/home/runner/esp/esp-idf-v5.5.5/components/freertos/config/xtensa/include -I/home/runner/esp/esp-idf-v5.5.5/components/freertos/FreeRTOS-Kernel/include -I/home/runner/esp/esp-idf-v5.5.5/components/freertos/FreeRTOS-Kernel/portable/xtensa/include -I/home/runner/esp/esp-idf-v5.5.5/components/freertos/FreeRTOS-Kernel/portable/xtensa/include/freertos -I/home/runner/esp/esp-idf-v5.5.5/components/freertos/esp_additions/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_hw_support/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_hw_support/include/soc -I/home/runner/esp/esp-idf-v5.5.5/components/esp_hw_support/include/soc/esp32s3 -I/home/runner/esp/esp-idf-v5.5.5/components/esp_hw_support/dma/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_hw_support/ldo/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_hw_support/debug_probe/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_hw_support/mspi_timing_tuning/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_hw_support/mspi_timing_tuning/tuning_scheme_impl/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_hw_support/power_supply/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_hw_support/port/esp32s3/. -I/home/runner/esp/esp-idf-v5.5.5/components/esp_hw_support/port/esp32s3/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_hw_support/mspi_timing_tuning/port/esp32s3/. -I/home/runner/esp/esp-idf-v5.5.5/components/esp_hw_support/mspi_timing_tuning/port/esp32s3/include -I/home/runner/esp/esp-idf-v5.5.5/components/heap/include -I/home/runner/esp/esp-idf-v5.5.5/components/heap/tlsf -I/home/runner/esp/esp-idf-v5.5.5/components/log/include -I/home/runner/esp/esp-idf-v5.5.5/components/soc/include -I/home/runner/esp/esp-idf-v5.5.5/components/soc/esp32s3 -I/home/runner/esp/esp-idf-v5.5.5/components/soc/esp32s3/include -I/home/runner/esp/esp-idf-v5.5.5/components/soc/esp32s3/register -I/home/runner/esp/esp-idf-v5.5.5/components/hal/platform_port/include -I/home/runner/esp/esp-idf-v5.5.5/components/hal/esp32s3/include -I/home/runner/esp/esp-idf-v5.5.5/components/hal/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_rom/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_rom/esp32s3/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_rom/esp32s3/include/esp32s3 -I/home/runner/esp/esp-idf-v5.5.5/components/esp_rom/esp32s3 -I/home/runner/esp/esp-idf-v5.5.5/components/esp_common/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_system/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_system/port/soc -I/home/runner/esp/esp-idf-v5.5.5/components/esp_system/port/include/private -I/home/runner/esp/esp-idf-v5.5.5/components/xtensa/esp32s3/include -I/home/runner/esp/esp-idf-v5.5.5/components/xtensa/include -I/home/runner/esp/esp-idf-v5.5.5/components/xtensa/deprecated_include -I/home/runner/esp/esp-idf-v5.5.5/components/lwip/include -I/home/runner/esp/esp-idf-v5.5.5/components/lwip/include/apps -I/home/runner/esp/esp-idf-v5.5.5/components/lwip/include/apps/sntp -I/home/runner/esp/esp-idf-v5.5.5/components/lwip/lwip/src/include -I/home/runner/esp/esp-idf-v5.5.5/components/lwip/port/include -I/home/runner/esp/esp-idf-v5.5.5/components/lwip/port/freertos/include -I/home/runner/esp/esp-idf-v5.5.5/components/lwip/port/esp32xx/include -I/home/runner/esp/esp-idf-v5.5.5/components/lwip/port/esp32xx/include/arch -I/home/runner/esp/esp-idf-v5.5.5/components/lwip/port/esp32xx/include/sys -I/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/include -I/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_executor/include -I/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/include -I/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/usb_keyboard/include -I/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/managed_components/espressif__esp_tinyusb/include -I/home/runner/esp/esp-idf-v5.5.5/components/fatfs/diskio -I/home/runner/esp/esp-idf-v5.5.5/components/fatfs/src -I/home/runner/esp/esp-idf-v5.5.5/components/fatfs/vfs -I/home/runner/esp/esp-idf-v5.5.5/components/wear_levelling/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_partition/include -I/home/runner/esp/esp-idf-v5.5.5/components/sdmmc/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_driver_sdmmc/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_driver_gpio/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_driver_sdspi/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_driver_spi/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_pm/include -I/home/runner/esp/esp-idf-v5.5.5/components/vfs/include -I/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/managed_components/espressif__tinyusb/src -I/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/wifi_ap/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_wifi/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_wifi/include/local -I/home/runner/esp/esp-idf-v5.5.5/components/esp_wifi/wifi_apps/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_wifi/wifi_apps/nan_app/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_event/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_phy/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_phy/esp32s3/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_netif/include -I/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/auth/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_timer/include -I/home/runner/esp/esp-idf-v5.5.5/components/mbedtls/port/include -I/home/runner/esp/esp-idf-v5.5.5/components/mbedtls/mbedtls/include -I/home/runner/esp/esp-idf-v5.5.5/components/mbedtls/mbedtls/library -I/home/runner/esp/esp-idf-v5.5.5/components/mbedtls/esp_crt_bundle/include -I/home/runner/esp/esp-idf-v5.5.5/components/mbedtls/mbedtls/3rdparty/everest/include -I/home/runner/esp/esp-idf-v5.5.5/components/mbedtls/mbedtls/3rdparty/p256-m -I/home/runner/esp/esp-idf-v5.5.5/components/mbedtls/mbedtls/3rdparty/p256-m/p256-m -I/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/provisioning/include -I/home/runner/esp/esp-idf-v5.5.5/components/nvs_flash/include -I/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/device_controls/include -I/home/runner/esp/esp-idf-v5.5.5/components/driver/deprecated -I/home/runner/esp/esp-idf-v5.5.5/components/driver/i2c/include -I/home/runner/esp/esp-idf-v5.5.5/components/driver/touch_sensor/include -I/home/runner/esp/esp-idf-v5.5.5/components/driver/twai/include -I/home/runner/esp/esp-idf-v5.5.5/components/driver/touch_sensor/esp32s3/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_ringbuf/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_driver_pcnt/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_driver_gptimer/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_driver_mcpwm/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_driver_ana_cmpr/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_driver_i2s/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_driver_sdio/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_driver_dac/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_driver_rmt/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_driver_tsens/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_driver_sdm/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_driver_i2c/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_driver_uart/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_driver_ledc/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_driver_parlio/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_driver_usb_serial_jtag/include -I/home/runner/esp/esp-idf-v5.5.5/components/esp_driver_twai/include -I/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/include -I/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/managed_components/joltwallet__littlefs/include -I/home/runner/esp/esp-idf-v5.5.5/components/json/cJSON -I/home/runner/esp/esp-idf-v5.5.5/components/esp_http_server/include -I/home/runner/esp/esp-idf-v5.5.5/components/http_parser @"/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build/toolchain/cflags" -fdiagnostics-color=always -ffunction-sections -fdata-sections -Wall -Werror=all -Wno-error=unused-function -Wno-error=unused-variable -Wno-error=unused-but-set-variable -Wno-error=deprecated-declarations -Wextra -Wno-error=extra -Wno-unused-parameter -Wno-sign-compare -Wno-enum-conversion -gdwarf-4 -ggdb -O2 -Wwrite-strings -fmacro-prefix-map=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware=. -fmacro-prefix-map=/home/runner/esp/esp-idf-v5.5.5=/IDF -fstrict-volatile-bitfields -fno-jump-tables -fno-tree-switch-conversion -std=gnu17 -Wno-old-style-declaration -Werror -Wshadow -Wconversion -Wsign-conversion -Wformat=2 -Wdouble-promotion -Wmissing-declarations -Wstrict-prototypes -DCFG_TUSB_MCU=OPT_MCU_ESP32S3 -MD -MT esp-idf/web_server/CMakeFiles/__idf_web_server.dir/web_api_sets.c.obj -MF esp-idf/web_server/CMakeFiles/__idf_web_server.dir/web_api_sets.c.obj.d -o esp-idf/web_server/CMakeFiles/__idf_web_server.dir/web_api_sets.c.obj -c /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_sets.c
/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_sets.c: In function 'handle_set_item':
/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_sets.c:121:15: error: 'WEB_SET_DELETE_RESPONSE_BYTES' undeclared (first use in this function)
  121 |     char data[WEB_SET_DELETE_RESPONSE_BYTES];
      |               ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_sets.c:121:15: note: each undeclared identifier is reported only once for each function it appears in
/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_sets.c:121:10: warning: unused variable 'data' [-Wunused-variable]
  121 |     char data[WEB_SET_DELETE_RESPONSE_BYTES];
      |          ^~~~
/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_sets.c:128:1: error: control reaches end of non-void function [-Werror=return-type]
  128 | }
      | ^
cc1: all warnings being treated as errors
[1062/1200] Building C object esp-idf/web_server/CMakeFiles/__idf_web_server.dir/web_api_response.c.obj
[1063/1200] Building C object esp-idf/web_server/CMakeFiles/__idf_web_server.dir/web_api_json.c.obj
[1064/1200] Building C object esp-idf/web_server/CMakeFiles/__idf_web_server.dir/web_api_core.c.obj
[1065/1200] Building C object esp-idf/web_server/CMakeFiles/__idf_web_server.dir/web_api_handler_common.c.obj
[1066/1200] Building C object esp-idf/protobuf-c/CMakeFiles/__idf_protobuf-c.dir/protobuf-c/protobuf-c/protobuf-c.c.obj
ninja: build stopped: subcommand failed.
ninja failed with exit code 1, output of the command is in the /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build/log/idf_py_stderr_output_10880 and /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build/log/idf_py_stdout_output_10880
```
