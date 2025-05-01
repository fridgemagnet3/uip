#!/bin/bash -e

# adds zero padding to a file to get size multiple of 256

size=$(stat --format=%s $1)
newsize=$(( (size+255)/256*256 ))
dd if=/dev/zero of=$1 conv=notrunc oflag=append bs=1 count=$((newsize-size)) status=noxfer
