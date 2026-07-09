#!/bin/sh
set -eu

version=4.0.3
archive="monocypher-${version}.tar.gz"
tmpdir="$(mktemp -d)"

trap 'rm -rf "$tmpdir"' EXIT

curl -fsSL "https://monocypher.org/download/${archive}" -o "$tmpdir/${archive}"
tar -xzf "$tmpdir/${archive}" -C "$tmpdir"

cp "$tmpdir/monocypher-${version}/src/monocypher.c" src/
cp "$tmpdir/monocypher-${version}/src/monocypher.h" src/
cp "$tmpdir/monocypher-${version}/src/optional/monocypher-ed25519.c" src/
cp "$tmpdir/monocypher-${version}/src/optional/monocypher-ed25519.h" src/
