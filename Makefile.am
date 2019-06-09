bin_PROGRAMS = dvd_info dvd_copy
man1_MANS = dvd_info.1 dvd_copy.1

dvd_info_SOURCES = dvd_info.c dvd_drive.c dvd_device.c dvd_vmg_ifo.c dvd_track.c dvd_cell.c dvd_video.c dvd_audio.c dvd_subtitles.c dvd_time.c dvd_json.c dvd_chapter.c dvd_xchap.c
dvd_info_CFLAGS = $(DVDREAD_CFLAGS)
dvd_info_LDADD = $(DVDREAD_LIBS)

dvd_copy_SOURCES = dvd_copy.c dvd_drive.c dvd_device.c dvd_vmg_ifo.c dvd_track.c dvd_cell.c dvd_vts.c dvd_vob.c dvd_audio.c dvd_subtitles.c dvd_time.c dvd_chapter.c
dvd_copy_CFLAGS = $(DVDREAD_CFLAGS)
dvd_copy_LDADD = $(DVDREAD_LIBS)

if LINUX_DRIVE_TOOLS

bin_PROGRAMS += dvd_drive_status
dvd_drive_status_SOURCES = dvd_drive_status.c

if LINUX_DRIVE_EJECT

bin_PROGRAMS += dvd_eject
dvd_eject_SOURCES = dvd_eject.c
dvd_eject_CFLAGS = $(DVDCSS_CFLAGS)
dvd_eject_LDADD = $(DVDCSS_LIBS)

endif

endif

if DVD_PLAYER
bin_PROGRAMS += dvd_player
dvd_player_SOURCES = dvd_player.c dvd_drive.c dvd_device.c dvd_vmg_ifo.c dvd_track.c dvd_cell.c dvd_vts.c dvd_audio.c dvd_subtitles.c dvd_time.c dvd_chapter.c dvd_video.c
dvd_player_CFLAGS = $(DVDREAD_CFLAGS) $(MPV_CFLAGS)
dvd_player_LDADD = $(DVDREAD_LIBS) $(MPV_LIBS)
endif

if DVD_TRIP
bin_PROGRAMS += dvd_trip
dvd_trip_SOURCES = dvd_trip.c dvd_drive.c dvd_device.c dvd_vmg_ifo.c dvd_track.c dvd_cell.c dvd_vts.c dvd_vob.c dvd_audio.c dvd_subtitles.c dvd_time.c dvd_chapter.c dvd_video.c
dvd_trip_CFLAGS = $(DVDREAD_CFLAGS) $(MPV_CFLAGS)
dvd_trip_LDADD = $(DVDREAD_LIBS) $(MPV_LIBS)
endif
