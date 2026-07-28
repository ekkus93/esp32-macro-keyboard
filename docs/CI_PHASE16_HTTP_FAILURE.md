# Phase 16 HTTP integration failure

## Error index

```text
108785:2026-07-28T13:01:33.0061451Z error: run-clang-tidy failed for /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware with status 1
109059:2026-07-28T13:01:33.0168743Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_json.c:117:73: error: 2 adjacent parameters of 'web_api_json_parse_resource_mutation' of similar type ('size_t') are easily swapped by mistake [bugprone-easily-swappable-parameters,-warnings-as-errors]
109070:2026-07-28T13:01:33.0177787Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_json.c:161:66: error: 2 adjacent parameters of 'web_api_json_parse_uuid_order' of similar type ('size_t') are easily swapped by mistake [bugprone-easily-swappable-parameters,-warnings-as-errors]
109087:2026-07-28T13:01:33.0191036Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_macros.c:22:29: error: no header providing "WEB_API_ROUTE_GLOBAL_MACROS" is directly included [misc-include-cleaner,-warnings-as-errors]
109090:2026-07-28T13:01:33.0193459Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_macros.c:23:29: error: no header providing "WEB_API_ROUTE_GLOBAL_MACRO" is directly included [misc-include-cleaner,-warnings-as-errors]
109093:2026-07-28T13:01:33.0195866Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_macros.c:24:29: error: no header providing "WEB_API_ROUTE_GLOBAL_MACRO_VALIDATE" is directly included [misc-include-cleaner,-warnings-as-errors]
109096:2026-07-28T13:01:33.0198646Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_macros.c:25:29: error: no header providing "WEB_API_ROUTE_GLOBAL_MACRO_DUPLICATE" is directly included [misc-include-cleaner,-warnings-as-errors]
109099:2026-07-28T13:01:33.0201105Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_macros.c:26:29: error: no header providing "WEB_API_ROUTE_GLOBAL_MACROS_REORDER" is directly included [misc-include-cleaner,-warnings-as-errors]
109102:2026-07-28T13:01:33.0203456Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_macros.c:66:25: error: no header providing "WEB_API_METHOD_GET" is directly included [misc-include-cleaner,-warnings-as-errors]
109105:2026-07-28T13:01:33.0205875Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_macros.c:75:61: error: 200U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
109108:2026-07-28T13:01:33.0208591Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_macros.c:106:35: error: 201U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
109111:2026-07-28T13:01:33.0211008Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_macros.c:124:16: error: variable name 'id' is too short, expected at least 3 characters [readability-identifier-length,-warnings-as-errors]
109114:2026-07-28T13:01:33.0213411Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_macros.c:142:43: error: 200U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
109117:2026-07-28T13:01:33.0215860Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_macros.c:149:25: error: no header providing "WEB_API_METHOD_PUT" is directly included [misc-include-cleaner,-warnings-as-errors]
109120:2026-07-28T13:01:33.0218312Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_macros.c:172:39: error: 200U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
109123:2026-07-28T13:01:33.0220888Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_macros.c:199:15: error: 80U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
109126:2026-07-28T13:01:33.0223034Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_macros.c:204:57: error: 200U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
109129:2026-07-28T13:01:33.0225520Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_macros.c:216:57: error: 200U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
109132:2026-07-28T13:01:33.0228446Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_macros.c:241:22: error: 192U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
109135:2026-07-28T13:01:33.0230696Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_macros.c:250:15: error: 160U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
109138:2026-07-28T13:01:33.0232831Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_macros.c:259:57: error: 200U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
109141:2026-07-28T13:01:33.0235191Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_macros.c:263:64: error: parameter 'out_name_size' is unused [misc-unused-parameters,-warnings-as-errors]
109144:2026-07-28T13:01:33.0237908Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_macros.c:304:35: error: 201U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
109147:2026-07-28T13:01:33.0240291Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_macros.c:314:10: error: no header providing "WEB_API_ROUTE_SET_MACROS" is directly included [misc-include-cleaner,-warnings-as-errors]
109150:2026-07-28T13:01:33.0242473Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_macros.c:317:10: error: no header providing "WEB_API_ROUTE_SET_MACRO" is directly included [misc-include-cleaner,-warnings-as-errors]
109153:2026-07-28T13:01:33.0244736Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_macros.c:320:10: error: no header providing "WEB_API_ROUTE_SET_MACROS_REORDER" is directly included [misc-include-cleaner,-warnings-as-errors]
109156:2026-07-28T13:01:33.0246975Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_macros.c:323:10: error: no header providing "WEB_API_ROUTE_SET_MACRO_VALIDATE" is directly included [misc-include-cleaner,-warnings-as-errors]
109159:2026-07-28T13:01:33.0249408Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_macros.c:326:10: error: no header providing "WEB_API_ROUTE_SET_MACRO_DUPLICATE" is directly included [misc-include-cleaner,-warnings-as-errors]
109208:2026-07-28T13:01:33.0293258Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_dispatch.c:68:17: error: no header providing "NULL" is directly included [misc-include-cleaner,-warnings-as-errors]
109232:2026-07-28T13:01:33.0314518Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c:48:59: error: 200U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
109235:2026-07-28T13:01:33.0317228Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c:48:77: error: 300U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
109238:2026-07-28T13:01:33.0320308Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c:62:71: error: 2 adjacent parameters of 'web_api_response_error' of convertible types are easily swapped by mistake [bugprone-easily-swappable-parameters,-warnings-as-errors]
109255:2026-07-28T13:01:33.0332978Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c:63:64: error: 2 adjacent parameters of 'web_api_response_error' of similar type ('const char *') are easily swapped by mistake [bugprone-easily-swappable-parameters,-warnings-as-errors]
109266:2026-07-28T13:01:33.0340756Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c:68:57: error: 400U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
109269:2026-07-28T13:01:33.0343355Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_response.c:68:74: error: 599U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
109303:2026-07-28T13:01:33.0374040Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_procedures.c:40:57: error: 200U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
109306:2026-07-28T13:01:33.0376600Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_procedures.c:48:25: error: no header providing "WEB_API_METHOD_GET" is directly included [misc-include-cleaner,-warnings-as-errors]
109309:2026-07-28T13:01:33.0379021Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_procedures.c:57:61: error: 200U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
109312:2026-07-28T13:01:33.0381685Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_procedures.c:85:39: error: 201U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
109315:2026-07-28T13:01:33.0384098Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_procedures.c:96:47: error: 200U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
109318:2026-07-28T13:01:33.0386600Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_procedures.c:103:25: error: no header providing "WEB_API_METHOD_PUT" is directly included [misc-include-cleaner,-warnings-as-errors]
109321:2026-07-28T13:01:33.0389005Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_procedures.c:127:43: error: 200U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
109324:2026-07-28T13:01:33.0391459Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_procedures.c:142:15: error: 80U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
109327:2026-07-28T13:01:33.0393776Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_procedures.c:147:57: error: 200U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
109330:2026-07-28T13:01:33.0396332Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_procedures.c:158:57: error: 200U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
109333:2026-07-28T13:01:33.0399085Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_procedures.c:207:85: error: parameter name 'id' is too short, expected at least 3 characters [readability-identifier-length,-warnings-as-errors]
109336:2026-07-28T13:01:33.0401865Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_procedures.c:216:78: error: parameter name 'id' is too short, expected at least 3 characters [readability-identifier-length,-warnings-as-errors]
109339:2026-07-28T13:01:33.0404879Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_procedures.c:241:25: error: function 'handle_progress_action' has cognitive complexity of 29 (threshold 25) [readability-function-cognitive-complexity,-warnings-as-errors]
109402:2026-07-28T13:01:33.0448002Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_procedures.c:304:10: error: no header providing "WEB_API_ROUTE_SET_PROCEDURES" is directly included [misc-include-cleaner,-warnings-as-errors]
109405:2026-07-28T13:01:33.0450371Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_procedures.c:306:10: error: no header providing "WEB_API_ROUTE_SET_PROCEDURE" is directly included [misc-include-cleaner,-warnings-as-errors]
109408:2026-07-28T13:01:33.0452648Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_procedures.c:308:10: error: no header providing "WEB_API_ROUTE_SET_PROCEDURES_REORDER" is directly included [misc-include-cleaner,-warnings-as-errors]
109411:2026-07-28T13:01:33.0455065Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_procedures.c:310:10: error: no header providing "WEB_API_ROUTE_PROCEDURE_PROGRESS" is directly included [misc-include-cleaner,-warnings-as-errors]
109414:2026-07-28T13:01:33.0457363Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_procedures.c:312:10: error: no header providing "WEB_API_ROUTE_PROGRESS_COMPLETE" is directly included [misc-include-cleaner,-warnings-as-errors]
109417:2026-07-28T13:01:33.0459730Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_procedures.c:314:10: error: no header providing "WEB_API_ROUTE_PROGRESS_SKIP" is directly included [misc-include-cleaner,-warnings-as-errors]
109546:2026-07-28T13:01:33.0577193Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_execution.c:93:15: error: 768U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
109549:2026-07-28T13:01:33.0579515Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_execution.c:104:22: error: 192U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
109552:2026-07-28T13:01:33.0581772Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_execution.c:130:15: error: 192U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
109682:2026-07-28T13:01:33.0701921Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_administration.c:135:25: error: no header providing "WEB_API_METHOD_GET" is directly included [misc-include-cleaner,-warnings-as-errors]
109685:2026-07-28T13:01:33.0704250Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_administration.c:147:9: error: no header providing "macro_set_t" is directly included [misc-include-cleaner,-warnings-as-errors]
109688:2026-07-28T13:01:33.0706489Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_administration.c:206:15: error: 80U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
109691:2026-07-28T13:01:33.0709017Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_administration.c:285:10: error: no header providing "WEB_API_ROUTE_AUTH_SESSION" is directly included [misc-include-cleaner,-warnings-as-errors]
109694:2026-07-28T13:01:33.0711327Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_administration.c:288:10: error: no header providing "WEB_API_ROUTE_SETTINGS" is directly included [misc-include-cleaner,-warnings-as-errors]
109697:2026-07-28T13:01:33.0721987Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_administration.c:290:10: error: no header providing "WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD" is directly included [misc-include-cleaner,-warnings-as-errors]
109700:2026-07-28T13:01:33.0724491Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_administration.c:292:10: error: no header providing "WEB_API_ROUTE_DEVICE_RESTART" is directly included [misc-include-cleaner,-warnings-as-errors]
109703:2026-07-28T13:01:33.0726832Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_administration.c:295:10: error: no header providing "WEB_API_ROUTE_DEVICE_RESET_SETTINGS" is directly included [misc-include-cleaner,-warnings-as-errors]
109706:2026-07-28T13:01:33.0729488Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_administration.c:297:10: error: no header providing "WEB_API_ROUTE_DEVICE_FACTORY_RESET" is directly included [misc-include-cleaner,-warnings-as-errors]
109709:2026-07-28T13:01:33.0731833Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_administration.c:305:10: error: no header providing "WEB_API_ROUTE_DIAGNOSTICS_STORAGE" is directly included [misc-include-cleaner,-warnings-as-errors]
109712:2026-07-28T13:01:33.0734288Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_administration.c:307:10: error: no header providing "WEB_API_ROUTE_DIAGNOSTICS_STORAGE_CHECK" is directly included [misc-include-cleaner,-warnings-as-errors]
109715:2026-07-28T13:01:33.0736704Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_administration.c:309:10: error: no header providing "WEB_API_ROUTE_DIAGNOSTICS_QUARANTINE" is directly included [misc-include-cleaner,-warnings-as-errors]
109718:2026-07-28T13:01:33.0739112Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_administration.c:311:10: error: no header providing "WEB_API_ROUTE_SET_EXPORT" is directly included [misc-include-cleaner,-warnings-as-errors]
109721:2026-07-28T13:01:33.0741477Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_administration.c:312:10: error: no header providing "WEB_API_ROUTE_SET_IMPORT" is directly included [misc-include-cleaner,-warnings-as-errors]
109724:2026-07-28T13:01:33.0743685Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_administration.c:313:10: error: no header providing "WEB_API_ROUTE_BACKUP" is directly included [misc-include-cleaner,-warnings-as-errors]
109727:2026-07-28T13:01:33.0745884Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_administration.c:314:10: error: no header providing "WEB_API_ROUTE_RESTORE" is directly included [misc-include-cleaner,-warnings-as-errors]
109746:2026-07-28T13:01:33.0762798Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_sets.c:45:61: error: 200U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
109749:2026-07-28T13:01:33.0765343Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_sets.c:72:39: error: 201U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
109752:2026-07-28T13:01:33.0767980Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_sets.c:82:62: error: 200U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
109755:2026-07-28T13:01:33.0770510Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_sets.c:102:62: error: 200U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
109758:2026-07-28T13:01:33.0773039Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_sets.c:115:15: error: 80U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
109761:2026-07-28T13:01:33.0775200Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_sets.c:121:51: error: 200U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
109764:2026-07-28T13:01:33.0777854Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_sets.c:148:57: error: 200U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
109783:2026-07-28T13:01:33.0795002Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_api.c:8:1: error: included header types.h is not used directly [misc-include-cleaner,-warnings-as-errors]
109787:2026-07-28T13:01:33.0797352Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_api.c:53:16: error: variable name 'id' is too short, expected at least 3 characters [readability-identifier-length,-warnings-as-errors]
109790:2026-07-28T13:01:33.0799776Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_api.c:200:54: error: no header providing "ssize_t" is directly included [misc-include-cleaner,-warnings-as-errors]
109793:2026-07-28T13:01:33.0802692Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_api.c:221:27: error: redundant explicit casting to the same type 'size_t' (aka 'unsigned int') as the sub-expression, remove this casting [readability-redundant-casting,-warnings-as-errors]
109799:2026-07-28T13:01:33.0807384Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_api.c:231:35: error: redundant explicit casting to the same type 'size_t' (aka 'unsigned int') as the sub-expression, remove this casting [readability-redundant-casting,-warnings-as-errors]
109805:2026-07-28T13:01:33.0812058Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_api.c:259:11: error: function 'api_handler' has cognitive complexity of 29 (threshold 25) [readability-function-cognitive-complexity,-warnings-as-errors]
109892:2026-07-28T13:01:33.0870433Z ##[error]Process completed with exit code 1.
```

## Terminal gate tail

```text
2026-07-28T13:01:33.0586621Z [47/80][0.4s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_static_path.c
2026-07-28T13:01:33.0588137Z 672 warnings generated.
2026-07-28T13:01:33.0588441Z Suppressed 672 warnings (672 in non-user code).
2026-07-28T13:01:33.0589140Z Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.
2026-07-28T13:01:33.0589625Z
2026-07-28T13:01:33.0591208Z [48/80][0.5s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/wifi_ap/wifi_ap.c
2026-07-28T13:01:33.0592573Z 1446 warnings generated.
2026-07-28T13:01:33.0592881Z Suppressed 1446 warnings (1446 in non-user code).
2026-07-28T13:01:33.0593582Z Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.
2026-07-28T13:01:33.0594050Z
2026-07-28T13:01:33.0595839Z [49/80][0.2s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_json.c
2026-07-28T13:01:33.0597324Z 745 warnings generated.
2026-07-28T13:01:33.0597733Z Suppressed 745 warnings (745 in non-user code).
2026-07-28T13:01:33.0598450Z Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.
2026-07-28T13:01:33.0598911Z
2026-07-28T13:01:33.0600609Z [50/80][0.1s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_adapter_body_auth.c
2026-07-28T13:01:33.0602134Z 154 warnings generated.
2026-07-28T13:01:33.0602430Z Suppressed 154 warnings (154 in non-user code).
2026-07-28T13:01:33.0603123Z Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.
2026-07-28T13:01:33.0603578Z
2026-07-28T13:01:33.0605184Z [51/80][3.3s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_content.c
2026-07-28T13:01:33.0606563Z 672 warnings generated.
2026-07-28T13:01:33.0606852Z Suppressed 672 warnings (672 in non-user code).
2026-07-28T13:01:33.0607627Z Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.
2026-07-28T13:01:33.0608078Z
2026-07-28T13:01:33.0609850Z [52/80][0.3s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_static.c
2026-07-28T13:01:33.0611272Z 1447 warnings generated.
2026-07-28T13:01:33.0611579Z Suppressed 1447 warnings (1447 in non-user code).
2026-07-28T13:01:33.0612270Z Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.
2026-07-28T13:01:33.0612717Z
2026-07-28T13:01:33.0614374Z [53/80][0.3s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_progress.c
2026-07-28T13:01:33.0615797Z 913 warnings generated.
2026-07-28T13:01:33.0616086Z Suppressed 913 warnings (913 in non-user code).
2026-07-28T13:01:33.0616758Z Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.
2026-07-28T13:01:33.0617204Z
2026-07-28T13:01:33.0618908Z [54/80][3.8s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_setup_json.c
2026-07-28T13:01:33.0620425Z 759 warnings generated.
2026-07-28T13:01:33.0620721Z Suppressed 759 warnings (759 in non-user code).
2026-07-28T13:01:33.0621390Z Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.
2026-07-28T13:01:33.0621840Z
2026-07-28T13:01:33.0623500Z [55/80][0.3s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_status_limits.c
2026-07-28T13:01:33.0624930Z 1437 warnings generated.
2026-07-28T13:01:33.0625297Z Suppressed 1437 warnings (1437 in non-user code).
2026-07-28T13:01:33.0625975Z Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.
2026-07-28T13:01:33.0626417Z
2026-07-28T13:01:33.0628132Z [56/80][0.1s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_mount_core.c
2026-07-28T13:01:33.0629523Z 144 warnings generated.
2026-07-28T13:01:33.0629810Z Suppressed 144 warnings (144 in non-user code).
2026-07-28T13:01:33.0630480Z Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.
2026-07-28T13:01:33.0630929Z
2026-07-28T13:01:33.0632586Z [57/80][0.1s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_keymap_us.c
2026-07-28T13:01:33.0633997Z 672 warnings generated.
2026-07-28T13:01:33.0634286Z Suppressed 672 warnings (672 in non-user code).
2026-07-28T13:01:33.0635101Z Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.
2026-07-28T13:01:33.0635555Z
2026-07-28T13:01:33.0637167Z [58/80][0.4s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/usb_keyboard/usb_keyboard.c
2026-07-28T13:01:33.0638677Z 1454 warnings generated.
2026-07-28T13:01:33.0638971Z Suppressed 1454 warnings (1454 in non-user code).
2026-07-28T13:01:33.0639648Z Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.
2026-07-28T13:01:33.0640088Z
2026-07-28T13:01:33.0641679Z [59/80][0.3s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/auth/auth_core_common.c
2026-07-28T13:01:33.0643048Z 687 warnings generated.
2026-07-28T13:01:33.0643335Z Suppressed 687 warnings (687 in non-user code).
2026-07-28T13:01:33.0644079Z Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.
2026-07-28T13:01:33.0644525Z
2026-07-28T13:01:33.0646097Z [60/80][0.3s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/app_core/app_core.c
2026-07-28T13:01:33.0647455Z 996 warnings generated.
2026-07-28T13:01:33.0647895Z Suppressed 996 warnings (996 in non-user code).
2026-07-28T13:01:33.0648581Z Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.
2026-07-28T13:01:33.0649037Z
2026-07-28T13:01:33.0650799Z [61/80][5.0s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/app_core/app_core_sequence.c
2026-07-28T13:01:33.0652220Z 687 warnings generated.
2026-07-28T13:01:33.0652520Z Suppressed 687 warnings (687 in non-user code).
2026-07-28T13:01:33.0653226Z Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.
2026-07-28T13:01:33.0653688Z
2026-07-28T13:01:33.0655322Z [62/80][0.0s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_model/app_error.c
2026-07-28T13:01:33.0658395Z [63/80][0.1s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/usb_keyboard/usb_keyboard_state.c
2026-07-28T13:01:33.0659933Z 134 warnings generated.
2026-07-28T13:01:33.0660227Z Suppressed 134 warnings (134 in non-user code).
2026-07-28T13:01:33.0660920Z Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.
2026-07-28T13:01:33.0661370Z
2026-07-28T13:01:33.0663039Z [64/80][0.3s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_executor/macro_executor_engine.c
2026-07-28T13:01:33.0664472Z 682 warnings generated.
2026-07-28T13:01:33.0664755Z Suppressed 682 warnings (682 in non-user code).
2026-07-28T13:01:33.0665435Z Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.
2026-07-28T13:01:33.0665883Z
2026-07-28T13:01:33.0667617Z [65/80][6.0s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic.c
2026-07-28T13:01:33.0669065Z 1115 warnings generated.
2026-07-28T13:01:33.0669366Z Suppressed 1115 warnings (1115 in non-user code).
2026-07-28T13:01:33.0670046Z Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.
2026-07-28T13:01:33.0670491Z
2026-07-28T13:01:33.0672066Z [66/80][0.1s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_origin.c
2026-07-28T13:01:33.0673430Z 667 warnings generated.
2026-07-28T13:01:33.0673709Z Suppressed 667 warnings (667 in non-user code).
2026-07-28T13:01:33.0674377Z Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.
2026-07-28T13:01:33.0674891Z
2026-07-28T13:01:33.0676498Z [67/80][0.4s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/provisioning/provisioning.c
2026-07-28T13:01:33.0678064Z 1320 warnings generated.
2026-07-28T13:01:33.0678366Z Suppressed 1320 warnings (1320 in non-user code).
2026-07-28T13:01:33.0679042Z Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.
2026-07-28T13:01:33.0679497Z
2026-07-28T13:01:33.0681087Z [68/80][0.4s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_json.c
2026-07-28T13:01:33.0682461Z 749 warnings generated.
2026-07-28T13:01:33.0682756Z Suppressed 749 warnings (749 in non-user code).
2026-07-28T13:01:33.0683446Z Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.
2026-07-28T13:01:33.0684042Z
2026-07-28T13:01:33.0685648Z [69/80][3.7s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/macro_parser/macro_parser.c
2026-07-28T13:01:33.0687061Z 866 warnings generated.
2026-07-28T13:01:33.0687361Z Suppressed 866 warnings (866 in non-user code).
2026-07-28T13:01:33.0688358Z Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.
2026-07-28T13:01:33.0688815Z
2026-07-28T13:01:33.0690469Z [70/80][0.7s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic_validators.c
2026-07-28T13:01:33.0691889Z 908 warnings generated.
2026-07-28T13:01:33.0692177Z Suppressed 908 warnings (908 in non-user code).
2026-07-28T13:01:33.0692849Z Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.
2026-07-28T13:01:33.0693384Z
2026-07-28T13:01:33.0694990Z [71/80][0.3s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_cancel.c
2026-07-28T13:01:33.0696378Z 1442 warnings generated.
2026-07-28T13:01:33.0696682Z Suppressed 1442 warnings (1442 in non-user code).
2026-07-28T13:01:33.0697365Z Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.
2026-07-28T13:01:33.0697918Z
2026-07-28T13:01:33.0699555Z [72/80][1.1s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_administration.c
2026-07-28T13:01:33.0701921Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_administration.c:135:25: error: no header providing "WEB_API_METHOD_GET" is directly included [misc-include-cleaner,-warnings-as-errors]
2026-07-28T13:01:33.0702876Z     1 |     if (call->method == WEB_API_METHOD_GET) {
2026-07-28T13:01:33.0703189Z       |                         ^
2026-07-28T13:01:33.0704250Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_administration.c:147:9: error: no header providing "macro_set_t" is directly included [misc-include-cleaner,-warnings-as-errors]
2026-07-28T13:01:33.0705133Z     1 |         macro_set_t selected = {0};
2026-07-28T13:01:33.0705397Z       |         ^
2026-07-28T13:01:33.0706489Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_administration.c:206:15: error: 80U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
2026-07-28T13:01:33.0707400Z   206 |     char data[80U];
2026-07-28T13:01:33.0707731Z       |               ^
2026-07-28T13:01:33.0709017Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_administration.c:285:10: error: no header providing "WEB_API_ROUTE_AUTH_SESSION" is directly included [misc-include-cleaner,-warnings-as-errors]
2026-07-28T13:01:33.0709991Z   285 |     case WEB_API_ROUTE_AUTH_SESSION:
2026-07-28T13:01:33.0710260Z       |          ^
2026-07-28T13:01:33.0711327Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_administration.c:288:10: error: no header providing "WEB_API_ROUTE_SETTINGS" is directly included [misc-include-cleaner,-warnings-as-errors]
2026-07-28T13:01:33.0712245Z   288 |     case WEB_API_ROUTE_SETTINGS:
2026-07-28T13:01:33.0720678Z       |          ^
2026-07-28T13:01:33.0721987Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_administration.c:290:10: error: no header providing "WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD" is directly included [misc-include-cleaner,-warnings-as-errors]
2026-07-28T13:01:33.0723065Z   290 |     case WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD:
2026-07-28T13:01:33.0723368Z       |          ^
2026-07-28T13:01:33.0724491Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_administration.c:292:10: error: no header providing "WEB_API_ROUTE_DEVICE_RESTART" is directly included [misc-include-cleaner,-warnings-as-errors]
2026-07-28T13:01:33.0725452Z   292 |     case WEB_API_ROUTE_DEVICE_RESTART:
2026-07-28T13:01:33.0725724Z       |          ^
2026-07-28T13:01:33.0726832Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_administration.c:295:10: error: no header providing "WEB_API_ROUTE_DEVICE_RESET_SETTINGS" is directly included [misc-include-cleaner,-warnings-as-errors]
2026-07-28T13:01:33.0728099Z   295 |     case WEB_API_ROUTE_DEVICE_RESET_SETTINGS:
2026-07-28T13:01:33.0728383Z       |          ^
2026-07-28T13:01:33.0729488Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_administration.c:297:10: error: no header providing "WEB_API_ROUTE_DEVICE_FACTORY_RESET" is directly included [misc-include-cleaner,-warnings-as-errors]
2026-07-28T13:01:33.0730468Z   297 |     case WEB_API_ROUTE_DEVICE_FACTORY_RESET: {
2026-07-28T13:01:33.0730743Z       |          ^
2026-07-28T13:01:33.0731833Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_administration.c:305:10: error: no header providing "WEB_API_ROUTE_DIAGNOSTICS_STORAGE" is directly included [misc-include-cleaner,-warnings-as-errors]
2026-07-28T13:01:33.0732896Z   305 |     case WEB_API_ROUTE_DIAGNOSTICS_STORAGE:
2026-07-28T13:01:33.0733176Z       |          ^
2026-07-28T13:01:33.0734288Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_administration.c:307:10: error: no header providing "WEB_API_ROUTE_DIAGNOSTICS_STORAGE_CHECK" is directly included [misc-include-cleaner,-warnings-as-errors]
2026-07-28T13:01:33.0735308Z   307 |     case WEB_API_ROUTE_DIAGNOSTICS_STORAGE_CHECK:
2026-07-28T13:01:33.0735595Z       |          ^
2026-07-28T13:01:33.0736704Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_administration.c:309:10: error: no header providing "WEB_API_ROUTE_DIAGNOSTICS_QUARANTINE" is directly included [misc-include-cleaner,-warnings-as-errors]
2026-07-28T13:01:33.0737788Z   309 |     case WEB_API_ROUTE_DIAGNOSTICS_QUARANTINE:
2026-07-28T13:01:33.0738061Z       |          ^
2026-07-28T13:01:33.0739112Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_administration.c:311:10: error: no header providing "WEB_API_ROUTE_SET_EXPORT" is directly included [misc-include-cleaner,-warnings-as-errors]
2026-07-28T13:01:33.0740035Z   311 |     case WEB_API_ROUTE_SET_EXPORT:
2026-07-28T13:01:33.0740292Z       |          ^
2026-07-28T13:01:33.0741477Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_administration.c:312:10: error: no header providing "WEB_API_ROUTE_SET_IMPORT" is directly included [misc-include-cleaner,-warnings-as-errors]
2026-07-28T13:01:33.0742397Z   312 |     case WEB_API_ROUTE_SET_IMPORT:
2026-07-28T13:01:33.0742644Z       |          ^
2026-07-28T13:01:33.0743685Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_administration.c:313:10: error: no header providing "WEB_API_ROUTE_BACKUP" is directly included [misc-include-cleaner,-warnings-as-errors]
2026-07-28T13:01:33.0744585Z   313 |     case WEB_API_ROUTE_BACKUP:
2026-07-28T13:01:33.0744835Z       |          ^
2026-07-28T13:01:33.0745884Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_administration.c:314:10: error: no header providing "WEB_API_ROUTE_RESTORE" is directly included [misc-include-cleaner,-warnings-as-errors]
2026-07-28T13:01:33.0746789Z   314 |     case WEB_API_ROUTE_RESTORE:
2026-07-28T13:01:33.0747037Z       |          ^
2026-07-28T13:01:33.0747253Z 1570 warnings generated.
2026-07-28T13:01:33.0747685Z Suppressed 1554 warnings (1554 in non-user code).
2026-07-28T13:01:33.0748422Z Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.
2026-07-28T13:01:33.0749000Z 16 warnings treated as errors
2026-07-28T13:01:33.0749159Z
2026-07-28T13:01:33.0750887Z [73/80][1.4s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_repository_procedures.c
2026-07-28T13:01:33.0752454Z 1445 warnings generated.
2026-07-28T13:01:33.0752769Z Suppressed 1445 warnings (1445 in non-user code).
2026-07-28T13:01:33.0753491Z Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.
2026-07-28T13:01:33.0753950Z
2026-07-28T13:01:33.0755564Z [74/80][0.4s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_login.c
2026-07-28T13:01:33.0757026Z 1504 warnings generated.
2026-07-28T13:01:33.0757331Z Suppressed 1504 warnings (1504 in non-user code).
2026-07-28T13:01:33.0758375Z Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.
2026-07-28T13:01:33.0758866Z
2026-07-28T13:01:33.0760543Z [75/80][0.2s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_sets.c
2026-07-28T13:01:33.0762798Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_sets.c:45:61: error: 200U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
2026-07-28T13:01:33.0763808Z    45 |             result = web_api_handler_success_json(response, 200U, json);
2026-07-28T13:01:33.0764225Z       |                                                             ^
2026-07-28T13:01:33.0765343Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_sets.c:72:39: error: 201U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
2026-07-28T13:01:33.0766429Z    72 |             return send_set(response, 201U, &committed);
2026-07-28T13:01:33.0766778Z       |                                       ^
2026-07-28T13:01:33.0767980Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_sets.c:82:62: error: 200U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
2026-07-28T13:01:33.0768970Z    82 |         return result == APP_ERROR_NONE ? send_set(response, 200U, &set)
2026-07-28T13:01:33.0769393Z       |                                                              ^
2026-07-28T13:01:33.0770510Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_sets.c:102:62: error: 200U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
2026-07-28T13:01:33.0771510Z   102 |         return result == APP_ERROR_NONE ? send_set(response, 200U, &committed)
2026-07-28T13:01:33.0771934Z       |                                                              ^
2026-07-28T13:01:33.0773039Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_sets.c:115:15: error: 80U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
2026-07-28T13:01:33.0773898Z   115 |     char data[80U];
2026-07-28T13:01:33.0774135Z       |               ^
2026-07-28T13:01:33.0775200Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_sets.c:121:51: error: 200U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
2026-07-28T13:01:33.0776257Z   121 |     return web_api_handler_success_json(response, 200U, data);
2026-07-28T13:01:33.0776634Z       |                                                   ^
2026-07-28T13:01:33.0777854Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_api_sets.c:148:57: error: 200U is a magic number; consider replacing it with a named constant [readability-magic-numbers,-warnings-as-errors]
2026-07-28T13:01:33.0778850Z   148 |         result = web_api_handler_success_json(response, 200U, json);
2026-07-28T13:01:33.0779256Z       |                                                         ^
2026-07-28T13:01:33.0779554Z 943 warnings generated.
2026-07-28T13:01:33.0779997Z Suppressed 936 warnings (936 in non-user code).
2026-07-28T13:01:33.0781005Z Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.
2026-07-28T13:01:33.0781592Z 7 warnings treated as errors
2026-07-28T13:01:33.0781752Z
2026-07-28T13:01:33.0783460Z [76/80][2.8s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_adapter_common.c
2026-07-28T13:01:33.0784900Z 149 warnings generated.
2026-07-28T13:01:33.0785201Z Suppressed 149 warnings (149 in non-user code).
2026-07-28T13:01:33.0785894Z Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.
2026-07-28T13:01:33.0786342Z
2026-07-28T13:01:33.0788133Z [77/80][0.1s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/support/app_operation_result.c
2026-07-28T13:01:33.0789580Z 15 warnings generated.
2026-07-28T13:01:33.0789877Z Suppressed 15 warnings (15 in non-user code).
2026-07-28T13:01:33.0790773Z Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.
2026-07-28T13:01:33.0791247Z
2026-07-28T13:01:33.0792863Z [78/80][3.2s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_api.c
2026-07-28T13:01:33.0795002Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_api.c:8:1: error: included header types.h is not used directly [misc-include-cleaner,-warnings-as-errors]
2026-07-28T13:01:33.0795834Z     8 | #include <sys/types.h>
2026-07-28T13:01:33.0796086Z       | ^~~~~~~~~~~~~~~~~~~~~~
2026-07-28T13:01:33.0796300Z     9 |
2026-07-28T13:01:33.0797352Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_api.c:53:16: error: variable name 'id' is too short, expected at least 3 characters [readability-identifier-length,-warnings-as-errors]
2026-07-28T13:01:33.0798542Z    53 |     app_uuid_t id = {0};
2026-07-28T13:01:33.0798787Z       |                ^
2026-07-28T13:01:33.0799776Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_api.c:200:54: error: no header providing "ssize_t" is directly included [misc-include-cleaner,-warnings-as-errors]
2026-07-28T13:01:33.0800881Z     6 |     return httpd_resp_send(request, response->body, (ssize_t)response->body_length);
2026-07-28T13:01:33.0801338Z       |                                                      ^
2026-07-28T13:01:33.0802692Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_api.c:221:27: error: redundant explicit casting to the same type 'size_t' (aka 'unsigned int') as the sub-expression, remove this casting [readability-redundant-casting,-warnings-as-errors]
2026-07-28T13:01:33.0803805Z   221 |         .content_length = (size_t)request->content_len,
2026-07-28T13:01:33.0804144Z       |                           ^~~~~~~~
2026-07-28T13:01:33.0804894Z /home/runner/esp/esp-idf-v5.5.5/components/esp_http_server/include/esp_http_server.h:377:21: note: source type originates from referencing this member
2026-07-28T13:01:33.0805715Z   377 |     size_t          content_len;                /*!< Length of the request body */
2026-07-28T13:01:33.0806080Z       |     ~~~~~~          ^
2026-07-28T13:01:33.0807384Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_api.c:231:35: error: redundant explicit casting to the same type 'size_t' (aka 'unsigned int') as the sub-expression, remove this casting [readability-redundant-casting,-warnings-as-errors]
2026-07-28T13:01:33.0808662Z   231 |     const size_t content_length = (size_t)request->content_len;
2026-07-28T13:01:33.0809041Z       |                                   ^~~~~~~~
2026-07-28T13:01:33.0809798Z /home/runner/esp/esp-idf-v5.5.5/components/esp_http_server/include/esp_http_server.h:377:21: note: source type originates from referencing this member
2026-07-28T13:01:33.0810513Z   377 |     size_t          content_len;                /*!< Length of the request body */
2026-07-28T13:01:33.0810874Z       |     ~~~~~~          ^
2026-07-28T13:01:33.0812058Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_api.c:259:11: error: function 'api_handler' has cognitive complexity of 29 (threshold 25) [readability-function-cognitive-complexity,-warnings-as-errors]
2026-07-28T13:01:33.0813069Z   259 | esp_err_t api_handler(httpd_req_t *request) {
2026-07-28T13:01:33.0813360Z       |           ^
2026-07-28T13:01:33.0814330Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_api.c:268:5: note: +1, including nesting penalty of 0, nesting level increased to 1
2026-07-28T13:01:33.0815352Z   268 |     if (result == APP_ERROR_NONE) {
2026-07-28T13:01:33.0815625Z       |     ^
2026-07-28T13:01:33.0816441Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_api.c:271:5: note: +1, including nesting penalty of 0, nesting level increased to 1
2026-07-28T13:01:33.0817202Z   271 |     if (result != APP_ERROR_NONE) {
2026-07-28T13:01:33.0817454Z       |     ^
2026-07-28T13:01:33.0818380Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_api.c:273:43: note: +2, including nesting penalty of 1, nesting level increased to 2
2026-07-28T13:01:33.0819310Z   273 |             result == APP_ERROR_NOT_FOUND ? WEB_HTTP_STATUS_NOT_FOUND : WEB_HTTP_STATUS_BAD_REQUEST;
2026-07-28T13:01:33.0819772Z       |                                           ^
2026-07-28T13:01:33.0820696Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_api.c:275:73: note: +2, including nesting penalty of 1, nesting level increased to 2
2026-07-28T13:01:33.0821543Z   275 |                                     status == WEB_HTTP_STATUS_NOT_FOUND ? "route not found"
2026-07-28T13:01:33.0821986Z       |                                                                         ^
2026-07-28T13:01:33.0822806Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_api.c:278:12: note: +1, nesting level increased to 1
2026-07-28T13:01:33.0823668Z   278 |     } else if (!web_api_route_allows_method(call.path.route, call.method)) {
2026-07-28T13:01:33.0824029Z       |            ^
2026-07-28T13:01:33.0824862Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_api.c:285:5: note: +1, including nesting penalty of 0, nesting level increased to 1
2026-07-28T13:01:33.0825648Z   285 |     if (!response_ready && result == APP_ERROR_NONE &&
2026-07-28T13:01:33.0826118Z       |     ^
2026-07-28T13:01:33.0826791Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_api.c:286:68: note: +1
2026-07-28T13:01:33.0827671Z   286 |         !web_api_route_requires_body(call.path.route, call.method) && request->content_len != 0U) {
2026-07-28T13:01:33.0828197Z       |                                                                    ^
2026-07-28T13:01:33.0829215Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_api.c:291:5: note: +1, including nesting penalty of 0, nesting level increased to 1
2026-07-28T13:01:33.0830005Z   291 |     if (!response_ready && result == APP_ERROR_NONE) {
2026-07-28T13:01:33.0830305Z       |     ^
2026-07-28T13:01:33.0830900Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_api.c:291:25: note: +1
2026-07-28T13:01:33.0831512Z   291 |     if (!response_ready && result == APP_ERROR_NONE) {
2026-07-28T13:01:33.0831830Z       |                         ^
2026-07-28T13:01:33.0832691Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_api.c:294:9: note: +2, including nesting penalty of 1, nesting level increased to 2
2026-07-28T13:01:33.0833453Z   294 |         if (policy_result != APP_ERROR_NONE) {
2026-07-28T13:01:33.0833741Z       |         ^
2026-07-28T13:01:33.0834562Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_api.c:301:5: note: +1, including nesting penalty of 0, nesting level increased to 1
2026-07-28T13:01:33.0835332Z   301 |     if (!response_ready && result == APP_ERROR_NONE) {
2026-07-28T13:01:33.0835616Z       |     ^
2026-07-28T13:01:33.0836196Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_api.c:301:25: note: +1
2026-07-28T13:01:33.0836911Z   301 |     if (!response_ready && result == APP_ERROR_NONE) {
2026-07-28T13:01:33.0837234Z       |                         ^
2026-07-28T13:01:33.0838200Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_api.c:303:34: note: +2, including nesting penalty of 1, nesting level increased to 2
2026-07-28T13:01:33.0838962Z   303 |         call.body = body == NULL ? "" : body;
2026-07-28T13:01:33.0839277Z       |                                  ^
2026-07-28T13:01:33.0840149Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_api.c:304:9: note: +2, including nesting penalty of 1, nesting level increased to 2
2026-07-28T13:01:33.0840895Z   304 |         if (result != APP_ERROR_NONE) {
2026-07-28T13:01:33.0840978Z       |         ^
2026-07-28T13:01:33.0841682Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_api.c:310:5: note: +1, including nesting penalty of 0, nesting level increased to 1
2026-07-28T13:01:33.0841859Z   310 |     if (!response_ready && result == APP_ERROR_NONE) {
2026-07-28T13:01:33.0841937Z       |     ^
2026-07-28T13:01:33.0842428Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_api.c:310:25: note: +1
2026-07-28T13:01:33.0842592Z   310 |     if (!response_ready && result == APP_ERROR_NONE) {
2026-07-28T13:01:33.0842697Z       |                         ^
2026-07-28T13:01:33.0843402Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_api.c:312:9: note: +2, including nesting penalty of 1, nesting level increased to 2
2026-07-28T13:01:33.0843674Z   312 |         if (result != APP_ERROR_NONE && response.body == NULL) {
2026-07-28T13:01:33.0843763Z       |         ^
2026-07-28T13:01:33.0844238Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_api.c:312:38: note: +1
2026-07-28T13:01:33.0844418Z   312 |         if (result != APP_ERROR_NONE && response.body == NULL) {
2026-07-28T13:01:33.0844542Z       |                                      ^
2026-07-28T13:01:33.0845006Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_api.c:316:51: note: +1
2026-07-28T13:01:33.0845252Z   316 |         response_ready = result == APP_ERROR_NONE && response.body != NULL;
2026-07-28T13:01:33.0845395Z       |                                                   ^
2026-07-28T13:01:33.0846171Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_api.c:318:5: note: +1, including nesting penalty of 0, nesting level increased to 1
2026-07-28T13:01:33.0846294Z   318 |     if (!response_ready) {
2026-07-28T13:01:33.0846379Z       |     ^
2026-07-28T13:01:33.0846855Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_api.c:324:55: note: +1
2026-07-28T13:01:33.0847191Z   324 |     const bool should_restart = response.body != NULL && restart_after_response(&call, &response);
2026-07-28T13:01:33.0847336Z       |                                                       ^
2026-07-28T13:01:33.0848162Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_api.c:328:5: note: +1, including nesting penalty of 0, nesting level increased to 1
2026-07-28T13:01:33.0848330Z   328 |     if (send_result == ESP_OK && should_restart) {
2026-07-28T13:01:33.0848413Z       |     ^
2026-07-28T13:01:33.0848895Z /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_api.c:328:31: note: +1
2026-07-28T13:01:33.0849042Z   328 |     if (send_result == ESP_OK && should_restart) {
2026-07-28T13:01:33.0849158Z       |                               ^
2026-07-28T13:01:33.0849264Z 1479 warnings generated.
2026-07-28T13:01:33.0849456Z Suppressed 1472 warnings (1472 in non-user code).
2026-07-28T13:01:33.0850104Z Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.
2026-07-28T13:01:33.0850218Z 6 warnings treated as errors
2026-07-28T13:01:33.0850228Z
2026-07-28T13:01:33.0851968Z [79/80][0.3s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/web_server/web_server_logout_execution.c
2026-07-28T13:01:33.0852078Z 1442 warnings generated.
2026-07-28T13:01:33.0852246Z Suppressed 1442 warnings (1442 in non-user code).
2026-07-28T13:01:33.0852755Z Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.
2026-07-28T13:01:33.0852763Z
2026-07-28T13:01:33.0854421Z [80/80][3.6s] /home/runner/.espressif/tools/esp-clang/esp-19.1.2_20250312/esp-clang/bin/clang-tidy --exclude-header-filter=(esp-idf|managed_components) -header-filter=/firmware/(main/|components/|test_app/main/) -p=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/build-clang /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/firmware/components/storage/storage_atomic_recovery.c
2026-07-28T13:01:33.0854523Z 1042 warnings generated.
2026-07-28T13:01:33.0854692Z Suppressed 1042 warnings (1042 in non-user code).
2026-07-28T13:01:33.0855245Z Use -header-filter=.* to display errors from all non-system headers. Use -system-headers to display errors from system headers as well.
2026-07-28T13:01:33.0855253Z
2026-07-28T13:01:33.0870433Z ##[error]Process completed with exit code 1.
2026-07-28T13:01:33.1011041Z Node 20 is being deprecated. This workflow is running with Node 24 by default. If you need to temporarily use Node 20, you can set the ACTIONS_ALLOW_USE_UNSECURE_NODE_VERSION=true environment variable. For more information see: https://github.blog/changelog/2025-09-19-deprecation-of-node-20-on-github-actions-runners/
2026-07-28T13:01:33.1011166Z Post job cleanup.
2026-07-28T13:01:33.1857886Z [command]/usr/bin/git version
2026-07-28T13:01:33.1895999Z git version 2.54.0
2026-07-28T13:01:33.1930533Z Temporarily overriding HOME='/home/runner/work/_temp/b54dbc34-8cb6-4723-9cb3-537f29d6bba4' before making global git config changes
2026-07-28T13:01:33.1936335Z Adding repository directory to the temporary git global config as a safe directory
2026-07-28T13:01:33.1937416Z [command]/usr/bin/git config --global --add safe.directory /home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard
2026-07-28T13:01:33.1975171Z [command]/usr/bin/git config --local --name-only --get-regexp core\.sshCommand
2026-07-28T13:01:33.2010819Z [command]/usr/bin/git submodule foreach --recursive sh -c "git config --local --name-only --get-regexp 'core\.sshCommand' && git config --local --unset-all 'core.sshCommand' || :"
2026-07-28T13:01:33.2251978Z [command]/usr/bin/git config --local --name-only --get-regexp http\.https\:\/\/github\.com\/\.extraheader
2026-07-28T13:01:33.2280424Z http.https://github.com/.extraheader
2026-07-28T13:01:33.2291663Z [command]/usr/bin/git config --local --unset-all http.https://github.com/.extraheader
2026-07-28T13:01:33.2325299Z [command]/usr/bin/git submodule foreach --recursive sh -c "git config --local --name-only --get-regexp 'http\.https\:\/\/github\.com\/\.extraheader' && git config --local --unset-all 'http.https://github.com/.extraheader' || :"
2026-07-28T13:01:33.2593238Z [command]/usr/bin/git config --local --name-only --get-regexp ^includeIf\.gitdir:
2026-07-28T13:01:33.2627789Z [command]/usr/bin/git submodule foreach --recursive git config --local --show-origin --name-only --get-regexp remote.origin.url
2026-07-28T13:01:33.3025498Z Cleaning up orphan processes
2026-07-28T13:01:33.3376542Z ##[warning]Node.js 20 is deprecated. The following actions target Node.js 20 but are being forced to run on Node.js 24: actions/checkout@v4, actions/setup-node@v4. For more information see: https://github.blog/changelog/2025-09-19-deprecation-of-node-20-on-github-actions-runners/
```
