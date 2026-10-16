#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
artifact_dir="${UXR_FIREFOX_ZOOM_OUTPUT:-"${repo_root}/docs/ui-redesign/phase10-firefox-200zoom"}"
mkdir -p "${artifact_dir}"

docker run --rm --init --interactive --network host \
	--env UXR_FIREFOX_ZOOM_MODE \
	--volume "${repo_root}:/repo:ro" \
	--volume "${artifact_dir}:/artifacts" \
	--workdir /repo/webapp \
	mcr.microsoft.com/playwright:v1.62.1-noble bash -s <<'CONTAINER'
set -euo pipefail

apt-get update -qq
DEBIAN_FRONTEND=noninteractive apt-get install --yes --quiet xdotool

Xvfb :99 -screen 0 2560x2000x24 -ac -nolisten tcp \
    >/tmp/uxr-firefox-zoom-xvfb.log 2>&1 &
xvfb_pid_task=$!
trap 'kill "${xvfb_pid_task}"' EXIT
export DISPLAY=:99
sleep 1

node tests/browser/firefox-zoom-200.mjs
CONTAINER
