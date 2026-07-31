# Phase 18.3 normalized generated test

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
   240	        .has_set_id = true,
   241	        .set_id = id,
   242	    };
   243	    macro_t macro = {0};
   244	    const app_uuid_t macro_id = parse_id(LOCAL_MACRO_ID);
   245	    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_macro_read(&location, &macro_id, &macro));
   246	    TEST_CHECK_EQ_U64(4U, macro.revision);
   247	    macro_model_free_macro(&macro);
   248	
   249	    procedure_t procedure = {0};
   250	    const storage_procedure_identity_t procedure_identity = {
   251	        .set_id = id,
   252	        .procedure_id = parse_id(PROCEDURE_ID),
   253	    };
   254	    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
   255	                      storage_procedure_read(&procedure_identity.set_id, &procedure_identity.procedure_id, &procedure));
   256	    TEST_CHECK_EQ_U64(3U, procedure.revision);
   257	    macro_model_free_procedure(&procedure);
   258	
   259	    storage_progress_snapshot_t progress = {0};
   260	    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
   261	                      storage_progress_read(&procedure_identity, &progress));
   262	    TEST_CHECK_EQ_U64(STORAGE_PROGRESS_STATUS_CURRENT, progress.status);
   263	
   264	    TEST_CHECK(directory_empty(STORAGE_DATA_MOUNT "/transactions"));
   265	    TEST_CHECK(directory_empty(STORAGE_DATA_MOUNT "/staging"));
   266	    TEST_CHECK(directory_empty(STORAGE_DATA_MOUNT "/trash"));
   267	    storage_set_index_t index = {0};
   268	    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_repository_load_index(&index));
   269	    TEST_CHECK_EQ_U64(1U, index.count);
   270	    TEST_CHECK(app_uuid_equal(&index.ids[0], &id));
   271	}
   272	
   273	int main(void) {
   274	    test_invalid_and_conflict_inputs_do_not_mutate();
   275	    test_global_dependency_must_match();
   276	    test_valid_replace_commits_complete_tree();
   277	    reset_storage();
   278	    storage_repository_lock_deinit();
   279	    puts("storage package replace tests passed");
   280	    return EXIT_SUCCESS;
   281	}
```
