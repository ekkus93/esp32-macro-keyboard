idf_component_register(
    SRCS
        "storage_mount.c"
        "storage_paths.c"
        "storage_atomic.c"
        "storage_transaction.c"
        "storage_repository_io.c"
        "storage_repository_json.c"
        "storage_repository_index.c"
        "storage_repository_sets.c"
        "storage_quarantine.c"
    INCLUDE_DIRS "include"
    REQUIRES
        macro_model
        littlefs
        json
)

target_compile_options(${COMPONENT_LIB} PRIVATE
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
