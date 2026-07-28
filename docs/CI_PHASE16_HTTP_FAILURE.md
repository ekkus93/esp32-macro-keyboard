# Phase 16 HTTP integration failure

```text
2026-07-28T12:39:14.9950850Z [ 60%] Building C object CMakeFiles/web_execution_submit_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_execution_submit.c.o
2026-07-28T12:39:14.9952758Z [ 60%] Building C object CMakeFiles/storage_atomic_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
2026-07-28T12:39:14.9954552Z [ 60%] Building C object CMakeFiles/web_execution_submit_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
2026-07-28T12:39:14.9956310Z [ 60%] Building C object CMakeFiles/storage_atomic_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
2026-07-28T12:39:14.9958171Z [ 42%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c.o
2026-07-28T12:39:14.9960321Z [ 60%] Building C object CMakeFiles/storage_repository_lock_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
2026-07-28T12:39:14.9961794Z [ 60%] Building C object CMakeFiles/storage_progress_repository_tests.dir/test_storage_progress.c.o
2026-07-28T12:39:14.9963563Z [ 60%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
2026-07-28T12:39:14.9964948Z [ 60%] Building C object CMakeFiles/web_api_core_tests.dir/test_web_api_core.c.o
2026-07-28T12:39:14.9965967Z [ 60%] Building C object CMakeFiles/storage_repository_io_tests.dir/test_storage_repository_io.c.o
2026-07-28T12:39:14.9966919Z [ 60%] Building C object CMakeFiles/storage_mount_tests.dir/test_storage_mount.c.o
2026-07-28T12:39:14.9968438Z [ 60%] Building C object CMakeFiles/provisioning_bootstrap_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/provisioning/provisioning_bootstrap_core.c.o
2026-07-28T12:39:14.9970386Z [ 60%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
2026-07-28T12:39:14.9972250Z [ 60%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
2026-07-28T12:39:14.9974222Z [ 60%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
2026-07-28T12:39:14.9976155Z [ 60%] Building C object CMakeFiles/web_api_response_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c.o
2026-07-28T12:39:14.9978020Z [ 60%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
2026-07-28T12:39:14.9979912Z [ 60%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
2026-07-28T12:39:14.9981952Z [ 60%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
2026-07-28T12:39:14.9983806Z [ 60%] Building C object CMakeFiles/web_api_response_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
2026-07-28T12:39:14.9985501Z [ 60%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
2026-07-28T12:39:14.9986810Z [ 60%] Building C object CMakeFiles/web_request_policy_tests.dir/test_web_request_policy.c.o
2026-07-28T12:39:15.0004271Z [ 60%] Building C object CMakeFiles/web_execution_submit_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
2026-07-28T12:39:15.0034161Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_auth.inc:24:51: error: unknown type name ‘size_t’
2026-07-28T12:39:15.0035169Z    24 |                                                   size_t output_size)
2026-07-28T12:39:15.0035686Z       |                                                   ^~~~~~
2026-07-28T12:39:15.0037183Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_auth.inc:1:1: note: ‘size_t’ is defined in header ‘<stddef.h>’; did you forget to ‘#include <stddef.h>’?
2026-07-28T12:39:15.0038185Z   +++ |+#include <stddef.h>
2026-07-28T12:39:15.0038497Z     1 | typedef struct {
2026-07-28T12:39:15.0039658Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_auth.inc:50:8: error: unknown type name ‘app_error_code_t’
2026-07-28T12:39:15.0040842Z    50 | static app_error_code_t validate_authorization(void *context,
2026-07-28T12:39:15.0041310Z       |        ^~~~~~~~~~~~~~~~
2026-07-28T12:39:15.0042352Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_auth.inc: In function ‘validate_authorization’:
2026-07-28T12:39:15.0044156Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_auth.inc:55:24: error: ‘NULL’ undeclared (first use in this function)
2026-07-28T12:39:15.0045181Z    55 |     TEST_CHECK(fake != NULL);
2026-07-28T12:39:15.0045557Z       |                        ^~~~
2026-07-28T12:39:15.0046942Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_auth.inc:55:24: note: ‘NULL’ is defined in header ‘<stddef.h>’; did you forget to ‘#include <stddef.h>’?
2026-07-28T12:39:15.0048824Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_auth.inc:55:24: note: each undeclared identifier is reported only once for each function it appears in
2026-07-28T12:39:15.0050486Z [ 60%] Building C object CMakeFiles/auth_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/auth/auth_core_rate_limit.c.o
2026-07-28T12:39:15.0051830Z In file included from /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_macro_executor.c:10:
2026-07-28T12:39:15.0053481Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc: In function ‘test_action_order_delay_and_status_progress’:
2026-07-28T12:39:15.0055402Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:3:5: error: unknown type name ‘executor_fake_t’; did you mean ‘execution_state_t’?
2026-07-28T12:39:15.0061973Z [ 60%] Building C object CMakeFiles/storage_transaction_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
2026-07-28T12:39:15.0064047Z [ 60%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
2026-07-28T12:39:15.0066175Z [ 60%] Building C object CMakeFiles/web_setup_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_setup_json.c.o
2026-07-28T12:39:15.0068027Z [ 81%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
2026-07-28T12:39:15.0069906Z [ 82%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
2026-07-28T12:39:15.0071017Z [ 82%] Linking C executable macro_model_tests
2026-07-28T12:39:15.0072219Z [ 83%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_cookie.c.o
2026-07-28T12:39:15.0074075Z [ 84%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
2026-07-28T12:39:15.0075404Z [ 85%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/test_storage_procedures.c.o
2026-07-28T12:39:15.0076724Z [ 86%] Building C object CMakeFiles/web_api_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_uuid.c.o
2026-07-28T12:39:15.0078493Z [ 87%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
2026-07-28T12:39:15.0079637Z [ 87%] Linking C executable test_support_tests
2026-07-28T12:39:15.0081117Z [ 88%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
2026-07-28T12:39:15.0083170Z [ 89%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
2026-07-28T12:39:15.0085032Z [ 90%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
2026-07-28T12:39:15.0086955Z [ 91%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_paths.c.o
2026-07-28T12:39:15.0088917Z [ 81%] Building C object CMakeFiles/storage_progress_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
2026-07-28T12:39:15.0090924Z [ 92%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_macros.c.o
2026-07-28T12:39:15.0093118Z [ 81%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
2026-07-28T12:39:15.0095113Z [ 81%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
2026-07-28T12:39:15.0097164Z [ 81%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
2026-07-28T12:39:15.0098369Z [ 86%] Built target app_operation_result_tests
2026-07-28T12:39:15.0099709Z [ 81%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
2026-07-28T12:39:15.0101609Z [ 81%] Building C object CMakeFiles/storage_atomic_recovery_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
2026-07-28T12:39:15.0103566Z [ 81%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
2026-07-28T12:39:15.0105476Z [ 81%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
2026-07-28T12:39:15.0107296Z [ 81%] Building C object CMakeFiles/web_api_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c.o
2026-07-28T12:39:15.0108931Z [ 81%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
2026-07-28T12:39:15.0137880Z [ 81%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_lock.c.o
2026-07-28T12:39:15.0143259Z [ 81%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
2026-07-28T12:39:15.0170584Z [ 81%] Building C object CMakeFiles/web_request_policy_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_origin.c.o
2026-07-28T12:39:15.0176526Z [ 81%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_sets.c.o
2026-07-28T12:39:15.0178795Z [ 81%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_io.c.o
2026-07-28T12:39:15.0180730Z [ 81%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
2026-07-28T12:39:15.0182777Z [ 81%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_transaction.c.o
2026-07-28T12:39:15.0183975Z     3 |     executor_fake_t fake;
2026-07-28T12:39:15.0214559Z [ 92%] Building C object CMakeFiles/storage_atomic_validators_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_fs_ops.c.o
2026-07-28T12:39:15.0233920Z [ 81%] Building C object CMakeFiles/storage_procedure_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c.o
2026-07-28T12:39:15.0238562Z [ 81%] Building C object CMakeFiles/web_server_adapter_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_adapter_lifecycle.c.o
2026-07-28T12:39:15.0240464Z [ 81%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
2026-07-28T12:39:15.0242440Z [ 81%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_progress.c.o
2026-07-28T12:39:15.0244654Z [ 81%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_quarantine.c.o
2026-07-28T12:39:15.0246613Z [ 81%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c.o
2026-07-28T12:39:15.0248657Z [ 81%] Building C object CMakeFiles/storage_macro_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_objects_json.c.o
2026-07-28T12:39:15.0250537Z [ 81%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c.o
2026-07-28T12:39:15.0252363Z [ 81%] Building C object CMakeFiles/storage_active_set_delete_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_order.c.o
2026-07-28T12:39:15.0253619Z       |     ^~~~~~~~~~~~~~~
2026-07-28T12:39:15.0254881Z [ 81%] Building C object CMakeFiles/storage_repository_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_index.c.o
2026-07-28T12:39:15.0256967Z [ 81%] Building C object CMakeFiles/web_api_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_json.c.o
2026-07-28T12:39:15.0258746Z [ 81%] Building C object CMakeFiles/storage_object_json_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/macro_model.c.o
2026-07-28T12:39:15.0259932Z [ 93%] Linking C executable storage_repository_lock_tests
2026-07-28T12:39:15.0260538Z [ 93%] Linking C executable macro_parser_tests
2026-07-28T12:39:15.0261143Z [ 93%] Built target macro_model_tests
2026-07-28T12:39:15.0261703Z [ 94%] Linking C executable web_execution_submit_tests
2026-07-28T12:39:15.0262266Z [ 94%] Linking C executable usb_keyboard_tests
2026-07-28T12:39:15.0263484Z [ 95%] Linking C executable web_api_core_tests
2026-07-28T12:39:15.0264015Z [ 95%] Linking C executable web_setup_json_tests
2026-07-28T12:39:15.0264522Z [ 95%] Built target test_support_tests
2026-07-28T12:39:15.0265070Z [ 95%] Linking C executable provisioning_bootstrap_tests
2026-07-28T12:39:15.0265645Z [ 95%] Linking C executable web_request_policy_tests
2026-07-28T12:39:15.0266387Z [ 96%] Linking C executable provisioning_settings_tests
2026-07-28T12:39:15.0266953Z [ 96%] Built target macro_parser_tests
2026-07-28T12:39:15.0267487Z [ 96%] Linking C executable device_controls_tests
2026-07-28T12:39:15.0268025Z [ 96%] Linking C executable web_setup_tests
2026-07-28T12:39:15.0268574Z [ 96%] Built target storage_repository_lock_tests
2026-07-28T12:39:15.0269112Z [ 96%] Linking C executable auth_tests
2026-07-28T12:39:15.0269698Z [ 96%] Linking C executable storage_atomic_tests
2026-07-28T12:39:15.0270606Z [ 96%] Linking C executable storage_parent_sync_tests
2026-07-28T12:39:15.0271161Z [ 96%] Built target web_api_core_tests
2026-07-28T12:39:15.0271653Z [ 96%] Built target usb_keyboard_tests
2026-07-28T12:39:15.0272145Z [ 96%] Built target web_setup_json_tests
2026-07-28T12:39:15.0272837Z [ 96%] Built target web_execution_submit_tests
2026-07-28T12:39:15.0273440Z [ 96%] Linking C executable storage_mount_tests
2026-07-28T12:39:15.0274008Z [ 96%] Linking C executable web_api_json_tests
2026-07-28T12:39:15.0274525Z [ 97%] Linking C executable wifi_ap_tests
2026-07-28T12:39:15.0275092Z [ 97%] Linking C executable storage_repository_io_tests
2026-07-28T12:39:15.0275669Z [ 97%] Built target provisioning_bootstrap_tests
2026-07-28T12:39:15.0276213Z [ 97%] Linking C executable provisioning_tests
2026-07-28T12:39:15.0276754Z [ 97%] Built target provisioning_settings_tests
2026-07-28T12:39:15.0277391Z [ 97%] Built target web_request_policy_tests
2026-07-28T12:39:15.0277960Z [ 97%] Linking C executable storage_object_json_tests
2026-07-28T12:39:15.0278467Z [ 97%] Built target web_setup_tests
2026-07-28T12:39:15.0278939Z [ 97%] Built target device_controls_tests
2026-07-28T12:39:15.0279460Z [ 97%] Linking C executable storage_transaction_tests
2026-07-28T12:39:15.0279996Z [ 97%] Built target storage_atomic_tests
2026-07-28T12:39:15.0280510Z [ 97%] Built target storage_parent_sync_tests
2026-07-28T12:39:15.0281049Z [ 97%] Built target storage_repository_io_tests
2026-07-28T12:39:15.0281572Z [ 97%] Built target web_api_json_tests
2026-07-28T12:39:15.0282130Z [ 97%] Linking C executable storage_atomic_recovery_tests
2026-07-28T12:39:15.0282937Z [ 97%] Built target auth_tests
2026-07-28T12:39:15.0283452Z [ 97%] Built target storage_transaction_tests
2026-07-28T12:39:15.0283946Z [ 97%] Built target wifi_ap_tests
2026-07-28T12:39:15.0284409Z [ 97%] Built target storage_mount_tests
2026-07-28T12:39:15.0284906Z [ 97%] Built target storage_object_json_tests
2026-07-28T12:39:15.0285396Z [ 97%] Built target provisioning_tests
2026-07-28T12:39:15.0343773Z       |     execution_state_t
2026-07-28T12:39:15.0344244Z [ 97%] Built target storage_atomic_recovery_tests
2026-07-28T12:39:15.0391589Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_auth.inc:56:5: error: implicit declaration of function ‘TEST_CHECK_EQ_STRING’ [-Werror=implicit-function-declaration]
2026-07-28T12:39:15.0433307Z    56 |     TEST_CHECK_EQ_STRING(TOKEN, session_token);
2026-07-28T12:39:15.0433802Z [ 97%] Linking C executable app_core_tests
2026-07-28T12:39:15.0463632Z       |     ^~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.0503337Z [ 97%] Linking C executable storage_atomic_validators_tests
2026-07-28T12:39:15.0534006Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_auth.inc:56:26: error: ‘TOKEN’ undeclared (first use in this function)
2026-07-28T12:39:15.0563885Z    56 |     TEST_CHECK_EQ_STRING(TOKEN, session_token);
2026-07-28T12:39:15.0633160Z       |                          ^~~~~
2026-07-28T12:39:15.0663228Z [ 98%] Linking C executable storage_macro_repository_tests
2026-07-28T12:39:15.0693961Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_auth.inc: In function ‘test_mutation_authorization’:
2026-07-28T12:39:15.0695007Z [ 98%] Linking C executable storage_procedure_repository_tests
2026-07-28T12:39:15.0716635Z [ 98%] Linking C executable storage_progress_repository_tests
2026-07-28T12:39:15.0736197Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_auth.inc:64:24: error: ‘AUTH_TOKEN_HEX_BYTES’ undeclared (first use in this function)
2026-07-28T12:39:15.0793292Z [ 99%] Linking C executable storage_quarantine_tests
2026-07-28T12:39:15.0823307Z    64 |     char session_token[AUTH_TOKEN_HEX_BYTES];
2026-07-28T12:39:15.0853369Z       |                        ^~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.0884091Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_auth.inc:65:55: error: ‘APP_ERROR_NONE’ undeclared (first use in this function)
2026-07-28T12:39:15.0960047Z    65 |     authorization_fake_t fake = {.validation_result = APP_ERROR_NONE};
2026-07-28T12:39:15.0973993Z [ 99%] Linking C executable storage_repository_tests
2026-07-28T12:39:15.0993444Z       |                                                       ^~~~~~~~~~~~~~
2026-07-28T12:39:15.0995371Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_auth.inc:66:5: error: implicit declaration of function ‘TEST_CHECK_APP_ERROR’ [-Werror=implicit-function-declaration]
2026-07-28T12:39:15.0996626Z    66 |     TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
2026-07-28T12:39:15.0997032Z       |     ^~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.0998627Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_auth.inc:67:26: error: implicit declaration of function ‘web_adapter_authorize_mutation’ [-Werror=implicit-function-declaration]
2026-07-28T12:39:15.0999988Z [ 99%] Built target storage_progress_repository_tests
2026-07-28T12:39:15.1000619Z    67 |                          web_adapter_authorize_mutation(get_authorization_header,
2026-07-28T12:39:15.1062914Z       |                          ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1063404Z [ 99%] Built target app_core_tests
2026-07-28T12:39:15.1067257Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_auth.inc:67:57: error: ‘get_authorization_header’ undeclared (first use in this function)
2026-07-28T12:39:15.1068466Z    67 |                          web_adapter_authorize_mutation(get_authorization_header,
2026-07-28T12:39:15.1069076Z       |                                                         ^~~~~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1072141Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_auth.inc:72:26: error: ‘TOKEN’ undeclared (first use in this function)
2026-07-28T12:39:15.1073704Z    72 |     TEST_CHECK_EQ_STRING(TOKEN, session_token);
2026-07-28T12:39:15.1074113Z       |                          ^~~~~
2026-07-28T12:39:15.1075583Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_auth.inc:74:5: error: implicit declaration of function ‘TEST_CHECK_EQ_U64’ [-Werror=implicit-function-declaration]
2026-07-28T12:39:15.1076747Z    74 |     TEST_CHECK_EQ_U64(1U, fake.validation_calls);
2026-07-28T12:39:15.1077147Z       |     ^~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1078294Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_auth.inc:76:53: error: ‘true’ undeclared (first use in this function)
2026-07-28T12:39:15.1079245Z    76 |     fake = (authorization_fake_t){.invalid_origin = true};
2026-07-28T12:39:15.1079727Z       |                                                     ^~~~
2026-07-28T12:39:15.1081137Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_auth.inc:1:1: note: ‘true’ is defined in header ‘<stdbool.h>’; did you forget to ‘#include <stdbool.h>’?
2026-07-28T12:39:15.1082191Z   +++ |+#include <stdbool.h>
2026-07-28T12:39:15.1082514Z     1 | typedef struct {
2026-07-28T12:39:15.1083104Z [ 99%] Linking C executable storage_active_set_delete_tests
2026-07-28T12:39:15.1083692Z [ 99%] Built target storage_procedure_repository_tests
2026-07-28T12:39:15.1114850Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_auth.inc:77:5: error: incompatible implicit declaration of built-in function ‘memcpy’ [-Werror=builtin-declaration-mismatch]
2026-07-28T12:39:15.1130850Z    77 |     memcpy(session_token, "dirty", sizeof("dirty"));
2026-07-28T12:39:15.1131390Z [ 99%] Built target storage_atomic_validators_tests
2026-07-28T12:39:15.1133372Z       |     ^~~~~~
2026-07-28T12:39:15.1134742Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_auth.inc:77:5: note: include ‘<string.h>’ or provide a declaration of ‘memcpy’
2026-07-28T12:39:15.1136720Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_auth.inc:78:26: error: ‘APP_ERROR_AUTH_REQUIRED’ undeclared (first use in this function)
2026-07-28T12:39:15.1137751Z    78 |     TEST_CHECK_APP_ERROR(APP_ERROR_AUTH_REQUIRED,
2026-07-28T12:39:15.1138241Z       |                          ^~~~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1139578Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_auth.inc:97:56: error: ‘APP_ERROR_AUTH_FAILED’ undeclared (first use in this function)
2026-07-28T12:39:15.1140667Z    97 |     fake = (authorization_fake_t){.validation_result = APP_ERROR_AUTH_FAILED};
2026-07-28T12:39:15.1141327Z       |                                                        ^~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1142869Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_auth.inc:64:10: error: unused variable ‘session_token’ [-Werror=unused-variable]
2026-07-28T12:39:15.1143928Z    64 |     char session_token[AUTH_TOKEN_HEX_BYTES];
2026-07-28T12:39:15.1144394Z       |          ^~~~~~~~~~~~~
2026-07-28T12:39:15.1145198Z In file included from /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_security.c:1:
2026-07-28T12:39:15.1146809Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/web_security_content.inc: In function ‘test_content_boundaries’:
2026-07-28T12:39:15.1147633Z [ 99%] Built target storage_macro_repository_tests
2026-07-28T12:39:15.1149113Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/web_security_content.inc:3:5: error: implicit declaration of function ‘TEST_CHECK’ [-Werror=implicit-function-declaration]
2026-07-28T12:39:15.1150231Z     3 |     TEST_CHECK(web_accept_encoding_gzip("gzip"));
2026-07-28T12:39:15.1150674Z       |     ^~~~~~~~~~
2026-07-28T12:39:15.1152084Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/web_security_content.inc:3:16: error: implicit declaration of function ‘web_accept_encoding_gzip’ [-Werror=implicit-function-declaration]
2026-07-28T12:39:15.1153452Z     3 |     TEST_CHECK(web_accept_encoding_gzip("gzip"));
2026-07-28T12:39:15.1153905Z       |                ^~~~~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1155140Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/web_security_content.inc:16:42: error: ‘NULL’ undeclared (first use in this function)
2026-07-28T12:39:15.1156100Z    16 |     TEST_CHECK(!web_accept_encoding_gzip(NULL));
2026-07-28T12:39:15.1156661Z       |                                          ^~~~
2026-07-28T12:39:15.1157994Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/web_security_content.inc:1:1: note: ‘NULL’ is defined in header ‘<stddef.h>’; did you forget to ‘#include <stddef.h>’?
2026-07-28T12:39:15.1158969Z   +++ |+#include <stddef.h>
2026-07-28T12:39:15.1159391Z     1 | static void test_content_boundaries(void)
2026-07-28T12:39:15.1160607Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/web_security_content.inc:16:42: note: each undeclared identifier is reported only once for each function it appears in
2026-07-28T12:39:15.1161680Z    16 |     TEST_CHECK(!web_accept_encoding_gzip(NULL));
2026-07-28T12:39:15.1162135Z       |                                          ^~~~
2026-07-28T12:39:15.1164060Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/web_security_content.inc:18:5: error: implicit declaration of function ‘TEST_CHECK_EQ_STRING’ [-Werror=implicit-function-declaration]
2026-07-28T12:39:15.1165214Z [ 99%] Built target storage_quarantine_tests
2026-07-28T12:39:15.1165866Z    18 |     TEST_CHECK_EQ_STRING("text/html; charset=utf-8", web_content_type("index.html"));
2026-07-28T12:39:15.1166473Z       |     ^~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1167724Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/web_security_content.inc:18:54: error: implicit declaration of function ‘web_content_type’ [-Werror=implicit-function-declaration]
2026-07-28T12:39:15.1168571Z    18 |     TEST_CHECK_EQ_STRING("text/html; charset=utf-8", web_content_type("index.html"));
2026-07-28T12:39:15.1168994Z       |                                                      ^~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1169982Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:4:5: error: implicit declaration of function ‘executor_fake_reset’ [-Werror=implicit-function-declaration]
2026-07-28T12:39:15.1170661Z     4 |     executor_fake_reset(&fake);
2026-07-28T12:39:15.1170933Z       |     ^~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1171832Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:6:5: error: implicit declaration of function ‘executor_init_engine’ [-Werror=implicit-function-declaration]
2026-07-28T12:39:15.1172783Z     6 |     executor_init_engine(&engine, &fake);
2026-07-28T12:39:15.1173608Z       |     ^~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1174728Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:7:41: error: implicit declaration of function ‘executor_make_request’ [-Werror=implicit-function-declaration]
2026-07-28T12:39:15.1175463Z     7 |     macro_execution_request_t request = executor_make_request(3U);
2026-07-28T12:39:15.1175811Z       |                                         ^~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1176375Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:7:41: error: invalid initializer
2026-07-28T12:39:15.1177098Z In file included from /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_macro_executor.c:8:
2026-07-28T12:39:15.1178186Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:17:42: error: implicit declaration of function ‘executor_execute_queued’ [-Werror=implicit-function-declaration]
2026-07-28T12:39:15.1178957Z    17 |     TEST_CHECK_APP_ERROR(APP_ERROR_NONE, executor_execute_queued(&engine, &fake));
2026-07-28T12:39:15.1179323Z       |                                          ^~~~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1180015Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/support/test_assert.h:27:50: note: in definition of macro ‘TEST_CHECK_EQ_U64’
2026-07-28T12:39:15.1180660Z    27 |         const uint64_t test_actual_ = (uint64_t)(actual_value);                                    \
2026-07-28T12:39:15.1181037Z       |                                                  ^~~~~~~~~~~~
2026-07-28T12:39:15.1181715Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/support/test_assert.h:64:5: note: in expansion of macro ‘TEST_CHECK_EQ_INT’
2026-07-28T12:39:15.1182251Z    64 |     TEST_CHECK_EQ_INT((expected_value), (actual_value))
2026-07-28T12:39:15.1182520Z       |     ^~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1183575Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:17:5: note: in expansion of macro ‘TEST_CHECK_APP_ERROR’
2026-07-28T12:39:15.1184204Z    17 |     TEST_CHECK_APP_ERROR(APP_ERROR_NONE, executor_execute_queued(&engine, &fake));
2026-07-28T12:39:15.1184537Z       |     ^~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1185528Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:19:5: error: implicit declaration of function ‘executor_assert_relevant_call’ [-Werror=implicit-function-declaration]
2026-07-28T12:39:15.1186251Z    19 |     executor_assert_relevant_call(&fake, 0U, "press", 2U);
2026-07-28T12:39:15.1186538Z       |     ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1187342Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:33:31: error: request for member ‘release_index’ in something not a structure or union
2026-07-28T12:39:15.1188026Z    33 |     TEST_CHECK_EQ_U64(3U, fake.release_index);
2026-07-28T12:39:15.1188289Z       |                               ^
2026-07-28T12:39:15.1188958Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/support/test_assert.h:27:50: note: in definition of macro ‘TEST_CHECK_EQ_U64’
2026-07-28T12:39:15.1189587Z    27 |         const uint64_t test_actual_ = (uint64_t)(actual_value);                                    \
2026-07-28T12:39:15.1189972Z       |                                                  ^~~~~~~~~~~~
2026-07-28T12:39:15.1190789Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:39:41: error: request for member ‘snapshot_count’ in something not a structure or union
2026-07-28T12:39:15.1191442Z    39 |     for (size_t index = 0U; index < fake.snapshot_count; ++index) {
2026-07-28T12:39:15.1191757Z       |                                         ^
2026-07-28T12:39:15.1192658Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:40:41: error: request for member ‘snapshots’ in something not a structure or union
2026-07-28T12:39:15.1193667Z    40 |         const size_t action_index = fake.snapshots[index].action_index;
2026-07-28T12:39:15.1194061Z       |                                         ^
2026-07-28T12:39:15.1195436Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:48:5: error: implicit declaration of function ‘executor_assert_terminal’ [-Werror=implicit-function-declaration]
2026-07-28T12:39:15.1196676Z    48 |     executor_assert_terminal(&engine, EXECUTION_COMPLETED, APP_ERROR_NONE, false);
2026-07-28T12:39:15.1197077Z       |     ^~~~~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1197768Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc: In function ‘test_cancel_before_and_during_actions’:
2026-07-28T12:39:15.1198840Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:53:5: error: unknown type name ‘executor_fake_t’; did you mean ‘execution_state_t’?
2026-07-28T12:39:15.1199416Z    53 |     executor_fake_t fake;
2026-07-28T12:39:15.1199625Z       |     ^~~~~~~~~~~~~~~
2026-07-28T12:39:15.1199820Z       |     execution_state_t
2026-07-28T12:39:15.1200707Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:57:5: error: implicit declaration of function ‘executor_submit_single_key’ [-Werror=implicit-function-declaration]
2026-07-28T12:39:15.1201392Z    57 |     executor_submit_single_key(&engine, &fake, 4U);
2026-07-28T12:39:15.1201660Z       |     ^~~~~~~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1202429Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:61:31: error: request for member ‘wait_count’ in something not a structure or union
2026-07-28T12:39:15.1203153Z    61 |     TEST_CHECK_EQ_U64(0U, fake.wait_count);
2026-07-28T12:39:15.1203428Z       |                               ^
2026-07-28T12:39:15.1204093Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/support/test_assert.h:27:50: note: in definition of macro ‘TEST_CHECK_EQ_U64’
2026-07-28T12:39:15.1204732Z    27 |         const uint64_t test_actual_ = (uint64_t)(actual_value);                                    \
2026-07-28T12:39:15.1205109Z       |                                                  ^~~~~~~~~~~~
2026-07-28T12:39:15.1206025Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:62:31: error: request for member ‘release_index’ in something not a structure or union
2026-07-28T12:39:15.1206628Z    62 |     TEST_CHECK_EQ_U64(1U, fake.release_index);
2026-07-28T12:39:15.1206889Z       |                               ^
2026-07-28T12:39:15.1207545Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/support/test_assert.h:27:50: note: in definition of macro ‘TEST_CHECK_EQ_U64’
2026-07-28T12:39:15.1208269Z    27 |         const uint64_t test_actual_ = (uint64_t)(actual_value);                                    \
2026-07-28T12:39:15.1208653Z       |                                                  ^~~~~~~~~~~~
2026-07-28T12:39:15.1209466Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:71:9: error: request for member ‘cancel_on_wait’ in something not a structure or union
2026-07-28T12:39:15.1210044Z    71 |     fake.cancel_on_wait = 1U;
2026-07-28T12:39:15.1210261Z       |         ^
2026-07-28T12:39:15.1211012Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:74:31: error: request for member ‘wait_count’ in something not a structure or union
2026-07-28T12:39:15.1211602Z    74 |     TEST_CHECK_EQ_U64(1U, fake.wait_count);
2026-07-28T12:39:15.1211854Z       |                               ^
2026-07-28T12:39:15.1212533Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/support/test_assert.h:27:50: note: in definition of macro ‘TEST_CHECK_EQ_U64’
2026-07-28T12:39:15.1213377Z    27 |         const uint64_t test_actual_ = (uint64_t)(actual_value);                                    \
2026-07-28T12:39:15.1213756Z       |                                                  ^~~~~~~~~~~~
2026-07-28T12:39:15.1214575Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:75:31: error: request for member ‘release_index’ in something not a structure or union
2026-07-28T12:39:15.1215182Z    75 |     TEST_CHECK_EQ_U64(2U, fake.release_index);
2026-07-28T12:39:15.1215439Z       |                               ^
2026-07-28T12:39:15.1216086Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/support/test_assert.h:27:50: note: in definition of macro ‘TEST_CHECK_EQ_U64’
2026-07-28T12:39:15.1216707Z    27 |         const uint64_t test_actual_ = (uint64_t)(actual_value);                                    \
2026-07-28T12:39:15.1217101Z       |                                                  ^~~~~~~~~~~~
2026-07-28T12:39:15.1217661Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:79:41: error: invalid initializer
2026-07-28T12:39:15.1218193Z    79 |     macro_execution_request_t request = executor_make_request(1U);
2026-07-28T12:39:15.1218524Z       |                                         ^~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1219333Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:82:9: error: request for member ‘cancel_on_wait’ in something not a structure or union
2026-07-28T12:39:15.1219909Z    82 |     fake.cancel_on_wait = 2U;
2026-07-28T12:39:15.1220116Z       |         ^
2026-07-28T12:39:15.1220596Z In file included from /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_body.inc:9,
2026-07-28T12:39:15.1221254Z                  from /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter.c:2:
2026-07-28T12:39:15.1221991Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/../../firmware/components/web_server/web_server_adapter.h: At top level:
2026-07-28T12:39:15.1223237Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:86:31: error: request for member ‘wait_count’ in something not a structure or union
2026-07-28T12:39:15.1223836Z    86 |     TEST_CHECK_EQ_U64(2U, fake.wait_count);
2026-07-28T12:39:15.1224149Z       |                               ^
2026-07-28T12:39:15.1224880Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/support/test_assert.h:27:50: note: in definition of macro ‘TEST_CHECK_EQ_U64’
2026-07-28T12:39:15.1225841Z    27 |         const uint64_t test_actual_ = (uint64_t)(actual_value);                                    \
2026-07-28T12:39:15.1226381Z       |                                                  ^~~~~~~~~~~~
2026-07-28T12:39:15.1227238Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:87:31: error: request for member ‘release_index’ in something not a structure or union
2026-07-28T12:39:15.1227971Z    87 |     TEST_CHECK_EQ_U64(1U, fake.release_index);
2026-07-28T12:39:15.1228337Z       |                               ^
2026-07-28T12:39:15.1229313Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/support/test_assert.h:27:50: note: in definition of macro ‘TEST_CHECK_EQ_U64’
2026-07-28T12:39:15.1230270Z    27 |         const uint64_t test_actual_ = (uint64_t)(actual_value);                                    \
2026-07-28T12:39:15.1230842Z       |                                                  ^~~~~~~~~~~~
2026-07-28T12:39:15.1231714Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc: In function ‘test_watchdog_and_wait_failures’:
2026-07-28T12:39:15.1233238Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:92:5: error: unknown type name ‘executor_fake_t’; did you mean ‘execution_state_t’?
2026-07-28T12:39:15.1233998Z    92 |     executor_fake_t fake;
2026-07-28T12:39:15.1234291Z       |     ^~~~~~~~~~~~~~~
2026-07-28T12:39:15.1234492Z       |     execution_state_t
2026-07-28T12:39:15.1235015Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:96:41: error: invalid initializer
2026-07-28T12:39:15.1235674Z    96 |     macro_execution_request_t request = executor_make_request(1U);
2026-07-28T12:39:15.1236014Z       |                                         ^~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1236881Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:99:9: error: request for member ‘extra_advance_on_wait_ms’ in something not a structure or union
2026-07-28T12:39:15.1237508Z    99 |     fake.extra_advance_on_wait_ms = 2000U;
2026-07-28T12:39:15.1237753Z       |         ^
2026-07-28T12:39:15.1238499Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:102:20: error: request for member ‘wait_count’ in something not a structure or union
2026-07-28T12:39:15.1239086Z   102 |     TEST_CHECK(fake.wait_count > 0U);
2026-07-28T12:39:15.1239320Z       |                    ^
2026-07-28T12:39:15.1240248Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/support/test_assert.h:19:15: note: in definition of macro ‘TEST_CHECK’
2026-07-28T12:39:15.1241162Z    19 |         if (!(expression)) {                                                                       \
2026-07-28T12:39:15.1241610Z       |               ^~~~~~~~~~
2026-07-28T12:39:15.1243038Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:103:31: error: request for member ‘release_index’ in something not a structure or union
2026-07-28T12:39:15.1244098Z   103 |     TEST_CHECK_EQ_U64(1U, fake.release_index);
2026-07-28T12:39:15.1244511Z       |                               ^
2026-07-28T12:39:15.1245502Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/support/test_assert.h:27:50: note: in definition of macro ‘TEST_CHECK_EQ_U64’
2026-07-28T12:39:15.1246503Z    27 |         const uint64_t test_actual_ = (uint64_t)(actual_value);                                    \
2026-07-28T12:39:15.1247092Z       |                                                  ^~~~~~~~~~~~
2026-07-28T12:39:15.1248572Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:110:9: error: request for member ‘wait_failure_on’ in something not a structure or union
2026-07-28T12:39:15.1249555Z   110 |     fake.wait_failure_on = 2U;
2026-07-28T12:39:15.1249875Z       |         ^
2026-07-28T12:39:15.1251125Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:111:9: error: request for member ‘wait_failure_result’ in something not a structure or union
2026-07-28T12:39:15.1252277Z   111 |     fake.wait_failure_result = APP_ERROR_IO;
2026-07-28T12:39:15.1252777Z       |         ^
2026-07-28T12:39:15.1253970Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:113:31: error: request for member ‘release_index’ in something not a structure or union
2026-07-28T12:39:15.1254938Z   113 |     TEST_CHECK_EQ_U64(2U, fake.release_index);
2026-07-28T12:39:15.1255271Z       |                               ^
2026-07-28T12:39:15.1256167Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/support/test_assert.h:27:50: note: in definition of macro ‘TEST_CHECK_EQ_U64’
2026-07-28T12:39:15.1256857Z    27 |         const uint64_t test_actual_ = (uint64_t)(actual_value);                                    \
2026-07-28T12:39:15.1257252Z       |                                                  ^~~~~~~~~~~~
2026-07-28T12:39:15.1257820Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc: At top level:
2026-07-28T12:39:15.1259304Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:117:6: error: no previous declaration for ‘executor_run_execution_tests’ [-Werror=missing-declarations]
2026-07-28T12:39:15.1259964Z   117 | void executor_run_execution_tests(void)
2026-07-28T12:39:15.1260223Z       |      ^~~~~~~~~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1260725Z In file included from /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_macro_executor.c:11:
2026-07-28T12:39:15.1261620Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc: In function ‘test_press_release_and_final_release_errors’:
2026-07-28T12:39:15.1262897Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc:3:5: error: unknown type name ‘executor_fake_t’; did you mean ‘execution_state_t’?
2026-07-28T12:39:15.1263539Z     3 |     executor_fake_t fake;
2026-07-28T12:39:15.1263749Z       |     ^~~~~~~~~~~~~~~
2026-07-28T12:39:15.1263945Z       |     execution_state_t
2026-07-28T12:39:15.1264895Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc:8:9: error: request for member ‘press_result’ in something not a structure or union
2026-07-28T12:39:15.1265637Z     8 |     fake.press_result = APP_ERROR_IO;
2026-07-28T12:39:15.1265865Z       |         ^
2026-07-28T12:39:15.1266756Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc:10:31: error: request for member ‘release_index’ in something not a structure or union
2026-07-28T12:39:15.1267367Z    10 |     TEST_CHECK_EQ_U64(2U, fake.release_index);
2026-07-28T12:39:15.1267626Z       |                               ^
2026-07-28T12:39:15.1268296Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/support/test_assert.h:27:50: note: in definition of macro ‘TEST_CHECK_EQ_U64’
2026-07-28T12:39:15.1268944Z    27 |         const uint64_t test_actual_ = (uint64_t)(actual_value);                                    \
2026-07-28T12:39:15.1269325Z       |                                                  ^~~~~~~~~~~~
2026-07-28T12:39:15.1270133Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc:16:9: error: request for member ‘release_results’ in something not a structure or union
2026-07-28T12:39:15.1270760Z    16 |     fake.release_results[0] = APP_ERROR_USB_NOT_READY;
2026-07-28T12:39:15.1271142Z       |         ^
2026-07-28T12:39:15.1271924Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc:17:9: error: request for member ‘release_result_count’ in something not a structure or union
2026-07-28T12:39:15.1272523Z    17 |     fake.release_result_count = 1U;
2026-07-28T12:39:15.1272941Z       |         ^
2026-07-28T12:39:15.1273756Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc:20:31: error: request for member ‘release_index’ in something not a structure or union
2026-07-28T12:39:15.1274391Z    20 |     TEST_CHECK_EQ_U64(2U, fake.release_index);
2026-07-28T12:39:15.1274647Z       |                               ^
2026-07-28T12:39:15.1275312Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/support/test_assert.h:27:50: note: in definition of macro ‘TEST_CHECK_EQ_U64’
2026-07-28T12:39:15.1275956Z    27 |         const uint64_t test_actual_ = (uint64_t)(actual_value);                                    \
2026-07-28T12:39:15.1276334Z       |                                                  ^~~~~~~~~~~~
2026-07-28T12:39:15.1277142Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc:29:9: error: request for member ‘press_result’ in something not a structure or union
2026-07-28T12:39:15.1277744Z    29 |     fake.press_result = APP_ERROR_IO;
2026-07-28T12:39:15.1277977Z       |         ^
2026-07-28T12:39:15.1278721Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc:30:9: error: request for member ‘release_results’ in something not a structure or union
2026-07-28T12:39:15.1279341Z    30 |     fake.release_results[1] = APP_ERROR_USB_NOT_READY;
2026-07-28T12:39:15.1279690Z       |         ^
2026-07-28T12:39:15.1280629Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc:31:9: error: request for member ‘release_result_count’ in something not a structure or union
2026-07-28T12:39:15.1281230Z    31 |     fake.release_result_count = 2U;
2026-07-28T12:39:15.1281449Z       |         ^
2026-07-28T12:39:15.1282456Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc:35:31: error: request for member ‘release_index’ in something not a structure or union
2026-07-28T12:39:15.1283517Z    35 |     TEST_CHECK_EQ_U64(2U, fake.release_index);
2026-07-28T12:39:15.1283867Z       |                               ^
2026-07-28T12:39:15.1284651Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/support/test_assert.h:27:50: note: in definition of macro ‘TEST_CHECK_EQ_U64’
2026-07-28T12:39:15.1285334Z    27 |         const uint64_t test_actual_ = (uint64_t)(actual_value);                                    \
2026-07-28T12:39:15.1285820Z       |                                                  ^~~~~~~~~~~~
2026-07-28T12:39:15.1286846Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc: In function ‘test_recovery_after_each_terminal_outcome’:
2026-07-28T12:39:15.1288390Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc:49:9: error: unknown type name ‘executor_fake_t’; did you mean ‘execution_state_t’?
2026-07-28T12:39:15.1289203Z    49 |         executor_fake_t fake;
2026-07-28T12:39:15.1289507Z       |         ^~~~~~~~~~~~~~~
2026-07-28T12:39:15.1289782Z       |         execution_state_t
2026-07-28T12:39:15.1290879Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc:57:17: error: request for member ‘press_result’ in something not a structure or union
2026-07-28T12:39:15.1291802Z    57 |             fake.press_result = APP_ERROR_IO;
2026-07-28T12:39:15.1292044Z       |                 ^
2026-07-28T12:39:15.1293102Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc:59:17: error: request for member ‘wait_failure_on’ in something not a structure or union
2026-07-28T12:39:15.1293706Z    59 |             fake.wait_failure_on = 1U;
2026-07-28T12:39:15.1293937Z       |                 ^
2026-07-28T12:39:15.1294934Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc:60:17: error: request for member ‘wait_failure_result’ in something not a structure or union
2026-07-28T12:39:15.1295866Z    60 |             fake.wait_failure_result = APP_ERROR_TIMEOUT;
2026-07-28T12:39:15.1296235Z       |                 ^
2026-07-28T12:39:15.1298223Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/../../firmware/components/web_server/web_server_adapter.h:63:18: error: conflicting types for ‘web_adapter_authorize_mutation’; have ‘app_error_code_t(app_error_code_t (*)(void *, const char *, char *, size_t), app_error_code_t (*)(void *, const char *, const char *), void *, char *, size_t)’ {aka ‘app_error_code_t(app_error_code_t (*)(void *, const char *, char *, long unsigned int), app_error_code_t (*)(void *, const char *, const char *), void *, char *, long unsigned int)’}
2026-07-28T12:39:15.1299670Z    63 | app_error_code_t web_adapter_authorize_mutation(web_adapter_get_header_fn get_header,
2026-07-28T12:39:15.1300072Z       |                  ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1300933Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_auth.inc:67:26: note: previous implicit declaration of ‘web_adapter_authorize_mutation’ with type ‘int()’
2026-07-28T12:39:15.1301634Z    67 |                          web_adapter_authorize_mutation(get_authorization_header,
2026-07-28T12:39:15.1301971Z       |                          ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1302487Z In file included from /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter.c:4:
2026-07-28T12:39:15.1303481Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_main.inc: In function ‘main’:
2026-07-28T12:39:15.1304734Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_main.inc:8:5: error: implicit declaration of function ‘test_static_streaming’; did you mean ‘test_fail_string’? [-Werror=implicit-function-declaration]
2026-07-28T12:39:15.1305494Z     8 |     test_static_streaming();
2026-07-28T12:39:15.1305739Z       |     ^~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1305955Z       |     test_fail_string
2026-07-28T12:39:15.1306437Z In file included from /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/web_security_cookie.inc:8,
2026-07-28T12:39:15.1307054Z                  from /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_security.c:2:
2026-07-28T12:39:15.1307758Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/../../firmware/components/web_server/web_content.h: At top level:
2026-07-28T12:39:15.1308883Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/../../firmware/components/web_server/web_content.h:6:13: error: conflicting types for ‘web_content_type’; have ‘const char *(const char *)’
2026-07-28T12:39:15.1309553Z     6 | const char *web_content_type(const char *path);
2026-07-28T12:39:15.1309827Z       |             ^~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1310605Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/web_security_content.inc:18:54: note: previous implicit declaration of ‘web_content_type’ with type ‘int()’
2026-07-28T12:39:15.1311330Z    18 |     TEST_CHECK_EQ_STRING("text/html; charset=utf-8", web_content_type("index.html"));
2026-07-28T12:39:15.1311715Z       |                                                      ^~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1312730Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/../../firmware/components/web_server/web_content.h:7:6: error: conflicting types for ‘web_accept_encoding_gzip’; have ‘_Bool(const char *)’
2026-07-28T12:39:15.1313534Z     7 | bool web_accept_encoding_gzip(const char *header);
2026-07-28T12:39:15.1313815Z       |      ^~~~~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1314611Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/web_security_content.inc:3:16: note: previous implicit declaration of ‘web_accept_encoding_gzip’ with type ‘int()’
2026-07-28T12:39:15.1315259Z     3 |     TEST_CHECK(web_accept_encoding_gzip("gzip"));
2026-07-28T12:39:15.1315537Z       |                ^~~~~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1316016Z In file included from /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_security.c:3:
2026-07-28T12:39:15.1316738Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/web_security_main.inc: In function ‘main’:
2026-07-28T12:39:15.1317922Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/web_security_main.inc:4:5: error: implicit declaration of function ‘test_origin_boundaries’; did you mean ‘test_cookie_boundaries’? [-Werror=implicit-function-declaration]
2026-07-28T12:39:15.1318627Z     4 |     test_origin_boundaries();
2026-07-28T12:39:15.1318856Z       |     ^~~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1319077Z       |     test_cookie_boundaries
2026-07-28T12:39:15.1320087Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/web_security_main.inc:5:5: error: implicit declaration of function ‘test_static_path_boundaries’; did you mean ‘test_content_boundaries’? [-Werror=implicit-function-declaration]
2026-07-28T12:39:15.1320816Z     5 |     test_static_path_boundaries();
2026-07-28T12:39:15.1321056Z       |     ^~~~~~~~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1321287Z       |     test_content_boundaries
2026-07-28T12:39:15.1321752Z In file included from /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_security.c:4:
2026-07-28T12:39:15.1322388Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/web_security_origin.inc: At top level:
2026-07-28T12:39:15.1323507Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/web_security_origin.inc:1:13: error: conflicting types for ‘test_origin_boundaries’; have ‘void(void)’ [-Werror]
2026-07-28T12:39:15.1324094Z     1 | static void test_origin_boundaries(void)
2026-07-28T12:39:15.1324354Z       |             ^~~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1325151Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/web_security_origin.inc:1:13: error: static declaration of ‘test_origin_boundaries’ follows non-static declaration
2026-07-28T12:39:15.1326270Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/web_security_main.inc:4:5: note: previous implicit declaration of ‘test_origin_boundaries’ with type ‘void(void)’
2026-07-28T12:39:15.1326840Z     4 |     test_origin_boundaries();
2026-07-28T12:39:15.1327069Z       |     ^~~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1327836Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc:66:35: error: request for member ‘free_count’ in something not a structure or union
2026-07-28T12:39:15.1328427Z    66 |         TEST_CHECK_EQ_U64(1U, fake.free_count);
2026-07-28T12:39:15.1328683Z       |                                   ^
2026-07-28T12:39:15.1329351Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/support/test_assert.h:27:50: note: in definition of macro ‘TEST_CHECK_EQ_U64’
2026-07-28T12:39:15.1330034Z    27 |         const uint64_t test_actual_ = (uint64_t)(actual_value);                                    \
2026-07-28T12:39:15.1330410Z       |                                                  ^~~~~~~~~~~~
2026-07-28T12:39:15.1331215Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc:68:13: error: request for member ‘press_result’ in something not a structure or union
2026-07-28T12:39:15.1331810Z    68 |         fake.press_result = APP_ERROR_NONE;
2026-07-28T12:39:15.1332046Z       |             ^
2026-07-28T12:39:15.1333041Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc:69:13: error: request for member ‘wait_failure_on’ in something not a structure or union
2026-07-28T12:39:15.1333649Z    69 |         fake.wait_failure_on = 0U;
2026-07-28T12:39:15.1333873Z       |             ^
2026-07-28T12:39:15.1334664Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc:70:13: error: request for member ‘wait_failure_result’ in something not a structure or union
2026-07-28T12:39:15.1335323Z    70 |         fake.wait_failure_result = APP_ERROR_NONE;
2026-07-28T12:39:15.1335563Z       |             ^
2026-07-28T12:39:15.1336301Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc:73:35: error: request for member ‘free_count’ in something not a structure or union
2026-07-28T12:39:15.1336884Z    73 |         TEST_CHECK_EQ_U64(2U, fake.free_count);
2026-07-28T12:39:15.1337145Z       |                                   ^
2026-07-28T12:39:15.1337808Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/support/test_assert.h:27:50: note: in definition of macro ‘TEST_CHECK_EQ_U64’
2026-07-28T12:39:15.1338432Z    27 |         const uint64_t test_actual_ = (uint64_t)(actual_value);                                    \
2026-07-28T12:39:15.1338822Z       |                                                  ^~~~~~~~~~~~
2026-07-28T12:39:15.1339542Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc: In function ‘test_cancel_and_status_lock_failures’:
2026-07-28T12:39:15.1340586Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc:80:5: error: unknown type name ‘executor_fake_t’; did you mean ‘execution_state_t’?
2026-07-28T12:39:15.1341149Z    80 |     executor_fake_t fake;
2026-07-28T12:39:15.1341359Z       |     ^~~~~~~~~~~~~~~
2026-07-28T12:39:15.1341566Z       |     execution_state_t
2026-07-28T12:39:15.1342311Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc:86:32: error: request for member ‘calls’ in something not a structure or union
2026-07-28T12:39:15.1343054Z    86 |     fake_call_log_fail_on(&fake.calls, "lock", 1U);
2026-07-28T12:39:15.1343325Z       |                                ^
2026-07-28T12:39:15.1344272Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc:88:5: error: implicit declaration of function ‘executor_clear_injected_failure’ [-Werror=implicit-function-declaration]
2026-07-28T12:39:15.1344969Z    88 |     executor_clear_injected_failure(&fake);
2026-07-28T12:39:15.1345231Z       |     ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1345993Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc:94:32: error: request for member ‘calls’ in something not a structure or union
2026-07-28T12:39:15.1346590Z    94 |     fake_call_log_fail_on(&fake.calls, "lock", 1U);
2026-07-28T12:39:15.1346859Z       |                                ^
2026-07-28T12:39:15.1347535Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc: In function ‘test_key_release_failure_after_success’:
2026-07-28T12:39:15.1348590Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc:104:5: error: unknown type name ‘executor_fake_t’; did you mean ‘execution_state_t’?
2026-07-28T12:39:15.1349201Z   104 |     executor_fake_t fake;
2026-07-28T12:39:15.1349408Z       |     ^~~~~~~~~~~~~~~
2026-07-28T12:39:15.1349601Z       |     execution_state_t
2026-07-28T12:39:15.1350495Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_main.inc:9:5: error: implicit declaration of function ‘test_server_lifecycle’ [-Werror=implicit-function-declaration]
2026-07-28T12:39:15.1351142Z     9 |     test_server_lifecycle();
2026-07-28T12:39:15.1351421Z       |     ^~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1351899Z In file included from /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter.c:5:
2026-07-28T12:39:15.1352715Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_stream_lifecycle.inc: At top level:
2026-07-28T12:39:15.1353837Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_stream_lifecycle.inc:66:13: error: conflicting types for ‘test_static_streaming’; have ‘void(void)’ [-Werror]
2026-07-28T12:39:15.1354484Z    66 | static void test_static_streaming(void) {
2026-07-28T12:39:15.1354736Z       |             ^~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1355596Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_stream_lifecycle.inc:66:13: error: static declaration of ‘test_static_streaming’ follows non-static declaration
2026-07-28T12:39:15.1356839Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_main.inc:8:5: note: previous implicit declaration of ‘test_static_streaming’ with type ‘void(void)’
2026-07-28T12:39:15.1357450Z     8 |     test_static_streaming();
2026-07-28T12:39:15.1357677Z       |     ^~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1358528Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_stream_lifecycle.inc:140:13: error: conflicting types for ‘test_server_lifecycle’; have ‘void(void)’ [-Werror]
2026-07-28T12:39:15.1359187Z   140 | static void test_server_lifecycle(void) {
2026-07-28T12:39:15.1359442Z       |             ^~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1360295Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_stream_lifecycle.inc:140:13: error: static declaration of ‘test_server_lifecycle’ follows non-static declaration
2026-07-28T12:39:15.1361507Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_main.inc:9:5: note: previous implicit declaration of ‘test_server_lifecycle’ with type ‘void(void)’
2026-07-28T12:39:15.1362098Z     9 |     test_server_lifecycle();
2026-07-28T12:39:15.1362317Z       |     ^~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1362874Z In file included from /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_security.c:5:
2026-07-28T12:39:15.1363867Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/web_security_static.inc:10:13: error: conflicting types for ‘test_static_path_boundaries’; have ‘void(void)’ [-Werror]
2026-07-28T12:39:15.1364493Z    10 | static void test_static_path_boundaries(void)
2026-07-28T12:39:15.1364765Z       |             ^~~~~~~~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1365573Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/web_security_static.inc:10:13: error: static declaration of ‘test_static_path_boundaries’ follows non-static declaration
2026-07-28T12:39:15.1366728Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/web_security_main.inc:5:5: note: previous implicit declaration of ‘test_static_path_boundaries’ with type ‘void(void)’
2026-07-28T12:39:15.1367318Z     5 |     test_static_path_boundaries();
2026-07-28T12:39:15.1367556Z       |     ^~~~~~~~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1368337Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/web_security_static.inc:10:13: error: ‘test_static_path_boundaries’ defined but not used [-Werror=unused-function]
2026-07-28T12:39:15.1368973Z    10 | static void test_static_path_boundaries(void)
2026-07-28T12:39:15.1369250Z       |             ^~~~~~~~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1370017Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/web_security_origin.inc:1:13: error: ‘test_origin_boundaries’ defined but not used [-Werror=unused-function]
2026-07-28T12:39:15.1370595Z     1 | static void test_origin_boundaries(void)
2026-07-28T12:39:15.1370930Z       |             ^~~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1371196Z cc1: all warnings being treated as errors
2026-07-28T12:39:15.1371991Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc:111:9: error: request for member ‘release_results’ in something not a structure or union
2026-07-28T12:39:15.1372710Z   111 |     fake.release_results[0] = APP_ERROR_NONE;
2026-07-28T12:39:15.1373003Z       |         ^
2026-07-28T12:39:15.1373769Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc:112:9: error: request for member ‘release_results’ in something not a structure or union
2026-07-28T12:39:15.1374406Z   112 |     fake.release_results[1] = APP_ERROR_USB_NOT_READY;
2026-07-28T12:39:15.1374662Z       |         ^
2026-07-28T12:39:15.1375435Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc:113:9: error: request for member ‘release_result_count’ in something not a structure or union
2026-07-28T12:39:15.1376027Z   113 |     fake.release_result_count = 2U;
2026-07-28T12:39:15.1376244Z       |         ^
2026-07-28T12:39:15.1376977Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc:117:31: error: request for member ‘release_index’ in something not a structure or union
2026-07-28T12:39:15.1377583Z   117 |     TEST_CHECK_EQ_U64(2U, fake.release_index);
2026-07-28T12:39:15.1377847Z       |                               ^
2026-07-28T12:39:15.1378502Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/support/test_assert.h:27:50: note: in definition of macro ‘TEST_CHECK_EQ_U64’
2026-07-28T12:39:15.1379149Z    27 |         const uint64_t test_actual_ = (uint64_t)(actual_value);                                    \
2026-07-28T12:39:15.1379529Z       |                                                  ^~~~~~~~~~~~
2026-07-28T12:39:15.1380317Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc: In function ‘test_terminal_publish_failure_leaves_executor_unavailable’:
2026-07-28T12:39:15.1381410Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc:124:5: error: unknown type name ‘executor_fake_t’; did you mean ‘execution_state_t’?
2026-07-28T12:39:15.1381977Z   124 |     executor_fake_t fake;
2026-07-28T12:39:15.1382189Z       |     ^~~~~~~~~~~~~~~
2026-07-28T12:39:15.1382385Z       |     execution_state_t
2026-07-28T12:39:15.1383027Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc:133:41: error: invalid initializer
2026-07-28T12:39:15.1383574Z   133 |     macro_execution_request_t request = executor_make_request(1U);
2026-07-28T12:39:15.1383915Z       |                                         ^~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1384718Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc:136:32: error: request for member ‘calls’ in something not a structure or union
2026-07-28T12:39:15.1385310Z   136 |     fake_call_log_fail_on(&fake.calls, "lock", 4U);
2026-07-28T12:39:15.1385579Z       |                                ^
2026-07-28T12:39:15.1386108Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc:142:42: error: invalid initializer
2026-07-28T12:39:15.1386646Z   142 |     macro_execution_request_t rejected = executor_make_request(1U);
2026-07-28T12:39:15.1387017Z       |                                          ^~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1387960Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc:146:5: error: implicit declaration of function ‘executor_free_unowned_request’ [-Werror=implicit-function-declaration]
2026-07-28T12:39:15.1388654Z   146 |     executor_free_unowned_request(&rejected);
2026-07-28T12:39:15.1388908Z       |     ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1389427Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc: At top level:
2026-07-28T12:39:15.1390458Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc:149:6: error: no previous declaration for ‘executor_run_terminal_tests’ [-Werror=missing-declarations]
2026-07-28T12:39:15.1391083Z   149 | void executor_run_terminal_tests(void)
2026-07-28T12:39:15.1391362Z       |      ^~~~~~~~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1391846Z In file included from /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_macro_executor.c:12:
2026-07-28T12:39:15.1392959Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_test_fixture.h:36:6: error: conflicting types for ‘executor_fake_reset’; have ‘void(executor_fake_t *)’ [-Werror]
2026-07-28T12:39:15.1393604Z    36 | void executor_fake_reset(executor_fake_t *fake);
2026-07-28T12:39:15.1393866Z       |      ^~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1394713Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:4:5: note: previous implicit declaration of ‘executor_fake_reset’ with type ‘void(executor_fake_t *)’
2026-07-28T12:39:15.1395327Z     4 |     executor_fake_reset(&fake);
2026-07-28T12:39:15.1395554Z       |     ^~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1396395Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_test_fixture.h:37:6: error: conflicting types for ‘executor_clear_injected_failure’; have ‘void(executor_fake_t *)’ [-Werror]
2026-07-28T12:39:15.1397075Z    37 | void executor_clear_injected_failure(executor_fake_t *fake);
2026-07-28T12:39:15.1397378Z       |      ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1398269Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc:88:5: note: previous implicit declaration of ‘executor_clear_injected_failure’ with type ‘void(executor_fake_t *)’
2026-07-28T12:39:15.1398940Z    88 |     executor_clear_injected_failure(&fake);
2026-07-28T12:39:15.1399203Z       |     ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1400112Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_test_fixture.h:39:6: error: conflicting types for ‘executor_init_engine’; have ‘void(macro_executor_engine_t *, executor_fake_t *)’ [-Werror]
2026-07-28T12:39:15.1400878Z    39 | void executor_init_engine(macro_executor_engine_t *engine, executor_fake_t *fake);
2026-07-28T12:39:15.1401222Z       |      ^~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1402151Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:6:5: note: previous implicit declaration of ‘executor_init_engine’ with type ‘void(macro_executor_engine_t *, executor_fake_t *)’
2026-07-28T12:39:15.1403024Z     6 |     executor_init_engine(&engine, &fake);
2026-07-28T12:39:15.1403277Z       |     ^~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1404287Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_test_fixture.h:40:27: error: conflicting types for ‘executor_make_request’; have ‘macro_execution_request_t(size_t)’ {aka ‘macro_execution_request_t(long unsigned int)’}
2026-07-28T12:39:15.1405096Z    40 | macro_execution_request_t executor_make_request(size_t action_count);
2026-07-28T12:39:15.1405435Z       |                           ^~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1406255Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:7:41: note: previous implicit declaration of ‘executor_make_request’ with type ‘int()’
2026-07-28T12:39:15.1406952Z     7 |     macro_execution_request_t request = executor_make_request(3U);
2026-07-28T12:39:15.1407295Z       |                                         ^~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1408251Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_test_fixture.h:41:6: error: conflicting types for ‘executor_free_unowned_request’; have ‘void(macro_execution_request_t *)’ [-Werror]
2026-07-28T12:39:15.1408978Z    41 | void executor_free_unowned_request(macro_execution_request_t *request);
2026-07-28T12:39:15.1409298Z       |      ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1410219Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_terminal_tests.inc:146:5: note: previous implicit declaration of ‘executor_free_unowned_request’ with type ‘void(macro_execution_request_t *)’
2026-07-28T12:39:15.1410945Z   146 |     executor_free_unowned_request(&rejected);
2026-07-28T12:39:15.1411203Z       |     ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1412138Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_test_fixture.h:42:18: error: conflicting types for ‘executor_execute_queued’; have ‘app_error_code_t(macro_executor_engine_t *, executor_fake_t *)’
2026-07-28T12:39:15.1413078Z    42 | app_error_code_t executor_execute_queued(macro_executor_engine_t *engine, executor_fake_t *fake);
2026-07-28T12:39:15.1413476Z       |                  ^~~~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1414297Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:17:42: note: previous implicit declaration of ‘executor_execute_queued’ with type ‘int()’
2026-07-28T12:39:15.1415002Z    17 |     TEST_CHECK_APP_ERROR(APP_ERROR_NONE, executor_execute_queued(&engine, &fake));
2026-07-28T12:39:15.1415371Z       |                                          ^~~~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1416066Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/support/test_assert.h:27:50: note: in definition of macro ‘TEST_CHECK_EQ_U64’
2026-07-28T12:39:15.1416697Z    27 |         const uint64_t test_actual_ = (uint64_t)(actual_value);                                    \
2026-07-28T12:39:15.1417073Z       |                                                  ^~~~~~~~~~~~
2026-07-28T12:39:15.1417759Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/support/test_assert.h:64:5: note: in expansion of macro ‘TEST_CHECK_EQ_INT’
2026-07-28T12:39:15.1418292Z    64 |     TEST_CHECK_EQ_INT((expected_value), (actual_value))
2026-07-28T12:39:15.1418552Z       |     ^~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1419229Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:17:5: note: in expansion of macro ‘TEST_CHECK_APP_ERROR’
2026-07-28T12:39:15.1419852Z    17 |     TEST_CHECK_APP_ERROR(APP_ERROR_NONE, executor_execute_queued(&engine, &fake));
2026-07-28T12:39:15.1420188Z       |     ^~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1421391Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_test_fixture.h:43:6: error: conflicting types for ‘executor_submit_single_key’; have ‘void(macro_executor_engine_t *, executor_fake_t *, uint8_t)’ {aka ‘void(macro_executor_engine_t *, executor_fake_t *, unsigned char)’} [-Werror]
2026-07-28T12:39:15.1422369Z    43 | void executor_submit_single_key(macro_executor_engine_t *engine, executor_fake_t *fake,
2026-07-28T12:39:15.1422870Z       |      ^~~~~~~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1424105Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:57:5: note: previous implicit declaration of ‘executor_submit_single_key’ with type ‘void(macro_executor_engine_t *, executor_fake_t *, uint8_t)’ {aka ‘void(macro_executor_engine_t *, executor_fake_t *, unsigned char)’}
2026-07-28T12:39:15.1425044Z    57 |     executor_submit_single_key(&engine, &fake, 4U);
2026-07-28T12:39:15.1425314Z       |     ^~~~~~~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1426352Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_test_fixture.h:45:6: error: conflicting types for ‘executor_assert_terminal’; have ‘void(const macro_executor_engine_t *, execution_state_t,  app_error_code_t,  _Bool)’ [-Werror]
2026-07-28T12:39:15.1427256Z    45 | void executor_assert_terminal(const macro_executor_engine_t *engine, execution_state_t state,
2026-07-28T12:39:15.1427680Z       |      ^~~~~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1428760Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:48:5: note: previous implicit declaration of ‘executor_assert_terminal’ with type ‘void(const macro_executor_engine_t *, execution_state_t,  app_error_code_t,  _Bool)’
2026-07-28T12:39:15.1429652Z    48 |     executor_assert_terminal(&engine, EXECUTION_COMPLETED, APP_ERROR_NONE, false);
2026-07-28T12:39:15.1429995Z       |     ^~~~~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1431266Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_test_fixture.h:47:6: error: conflicting types for ‘executor_assert_relevant_call’; have ‘void(const executor_fake_t *, size_t,  const char *, uint64_t)’ {aka ‘void(const executor_fake_t *, long unsigned int,  const char *, long unsigned int)’} [-Werror]
2026-07-28T12:39:15.1432305Z    47 | void executor_assert_relevant_call(const executor_fake_t *fake, size_t ordinal, const char *name,
2026-07-28T12:39:15.1432808Z       |      ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1434119Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/executor_execution_tests.inc:19:5: note: previous implicit declaration of ‘executor_assert_relevant_call’ with type ‘void(const executor_fake_t *, size_t,  const char *, uint64_t)’ {aka ‘void(const executor_fake_t *, long unsigned int,  const char *, long unsigned int)’}
2026-07-28T12:39:15.1435074Z    19 |     executor_assert_relevant_call(&fake, 0U, "press", 2U);
2026-07-28T12:39:15.1435354Z       |     ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1436199Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_stream_lifecycle.inc:140:13: error: ‘test_server_lifecycle’ defined but not used [-Werror=unused-function]
2026-07-28T12:39:15.1436844Z   140 | static void test_server_lifecycle(void) {
2026-07-28T12:39:15.1437098Z       |             ^~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1437934Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_stream_lifecycle.inc:66:13: error: ‘test_static_streaming’ defined but not used [-Werror=unused-function]
2026-07-28T12:39:15.1438555Z    66 | static void test_static_streaming(void) {
2026-07-28T12:39:15.1438806Z       |             ^~~~~~~~~~~~~~~~~~~~~
2026-07-28T12:39:15.1439557Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/tests/host/test_web_server_adapter_auth.inc:10:13: error: ‘append_call’ defined but not used [-Werror=unused-function]
2026-07-28T12:39:15.1440201Z    10 | static void append_call(authorization_fake_t *fake, const char *name)
2026-07-28T12:39:15.1440512Z       |             ^~~~~~~~~~~
2026-07-28T12:39:15.1440755Z cc1: all warnings being treated as errors
2026-07-28T12:39:15.1441275Z gmake[2]: *** [CMakeFiles/web_security_tests.dir/build.make:79: CMakeFiles/web_security_tests.dir/test_web_security.c.o] Error 1
2026-07-28T12:39:15.1441743Z gmake[2]: *** Waiting for unfinished jobs....
2026-07-28T12:39:15.1442306Z gmake[2]: *** [CMakeFiles/web_server_adapter_tests.dir/build.make:79: CMakeFiles/web_server_adapter_tests.dir/test_web_server_adapter.c.o] Error 1
2026-07-28T12:39:15.1443007Z gmake[2]: *** Waiting for unfinished jobs....
2026-07-28T12:39:15.1443284Z cc1: all warnings being treated as errors
2026-07-28T12:39:15.1443810Z gmake[2]: *** [CMakeFiles/macro_executor_tests.dir/build.make:79: CMakeFiles/macro_executor_tests.dir/test_macro_executor.c.o] Error 1
2026-07-28T12:39:15.1444320Z gmake[2]: *** Waiting for unfinished jobs....
2026-07-28T12:39:15.1444719Z gmake[1]: *** [CMakeFiles/Makefile2:358: CMakeFiles/macro_executor_tests.dir/all] Error 2
2026-07-28T12:39:15.1445092Z gmake[1]: *** Waiting for unfinished jobs....
2026-07-28T12:39:15.1445484Z gmake[1]: *** [CMakeFiles/Makefile2:422: CMakeFiles/web_security_tests.dir/all] Error 2
2026-07-28T12:39:15.1446308Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c: In function ‘parse_data’:
2026-07-28T12:39:15.1447294Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c:34:72: error: ‘false’ undeclared (first use in this function)
2026-07-28T12:39:15.1447944Z    34 |     cJSON *value = cJSON_ParseWithLengthOpts(json, length, &parse_end, false);
2026-07-28T12:39:15.1448346Z       |                                                                        ^~~~~
2026-07-28T12:39:15.1449250Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c:9:1: note: ‘false’ is defined in header ‘<stdbool.h>’; did you forget to ‘#include <stdbool.h>’?
2026-07-28T12:39:15.1449859Z     8 | #include "cJSON.h"
2026-07-28T12:39:15.1450066Z   +++ |+#include <stdbool.h>
2026-07-28T12:39:15.1450259Z     9 |
2026-07-28T12:39:15.1450968Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c:34:72: note: each undeclared identifier is reported only once for each function it appears in
2026-07-28T12:39:15.1451690Z    34 |     cJSON *value = cJSON_ParseWithLengthOpts(json, length, &parse_end, false);
2026-07-28T12:39:15.1452058Z       |                                                                        ^~~~~
2026-07-28T12:39:15.1452840Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c: In function ‘web_api_response_success’:
2026-07-28T12:39:15.1453877Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c:52:76: error: ‘true’ undeclared (first use in this function)
2026-07-28T12:39:15.1454518Z    52 |     if (data == NULL || root == NULL || !cJSON_AddBoolToObject(root, "ok", true) ||
2026-07-28T12:39:15.1454888Z       |                                                                            ^~~~
2026-07-28T12:39:15.1455802Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c:52:76: note: ‘true’ is defined in header ‘<stdbool.h>’; did you forget to ‘#include <stdbool.h>’?
2026-07-28T12:39:15.1456875Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c: In function ‘web_api_response_error’:
2026-07-28T12:39:15.1457888Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c:72:77: error: ‘false’ undeclared (first use in this function)
2026-07-28T12:39:15.1458537Z    72 |     if (root == NULL || error == NULL || !cJSON_AddBoolToObject(root, "ok", false) ||
2026-07-28T12:39:15.1458910Z       |                                                                             ^~~~~
2026-07-28T12:39:15.1459805Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c:72:77: note: ‘false’ is defined in header ‘<stdbool.h>’; did you forget to ‘#include <stdbool.h>’?
2026-07-28T12:39:15.1461051Z gmake[2]: *** [CMakeFiles/web_api_response_tests.dir/build.make:107: CMakeFiles/web_api_response_tests.dir/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c.o] Error 1
2026-07-28T12:39:15.1461889Z gmake[1]: *** [CMakeFiles/Makefile2:1190: CMakeFiles/web_api_response_tests.dir/all] Error 2
2026-07-28T12:39:15.1462396Z gmake[1]: *** [CMakeFiles/Makefile2:454: CMakeFiles/web_server_adapter_tests.dir/all] Error 2
2026-07-28T12:39:15.1462958Z [ 99%] Built target storage_repository_tests
2026-07-28T12:39:15.1463253Z gmake: *** [Makefile:101: all] Error 2
2026-07-28T12:39:15.1463532Z [ 99%] Built target storage_active_set_delete_tests
2026-07-28T12:39:15.1472817Z ##[error]Process completed with exit code 2.
2026-07-28T12:39:15.1580386Z Node 20 is being deprecated. This workflow is running with Node 24 by default. If you need to temporarily use Node 20, you can set the ACTIONS_ALLOW_USE_UNSECURE_NODE_VERSION=true environment variable. For more information see: https://github.blog/changelog/2025-09-19-deprecation-of-node-20-on-github-actions-runners/
2026-07-28T12:39:15.1581282Z Post job cleanup.
2026-07-28T12:39:15.2232848Z [command]/usr/bin/git version
2026-07-28T12:39:15.2258512Z git version 2.54.0
2026-07-28T12:39:15.2284719Z Temporarily overriding HOME='/home/runner/work/_temp/4bde72b0-f610-496b-8301-972220b09db8' before making global git config changes
2026-07-28T12:39:15.2285428Z Adding repository directory to the temporary git global config as a safe directory
2026-07-28T12:39:15.2288573Z [command]/usr/bin/git config --global --add safe.directory /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard
2026-07-28T12:39:15.2314317Z [command]/usr/bin/git config --local --name-only --get-regexp core\.sshCommand
2026-07-28T12:39:15.2338272Z [command]/usr/bin/git submodule foreach --recursive sh -c "git config --local --name-only --get-regexp 'core\.sshCommand' && git config --local --unset-all 'core.sshCommand' || :"
2026-07-28T12:39:15.2493077Z [command]/usr/bin/git config --local --name-only --get-regexp http\.https\:\/\/github\.com\/\.extraheader
2026-07-28T12:39:15.2508199Z http.https://github.com/.extraheader
2026-07-28T12:39:15.2516099Z [command]/usr/bin/git config --local --unset-all http.https://github.com/.extraheader
2026-07-28T12:39:15.2537962Z [command]/usr/bin/git submodule foreach --recursive sh -c "git config --local --name-only --get-regexp 'http\.https\:\/\/github\.com\/\.extraheader' && git config --local --unset-all 'http.https://github.com/.extraheader' || :"
2026-07-28T12:39:15.2691270Z [command]/usr/bin/git config --local --name-only --get-regexp ^includeIf\.gitdir:
2026-07-28T12:39:15.2713394Z [command]/usr/bin/git submodule foreach --recursive git config --local --show-origin --name-only --get-regexp remote.origin.url
2026-07-28T12:39:15.2955932Z Cleaning up orphan processes
2026-07-28T12:39:15.3174140Z ##[warning]Node.js 20 is deprecated. The following actions target Node.js 20 but are being forced to run on Node.js 24: actions/checkout@v4, actions/setup-node@v4. For more information see: https://github.blog/changelog/2025-09-19-deprecation-of-node-20-on-github-actions-runners/
```
