#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
readonly repo_root
readonly valid_labels="support parser storage executor auth web startup usb controls wifi model"

mode="normal"
mode_seen=false
label=""

for argument in "$@"; do
	case "${argument}" in
	--normal)
		if [[ "${mode_seen}" == true ]]; then
			printf 'only one host-test mode may be selected\n' >&2
			exit 2
		fi
		mode="normal"
		mode_seen=true
		;;
	--sanitizers)
		if [[ "${mode_seen}" == true ]]; then
			printf 'only one host-test mode may be selected\n' >&2
			exit 2
		fi
		mode="sanitizers"
		mode_seen=true
		;;
	--coverage)
		if [[ "${mode_seen}" == true ]]; then
			printf 'only one host-test mode may be selected\n' >&2
			exit 2
		fi
		mode="coverage"
		mode_seen=true
		;;
	--*)
		printf 'unknown host-test option: %s\n' "${argument}" >&2
		printf 'usage: %s [--normal|--sanitizers|--coverage] [label]\n' "$0" >&2
		exit 2
		;;
	*)
		if [[ -n "${label}" ]]; then
			printf 'only one host-test label may be selected\n' >&2
			printf 'usage: %s [--normal|--sanitizers|--coverage] [label]\n' "$0" >&2
			exit 2
		fi
		label="${argument}"
		;;
	esac
done

if [[ -n "${label}" ]]; then
	case " ${valid_labels} " in
	*" ${label} "*) ;;
	*)
		printf 'unknown host-test label: %s\nvalid labels: %s\n' "${label}" "${valid_labels}" >&2
		exit 2
		;;
	esac
fi

case "${mode}" in
normal) readonly build_dir="${repo_root}/tests/host/build" ;;
sanitizers) readonly build_dir="${repo_root}/tests/host/build-sanitizers" ;;
coverage) readonly build_dir="${repo_root}/tests/host/build-coverage" ;;
*)
	printf 'internal error: unsupported host-test mode %s\n' "${mode}" >&2
	exit 2
	;;
esac

if [[ "${mode}" != "normal" ]]; then
	rm -rf -- "${build_dir}"
fi

cmake \
	-S "${repo_root}/tests/host" \
	-B "${build_dir}" \
	-DHOST_TEST_MODE="${mode}" \
	-DCMAKE_PROJECT_INCLUDE="${repo_root}/tests/host/cmake/host_test_project.cmake"
cmake --build "${build_dir}" --parallel

ctest_args=(--test-dir "${build_dir}" --output-on-failure)
if [[ -n "${label}" ]]; then
	ctest_args+=(-L "${label}")
fi

if [[ "${mode}" == "sanitizers" ]]; then
	env \
		ASAN_OPTIONS="detect_leaks=1:halt_on_error=1:abort_on_error=1:allocator_may_return_null=0" \
		UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1" \
		ctest "${ctest_args[@]}"
else
	ctest "${ctest_args[@]}"
fi
