#ifndef DSP_H_
#define DSP_H_

#include <getopt.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
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
extern double Tau_trap;
extern unsigned int K_trap;
extern unsigned int L_trap;
extern double EN_normal;
extern double CFT_fraction;

int init_controller(cyusb_handle **usb_h);

int **read_data_ep(cyusb_handle *usb_h);

double area_signal(int *a);
void min_bubble(int *a, int len, int *min, int *min_num);
double time_signal(int *a); 

int save_data_in_file(const int out_fd, int **data);

#endif
