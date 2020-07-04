#include "dsp.h"
#define JSMN_HEADER
#include "parse_args.h"

int main(int argc, char **argv)
{
    int res = 0;
    unsigned int time = 100;
    cyusb_handle *usb_h = NULL;

    res = init_controller(&usb_h);
	if (res != 0) {
		fprintf(stderr, "Error in init_controller()\n");

        _exit(-1);
	}

    printf("argv = %s\n", *argv);
    const char *out_foldername = parse_and_give_comm(argc, argv, usb_h, NULL, &time, NULL);
    if (out_foldername == NULL) {
        exit_controller(usb_h);
        
        _exit(-2);
    }

    exit_controller(usb_h);

    _exit(0);
}
