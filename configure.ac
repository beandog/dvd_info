AC_INIT([dvd_info], [1.0], , [https://github.com/beandog/dvd_info])

dnl This is not a GNU package, so ignore required files / format
AM_INIT_AUTOMAKE([foreign])

AC_PROG_LN_S

dnl Check for C99 support
AC_PROG_CC_C99

dnl Use pkg-config to check for libdvdread, libdvdcss
PKG_CHECK_MODULES([DVDREAD], [dvdread >= 4.2.1])
PKG_CHECK_MODULES([DVDCSS], [libdvdcss >= 1.2.13])

dnl The dvd_drive_status and dvd_eject programs rely on Linux kernel internals,
dnl so they are ignored on other operating systems.
AC_CANONICAL_HOST
case "$host_os" in
  linux*)
    dvd_drive_tools=yes
    ;;
  *)
    dvd_drive_tools=no
    ;;
esac
AM_CONDITIONAL([DVD_DRIVE_TOOLS], [test x$dvd_drive_tools = xyes])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
