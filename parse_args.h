#ifndef PARSE_ARGS_H_
#define PARSE_ARGS_H_

#include <getopt.h>
#include <string.h>
#include <unistd.h>

#include "dsp.h"

const char *parse_and_give_comm(int argc, char **argv, cyusb_handle *usb_h, unsigned int *time);

#endif
