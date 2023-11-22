/* libc/sys/linux/sys/ioctl.h - ioctl prototype */

/* Written 2000 by Werner Almesberger */

/* Adapted to the ivm64 machine
   2023, by E. Gutierrez, S. Romero, O. Plata;
   University of Malaga, Spain */

#ifndef _SYS_IOCTL_H
#define _SYS_IOCTL_H

#ifdef __ivm64__
    #include <sys/ioctls.h>

    #ifdef __cplusplus
    extern "C" {
    #endif
        int ioctl(int fd, unsigned long request,...);
    #ifdef __cplusplus
    }
    #endif

#endif

#endif

