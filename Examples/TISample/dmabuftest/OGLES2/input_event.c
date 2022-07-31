#include "linux/input.h"
#include <fcntl.h>

#include "util.h"

/* avoid to use linux key 0(reserve) and 1(esc) */
#define KEY_CODE_OFFSET 30

int open_input_dev(char *dev_path)
{
	int fd;

	while(access(dev_path, F_OK)) {
		sleep(5);
		MSG("Input device no exists, try again ...");
	}

	if ((fd = open(dev_path, O_RDWR | O_CLOEXEC)) >= 0)
		return fd;

	perror("opne input device error :");
	return -1;
}

int snapshot_status_get(int fd)
{
	int need_snapshot = 0;
	int valid_event;
	struct input_event ie;

	if(read(fd, &ie, sizeof(ie)) > 0) {
		/* MSG("get input:0x%x 0x%x 0x%x", ie.type, ie.code, ie.value); */
		if (ie.type == EV_KEY) {
			if (ie.code == BTN_LEFT || ie.code == BTN_TOUCH) {
				if (ie.value) {
					valid_event = true;
					need_snapshot = 1;
					MSG("Snapshot event get");
				}
			}
		}
	}

	return need_snapshot;
}

void close_input_dev(int fd)
{
	close(fd);
	return;
}
