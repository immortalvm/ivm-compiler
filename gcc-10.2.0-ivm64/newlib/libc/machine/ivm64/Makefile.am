## Process this file with automake to generate Makefile.in

#AUTOMAKE_OPTIONS = cygnus
AUTOMAKE_OPTIONS =

INCLUDES = $(NEWLIB_CFLAGS) $(CROSS_CFLAGS) $(TARGET_CFLAGS)

AM_CCASFLAGS = $(INCLUDES)

noinst_LIBRARIES = lib.a

lib_a_SOURCES = setjmp.c alloca.c string/memccpy.c  string/memchr.c  string/memcmp.c  string/memcpy.c \
				 string/memmove.c  string/mempcpy.c  string/memrchr.c  string/memset.c  string/rawmemchr.c  string/stpncpy.c \
				 string/stpcpy.c  string/strchr.c  string/strcmp.c  string/strcpy.c  string/strlen.c  string/strncmp.c  string/strncpy.c
lib_a_CCASFLAGS=$(AM_CCASFLAGS)
lib_a_CFLAGS=$(AM_CFLAGS)

ACLOCAL_AMFLAGS = -I ../../.. -I ../../../..
CONFIG_STATUS_DEPENDENCIES = $(newlib_basedir)/configure.host
