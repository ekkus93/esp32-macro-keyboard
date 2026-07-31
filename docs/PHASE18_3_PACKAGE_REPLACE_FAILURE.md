# Phase 18.3 package replacement failure

The production package replacement validation failed.

## Validation log

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
[2/61] Building C object CMakeFiles/test_support.dir/fakes/fake_call_log.c.o
[3/61] Building C object CMakeFiles/test_support.dir/support/test_memory.c.o
[4/61] Building C object CMakeFiles/test_support.dir/fakes/fake_random.c.o
[5/61] Building C object CMakeFiles/test_support.dir/support/test_assert.c.o
[6/61] Building C object CMakeFiles/test_support.dir/support/test_temp_dir.c.o
[7/61] Building C object CMakeFiles/test_support.dir/fakes/fake_wifi_backend.c.o
[8/61] Building C object CMakeFiles/test_support.dir/fakes/fake_freertos.c.o
[9/61] Building C object CMakeFiles/test_support.dir/fakes/fake_usb_backend.c.o
[10/61] Building C object CMakeFiles/test_support.dir/fakes/fake_gpio_backend.c.o
[11/61] Building C object CMakeFiles/test_support.dir/fakes/fake_http_backend.c.o
[12/61] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[13/61] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[14/61] Building C object CMakeFiles/test_support.dir/fakes/fake_fs_backend.c.o
[15/61] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[16/61] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[17/61] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[18/61] Building C object CMakeFiles/storage_transaction_tests.dir/test_storage_transactions.c.o
[19/61] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction_replace.c.o
[20/61] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[21/61] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[22/61] Building C object CMakeFiles/storage_set_tree_tests.dir/test_storage_set_tree.c.o
[23/61] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_keymap_us.c.o
[24/61] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[25/61] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[26/61] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_parser.c.o
[27/61] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[28/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
[29/61] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[30/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
[31/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
[32/61] Building C object CMakeFiles/storage_package_replace_tests.dir/test_storage_package_replace.c.o
[33/61] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[34/61] Building C object CMakeFiles/storage_set_tree_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_set_tree.c.o
[35/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
[36/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
[37/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
[38/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
[39/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
[40/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction_replace.c.o
[41/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
[42/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
[43/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
[44/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
[45/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
[46/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
[47/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
[48/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_set_operations.c.o
[49/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_progress.c.o
[50/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
[51/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_keymap_us.c.o
[52/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
[53/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_parser.c.o
[54/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
[55/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_package_replace.c.o
[56/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_set_tree.c.o
[57/61] Building C object CMakeFiles/storage_package_replace_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_package.c.o
[58/61] Linking C static library libtest_support.a
[59/61] Linking C executable storage_package_replace_tests
[60/61] Linking C executable storage_transaction_tests
[61/61] Linking C executable storage_set_tree_tests
Internal ctest changing into directory: /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/build/phase18-3-package-replace
Test project /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/build/phase18-3-package-replace
    Start 21: storage_transaction
1/3 Test #21: storage_transaction ..............   Passed    0.11 sec
    Start 22: storage_set_tree
2/3 Test #22: storage_set_tree .................   Passed    0.02 sec
    Start 25: storage_package_replace
3/3 Test #25: storage_package_replace ..........***Failed    0.02 sec
PHASE18_3_DIAGNOSTIC validate=0
PHASE18_3_DIAGNOSTIC open=0
PHASE18_3_DIAGNOSTIC validate=1
PHASE18_3_DIAGNOSTIC validate=0
PHASE18_3_DIAGNOSTIC open=0
PHASE18_3_DIAGNOSTIC current=0
PHASE18_3_DIAGNOSTIC replace=3
PHASE18_3_DIAGNOSTIC validate=0
PHASE18_3_DIAGNOSTIC open=0
PHASE18_3_DIAGNOSTIC current=0
PHASE18_3_DIAGNOSTIC globals=3
PHASE18_3_DIAGNOSTIC replace=3
PHASE18_3_DIAGNOSTIC validate=0
PHASE18_3_DIAGNOSTIC open=0
PHASE18_3_DIAGNOSTIC current=0
PHASE18_3_DIAGNOSTIC globals=3
PHASE18_3_DIAGNOSTIC replace=3
PHASE18_3_DIAGNOSTIC validate=0
PHASE18_3_DIAGNOSTIC open=0
PHASE18_3_DIAGNOSTIC current=0
PHASE18_3_DIAGNOSTIC globals=0
PHASE18_3_DIAGNOSTIC uuid=0
PHASE18_3_DIAGNOSTIC manifest-init=0
PHASE18_3_DIAGNOSTIC manifest-prepared=0
PHASE18_3_DIAGNOSTIC staging-create=0
PHASE18_3_DIAGNOSTIC staging-materialize=1
PHASE18_3_DIAGNOSTIC replace=1
test failure at /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_storage_package_replace.c:231: (uint64_t)(int64_t)(storage_package_replace_set(&id, 3U, PACKAGE, sizeof(PACKAGE) - 1U, &committed)) == (uint64_t)(int64_t)(APP_ERROR_NONE); expected=0, actual=1


67% tests passed, 1 tests failed out of 3

Label Time Summary:
storage    =   0.15 sec*proc (3 tests)

Total Test time (real) =   0.16 sec

The following tests FAILED:
	 25 - storage_package_replace (Failed)                  storage
Errors while running CTest
```

## Test fixture around the failure

```c
     1	#define _POSIX_C_SOURCE 200809L
     2	
     3	#include <dirent.h>
     4	#include <errno.h>
     5	#include <stdbool.h>
     6	#include <stdio.h>
     7	#include <stdlib.h>
     8	#include <string.h>
     9	#include <sys/stat.h>
    10	
    11	#include "app_error.h"
    12	#include "app_uuid.h"
    13	#include "cJSON.h"
    14	#include "macro_limits.h"
    15	#include "macro_model.h"
    16	#include "storage.h"
    17	#include "storage_object_json.h"
    18	#include "storage_package.h"
    19	#include "storage_repository.h"
    20	#include "storage_repository_internal.h"
    21	#include "storage_repository_lock.h"
    22	#include "test_assert.h"
    23	
    24	#define SET_ID "11111111-1111-4111-8111-111111111111"
    25	#define OTHER_SET_ID "12121212-1212-4212-8212-121212121212"
    26	#define LOCAL_MACRO_ID "22222222-2222-4222-8222-222222222222"
    27	#define GLOBAL_MACRO_ID "23232323-2323-4232-8232-232323232323"
    28	#define PROCEDURE_ID "33333333-3333-4333-8333-333333333333"
    29	#define STEP_ID "44444444-4444-4444-8444-444444444444"
    30	
    31	static const char PACKAGE[] =
    32	    "{\"schema_version\":1,\"package_type\":\"set\",\"sets\":["
    33	    "{\"schema_version\":1,\"id\":\"" SET_ID
    34	    "\",\"revision\":7,\"name\":\"Replacement\",\"description\":\"new\","
    35	    "\"manufacturer\":\"Vendor\",\"model\":\"Model\",\"board\":\"Board\","
    36	    "\"keyboard_layout\":\"en-US\",\"sort_order\":0}],\"macros\":["
    37	    "{\"schema_version\":1,\"id\":\"" LOCAL_MACRO_ID
    38	    "\",\"revision\":4,\"scope\":\"set\",\"name\":\"Local\",\"source\":\"a\","
    39	    "\"favorite\":false,\"key_press_ms\":8,\"inter_key_ms\":15,\"set_id\":\"" SET_ID
    40	    "\"}],\"global_macros\":["
    41	    "{\"schema_version\":1,\"id\":\"" GLOBAL_MACRO_ID
    42	    "\",\"revision\":2,\"scope\":\"global\",\"name\":\"Global\",\"source\":\"b\","
    43	    "\"favorite\":false,\"key_press_ms\":8,\"inter_key_ms\":15}],\"procedures\":["
    44	    "{\"schema_version\":1,\"id\":\"" PROCEDURE_ID
    45	    "\",\"revision\":3,\"set_id\":\"" SET_ID
    46	    "\",\"name\":\"Procedure\",\"description\":\"\",\"steps\":[{\"id\":\"" STEP_ID
    47	    "\",\"type\":\"macro\",\"title\":\"Step\",\"macro_id\":\"" GLOBAL_MACRO_ID
    48	    "\",\"required\":true,\"auto_complete_on_success\":false}],\"sort_order\":0}],"
    49	    "\"progress\":[{\"schema_version\":1,\"set_id\":\"" SET_ID
    50	    "\",\"procedure_id\":\"" PROCEDURE_ID
    51	    "\",\"procedure_revision\":3,\"current_step_id\":\"" STEP_ID
    52	    "\",\"completed_step_ids\":[],\"skipped_step_ids\":[]}]}";
    53	
    54	static app_uuid_t parse_id(const char *value) {
    55	    app_uuid_t id = {0};
    56	    TEST_CHECK_EQ_INT(APP_ERROR_NONE, app_uuid_parse(value, &id));
    57	    return id;
    58	}
    59	
    60	static void make_directory(const char *path) {
    61	    TEST_CHECK(mkdir(path, 0700) == 0 || errno == EEXIST);
    62	}
    63	
    64	static void join_path(char *output, size_t output_size, const char *parent, const char *name) {
    65	    const int written = snprintf(output, output_size, "%s/%s", parent, name);
    66	    TEST_CHECK(written > 0);
    67	    TEST_CHECK((size_t)written < output_size);
    68	}
    69	
    70	static void reset_storage(void) {
    71	    storage_repository_lock_deinit();
    72	    char command[APP_PATH_MAX_BYTES + 32U];
    73	    const int written = snprintf(command, sizeof(command), "rm -rf '%s'", STORAGE_DATA_MOUNT);
    74	    TEST_CHECK(written > 0);
    75	    TEST_CHECK((size_t)written < sizeof(command));
    76	    TEST_CHECK_EQ_INT(0, system(command));
    77	    make_directory(STORAGE_DATA_MOUNT);
    78	    static const char *const roots[] = {
    79	        "transactions", "staging", "sets", "trash", "global", "quarantine",
    80	    };
    81	    char path[APP_PATH_MAX_BYTES];
    82	    for (size_t index = 0U; index < sizeof(roots) / sizeof(roots[0]); ++index) {
    83	        join_path(path, sizeof(path), STORAGE_DATA_MOUNT, roots[index]);
    84	        make_directory(path);
    85	    }
    86	    join_path(path, sizeof(path), STORAGE_DATA_MOUNT "/global", "macros");
    87	    make_directory(path);
    88	    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_repository_lock_init());
    89	}
    90	
    91	static void write_set_file(const char *set_path, const macro_set_t *set) {
    92	    char *json = NULL;
    93	    size_t length = 0U;
    94	    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
    95	                      storage_repository_serialize_set_json(set, &json, &length));
    96	    char path[APP_PATH_MAX_BYTES];
    97	    join_path(path, sizeof(path), set_path, "set.json");
    98	    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_atomic_write(path, json, length, true));
    99	    cJSON_free(json);
   100	}
   101	
   102	static void create_current_set(void) {
   103	    const app_uuid_t id = parse_id(SET_ID);
   104	    macro_set_t set = {
   105	        .schema_version = APP_SCHEMA_VERSION,
   106	        .id = id,
   107	        .revision = 3U,
   108	        .sort_order = 0,
   109	    };
   110	    memcpy(set.name, "Current", sizeof("Current"));
   111	    memcpy(set.keyboard_layout, "en-US", sizeof("en-US"));
   112	    char set_path[APP_PATH_MAX_BYTES];
   113	    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_make_set_path(&id, set_path, sizeof(set_path)));
   114	    make_directory(set_path);
   115	    static const char *const children[] = {"macros", "procedures", "progress"};
   116	    char path[APP_PATH_MAX_BYTES];
   117	    for (size_t index = 0U; index < sizeof(children) / sizeof(children[0]); ++index) {
   118	        join_path(path, sizeof(path), set_path, children[index]);
   119	        make_directory(path);
   120	    }
   121	    write_set_file(set_path, &set);
   122	    static const char empty_order[] = "{\"schema_version\":1,\"ids\":[]}";
   123	    join_path(path, sizeof(path), set_path, "macro-order.json");
   124	    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
   125	                      storage_atomic_write(path, empty_order, strlen(empty_order), true));
   126	    join_path(path, sizeof(path), set_path, "procedure-order.json");
   127	    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
   128	                      storage_atomic_write(path, empty_order, strlen(empty_order), true));
   129	    storage_set_index_t index = {.ids = {id}, .count = 1U};
   130	    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_repository_write_index(&index));
   131	}
   132	
   133	static void create_global_macro(const char *source) {
   134	    macro_t macro = {
   135	        .schema_version = APP_SCHEMA_VERSION,
   136	        .id = parse_id(GLOBAL_MACRO_ID),
   137	        .revision = 2U,
   138	        .scope = MACRO_SCOPE_GLOBAL,
   139	        .has_set_id = false,
   140	        .source = (char *)source,
   141	        .source_length = strlen(source),
   142	        .favorite = false,
   143	        .key_press_ms = 8U,
   144	        .inter_key_ms = 15U,
   145	    };
   146	    memcpy(macro.name, "Global", sizeof("Global"));
   147	    char *json = NULL;
   148	    size_t length = 0U;
   149	    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
   150	                      storage_repository_serialize_macro_json(&macro, &json, &length));
   151	    char path[APP_PATH_MAX_BYTES];
   152	    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
   153	                      storage_make_global_macro_path(&macro.id, path, sizeof(path)));
   154	    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_atomic_write(path, json, length, true));
   155	    cJSON_free(json);
   156	}
   157	
   158	static bool directory_empty(const char *path) {
   159	    DIR *directory = opendir(path);
   160	    TEST_CHECK(directory != NULL);
   161	    bool empty = true;
   162	    while (true) {
   163	        errno = 0;
   164	        const struct dirent *entry = readdir(directory);
   165	        if (entry == NULL) {
   166	            TEST_CHECK_EQ_INT(0, errno);
   167	            break;
   168	        }
   169	        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
   170	            empty = false;
   171	            break;
   172	        }
   173	    }
   174	    TEST_CHECK_EQ_INT(0, closedir(directory));
   175	    return empty;
   176	}
   177	
   178	static void prepare_valid_state(void) {
   179	    reset_storage();
   180	    create_current_set();
   181	    create_global_macro("b");
   182	}
   183	
   184	static void assert_current_revision(uint32_t revision) {
   185	    macro_set_t set = {0};
   186	    const app_uuid_t id = parse_id(SET_ID);
   187	    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_set_read(&id, &set));
   188	    TEST_CHECK_EQ_U64(revision, set.revision);
   189	}
   190	
   191	static void test_invalid_and_conflict_inputs_do_not_mutate(void) {
   192	    prepare_valid_state();
   193	    macro_set_t committed = {0};
   194	    const app_uuid_t id = parse_id(SET_ID);
   195	    const app_uuid_t other = parse_id(OTHER_SET_ID);
   196	    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
   197	                      storage_package_replace_set(NULL, 3U, PACKAGE,
   198	                                                  sizeof(PACKAGE) - 1U, &committed));
   199	    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
   200	                      storage_package_replace_set(&other, 3U, PACKAGE,
   201	                                                  sizeof(PACKAGE) - 1U, &committed));
   202	    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
   203	                      storage_package_replace_set(&id, 3U, "{}", 2U, &committed));
   204	    TEST_CHECK_EQ_INT(APP_ERROR_CONFLICT,
   205	                      storage_package_replace_set(&id, 2U, PACKAGE,
   206	                                                  sizeof(PACKAGE) - 1U, &committed));
   207	    assert_current_revision(3U);
   208	}
   209	
   210	static void test_global_dependency_must_match(void) {
   211	    reset_storage();
   212	    create_current_set();
   213	    macro_set_t committed = {0};
   214	    const app_uuid_t id = parse_id(SET_ID);
   215	    TEST_CHECK_EQ_INT(APP_ERROR_CONFLICT,
   216	                      storage_package_replace_set(&id, 3U, PACKAGE,
   217	                                                  sizeof(PACKAGE) - 1U, &committed));
   218	    assert_current_revision(3U);
   219	
   220	    create_global_macro("c");
   221	    TEST_CHECK_EQ_INT(APP_ERROR_CONFLICT,
   222	                      storage_package_replace_set(&id, 3U, PACKAGE,
   223	                                                  sizeof(PACKAGE) - 1U, &committed));
   224	    assert_current_revision(3U);
   225	}
   226	
   227	static void test_valid_replace_commits_complete_tree(void) {
   228	    prepare_valid_state();
   229	    macro_set_t committed = {0};
   230	    const app_uuid_t id = parse_id(SET_ID);
   231	    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
   232	                      storage_package_replace_set(&id, 3U, PACKAGE,
   233	                                                  sizeof(PACKAGE) - 1U, &committed));
   234	    TEST_CHECK_EQ_U64(7U, committed.revision);
   235	    TEST_CHECK_EQ_STRING("Replacement", committed.name);
   236	    assert_current_revision(7U);
   237	
   238	    storage_macro_location_t location = {
   239	        .scope = MACRO_SCOPE_SET,
   240	        .set_id = id,
   241	    };
   242	    macro_t macro = {0};
   243	    const app_uuid_t macro_id = parse_id(LOCAL_MACRO_ID);
   244	    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_macro_read(&location, &macro_id, &macro));
   245	    TEST_CHECK_EQ_U64(4U, macro.revision);
   246	    macro_model_free_macro(&macro);
   247	
   248	    procedure_t procedure = {0};
   249	    const storage_procedure_identity_t procedure_identity = {
   250	        .set_id = id,
   251	        .procedure_id = parse_id(PROCEDURE_ID),
   252	    };
   253	    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
   254	                      storage_procedure_read(&procedure_identity.set_id, &procedure_identity.procedure_id, &procedure));
   255	    TEST_CHECK_EQ_U64(3U, procedure.revision);
   256	    macro_model_free_procedure(&procedure);
   257	
   258	    storage_progress_snapshot_t progress = {0};
   259	    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
   260	                      storage_progress_read(&procedure_identity, &progress));
   261	    TEST_CHECK_EQ_U64(STORAGE_PROGRESS_STATUS_CURRENT, progress.status);
   262	
   263	    TEST_CHECK(directory_empty(STORAGE_DATA_MOUNT "/transactions"));
   264	    TEST_CHECK(directory_empty(STORAGE_DATA_MOUNT "/staging"));
   265	    TEST_CHECK(directory_empty(STORAGE_DATA_MOUNT "/trash"));
   266	    storage_set_index_t index = {0};
   267	    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_repository_load_index(&index));
   268	    TEST_CHECK_EQ_U64(1U, index.count);
   269	    TEST_CHECK(app_uuid_equal(&index.ids[0], &id));
   270	}
   271	
   272	int main(void) {
   273	    test_invalid_and_conflict_inputs_do_not_mutate();
   274	    test_global_dependency_must_match();
   275	    test_valid_replace_commits_complete_tree();
   276	    reset_storage();
   277	    storage_repository_lock_deinit();
   278	    puts("storage package replace tests passed");
   279	    return EXIT_SUCCESS;
   280	}
```

## Replacement service entry and validators

```c
     1	#include "storage_package.h"
     2	
     3	#include <errno.h>
     4	#include <stdbool.h>
     5	#include <stddef.h>
     6	#include <stdint.h>
     7	#include <stdio.h>
     8	#include <stdlib.h>
     9	#include <string.h>
    10	
    11	#include "app_error.h"
    12	#include "app_uuid.h"
    13	#include "cJSON.h"
    14	#include "macro_limits.h"
    15	#include "macro_model.h"
    16	#include "storage.h"
    17	#include "storage_atomic_internal.h"
    18	#include "storage_fs_ops.h"
    19	#include "storage_object_json.h"
    20	#include "storage_repository.h"
    21	#include "storage_repository_internal.h"
    22	#include "storage_repository_lock.h"
    23	#include "storage_repository_macros_internal.h"
    24	#include "storage_repository_sets_internal.h"
    25	#include "storage_set_tree_internal.h"
    26	#include "storage_transaction_internal.h"
    27	
    28	#define PACKAGE_REPLACE_ARRAY_COUNT 5U
    29	
    30	typedef enum {
    31	    PACKAGE_REPLACE_SETS = 0,
    32	    PACKAGE_REPLACE_MACROS,
    33	    PACKAGE_REPLACE_GLOBAL_MACROS,
    34	    PACKAGE_REPLACE_PROCEDURES,
    35	    PACKAGE_REPLACE_PROGRESS,
    36	} package_replace_array_t;
    37	
    38	typedef struct {
    39	    cJSON *root;
    40	    const cJSON *arrays[PACKAGE_REPLACE_ARRAY_COUNT];
    41	    macro_set_t replacement;
    42	} package_replace_document_t;
    43	
    44	static app_error_code_t package_uuid_generate(void *context, app_uuid_t *out_uuid) {
    45	    (void)context;
    46	    return app_uuid_generate(out_uuid);
    47	}
    48	
    49	static app_error_code_t package_index_presence(void *context, const app_uuid_t *set_id,
    50	                                               bool should_be_present) {
    51	    (void)context;
    52	    return storage_repository_set_index_presence(set_id, should_be_present);
    53	}
    54	
    55	static app_error_code_t package_validate_tree(void *context, const char *path,
    56	                                              const app_uuid_t *set_id,
    57	                                              uint32_t expected_revision) {
    58	    (void)context;
    59	    return storage_set_tree_validate(path, set_id, expected_revision);
    60	}
    61	
    62	static app_error_code_t package_remove_tree(void *context, const char *path) {
    63	    (void)context;
    64	    return storage_repository_remove_tree(path);
    65	}
    66	
    67	static app_error_code_t map_error_number(int error_number) {
    68	    if (error_number == ENOSPC) {
    69	        return APP_ERROR_STORAGE_FULL;
    70	    }
    71	    if (error_number == ENOENT) {
    72	        return APP_ERROR_NOT_FOUND;
    73	    }
    74	    return APP_ERROR_IO;
    75	}
    76	
    77	static app_error_code_t join_path(const char *parent, const char *name, char *output,
    78	                                  size_t output_size) {
    79	    if (parent == NULL || name == NULL || output == NULL || output_size == 0U) {
    80	        return APP_ERROR_INVALID_ARGUMENT;
    81	    }
    82	    const int written = snprintf(output, output_size, "%s/%s", parent, name);
    83	    if (written < 0 || (size_t)written >= output_size) {
    84	        output[0] = '\0';
    85	        return APP_ERROR_INVALID_ARGUMENT;
    86	    }
    87	    return APP_ERROR_NONE;
    88	}
    89	
    90	static app_error_code_t sync_parent(const char *path) {
    91	    return storage_fs_sync_parent_path(NULL, path) == 0 ? APP_ERROR_NONE
    92	                                                        : map_error_number(errno);
    93	}
    94	
    95	static app_error_code_t make_directory(const char *path) {
    96	    app_error_code_t result = storage_repository_make_directory(path);
    97	    if (result == APP_ERROR_NONE) {
    98	        result = sync_parent(path);
    99	    }
   100	    return result;
   101	}
   102	
   103	static app_error_code_t node_json(const cJSON *node, char **out_json, size_t *out_length) {
   104	    if (node == NULL || out_json == NULL || out_length == NULL) {
   105	        return APP_ERROR_INVALID_ARGUMENT;
   106	    }
   107	    *out_json = NULL;
   108	    *out_length = 0U;
   109	    char *json = cJSON_PrintUnformatted(node);
   110	    if (json == NULL) {
   111	        return APP_ERROR_INTERNAL;
   112	    }
   113	    *out_length = strlen(json);
   114	    *out_json = json;
   115	    return APP_ERROR_NONE;
   116	}
   117	
   118	static app_error_code_t parse_set_node(const cJSON *node, macro_set_t *out_set) {
   119	    char *json = NULL;
   120	    size_t length = 0U;
   121	    app_error_code_t result = node_json(node, &json, &length);
   122	    if (result == APP_ERROR_NONE) {
   123	        result = storage_repository_parse_set_json(json, length, out_set);
   124	    }
   125	    cJSON_free(json);
   126	    return result == APP_ERROR_STORAGE_CORRUPT ? APP_ERROR_INVALID_ARGUMENT : result;
   127	}
   128	
   129	static app_error_code_t parse_macro_node(const cJSON *node, macro_t *out_macro) {
   130	    char *json = NULL;
   131	    size_t length = 0U;
   132	    app_error_code_t result = node_json(node, &json, &length);
   133	    if (result == APP_ERROR_NONE) {
   134	        result = storage_repository_parse_macro_json(json, length, out_macro);
   135	    }
   136	    cJSON_free(json);
   137	    return result == APP_ERROR_STORAGE_CORRUPT ? APP_ERROR_INVALID_ARGUMENT : result;
   138	}
   139	
   140	static app_error_code_t parse_procedure_node(const cJSON *node, procedure_t *out_procedure) {
   141	    char *json = NULL;
   142	    size_t length = 0U;
   143	    app_error_code_t result = node_json(node, &json, &length);
   144	    if (result == APP_ERROR_NONE) {
   145	        result = storage_repository_parse_procedure_json(json, length, out_procedure);
   146	    }
   147	    cJSON_free(json);
   148	    return result == APP_ERROR_STORAGE_CORRUPT ? APP_ERROR_INVALID_ARGUMENT : result;
   149	}
   150	
   151	static app_error_code_t parse_progress_node(const cJSON *node,
   152	                                            procedure_progress_t *out_progress) {
   153	    char *json = NULL;
   154	    size_t length = 0U;
   155	    app_error_code_t result = node_json(node, &json, &length);
   156	    if (result == APP_ERROR_NONE) {
   157	        result = storage_repository_parse_progress_json(json, length, out_progress);
   158	    }
   159	    cJSON_free(json);
   160	    return result == APP_ERROR_STORAGE_CORRUPT ? APP_ERROR_INVALID_ARGUMENT : result;
   161	}
   162	
   163	static void close_document(package_replace_document_t *document) {
   164	    if (document == NULL) {
   165	        return;
   166	    }
   167	    cJSON_Delete(document->root);
   168	    memset(document, 0, sizeof(*document));
   169	}
   170	
   171	static app_error_code_t open_document(const char *data, size_t length,
   172	                                      package_replace_document_t *out_document) {
   173	    memset(out_document, 0, sizeof(*out_document));
   174	    const char *parse_end = NULL;
   175	    cJSON *root = cJSON_ParseWithLengthOpts(data, length, &parse_end, false);
   176	    if (root == NULL || parse_end != data + length || !cJSON_IsObject(root)) {
   177	        cJSON_Delete(root);
   178	        return APP_ERROR_INVALID_ARGUMENT;
   179	    }
   180	    static const char *const names[PACKAGE_REPLACE_ARRAY_COUNT] = {
   181	        [PACKAGE_REPLACE_SETS] = "sets",
   182	        [PACKAGE_REPLACE_MACROS] = "macros",
   183	        [PACKAGE_REPLACE_GLOBAL_MACROS] = "global_macros",
   184	        [PACKAGE_REPLACE_PROCEDURES] = "procedures",
   185	        [PACKAGE_REPLACE_PROGRESS] = "progress",
   186	    };
   187	    for (size_t index = 0U; index < PACKAGE_REPLACE_ARRAY_COUNT; ++index) {
   188	        out_document->arrays[index] = cJSON_GetObjectItemCaseSensitive(root, names[index]);
   189	        if (!cJSON_IsArray(out_document->arrays[index])) {
   190	            cJSON_Delete(root);
   191	            memset(out_document, 0, sizeof(*out_document));
   192	            return APP_ERROR_INVALID_ARGUMENT;
   193	        }
   194	    }
   195	    const cJSON *set_node = cJSON_GetArrayItem(out_document->arrays[PACKAGE_REPLACE_SETS], 0);
   196	    app_error_code_t result = parse_set_node(set_node, &out_document->replacement);
   197	    if (result != APP_ERROR_NONE) {
   198	        cJSON_Delete(root);
   199	        memset(out_document, 0, sizeof(*out_document));
   200	        return result;
   201	    }
   202	    out_document->root = root;
   203	    return APP_ERROR_NONE;
   204	}
   205	
   206	static app_error_code_t canonical_macro_json(const macro_t *macro, char **out_json,
   207	                                             size_t *out_length) {
   208	    return storage_repository_serialize_macro_json(macro, out_json, out_length);
   209	}
   210	
   211	static app_error_code_t verify_global_dependency(const cJSON *node) {
   212	    macro_t package_macro = {0};
   213	    app_error_code_t result = parse_macro_node(node, &package_macro);
   214	    if (result == APP_ERROR_NONE &&
   215	        (package_macro.scope != MACRO_SCOPE_GLOBAL || package_macro.has_set_id)) {
   216	        result = APP_ERROR_INVALID_ARGUMENT;
   217	    }
   218	    macro_t stored = {0};
   219	    if (result == APP_ERROR_NONE) {
   220	        const storage_macro_location_t location = {
   520	        NULL, package_validate_tree, NULL, package_remove_tree, NULL);
   521	}
   522	
   523	static app_error_code_t replace_locked(const app_uuid_t *target_set_id,
   524	                                       uint32_t expected_revision,
   525	                                       package_replace_document_t *document,
   526	                                       macro_set_t *out_set) {
   527	    macro_set_t current = {0};
   528	    app_error_code_t result = storage_set_read_locked(target_set_id, &current);
   529	    fprintf(stderr, "PHASE18_3_DIAGNOSTIC current=%d\n", (int)result);
   530	    if (result == APP_ERROR_NONE && current.revision != expected_revision) {
   531	        result = APP_ERROR_CONFLICT;
   532	    }
   533	    if (result == APP_ERROR_NONE) {
   534	        result = verify_global_dependencies(document->arrays[PACKAGE_REPLACE_GLOBAL_MACROS]);
   535	        fprintf(stderr, "PHASE18_3_DIAGNOSTIC globals=%d\n", (int)result);
   536	    }
   537	    app_uuid_t transaction_id = {0};
   538	    if (result == APP_ERROR_NONE) {
   539	        result = app_uuid_generate(&transaction_id);
   540	        fprintf(stderr, "PHASE18_3_DIAGNOSTIC uuid=%d\n", (int)result);
   541	    }
   542	    char staging[APP_PATH_MAX_BYTES] = {0};
   543	    storage_transaction_manifest_t manifest = {0};
   544	    bool manifest_written = false;
   545	    if (result == APP_ERROR_NONE) {
   546	        const int written = snprintf(staging, sizeof(staging), STORAGE_DATA_MOUNT "/staging/%s",
   547	                                     transaction_id.value);
   548	        result = written >= 0 && (size_t)written < sizeof(staging) ? APP_ERROR_NONE
   549	                                                                   : APP_ERROR_INVALID_ARGUMENT;
   550	    }
   551	    if (result == APP_ERROR_NONE) {
   552	        result = initialize_manifest(&transaction_id, &current, &document->replacement, staging,
   553	                                     &manifest);
   554	        fprintf(stderr, "PHASE18_3_DIAGNOSTIC manifest-init=%d\n", (int)result);
   555	    }
   556	    if (result == APP_ERROR_NONE) {
   557	        result = storage_transaction_write_manifest(&manifest);
   558	        fprintf(stderr, "PHASE18_3_DIAGNOSTIC manifest-prepared=%d\n", (int)result);
   559	        manifest_written = result == APP_ERROR_NONE;
   560	    }
   561	    if (result == APP_ERROR_NONE) {
   562	        result = create_staging(&transaction_id, staging, sizeof(staging));
   563	        fprintf(stderr, "PHASE18_3_DIAGNOSTIC staging-create=%d\n", (int)result);
   564	    }
   565	    if (result == APP_ERROR_NONE) {
   566	        result = materialize_staging(document, staging);
   567	        fprintf(stderr, "PHASE18_3_DIAGNOSTIC staging-materialize=%d\n", (int)result);
   568	    }
   569	    if (result == APP_ERROR_NONE) {
   570	        result = storage_set_tree_validate(staging, target_set_id,
   571	                                           document->replacement.revision);
   572	        fprintf(stderr, "PHASE18_3_DIAGNOSTIC staging-validate=%d\n", (int)result);
   573	    }
   574	    if (result == APP_ERROR_NONE) {
   575	        manifest.phase = STORAGE_TRANSACTION_STAGED;
   576	        result = storage_transaction_write_manifest(&manifest);
   577	        fprintf(stderr, "PHASE18_3_DIAGNOSTIC manifest-staged=%d\n", (int)result);
   578	    }
   579	    if (result == APP_ERROR_NONE) {
   580	        result = recover_replace(&manifest);
   581	        fprintf(stderr, "PHASE18_3_DIAGNOSTIC recover=%d\n", (int)result);
   582	    } else if (manifest_written) {
   583	        const app_error_code_t rollback = recover_replace(&manifest);
   584	        if (rollback != APP_ERROR_NONE) {
   585	            return result;
   586	        }
   587	    }
   588	    if (result == APP_ERROR_NONE) {
   589	        result = storage_set_read_locked(target_set_id, out_set);
   590	    }
   591	    if (result == APP_ERROR_NONE && out_set->revision != document->replacement.revision) {
   592	        memset(out_set, 0, sizeof(*out_set));
   593	        result = APP_ERROR_STORAGE_CORRUPT;
   594	    }
   595	    return result;
   596	}
   597	
   598	app_error_code_t storage_package_replace_set(const app_uuid_t *target_set_id,
   599	                                             uint32_t expected_revision, const char *data,
   600	                                             size_t length, macro_set_t *out_set) {
   601	    if (out_set != NULL) {
   602	        memset(out_set, 0, sizeof(*out_set));
   603	    }
   604	    if (target_set_id == NULL || expected_revision == 0U || data == NULL || length == 0U ||
   605	        out_set == NULL || !app_uuid_is_valid_string(target_set_id->value)) {
   606	        return APP_ERROR_INVALID_ARGUMENT;
   607	    }
   608	    storage_package_summary_t summary = {0};
   609	    app_error_code_t result =
   610	        storage_package_validate(data, length, STORAGE_PACKAGE_KIND_SET, &summary);
   611	    fprintf(stderr, "PHASE18_3_DIAGNOSTIC validate=%d\n", (int)result);
   612	    if (result != APP_ERROR_NONE) {
   613	        return result;
   614	    }
   615	    package_replace_document_t document = {0};
   616	    result = open_document(data, length, &document);
   617	    fprintf(stderr, "PHASE18_3_DIAGNOSTIC open=%d\n", (int)result);
   618	    if (result == APP_ERROR_NONE &&
   619	        !app_uuid_equal(target_set_id, &document.replacement.id)) {
   620	        result = APP_ERROR_INVALID_ARGUMENT;
   621	    }
   622	    if (result == APP_ERROR_NONE) {
   623	        result = storage_repository_lock_take();
   624	    }
   625	    if (result == APP_ERROR_NONE) {
   626	        result = replace_locked(target_set_id, expected_revision, &document, out_set);
   627	        fprintf(stderr, "PHASE18_3_DIAGNOSTIC replace=%d\n", (int)result);
   628	        const app_error_code_t unlock = storage_repository_lock_give();
   629	        if (unlock != APP_ERROR_NONE) {
   630	            memset(out_set, 0, sizeof(*out_set));
   631	            result = APP_ERROR_INTERNAL;
   632	        }
   633	    }
   634	    close_document(&document);
   635	    return result;
   636	}
```
