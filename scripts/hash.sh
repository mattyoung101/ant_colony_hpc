#!/bin/bash
# Copyright (c) 2022 Matt Young. All rights reserved.
#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
# If a copy of the MPL was not distributed with this file, You can obtain one at
# http://mozilla.org/MPL/2.0/.

# Hashes all the PNG files in a PNG TAR, for comparing results across code changes
# Makes sure they're identical
TMPDIR=$(mktemp -d /tmp/ants_XXXXXXXX)

echo "Extracting tar $1 to $TMPDIR"
tar -xf "$1" -C "$TMPDIR"
# !!!!!!!!!!!!!! ANY ADDITIONAL FILES YOU ADD, YOU MUST ADD THEM HERE TO EXCLUDE FROM THE HASH !!!!!!!!!!!!!!!!!!!!!
rm $TMPDIR/stats.txt
rm $TMPDIR/ants_vs_time.csv

echo "Processing"
cd $TMPDIR
# https://unix.stackexchange.com/a/35834/374455
find . -type f -exec sha256sum {} \; | sha256sum

echo "Cleaning up"
rm -r $TMPDIR