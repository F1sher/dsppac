#include <stdio.h>
#include <unistd.h>

#include "dsp.h"

volatile int read_cycle_flag = 1;
const char *HDrainer_res_file = "../test/hdrainer_res";
const int en_range[4][4] = {{600, 700, 600, 700}, {600, 700, 600, 700}, {600, 700, 600, 700}, {600, 700, 600, 700}};


void catch_alarm(int sig_num)
{
	read_cycle_flag = 0;

	signal(sig_num, catch_alarm);
}

const char *parse_and_give_comm(int argc, char **argv, cyusb_handle *usb_h)
{
	int i;
	int option = 0;
	int time = 10;
	const char *out_foldername = "../test/histos";
	unsigned int p[4] = {100, 100, 100, 100};
	char porogs[512];
	strncpy(porogs, "100 100 100 100", 512);
	char *token = NULL;
	int delay = 100;
	int coinc_flag = 1;

	while ( (option = getopt (argc, argv, "t:o:p:d:n") ) != -1) {
		switch (option) {
		case 't':
			//set time of measurements
			time = atoi(optarg);

			break;
		case 'o':
			//select foldername for output
			out_foldername = optarg;
		
			break;
		case 'p':
			strncpy(porogs, optarg, 512);
			
			break;
		case 'd':
			//set delay
			delay = atoi(optarg);

			break;
		case 'n':
			//set no-coinc mode
			coinc_flag = 0;
			break;
		default:
			return NULL;
		}
	}

	i = 0;
	token = strtok(porogs, " ");
	while (token != NULL) {
		p[i++] = atoi(token);

		token = strtok(NULL, " ");
	}

	printf("time = %d, out_foldername = %s, delay = %d, c_flag = %d\n", time, out_foldername, delay, coinc_flag);
	for (i = 0; i < 4; i++) {
		printf("p[%d] = %d\n", i, p[i]);
	}
	
	control_send_comm(usb_h, "s0", p[0]);
	usleep(1000);
	control_send_comm(usb_h, "s1", p[1]);
	usleep(1000);
	control_send_comm(usb_h, "s2", p[2]);
	usleep(1000);
	control_send_comm(usb_h, "s3", p[3]);
	usleep(1000);
	control_send_comm(usb_h, "r", 0);

	control_send_comm(usb_h, "w", delay);
	control_send_comm(usb_h, "r", 0);

	if (coinc_flag == 1) {
		control_send_comm(usb_h, "c", 0);
	}
	else {
		control_send_comm(usb_h, "n", 0);
	}
	
	control_send_comm(usb_h, "r", 0);

	alarm(time);

	return out_foldername;
}


int main(int argc, char **argv)
{
	int i;
	int res = 0;
	const char *out_foldername = NULL;
	cyusb_handle *usb_h = NULL;
	
	res = init_controller(&usb_h);
	if (res != 0) {
		fprintf(stderr, "Error in init_controller()\n");

		return -1;
	}

	out_foldername = parse_and_give_comm(argc, argv, usb_h);
	if (out_foldername == NULL) {
		exit_controller(usb_h);

		fprintf(stderr, "Error in parse_and_give_comm()\n");

		return -1;
	}

	int **data = NULL;
	res = alloc_mem_data(&data);
	if (res != 0) {
		exit_controller(usb_h);

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

	unsigned long int cycles = 0;
	int counter_events = 0;
	
	einfo_t **events = NULL;
	res = alloc_mem_events(&events);
	if (res != 0) {
		free_mem_data(&data);

		exit_controller(usb_h);

		fprintf(stderr, "Error in alloc_mem_events()\n");

		return -1;
	}

	unsigned int **histo_en = NULL;
	unsigned int **start = NULL;
	res = alloc_mem_histo(&histo_en, &start);
	if (res != 0) {
		free_mem_data(&data);
		free_mem_events(&events);
		
		exit_controller(usb_h);

		fprintf(stderr, "Error in alloc_mem_histo()\n");

		return -1;
	}

	int *out_histo_fd = open_files_for_histo(out_foldername);
	if (out_histo_fd == NULL) {
		free_mem_data(&data);
		free_mem_events(&events);

		exit_controller(usb_h);

		return -1;
	}

	signal(SIGALRM, catch_alarm);

	while (read_cycle_flag) {
		data = read_data_ep(usb_h, data);
		if (data == NULL) {
			fprintf(stderr, "Error in read_data_ep()\n");
			
			return -1;
		}

		save_data_in_file(out_fd, data);
		
		for (i = 0; i < 4; i++) {
			calc_en_t(data[i], events[counter_events + i], area_integral, time_line_signal);
			//	printf("area = %.2e, time = %.2e, det = %d\n", event.en, event.t, event.det);
		}

		if (counter_events % CALC_SIZE == 0) {
			calc_histo(events, en_range, histo_en, start);
			save_histo_in_file(out_histo_fd, histo_en, start);
			counter_events = 0;
			printf("calc_histo() should be\n");
		}

		cycles++;
		counter_events += 4;

		//printf("d0 counts = %d, d1 counts = %d\n", data[0][SIZEOF_SIGNAL - 2], data[1][SIZEOF_SIGNAL - 2]);
		printf("num of cycles = %ld, counter = %d, %d\n", cycles, counter_events, counter_events % CALC_SIZE);
	}

	printf("d0 counts = %d, d1 counts = %d\n", data[0][SIZEOF_SIGNAL - 2], data[1][SIZEOF_SIGNAL - 2]);
	printf("num of cycles end = %ld\n", cycles);
 
	save_histo_in_ascii(out_foldername, histo_en, start);

	exit_controller(usb_h);

	free_mem_data(&data);
	free_mem_events(&events);
	free_mem_histo(&histo_en, &start);

	close(out_fd);
	for (i = 0; i < 16; i++) {
		close(out_histo_fd[i]);
	}
	free(out_histo_fd); out_histo_fd = NULL;

	return 0;
}
