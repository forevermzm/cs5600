/*
 * file:        homework.c
 * description: skeleton code for CS 5600 Homework 3
 *
 * Peter Desnoyers, Northeastern Computer Science, 2011
 * $Id: homework.c 410 2011-11-07 18:42:45Z pjd $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "blkdev.h"

/********** MIRRORING ***************/

/* example state for mirror device. See mirror_create for how to
 * initialize a struct blkdev with this.
 */
struct mirror_dev {
    struct blkdev *disks[2];    /* flag bad disk by setting to NULL */
    int nblks;
};

static int mirror_num_blocks(struct blkdev *dev)
{
    /* your code here */
    struct mirror_dev *m_dev = dev -> private;
    return m_dev -> nblks;
}

/* read from one of the sides of the mirror. (if one side has failed,
 * it had better be the other one...) If both sides have failed,
 * return an error.
 * Note that a read operation may return an error to indicate that the
 * underlying device has failed, in which case you should close the
 * device and flag it (e.g. as a null pointer) so you won't try to use
 * it again.
 */
static int mirror_read(struct blkdev *dev, int first_blk,
                       int num_blks, void *buf)
{
    /* your code here */
    struct mirror_dev *m_dev = dev -> private;

    // Check if the requested address is valid.
    if ( first_blk < 0 || first_blk + num_blks > m_dev -> nblks ) {
        return E_BADADDR;
    }

    int i;
    for ( i = 0 ; i < 2 ; i ++ ) {
        struct blkdev *disk = m_dev -> disks[ i ];
        if ( disk == NULL ) {
            continue;
        } else {
            // Try to read fron disk.
            int result = disk -> ops -> read ( disk, first_blk,
                                               num_blks, buf );

            if ( result == SUCCESS ) {
                return SUCCESS;
            } else if ( result == E_UNAVAIL ) {
                // Close and set the disk to NULL.
                disk -> ops -> close ( disk );
                m_dev -> disks[ i ] = NULL;
                continue;
            }
        }
    }
    // Non of the disks is working. return E_UNAVAIL.
    return E_UNAVAIL;
}

/* write to both sides of the mirror, or the remaining side if one has
 * failed. If both sides have failed, return an error.
 * Note that a write operation may indicate that the underlying device
 * has failed, in which case you should close the device and flag it
 * (e.g. as a null pointer) so you won't try to use it again.
 */
static int mirror_write(struct blkdev *dev, int first_blk,
                        int num_blks, void *buf)
{
    /* your code here */
    struct mirror_dev *m_dev = dev -> private;

    // Check if the requested address is valid.
    if ( first_blk < 0 || first_blk + num_blks > m_dev -> nblks ) {
        return E_BADADDR;
    }

    int i, result[ 2 ];
    for ( i = 0 ; i < 2 ; i ++ ) {
        struct blkdev *disk = m_dev -> disks[ i ];
        if ( disk == NULL ) {
            result[ i ] = E_UNAVAIL;
            continue;
        } else {
            // Try to read fron disk.
            result[ i ] = disk -> ops -> write ( disk, first_blk,
                                                 num_blks, buf );

            if ( result[ i ] == E_UNAVAIL ) {
                // Close and set the disk to NULL.
                disk -> ops -> close ( disk );
                m_dev -> disks[ i ] = NULL;
            }
        }
    }
    if ( result[ 0 ] == SUCCESS || result[ 1 ] == SUCCESS ) {
        return SUCCESS;
    } else {
        return E_UNAVAIL;
    }
}

/* clean up, including: close any open (i.e. non-failed) devices, and
 * free any data structures you allocated in mirror_create.
 */
static void mirror_close(struct blkdev *dev)
{
    /* your code here */
    struct mirror_dev *m_dev = dev -> private;

    int i;
    for ( i = 0 ; i < 2 ; i ++ ) {
        struct blkdev *disk = m_dev -> disks[ i ];
        if ( disk != NULL ) {
            disk -> ops -> close ( disk );
        }
    }
    free( m_dev );
    free( dev );
}

struct blkdev_ops mirror_ops = {
    .num_blocks = mirror_num_blocks,
    .read = mirror_read,
    .write = mirror_write,
    .close = mirror_close
};

/* create a mirrored volume from two disks. Do not write to the disks
 * in this function - you should assume that they contain identical
 * contents.
 */
struct blkdev *mirror_create(struct blkdev *disks[2])
{
    struct blkdev *dev = malloc(sizeof(*dev));
    struct mirror_dev *mdev = malloc(sizeof(*mdev));

    /* your code here */
    if ( disks[0] -> ops -> num_blocks( disks[0] ) != disks[1] ->
            ops -> num_blocks( disks[1] ) ) {
        printf("The disks size is not equal.\n");
        return NULL;
    }
    mdev -> disks[0] = disks[0];
    mdev -> disks[1] = disks[1];
    mdev -> nblks = disks[0] -> ops -> num_blocks( disks[0] );

    dev->private = mdev;
    dev->ops = &mirror_ops;

    return dev;
}

/* replace failed device 'i' (0 or 1) in a mirror. Note that we assume
 * the upper layer knows which device failed. You will need to
 * replicate content from the other underlying device before returning
 * from this call.
 */
int mirror_replace(struct blkdev *volume, int i, struct blkdev *newdisk)
{
    struct mirror_dev *m_dev = volume -> private;

    // If the size doesn't match, return E_SIZE error.
    if ( m_dev -> nblks != newdisk -> ops -> num_blocks( newdisk ) ) {
        return E_SIZE;
    }

    int working_disk = ~i + 2;
    struct blkdev *disk = m_dev -> disks[ working_disk ];

    // Replicate the working disk.
    char buffer[ BLOCK_SIZE ];
    int lba;
    for ( lba = 0 ; lba < m_dev -> nblks ; lba ++ ) {
        int result = disk -> ops -> read( disk, lba, 1, buffer );
        if ( result != SUCCESS ) {
            return result;
        }
        result = newdisk -> ops -> write ( newdisk, lba, 1, buffer );
        if ( result != SUCCESS ) {
            return result;
        }
    }

    // Replace the failed disk with newdisk.
    m_dev -> disks[ i ] = newdisk;
    return SUCCESS;
}

/**********  STRIPING ***************/

struct stripe_dev {
    struct blkdev **disks;
    int ndisks;     // Number of disks.
    int nblks;      // Number of blocks in each disk.
    int unit;
    int status;
};

enum { WORKING = 1, FAILED = -1 };

int stripe_num_blocks(struct blkdev *dev)
{
    struct stripe_dev *s_dev = dev -> private;
    return ( s_dev -> ndisks ) * ( s_dev -> nblks ) ;
}

/* read blocks from a striped volume.
 * Note that a read operation may return an error to indicate that the
 * underlying device has failed, in which case you should (a) close the
 * device and (b) return an error on this and all subsequent read or
 * write operations.
 */
static int stripe_read(struct blkdev *dev, int first_blk,
                       int num_blks, void *buf)
{
    struct stripe_dev *s_dev = dev -> private;

    // First, let's make sure that volume is working now.
    if ( s_dev -> status == FAILED ) {
        return E_UNAVAIL;
    }

    // Check if the requested address is valid.
    if ( first_blk < 0 || first_blk + num_blks > dev -> ops ->
            num_blocks ( dev ) ) {
        return E_BADADDR;
    }

    int unit = s_dev -> unit;
    int N = s_dev -> ndisks;
    while ( num_blks > 0 ) {
        int disk_num = ( first_blk % ( N * unit ) ) / unit ;
        int lba_in_disk = ( first_blk / ( ( s_dev -> ndisks ) *
                                          unit ) ) * unit + ( first_blk % unit );
        int count_in_disk = unit - ( lba_in_disk % unit );
        if ( count_in_disk > num_blks ) {
            count_in_disk = num_blks;
        }
        // printf ( "Disk num is %d, lba in disk is %d, count in disk is %d\n", disk_num, lba_in_disk, count_in_disk );
        struct blkdev *disk = s_dev -> disks[ disk_num ];
        int result = disk -> ops -> read ( disk, lba_in_disk,
                                           count_in_disk, buf );
        if ( result == SUCCESS ) {
            buf += count_in_disk * BLOCK_SIZE;
            first_blk += count_in_disk;
            num_blks -= count_in_disk;
        } else if ( result == E_UNAVAIL ) {
            disk -> ops -> close ( disk );
            s_dev -> disks[ disk_num ] = NULL;
            s_dev -> status = FAILED;
            return E_UNAVAIL;
        }
    }
    return SUCCESS;
}

/* write blocks to a striped volume.
 * Again if an underlying device fails you should close it and return
 * an error for this and all subsequent read or write operations.
 */
static int stripe_write(struct blkdev *dev, int first_blk,
                        int num_blks, void *buf)
{
    struct stripe_dev *s_dev = dev -> private;

    // First, let's make sure that volume is working now.
    if ( s_dev -> status == FAILED ) {
        return E_UNAVAIL;
    }

    // Check if the requested address is valid.
    if ( first_blk < 0 || first_blk + num_blks > dev -> ops ->
            num_blocks ( dev ) ) {
        return E_BADADDR;
    }

    int unit = s_dev -> unit;
    int N = s_dev -> ndisks;
    while ( num_blks > 0 ) {
        int disk_num = ( first_blk % ( N * unit ) ) / unit ;
        int lba_in_disk = ( first_blk / ( ( s_dev -> ndisks ) *
                                          unit ) ) * unit + ( first_blk % unit );
        int count_in_disk = unit - ( lba_in_disk % unit );
        if ( count_in_disk > num_blks ) {
            count_in_disk = num_blks;
        }
        // printf ( "Disk num is %d, lba in disk is %d, count in disk is %d\n", disk_num, lba_in_disk, count_in_disk );
        struct blkdev *disk = s_dev -> disks[ disk_num ];
        int result = disk -> ops -> write ( disk, lba_in_disk,
                                            count_in_disk, buf );
        if ( result == SUCCESS ) {
            buf += count_in_disk * BLOCK_SIZE;
            first_blk += count_in_disk;
            num_blks -= count_in_disk;
        } else if ( result == E_UNAVAIL ) {
            disk -> ops -> close ( disk );
            s_dev -> disks[ disk_num ] = NULL;
            s_dev -> status = FAILED;
            return E_UNAVAIL;
        }
    }
    return SUCCESS;
}

/* clean up, including: close all devices and free any data structures
 * you allocated in stripe_create.
 */
static void stripe_close(struct blkdev *dev)
{
    struct stripe_dev *s_dev = dev -> private;
    int i;
    for ( i = 0 ; i < s_dev -> ndisks ; i ++ ) {
        struct blkdev *disk = s_dev -> disks[ i ];
        if ( disk != NULL )
            disk -> ops -> close ( disk );
    }

    s_dev -> status = FAILED;
    free ( s_dev );
    free ( dev );
}

struct blkdev_ops stripe_ops = {
    .num_blocks = stripe_num_blocks,
    .read = stripe_read,
    .write = stripe_write,
    .close = stripe_close
};

/* create a striped volume across N disks, with a stripe size of
 * 'unit'. (i.e. if 'unit' is 4, then blocks 0..3 will be on disks[0],
 * 4..7 on disks[1], etc.)
 * Check the size of the disks to compute the final volume size, and
 * fail (return NULL) if they aren't all the same.
 * Do not write to the disks in this function.
 */
struct blkdev *striped_create(int N, struct blkdev *disks[], int unit)
{
    // First, check if the sizes match.
    int disk_size = disks[ 0 ] -> ops -> num_blocks ( disks[ 0 ] );
    int i;
    for ( i = 0 ; i < N ; i ++ ) {
        if ( disk_size != disks[ i ] -> ops -> num_blocks (
                    disks[ i ] ) ) {
            printf( "Failed to create strip disk. Sizes are not equal.");
            return NULL;
        }
    }

    struct blkdev *dev = malloc ( sizeof( *dev ) );
    struct stripe_dev *s_dev = malloc ( sizeof( *s_dev ) );

    s_dev -> disks = disks;
    s_dev -> ndisks = N;
    // Only use the first nblks that is multiple of unit.
    s_dev -> nblks = ( disk_size / unit ) * unit;
    s_dev -> unit = unit;
    s_dev -> status = WORKING;

    dev -> private = s_dev;
    dev -> ops = &stripe_ops;

    return dev;
}

/**********   RAID 4  ***************/

/* helper function - compute parity function across two blocks of
 * 'len' bytes and put it in a third block. Note that 'dst' can be the
 * same as either 'src1' or 'src2', so to compute parity across N
 * blocks you can do:
 *
 *     void **block[i] - array of pointers to blocks
 *     dst = <zeros[len]>
 *     for (i = 0; i < N; i++)
 *        parity(block[i], dst, dst);
 *
 * Yes, it's slow.
 */
void parity(int len, void *src1, void *src2, void *dst)
{
    unsigned char *s1 = src1, *s2 = src2, *d = dst;
    int i;
    for (i = 0; i < len; i++)
        d[i] = s1[i] ^ s2[i];
}

struct raid4_dev {
    struct blkdev **disks;
    struct blkdev *parity;
    int ndisks;
    int nblks;
    int unit;

    int mode;
    int failed_disk;
};

enum { NORMAL = 1, DEGRADED = 0};

int raid_num_blocks( struct blkdev *volume )
{
    struct raid4_dev *r_dev = volume -> private;
    return ( r_dev -> ndisks ) * ( r_dev -> nblks );
}

int get_data_for_disk( struct blkdev *volume, int disk_num, int lba, int count, void *buf)
{
    struct raid4_dev *r_dev = volume -> private;

    memset ( buf, '\0', count * BLOCK_SIZE );
    char temp[ count * BLOCK_SIZE ];
    int i, result;
    for ( i = 0 ; i < r_dev -> ndisks + 1 ; i ++) {
        if ( i != disk_num ) {
            struct blkdev *disk = r_dev -> disks[ i ];
            result = disk -> ops -> read ( disk, lba, count, temp );
            if ( result == E_UNAVAIL ) {
                return result;
            }
            parity( count * BLOCK_SIZE, temp, buf, buf );
        }
    }
    return SUCCESS;
}

/* read blocks from a RAID 4 volume.
 * If the volume is in a degraded state you may need to reconstruct
 * data from the other stripes of the stripe set plus parity.
 * If a drive fails during a read and all other drives are
 * operational, close that drive and continue in degraded state.
 * If a drive fails and the volume is already in a degraded state,
 * close the drive and return an error.
 */
static int raid4_read(struct blkdev *dev, int first_blk,
                      int num_blks, void *buf)
{
    // First, let's make sure that volume is working now.
    if ( dev == NULL ) {
        return E_UNAVAIL;
    }
    struct raid4_dev *r_dev = dev -> private;

    // Check if the requested address is valid.
    if ( first_blk < 0 || first_blk + num_blks > dev -> ops ->
            num_blocks ( dev ) ) {
        return E_BADADDR;
    }

    int unit = r_dev -> unit;
    int N = r_dev -> ndisks;
    while ( num_blks > 0 ) {
        int disk_num = ( first_blk % ( N * unit ) ) / unit ;
        int lba_in_disk = ( first_blk / ( ( r_dev -> ndisks ) *
                                          unit ) ) * unit + ( first_blk % unit );
        int count_in_disk = unit - ( lba_in_disk % unit );
        if ( count_in_disk > num_blks ) {
            count_in_disk = num_blks;
        }
        // printf ( "Disk num is %d, lba in disk is %d, count in disk is %d\n", disk_num, lba_in_disk, count_in_disk );
        struct blkdev *disk = r_dev -> disks[ disk_num ];

        if ( r_dev -> mode == DEGRADED ) {
            if ( r_dev -> failed_disk == disk_num ) {
                if ( get_data_for_disk( dev, disk_num, lba_in_disk, count_in_disk, buf ) == SUCCESS ) {
                    buf += count_in_disk * BLOCK_SIZE;
                    first_blk += count_in_disk;
                    num_blks -= count_in_disk;
                    continue;
                } else {
                    r_dev -> mode = FAILED;
                    dev -> ops -> close( dev );
                    return E_UNAVAIL;
                }
            }
        }

        int result = disk -> ops -> read( disk, lba_in_disk,
                                          count_in_disk, buf );
        if ( result == SUCCESS ) {
            buf += count_in_disk * BLOCK_SIZE;
            first_blk += count_in_disk;
            num_blks -= count_in_disk;
            continue;
        } else {
            disk -> ops -> close( disk );
            r_dev -> disks[ disk_num ] = NULL;
            r_dev -> mode -= 1;
            r_dev -> failed_disk = disk_num;
            if ( r_dev -> mode == FAILED ) {
                dev -> ops -> close( dev );
                return E_UNAVAIL;
            }
            continue;
        }
    }
    return SUCCESS;
}

/* write blocks to a RAID 4 volume.
 * Note that you must handle short writes - i.e. less than a full
 * stripe set. You may either use the optimized algorithm (for N>3
 * read old data, parity, write new data, new parity) or you can read
 * the entire stripe set, modify it, and re-write it. Your code will
 * be graded on correctness, not speed.
 * If an underlying device fails you should close it and complete the
 * write in the degraded state. If a drive fails in the degraded
 * state, close it and return an error.
 * In the degraded state perform all writes to non-failed drives, and
 * forget about the failed one. (parity will handle it)
 */
static int raid4_write(struct blkdev *dev, int first_blk,
                       int num_blks, void *buf)
{
    // First, let's make sure that volume is working now.
    if ( dev == NULL ) {
        return E_UNAVAIL;
    }
    struct raid4_dev *r_dev = dev -> private;

    // Check if the requested address is valid.
    if ( first_blk < 0 || first_blk + num_blks > dev -> ops ->
            num_blocks ( dev ) ) {
        return E_BADADDR;
    }

    int unit = r_dev -> unit;
    int N = r_dev -> ndisks;
    while ( num_blks > 0 ) {
        int disk_num = ( first_blk % ( N * unit ) ) / unit ;
        int lba_in_disk = ( first_blk / ( ( r_dev -> ndisks ) *
                                          unit ) ) * unit + ( first_blk % unit );
        int count_in_disk = unit - ( lba_in_disk % unit );
        if ( count_in_disk > num_blks ) {
            count_in_disk = num_blks;
        }
        // printf ( "Disk num is %d, lba in disk is %d, count in disk is %d\n", disk_num, lba_in_disk, count_in_disk );
        struct blkdev *disk = r_dev -> disks[ disk_num ];
        struct blkdev *parity_disk = r_dev -> parity;
        char temp[ count_in_disk * BLOCK_SIZE ];

        if ( r_dev -> mode == DEGRADED ) {
            if ( r_dev -> failed_disk == disk_num ) {
                if ( get_data_for_disk( dev, disk_num, lba_in_disk, count_in_disk, temp ) == SUCCESS ) {
                    // disk -> ops -> write ( disk, lba_in_disk, count_in_disk, buf );
                } else {
                    dev -> ops -> close ( dev );
                    return E_UNAVAIL;
                }


            } else {
                if ( disk -> ops -> read ( disk, lba_in_disk, count_in_disk, temp ) == SUCCESS ) {
                    disk -> ops -> write ( disk, lba_in_disk, count_in_disk, buf );
                } else {
                    dev -> ops -> close( dev );
                    return E_UNAVAIL;
                }

            }
            if ( r_dev -> failed_disk != N ) {
                parity ( count_in_disk * BLOCK_SIZE, buf, temp, temp );
                char par_buf[ count_in_disk * BLOCK_SIZE ];
                if ( parity_disk -> ops -> read ( parity_disk, lba_in_disk, count_in_disk, par_buf) == SUCCESS ) {
                    parity ( count_in_disk * BLOCK_SIZE, temp, par_buf, par_buf );
                    parity_disk -> ops -> write( parity_disk, lba_in_disk, count_in_disk, par_buf );

                    buf += count_in_disk * BLOCK_SIZE;
                    first_blk += count_in_disk;
                    num_blks -= count_in_disk;

                    continue;
                } else {
                    dev -> ops -> close ( dev );
                    return E_UNAVAIL;
                }
            }
        }

        int result = disk -> ops -> read ( disk, lba_in_disk, count_in_disk, temp );
        if ( result == SUCCESS ) {
            disk -> ops -> write( disk, lba_in_disk, count_in_disk, buf );
            parity( count_in_disk * BLOCK_SIZE, buf, temp, temp );
            char par_buf[ count_in_disk * BLOCK_SIZE ];
            if ( parity_disk -> ops -> read ( parity_disk, lba_in_disk, count_in_disk, par_buf) == SUCCESS ) {
                parity ( count_in_disk * BLOCK_SIZE, temp, par_buf, par_buf );
                parity_disk -> ops -> write( parity_disk, lba_in_disk, count_in_disk, par_buf );

                buf += count_in_disk * BLOCK_SIZE;
                first_blk += count_in_disk;
                num_blks -= count_in_disk;

                continue;
            } else {
                r_dev -> mode = DEGRADED;
                r_dev -> failed_disk = r_dev -> ndisks;

                parity_disk -> ops -> close ( parity_disk );
                r_dev -> parity = NULL;
                continue;
            }
        } else {
            r_dev -> mode = DEGRADED;
            disk -> ops -> close( disk );
            r_dev -> disks[ disk_num ] = NULL;
            continue;
        }

    }
    return SUCCESS;
}

/* clean up, including: close all devices and free any data structures
 * you allocated in raid4_create.
 */
static void raid4_close(struct blkdev *dev)
{
    struct raid4_dev *r_dev = dev -> private;
    int i;
    for ( i = 0 ; i < r_dev -> ndisks + 1 ; i ++ ) {
        struct blkdev *disk = r_dev -> disks[ i ];
        if ( disk != NULL )
            disk -> ops -> close ( disk );
    }

    free ( r_dev );
    free ( dev );
}

struct blkdev_ops raid4_ops = {
    .num_blocks = raid_num_blocks,
    .read = raid4_read,
    .write = raid4_write,
    .close = raid4_close
};

/* Initialize a RAID 4 volume with stripe size 'unit', using
 * disks[N-1] as the parity drive. Do not write to the disks - assume
 * that they are properly initialized with correct parity. (warning -
 * some of the grading scripts may fail if you modify data on the
 * drives in this function)
 */
struct blkdev *raid4_create(int N, struct blkdev *disks[], int unit)
{
    // First, check if the sizes match.
    int disk_size = disks[ 0 ] -> ops -> num_blocks ( disks[ 0 ] );
    int i;
    for ( i = 0 ; i < N ; i ++ ) {
        if ( disk_size != disks[ i ] -> ops -> num_blocks (
                    disks[ i ] ) ) {
            printf( "Failed to create raid4 disk. Sizes are not equal.");
            return NULL;
        }
    }

    struct blkdev *dev = malloc ( sizeof( *dev ) );
    struct raid4_dev *r_dev = malloc ( sizeof( *r_dev ) );

    // Though the sisks point to all N disks, we should only use
    // the first N - 1 one, the last is parity.
    r_dev -> disks = disks;
    struct blkdev *parity = disks[ N - 1 ];
    r_dev -> parity = parity;

    r_dev -> ndisks = N - 1;
    r_dev -> nblks = ( disk_size / unit ) * unit;
    r_dev -> unit = unit;

    r_dev -> mode = NORMAL;
    r_dev -> failed_disk = -1;

    dev -> private = r_dev;
    dev -> ops = &raid4_ops;

    return dev;
}

/* replace failed device 'i' in a RAID 4. Note that we assume
 * the upper layer knows which device failed. You will need to
 * reconstruct content from data and parity before returning
 * from this call.
 */
int raid4_replace(struct blkdev *volume, int i, struct blkdev *newdisk)
{
    struct raid4_dev *r_dev = volume -> private;

    // If the size doesn't match, return E_SIZE error.
    if ( r_dev -> nblks > newdisk -> ops -> num_blocks( newdisk ) ) {
        return E_SIZE;
    }

    char buf[ ( r_dev -> nblks ) * BLOCK_SIZE ];

    get_data_for_disk( volume, i, 0, r_dev -> nblks, buf );

    newdisk -> ops -> write ( newdisk, 0, r_dev -> nblks, buf );

    struct blkdev *disk = r_dev -> disks[ i ];
    if ( disk != NULL ) {
        disk -> ops -> close ( disk );
    }
    r_dev -> disks[ i ] = newdisk;

    return SUCCESS;
}

