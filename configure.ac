AC_INIT([dvd_info], [1.18], [https://github.com/beandog/dvd_info/issues], [], [https://github.com/beandog/dvd_info])

dnl Generate config header to store defines like version info
AC_CONFIG_HEADERS([config.h])

dnl This is not a GNU package, so ignore required files / format
AM_INIT_AUTOMAKE([foreign])

dnl need math.h to do MBs calculations
AC_CHECK_HEADERS([math.h])

dnl Use pkg-config to check for libdvdread
PKG_CHECK_MODULES([DVDREAD], [dvdread >= 6.1.0])

dnl The DVD drive tools are OS-specific
AM_CONDITIONAL([DVD_DRIVE_STATUS], [false])
AC_CANONICAL_HOST
case "$host_os" in
	*linux*)
		dnl Need ioctl.h for checking drive status in programs
		AC_CHECK_HEADERS([sys/ioctl.h], [], [])
		AM_CONDITIONAL([DVD_DRIVE_STATUS], [true])
		;;
	*)
		;;
esac

dnl Using libmpv for the player is optional, builds dvd_player
AC_ARG_WITH([libmpv], [AS_HELP_STRING([--with-libmpv], [Enable mpv support to build dvd_player])], [with_libmpv=${withval}], [with_libmpv=no])

AS_IF([test "x$with_libmpv" != "xno"],
	[
		AC_DEFINE(HAVE_LIBMPV, [], [libmpv])
		PKG_CHECK_MODULES([MPV], [mpv])
	]
)

AM_CONDITIONAL([DVD_PLAYER], [test "x$with_libmpv" != "xno"])
AM_CONDITIONAL([DVD_RIPPER], [test "x$with_libmpv" != "xno"])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
