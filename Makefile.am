AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = dvd_info dvd_eject

dvd_info_SOURCES = dvd_info.c dvd_drive.c dvd_device.c dvd_vmg_ifo.c dvd_track.c
dvd_info_CFLAGS = -std=gnu99
dvd_info_LDADD = -ldvdread -ljansson

dvd_eject_SOURCES = dvd_eject.c
dvd_eject_CFLAGS = -std=gnu99
dvd_eject_LDADD = -ldvdcss
