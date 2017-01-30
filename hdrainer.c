#include <stdio.h>
#include <string.h>
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

int *open_files_for_histo(const char *foldername)
{
	int i;
	int ret = 0;
	struct stat st = {0};
	char *tempstr = (char *)calloc(512, sizeof(char));
	if (tempstr == NULL) {
		return NULL;
	}
	int *out_fd = (int *)calloc(16, sizeof(int));

	if ( (foldername == NULL) || strlen(foldername) == 0 ) {
		return NULL;
	}
	
	ret = stat(foldername, &st);
    if (ret == -1) {
		#ifdef DEBUG
		fprintf(stderr, "stat(%s) returns -1\n", foldername);
		#endif
        ret = mkdir(foldername, 0777);
		if (ret == -1) {
			perror("");
			free(out_fd); out_fd = NULL;
			free(tempstr); tempstr = NULL;

			return NULL;
		}
    }

	for (i = 0; i < 4; i++) {
		sprintf(tempstr, "%s/BUFKA%d.SPK", foldername, i + 1);
		out_fd[i] = open(tempstr, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		if (out_fd[i] == -1) {
			free(out_fd); out_fd = NULL;
			free(tempstr); tempstr = NULL;

			perror("Error in open file for BUFKA");

			return NULL;
		}
		strncpy(tempstr, "", 512);
	}

	for (i = 0; i < 12; i++) {
		sprintf(tempstr, "%s/TIME%d.SPK", foldername, i + 1);
		out_fd[i + 4] = open(tempstr, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		if (out_fd[i + 4] == -1) {
			free(out_fd); out_fd = NULL;
			free(tempstr); tempstr = NULL;

			perror("Error in open file for TIME");

			return NULL;
		}
		strncpy(tempstr, "", 512);		
	}

	free(tempstr); tempstr = NULL;

	return out_fd;
}


int main(int argc, char **argv)
{
	int i, j;
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

	unsigned long int cycles = 0;
	int counter_events = 0;
	
	einfo_t **events = NULL;
	res = alloc_mem_events(&events);
	if (res != 0) {
		fprintf(stderr, "Error in alloc_mem_events()\n");

		return -1;
	}

	unsigned int **histo_en = (unsigned int **)calloc(4, sizeof(unsigned int *));
	if (histo_en == NULL) {
		fprintf(stderr, "Error in alloc **histo_en\n");

		return -1;
	}
	for (i = 0; i < 4; i++) {
		histo_en[i] = (unsigned int *)calloc(HIST_SIZE, sizeof(unsigned int));
		if (histo_en[i] == NULL) {
			for (j = 0; j < i; j++) {
				free(histo_en[j]); histo_en[j] = NULL;
			}
			free(histo_en); histo_en = NULL;

			return -1;
		}
	}

	unsigned int **start = (unsigned int **)calloc(12, sizeof(unsigned int *));
	if (start == NULL) {
		fprintf(stderr, "Error in alloc **start\n");
		
		return -1;
	}
	for (i = 0; i < 12; i++) {
		start[i] = (unsigned int *)calloc(HIST_SIZE, sizeof(unsigned int));
		if (start[i] == NULL) {
			for (j = 0; j < i; j++) {
				free(start[j]); start[j] = NULL;
			}
			free(start); start = NULL;

			return -1;
		}
	}

	int *out_histo_fd = open_files_for_histo("/home/das/job/dsp/test/histos");
	if (out_histo_fd == NULL) {
		free_mem_data(&data);
		free_mem_events(&events);

		exit_controller(usb_h);

		return -1;
	}

	//10 seconds to alarm
	signal(SIGALRM, catch_alarm);
	alarm(10);

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
 
	exit_controller(usb_h);

	free_mem_data(&data);
	free_mem_events(&events);

	close(out_fd);
	for (i = 0; i < 16; i++) {
		close(out_histo_fd[i]);
	}

	return 0;
}
