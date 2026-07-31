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
-- Configuring done (1.5s)
-- Generating done (0.1s)
-- Build files have been written to: /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/build/phase18-3-package-replace
[1/61] Building C object CMakeFiles/test_support.dir/fakes/fake_random.c.o
[2/61] Building C object CMakeFiles/test_support.dir/fakes/fake_clock.c.o
[3/61] Building C object CMakeFiles/test_support.dir/fakes/fake_call_log.c.o
[4/61] Building C object CMakeFiles/test_support.dir/support/test_assert.c.o
[5/61] Building C object CMakeFiles/test_support.dir/support/test_memory.c.o
[6/61] Building C object CMakeFiles/test_support.dir/fakes/fake_freertos.c.o
[7/61] Building C object CMakeFiles/test_support.dir/fakes/fake_usb_backend.c.o
[8/61] Building C object CMakeFiles/test_support.dir/fakes/fake_wifi_backend.c.o
[9/61] Building C object CMakeFiles/test_support.dir/fakes/fake_gpio_backend.c.o
[10/61] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[11/61] Building C object CMakeFiles/test_support.dir/fakes/fake_http_backend.c.o
[12/61] Building C object CMakeFiles/test_support.dir/support/test_temp_dir.c.o
[13/61] Building C object CMakeFiles/test_support.dir/fakes/fake_fs_backend.c.o
[14/61] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[15/61] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[16/61] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[17/61] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[18/61] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[19/61] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[20/61] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction_replace.c.o
[21/61] Building C object CMakeFiles/storage_set_tree_tests.dir/test_storage_set_tree.c.o
[22/61] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[23/61] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[24/61] Building C object CMakeFiles/storage_transaction_tests.dir/test_storage_transactions.c.o
[25/61] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_keymap_us.c.o
[26/61] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_parser.c.o
[27/61] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[28/61] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[29/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[30/61] Building C object CMakeFiles/storage_package_replace_tests.dir/test_storage_package_replace.c.o
[31/61] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_set_tree.c.o
[32/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[33/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[34/61] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[35/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[36/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[37/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[38/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction_replace.c.o
[39/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[40/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[41/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[42/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[43/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[44/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[45/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[46/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[47/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[48/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_set_operations.c.o
[49/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_progress.c.o
[50/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_parser.c.o
[51/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_keymap_us.c.o
[52/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[53/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[54/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[55/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_package.c.o
FAILED: [code=1] CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_package.c.o 
/usr/bin/cc -DSTORAGE_DATA_MOUNT=\"/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/build/phase18-3-package-replace/storage-package-replace-data\" -D_POSIX_C_SOURCE=200809L -I/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/../../firmware/components/macro_model/include -I/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/../../firmware/components/macro_parser/include -I/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/../../firmware/components/storage/include -I/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/../../firmware/components/storage -I/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/support -I/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/fakes -isystem /usr/include/cjson -std=c11 -Wall -Wextra -Werror -Wshadow -Wconversion -Wsign-conversion -Wformat=2 -Wundef -Wdouble-promotion -Wmissing-declarations -Wstrict-prototypes -MD -MT CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_package.c.o -MF CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_package.c.o.d -o CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_package.c.o -c /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_package.c
In file included from /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_package.c:1:
/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/../../firmware/components/storage/include/storage_package.h:32:61: error: unknown type name ‘macro_set_t’
   32 |                                              size_t length, macro_set_t *out_set);
      |                                                             ^~~~~~~~~~~
[56/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_package_replace.c.o
FAILED: [code=1] CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_package_replace.c.o 
/usr/bin/cc -DSTORAGE_DATA_MOUNT=\"/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/build/phase18-3-package-replace/storage-package-replace-data\" -D_POSIX_C_SOURCE=200809L -I/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/../../firmware/components/macro_model/include -I/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/../../firmware/components/macro_parser/include -I/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/../../firmware/components/storage/include -I/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/../../firmware/components/storage -I/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/support -I/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/fakes -isystem /usr/include/cjson -std=c11 -Wall -Wextra -Werror -Wshadow -Wconversion -Wsign-conversion -Wformat=2 -Wundef -Wdouble-promotion -Wmissing-declarations -Wstrict-prototypes -MD -MT CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_package_replace.c.o -MF CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_package_replace.c.o.d -o CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_package_replace.c.o -c /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_package_replace.c
In file included from /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_package_replace.c:1:
/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/../../firmware/components/storage/include/storage_package.h:32:61: error: unknown type name ‘macro_set_t’
   32 |                                              size_t length, macro_set_t *out_set);
      |                                                             ^~~~~~~~~~~
/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_package_replace.c:588:18: error: no previous declaration for ‘storage_package_replace_set’ [-Werror=missing-declarations]
  588 | app_error_code_t storage_package_replace_set(const app_uuid_t *target_set_id,
      |                  ^~~~~~~~~~~~~~~~~~~~~~~~~~~
cc1: all warnings being treated as errors
[57/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_set_tree.c.o
[58/61] Linking C static library libtest_support.a
ninja: build stopped: subcommand failed.
```
