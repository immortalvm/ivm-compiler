#ifndef HAVE_OPENDIR

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)opendir.c	5.11 (Berkeley) 2/23/91";
#endif /* LIBC_SCCS and not lint */

#include <dirent.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/lock.h>

#ifdef __ivm64__
    /* Enough space for direct; keep a multiple of 512 */
    #define IVM_DDLEN0 (sizeof(struct dirent)*16)
    #define IVM_DDLEN  ((IVM_DDLEN0 + 512L -1) & (-512L)) 
#endif

static DIR *
_opendir_common(int fd)
{
	DIR *dirp;

	if ((dirp = (DIR *)malloc(sizeof(DIR))) == NULL) {
		close (fd);
		return NULL;
	}
    #ifndef __ivm64__
	/*
	 * If CLSIZE is an exact multiple of DIRBLKSIZ, use a CLSIZE
	 * buffer that it cluster boundary aligned.
	 * Hopefully this can be a big win someday by allowing page trades
	 * to user space to be done by getdirentries()
	 */
	dirp->dd_buf = malloc (512);
	dirp->dd_len = 512;
    #else
    /* Adapt this size to our dirent entry */
	dirp->dd_buf = malloc (IVM_DDLEN);
	dirp->dd_len = IVM_DDLEN;
	dirp->dd_size = 0;
    #endif

	if (dirp->dd_buf == NULL) {
		free (dirp);
		close (fd);
		return NULL;
	}
	dirp->dd_fd = fd;
	dirp->dd_loc = 0;
	dirp->dd_seek = 0;
	/*
	 * Set up seek point for rewinddir.
	 */

#ifdef HAVE_DD_LOCK
	/* if we have a locking mechanism, initialize it */
	__lock_init_recursive(dirp->dd_lock);
#endif

	return dirp;
}

DIR *
opendir(const char *name)
{
	int fd;

	if ((fd = open(name, O_RDONLY | O_DIRECTORY | O_CLOEXEC)) == -1)
		return (NULL);
	return (_opendir_common(fd));
}

DIR *
fdopendir(int fd)
{

	if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1)
		return (NULL);
	return (_opendir_common(fd));
}

#endif /* ! HAVE_OPENDIR */
