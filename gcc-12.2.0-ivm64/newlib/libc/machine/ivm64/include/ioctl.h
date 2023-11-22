/* libc/sys/linux/sys/ioctl.h - ioctl prototype */

/* Written 2000 by Werner Almesberger */

/* Adapted to the ivm64 machine
   2023, by E. Gutierrez, S. Romero, O. Plata;
   University of Malaga, Spain */

#ifndef _INCLUDE_IOCTL_H
#define _INCLUDE_IOCTL_H

#ifndef __ivm64__
    #include <bits/ioctls.h>
    int ioctl(int fd,int request,...);
#else
    #include <sys/ioctl.h>
#endif

#endif
