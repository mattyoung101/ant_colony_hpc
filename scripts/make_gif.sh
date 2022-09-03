#!/bin/bash
# Uses ffmpeg to combine all the images together into a gif
TMPDIR=$(mktemp -d /tmp/ants_XXXXXXXX)

echo "Extracting tar $1 to $TMPDIR"
tar -xf "$1" -C "$TMPDIR"

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
ffmpeg -y -framerate 20 -i "$TMPDIR/%d.png" -vf scale=800:800:flags=neighbor \
  -c:v libx264 -crf 25 -b:v 0 ants.mp4

echo "Cleaning up"
rm -r $TMPDIR

# vp9 is cool but slow:
#ffmpeg -y -framerate 10 -i "/tmp/ants_pqV8ISde/%d.png" -vf scale=800:800:flags=neighbor \
#  -c:v libvpx-vp9 -threads 16 -crf 10 -b:v 0 ants.webm