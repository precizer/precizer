#!/usr/bin/env bash
set -euo pipefail

if [ $# -lt 1 ]; then
    echo "Usage: $0 <version> [remote]" >&2
    echo "Example: $0 0.2.11 origin" >&2
    exit 1
fi

TAG="$1"
REMOTE="${2:-origin}"
VERSION_FILE="src/version.h"

# Validate version format: X.Y.Z (e.g. 1.2.3)
VERSION_REGEX='^[0-9]+\.[0-9]+\.[0-9]+$'
if ! [[ "$TAG" =~ $VERSION_REGEX ]]; then
    echo "Error: version '$TAG' does not match pattern X.Y.Z (e.g. 1.2.3)" >&2
    exit 1
fi

# Ensure we are inside a git repository
if ! git rev-parse --git-dir >/dev/null 2>&1; then
    echo "Error: not a git repository" >&2
    exit 1
fi

# 1. Check and delete existing local tag, if any
if git rev-parse -q --verify "refs/tags/$TAG" >/dev/null 2>&1; then
    echo "Local tag '$TAG' already exists, deleting..."
    git tag -d "$TAG"
fi

# 1a. Check and delete existing remote tag, if any
if git ls-remote --tags "$REMOTE" "refs/tags/$TAG" | grep -q "refs/tags/$TAG"; then
    echo "Remote tag '$TAG' exists on '$REMOTE'. Deleting remote tag..."
    git push --delete "$REMOTE" "$TAG"
fi

# 2. Update src/version.h with the new version
if [ ! -f "$VERSION_FILE" ]; then
    echo "Error: $VERSION_FILE not found" >&2
    exit 1
fi

tmp_file="${VERSION_FILE}.tmp"

# Replace the line: #define APP_VERSION "..."
sed -E "s|^#define[[:space:]]+APP_VERSION[[:space:]]+\"[^\"]*\"|#define APP_VERSION \"$TAG\"|" \
    "$VERSION_FILE" > "$tmp_file"

mv "$tmp_file" "$VERSION_FILE"

echo "Updated $VERSION_FILE to APP_VERSION \"$TAG\""

# Stage version file
git add "$VERSION_FILE"

# Commit only if there is an actual change (in case the version was already the same)
if git diff --cached --quiet; then
    echo "No changes in $VERSION_FILE to commit (APP_VERSION is already '$TAG'). Skipping commit."
else
    git commit -m "Bump version to $TAG"
fi

# 3. Create local annotated tag
git tag -a "$TAG" -m "Release $TAG"
echo "Created local tag '$TAG'"

# 4. Push the tag to the remote repository
git push "$REMOTE" "$TAG"
echo "Pushed tag '$TAG' to remote '$REMOTE'"

echo "Done"
