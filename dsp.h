#ifndef DSP_H_
#define DSP_H_

#include <getopt.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <cyusb.h>


typedef struct {
	double en;
	double t;
	char det;
} einfo_t;


extern const int SIZEOF_SIGNAL;
extern const int CALC_SIZE;
extern double Tau_trap;
extern unsigned int K_trap;
extern unsigned int L_trap;
extern double EN_normal;
extern double CFT_fraction;

int init_controller(cyusb_handle **usb_h);
int exit_controller(cyusb_handle *usb_h);
int control_send_comm(cyusb_handle *usb_h, const char *comm, int args);

int **read_data_ep(cyusb_handle *usb_h, int **data);

double area_trap_signal(int *a);
void min_bubble(int *a, int len, int *min, int *min_num);
double time_line_signal(int *a);
int calc_en_t(int *data, einfo_t *event, double (*area_f)(int *a), double (*time_f)(int *a));

int save_data_in_file(const int out_fd, int **data);

int calc_histo(einfo_t *events, const int **en_range, unsigned int **histo_en, unsigned int **start);

int alloc_mem_data(int ***data);
void free_mem_data(int ***data);
int alloc_mem_events(einfo_t ***events);
void free_mem_events(einfo_t ***events); 

#endif
