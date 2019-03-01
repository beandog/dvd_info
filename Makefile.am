bin_PROGRAMS = dvd_info dvd_copy dvd_player dvd_trip
man1_MANS = docs/dvd_info.1

dvd_info_SOURCES = dvd_info.c dvd_drive.c dvd_device.c dvd_vmg_ifo.c dvd_track.c dvd_cell.c dvd_video.c dvd_audio.c dvd_subtitles.c dvd_time.c dvd_json.c dvd_chapter.c dvd_ogm.c
dvd_info_CFLAGS = $(DVDREAD_CFLAGS) $(DVDCSS_CFLAGS)
dvd_info_LDADD = $(DVDREAD_LIBS) $(DVDCSS_LIBS)

dvd_copy_SOURCES = dvd_copy.c dvd_drive.c dvd_device.c dvd_vmg_ifo.c dvd_track.c dvd_cell.c dvd_vts.c dvd_vob.c dvd_audio.c dvd_subtitles.c dvd_time.c dvd_chapter.c
dvd_copy_CFLAGS = $(DVDREAD_CFLAGS) $(DVDCSS_CFLAGS)
dvd_copy_LDADD = $(DVDREAD_LIBS) $(DVDCSS_LIBS)

dvd_player_SOURCES = dvd_player.c dvd_drive.c dvd_device.c dvd_vmg_ifo.c dvd_track.c dvd_cell.c dvd_vts.c dvd_vob.c dvd_audio.c dvd_subtitles.c dvd_time.c dvd_chapter.c
dvd_player_CFLAGS = $(DVDREAD_CFLAGS) $(DVDCSS_CFLAGS) $(MPV_CFLAGS)
dvd_player_LDADD = $(DVDREAD_LIBS) $(DVDCSS_LIBS) $(MPV_LIBS)

dvd_trip_SOURCES = dvd_trip.c dvd_drive.c dvd_device.c dvd_vmg_ifo.c dvd_track.c dvd_cell.c dvd_vts.c dvd_vob.c dvd_audio.c dvd_subtitles.c dvd_time.c dvd_chapter.c
dvd_trip_CFLAGS = $(DVDREAD_CFLAGS) $(DVDCSS_CFLAGS) $(MPV_CFLAGS)
dvd_trip_LDADD = $(DVDREAD_LIBS) $(DVDCSS_LIBS) $(MPV_LIBS)

if LINUX_DRIVE_TOOLS
bin_PROGRAMS += dvd_drive_status dvd_eject

dvd_drive_status_SOURCES = dvd_drive_status.c

dvd_eject_SOURCES = dvd_eject.c
dvd_eject_CFLAGS = $(DVDCSS_CFLAGS)
dvd_eject_LDADD = $(DVDCSS_LIBS)

man1_MANS += docs/linux/dvd_eject.1 docs/linux/dvd_drive_status.1
endif

if BSD_DRIVE_STATUS_TOOL
bin_PROGRAMS += dvd_drive_status

dvd_drive_status_SOURCES = bsd_drive_status.c
dvd_drive_status_CFLAGS = $(DVDREAD_CFLAGS)
dvd_drive_status_LDADD = $(DVDREAD_LIBS)

man1_MANS += docs/bsd/dvd_drive_status.1
endif

if BSD_EJECT_TOOL
bin_PROGRAMS += dvd_eject

dvd_eject_SOURCES = bsd_eject.c

man1_MANS += docs/bsd/dvd_eject.1
endif
