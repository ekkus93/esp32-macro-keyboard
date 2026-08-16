#!/usr/bin/env bash
# Regression tests for scripts/check-no-wall-clock.py (SPEC_V2 §5.4).
#
# The guard has two ways to be useless: missing a real wall-clock call, or
# firing on English prose and getting disabled. Both directions are tested.
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
readonly repo_root
readonly checker="${repo_root}/scripts/check-no-wall-clock.py"
temporary_dir="$(mktemp -d)"
readonly temporary_dir
trap 'rm -rf -- "${temporary_dir}"' EXIT

readonly source_dir="${temporary_dir}/firmware/components/demo"
mkdir -p -- "${source_dir}"

pass_count=0

write_source() {
	printf '%s\n' "$1" >"${source_dir}/probe.c"
}

expect_pass() {
	local name="$1"
	if ! python3 "${checker}" --root "${temporary_dir}" >"${temporary_dir}/output" 2>&1; then
		printf 'FAIL: %s unexpectedly failed\n' "${name}" >&2
		cat -- "${temporary_dir}/output" >&2
		exit 1
	fi
	pass_count=$((pass_count + 1))
}

expect_fail() {
	local name="$1"
	local expected="$2"
	if python3 "${checker}" --root "${temporary_dir}" >"${temporary_dir}/output" 2>&1; then
		printf 'FAIL: %s unexpectedly passed\n' "${name}" >&2
		cat -- "${temporary_dir}/output" >&2
		exit 1
	fi
	if ! grep -F -- "${expected}" "${temporary_dir}/output" >/dev/null; then
		printf 'FAIL: %s did not report %s\n' "${name}" "${expected}" >&2
		cat -- "${temporary_dir}/output" >&2
		exit 1
	fi
	pass_count=$((pass_count + 1))
}

# --- allowed: monotonic time, and calendar words that are not calls ---------

write_source 'void f(void) { (void)esp_timer_get_time(); }'
expect_pass 'esp_timer_get_time is monotonic'

write_source 'void f(void) { (void)xTaskGetTickCount(); }'
expect_pass 'xTaskGetTickCount is monotonic'

write_source 'void f(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); }'
expect_pass 'clock_gettime against a monotonic clock'

# The real tree contains exactly this shape of prose; a naive pattern matches it.
write_source '/* Each sub-object is attached one at a time (rather than at once). */'
expect_pass 'prose containing "a time (" is not a call'

write_source 'static const char *m = "time(NULL) is forbidden";'
expect_pass 'a forbidden call named inside a string literal'

write_source '// time(NULL) in a line comment
void f(void) { (void)esp_timer_get_time(); }'
expect_pass 'a forbidden call inside a line comment'

write_source 'struct s { int uptime; }; void f(struct s *p) { p->uptime = 0; }'
expect_pass 'an identifier merely ending in "time"'

# --- forbidden: every calendar-time entry point -----------------------------

write_source 'void f(void) { time_t n = time(NULL); (void)n; }'
expect_fail 'time()' 'wall-clock call time() is forbidden'

write_source 'void f(void) { struct timeval tv; gettimeofday(&tv, NULL); }'
expect_fail 'gettimeofday()' 'wall-clock call gettimeofday() is forbidden'

write_source 'void f(struct timeval *tv) { settimeofday(tv, NULL); }'
expect_fail 'settimeofday()' 'wall-clock call settimeofday() is forbidden'

write_source 'void f(const time_t *t) { (void)localtime(t); }'
expect_fail 'localtime()' 'wall-clock call localtime() is forbidden'

write_source 'void f(char *b, const struct tm *t) { strftime(b, 32, "%F", t); }'
expect_fail 'strftime()' 'wall-clock call strftime() is forbidden'

write_source 'void f(void) { esp_netif_sntp_init(0); }'
expect_fail 'esp_netif_sntp_init()' 'SNTP is forbidden'

write_source 'void f(void) { sntp_setoperatingmode(0); }'
expect_fail 'sntp_setoperatingmode()' 'SNTP is forbidden'

write_source 'void f(void) { struct timespec ts; clock_gettime(CLOCK_REALTIME, &ts); }'
expect_fail 'clock_gettime(CLOCK_REALTIME)' 'is not a monotonic clock'

printf 'check-no-wall-clock regression tests passed: %d\n' "${pass_count}"
