if(NOT DEFINED WEB_SERVER_SOURCE_DIR)
    message(FATAL_ERROR "WEB_SERVER_SOURCE_DIR is required")
endif()

file(GLOB web_server_sources "${WEB_SERVER_SOURCE_DIR}/*.c")
foreach(source IN LISTS web_server_sources)
    get_filename_component(source_name "${source}" NAME)
    if(source_name STREQUAL "web_server_password_record.c")
        continue()
    endif()

    file(READ "${source}" source_text)
    foreach(
        forbidden
        IN
        ITEMS "server_configuration.password_record"
              "server_configuration ="
              "memset(&server_configuration"
              "memcpy(&server_configuration"
    )
        string(FIND "${source_text}" "${forbidden}" match_index)
        if(NOT match_index EQUAL -1)
            message(
                FATAL_ERROR
                    "${source_name} bypasses synchronized server-configuration access: ${forbidden}"
            )
        endif()
    endforeach()
endforeach()
