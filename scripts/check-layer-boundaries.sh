#!/usr/bin/env bash
set -euo pipefail

# Two prohibitions that are properties of which component may call what, rather
# than behaviours a unit test can observe. Both currently hold; neither was
# enforced, so nothing would have caught a regression.
#
# SPEC 11.5: "There is one macro-executor task. HTTP handlers MUST NOT type
# directly." The web layer already links usb_keyboard -- it needs
# usb_keyboard_state_t to report USB readiness -- so the typing entry points are
# in scope for it and a handler calling them would compile cleanly. What keeps
# input serialized is that only the executor calls them.
#
# SPEC 13.2: "Web-assets failure MUST NOT expose an unauthenticated fallback UI."
# The static route is reachable before any session exists, so a built-in page
# compiled into the firmware and served when the partition is unavailable is
# exactly the forbidden fallback. The behavioural half -- that a failed open
# yields nothing to serve -- is covered by the web_server_adapter host tests;
# this checks that no such page exists to be served in the first place.

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
readonly repo_root
cd "${repo_root}"

status=0

# --- SPEC 11.5: only the executor may drive the keyboard -------------------
typing_callers="$(grep -rln --include='*.c' \
	-E '\busb_keyboard_(press|release_all)[[:space:]]*\(' \
	firmware/components firmware/main 2>/dev/null || true)"
for file in ${typing_callers}; do
	case "${file}" in
	firmware/components/macro_executor/* | firmware/components/usb_keyboard/*) ;;
	*)
		echo "error: ${file} types directly; only the macro executor may (SPEC 11.5)" >&2
		status=1
		;;
	esac
done

# --- SPEC 13.2: no fallback UI compiled into the firmware ------------------
# A served page is a whole document. Matching on the document markers rather than
# on "<div" or similar avoids flagging an error string that merely contains a
# tag-like fragment, which is not a UI.
embedded_ui="$(grep -rlni --include='*.c' --include='*.h' \
	-E '<!doctype[[:space:]]+html|<html[[:space:]>]' \
	firmware/components firmware/main 2>/dev/null || true)"
if [ -n "${embedded_ui}" ]; then
	echo "error: firmware embeds an HTML document, which the static route could serve" >&2
	echo "       without a session when web assets are unavailable (SPEC 13.2)" >&2
	echo "${embedded_ui}" >&2
	status=1
fi

if [ "${status}" -eq 0 ]; then
	echo "layer boundaries: executor owns typing; no fallback UI compiled in"
fi
exit "${status}"
