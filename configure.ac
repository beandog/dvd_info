AC_INIT([dvd_info], [1.12], [https://github.com/beandog/dvd_info/issues], [], [http://dvds.beandog.org/])

dnl Generate config header to store defines like version info
AC_CONFIG_HEADERS([config.h])

dnl This is not a GNU package, so ignore required files / format
AM_INIT_AUTOMAKE([foreign])

dnl Check for C99 support
AC_PROG_CC_C99

dnl need math.h to do MBs calculations
AC_CHECK_HEADERS([math.h])

dnl Use pkg-config to check for libdvdread, libdvdcss
PKG_CHECK_MODULES([DVDREAD], [dvdread >= 4.2.1])

dnl The DVD drive tools are OS-specific
AM_CONDITIONAL([DVD_DRIVE_STATUS], [false])
AM_CONDITIONAL([DVD_EJECT], [false])
AC_CANONICAL_HOST
case "$host_os" in
	*linux*)
		AM_CONDITIONAL([DVD_DRIVE_STATUS], [true])
		PKG_CHECK_MODULES([DVDCSS], [libdvdcss > 1.4.1], [AM_CONDITIONAL([DVD_EJECT], [true])], [AM_CONDITIONAL([DVD_EJECT], [false])])
		;;
	*)
		;;
esac

dnl Need ioctl.h for checking drive status in programs
AC_CHECK_HEADERS([sys/ioctl.h], [], [])

dnl Using libmpv for the player is optional, but enabled by default, builds dvd_player
AC_ARG_WITH([libmpv], [AS_HELP_STRING([--with-libmpv], [Build DVD player using libmpv])], [PKG_CHECK_MODULES([MPV], [mpv >= 1.101.0], [
	with_libmpv=yes
	AC_DEFINE(HAVE_LIBMPV, [], [libmpv])
])])
AM_CONDITIONAL([DVD_PLAYER], [test "x$with_libmpv" = xyes])
AM_CONDITIONAL([DVD_RIPPER], [test "x$with_libmpv" = xyes])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
