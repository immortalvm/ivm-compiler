#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#ifndef MAXPATHLEN
#define MAXPATHLEN PATH_MAX
#endif

char *
getwd (char *buf)
{
  char tmp[MAXPATHLEN];

  if (buf == NULL)
    {
      errno = EINVAL;
      return NULL;
    }

  if (getcwd (tmp, MAXPATHLEN) == NULL)
    return NULL;

  return strncpy (buf, tmp, MAXPATHLEN);
}
