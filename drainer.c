#include <stdio.h>
#include <unistd.h>

#include "dsp.h"

volatile int read_cycle_flag = 1;


void catch_alarm(int sig_num)
{
	read_cycle_flag = 0;

	signal(sig_num, catch_alarm);
}


int main(int argc, char **argv)
{
	int i;
	int res = 0;
	cyusb_handle *usb_h = NULL;
	
	res = init_controller(&usb_h);
	if (res == -1) {
		fprintf(stderr, "Error in init_controller()\n");

		return -1;
	}

	int **data = NULL;
	int out_fd = open("../test/drainer_res", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (out_fd == -1) {
		perror("Error in open file for drainer");

		return -1;
	}

	//10 seconds to alarm
	signal(SIGALRM, catch_alarm);
	alarm(10);
	while (read_cycle_flag) {
		data = read_data_ep(usb_h);
		if (data == NULL) {
			fprintf(stderr, "Error in read_data_ep()\n");
			
			return -1;
		}
	
		save_data_in_file(out_fd, data);

		for (i = 0; i < 4; i++) {
			free(data[i]); data[i] = NULL;
		}
		free(data); data = NULL;
	}

	return 0;
}
