# Phase 18.3 materialization implementation

```c
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
   221	            .scope = MACRO_SCOPE_GLOBAL,
   222	        };
   223	        result = storage_macro_read_locked(&location, &package_macro.id, &stored);
   224	        if (result == APP_ERROR_NOT_FOUND) {
   225	            result = APP_ERROR_CONFLICT;
   226	        }
   227	    }
   228	    char *package_json = NULL;
   229	    char *stored_json = NULL;
   230	    size_t package_length = 0U;
   231	    size_t stored_length = 0U;
   232	    if (result == APP_ERROR_NONE) {
   233	        result = canonical_macro_json(&package_macro, &package_json, &package_length);
   234	    }
   235	    if (result == APP_ERROR_NONE) {
   236	        result = canonical_macro_json(&stored, &stored_json, &stored_length);
   237	    }
   238	    if (result == APP_ERROR_NONE &&
   239	        (package_length != stored_length ||
   240	         memcmp(package_json, stored_json, package_length) != 0)) {
   241	        result = APP_ERROR_CONFLICT;
   242	    }
   243	    cJSON_free(package_json);
   244	    cJSON_free(stored_json);
   245	    macro_model_free_macro(&package_macro);
   246	    macro_model_free_macro(&stored);
   247	    return result;
   248	}
   249	
   250	static app_error_code_t verify_global_dependencies(const cJSON *array) {
   251	    const int count = cJSON_GetArraySize(array);
   252	    for (int index = 0; index < count; ++index) {
   253	        const app_error_code_t result = verify_global_dependency(cJSON_GetArrayItem(array, index));
   254	        if (result != APP_ERROR_NONE) {
   255	            return result;
   256	        }
   257	    }
   258	    return APP_ERROR_NONE;
   259	}
   260	
   261	static app_error_code_t write_json_file(const char *path, const char *json, size_t length) {
   262	    return storage_atomic_write(path, json, length, true);
   263	}
   264	
   265	static app_error_code_t write_set(const char *staging, const macro_set_t *set) {
   266	    char *json = NULL;
   267	    size_t length = 0U;
   268	    app_error_code_t result = storage_repository_serialize_set_json(set, &json, &length);
   269	    char path[APP_PATH_MAX_BYTES];
   270	    if (result == APP_ERROR_NONE) {
   271	        result = join_path(staging, "set.json", path, sizeof(path));
   272	    }
   273	    if (result == APP_ERROR_NONE) {
   274	        result = write_json_file(path, json, length);
   275	    }
   276	    cJSON_free(json);
   277	    return result;
   278	}
   279	
   280	static app_error_code_t write_order(const char *staging, const char *name,
   281	                                    const storage_uuid_order_t *order, size_t maximum) {
   282	    char *json = NULL;
   283	    size_t length = 0U;
   284	    app_error_code_t result =
   285	        storage_repository_serialize_order_json(order, maximum, &json, &length);
   286	    char path[APP_PATH_MAX_BYTES];
   287	    if (result == APP_ERROR_NONE) {
   288	        result = join_path(staging, name, path, sizeof(path));
   289	    }
   290	    if (result == APP_ERROR_NONE) {
   291	        result = write_json_file(path, json, length);
   292	    }
   293	    cJSON_free(json);
   294	    return result;
   295	}
   296	
   297	static app_error_code_t write_macro_node(const char *directory, const cJSON *node,
   298	                                         const app_uuid_t *set_id,
   299	                                         storage_uuid_order_t *order) {
   300	    macro_t macro = {0};
   301	    app_error_code_t result = parse_macro_node(node, &macro);
   302	    if (result == APP_ERROR_NONE &&
   303	        (macro.scope != MACRO_SCOPE_SET || !macro.has_set_id ||
   304	         !app_uuid_equal(&macro.set_id, set_id) || order->count >= APP_MACROS_PER_SET_MAX)) {
   305	        result = APP_ERROR_INVALID_ARGUMENT;
   306	    }
   307	    char *json = NULL;
   308	    size_t length = 0U;
   309	    if (result == APP_ERROR_NONE) {
   310	        result = storage_repository_serialize_macro_json(&macro, &json, &length);
   311	    }
   312	    char name[APP_UUID_STRING_LENGTH + 6U];
   313	    char path[APP_PATH_MAX_BYTES];
   314	    if (result == APP_ERROR_NONE) {
   315	        const int written = snprintf(name, sizeof(name), "%s.json", macro.id.value);
   316	        result = written >= 0 && (size_t)written < sizeof(name)
   317	                     ? join_path(directory, name, path, sizeof(path))
   318	                     : APP_ERROR_INVALID_ARGUMENT;
   319	    }
   320	    if (result == APP_ERROR_NONE) {
   321	        result = write_json_file(path, json, length);
   322	    }
   323	    if (result == APP_ERROR_NONE) {
   324	        order->ids[order->count++] = macro.id;
   325	    }
   326	    cJSON_free(json);
   327	    macro_model_free_macro(&macro);
   328	    return result;
   329	}
   330	
   331	static app_error_code_t write_macros(const char *staging, const cJSON *array,
   332	                                     const app_uuid_t *set_id) {
   333	    char directory[APP_PATH_MAX_BYTES];
   334	    app_error_code_t result = join_path(staging, "macros", directory, sizeof(directory));
   335	    storage_uuid_order_t order = {0};
   336	    const int count = cJSON_GetArraySize(array);
   337	    for (int index = 0; result == APP_ERROR_NONE && index < count; ++index) {
   338	        result = write_macro_node(directory, cJSON_GetArrayItem(array, index), set_id, &order);
   339	    }
   340	    return result == APP_ERROR_NONE
   341	               ? write_order(staging, "macro-order.json", &order, APP_MACROS_PER_SET_MAX)
   342	               : result;
   343	}
   344	
   345	static app_error_code_t write_procedure_node(const char *directory, const cJSON *node,
   346	                                             const app_uuid_t *set_id,
   347	                                             storage_uuid_order_t *order) {
   348	    procedure_t procedure = {0};
   349	    app_error_code_t result = parse_procedure_node(node, &procedure);
   350	    if (result == APP_ERROR_NONE &&
   351	        (!app_uuid_equal(&procedure.set_id, set_id) ||
   352	         order->count >= APP_PROCEDURES_PER_SET_MAX)) {
   353	        result = APP_ERROR_INVALID_ARGUMENT;
   354	    }
   355	    char *json = NULL;
   356	    size_t length = 0U;
   357	    if (result == APP_ERROR_NONE) {
   358	        result = storage_repository_serialize_procedure_json(&procedure, &json, &length);
   359	    }
   360	    char name[APP_UUID_STRING_LENGTH + 6U];
   361	    char path[APP_PATH_MAX_BYTES];
   362	    if (result == APP_ERROR_NONE) {
   363	        const int written = snprintf(name, sizeof(name), "%s.json", procedure.id.value);
   364	        result = written >= 0 && (size_t)written < sizeof(name)
   365	                     ? join_path(directory, name, path, sizeof(path))
   366	                     : APP_ERROR_INVALID_ARGUMENT;
   367	    }
   368	    if (result == APP_ERROR_NONE) {
   369	        result = write_json_file(path, json, length);
   370	    }
   371	    if (result == APP_ERROR_NONE) {
   372	        order->ids[order->count++] = procedure.id;
   373	    }
   374	    cJSON_free(json);
   375	    macro_model_free_procedure(&procedure);
   376	    return result;
   377	}
   378	
   379	static app_error_code_t write_procedures(const char *staging, const cJSON *array,
   380	                                         const app_uuid_t *set_id) {
   381	    char directory[APP_PATH_MAX_BYTES];
   382	    app_error_code_t result = join_path(staging, "procedures", directory, sizeof(directory));
   383	    storage_uuid_order_t order = {0};
   384	    const int count = cJSON_GetArraySize(array);
   385	    for (int index = 0; result == APP_ERROR_NONE && index < count; ++index) {
   386	        result =
   387	            write_procedure_node(directory, cJSON_GetArrayItem(array, index), set_id, &order);
   388	    }
   389	    return result == APP_ERROR_NONE
   390	               ? write_order(staging, "procedure-order.json", &order,
   391	                             APP_PROCEDURES_PER_SET_MAX)
   392	               : result;
   393	}
   394	
   395	static app_error_code_t write_progress_node(const char *directory, const cJSON *node,
   396	                                            const app_uuid_t *set_id) {
   397	    procedure_progress_t progress = {0};
   398	    app_error_code_t result = parse_progress_node(node, &progress);
   399	    if (result == APP_ERROR_NONE && !app_uuid_equal(&progress.set_id, set_id)) {
   400	        result = APP_ERROR_INVALID_ARGUMENT;
   401	    }
   402	    char *json = NULL;
   403	    size_t length = 0U;
   404	    if (result == APP_ERROR_NONE) {
   405	        result = storage_repository_serialize_progress_json(&progress, &json, &length);
   406	    }
   407	    char name[APP_UUID_STRING_LENGTH + 6U];
   408	    char path[APP_PATH_MAX_BYTES];
   409	    if (result == APP_ERROR_NONE) {
   410	        const int written = snprintf(name, sizeof(name), "%s.json", progress.procedure_id.value);
   411	        result = written >= 0 && (size_t)written < sizeof(name)
   412	                     ? join_path(directory, name, path, sizeof(path))
   413	                     : APP_ERROR_INVALID_ARGUMENT;
   414	    }
   415	    if (result == APP_ERROR_NONE) {
   416	        result = write_json_file(path, json, length);
   417	    }
   418	    cJSON_free(json);
   419	    return result;
   420	}
   421	
   422	static app_error_code_t write_progress(const char *staging, const cJSON *array,
   423	                                       const app_uuid_t *set_id) {
   424	    char directory[APP_PATH_MAX_BYTES];
   425	    app_error_code_t result = join_path(staging, "progress", directory, sizeof(directory));
   426	    const int count = cJSON_GetArraySize(array);
   427	    for (int index = 0; result == APP_ERROR_NONE && index < count; ++index) {
   428	        result = write_progress_node(directory, cJSON_GetArrayItem(array, index), set_id);
   429	    }
   430	    return result;
   431	}
   432	
   433	static app_error_code_t create_staging(const app_uuid_t *transaction_id, char *staging,
   434	                                       size_t staging_size) {
   435	    const int written = snprintf(staging, staging_size, STORAGE_DATA_MOUNT "/staging/%s",
   436	                                 transaction_id->value);
   437	    if (written < 0 || (size_t)written >= staging_size) {
   438	        return APP_ERROR_INVALID_ARGUMENT;
   439	    }
   440	    app_error_code_t result = make_directory(staging);
   441	    static const char *const children[] = {"macros", "procedures", "progress"};
   442	    for (size_t index = 0U;
   443	         result == APP_ERROR_NONE && index < sizeof(children) / sizeof(children[0]); ++index) {
   444	        char path[APP_PATH_MAX_BYTES];
   445	        result = join_path(staging, children[index], path, sizeof(path));
   446	        if (result == APP_ERROR_NONE) {
   447	            result = make_directory(path);
   448	        }
   449	    }
   450	    return result;
   451	}
   452	
   453	static app_error_code_t materialize_staging(const package_replace_document_t *document,
   454	                                            const char *staging) {
   455	    app_error_code_t result = write_set(staging, &document->replacement);
   456	    if (result == APP_ERROR_NONE) {
   457	        result = write_macros(staging, document->arrays[PACKAGE_REPLACE_MACROS],
   458	                              &document->replacement.id);
   459	    }
   460	    if (result == APP_ERROR_NONE) {
   461	        result = write_procedures(staging, document->arrays[PACKAGE_REPLACE_PROCEDURES],
   462	                                  &document->replacement.id);
   463	    }
   464	    if (result == APP_ERROR_NONE) {
   465	        result = write_progress(staging, document->arrays[PACKAGE_REPLACE_PROGRESS],
   466	                                &document->replacement.id);
   467	    }
   468	    return result;
   469	}
   470	
   471	static app_error_code_t copy_manifest_path(char *destination, size_t destination_size,
   472	                                           const char *source) {
   473	    const int written = snprintf(destination, destination_size, "%s", source);
   474	    return written >= 0 && (size_t)written < destination_size ? APP_ERROR_NONE
   475	                                                              : APP_ERROR_INVALID_ARGUMENT;
   476	}
   477	
   478	static app_error_code_t initialize_manifest(const app_uuid_t *transaction_id,
   479	                                            const macro_set_t *current,
   480	                                            const macro_set_t *replacement,
   481	                                            const char *staging,
   482	                                            storage_transaction_manifest_t *out_manifest) {
   483	    memset(out_manifest, 0, sizeof(*out_manifest));
   484	    out_manifest->schema_version = APP_SCHEMA_VERSION;
   485	    out_manifest->id = *transaction_id;
   486	    out_manifest->type = STORAGE_TRANSACTION_REPLACE_SET;
   487	    out_manifest->phase = STORAGE_TRANSACTION_PREPARED;
   488	    out_manifest->expected_revision = current->revision;
   489	    out_manifest->replacement_revision = replacement->revision;
   490	    char destination[APP_PATH_MAX_BYTES];
   491	    app_error_code_t result =
   492	        storage_make_set_path(&replacement->id, destination, sizeof(destination));
   493	    char backup[APP_PATH_MAX_BYTES];
   494	    if (result == APP_ERROR_NONE) {
   495	        const int written = snprintf(backup, sizeof(backup), STORAGE_DATA_MOUNT "/trash/%s-%s",
   496	                                     replacement->id.value, transaction_id->value);
   497	        result = written >= 0 && (size_t)written < sizeof(backup) ? APP_ERROR_NONE
   498	                                                                  : APP_ERROR_INVALID_ARGUMENT;
   499	    }
   500	    if (result == APP_ERROR_NONE) {
   501	        result = copy_manifest_path(out_manifest->source, sizeof(out_manifest->source),
   502	                                    destination);
   503	    }
   504	    if (result == APP_ERROR_NONE) {
   505	        result = copy_manifest_path(out_manifest->staging, sizeof(out_manifest->staging), staging);
   506	    }
   507	    if (result == APP_ERROR_NONE) {
   508	        result = copy_manifest_path(out_manifest->destination,
   509	                                    sizeof(out_manifest->destination), destination);
   510	    }
   511	    if (result == APP_ERROR_NONE) {
   512	        result = copy_manifest_path(out_manifest->backup, sizeof(out_manifest->backup), backup);
   513	    }
   514	    return result;
   515	}
   516	
   517	static app_error_code_t recover_replace(storage_transaction_manifest_t *manifest) {
   518	    return storage_transaction_recover_replace_with_ops(
   519	        manifest, storage_fs_ops_posix(), package_uuid_generate, NULL, package_index_presence,
   520	        NULL, package_validate_tree, NULL, package_remove_tree, NULL);
   521	}
   522	
   523	static app_error_code_t replace_locked(const app_uuid_t *target_set_id,
   524	                                       uint32_t expected_revision,
   525	                                       package_replace_document_t *document,
```
