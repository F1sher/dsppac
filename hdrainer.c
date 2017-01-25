#include <stdio.h>
#include <unistd.h>

#include "dsp.h"

volatile int read_cycle_flag = 1;
const char *HDrainer_res_file = "../test/hdrainer_res";


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
	if (res != 0) {
		fprintf(stderr, "Error in init_controller()\n");

		return -1;
	}

	int **data = NULL;
	res = alloc_mem_data(&data);
	if (res != 0) {
		fprintf(stderr, "Error in alloc_mem_data()\n");
		
		return -1;
	}

	int out_fd = open(HDrainer_res_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (out_fd == -1) {
		perror("Error in open file for drainer");

		return -1;
	}

	//set wait = 0
	//control_send_comm(usb_h, "w", 0);

	//10 seconds to alarm
	signal(SIGALRM, catch_alarm);
	alarm(10);
	unsigned long int cycles = 0;
	int counter_events = 0;
	
	einfo_t **events = NULL;
	res = alloc_mem_events(&events);
	if (res != 0) {
		fprintf(stderr, "Error in alloc_mem_events()\n");

		return -1;
	}

	while (read_cycle_flag) {
		data = read_data_ep(usb_h, data);
		if (data == NULL) {
			fprintf(stderr, "Error in read_data_ep()\n");
			
			return -1;
		}
	
		save_data_in_file(out_fd, data);
		
		for (i = 0; i < 4; i++) {
			calc_en_t(data[i], events[counter_events + i], area_trap_signal, time_line_signal);
			//	printf("area = %.2e, time = %.2e, det = %d\n", event.en, event.t, event.det);
		}

		cycles++;
		counter_events += 4;
		if (cycles % CALC_SIZE == 0) {
			//	calc_histo(events, en_range, histo_en, start);
			counter_events = 0;
			printf("calc_histo() should be\n");
		}
	}
	printf("num of cycles = %ld\n", cycles);
 
	free_mem_data(&data);
	free_mem_events(&events);

	exit_controller(usb_h);

	return 0;
}
