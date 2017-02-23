#include "parse_args.h"

static const char *out_foldername_default = "../test/histos";
static int with_signal_flag = 0;

const char *parse_and_give_comm(int argc, char **argv, cyusb_handle *usb_h, unsigned int *time)
{
	int i;
    int res = 0;
	int option = 0;
	const char *out_foldername = out_foldername_default;
	unsigned int p[4] = {100, 100, 100, 100};
	char porogs[512];
	strncpy(porogs, "100 100 100 100", 512);
	char *token = NULL;
	int delay = 100;

	if (argc == 1) {
		*time = 100;

		return out_foldername;
	}

	while ( (option = getopt (argc, argv, "t:o:p:d:rncs") ) != -1) {
		switch (option) {
		case 't':
			//set time of measurements
			*time = atoi(optarg);

			break;
		case 'o':
			//select foldername for output
			out_foldername = optarg;
		
			break;
		case 'p':
			strncpy(porogs, optarg, 512);
            
            i = 0;
            token = strtok(porogs, " ");
            while (token != NULL) {
                p[i++] = atoi(token);
                token = strtok(NULL, " ");
            }
			
            usleep(100);
			res = control_send_comm(usb_h, "s0", p[0]);
            
            if (res != 0) {
                return NULL;
            }
            
            usleep(100);
			res = control_send_comm(usb_h, "s1", p[1]);
            
            if (res != 0) {
                return NULL;
            }
          
			usleep(100);
            res = control_send_comm(usb_h, "s2", p[2]);
          
            if (res != 0) {
                return NULL;
            }
            
			usleep(100);
            res = control_send_comm(usb_h, "s3", p[3]);
            
            if (res != 0) {
                return NULL;
            }
            
			break;
		case 'd':
			//set delay
			delay = atoi(optarg);
            
            usleep(100);
			res = control_send_comm(usb_h, "w", delay);
            
            if (res != 0) {
                return NULL;
            }
            
            /*
            res = control_send_comm(usb_h, "r", 0);
            if (res != 0) {
                return NULL;
            }
            */
			break;
        case 'r':
			//reset
			usleep(100);
            res = control_send_comm(usb_h, "r", 0);
            
			if (res != 0) {
                return NULL;
            }
            
            break;
		case 'n':
			//set no-coinc mode
            usleep(100);
			res = control_send_comm(usb_h, "n", 0);
            
			if (res != 0) {
                return NULL;
            }
            
			break;
        case 'c':
            //set coinc mode
            usleep(100);
			res = control_send_comm(usb_h, "c", 0);
            
			if (res != 0) {
                return NULL;
            }
            
            break;
		case 's':
			with_signal_flag = 1;

			break;
		default:
			return NULL;
		}
	}

//	printf("time = %u, out_foldername = %s, delay = %d, c_flag = %d\n", *time, out_foldername, delay, coinc_flag);
//	for (i = 0; i < 4; i++) {
//		printf("p[%d] = %d\n", i, p[i]);
//	}

	return out_foldername;
}
