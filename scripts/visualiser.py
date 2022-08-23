#!/usr/bin/env python3
import sys
import time

if __name__ == "__main__":
    # basically what we want to do is open the specified PNG TAR, and play back the frames at some
    # rate probably in matplotlib
    try:
        filename = sys.argv[1]
    except IndexError:
        print(f"Usage: visualiser.py <filename>.tar")
        sys.exit(1)
