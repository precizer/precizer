#!/usr/bin/env bash

# Creates the complete Markdown body used for a precizer GitHub Release.
# The script accepts a release tag and the path of the output file. Release tags
# may use either the X.Y.Z or vX.Y.Z form.
#
# The matching release description is taken from CHANGELOG.md. Current
# "Release X.Y.Z" headings and legacy "vX.Y.Z" headings are supported. The
# changelog heading itself is omitted because the GitHub Release title already
# identifies the version.
#
# The release description is followed by the curated download and documentation
# section. Its links point to assets and repository files for the requested
# release tag. The completed body replaces the requested output file.
#
# Invalid release tags and missing or empty changelog entries stop release-note
# generation. CHANGELOG.md and the release download template are read-only
# inputs and are never modified

set -euo pipefail

if [ "$#" -ne 2 ]; then
	echo "Usage: $0 <release-tag> <output-file>" >&2
	echo "Example: $0 0.17.0 release-notes.md" >&2
	exit 1
fi

release_tag="$1"
output_file="$2"
version="${release_tag#v}"

if [[ ! "$release_tag" =~ ^v?[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
	echo "Error: release tag '$release_tag' does not match X.Y.Z or vX.Y.Z" >&2
	exit 1
fi

escaped_version="${version//./\\.}"
selector="#{1} /^(Release |v)${escaped_version}( |$)/"

if ! mdq --link-format keep --no-br "$selector" CHANGELOG.md |
	sed '1,2d' > "$output_file"; then
	rm -f "$output_file"
	echo "Error: failed to read the CHANGELOG.md release section for version $version" >&2
	exit 1
fi

if [ ! -s "$output_file" ]; then
	rm -f "$output_file"
	echo "Error: CHANGELOG.md has no non-empty release section for version $version" >&2
	exit 1
fi

printf '\n' >> "$output_file"
sed "s|{{TAG}}|$release_tag|g" .github/release-downloads.md >> "$output_file"
