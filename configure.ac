AC_INIT([dvd_info], [1.3])

dnl This is not a GNU package, so ignore required files / format
AM_INIT_AUTOMAKE([foreign])

dnl Check for C99 support
AC_PROG_CC_C99

dnl Use pkg-config to check for libdvdread, libdvdcss
PKG_CHECK_MODULES([DVDREAD], [dvdread >= 4.2.1])
PKG_CHECK_MODULES([DVDCSS], [libdvdcss >= 1.4.1])

dnl Using libmpv for the ripper and player is optional, but enabled by default
AC_ARG_ENABLE([libmpv], AS_HELP_STRING([--disable-libmpv], [Disable libmpv support for player and ripper]))
AS_IF([test "x$enable_libmpv" != "xno"], [PKG_CHECK_MODULES([MPV], [mpv >= 1.25.0])])
AM_CONDITIONAL([DVD_MPV_SUPPORT], [test x$enable_libmpv != xno])

dnl The DVD drive tools are OS-specific
AC_CANONICAL_HOST
case "$host_os" in
  linux*)
    linux_drive_tools=yes
    ;;
  netbsd* | openbsd* | freebsd*)
    bsd_drive_status_tool=yes
    bsd_eject_tool=yes
    ;;
  dragonfly*)
    bsd_eject_tool=yes
    ;;
  *)
    dvd_drive_tools=no
    ;;
esac
AM_CONDITIONAL([LINUX_DRIVE_TOOLS], [test x$linux_drive_tools = xyes])
AM_CONDITIONAL([BSD_DRIVE_STATUS_TOOL], [test x$bsd_drive_status_tool = xyes])
AM_CONDITIONAL([BSD_EJECT_TOOL], [test x$bsd_eject_tool = xyes])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
