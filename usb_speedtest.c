#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include "dsp.h"

int main(int argc, char **argv)
{
	const long int MAX_CYCLES = 100000;
	long int cycles = 0;
	int res = 0;
	cyusb_handle *usb_h = NULL;

	clock_t start, stop;
	double cpu_time_used;

	start = clock();

	res = init_controller(&usb_h);
	if (res != 0) {
		fprintf(stderr, "Error in init_controller()\n");

		return -1;
	}

	int **data = NULL;
	res = alloc_mem_data(&data);
	if (res != 0) {
		exit_controller(usb_h);

		fprintf(stderr, "Error in alloc_mem_data()\n");
		
		return -1;
	}

	stop = clock();
	cpu_time_used = ((double) (stop - start))/CLOCKS_PER_SEC;
	printf("CPU TIME USED for INIT= %.6f s\n", cpu_time_used);

	start = clock();

	while (cycles < MAX_CYCLES) {
		data = read_data_ep(usb_h, data);
		if (data == NULL) {
			fprintf(stderr, "Error in read_data_ep()\n");

			exit_controller(usb_h);

			free_mem_data(&data);
		    
			return -1;
		}

		cycles++;
	}

	stop = clock();
	cpu_time_used = ((double) (stop - start))/CLOCKS_PER_SEC;
	printf("CPU TIME USED for CYCLE = %.6f s\n", cpu_time_used);

	exit_controller(usb_h);
	free_mem_data(&data);

	return 0;
}
