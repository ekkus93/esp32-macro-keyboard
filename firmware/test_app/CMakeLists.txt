cmake_minimum_required(VERSION 3.16)

set(IDF_TARGET "esp32s3" CACHE STRING "ESP-IDF target")
set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../components/macro_model"
    "${CMAKE_CURRENT_LIST_DIR}/../components/macro_parser"
    "${CMAKE_CURRENT_LIST_DIR}/../components/usb_keyboard"
    "${CMAKE_CURRENT_LIST_DIR}/../components/macro_executor"
    "${CMAKE_CURRENT_LIST_DIR}/../components/auth"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32_macro_keyboard_device_tests)
