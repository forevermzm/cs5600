#!/bin/sh

# any scripting support you need for running the test. (e.g. creating
# image files, running the actual test executable)
 
# Compile the mirror-test.c.
sh make-mirror-test.sh

# First, put 10 blocks of Cs in testC.img, Ds in testD.img
dd if=/dev/zero bs=512 count=10 | tr '\0' 'C' > mirror/disk1.img
dd if=/dev/zero bs=512 count=10 | tr '\0' 'D' > mirror/disk2.img
dd if=/dev/zero bs=512 count=10 | tr '\0' 'R' > mirror/replace_disk.img

./mirror-test mirror/disk1.img mirror/disk2.img

