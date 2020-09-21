#include "dvd_device.h"

/**
 * Check to see if the file can actually be accessed by the user running
 * the program.  This would check to see if the file exists as well.
 */
bool dvd_device_access(const char *device_filename) {

	if(access(device_filename, F_OK) == 0)
		return true;
	else
		return false;

}

/**
 * Check if device is hardware (/dev/dvd, /dev/dvd1, etc.)
 */
bool dvd_device_is_hardware(const char *device_filename) {

	if(strncmp(device_filename, "/dev/", 5) == 0)
		return true;
	else
		return false;

}

/**
 * Check if device is an image (filename)
 */
bool dvd_device_is_image(const char *device_filename) {

	if(dvd_device_is_hardware(device_filename))
		return false;
	else
		return true;

}
