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
-- Configuring done (0.3s)
-- Generating done (0.1s)
-- Build files have been written to: /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/build/phase18-3-package-replace
[1/61] Building C object CMakeFiles/test_support.dir/fakes/fake_clock.c.o
[2/61] Building C object CMakeFiles/test_support.dir/fakes/fake_random.c.o
[3/61] Building C object CMakeFiles/test_support.dir/support/test_assert.c.o
[4/61] Building C object CMakeFiles/test_support.dir/fakes/fake_call_log.c.o
[5/61] Building C object CMakeFiles/test_support.dir/support/test_memory.c.o
[6/61] Building C object CMakeFiles/test_support.dir/support/test_temp_dir.c.o
[7/61] Building C object CMakeFiles/test_support.dir/fakes/fake_usb_backend.c.o
[8/61] Building C object CMakeFiles/test_support.dir/fakes/fake_freertos.c.o
[9/61] Building C object CMakeFiles/test_support.dir/fakes/fake_gpio_backend.c.o
[10/61] Building C object CMakeFiles/test_support.dir/fakes/fake_wifi_backend.c.o
[11/61] Building C object CMakeFiles/test_support.dir/fakes/fake_http_backend.c.o
[12/61] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[13/61] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[14/61] Building C object CMakeFiles/test_support.dir/fakes/fake_fs_backend.c.o
[15/61] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[16/61] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[17/61] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[18/61] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[19/61] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[20/61] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction_replace.c.o
[21/61] Building C object CMakeFiles/storage_transaction_tests.dir/test_storage_transactions.c.o
[22/61] Building C object CMakeFiles/storage_set_tree_tests.dir/test_storage_set_tree.c.o
[23/61] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[24/61] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_keymap_us.c.o
[25/61] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[26/61] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[27/61] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[28/61] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_parser.c.o
[29/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[30/61] Building C object CMakeFiles/storage_package_replace_tests.dir/test_storage_package_replace.c.o
FAILED: [code=1] CMakeFiles/storage_package_replace_tests.dir/test_storage_package_replace.c.o 
/usr/bin/cc -DSTORAGE_DATA_MOUNT=\"/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/build/phase18-3-package-replace/storage-package-replace-data\" -D_POSIX_C_SOURCE=200809L -I/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/../../firmware/components/macro_model/include -I/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/../../firmware/components/macro_parser/include -I/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/../../firmware/components/storage/include -I/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/../../firmware/components/storage -I/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/support -I/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/fakes -isystem /usr/include/cjson -std=c11 -Wall -Wextra -Werror -Wshadow -Wconversion -Wsign-conversion -Wformat=2 -Wundef -Wdouble-promotion -Wmissing-declarations -Wstrict-prototypes -MD -MT CMakeFiles/storage_package_replace_tests.dir/test_storage_package_replace.c.o -MF CMakeFiles/storage_package_replace_tests.dir/test_storage_package_replace.c.o.d -o CMakeFiles/storage_package_replace_tests.dir/test_storage_package_replace.c.o -c /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_storage_package_replace.c
In file included from /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_storage_package_replace.c:22:
/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_storage_package_replace.c: In function ‘test_valid_replace_commits_complete_tree’:
/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_storage_package_replace.c:254:46: error: passing argument 1 of ‘storage_procedure_read’ from incompatible pointer type [-Werror=incompatible-pointer-types]
  254 |                       storage_procedure_read(&procedure_identity, &procedure));
      |                                              ^~~~~~~~~~~~~~~~~~~
      |                                              |
      |                                              const storage_procedure_identity_t *
/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/support/test_assert.h:39:50: note: in definition of macro ‘TEST_CHECK_EQ_U64’
   39 |         const uint64_t test_actual_ = (uint64_t)(actual_value);                        \
      |                                                  ^~~~~~~~~~~~
/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_storage_package_replace.c:253:5: note: in expansion of macro ‘TEST_CHECK_EQ_INT’
  253 |     TEST_CHECK_EQ_INT(APP_ERROR_NONE,
      |     ^~~~~~~~~~~~~~~~~
In file included from /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_storage_package_replace.c:19:
/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/../../firmware/components/storage/include/storage_repository.h:100:59: note: expected ‘const app_uuid_t *’ but argument is of type ‘const storage_procedure_identity_t *’
  100 | app_error_code_t storage_procedure_read(const app_uuid_t *set_id, const app_uuid_t *procedure_id,
      |                                         ~~~~~~~~~~~~~~~~~~^~~~~~
/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_storage_package_replace.c:254:67: error: passing argument 2 of ‘storage_procedure_read’ from incompatible pointer type [-Werror=incompatible-pointer-types]
  254 |                       storage_procedure_read(&procedure_identity, &procedure));
      |                                                                   ^~~~~~~~~~
      |                                                                   |
      |                                                                   procedure_t *
/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/support/test_assert.h:39:50: note: in definition of macro ‘TEST_CHECK_EQ_U64’
   39 |         const uint64_t test_actual_ = (uint64_t)(actual_value);                        \
      |                                                  ^~~~~~~~~~~~
/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_storage_package_replace.c:253:5: note: in expansion of macro ‘TEST_CHECK_EQ_INT’
  253 |     TEST_CHECK_EQ_INT(APP_ERROR_NONE,
      |     ^~~~~~~~~~~~~~~~~
/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/../../firmware/components/storage/include/storage_repository.h:100:85: note: expected ‘const app_uuid_t *’ but argument is of type ‘procedure_t *’
  100 | app_error_code_t storage_procedure_read(const app_uuid_t *set_id, const app_uuid_t *procedure_id,
      |                                                                   ~~~~~~~~~~~~~~~~~~^~~~~~~~~~~~
/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_storage_package_replace.c:254:23: error: too few arguments to function ‘storage_procedure_read’
  254 |                       storage_procedure_read(&procedure_identity, &procedure));
      |                       ^~~~~~~~~~~~~~~~~~~~~~
/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/support/test_assert.h:39:50: note: in definition of macro ‘TEST_CHECK_EQ_U64’
   39 |         const uint64_t test_actual_ = (uint64_t)(actual_value);                        \
      |                                                  ^~~~~~~~~~~~
/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_storage_package_replace.c:253:5: note: in expansion of macro ‘TEST_CHECK_EQ_INT’
  253 |     TEST_CHECK_EQ_INT(APP_ERROR_NONE,
      |     ^~~~~~~~~~~~~~~~~
/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/../../firmware/components/storage/include/storage_repository.h:100:18: note: declared here
  100 | app_error_code_t storage_procedure_read(const app_uuid_t *set_id, const app_uuid_t *procedure_id,
      |                  ^~~~~~~~~~~~~~~~~~~~~~
cc1: all warnings being treated as errors
[31/61] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[32/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[33/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[34/61] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_set_tree.c.o
[35/61] Linking C static library libtest_support.a
ninja: build stopped: subcommand failed.
```
