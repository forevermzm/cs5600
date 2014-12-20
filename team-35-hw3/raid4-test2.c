#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include "blkdev.h"

int memtest(unsigned char *ptr, int val, int len)
{
    int i;
    for (i = 0; i < len; i++)
        if (ptr[i] != val)
            return 0;
    return 1;
}

int main(int argc, char **argv)
{
    /* we'll just create the disks in the C program */
    char filename[10][32];
    int vals[4] = {0x11, 0x22, 0x44, 0x77};
    int i, j, fd;
    void *buf = malloc(512);
    struct blkdev *disks[4];
    
    for (i = 0; i < 4; i++) {
        sprintf(filename[i], "/tmp/test2-%d.img", i);
        fd = open(filename[i], O_TRUNC | O_CREAT | O_WRONLY, 0666);
        memset(buf, vals[i], 512);
        for (j = 0; j < 256; j++)
            write(fd, buf, 512);
        close(fd);
        disks[i] = image_create(filename[i]);
    }

    /* chunk size 1 - should read back [11], [22], [33], [11],...
     */
    struct blkdev *raid = raid4_create(4, disks, 1);
    assert(raid != NULL);
    printf("read, chunk size 1: ");
    for (i = 0; i < 256*3; i++) {
        raid->ops->read(raid, i, 1, buf);
        assert(memtest(buf, vals[i%3], 512));
    }
    raid->ops->close(raid);
    printf(" OK\n");

    /* chunk size 2 - should read back [11], [11], [22], [22], [33], ...
     */
    printf("read, chunk size 2:");
    for (i = 0; i < 4; i++)
        disks[i] = image_create(filename[i]);
    raid = raid4_create(4, disks, 2);
    assert(raid != NULL);
    for (i = 0; i < 256*3; i++) {
        raid->ops->read(raid, i, 1, buf);
        assert(memtest(buf, vals[(i/2)%3], 512));
    }
    raid->ops->close(raid);
    printf(" OK\n");

    /* now let's test read with a failed disk, chunk size=2
     */
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++)
            disks[j] = image_create(filename[j]);
        raid = raid4_create(4, disks, 2);
        assert(raid != NULL);
        printf("fail disk %d, read back:", i);
        image_fail(disks[i]);
        for (j = 0; j < 256*3; j++) {
            raid->ops->read(raid, j, 1, buf);
            assert(memtest(buf, vals[(j/2)%3], 512));
        }
        raid->ops->close(raid);
        printf(" OK\n");
    }

    /* create a non-uniform data pattern, so we can tell if you wrote
     * something to the wrong place.
     */
    char *data = "the quick brown fox jumped over the lazy dog testing";
    char val2[4][16], *p = data;
    for (i = 0; i < 16; i++)
        for (j = 0; j < 3; j++)
            val2[j][i] = *p++;
    for (i = 0; i < 16; i++)
        val2[3][i] = val2[0][i] ^ val2[1][i] ^ val2[2][i];

    /* create our RAID volume again
     */
    for (j = 0; j < 4; j++)
        disks[j] = image_create(filename[j]);
    raid = raid4_create(4, disks, 1);
    assert(raid != NULL);

    printf("verifying data written to correct location: ");
    
    /* write to it and verify that the right data gets in the right place
     */
    for (i = 0; i < 256*3; i++) {
        j = i % 3;
        memset(buf, val2[i%3][(i/3)%16], 512);
        raid->ops->write(raid, i, 1, buf);
    }
    raid->ops->close(raid);

    for (i = 0; i < 4; i++) {
        fd = open(filename[i], O_RDONLY);
        for (j = 0; j < 256; j++) {
            read(fd, buf, 512);
            assert(memtest(buf, val2[i][j%16], 512));
        }
        close(fd);
    }
    printf(" OK\n");
            
    for (i = 0; i < 4; i++)
        unlink(filename[i]);
    return 0;
}
