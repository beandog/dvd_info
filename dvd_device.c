#include "dvd_device.h"

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
