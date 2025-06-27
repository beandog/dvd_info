# Required - cmake complains about 3.1 being deprecated, so bump to 3.10
cmake_minimum_required(VERSION 3.10)
# Yay! :D
set(PROJECT_NAME "dvd_info")
# Can use a macro in the programs if config.h is included
set(PROJECT_VERSION "1.18")
configure_file("config.h.in" "config.h")

# Display what compiler is doing
set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "ON")

# Don't check for c++ compiler
project(dvd_info LANGUAGES C)

# Use pkg-config
find_package(PkgConfig REQUIRED)
# libdvdread 6.1.0 changed how files are accessed
pkg_check_modules(DVDREAD REQUIRED dvdread>=6.1.0)
# libmpv doesn't have a specific version needed
pkg_check_modules(MPV mpv)

# Build dvd_info, requires math library as well (-lm)
add_executable(dvd_info dvd_info.c dvd_open.c dvd_drive.c dvd_vmg_ifo.c dvd_track.c dvd_cell.c dvd_vts.c dvd_video.c dvd_audio.c dvd_subtitles.c dvd_time.c dvd_json.c dvd_chapter.c dvd_xchap.c dvd_init.c)
target_link_libraries(dvd_info PRIVATE dvdread)
target_link_libraries(dvd_info PRIVATE m)
INSTALL(FILES dvd_info PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE DESTINATION bin)
INSTALL(FILES dvd_info.1 DESTINATION man/man1)

# Build dvd_copy
add_executable(dvd_copy dvd_copy.c dvd_drive.c dvd_open.c dvd_vmg_ifo.c dvd_track.c dvd_cell.c dvd_vts.c dvd_vob.c dvd_audio.c dvd_subtitles.c dvd_time.c dvd_chapter.c)
target_link_libraries(dvd_copy PRIVATE dvdread)
target_link_libraries(dvd_copy PRIVATE m)
INSTALL(FILES dvd_copy PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE DESTINATION bin)
INSTALL(FILES dvd_copy.1 DESTINATION man/man1)

# Build dvd_backup
add_executable(dvd_backup dvd_backup.c dvd_drive.c dvd_open.c dvd_vmg_ifo.c dvd_vts.c dvd_vob.c)
target_link_libraries(dvd_backup PRIVATE dvdread)
target_link_libraries(dvd_backup PRIVATE m)
INSTALL(FILES dvd_backup PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE DESTINATION bin)
INSTALL(FILES dvd_backup.1 DESTINATION man/man1)

# Build dvd_debug
add_executable(dvd_debug dvd_debug.c)
target_link_libraries(dvd_debug PRIVATE dvdread)
INSTALL(FILES dvd_debug PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE DESTINATION bin)
# dvd_debug doesn't have a man page
# INSTALL(FILES dvd_debug.1 DESTINATION man/man1)

# dvd_rip is a shell script, so just copy it over
INSTALL(FILES dvd_rip PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE DESTINATION bin)
INSTALL(FILES dvd_rip.1 DESTINATION man/man1)

# Build dvd_drive_status, which is Linux only
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	add_executable(dvd_drive_status dvd_drive_status.c)
	INSTALL(FILES dvd_drive_status PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE DESTINATION bin)
	INSTALL(FILES dvd_drive_status.1 DESTINATION man/man1)
endif()

# Documentation
INSTALL(FILES README.md LICENSE PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ DESTINATION share/doc/${PROJECT_NAME})

# If libmpv was found by pkgconfig, then build dvd_player and dvd_trip
if(MPV_FOUND)
	add_executable(dvd_player dvd_player.c dvd_drive.c dvd_open.c dvd_vmg_ifo.c dvd_track.c dvd_cell.c dvd_vts.c dvd_audio.c dvd_subtitles.c dvd_time.c dvd_chapter.c dvd_video.c)
	target_link_libraries(dvd_player PRIVATE dvdread)
	target_link_libraries(dvd_player PRIVATE m)
	target_link_libraries(dvd_player PRIVATE mpv)
	INSTALL(FILES dvd_player PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE DESTINATION bin)
	INSTALL(FILES dvd_player.1 DESTINATION man/man1)

	add_executable(dvd_trip dvd_trip.c dvd_drive.c dvd_open.c dvd_vmg_ifo.c dvd_track.c dvd_cell.c dvd_vts.c dvd_audio.c dvd_subtitles.c dvd_time.c dvd_chapter.c dvd_video.c)
	target_link_libraries(dvd_trip PRIVATE dvdread)
	target_link_libraries(dvd_trip PRIVATE m)
	target_link_libraries(dvd_trip PRIVATE mpv)
	INSTALL(FILES dvd_trip PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE DESTINATION bin)
	INSTALL(FILES dvd_trip.1 DESTINATION man/man1)
endif()

# cmake doesn't have a native uninstall option
add_custom_target("uninstall")
add_custom_command(
	TARGET "uninstall"
	POST_BUILD
	# I don't like using <
	COMMAND xargs -a install_manifest.txt rm -v
	# Not going to use rm -fr either to avoid possible incidents
	COMMAND rmdir -v ${CMAKE_INSTALL_PREFIX}/share/doc/${PROJECT_NAME}
)
