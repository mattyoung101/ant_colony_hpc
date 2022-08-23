#!/bin/bash
# Uploads the release build to getafix. First tunnel into moss, then upload to getafix.
PROXY=s4697249@moss.labs.eait.uq.edu.au
GETAFIX=s4697249@getafix.smp.uq.edu.au

GETAFIX_PATH=/home/s4697249/workspace/cosc3500/build

# TODO use rsync instead of scp (doesn't copy if already uploaded)

# upload build
scp -oProxyJump=$PROXY cmake-build-release-llvm/ant_colony $GETAFIX:$GETAFIX_PATH
# upload config
scp -oProxyJump=$PROXY antconfig.ini $GETAFIX:$GETAFIX_PATH
# upload maps
scp -r -oProxyJump=$PROXY maps $GETAFIX:$GETAFIX_PATH/maps