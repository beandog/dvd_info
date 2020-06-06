AC_INIT([dvd_info], [1.8], [https://github.com/beandog/dvd_info/issues], [], [http://dvds.beandog.org/])

dnl Generate config header to store defines like version info
AC_CONFIG_HEADERS([dvd_config.h])

dnl This is not a GNU package, so ignore required files / format
AM_INIT_AUTOMAKE([foreign])

dnl Check for C99 support
AC_PROG_CC_C99

dnl need math.h to do MBs calculations
AC_CHECK_HEADERS([math.h])

dnl Use pkg-config to check for libdvdread, libdvdcss
PKG_CHECK_MODULES([DVDREAD], [dvdread >= 4.2.1])

dnl The DVD drive tools are OS-specific
AC_CANONICAL_HOST
case "$host_os" in
	linux*)
		linux_drive_tools=yes
		PKG_CHECK_MODULES([DVDCSS], [libdvdcss > 1.4.1], [
			has_libdvdcss=yes
			linux_drive_eject=yes
		],[
			has_libdvdcss=no
			linux_drive_eject=no
		])
	;;
esac

dnl build dvd_drive_status, dvd_eject
AM_CONDITIONAL([LINUX_DRIVE_TOOLS], [test "x$linux_drive_tools" = xyes])
AM_CONDITIONAL([LINUX_DRIVE_EJECT], [test "x$linux_drive_eject" = xyes])

dnl Using libmpv for the player is optional, but enabled by default, builds dvd_player
AC_ARG_WITH([libmpv], [AS_HELP_STRING([--with-libmpv], [Build DVD player using libmpv])], [PKG_CHECK_MODULES([MPV], [mpv >= 1.101.0], [
	with_libmpv=yes
	AC_DEFINE(HAVE_LIBMPV, [], [libmpv])
])])
AM_CONDITIONAL([DVD_PLAYER], [test "x$with_libmpv" = xyes])
AM_CONDITIONAL([DVD_RIPPER], [test "x$with_libmpv" = xyes])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
