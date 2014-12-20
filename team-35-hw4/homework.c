/*
 * file:        homework.c
 * description: skeleton file for CS 5600 homework 3
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, updated April 2012
 * $Id: homework.c 452 2011-11-28 22:25:31Z pjd $
 */

#define FUSE_USE_VERSION 27

#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <fuse.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "cs5600fs.h"
#include "blkdev.h"

/*
 * disk access - the global variable 'disk' points to a blkdev
 * structure which has been initialized to access the image file.
 *
 * NOTE - blkdev access is in terms of 512-byte SECTORS, while the
 * file system uses 1024-byte BLOCKS. Remember to multiply everything
 * by 2.
 */

extern struct blkdev *disk;

// Variables that should be set up in init. These should contain
// all information of the super_block, fat, root_dir and so on.

#define NUM_ENTRY 16
#define MAX_ENTRY_NAME 45

struct cs5600fs_super *super_blk;
struct cs5600fs_entry *fat;
struct cs5600fs_dirent root;
struct cs5600fs_dirent *root_dir;
unsigned int fat_size;

int root_dir_start;
/* init - this is called once by the FUSE framework at startup.
 * This might be a good place to read in the super-block and set up
 * any global variables you need. You don't need to worry about the
 * argument or the return value.
 */
void *hw3_init(struct fuse_conn_info *conn)
{
    // Set up the super block from disk.
    super_blk = ( struct cs5600fs_super *) malloc( FS_BLOCK_SIZE );
    disk -> ops -> read ( disk, 0, 2, (void *)super_blk );

    // Set up the FAT from disk.
    fat_size = super_blk -> fat_len;
    fat = ( struct cs5600fs_entry *) malloc( fat_size * FS_BLOCK_SIZE );
    disk -> ops -> read( disk, 2, 2 * fat_size, (void *)fat );

    root = super_blk -> root_dirent;
    root_dir_start = root.start; //The location right after fat.

    root_dir = ( struct cs5600fs_dirent *) malloc( FS_BLOCK_SIZE );
    disk -> ops -> read( disk, root_dir_start * 2, 2, (void *) root_dir );

    return NULL;
}

/* find the next word starting at 's', delimited by characters
 * in the string 'delim', and store up to 'len' bytes into *buf
 * returns pointer to immediately after the word, or NULL if done.
 */
static char *strwrd(char *s, char *buf, size_t len, char *delim)
{
    s += strspn(s, delim);
    int n = strcspn(s, delim); /* count the span (spn) of bytes in */
    if (len - 1 < n) /* the complement (c) of *delim */
        n = len - 1;
    memcpy(buf, s, n);
    buf[n] = 0;
    s += n;
    return (*s == 0) ? NULL : s;
}

static void pathParser( char **argv, const char *path )
{
    char *line = ( char *) path;
    int argc;
    for (argc = 0; argc < NUM_ENTRY; argc++) {
        line = strwrd( line, argv[argc], MAX_ENTRY_NAME, "/" );
        if ( line == NULL )
            break;
    }
}

char **initCharStringArray( int m, int n )
{
    char **result = ( char **) malloc ( m * sizeof(char *) );
    int c;
    for ( c = 0; c < m; ++c ) {
        result[ c ] = malloc( n * sizeof( char ) );
        memset( result[ c ] , 0, n * sizeof( char ) );
    }
    return result;
}

void destroyCharStringArray( char **array, int m )
{
    int i;
    for ( i = 0; i < m ; i ++ ) {
        if ( array[ i ] != NULL ) {
            // free( array[ i ] );
        }
    }
    // free( array );
}

struct cs5600fs_dirent *pathTrans( char **argv, struct cs5600fs_dirent directory[ NUM_ENTRY ] )
{
    // Start from Root Directory.
    disk -> ops -> read( disk, root_dir_start * 2, 2, (void *) directory );
    int argc = 0;
    int valid_sign = 0;
    while ( 1 ) {
        if ( argv[ argc ][ 0 ] == 0 ) {
            break;
        }
        char *name = argv[ argc ++ ];
        // printf( "Current folder is: %s.\n", name );
        int i;
        for ( i = 0 ; i < NUM_ENTRY; i ++ ) {
            if ( directory[ i ].valid ) {
                // printf("Looking for %s, name is %s.\n", name, directory[ i ].name );
                if ( strcmp( name, directory[ i ].name ) == 0 ) {
                    valid_sign = 1;
                    if ( argv[ argc ][ 0 ] != 0 && directory[ i ].isDir ) {
                        disk -> ops -> read( disk, directory[ i ].start * 2, 2, (void *) directory );
                        break;
                    }
                    return &directory[ i ];
                }
            }
        }
        if ( !valid_sign ) {
            return NULL;
        }
    }
    return NULL;
}

void setAttr( const struct cs5600fs_dirent directory, struct stat *sb )
{
    sb -> st_dev = 0;
    sb -> st_ino = 0;
    sb -> st_rdev = 0;
    sb -> st_blocks = (directory.length + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE;
    sb -> st_blksize = FS_BLOCK_SIZE;

    sb -> st_nlink = 1;

    sb -> st_uid = directory.uid;
    sb -> st_gid = directory.gid;
    sb -> st_size = directory.length;

    sb -> st_mtime = directory.mtime;

    sb -> st_atime = directory.mtime;
    sb -> st_ctime = directory.mtime;

    sb -> st_mode = directory.mode | ( directory.isDir ? S_IFDIR : S_IFREG );
}

struct cs5600fs_dirent *findTargetDir( const char *path )
{
    if ( strcmp( path, "/" ) == 0 ) {
        return &root;
    }
    char **argv = initCharStringArray( NUM_ENTRY, MAX_ENTRY_NAME );
    pathParser( argv, path );

    struct cs5600fs_dirent *directory = ( struct cs5600fs_dirent *) malloc ( FS_BLOCK_SIZE );
    struct cs5600fs_dirent *dir = pathTrans( argv, directory );

    destroyCharStringArray( argv, NUM_ENTRY );

    return dir;
}

/* Note on path translation errors:
 * In addition to the method-specific errors listed below, almost
 * every method can return one of the following errors if it fails to
 * locate a file or directory corresponding to a specified path.
 *
 * ENOENT - a component of the path is not present.
 * ENOTDIR - an intermediate component of the path (e.g. 'b' in
 *           /a/b/c) is not a directory
 */

/* getattr - get file or directory attributes. For a description of
 *  the fields in 'struct stat', see 'man lstat'.
 *
 * Note - fields not provided in CS5600fs are:
 *    st_nlink - always set to 1
 *    st_atime, st_ctime - set to same value as st_mtime
 *
 * errors - path translation, ENOENT
 */
static int hw3_getattr(const char *path, struct stat *sb)
{
    struct cs5600fs_dirent *dir = findTargetDir( path );
    if ( dir == NULL ) {
        return -ENOENT;
    } else {
        // printf( "Path is %s\n", name );
        setAttr( *dir, sb);
        return SUCCESS;
    }
}

/* readdir - get directory contents.
 *
 * for each entry in the directory, invoke the 'filler' function,
 * which is passed as a function pointer, as follows:
 *     filler(buf, <name>, <statbuf>, 0)
 * where <statbuf> is a struct stat, just like in getattr.
 *
 * Errors - path resolution, ENOTDIR, ENOENT
 */
static int hw3_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi)
{
    struct cs5600fs_dirent *dir = findTargetDir( path );

    // If not found, return ENOENT.
    if ( dir == NULL ) {
        return -ENOENT;
    } else if ( dir -> isDir == 0 ) {
        return -ENOTDIR;     // Not a directory.
    } else {
        // struct cs5600fs_dirent directory[ NUM_ENTRY ];
        struct cs5600fs_dirent *directory = ( struct cs5600fs_dirent *) malloc ( FS_BLOCK_SIZE );
        disk -> ops -> read( disk, 2 * ( dir -> start), 2, (void *) directory );

        int i;
        for ( i = 0 ; i < NUM_ENTRY ; i ++ ) {
            if ( directory[ i ].valid ) {
                char *name;
                struct stat sb;
                memset(&sb, 0, sizeof(sb));
                // sb.st_mode = directory[ i ].mode | ( directory[ i ].isDir ? S_IFDIR : S_IFREG ); /* permissions | (isdir ? S_IFDIR : S_IFREG) */
                // sb.st_size = directory[ i ].length; /* obvious */
                // sb.st_atime = sb.st_ctime = sb.st_mtime = directory[ i ].mtime; /* modification time */
                //
                name = directory[ i ].name;
                setAttr( directory[ i ], &sb );
                //
                filler(buf, name, &sb, 0); /* invoke callback function */
            }
        }
        return SUCCESS;
    }

    /* Example code - you have to iterate over all the files in a
     * directory and invoke the 'filler' function for each.
     */
    // memset(&sb, 0, sizeof(sb));

    // for (;;) {
    //     sb.st_mode = 0; /* permissions | (isdir ? S_IFDIR : S_IFREG) */
    //     sb.st_size = 0; /* obvious */
    //     sb.st_atime = sb.st_ctime = sb.st_mtime = 0; /* modification time */
    //     name = "";
    //     filler(buf, name, &sb, 0); /* invoke callback function */
    // }
    // return 0;
}

void splitPath( const char *path, char *containingDir, char *base )
{
    char *last = strrchr( path, '/' ) + 1;

    strcpy( base, last );
    if ( last - path - 1 == 0 ) {
        strcpy( containingDir, "/" );
    } else {
        memcpy( containingDir, path, last - path - 1);
    }
}

// Find a free entry in FAT. If not found, return -1.
int findFreeEntryInFAT()
{
    int i;
    for ( i = 0 ; i < ( super_blk -> fat_len * FS_BLOCK_SIZE ) / 4 ; i ++ ) {
        struct cs5600fs_entry *entry = fat + i;
        if ( !entry -> inUse ) {
            return i;
        }
    }
    return -1;
}

// Set the attributes of a FAT entry.
void setFAT( int entryNum, uint32_t inUse, uint32_t eof, uint32_t next )
{
    struct cs5600fs_entry *entry = fat + entryNum;
    entry -> inUse = inUse;
    entry -> eof = eof;
    entry -> next = next;

    fat[ entryNum ] = *entry;
    disk -> ops -> write( disk, 2, 2 * fat_size, ( void *) fat );
    // disk -> ops -> read( disk, 2, 2 * fat_size, (void *)fat );

    // int start = entryNum / 128;
    // struct cs5600fs_entry entries[ 128 ];
    // disk -> ops -> read( disk, 2 + start, 1, (void *) entries );
    // entries[ entryNum % 128 ] = *entry;
    // disk -> ops -> write( disk, 2 + start, 1, (void *) entries );
}

// This one should set the entry to the passed in arguments entry
// in the containing folder of either the directory or the file.
//
// NOTE!!! The caller should gurantee that containing folder is
// valid.
void setEntry( const char *path, struct cs5600fs_dirent entry )
{
    char *containingDir = malloc( 45 );
    char *base = malloc( 20 );
    splitPath( path, containingDir, base );


    struct cs5600fs_dirent *dir = findTargetDir( containingDir );

    struct cs5600fs_dirent *directory = ( struct cs5600fs_dirent *) malloc ( FS_BLOCK_SIZE );

    disk -> ops -> read( disk, 2 * ( dir -> start ), 2, (void *) directory );

    int i;
    for ( i = 0 ; i < NUM_ENTRY ; i ++ ) {
        if ( directory[ i ].valid ) {
            if ( strcmp( base, directory[ i ].name ) == 0 ) {
                directory[ i ] = entry;
                break;
            }
        }
    }
    disk -> ops -> write( disk, 2 * ( dir -> start ), 2,
                          (void *) directory );
    // free( containingDir );
    // free( base );
}

// This function creates an entry in the containing folder for
// either the directory or the file. And sets the mode of it.
int addEntry( const char *path, mode_t mode, uint16_t isDir)
{
    // TODO. Free the allocated memory in a proper way.
    char *containingDir = malloc( 45 );
    char *base = malloc( 20 );
    splitPath( path, containingDir, base );

    // printf( "Base is :%s\n", base );
    // printf( "ContainingDir is :%s\n", containingDir );

    struct cs5600fs_dirent *dir = findTargetDir( containingDir );
    if ( dir == NULL ) {
        return -ENOENT;
    } else if ( dir -> isDir == 0 ) {
        return -ENOTDIR;
    } else {
        // Scan the whole directory to make sure the file/dir doesn't
        // exist now while looking for the first valid directory.
        // struct cs5600fs_dirent directory[ NUM_ENTRY ];
        struct cs5600fs_dirent *directory = ( struct cs5600fs_dirent *) malloc ( FS_BLOCK_SIZE );
        disk -> ops -> read( disk, 2 * ( dir -> start ), 2,
                             (void *) directory );

        int i;
        int validEntry = NUM_ENTRY;
        for ( i = 0 ; i < NUM_ENTRY ; i ++ ) {
            if ( directory[ i ].valid ) {
                if ( strcmp( base, directory[ i ].name ) == 0 ) {
                    return -EEXIST;
                }
            } else {
                validEntry = validEntry < i ? validEntry : i;
                break;
            }
        }
        // Find a free space in FAT.
        int freeEntryInFAT = findFreeEntryInFAT();

        // No available entry.
        if ( validEntry == NUM_ENTRY || freeEntryInFAT == -1 ) {
            return -ENOSPC;
        }

        setFAT( freeEntryInFAT, 1, 1, 0 );
        directory[ validEntry ].valid = 1;
        // directory[ validEntry ].name = ( char *)base;
        strcpy( directory[ validEntry ].name, base );
        directory[ validEntry ].mode = mode;
        directory[ validEntry ].isDir = isDir;
        directory[ validEntry ].start = freeEntryInFAT;
        directory[ validEntry ].length = 0;
        directory[ validEntry ].mtime = time( NULL );
        directory[ validEntry ].uid = getuid();
        directory[ validEntry ].gid = getgid();

        // if ( isDir ) {
        //     directory[ validEntry ].length = 4096;
        // }

        // Write back the directory.
        disk -> ops -> write( disk, 2 * ( dir -> start ), 2,
                              (void *) directory );
        return SUCCESS;
    }
}

/* create - create a new file with permissions (mode & 01777)
 *
 * Errors - path resolution, EEXIST
 *
 * If a file or directory of this name already exists, return -EEXIST.
 */
static int hw3_create(const char *path, mode_t mode,
                      struct fuse_file_info *fi)
{
    return addEntry( path, mode, 0 );

    // return -EOPNOTSUPP;
}

/* mkdir - create a directory with the given mode.
 * Errors - path resolution, EEXIST
 * Conditions for EEXIST are the same as for create.
 */
static int hw3_mkdir(const char *path, mode_t mode)
{
    return addEntry( path, mode, 1 );

    // return -EOPNOTSUPP;
}

/* unlink - delete a file
 *  Errors - path resolution, ENOENT, EISDIR
 */
static int hw3_unlink(const char *path)
{
    struct cs5600fs_dirent *dir = findTargetDir( path );

    // If not found, return ENOENT.
    if ( dir == NULL ) {
        return -ENOENT;
    } else if ( dir -> isDir == 1 ) {
        return -EISDIR;     // Is a directory.
    } else {
        // First clean the entries in FAT.
        int file_len = dir -> length;
        int i;
        int current;
        int next = dir -> start;
        for ( i = 0 ; i < file_len ; i += FS_BLOCK_SIZE ) {
            current = next;
            struct cs5600fs_entry *entry = fat + current;
            next = entry -> next;
            setFAT( current, 0, 1, 0 );

            // Set all directories valid to 0.
            // struct cs5600fs_dirent directory[ NUM_ENTRY ];
            struct cs5600fs_dirent *directory = ( struct cs5600fs_dirent *) malloc ( FS_BLOCK_SIZE );
            disk -> ops -> read( disk, current * 2, 2, ( void *) directory );
            int j;
            for ( j = 0 ; j < NUM_ENTRY ; j ++ ) {
                directory[ j ].valid = 0;
            }
            disk -> ops -> write( disk, current * 2, 2, ( void *) directory );
        }

        // Then clean the entry in containing folder.

        // deleteEntry( path );
        dir -> valid = 0;
        setEntry( path, *dir );

        return SUCCESS;
    }

    // return -EOPNOTSUPP;
}

/* rmdir - remove a directory
 *  Errors - path resolution, ENOENT, ENOTDIR, ENOTEMPTY
 */
static int hw3_rmdir(const char *path)
{
    struct cs5600fs_dirent *dir = findTargetDir( path );

    // If not found, return ENOENT.
    if ( dir == NULL ) {
        return -ENOENT;
    } else if ( dir -> isDir == 0 ) {
        return -ENOTDIR;     // Not a directory.
    } else {
        // First check if the directory is empty.
        // struct cs5600fs_dirent directory[ NUM_ENTRY ];
        struct cs5600fs_dirent *directory = ( struct cs5600fs_dirent *) malloc ( FS_BLOCK_SIZE );
        disk -> ops -> read( disk, 2 * ( dir -> start ), 2, ( void *) directory );

        int i;
        for ( i = 0 ; i < NUM_ENTRY ; i ++ ) {
            if ( directory[ i ].valid == 1 ) {
                return -ENOTEMPTY;
            }
        }
        // If so, set inUse to 0 in FAT.
        setFAT( dir -> start, 0, 1, 0 );

        // And set the entry to 0. Then write back to disk.

        // deleteEntry( path );
        dir -> valid = 0;
        setEntry( path, *dir );

        return SUCCESS;
    }

    // return -EOPNOTSUPP;
}

int isInSameDir( const char *src_path, const char *des_path )
{
    char *srcDir = malloc( 45 );
    char *srcName = malloc( 20 );
    splitPath( src_path, srcDir, srcName );

    char *desDir = malloc( 45 );
    char *desName = malloc( 20 );
    splitPath( des_path, desDir, desName );

    int result = strcmp( srcDir, desDir );

    // free( srcDir );
    // free( srcName );
    // free( desDir );
    // free( desName );

    return result;
}

/* rename - rename a file or directory
 * Errors - path resolution, ENOENT, EINVAL, EEXIST
 *
 * ENOENT - source does not exist
 * EEXIST - destination already exists
 * EINVAL - source and destination are not in the same directory
 *
 * Note that this is a simplified version of the UNIX rename
 * functionality - see 'man 2 rename' for full semantics. In
 * particular, the full version can move across directories, replace a
 * destination file, and replace an empty directory with a full one.
 */
static int hw3_rename(const char *src_path, const char *dst_path)
{
    struct cs5600fs_dirent *src_dir = findTargetDir( src_path );
    struct cs5600fs_dirent *des_dir = findTargetDir( dst_path );

    // Check if source is valid.
    if ( src_dir == NULL ) {
        return -ENOENT;
    }

    if ( des_dir != NULL ) {
        return -EEXIST;
    }

    if ( isInSameDir( src_path, dst_path ) != 0 ) {
        return -EINVAL;
    }

    char *dirName = malloc( 45 );
    char *entryName = malloc( 20 );
    splitPath( dst_path, dirName, entryName );

    strcpy( src_dir -> name, entryName );
    src_dir -> mtime = time( NULL );
    // src_dir -> name = entryName;
    setEntry( src_path, *src_dir );

    return SUCCESS;

    // return -EOPNOTSUPP;
}

/* chmod - change file permissions
 * utime - change access and modification times
 *         (for definition of 'struct utimebuf', see 'man utime')
 *
 * Errors - path resolution, ENOENT.
 */
static int hw3_chmod(const char *path, mode_t mode)
{
    struct cs5600fs_dirent *dir = findTargetDir( path );

    // If not found, return ENOENT.
    if ( dir == NULL ) {
        return -ENOENT;
    } else {
        dir -> mode = mode;
        setEntry( path, *dir );

        return SUCCESS;
    }

    // return -EOPNOTSUPP;
}
int hw3_utime(const char *path, struct utimbuf *ut)
{
    struct cs5600fs_dirent *dir = findTargetDir( path );

    // If not found, return ENOENT.
    if ( dir == NULL ) {
        return -ENOENT;
    } else {
        dir -> mtime = ut -> modtime;
        setEntry( path, *dir );

        return SUCCESS;
    }

    // return -EOPNOTSUPP;
}

/* truncate - truncate file to exactly 'len' bytes
 * Errors - path resolution, ENOENT, EISDIR, EINVAL
 *    return EINVAL if len > 0.
 */
static int hw3_truncate(const char *path, off_t len)
{
    /* you can cheat by only implementing this for the case of len==0,
     * and an error otherwise.
     */
    if (len != 0) {
        return -EINVAL;     /* invalid argument */
    }

    struct cs5600fs_dirent *dir = findTargetDir( path );
    if ( dir == NULL ) {
        return -ENOENT;
    } else if ( dir -> isDir == 1 ) {
        return -EISDIR;
    } else {
        int ori_len = dir -> length;

        // Set the length and modification time of the directory in
        // containing folder.
        dir -> length = 0;
        dir -> mtime = time( NULL );
        setEntry( path, *dir );

        // Clean the FAT entries. Set them to 0 in inUse.
        int next = dir -> start;
        int start;
        while ( ori_len > 0 ) {
            start = next;
            struct cs5600fs_entry *entry = fat + start;
            next = entry -> next;
            setFAT( start, 0, 1, 0 );
            ori_len -= FS_BLOCK_SIZE;
        }
        setFAT( dir -> start, 1, 1, 0 );
        return SUCCESS;
    }


    // return -EOPNOTSUPP;
}

/* read - read data from an open file.
 * should return exactly the number of bytes requested, except:
 *   - if offset >= len, return 0
 *   - on error, return <0
 * Errors - path resolution, ENOENT, EISDIR
 */
static int hw3_read(const char *path, char *buf, size_t len, off_t offset,
                    struct fuse_file_info *fi)
{
    struct cs5600fs_dirent *dir = findTargetDir( path );
    // If not found, return ENOENT.
    if ( dir == NULL ) {
        return -ENOENT;
    } else if ( dir -> isDir == 1 ) {
        return -EISDIR;     // Is a directory.
    } else {
        size_t file_len = dir -> length;
        if ( offset >= file_len ) {
            return 0;
        } else {
            size_t start_block_num = offset / FS_BLOCK_SIZE;
            offset = offset % FS_BLOCK_SIZE;
            int start_block_location = dir -> start;
            // Locate the block that offset is in.
            struct cs5600fs_entry *entry = fat + start_block_location;
            int i;
            for ( i = 0 ; i < start_block_num ; i ++ ) {
                if ( entry -> inUse == 0 ) {
                    return 0;
                } else {
                    start_block_location = entry -> next;
                    entry = fat + ( entry -> next );
                }
            }
            // Now, the entry should point to the fat entry that
            // offset resides in. start_block_location points to
            // the start block number in disk. ( as always,
            // multiply 2 ).
            int already_read = 0;
            while ( len ) {
                if ( !entry -> inUse ) {
                    return already_read;
                }
                int to_read = 0;
                if ( entry -> eof ) {
                    to_read = file_len % FS_BLOCK_SIZE;
                    if ( len > to_read ) {
                        len = to_read;
                    }
                } else {
                    to_read = FS_BLOCK_SIZE - offset;
                }

                if ( len < to_read ) {
                    to_read = len;
                }
                char temp[ FS_BLOCK_SIZE ];
                int result = disk -> ops -> read( disk, start_block_location * 2, 2, temp );
                if ( result != SUCCESS ) {
                    return result;
                }
                memcpy( buf, temp + offset, to_read );

                if ( offset + to_read == FS_BLOCK_SIZE ) {
                    start_block_location = entry -> next;
                    entry = fat + ( entry -> next );
                }
                offset = ( offset + to_read ) % FS_BLOCK_SIZE;
                buf += to_read;
                already_read += to_read;
                len -= to_read;
            }
            return already_read;
        }
    }
}

/* write - write data to a file
 * It should return exactly the number of bytes requested, except on
 * error.
 * Errors - path resolution, ENOENT, EISDIR
 *  return EINVAL if 'offset' is greater than current file length.
 */
static int hw3_write(const char *path, const char *buf, size_t len,
                     off_t offset, struct fuse_file_info *fi)
{
    struct cs5600fs_dirent *dir = findTargetDir( path );
    // If not found, return ENOENT.
    if ( dir == NULL ) {
        return -ENOENT;
    } else if ( dir -> isDir == 1 ) {
        return -EISDIR;     // Is a directory.
    } else {
        // Always remember to set dir -> length and FAT entry of eof
        // and next.
        size_t file_len = dir -> length;
        if ( offset > file_len ) {
            return -EINVAL;
        } else {
            off_t origin_offset = offset;
            size_t start_block_num = offset / FS_BLOCK_SIZE;
            offset = offset % FS_BLOCK_SIZE;
            int start_block_location = dir -> start;
            char *name = malloc( 25 );
            strcpy( name, dir -> name );

            // Locate the block that offset is in.
            struct cs5600fs_entry *entry = fat + start_block_location;
            int i;
            for ( i = 0 ; i < start_block_num ; i ++ ) {
                if ( entry -> inUse == 0 ) {
                    return 0;
                } else {
                    if ( entry -> next == 0 ) {
                        if ( offset != 0 ) {
                            return 0;
                        }
                        int freeFAT = findFreeEntryInFAT();
                        setFAT( start_block_location, 1, 0, freeFAT);
                        setFAT( freeFAT, 1, 1, 0);
                    }
                    start_block_location = entry -> next;
                    entry = fat + ( entry -> next );
                }
            }

            // Now, the entry should point to the fat entry that
            // offset resides in. start_block_location points to
            // the start block number in disk. ( as always,
            // multiply 2 ).
            int already_written = 0;
            while ( len ) {
                if ( !entry -> inUse ) {
                    return already_written;
                }
                int to_write = FS_BLOCK_SIZE - offset;
                if ( len < to_write ) {
                    to_write = len;
                }

                char *temp = ( char *) malloc( FS_BLOCK_SIZE );
                int result = disk -> ops -> read ( disk, start_block_location * 2, 2, temp );
                if ( result != SUCCESS ) {
                    return result;
                }

                memcpy( temp + offset, buf, to_write );
                disk -> ops -> write( disk, start_block_location * 2, 2, temp );

                if ( offset + to_write == FS_BLOCK_SIZE ) {
                    if ( len - to_write > 0 ) {
                        if ( entry -> eof == 0 && entry -> next != 0 ) {
                            start_block_location = entry -> next;
                            entry = fat + start_block_location;
                        } else {
                            int freeFAT = findFreeEntryInFAT();
                            setFAT( start_block_location, 1, 0, freeFAT );
                            setFAT( freeFAT, 1, 1, 0 );
                            start_block_location = freeFAT;
                            entry = fat + start_block_location;
                        }
                    } else {
                        setFAT( start_block_location, 1, 1, 0 );
                    }
                }
                offset = ( offset + to_write ) % FS_BLOCK_SIZE;
                buf += to_write;
                already_written += to_write;
                len -= to_write;
            }
            strcpy( dir -> name, name );
            if ( origin_offset + already_written > dir -> length )
                dir -> length = origin_offset + already_written;
            // dir -> length += already_written;
            setEntry( path, *dir );
            return already_written;
        }
    }

    // return -EOPNOTSUPP;
}

int calculateFreeBlocks()
{
    int result = 0;
    int i;
    for ( i = 0 ; i < ( super_blk -> fat_len * FS_BLOCK_SIZE ) / 4 ; i ++ ) {
        struct cs5600fs_entry *entry = fat + i;
        if ( !entry -> inUse ) {
            result += 1;
        }
    }
    return result;
}

/* statfs - get file system statistics
 * see 'man 2 statfs' for description of 'struct statvfs'.
 * Errors - none. Needs to work.
 */
static int hw3_statfs(const char *path, struct statvfs *st)
{
    /* needs to return the following fields (set others to zero):
     *   f_bsize = BLOCK_SIZE
     *   f_blocks = total image - (superblock + FAT)
     *   f_bfree = f_blocks - blocks used
     *   f_bavail = f_bfree
     *   f_namelen = <whatever your max namelength is>
     *
     * it's OK to calculate this dynamically on the rare occasions
     * when this function is called.
     */
    st -> f_bsize = FS_BLOCK_SIZE;
    st -> f_blocks = ( super_blk -> fs_size ) - root.start;

    int numFreeBlocks = calculateFreeBlocks();
    st -> f_bfree = numFreeBlocks;
    st -> f_bavail = numFreeBlocks;
    st -> f_namemax = 43;

    st -> f_files = 0;
    st -> f_ffree = 0;
    st -> f_fsid = 0;
    st -> f_frsize = 0;
    return SUCCESS;
}

/* operations vector. Please don't rename it, as the skeleton code in
 * misc.c assumes it is named 'hw3_ops'.
 */
struct fuse_operations hw3_ops = {
    .init = hw3_init,
    .getattr = hw3_getattr,
    .readdir = hw3_readdir,
    .create = hw3_create,
    .mkdir = hw3_mkdir,
    .unlink = hw3_unlink,
    .rmdir = hw3_rmdir,
    .rename = hw3_rename,
    .chmod = hw3_chmod,
    .utime = hw3_utime,
    .truncate = hw3_truncate,
    .read = hw3_read,
    .write = hw3_write,
    .statfs = hw3_statfs,
};

