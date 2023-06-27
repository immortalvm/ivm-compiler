/* <dirent.h> includes <sys/dirent.h>, which is this file.  On a
   system which supports <dirent.h>, this file is overridden by
   dirent.h in the libc/sys/.../sys directory.  On a system which does
   not support <dirent.h>, we will get this file which uses #error to force
   an error.  */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __ivm64__
    //-- /*e Copied from <NEWLIB>/newlib/libc/sys/sysvi386/sys/dirent.h
    typedef struct _dirdesc {
        int     dd_fd;
        long    dd_loc;
        long    dd_size;
        char    *dd_buf;
        int     dd_len;
        long    dd_seek;
    } DIR;

    # define __dirfd(dp)    ((dp)->dd_fd)

    #include <sys/types.h>
    #include <limits.h>

    //-- Do not forget to modify opendir.c to make enough space
    //-- for this dirent struct
    struct dirent {
       long           d_ino;       /* Inode number */
       off_t          d_off;       /* Not an offset; see below */
       unsigned long  d_reclen;    /* Length of this record */
       //-- #ifdef _DIRENT_HAVE_D_TYPE
       unsigned char  d_type;      /* Type of file; not supported
       //                             by all filesystem types */
       //-- #endif
       char           d_name[PATH_MAX]; /* Null-terminated filename */
       //-- Specific ivmfs fields
       //-- unsigned char  d_ivmfs_type; /*ivmfs file type */
       //-- unsigned long  d_ivmfs_size; /*ivmfs file size in bytes*/
       //-- char           d_ivmfs_fullname[PATH_MAX]; /* Null-terminated full filename (including path) */
    };

    enum
    {
      DT_UNKNOWN = 0,
      DT_FIFO = 1,
      DT_CHR = 2,
      DT_DIR = 4,
      DT_BLK = 6,
      DT_REG = 8,
      DT_LNK = 10,
      DT_SOCK = 12,
      DT_WHT = 14
      # define DT_UNKNOWN     DT_UNKNOWN
      # define DT_FIFO        DT_FIFO
      # define DT_CHR         DT_CHR
      # define DT_DIR         DT_DIR
      # define DT_BLK         DT_BLK
      # define DT_REG         DT_REG
      # define DT_LNK         DT_LNK
      # define DT_WHT         DT_WHT
      # define DT_SOCK        DT_SOCK
    };
    /* Convert between stat structure types and directory types.  */
    # define IFTODT(mode)       (((mode) & 0170000) >> 12)
    # define DTTOIF(dirtype)    ((dirtype) << 12)

    //-- /*e This fragment is copied from <NEWLIB>/winsup/cygwin/include/sys/dirent.h*/
    //-- /*  To make libstdc++-v3 compile; as one of the tests done by libstdc++-v3/configure
    //--     enables HAVE_STRUCT_DIRENT_D_TYPE (see libstdc++-v3/configure:76323 when
    //--     'dirent' has a 'd_type' field */
    //-- /*  Also you can eliminate dirent.d_type */
    //-- #if __BSD_VISIBLE
    //-- #ifdef _DIRENT_HAVE_D_TYPE
    //-- /* File types for `d_type'.  */
    //-- enum
    //-- {
    //--   DT_UNKNOWN = 0,
    //-- # define DT_UNKNOWN     DT_UNKNOWN
    //--   DT_FIFO = 1,
    //-- # define DT_FIFO        DT_FIFO
    //--   DT_CHR = 2,
    //-- # define DT_CHR         DT_CHR
    //--   DT_DIR = 4,
    //-- # define DT_DIR         DT_DIR
    //--   DT_BLK = 6,
    //-- # define DT_BLK         DT_BLK
    //--   DT_REG = 8,
    //-- # define DT_REG         DT_REG
    //--   DT_LNK = 10,
    //-- # define DT_LNK         DT_LNK
    //--   DT_SOCK = 12,
    //-- # define DT_SOCK        DT_SOCK
    //--   DT_WHT = 14
    //-- # define DT_WHT         DT_WHT
    //-- };
    //--
    //-- /* Convert between stat structure types and directory types.  */
    //-- # define IFTODT(mode)        (((mode) & 0170000) >> 12)
    //-- # define DTTOIF(dirtype)    ((dirtype) << 12)
    //-- #endif /* _DIRENT_HAVE_D_TYPE */
    //-- #endif /* __BSD_VISIBLE */

#else
    #error "<dirent.h> not supported"
#endif


#ifdef __cplusplus
}
#endif
