# Process this file with autoconf to produce a configure script.

# Copyright 2003-2010 Brian Smith
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

AC_PREREQ([2.71])
AC_INIT([CMConvert],[1.9.6],[support@smittyware.com])
AC_CONFIG_SRCDIR([config.h.in])
AM_INIT_AUTOMAKE(foreign)
AC_CONFIG_HEADERS([config.h])
AC_REVISION($Revision: 1.3 $)

# Checks for programs.
AC_PROG_INSTALL
AC_PROG_CXX
AC_PROG_CC

# Checks for header files.
m4_warn([obsolete],
[The preprocessor macro `STDC_HEADERS' is obsolete.
  Except in unusual embedded environments, you can safely include all
  ISO C90 headers unconditionally.])dnl
# Autoupdate added the next two lines to ensure that your configure
# script's behavior did not change.  They are probably safe to remove.
AC_CHECK_INCLUDES_DEFAULT
AC_PROG_EGREP

m4_warn([obsolete],
[Update your code to rely only on HAVE_SYS_TIME_H,
then remove this warning and the obsolete code below it.
All current systems provide time.h; it need not be checked for.
Not all systems provide sys/time.h, but those that do, all allow
you to include it and time.h simultaneously.])dnl
AC_CHECK_HEADERS_ONCE([sys/time.h])
# Obsolete code to be removed.
if test $ac_cv_header_sys_time_h = yes; then
  AC_DEFINE([TIME_WITH_SYS_TIME],[1],[Define to 1 if you can safely include both <sys/time.h>
	     and <time.h>.  This macro is obsolete.])
fi
# End of obsolete code.

AC_CHECK_HEADERS([fcntl.h stddef.h locale.h zzip/lib.h])
AC_CREATE_STDINT_H(src/cmconvert-stdint.h)

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_BIGENDIAN

# Parameter to specify dir for expat
AC_ARG_WITH(expat-dir, [  --with-expat-dir=DIR   Specify directory where Expat is installed])
if test ! "X$with_expat_dir" = "X"; then
	CXXFLAGS="$CXXFLAGS -I${with_expat_dir}/include"
	LDFLAGS="$LDFLAGS -L${with_expat_dir}/lib"
fi

# Parameter to specify dir for zzip library
AC_ARG_WITH(zzip-dir, [  --with-zzip-dir=DIR   Specify directory where zziplib is installed])
if test ! "X$with_zzip_dir" = "X"; then
	CXXFLAGS="$CXXFLAGS -I${with_zzip_dir}/include"
	LDFLAGS="$LDFLAGS -L${with_zzip_dir}/lib"
fi

# Parameter to specify dir for zlib
AC_ARG_WITH(zlib-dir, [  --with-zlib-dir=DIR   Specify directory where zlib is installed])
if test ! "X$with_zlib_dir" = "X"; then
	CXXFLAGS="$CXXFLAGS -I${with_zlib_dir}/include"
	LDFLAGS="$LDFLAGS -L${with_zlib_dir}/lib"
fi

# Checks for library functions.
AC_CHECK_LIB(expat, XML_ParserCreate,,AC_MSG_ERROR(cannot find Expat library))
AC_CHECK_LIB(z, deflateEnd)
AC_CHECK_LIB(zzip, zzip_dir_open)
AC_CHECK_LIB(m, sin)
AC_CHECK_FUNCS([memset memcpy strchr setlocale])
AC_FUNC_STRFTIME

AC_CONFIG_FILES([Makefile src/Makefile man/Makefile])
AC_OUTPUT
