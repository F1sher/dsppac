#ifndef PARSE_ARGS_H_
#define PARSE_ARGS_H_

#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dsp.h"
#define JSMN_HEADER
#include "jsmn.h"

typedef struct {
    double EN_normal;
    unsigned int K_trap;
    unsigned int L_trap;
    double Tau_trap;

	int AVERAGE_trap;
	int I_MIN_S_SHIFT_trap;

    int INTEGRAL_steps_back;
	int INTEGRAL_steps_forw;

    double CFT_fraction;
    double T_SCALE[2];
} const_t;

const char *parse_and_give_comm(int argc, char **argv, cyusb_handle *usb_h, int *with_signal_flag, unsigned int *time, int en_range[][4]);

int const_parser(const char *const_filename, const_t *const_params);
void set_const_params(const_t const_params);
int is_uinput_yes();
char *get_num_foldername_spectra(const char *out_foldername);

#endif
