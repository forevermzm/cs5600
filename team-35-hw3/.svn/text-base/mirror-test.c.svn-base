#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "blkdev.h"

/* example main() function that takes several disk image filenames as
 * arguments on the command line.
 * Note that you'll need three images to test recovery after a failure.
 */
int main(int argc, char **argv)
{
    /* create the underlying blkdevs from the images
     */
    struct blkdev *d1 = image_create(argv[1]);
    struct blkdev *d2 = image_create(argv[2]);
    /* ... */

    /* create the mirror
     */
    struct blkdev *disks[] = {d1, d2};
    struct blkdev *mirror = mirror_create(disks);

    /* two asserts - that the mirror create worked, and that the size
     * is correct.
    */
    assert(mirror != NULL);
    assert(mirror->ops->num_blocks(mirror) == d1->ops->num_blocks(d1));

    /* put your test code here. Errors should exit via either 'assert'
     * or 'exit(1)' - that way the shell script knows that the test failed.
     */
    const int TOTAL_BLKS = 10;
    // Test for write and read to volume.
    int lba = 2;
    int count = 4;
    char zs[ count * BLOCK_SIZE ];
    memset( zs, 'Z', count * BLOCK_SIZE );

    // Write to incorrect address.
    assert ( mirror -> ops -> write ( mirror, -2, count, zs )
             == E_BADADDR );
    assert ( mirror -> ops -> write ( mirror, lba, 15, zs)
             == E_BADADDR );

    // Write Zs to volume.
    assert ( mirror -> ops -> write ( mirror, lba, count, zs )
             == SUCCESS );

    // Read from volume.
    char test1[ count * BLOCK_SIZE ];
    assert ( mirror -> ops -> read ( mirror, lba, count, test1 )
             == SUCCESS );
    assert ( strncmp ( zs, test1, count * BLOCK_SIZE ) == 0 );

    // Write and read with a different size.
    lba = 0, count = TOTAL_BLKS;
    char ys[ TOTAL_BLKS * BLOCK_SIZE ];
    char test2[ TOTAL_BLKS * BLOCK_SIZE ];
    memset ( ys, 'Y', TOTAL_BLKS * BLOCK_SIZE );
    assert ( mirror -> ops -> write ( mirror, lba, count, ys )
             == SUCCESS );
    assert ( mirror -> ops -> read ( mirror, lba, count, test2 )
             == SUCCESS );
    assert ( strncmp( ys, test2, count * BLOCK_SIZE ) == 0 );


    // Continue read and write after image fail.
    image_fail ( d1 );
    lba = 6, count = 3;
    char xs[ count * BLOCK_SIZE ];
    char test3[ count * BLOCK_SIZE ];
    memset ( xs, 'X', count * BLOCK_SIZE );
    assert ( mirror -> ops -> write ( mirror, lba, count, xs )
             == SUCCESS );
    assert ( mirror -> ops -> read ( mirror, lba, count, test3 )
             == SUCCESS );
    assert ( strncmp( xs, test3, count * BLOCK_SIZE ) == 0 );

    // Replace the failed disk.
    // struct blkdev *d1 = image_create(argv[1]);
    struct blkdev *replace_disk = image_create( "mirror/replace_disk.img" );
    assert ( replace_disk != NULL );
    assert ( replace_disk -> ops -> num_blocks( replace_disk ) );
    // Check if the contents has been copied to replace_disk.
    assert ( mirror_replace ( mirror, 0, replace_disk ) == SUCCESS );
    assert ( mirror -> ops -> read ( mirror, lba, count, test3 )
             == SUCCESS );
    assert ( strncmp( xs, test3, count * BLOCK_SIZE ) == 0 );
    // Check read and write again.
    lba = 8, count = 2;
    char ws[ count * BLOCK_SIZE ];
    char test4[ count * BLOCK_SIZE ];
    memset ( ws, 'W', count * BLOCK_SIZE );
    assert ( mirror -> ops -> write ( mirror, lba, count, ws )
             == SUCCESS );
    assert ( mirror -> ops -> read ( mirror, lba, count, test4 )
             == SUCCESS );
    assert ( strncmp( ws, test4, count * BLOCK_SIZE ) == 0 );

    // Continue read and write after another disk is also failed.
    image_fail ( d2 );
    lba = 0, count = 3;
    char vs[ count * BLOCK_SIZE ];
    char test5[ count * BLOCK_SIZE ];
    memset ( vs, 'V', count * BLOCK_SIZE );
    assert ( mirror -> ops -> write ( mirror, lba, count, vs )
             == SUCCESS );
    assert ( mirror -> ops -> read ( mirror, lba, count, test5 )
             == SUCCESS );
    assert ( strncmp( vs, test5, count * BLOCK_SIZE ) == 0 );

    // Fail the last disk. Read and write should be failed.
    image_fail( replace_disk );
    assert ( mirror -> ops -> write ( mirror, lba, count, vs )
             == E_UNAVAIL );
    assert ( mirror -> ops -> read ( mirror, lba, count, test5 )
             == E_UNAVAIL );

    /* suggested test codes (from the assignment PDF)
         You should test that your code:
          - creates a volume properly
          - returns the correct length
          - can handle reads and writes of different sizes, and
            returns the same data as was written
          - reads data from the proper location in the images, and
            doesn't overwrite incorrect locations on write.
          - continues to read and write correctly after one of the
            disks fails. (call image_fail() on the image blkdev to
            force it to fail)
          - continues to read and write (correctly returning data
            written before the failure) after the disk is replaced.
          - reads and writes (returning data written before the first
            failure) after the other disk fails
    */
    mirror -> ops -> close ( mirror );
    printf("Mirror Test: SUCCESS\n");
    return 0;
}
