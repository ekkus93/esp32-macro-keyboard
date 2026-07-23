#!/usr/bin/env bash
set -euo pipefail

shellcheck scripts/*.sh
shfmt -d scripts/*.sh
bash -n scripts/*.sh
