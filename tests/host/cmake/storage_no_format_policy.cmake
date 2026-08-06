if(NOT DEFINED STORAGE_MOUNT_SOURCE OR NOT EXISTS "${STORAGE_MOUNT_SOURCE}")
    message(FATAL_ERROR "storage mount source is unavailable")
endif()

file(READ "${STORAGE_MOUNT_SOURCE}" mount_source)
string(
    REGEX MATCHALL
    "format_if_mount_failed[ \t\r\n]*=[ \t\r\n]*false"
    mount_disable_matches
    "${mount_source}"
)
list(LENGTH mount_disable_matches mount_disable_count)
if(NOT mount_disable_count EQUAL 1)
    message(FATAL_ERROR "storage mount must set format_if_mount_failed=false exactly once")
endif()

get_filename_component(storage_component_dir "${STORAGE_MOUNT_SOURCE}" DIRECTORY)
get_filename_component(components_dir "${storage_component_dir}" DIRECTORY)
get_filename_component(firmware_root "${components_dir}" DIRECTORY)
set(
    production_roots
    "${firmware_root}/components"
    "${firmware_root}/main"
)
set(production_sources)
foreach(production_root IN LISTS production_roots)
    if(NOT IS_DIRECTORY "${production_root}")
        message(FATAL_ERROR "firmware production source root is unavailable: ${production_root}")
    endif()
    file(
        GLOB_RECURSE root_sources
        LIST_DIRECTORIES false
        "${production_root}/*.c"
        "${production_root}/*.h"
    )
    list(APPEND production_sources ${root_sources})
endforeach()
if(NOT production_sources)
    message(FATAL_ERROR "firmware production sources are unavailable")
endif()

foreach(source_path IN LISTS production_sources)
    file(READ "${source_path}" source)
    if(source MATCHES "format_if_mount_failed[ \t\r\n]*=[ \t\r\n]*true")
        message(FATAL_ERROR "automatic LittleFS formatting enabled in ${source_path}")
    endif()
    if(source MATCHES "esp_littlefs_format[ \t\r\n]*\\(")
        message(FATAL_ERROR "explicit LittleFS formatting call found in ${source_path}")
    endif()
endforeach()
