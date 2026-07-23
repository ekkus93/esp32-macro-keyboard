cmake_minimum_required(VERSION 3.20)
project(esp32_macro_keyboard_host_tests C)

enable_testing()
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(STRICT_WARNINGS
    -Wall
    -Wextra
    -Werror
    -Wshadow
    -Wconversion
    -Wsign-conversion
    -Wformat=2
    -Wundef
    -Wdouble-promotion
    -Wmissing-declarations
    -Wstrict-prototypes
)

add_library(test_support STATIC
    support/test_assert.c
    support/test_memory.c
    support/test_temp_dir.c
    fakes/fake_call_log.c
    fakes/fake_clock.c
    fakes/fake_random.c
    fakes/fake_freertos.c
    fakes/fake_usb_backend.c
    fakes/fake_gpio_backend.c
    fakes/fake_wifi_backend.c
    fakes/fake_http_backend.c
    fakes/fake_fs_backend.c
)
target_include_directories(test_support PUBLIC support fakes)
target_compile_definitions(test_support PUBLIC _POSIX_C_SOURCE=200809L)
target_compile_options(test_support PRIVATE ${STRICT_WARNINGS})

add_executable(test_support_tests test_support.c)
target_link_libraries(test_support_tests PRIVATE test_support)
target_compile_options(test_support_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME test_support COMMAND test_support_tests)
set_tests_properties(test_support PROPERTIES LABELS "support")

add_executable(macro_parser_tests
    test_macro_parser.c
    ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/macro_model/app_uuid.c
    ../../firmware/components/macro_model/macro_model.c
    ../../firmware/components/macro_parser/macro_parser.c
    ../../firmware/components/macro_parser/macro_keymap_us.c
)
target_include_directories(macro_parser_tests PRIVATE
    ../../firmware/components/macro_model/include
    ../../firmware/components/macro_parser/include
)
target_compile_options(macro_parser_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME macro_parser COMMAND macro_parser_tests)
set_tests_properties(macro_parser PROPERTIES LABELS "parser")

add_executable(macro_executor_tests
    test_macro_executor.c
    ../../firmware/components/macro_executor/macro_executor_engine.c
)
target_include_directories(macro_executor_tests PRIVATE
    ../../firmware/components/macro_model/include
    ../../firmware/components/macro_parser/include
    ../../firmware/components/macro_executor/include
    ../../firmware/components/macro_executor
)
target_link_libraries(macro_executor_tests PRIVATE test_support)
target_compile_options(macro_executor_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME macro_executor COMMAND macro_executor_tests)
set_tests_properties(macro_executor PROPERTIES LABELS "executor")

add_executable(auth_tests
    test_auth.c
    ../../firmware/components/auth/auth_core.c
)
target_include_directories(auth_tests PRIVATE
    ../../firmware/components/macro_model/include
    ../../firmware/components/auth/include
    ../../firmware/components/auth
)
target_link_libraries(auth_tests PRIVATE test_support)
target_compile_options(auth_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME auth COMMAND auth_tests)
set_tests_properties(auth PROPERTIES LABELS "auth")

add_executable(web_security_tests
    test_web_security.c
    ../../firmware/components/web_server/web_cookie.c
    ../../firmware/components/web_server/web_origin.c
    ../../firmware/components/web_server/web_static_path.c
    ../../firmware/components/web_server/web_content.c
)
target_include_directories(web_security_tests PRIVATE
    ../../firmware/components/macro_model/include
    ../../firmware/components/auth/include
    ../../firmware/components/web_server
)
target_link_libraries(web_security_tests PRIVATE test_support)
target_compile_options(web_security_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME web_security COMMAND web_security_tests)
set_tests_properties(web_security PROPERTIES LABELS "web")

add_executable(app_core_tests
    test_app_core.c
    ../../firmware/components/app_core/app_core_sequence.c
)
target_include_directories(app_core_tests PRIVATE
    ../../firmware/components/macro_model/include
    ../../firmware/components/auth/include
    ../../firmware/components/device_controls/include
    ../../firmware/components/web_server/include
    ../../firmware/components/app_core
)
target_link_libraries(app_core_tests PRIVATE test_support)
target_compile_options(app_core_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME app_core COMMAND app_core_tests)
set_tests_properties(app_core PROPERTIES LABELS "startup")

add_executable(usb_keyboard_tests
    test_usb_keyboard.c
    ../../firmware/components/usb_keyboard/usb_keyboard_state.c
)
target_include_directories(usb_keyboard_tests PRIVATE
    ../../firmware/components/macro_model/include
    ../../firmware/components/usb_keyboard/include
    ../../firmware/components/usb_keyboard
)
target_link_libraries(usb_keyboard_tests PRIVATE test_support)
target_compile_options(usb_keyboard_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME usb_keyboard COMMAND usb_keyboard_tests)
set_tests_properties(usb_keyboard PROPERTIES LABELS "usb")

add_executable(storage_atomic_tests
    test_storage_atomic.c
    ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/macro_model/app_uuid.c
    ../../firmware/components/storage/storage_fs_ops.c
    ../../firmware/components/storage/storage_atomic.c
)
target_include_directories(storage_atomic_tests PRIVATE
    ../../firmware/components/macro_model/include
    ../../firmware/components/storage/include
    ../../firmware/components/storage
)
target_link_libraries(storage_atomic_tests PRIVATE test_support)
target_compile_options(storage_atomic_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME storage_atomic COMMAND storage_atomic_tests)
set_tests_properties(storage_atomic PROPERTIES LABELS "storage")

find_package(PkgConfig REQUIRED)
pkg_check_modules(CJSON REQUIRED IMPORTED_TARGET libcjson)

add_executable(storage_repository_tests
    test_storage_repository.c
    ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/macro_model/app_uuid.c
    ../../firmware/components/macro_model/macro_model.c
    ../../firmware/components/storage/storage_atomic.c
    ../../firmware/components/storage/storage_fs_ops.c
    ../../firmware/components/storage/storage_paths.c
    ../../firmware/components/storage/storage_transaction.c
    ../../firmware/components/storage/storage_repository_io.c
    ../../firmware/components/storage/storage_repository_json.c
    ../../firmware/components/storage/storage_repository_index.c
    ../../firmware/components/storage/storage_repository_sets.c
    ../../firmware/components/storage/storage_quarantine.c
)
target_include_directories(storage_repository_tests PRIVATE
    ../../firmware/components/macro_model/include
    ../../firmware/components/storage/include
    ../../firmware/components/storage
)
target_compile_definitions(storage_repository_tests PRIVATE
    STORAGE_DATA_MOUNT="${CMAKE_CURRENT_BINARY_DIR}/storage-data"
)
target_compile_options(storage_repository_tests PRIVATE ${STRICT_WARNINGS})
target_link_libraries(storage_repository_tests PRIVATE PkgConfig::CJSON)
add_test(NAME storage_repository COMMAND storage_repository_tests)
set_tests_properties(storage_repository PROPERTIES LABELS "storage")
