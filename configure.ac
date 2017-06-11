AC_INIT([dvd_info], [1.0])

dnl This is not a GNU package, so ignore required files / format
AM_INIT_AUTOMAKE([foreign])

dnl Check for C99 support
AC_PROG_CC_C99

dnl Use pkg-config to check for libdvdread, libdvdcss
PKG_CHECK_MODULES([DVDREAD], [dvdread >= 4.2.1])
PKG_CHECK_MODULES([DVDCSS], [libdvdcss >= 1.2.13])

dnl The DVD drive tools are OS-specific
AC_CANONICAL_HOST
case "$host_os" in
  linux*)
    linux_drive_tools=yes
    ;;
  netbsd* | openbsd* | freebsd*)
    bsd_drive_tools=yes
    ;;
  *)
    dvd_drive_tools=no
    ;;
esac
AM_CONDITIONAL([LINUX_DRIVE_TOOLS], [test x$linux_drive_tools = xyes])
AM_CONDITIONAL([BSD_DRIVE_TOOLS], [test x$bsd_drive_tools = xyes])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
