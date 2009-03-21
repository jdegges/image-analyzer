#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([image-analyzer], [dev], [jdegges@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
#AC_CANONICAL_TARGET
AC_CANONICAL_HOST
AC_CANONICAL_BUILD
AC_PROG_CC

# Checks for libraries.
AC_ARG_ENABLE([sdl],
    [AS_HELP_STRING([--enable-sdl],
        [use SDL to optionally display output @<:@default=check@:>@])],
    [],
    [enable_sdl=check])

AC_CHECK_LIB([freeimage], [main])
AC_CHECK_LIB([pthread], [main])
AS_IF([test "x$enable_sdl" != xno],
    [AC_HAVE_LIBRARY([-lSDL])])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h limits.h stdint.h stdlib.h string.h sys/ioctl.h sys/time.h unistd.h])
#AC_CHECK_HEADERS([FreeImage.h pthread.h SDL/SDL.h linux/videodev2.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_HEADER_TIME
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_SELECT_ARGTYPES
AC_CHECK_FUNCS([memset munmap pow select sqrt strcasecmp strerror strrchr strtoul])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
