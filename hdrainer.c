#include <stdio.h>
#include <unistd.h>
#include <zmq.h>

#include "dsp.h"
#include "parse_args.h"

volatile int read_cycle_flag = 1;
static unsigned long int cycles = 0;
const char *HDrainer_res_file = "/home/das/job/dsp/test/hdrainer_res";
const int en_range[4][4] = {{600, 700, 600, 700}, {600, 700, 600, 700}, {600, 700, 600, 700}, {600, 700, 600, 700}};


void catch_alarm(int sig_num)
{
	read_cycle_flag = 0;

	signal(sig_num, catch_alarm);
}

void catch_sigusr1(int sig_num)
{
	fprintf(stderr, "sig num = %d\n", sig_num);
	printf("%ld\n", cycles); 

	signal(sig_num, catch_sigusr1);
}


int main(int argc, char **argv)
{
	int i;
	int res = 0;
    unsigned int time = 100;
	const char *out_foldername = NULL;
	cyusb_handle *usb_h = NULL;
	
	res = init_controller(&usb_h);
	if (res != 0) {
		fprintf(stderr, "Error in init_controller()\n");

		return -1;
	}

	out_foldername = parse_and_give_comm(argc, argv, usb_h, &time);
	if (out_foldername == NULL) {
		exit_controller(usb_h);

		fprintf(stderr, "Error in parse_and_give_comm()\n");

		return -1;
	}
#ifdef DEBUG
	fprintf(stdout, "out_foldername = %s\n", out_foldername);
#endif


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
	
	/*
	void *zmq_context = zmq_ctx_new();
	void *zmq_sock = zmq_socket(zmq_context, ZMQ_REQ);
	zmq_connect(zmq_sock, "tcp://localhost:5555");
	zmq_msg_t msg;
	res = zmq_msg_init_size(&msg, 512);
	if (res != 0) {
		perror("Error in zmq_msg_init_size()");

		exit_controller(usb_h);

		free_mem_data(&data);
		free_mem_events(&events);
		free_mem_histo(&histo_en, &start);
		
		close(out_fd);
		for (i = 0; i < 16; i++) {
			close(out_histo_fd[i]);
		}
		free(out_histo_fd); out_histo_fd = NULL;

		return -1;
	}
	*/

	fprintf(stderr, "not here or here? #0\n");

	//set alarm and its handler
	alarm(time);
	signal(SIGALRM, catch_alarm);

#ifdef DEBUG
	printf("line=%d | time = %u\n", __LINE__, time);
#endif

	while (read_cycle_flag) {
		data = read_data_ep(usb_h, data);
		if (data == NULL) {
			fprintf(stderr, "Error in read_data_ep()\n");

			exit_controller(usb_h);

			free_mem_data(&data);
			free_mem_events(&events);
			free_mem_histo(&histo_en, &start);
			
			close(out_fd);
			for (i = 0; i < 16; i++) {
				close(out_histo_fd[i]);
			}
			free(out_histo_fd); out_histo_fd = NULL;
			
			return -1;
		}

#ifdef DEBUG
		//	printf("I'm here line = %d and data was read | counter_events = %d\n", __LINE__, counter_events);
#endif
		save_data_in_file(out_fd, data);

		if (counter_events % CALC_SIZE == 0) {
			calc_histo(events, en_range, histo_en, start);
			save_histo_in_file(out_histo_fd, histo_en, start);
			counter_events = 0;
		}

		for (i = 0; i < 4; i++) {
			calc_en_t(data[i], events[counter_events + i], area_integral, time_line_signal);
#ifdef DEBUG
		printf("area = %.2e, time = %.2e, det = %d\n", events[counter_events + i]->en, events[counter_events + i]->t, events[counter_events + i]->det);
#endif	
}

		cycles++;
		counter_events += 4;

		//printf("d0 counts = %d, d1 counts = %d\n", data[0][SIZEOF_SIGNAL - 2], data[1][SIZEOF_SIGNAL - 2]);
		//printf("num of cycles = %ld, counter = %d, %d\n", cycles, counter_events, counter_events % CALC_SIZE);
		//printf("%ld\n", cycles);
		//sprintf((char *)zmq_msg_data(&msg), "%ld", cycles);
		//zmq_msg_send(&msg, zmq_sock, 0);
	}
	printf("\n");

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
