#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include "dsp.h"
#include "parse_args.h"

volatile int read_cycle_flag = 1;
const char *OConsumer_res_file = "../test/oconsumer_res";
const int en_range[4][4] = {{30, 200, 30, 200}, {30, 200, 30, 200}, {30, 200, 30, 200}, {30, 200, 30, 200}};


void catch_alarm(int sig_num)
{
	read_cycle_flag = 0;

	signal(sig_num, catch_alarm);
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
	alarm(time);

	int **data = NULL;
	res = alloc_mem_data(&data);
	if (res != 0) {
		exit_controller(usb_h);

		fprintf(stderr, "Error in alloc_mem_data()\n");
		
		return -1;
	}

	int out_fd = open(OConsumer_res_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (out_fd == -1) {
		perror("Error in open file for drainer");

		return -1;
	}

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
		exit_controller(usb_h);

		free_mem_data(&data);
		free_mem_events(&events);
		free_mem_histo(&histo_en, &start);

		close(out_fd);
	
		fprintf(stderr, "Error in open_files_for_histo()... Exit\n");

		return -1;
	}
    
	int **fds = create_FIFO(FIFO_FOLDERNAME);
	if (fds == NULL) {
		exit_controller(usb_h);

		free_mem_data(&data);
		free_mem_events(&events);
		free_mem_histo(&histo_en, &start);

		close(out_fd);
		for (i = 0; i < 16; i++) {
			close(out_histo_fd[i]);
		}
		free(out_histo_fd); out_histo_fd = NULL;

		fprintf(stderr, "Error in create_FIFO()... Exit\n");
		perror("");

		return -1;
	}

	signal(SIGALRM, catch_alarm);
    
    clock_t start_t, end_t;

	while (read_cycle_flag) {
		data = read_data_ep(usb_h, data);
		if (data == NULL) {
			fprintf(stderr, "Error in read_data_ep()\n");
			
			return -1;
		}

		save_data_in_file(out_fd, data);

		
		if (counter_events % CALC_SIZE == 0) {
            start_t = clock();
			
            calc_histo(events, en_range, histo_en, start);
			
            end_t = clock();
            
			save_histo_in_file(out_histo_fd, histo_en, start);
            
            transfer_data_to_FIFO(fds, histo_en, start);

			counter_events = 0;

			printf("calc_histo() should be and it has taken %.6f s\n", (end_t - start_t)/(double)CLOCKS_PER_SEC);
		}
		
		for (i = 0; i < 4; i++) {
			calc_en_t(data[i], events[counter_events + i], area_trap_signal, time_line_signal);
		}

		cycles++;
		counter_events += 4;

		//printf("num of cycles = %ld, counter = %d, %d\n", cycles, counter_events, counter_events % CALC_SIZE);
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

	close_fifo(&fds);

	return 0;
}
