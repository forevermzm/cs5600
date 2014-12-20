#sh make-raid4-test.sh

for i in 1 2 3; do
    disk="raid4/disk$i.img"
    disks="$disks $disk"
    dd if=/dev/zero bs=512 count=1024 | tr '\0' $i > $disk
done

dd if=/dev/zero bs=512 count=1024 | tr '\0' 0 > raid4/disk4.img
dd if=/dev/zero bs=512 count=1024 | tr '\0' '\0' > raid4/disk5.img
dd if=/dev/zero bs=512 count=1024 | tr '\0' 6 > raid4/disk6.img

#./raid4-test 5 raid4/disk1.img raid4/disk2.img raid4/disk3.img raid4/disk4.img
