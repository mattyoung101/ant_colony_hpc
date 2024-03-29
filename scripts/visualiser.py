#!/usr/bin/env python3
# Copyright (c) 2022 Matt Young. All rights reserved.
#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
# If a copy of the MPL was not distributed with this file, You can obtain one at
# http://mozilla.org/MPL/2.0/.
import sys
import time
import tarfile
import cv2
import numpy as np
# TODO support readline
import readline

# Desired milliseconds between each frame
FRAME_MS = 10.0
# Factor to resize the images by, 1.0 = same size
SCALE_FACTOR = 5.0


# https://stackoverflow.com/a/25198846/5007892
def tarfile_image(tar_file):
    return np.asarray(bytearray(tar_file.read()), dtype=np.uint8)


if __name__ == "__main__":
    # basically what we want to do is open the specified PNG TAR, and play back the frames at some
    # rate probably in matplotlib
    while True:
        try:
            filename = input("Filename: ")
            with tarfile.open(filename) as tar:
                images = [x for x in tar.getmembers() if x.name.endswith(".png")]
                for file in images:
                    # get the image out of the tar and load into opencv
                    png_file = tar.extractfile(file)
                    img = cv2.imdecode(tarfile_image(png_file), cv2.IMREAD_COLOR)

                    # render
                    img = cv2.resize(img, (0, 0), fx=SCALE_FACTOR, fy=SCALE_FACTOR, interpolation=cv2.INTER_NEAREST)
                    cv2.imshow("Display", img)

                    key = cv2.waitKey(int(FRAME_MS))
                    # check if escape was pressed
                    if key == 27:
                        break
        except Exception as e:
            if isinstance(e, EOFError):
                break
            print(f"Failed to display recording {e} {type(e)}")
            print(e)
            continue
