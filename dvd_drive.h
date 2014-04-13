/**
 * @file dvd_drive.h
 *
 * Functions to get status of DVD drive hardware tray
 *
 * There's two types of functions: ones that can be used externally to see if
 * there is a DVD or not, and others internally to get a closer look at the
 * actual status of the drive.
 *
 * To simply check if everything is READY TO GO, use dvd_drive_has_media()
 *
 * For example:
 *
 * if(dvd_drive_has_media("/dev/dvd"))
 * 	printf("Okay to go!\n");
 * else
 * 	printf("Need a DVD before I can proceed.\n");
 *
 */

/**
 * Get CDROM_DRIVE_STATUS from the kernel
 * See 'linux/cdrom.h'
 *
 * Should be used by internal functions to get drive status, and then set
 * booleans based on the return value.
 *
 * @param DVD device filename (/dev/dvd, /dev/bluray, etc.)
 * @retval 1 CDS_NO_DISC
 * @retval 2 CDS_TRAY_OPEN
 * @retval 3 CDS_DRIVE_NOT_READY
 * @retval 4 CDS_DISC_OK
 */
int dvd_drive_get_status(const char *device_filename);

/**
 * Check if the DVD tray IS CLOSED and HAS A DVD
 *
 * @param DVD device filename (/dev/dvd, /dev/bluray, etc.)
 * @return bool
 */
bool dvd_drive_has_media(const char *device_filename);

/**
 * Check if the DVD tray IS OPEN
 *
 * @param DVD device filename (/dev/dvd, /dev/bluray, etc.)
 * @return bool
 */
bool dvd_drive_is_open(const char *device_filename);

/**
 * Check if the DVD tray IS CLOSED
 *
 * @param DVD device filename (/dev/dvd, /dev/bluray, etc.)
 * @return bool
 */
bool dvd_drive_is_closed(const char *device_filename);

/**
 * Check if the DVD tray IS READY TO QUERY FOR STATUS
 *
 * Should be used internally only.  All other functions run this anytime it is
 * doing a check to see if it's okay to query the status.
 *
 * This function is necessary for handling those states where a drive tray is
 * being opened or closed, and it hasn't finished initilization.  Once this
 * returns true, everything else has the green light to go.
 *
 * @param DVD device filename (/dev/dvd, /dev/bluray, etc.)
 * @return bool
 */
bool dvd_drive_is_ready(const char *device_filename);

/**
 * Human-friendly print-out of the dvd drive status
 *
 * @param DVD device filename (/dev/dvd, /dev/bluray, etc.)
 */
void dvd_drive_display_status(const char *device_filename);
