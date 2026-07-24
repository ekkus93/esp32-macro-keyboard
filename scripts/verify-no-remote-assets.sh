#!/usr/bin/env bash
set -euo pipefail

readonly directory="${1:?usage: verify-no-remote-assets.sh DIRECTORY}"

# Flag only genuine remote *asset references* — the mechanisms by which a built
# page would load code, styles, fonts, or images from the network. Bare URL
# strings in comments or error messages (a license header, a framework's
# error-doc link) are not loads and must not trip the check.
#
# Covered load vectors, with an absolute or protocol-relative remote URL:
#   HTML/JS : src= / href= attributes
#   CSS     : url(...) and @import
#   JS      : import ... from '<url>'  and  import('<url>')
remote='(https?:)?//'
if grep -RInE \
	--include='*.html' --include='*.css' --include='*.js' \
	-e "(src|href)[[:space:]]*=[[:space:]]*[\"']?${remote}" \
	-e "url\([[:space:]]*[\"']?${remote}" \
	-e "@import[[:space:]]+[\"']?${remote}" \
	-e "from[[:space:]]+[\"']${remote}" \
	-e "import\([[:space:]]*[\"']${remote}" \
	"${directory}"; then
	printf 'error: production web assets reference a remote URL\n' >&2
	exit 1
fi
