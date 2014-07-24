#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/cdrom.h>
#include "dvd_device.h"

bool dvd_device_access(const char *device_filename) {

	if(access(device_filename, F_OK) == 0)
		return true;
	else
		return false;

}

// Returns DVD device file descriptor
int dvd_device_open(const char *device_filename) {

	return open(device_filename, O_RDONLY | O_NONBLOCK);

}

int dvd_device_close(const int dvd_fd) {

	return close(dvd_fd);

}

bool dvd_device_is_hardware(const char *device_filename) {

	if(strncmp(device_filename, "/dev/", 5) == 0)
		return true;
	else
		return false;

}

bool dvd_device_is_image(const char *device_filename) {

	if(dvd_device_is_hardware(device_filename))
		return false;
	else
		return true;

}
