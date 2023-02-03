#!/bin/bash
# Copyright (c) 2022 Matt Young. All rights reserved.
#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
# If a copy of the MPL was not distributed with this file, You can obtain one at
# http://mozilla.org/MPL/2.0/.

# Uses ffmpeg to combine all the images together into a gif
TMPDIR=$(mktemp -d /tmp/ants_XXXXXXXX)

echo "Extracting tar $1 to $TMPDIR"
tar -xf "$1" --warning=no-timestamp -C "$TMPDIR"

echo "Processing"
# NOTES:
# - would have like to use vp9 but damn it's slow!
# - you'll have to change the vf scale 800:800 to whatever size you want
# - if file is too large, try changing the crf parameter (higher = worse quality)
# REFERENCES:
# - https://stackoverflow.com/questions/30992760/ffmpeg-scale-video-without-filtering
# - https://trac.ffmpeg.org/wiki/Encode/VP9
# - https://stackoverflow.com/questions/24961127/how-to-create-a-video-from-images-with-ffmpeg
# - https://ottverse.com/how-to-create-gif-from-images-using-ffmpeg/

# use x265
#ffmpeg -y -framerate 40 -i "$TMPDIR/%d.png" -vf scale=960:540:flags=neighbor \
#  -c:v libx265 -crf 20 -b:v 0 ants.mp4

# use vp9
ffmpeg -y -framerate 40 -i "$TMPDIR/%d.png" -vf scale=1024:1024:flags=neighbor \
  -c:v libvpx-vp9 -crf 20 -b:v 0 -deadline good -cpu-used 1 -row-mt 1 -threads 0 ants.webm

echo "Cleaning up"
rm -r $TMPDIR