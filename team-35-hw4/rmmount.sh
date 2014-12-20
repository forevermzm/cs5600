#!/bin/bash

echo $1
MNT=/tmp/mnt-$1
TMP=/tmp/tmp-$1
DISK=/tmp/disk.$1.img

"fusermount -u $MNT; rm -rf $MNT; rm -f $TMP $DISK" 
