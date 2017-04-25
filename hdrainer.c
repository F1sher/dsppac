#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <zmq.h>

#include "dsp.h"
#include "parse_args.h"

volatile int read_cycle_flag = 1;
static unsigned long int cycles = 0;
const char *CONST_file_path = "/home/das/job/dsp/constants.json";
const char *HDrainer_res_file = "/home/das/job/dsp/test/hdrainer_res.sgnl";
int en_range[4][4] = {{600, 700, 600, 700}, {600, 700, 600, 700}, {600, 700, 600, 700}, {600, 700, 600, 700}};
double t_scale[2] = {100.0, 10.0};

const char *socket_communication_path = "./hidden";
int create_socket(const char *socket_path)
{
	struct sockaddr_un addr;
	int ret = 0;
	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd == -1) {
		perror("socket() error");

		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	if (*socket_path == '\0') {
		*addr.sun_path = '\0';
		strncpy(addr.sun_path + 1, socket_path + 1, sizeof(addr.sun_path) - 2);
	}
	else {
		strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path));
	}

	ret = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
	if (ret == -1) {
		perror("connect() error");

		return -1;
	}

	return fd;
}

void catch_alarm(int sig_num)
{
	read_cycle_flag = 0;

	signal(sig_num, catch_alarm);
}

void catch_sigusr1(int sig_num)
{
	fprintf(stdout, "Catch sigusr1 signal");
	
	read_cycle_flag = 0;
}


int main(int argc, char **argv)
{
	int i;
	int res = 0;
	int with_signal_flag = 0;
    unsigned int time_acq = 100;
	const char *out_foldername = NULL;
	cyusb_handle *usb_h = NULL;
	const_t const_params = {0};
	
	res = init_controller(&usb_h);
	if (res != 0) {
		fprintf(stderr, "Error in init_controller()\n");

		return -1;
	}

	out_foldername = parse_and_give_comm(argc, argv, usb_h, \
										 &with_signal_flag, &time_acq, en_range);
	if (out_foldername == NULL) {
		exit_controller(usb_h);

		fprintf(stderr, "Error in parse_and_give_comm()\n");

		return -1;
	}

	res = const_parser(CONST_file_path, &const_params);
	if (res != 0) {
	    exit_controller(usb_h);

	    fprintf(stderr, "Error in const_parser(). res = %d\n", res);
		
		return -1;
    }
	
	set_const_params(const_params);
	
#ifdef DEBUG
	printf("\n");
	int j;
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) 
			printf("en_range[%d][%d] = %d\t", i, j, en_range[i][j]);
		printf("\n");
	}

	fprintf(stdout, "out_foldername = %s\n", out_foldername);

	printf("EN_normal = %.2f | K_trap = %d | L_trap = %d | Tau_trap = %.2f\n", \
		EN_normal, K_trap, L_trap, Tau_trap);
    printf("INT_steps_back = %d | INT_steps_forw = %d\n", INTEGRAL_steps_back, INTEGRAL_steps_forw);
    printf("CFT_fraction = %.2f | T_SCALE = [%.2f, %.2f]\n", CFT_fraction, T_SCALE[0], T_SCALE[1]);
	printf("\n");
#endif

	int **data = NULL;
	res = alloc_mem_data(&data);
	if (res != 0) {
		exit_controller(usb_h);

		fprintf(stderr, "Error in alloc_mem_data()\n");
		
		return -1;
	}

	intens_t intens[4];
	for (i = 0; i < 4; i++) {
		intens[i] = (intens_t) {.get_flag=-1, .counts=-1, .d_counts=0};
	}

	int out_fd = open(HDrainer_res_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (out_fd == -1) {
		exit_controller(usb_h);

		free_mem_data(&data);

		perror("Error in open file for drainer");

		return -1;
	}
	
	int len_out_foldername = strlen(out_foldername);
	char out_sgnl_filename[1024];
	if (out_foldername[len_out_foldername-1] == '/') {
		snprintf(out_sgnl_filename, 1024, "%s%s", out_foldername, "hdrainer_res.sgnl");
	}
	else {
		snprintf(out_sgnl_filename, 1024, "%s/%s", out_foldername, "hdrainer_res.sgnl");
	}
	
	int out_sgnl_fd = open(out_sgnl_filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (out_sgnl_fd == -1) {
		exit_controller(usb_h);

		free_mem_data(&data);

		close(out_fd);

		perror("Error in open file for signal drainer");

		return -1;
	}

	//set wait = 0
	//control_send_comm(usb_h, "w", 0);

	int counter_events = 0;
	
	einfo_t **events = NULL;
	res = alloc_mem_events(&events);
	if (res != 0) {
		exit_controller(usb_h);

		free_mem_data(&data);

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
	
	long int buf[7];
	int fd_sock = create_socket(socket_communication_path);
	if (fd_sock == -1) {
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

	//set alarm and its handler
	alarm(time_acq);
	signal(SIGALRM, catch_alarm);

	//set STOP signal handler
	signal(SIGUSR1, catch_sigusr1);

	//set start time
	struct timeval timeval_start_time, timeval_curr_time;
	gettimeofday(&timeval_start_time, NULL);

	//time_t start_time = time(NULL);
	long int start_time = 1000*timeval_start_time.tv_sec + timeval_start_time.tv_usec/1000;
	
	long int seconds = timeval_start_time.tv_sec;
	long int u_seconds = timeval_start_time.tv_usec;
	

#ifdef DEBUG
	printf("line=%d | time = %u\n", __LINE__, time_acq);
	printf("with_signal_flag = %d\n", with_signal_flag);
#endif

	//Read cycle
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

		get_det_counts(data, intens, 0);

		//if signal mode is ON or OFF?
		if (with_signal_flag == 1) {
			//save to HDrainer_res_file
			save_data_in_file(out_fd, data);
			//save to signal file in directory with histo
			save_data_in_file(out_sgnl_fd, data);
		}

		if ((counter_events % CALC_SIZE == 0) && (counter_events != 0)) {
			#ifdef DEBUG
			printf("calc&save histo | send info to socket\n");
			#endif

			calc_histo(events, en_range, histo_en, start);
			
			save_histo_in_file(out_histo_fd, histo_en, start);

			get_det_counts(data, intens, 1);
			//write to socket and check result
			//cycles == number of read from USB controller. In coinc on mode each read cointains 2 events. Coinc off mode contains 1 event per read.
			//need to create func prepare_buf_to_sock(long int buf[], start_time, inens_t intens, long int *seconds, long int *u_seconds)
			gettimeofday(&timeval_curr_time, NULL);

			buf[0] = cycles; 
			//buf[1] = (long)(time(NULL) - start_time);
			buf[1] = (long)(1000*(timeval_curr_time.tv_sec) + timeval_curr_time.tv_usec/1000 - start_time);

			for (i = 0; i < DET_NUM; i++) {
				buf[2 + i] = (long)(intens[i].d_counts);
			}

#ifdef DEBUG
			printf("BEFORE sec = %ld, u_sec = %ld\n", seconds, u_seconds);
			printf("curr: sec = %ld, u_sec = %ld\n", timeval_curr_time.tv_sec, timeval_curr_time.tv_usec);
			printf("Intens det #3 = %d\n", intens[2].d_counts);
#endif	

			int diff_sec = seconds - timeval_curr_time.tv_sec;
			int diff_usec = u_seconds - timeval_curr_time.tv_usec;
			buf[6] = (long)(-1.0*(1000*diff_sec + diff_usec/1000.0));
			
			seconds = timeval_curr_time.tv_sec;
			u_seconds = timeval_curr_time.tv_usec;

			res = write(fd_sock, buf, sizeof(buf));
			if (res != sizeof(buf)) {
				fprintf(stderr, "Error in writting to the sock! res = %d\n", res);
			}

			counter_events = 0;
		}

		for (i = 0; i < 4; i++) { //add condition to check if ((counter_events + i) < CALC_SIZE)
			calc_en_t(data[i], events[counter_events + i], area_trap_signal, time_line_signal);

#ifdef DEBUG
			//printf("area = %.2e, time = %.2e, det = %d\n", events[counter_events + i]->en, events[counter_events + i]->t, events[counter_events + i]->det);
#endif	
		}

		cycles++;
		counter_events += 4;
	}
	printf("\n");
	
	printf("d0 counts = %d, d1 counts = %d\n", data[0][SIZEOF_SIGNAL - 2], data[1][SIZEOF_SIGNAL - 2]);
	printf("num of cycles end = %ld\n", cycles);
 
	get_det_counts(data, intens, 1);
	for (i = 0; i < DET_NUM; i++) {
		printf("counts[%d] = %08d | get_flag = %d\n", i, intens[i].d_counts, intens[i].get_flag);
	}

	//Save histo
	save_histo_in_file(out_histo_fd, histo_en, start);
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
