# Phase 16 quality failure

Run: `30352059484`
Job: `90251632269`
Head: `96867411d5895d39339b48157661fea50fdd801f`

```text
2026-07-28T10:52:03.3349155Z     const macro_compile_options_t *options, macro_plan_t *out_plan,
2026-07-28T10:52:03.3349544Z                                            ^
2026-07-28T10:52:03.3350354Z firmware/components/web_server/web_execution_submit.h:38:68: error: code should be clang-formatted [-Wclang-format-violations]
2026-07-28T10:52:03.3351011Z     const macro_compile_options_t *options, macro_plan_t *out_plan,
2026-07-28T10:52:03.3351414Z                                                                    ^
2026-07-28T10:52:03.3352112Z firmware/components/web_server/web_execution_submit.h:41:74: error: code should be clang-formatted [-Wclang-format-violations]
2026-07-28T10:52:03.3352845Z typedef app_error_code_t (*web_execution_uuid_generate_fn)(void *context,
2026-07-28T10:52:03.3353287Z                                                                          ^
2026-07-28T10:52:03.3353994Z firmware/components/web_server/web_execution_submit.h:43:67: error: code should be clang-formatted [-Wclang-format-violations]
2026-07-28T10:52:03.3355054Z typedef app_error_code_t (*web_execution_submit_fn)(void *context,
2026-07-28T10:52:03.3355476Z                                                                   ^
2026-07-28T10:52:03.3356193Z firmware/components/web_server/web_execution_submit.h:56:49: error: code should be clang-formatted [-Wclang-format-violations]
2026-07-28T10:52:03.3356841Z app_error_code_t web_execution_submit_persisted(
2026-07-28T10:52:03.3357172Z                                                 ^
2026-07-28T10:52:03.3357885Z firmware/components/web_server/web_execution_submit.h:57:51: error: code should be clang-formatted [-Wclang-format-violations]
2026-07-28T10:52:03.3358766Z     const web_execution_submit_request_t *request, const web_execution_ops_t *operations,
2026-07-28T10:52:03.3359239Z                                                   ^
2026-07-28T10:52:03.3359915Z firmware/components/web_server/web_execution_submit.h:57:90: error: code should be clang-formatted [-Wclang-format-violations]
2026-07-28T10:52:03.3360660Z     const web_execution_submit_request_t *request, const web_execution_ops_t *operations,
2026-07-28T10:52:03.3361179Z                                                                                          ^
2026-07-28T10:52:03.3361912Z firmware/components/web_server/web_execution_submit.h:58:44: error: code should be clang-formatted [-Wclang-format-violations]
2026-07-28T10:52:03.3362633Z     web_execution_accepted_t *out_accepted, macro_parse_error_t *out_parse_error);
2026-07-28T10:52:03.3363063Z                                            ^
2026-07-28T10:52:03.3390284Z firmware/components/web_server/web_request_policy.c:36:77: error: code should be clang-formatted [-Wclang-format-violations]
2026-07-28T10:52:03.3391725Z                                              const char *name, char *buffer,
2026-07-28T10:52:03.3392558Z                                                                             ^
2026-07-28T10:52:03.3394348Z firmware/components/web_server/web_request_policy.c:40:74: error: code should be clang-formatted [-Wclang-format-violations]
2026-07-28T10:52:03.3395857Z     return result == APP_ERROR_NONE && buffer[0] != '\0' ? APP_ERROR_NONE
2026-07-28T10:52:03.3396796Z                                                                          ^
2026-07-28T10:52:03.3398374Z firmware/components/web_server/web_request_policy.c:48:43: error: code should be clang-formatted [-Wclang-format-violations]
2026-07-28T10:52:03.3399677Z     const app_error_code_t header_result = operations->get_header(
2026-07-28T10:52:03.3400317Z                                           ^
2026-07-28T10:52:03.3401520Z firmware/components/web_server/web_request_policy.c:48:67: error: code should be clang-formatted [-Wclang-format-violations]
2026-07-28T10:52:03.3402680Z     const app_error_code_t header_result = operations->get_header(
2026-07-28T10:52:03.3403137Z                                                                   ^
2026-07-28T10:52:03.3403835Z firmware/components/web_server/web_request_policy.c:60:45: error: code should be clang-formatted [-Wclang-format-violations]
2026-07-28T10:52:03.3404716Z     if (generate_result != APP_ERROR_NONE ||
2026-07-28T10:52:03.3405205Z                                             ^
2026-07-28T10:52:03.3405871Z firmware/components/web_server/web_request_policy.c:68:46: error: code should be clang-formatted [-Wclang-format-violations]
2026-07-28T10:52:03.3406484Z app_error_code_t web_request_policy_evaluate(
2026-07-28T10:52:03.3406802Z                                              ^
2026-07-28T10:52:03.3407464Z firmware/components/web_server/web_request_policy.c:69:45: error: code should be clang-formatted [-Wclang-format-violations]
2026-07-28T10:52:03.3408196Z     const web_request_policy_input_t *input, const web_request_policy_ops_t *operations,
2026-07-28T10:52:03.3408644Z                                             ^
2026-07-28T10:52:03.3409301Z firmware/components/web_server/web_request_policy.c:69:89: error: code should be clang-formatted [-Wclang-format-violations]
2026-07-28T10:52:03.3410105Z     const web_request_policy_input_t *input, const web_request_policy_ops_t *operations,
2026-07-28T10:52:03.3410673Z                                                                                         ^
2026-07-28T10:52:03.3412002Z firmware/components/web_server/web_request_policy.c:70:45: error: code should be clang-formatted [-Wclang-format-violations]
2026-07-28T10:52:03.3413310Z     web_request_policy_result_t *out_result, web_request_policy_failure_t *out_failure) {
2026-07-28T10:52:03.3414085Z                                             ^
2026-07-28T10:52:03.3415649Z firmware/components/web_server/web_request_policy.c:126:44: error: code should be clang-formatted [-Wclang-format-violations]
2026-07-28T10:52:03.3416414Z         const app_error_code_t validation = operations->validate_session(
2026-07-28T10:52:03.3416805Z                                            ^
2026-07-28T10:52:03.3417489Z firmware/components/web_server/web_request_policy.c:126:74: error: code should be clang-formatted [-Wclang-format-violations]
2026-07-28T10:52:03.3418172Z         const app_error_code_t validation = operations->validate_session(
2026-07-28T10:52:03.3418607Z                                                                          ^
2026-07-28T10:52:03.3419315Z firmware/components/web_server/web_request_policy.c:139:49: error: code should be clang-formatted [-Wclang-format-violations]
2026-07-28T10:52:03.3419905Z             return fail(out_result, out_failure,
2026-07-28T10:52:03.3420224Z                                                 ^
2026-07-28T10:52:03.3420893Z firmware/components/web_server/web_request_policy.c:140:74: error: code should be clang-formatted [-Wclang-format-violations]
2026-07-28T10:52:03.3421634Z                         WEB_REQUEST_POLICY_FAILURE_PHYSICAL_CONFIRMATION, APP_ERROR_INTERNAL);
2026-07-28T10:52:03.3422108Z                                                                          ^
2026-07-28T10:52:03.3422816Z firmware/components/web_server/web_request_policy.c:144:49: error: code should be clang-formatted [-Wclang-format-violations]
2026-07-28T10:52:03.3423411Z             return fail(out_result, out_failure,
2026-07-28T10:52:03.3423724Z                                                 ^
2026-07-28T10:52:03.3424670Z firmware/components/web_server/web_request_policy.c:145:74: error: code should be clang-formatted [-Wclang-format-violations]
2026-07-28T10:52:03.3425363Z                         WEB_REQUEST_POLICY_FAILURE_PHYSICAL_CONFIRMATION, result);
2026-07-28T10:52:03.3425788Z                                                                          ^
2026-07-28T10:52:03.3426505Z firmware/components/web_server/web_request_policy.h:49:46: error: code should be clang-formatted [-Wclang-format-violations]
2026-07-28T10:52:03.3427125Z app_error_code_t web_request_policy_evaluate(
2026-07-28T10:52:03.3427440Z                                              ^
2026-07-28T10:52:03.3428102Z firmware/components/web_server/web_request_policy.h:50:45: error: code should be clang-formatted [-Wclang-format-violations]
2026-07-28T10:52:03.3428830Z     const web_request_policy_input_t *input, const web_request_policy_ops_t *operations,
2026-07-28T10:52:03.3429373Z                                             ^
2026-07-28T10:52:03.3430031Z firmware/components/web_server/web_request_policy.h:50:89: error: code should be clang-formatted [-Wclang-format-violations]
2026-07-28T10:52:03.3430747Z     const web_request_policy_input_t *input, const web_request_policy_ops_t *operations,
2026-07-28T10:52:03.3431250Z                                                                                         ^
2026-07-28T10:52:03.3431963Z firmware/components/web_server/web_request_policy.h:51:45: error: code should be clang-formatted [-Wclang-format-violations]
2026-07-28T10:52:03.3432684Z     web_request_policy_result_t *out_result, web_request_policy_failure_t *out_failure);
2026-07-28T10:52:03.3433116Z                                             ^
2026-07-28T10:52:03.4184580Z ##[error]Process completed with exit code 1.
2026-07-28T10:52:03.4325432Z Node 20 is being deprecated. This workflow is running with Node 24 by default. If you need to temporarily use Node 20, you can set the ACTIONS_ALLOW_USE_UNSECURE_NODE_VERSION=true environment variable. For more information see: https://github.blog/changelog/2025-09-19-deprecation-of-node-20-on-github-actions-runners/
2026-07-28T10:52:03.4326735Z Post job cleanup.
2026-07-28T10:52:03.5188826Z [command]/usr/bin/git version
2026-07-28T10:52:03.5225958Z git version 2.54.0
2026-07-28T10:52:03.5264567Z Temporarily overriding HOME='/home/runner/work/_temp/519d706b-246e-4abd-b930-e29cb7c41082' before making global git config changes
2026-07-28T10:52:03.5265572Z Adding repository directory to the temporary git global config as a safe directory
2026-07-28T10:52:03.5269647Z [command]/usr/bin/git config --global --add safe.directory /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard
2026-07-28T10:52:03.5309058Z [command]/usr/bin/git config --local --name-only --get-regexp core\.sshCommand
2026-07-28T10:52:03.5343361Z [command]/usr/bin/git submodule foreach --recursive sh -c "git config --local --name-only --get-regexp 'core\.sshCommand' && git config --local --unset-all 'core.sshCommand' || :"
2026-07-28T10:52:03.5568513Z [command]/usr/bin/git config --local --name-only --get-regexp http\.https\:\/\/github\.com\/\.extraheader
2026-07-28T10:52:03.5594371Z http.https://github.com/.extraheader
2026-07-28T10:52:03.5605521Z [command]/usr/bin/git config --local --unset-all http.https://github.com/.extraheader
2026-07-28T10:52:03.5636584Z [command]/usr/bin/git submodule foreach --recursive sh -c "git config --local --name-only --get-regexp 'http\.https\:\/\/github\.com\/\.extraheader' && git config --local --unset-all 'http.https://github.com/.extraheader' || :"
2026-07-28T10:52:03.5856080Z [command]/usr/bin/git config --local --name-only --get-regexp ^includeIf\.gitdir:
2026-07-28T10:52:03.5887944Z [command]/usr/bin/git submodule foreach --recursive git config --local --show-origin --name-only --get-regexp remote.origin.url
2026-07-28T10:52:03.6243615Z Cleaning up orphan processes
2026-07-28T10:52:03.6596467Z ##[warning]Node.js 20 is deprecated. The following actions target Node.js 20 but are being forced to run on Node.js 24: actions/checkout@v4, actions/setup-node@v4. For more information see: https://github.blog/changelog/2025-09-19-deprecation-of-node-20-on-github-actions-runners/
```
