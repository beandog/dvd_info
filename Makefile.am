bin_PROGRAMS = dvd_info
man1_MANS = dvd_info.1
dvd_info_SOURCES = dvd_info.c dvd_open.c dvd_drive.c dvd_vmg_ifo.c dvd_track.c dvd_cell.c dvd_vts.c dvd_video.c dvd_audio.c dvd_subtitles.c dvd_time.c dvd_json.c dvd_chapter.c dvd_xchap.c dvd_init.c
dvd_info_CFLAGS = $(DVDREAD_CFLAGS)
dvd_info_LDADD = -lm $(DVDREAD_LIBS)

bin_PROGRAMS += dvd_copy
man1_MANS += dvd_copy.1
dvd_copy_SOURCES = dvd_copy.c dvd_drive.c dvd_open.c dvd_vmg_ifo.c dvd_track.c dvd_cell.c dvd_vts.c dvd_vob.c dvd_audio.c dvd_subtitles.c dvd_time.c dvd_chapter.c
dvd_copy_CFLAGS = $(DVDREAD_CFLAGS)
dvd_copy_LDADD = -lm $(DVDREAD_LIBS)

if DVD_BACKUP
bin_PROGRAMS += dvd_backup
man1_MANS += dvd_backup.1
dvd_backup_SOURCES = dvd_backup.c dvd_drive.c dvd_open.c dvd_vmg_ifo.c dvd_vts.c dvd_vob.c
dvd_backup_CFLAGS = $(DVDREAD_CFLAGS)
dvd_backup_LDADD = -lm $(DVDREAD_LIBS)
endif

bin_PROGRAMS += dvd_debug
dvd_debug_SOURCES = dvd_debug.c
dvd_debug_CFLAGS = $(DVDREAD_CFLAGS)
dvd_debug_LDADD = $(DVDREAD_LIBS)

if DVD_DRIVE_STATUS
bin_PROGRAMS += dvd_drive_status
man1_MANS += dvd_drive_status.1
dvd_drive_status_SOURCES = dvd_drive_status.c
endif

if DVD_PLAYER
bin_PROGRAMS += dvd_player
man1_MANS += dvd_player.1
dvd_player_SOURCES = dvd_player.c dvd_drive.c dvd_open.c dvd_vmg_ifo.c dvd_track.c dvd_cell.c dvd_vts.c dvd_audio.c dvd_subtitles.c dvd_time.c dvd_chapter.c dvd_video.c
dvd_player_CFLAGS = $(DVDREAD_CFLAGS) $(MPV_CFLAGS)
dvd_player_LDADD = -lm $(DVDREAD_LIBS) $(MPV_LIBS)
endif

if DVD_RIPPER
bin_PROGRAMS += dvd_rip
man1_MANS += dvd_rip.1
dvd_rip_SOURCES = dvd_rip.c dvd_drive.c dvd_open.c dvd_vmg_ifo.c dvd_track.c dvd_cell.c dvd_vts.c dvd_vob.c dvd_audio.c dvd_subtitles.c dvd_time.c dvd_chapter.c dvd_video.c
dvd_rip_CFLAGS = $(DVDREAD_CFLAGS) $(MPV_CFLAGS)
dvd_rip_LDADD = -lm $(DVDREAD_LIBS) $(MPV_LIBS)
endif
