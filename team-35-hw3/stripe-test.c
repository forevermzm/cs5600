#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "blkdev.h"

int main(int argc, char **argv)
{
    /* This code reads arguments <stripesize> d1 d2 d3 ...
     * Note that you don't need any extra images, as there's no way to
     * repair a striped volume after failure.
     */
    struct blkdev *disks[10];
    int i, ndisks, stripesize = atoi(argv[1]);

    for (i = 2, ndisks = 0; i < argc; i++)
        disks[ndisks++] = image_create(argv[i]);

    /* and creates a striped volume with the specified stripe size
     */
    struct blkdev *striped = striped_create(ndisks, disks, stripesize);
    assert(striped != NULL);

    /* your tests here */

    // First, check the size.
    int nblks = disks[ 0 ] -> ops -> num_blocks( disks[0] );
    nblks = nblks - ( nblks % stripesize );
    assert ( striped -> ops -> num_blocks( striped ) == ndisks * nblks );


    int lba = 2;
    int count = 128;
    char buffer1[ count * BLOCK_SIZE ];
    // Read or write to invalid address.
    assert ( striped -> ops -> read ( striped, -3, 4, buffer1) ==
             E_BADADDR );
    assert ( striped -> ops -> write ( striped, 100, 1024 * 6,
                                       buffer1) == E_BADADDR );

    // Write first, then read from that location.
    memset( buffer1, 'A', count * BLOCK_SIZE );
    assert( striped -> ops -> write ( striped, lba, count, buffer1 ) == SUCCESS );
    char read_buffer1[ count * BLOCK_SIZE ];
    assert ( striped -> ops -> read ( striped, lba, count, read_buffer1 ) == SUCCESS );
    assert ( strncmp ( buffer1, read_buffer1, count * BLOCK_SIZE ) == 0);


    int one_chunk = stripesize * BLOCK_SIZE;
    char buf[ ndisks * one_chunk ];
    for (i = 0; i < ndisks; i++)
        memset(buf + i * one_chunk, 'A' + i, one_chunk);

    int j;
    for (i = 0; i < 16; i++) {
        assert( striped -> ops -> write( striped, i * ndisks *
                                         stripesize, ndisks * stripesize, buf ) == SUCCESS );
    }

    char buf2[ ndisks * one_chunk ];

    for (i = 0; i < 16; i++) {
        assert( striped -> ops -> read( striped, i * ndisks *
                                        stripesize, ndisks * stripesize, buf2 ) == SUCCESS );
        assert( strncmp(buf, buf2, ndisks * one_chunk) == 0 );
    }

    /* now we test that the data got onto the disks in the right
     * places.
     */
    for (i = 0; i < ndisks; i++) {
        assert( disks[i] -> ops -> read( disks[i], i * stripesize,
                                         stripesize, buf2 ) == SUCCESS );
        assert( strncmp( buf + i * one_chunk, buf2, one_chunk ) == 0);
    }

    /* now we test that small writes work
     */
    for (i = 0; i < ndisks; i++)
        memset( buf + i * one_chunk, 'a' + i, one_chunk );

    for (i = 0; i < 8; i++) {
        for (j = 0; j < ndisks * stripesize; j ++) {
            assert( striped -> ops -> write( striped, i * ndisks *
                                             stripesize + j, 1, buf + j * BLOCK_SIZE ) == SUCCESS);
        }
    }

    for (i = 0; i < 8; i++) {
        assert( striped -> ops -> read( striped, i * ndisks * stripesize, ndisks * stripesize, buf2 ) == SUCCESS );
        assert( strncmp ( buf, buf2, ndisks * one_chunk ) == 0 );
    }

    /* finally we test that large and overlapping writes work.
     */
    char big[ 5 * ndisks * one_chunk ];
    for (i = 0; i < 5; i++)
        for (j = 0; j < ndisks; j++)
            memset( big + j * one_chunk + i * ndisks * one_chunk, 'f' + i, one_chunk );

    int offset = ndisks * stripesize / 2;
    assert( striped -> ops -> write( striped, offset, 5 * ndisks *
                                     stripesize, big ) == SUCCESS );

    char big2[ 5 * ndisks * one_chunk ];
    assert( striped -> ops -> read( striped, offset, 5 * ndisks *
                                    stripesize, big2) == SUCCESS);
    assert( strncmp( big, big2, 5 * ndisks * one_chunk ) == 0 );

    /* and check we didn't muck up any previously-written data
     */
    assert( striped -> ops -> read( striped, 0, offset, buf2 ) ==
            SUCCESS );
    assert( strncmp( buf, buf2, offset * BLOCK_SIZE ) == 0 );

    assert( striped -> ops -> read( striped, 5 * ndisks *
                                    stripesize + offset, offset,
                                    buf2 ) == SUCCESS );
    assert( strncmp( buf + offset * BLOCK_SIZE, buf2, offset *
                     BLOCK_SIZE) == 0 );

    // now we fail a disk
    image_fail( disks[0] );
    assert( striped -> ops -> read( striped, 0, 1, buffer1 ) ==
            E_UNAVAIL);

    /* from pdf:
       - reports the correct size
       - reads data from the right disks and locations. (prepare disks
         with known data at various locations and make sure you can read it back)
       - overwrites the correct locations. i.e. write to your prepared disks
         and check the results (using something other than your stripe
         code) to check that the right sections got modified.
       - fail a disk and verify that the volume fails.
       - large (> 1 stripe set), small, unaligned reads and writes
         (i.e. starting, ending in the middle of a stripe), as well as small
         writes wrapping around the end of a stripe.
       - Passes all your other tests with different stripe sizes (e.g. 2,
         4, 7, and 32 sectors) and different numbers of disks.
     */
    striped -> ops -> close ( striped );

    printf("Striping Test: SUCCESS\n");
    return 0;
}
