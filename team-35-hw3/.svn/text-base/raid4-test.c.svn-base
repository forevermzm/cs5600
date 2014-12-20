#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "blkdev.h"

int main(int argc, char **argv)
{
    int part1 = false, part2 = false, part3 = false, part4 = false, part5 = false;
    while (argv[1][0] == '-') {
        if (!strcmp(argv[1], "-part1"))
            part1 = true;
        if (!strcmp(argv[1], "-part2"))
            part2 = true;
        if (!strcmp(argv[1], "-part3"))
            part3 = true;
        if (!strcmp(argv[1], "-part4"))
            part4 = true;
        if (!strcmp(argv[1], "-part5"))
            part5 = true;
        argc--;
        argv++;
    }
    if (!(part1 || part2 || part3 || part4 || part5))
        part1 = part2 = part3 = part4 = part5 = true;

    struct blkdev *disks[10];
    int i, ndisks, stripesize = atoi(argv[1]);
    for (i = 2, ndisks = 0; i < argc; i++)
        disks[ndisks++] = image_create(argv[i]);
    disks[ndisks] = NULL;

    struct blkdev *raid = raid4_create(ndisks, disks, stripesize);
    assert(raid != NULL);
    printf("%d \n", ndisks);
    int nblks = disks[0]->ops->num_blocks(disks[0]);
    nblks = nblks - (nblks % stripesize);
    assert(raid->ops->num_blocks(raid) == (ndisks - 1)*nblks);

    int one_chunk = stripesize * BLOCK_SIZE;
    int ndata = ndisks - 1;
    char *buf = calloc(ndata * one_chunk + 100, sizeof(char));
    for (i = 0; i < ndata; i++)
        memset(buf + i * one_chunk, 'A' + i, one_chunk);

    int result, j;
    char *buf2 = calloc(ndata * one_chunk + 100, 1);

    if (part1) {
        // printf( "Testing part1.\n" );
        for (i = 0; i < 16; i++) {
            result = raid->ops->write(raid, i * ndata * stripesize,
                                      ndata * stripesize, buf);
            assert(result == SUCCESS);
        }

        for (i = 0; i < 16; i++) {
            result = raid->ops->read(raid, i * ndata * stripesize,
                                     ndata * stripesize, buf2);
            assert(result == SUCCESS);
            assert(memcmp(buf, buf2, ndata * one_chunk) == 0);
        }

        /* now we test that the data got onto the disks in the right
         * places.
         */
        for (i = 0; i < ndata; i++) {
            for (j = 0; j < 16; j++) {
                result = disks[i]->ops->read(disks[i], j * stripesize,
                                             stripesize, buf2);
                assert(result == SUCCESS);
                assert(memcmp(buf + i * one_chunk, buf2, one_chunk) == 0);
            }
        }
    }

    /* now we test that small writes work
     */
    if (part2) {
        // printf( "Testing part2.\n" );
        for (i = 0; i < ndata; i++)
            memset(buf + i * one_chunk, 'a' + i, one_chunk);

        for (i = 0; i < 8; i++) {
            for (j = 0; j < ndata * stripesize; j ++) {
                result = raid->ops->write(raid, i * ndata * stripesize + j, 1,
                                          buf + j * BLOCK_SIZE);
                assert(result == SUCCESS);
            }
        }

        for (i = 0; i < 8; i++) {
            result = raid->ops->read(raid, i * ndata * stripesize,
                                     ndata * stripesize, buf2);
            assert(result == SUCCESS);
            assert(memcmp(buf, buf2, ndata * one_chunk) == 0);
        }
    }
    /* finally we test that large and overlapping writes work.
     */
    char *big = calloc(5 * ndata * one_chunk + 100, 1);
    memset(big, 'Q', 5 * ndata * one_chunk);
    char *big2 = calloc(5 * ndata * one_chunk + 100, 1);
    int offset = ndata * stripesize / 2;

    if (part3) {
        // printf( "Testing part3.\n" );
        result = raid->ops->write(raid, offset, 5 * ndata * stripesize, big);
        assert(result == SUCCESS);

        result = raid->ops->read(raid, offset, 5 * ndata * stripesize, big2);
        assert(result == SUCCESS);
        assert(memcmp(big, big2, 5 * ndata * one_chunk) == 0);

        /* and check we didn't muck up any previously-written data
         */
        result = raid->ops->read(raid, 0, offset, buf2);
        assert(result == SUCCESS);
        assert(memcmp(buf, buf2, offset * BLOCK_SIZE) == 0);

        result = raid->ops->read(raid, 5 * ndata * stripesize + offset, offset, buf2);
        assert(result == SUCCESS);
        assert(memcmp(buf + offset * BLOCK_SIZE, buf2, offset * BLOCK_SIZE) == 0);
    }

    /* fail a disk and see that we can read back again */
    if (part4) {
        // printf( "Testing part4.\n" );
        char line[2 * ndata * one_chunk];
        char line2[2 * ndata * one_chunk];
        int i;
        for (i = 0; i < 2 * ndata * one_chunk; i++) {
            line[i] = 'a';
            line2[i] = 'b';
        }

        assert (raid->ops->write(raid, 0, 2 * ndata * stripesize, line) == SUCCESS) ;
        image_fail(disks[0]);
        assert( result = raid->ops->read(raid, 0, 2 * ndata * stripesize, line2) == SUCCESS );
        assert(strncmp(line, line2, 2 * ndata * one_chunk) == 0);
        // printf("OKOKOK\n");

        result = raid->ops->write(raid, 8600000, 2 * ndata * stripesize, line);
        result = raid->ops->read(raid, 8600000, 2 * ndata * stripesize, line2);
        assert(memcmp(line, line2, 2 * ndata * one_chunk) == 0);
    }

    if (part5) {
        printf( "Testing part5.\n" );
        // struct blkdev *xtra = image_create("raid4/disk6.img");
        // result = raid4_replace(raid, 0, xtra);
        // assert(result == SUCCESS);

        // for (i = 0; i < 16; i++) {
        //     memset(buf2, 0, ndata * stripesize * BLOCK_SIZE);
        //     result = raid->ops->read(raid, i * ndata * stripesize,
        //                              ndata * stripesize, buf2);
        //     assert(result == SUCCESS);
        //     assert(strncmp(buf, buf2, ndata * one_chunk) == 0);
        // }
        // image_fail(disks[1]);
        // buf[0] = 'z';
        // for (i = 0; i < 16; i++) {
        //     raid->ops->write(raid, i * ndata * stripesize,
        //                      ndata * stripesize, buf);
        //     assert(result == SUCCESS);
        // }

        // for (i = 0; i < 16; i++) {
        //     result = raid->ops->read(raid, i * ndata * stripesize,
        //                              ndata * stripesize, buf2);
        //     assert(result == SUCCESS);
        //     assert(memcmp(buf, buf2, ndata * one_chunk) == 0);
        // }

    }

    printf("RAID4 Test: SUCCESS\n");
    return 0;
}
