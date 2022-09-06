#!/bin/bash
# Hashes all the PNG files in a PNG TAR, for comparing results across code changes
# Makes sure they're identical
TMPDIR=$(mktemp -d /tmp/ants_XXXXXXXX)

echo "Extracting tar $1 to $TMPDIR"
tar -xf "$1" -C "$TMPDIR"
# remove the stats file because that changes when image data doesn't
rm $TMPDIR/stats.txt

echo "Processing"
cd $TMPDIR
# https://unix.stackexchange.com/a/35834/374455
find . -type f -exec sha256sum {} \; | sha256sum

echo "Cleaning up"
rm -r $TMPDIR