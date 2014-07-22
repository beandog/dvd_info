#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/cdrom.h>
#include "dvd_info/dvd_device.h"

bool dvd_device_access(const char* device_filename) {

	int device_access;
	device_access = access(device_filename, F_OK);
	if(device_access == 0)
		return true;
	else
		return false;

}

// Returns DVD device file descriptor
int dvd_device_open(const char* device_filename) {

	int dvd_fd;

	dvd_fd = open(device_filename, O_RDONLY | O_NONBLOCK);

	return dvd_fd;
}

int dvd_device_close(const int dvd_fd) {

	int dvd_close;

	dvd_close = close(dvd_fd);

	return dvd_close;

}

bool dvd_device_is_hardware(const char *device_filename) {

	bool is_hardware;
	is_hardware = false;

	if(strncmp(device_filename, "/dev/", 5) == 0) {
		is_hardware = true;
	}

	return is_hardware;

}

bool dvd_device_is_image(const char *device_filename) {

	bool is_hardware;

	is_hardware = dvd_device_is_hardware(device_filename);

	if(is_hardware)
		return false;
	else
		return true;

}
