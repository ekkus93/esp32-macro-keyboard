#include "storage.h"

#include <errno.h>
#include <stddef.h>
#include <sys/stat.h>

#include "app_error.h"

/* rwxr-x--- for storage directories we create. */
#define STORAGE_DIR_MODE 0750

/* Create a directory, tolerating a pre-existing one only when it is genuinely a
 * directory. An existing non-directory (regular file, symlink, device node, ...)
 * at a required path is a corrupt topology, not a usable directory, so it is
 * rejected rather than silently accepted. */
static app_error_code_t ensure_directory(const char *path) {
    if (mkdir(path, STORAGE_DIR_MODE) == 0) {
        return APP_ERROR_NONE;
    }

    const int create_error = errno;
    if (create_error != EEXIST) {
        return create_error == ENOSPC ? APP_ERROR_STORAGE_FULL : APP_ERROR_IO;
    }

    struct stat metadata;
    if (stat(path, &metadata) != 0) {
        return errno == ENOSPC ? APP_ERROR_STORAGE_FULL : APP_ERROR_IO;
    }
    return S_ISDIR(metadata.st_mode) ? APP_ERROR_NONE : APP_ERROR_STORAGE_CORRUPT;
}

app_error_code_t storage_prepare_directories(void) {
    /* SPEC 13.3: /data holds the set index and sets/, and nothing else. There
     * is no staging, trash, or transactions directory. */
    static const char *const paths[] = {
        STORAGE_DATA_MOUNT "/sets",
    };
    for (size_t index = 0U; index < (sizeof(paths) / sizeof(paths[0])); ++index) {
        const app_error_code_t result = ensure_directory(paths[index]);
        if (result != APP_ERROR_NONE) {
            return result;
        }
    }
    return APP_ERROR_NONE;
}
