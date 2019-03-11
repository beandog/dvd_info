AC_INIT([dvd_info], [1.3], [https://github.com/beandog/dvd_info/issues], [], [http://dvds.beandog.org/])

dnl This is not a GNU package, so ignore required files / format
AM_INIT_AUTOMAKE([foreign])

dnl Check for C99 support
AC_PROG_CC_C99

dnl Use pkg-config to check for libdvdread, libdvdcss
PKG_CHECK_MODULES([DVDREAD], [dvdread >= 4.2.1])

PKG_CHECK_MODULES([DVDCSS], [libdvdcss > 1.4.1], [
	has_libdvdcss=yes
dnl	enable_eject=yes
], [has_libmpv=no])

AC_ARG_ENABLE([eject], AS_HELP_STRING([--enable-eject], [build DVD eject tool, requires libdvdcss],), [PKG_CHECK_MODULES([DVDCSS], [libdvdcss >= 1.4.1], [enable_eject=yes])],[enable_eject=no])

PKG_CHECK_MODULES([MPV], [mpv >= 1.25.0], [
	has_libmpv=yes
dnl	enable_player=yes
dnl	enable_ripper=yes
], [has_libmpv=no])
AC_ARG_ENABLE([player], [AS_HELP_STRING([--enable-player], [build DVD player, requires libmpv])], [AS_IF([test "x$has_libmpv" = xyes], [enable_player=yes], [AC_MSG_ERROR([DVD player requires libmpv support])])])
AC_ARG_ENABLE([ripper], [AS_HELP_STRING([--enable-ripper], [build DVD ripper, requires libmpv, audio and video codecs])], [AS_IF([test "x$has_libmpv" = xyes], [enable_ripper=yes], [AC_MSG_ERROR([DVD ripper requires libmpv support])])])

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

AM_CONDITIONAL([LINUX_DRIVE_TOOLS], [test "x$linux_drive_tools" = xyes])
AM_CONDITIONAL([BSD_DRIVE_STATUS_TOOL], [test "x$bsd_drive_status_tool" = xyes])
AM_CONDITIONAL([BSD_EJECT_TOOL], [test "x$bsd_eject_tool" = xyes])
AM_CONDITIONAL([DVD_PLAYER], [test "x$enable_player" = xyes])
AM_CONDITIONAL([DVD_RIPPER], [test "x$enable_ripper" = xyes])

AC_CONFIG_HEADERS([config.h])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
