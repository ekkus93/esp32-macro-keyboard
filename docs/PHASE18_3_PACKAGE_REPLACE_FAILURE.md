# Phase 18.3 package replacement failure

The production package replacement validation failed.

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
-- Configuring done (1.8s)
-- Generating done (0.0s)
-- Build files have been written to: /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/build/phase18-3-package-replace
[1/61] Building C object CMakeFiles/test_support.dir/support/test_assert.c.o
[2/61] Building C object CMakeFiles/test_support.dir/fakes/fake_clock.c.o
[3/61] Building C object CMakeFiles/test_support.dir/fakes/fake_random.c.o
[4/61] Building C object CMakeFiles/test_support.dir/fakes/fake_freertos.c.o
[5/61] Building C object CMakeFiles/test_support.dir/fakes/fake_usb_backend.c.o
[6/61] Building C object CMakeFiles/test_support.dir/support/test_memory.c.o
[7/61] Building C object CMakeFiles/test_support.dir/fakes/fake_call_log.c.o
[8/61] Building C object CMakeFiles/test_support.dir/fakes/fake_gpio_backend.c.o
[9/61] Building C object CMakeFiles/test_support.dir/fakes/fake_wifi_backend.c.o
[10/61] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[11/61] Building C object CMakeFiles/test_support.dir/fakes/fake_http_backend.c.o
[12/61] Building C object CMakeFiles/test_support.dir/fakes/fake_fs_backend.c.o
[13/61] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[14/61] Building C object CMakeFiles/test_support.dir/support/test_temp_dir.c.o
[15/61] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[16/61] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[17/61] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[18/61] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[19/61] Building C object CMakeFiles/storage_set_tree_tests.dir/test_storage_set_tree.c.o
[20/61] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[21/61] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction_replace.c.o
[22/61] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[23/61] Building C object CMakeFiles/storage_transaction_tests.dir/test_storage_transactions.c.o
[24/61] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[25/61] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_keymap_us.c.o
[26/61] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_parser.c.o
[27/61] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[28/61] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[29/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[30/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[31/61] Building C object CMakeFiles/storage_package_replace_tests.dir/test_storage_package_replace.c.o
[32/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[33/61] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_set_tree.c.o
[34/61] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[35/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[36/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[37/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[38/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[39/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[40/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction_replace.c.o
[41/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[42/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[43/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[44/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[45/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[46/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[47/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_set_operations.c.o
[48/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[49/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[50/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[51/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_progress.c.o
[52/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_keymap_us.c.o
[53/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_parser.c.o
[54/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[55/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_set_tree.c.o
[56/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_package_replace.c.o
[57/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_package.c.o
[58/61] Linking C static library libtest_support.a
[59/61] Linking C executable storage_set_tree_tests
[60/61] Linking C executable storage_transaction_tests
[61/61] Linking C executable storage_package_replace_tests
package replacement stress iteration 1
test failure at /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_storage_package_replace.c:231: (uint64_t)(int64_t)(storage_package_replace_set(&id, 3U, PACKAGE, sizeof(PACKAGE) - 1U, &committed)) == (uint64_t)(int64_t)(APP_ERROR_NONE); expected=0, actual=1
```
