#include "fake_call_log.h"

#include <stdlib.h>
#include <string.h>

static void require_valid(const fake_call_log_t *log, const char *name)
{
    if (log == NULL || name == NULL || name[0] == '\0') {
        abort();
    }
}

void fake_call_log_reset(fake_call_log_t *log)
{
    if (log == NULL) {
        abort();
    }
    memset(log, 0, sizeof(*log));
}

void fake_call_log_set_strict(fake_call_log_t *log, bool strict)
{
    if (log == NULL) {
        abort();
    }
    log->strict = strict;
}

void fake_call_log_expect(fake_call_log_t *log, const char *name)
{
    require_valid(log, name);
    if (log->expectation_count >= FAKE_EXPECTATION_CAPACITY) {
        abort();
    }
    log->expectations[log->expectation_count++] = name;
}

void fake_call_log_verify(const fake_call_log_t *log)
{
    if (log == NULL || log->expectation_index != log->expectation_count) {
        abort();
    }
}

void fake_call_log_fail_on(fake_call_log_t *log, const char *name, size_t occurrence)
{
    require_valid(log, name);
    if (occurrence == 0U) {
        abort();
    }
    log->failure_name = name;
    log->failure_occurrence = occurrence;
    log->failure_seen = 0U;
}

bool fake_call_log_record(fake_call_log_t *log,
                          const char *name,
                          uint64_t argument0,
                          uint64_t argument1)
{
    require_valid(log, name);
    if (log->call_count >= FAKE_CALL_LOG_CAPACITY) {
        abort();
    }
    if (log->strict) {
        if (log->expectation_index >= log->expectation_count ||
            strcmp(log->expectations[log->expectation_index], name) != 0) {
            abort();
        }
        ++log->expectation_index;
    }
    log->calls[log->call_count++] = (fake_call_t){
        .name = name,
        .argument0 = argument0,
        .argument1 = argument1,
    };
    if (log->failure_name != NULL && strcmp(log->failure_name, name) == 0) {
        ++log->failure_seen;
        return log->failure_seen == log->failure_occurrence;
    }
    return false;
}

const fake_call_t *fake_call_log_at(const fake_call_log_t *log, size_t index)
{
    if (log == NULL || index >= log->call_count) {
        abort();
    }
    return &log->calls[index];
}
