#include "dsp.h"

static const double EPS = 1.0e-06;

static const int IN_EP = 0x86;
static const int OUT_EP = 0x02;
static const int SIZEOF_DATA_EP = 2048;
const int SIZEOF_SIGNAL = 256;
static const int CONTROL_REQUEST_TYPE_IN = LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_DEVICE;

double Tau_trap = 0.95;
unsigned int K_trap = 4;
unsigned int L_trap = 16;
double EN_normal = 1000.0;
double CFT_fraction = 0.5;

static int flag_fifo_wr = 1;

const char *FIFO_fullname = "./fifo/events";


int init_controller(cyusb_handle **usb_h)
{
    int ret = 0;
	
	ret = cyusb_open();
	if (ret != 1) {
		if (ret < 0) {
			fprintf(stderr, "Error in opening cyusb lib\n");
			
			return -1;
		}
		else if (ret == 0) {
			fprintf(stderr, "No device found\n");
			
			return -1;
		}
		if (ret > 1) {
			fprintf(stderr, "More than 1 devices of interest found. Disconnect unwanted devices\n");
			
			return -1;
		}
	}

	*usb_h = cyusb_gethandle(0);
	if (cyusb_getvendor(*usb_h) != 0x04b4) {
		fprintf(stderr, "Cypress chipset not detected\n");
		cyusb_close();
	
		return -1;
	}

	ret = cyusb_kernel_driver_active(*usb_h, 0);
	if (ret != 0) {
		fprintf(stderr, "kernel driver active. Exitting\n");
		cyusb_close();

		return -1;
	}

	ret = cyusb_claim_interface(*usb_h, 0);
	if (ret != 0) {
		fprintf(stderr, "Error in claiming interface\n");
		cyusb_close();

		return -1;
	}
	else { 
		fprintf(stderr, "Successfully claimed interface\n");
	}
		
	cyusb_clear_halt(*usb_h, OUT_EP);

	return 0;
}


///FUNCTIONS FOR READ DATA from ENDPOINT
int control_test(cyusb_handle *usb_h, unsigned char bmRequestType)
{
	unsigned char datac[2] = {15};
	
	cyusb_control_transfer(usb_h, bmRequestType, 0x00, 0x0000, 0x0000, datac, 2, 0); //wIndex = 0x0000 or LIBUSB_ENDPOINT_IN | IN_EP 0b0000 0000 1000 xxxx
	
	return datac[0];
}

void swap_int(int *a, int *b)
{
	int temp = *b;
	
	*b = *a;
	*a = temp;
}

int **read_data_ep(cyusb_handle *usb_h, int **data)
{
	int i = 0;
	int j = 0;
	int res = 0;
	int trans = 0;
	int det_num = -1;
	unsigned char buf[SIZEOF_DATA_EP] = {0};

	//remove after tests
	#ifdef DEBUG
	int test_cntrl = 0;
	#endif

	//...malloc data...
	if (data == NULL) {
		fprintf(stdin, "malloc int **data\n");

		data = (int **)calloc(4, sizeof(int *));
		if (data == NULL) {
			fprintf(stderr, "Error in memory allocation for int **data\n");
			return NULL;
		}

		for (i = 0; i < 4; i++) {
			data[i] = (int *)calloc(SIZEOF_SIGNAL, sizeof(int));
			if (data[i] == NULL) {
				for (j = 0; j < i; j++) {
					free(data[j]); data[j] = NULL;
				}
				free(data); data = NULL;
	
				fprintf(stderr, "Error in memory allocationt for int *data[%d]\n", i);
				return NULL;
			}
		}
	}
	//...check malloc errors...

	while ( (res = control_test(usb_h, CONTROL_REQUEST_TYPE_IN)) != 1 ) {
		#ifdef DEBUG
		test_cntrl++;
		#endif

		;
	}
	#ifdef DEBUG
	if (test_cntrl != 0) {
		printf("test_cntrl = %d\n", test_cntrl);
	}
	#endif

	res = cyusb_bulk_transfer(usb_h, IN_EP, buf, SIZEOF_DATA_EP, &trans, 0);
	if (res != 0) {
		if (trans != SIZEOF_DATA_EP) {
			//ERROR in TRANSFER SIZE OF DATA
		}
		cyusb_error(res);
		//...
		return NULL;
	}

	for (i = 0; i < 4; i++) {
		det_num = (buf[2*SIZEOF_SIGNAL*(i + 1) - 1] >> 6) + 1;
		
		for (j = 0; j < SIZEOF_SIGNAL; j++) {
			data[i][j] = buf[2*SIZEOF_SIGNAL*i + 2*j] + 256*(buf[2*SIZEOF_SIGNAL*i + 2*j + 1] & 0b00111111);
		}

		for (j = 0; j < SIZEOF_SIGNAL/2; j++) {
			swap_int( &(data[i][j]), &(data[i][SIZEOF_SIGNAL - j - 1]) );
		}

		data[i][SIZEOF_SIGNAL - 1] = det_num;
	}

	return data;
}


//FUNCTIONS FOR CALC EN and T for SIGNALS

double area_signal(int *a)
{
    const int Averaging = 10;
    int i = 0;
	int i_min_s = 0;
	double res = 0.0;
   
    double baseline = 0.0;
    for (i = 0; i < 10; i++) {
	    baseline += (double)a[i];
    }
    baseline = baseline/10.0;
    
    double *a_clear = (double *)calloc(SIZEOF_SIGNAL/2, sizeof(double));
	if (a_clear == NULL) {
	    return -1.0;
	}
    double *cTr = (double *)calloc(SIZEOF_SIGNAL/2, sizeof(double));
    if (cTr == NULL) {
	    free(a_clear); a_clear = NULL;

	    return -1.0;
	}
	double *cs = (double *)calloc(SIZEOF_SIGNAL/2, sizeof(double));
    if (cs == NULL) {
		free(a_clear); a_clear = NULL;
		free(cTr); cTr = NULL;

		return -1.0;
	}
    
    for (i = 1; i < SIZEOF_SIGNAL/2; i++) {
		if ((int)(a[i] - baseline) > 0) {
            a_clear[i] = 0.0;
        }
        else {
            a_clear[i] = a[i] - baseline;
        }
        
        cs[i] = cTr[i] = a_clear[i]; 
    }
    
    for (i = 1; i < SIZEOF_SIGNAL/2; i++) {
        cTr[i] = cTr[i-1] + a_clear[i] - a_clear[i-1]*(1.0 - 1.0/Tau_trap);
    }
    
    for (i = 1; i < SIZEOF_SIGNAL/2; i++) {
        if (i - L_trap - K_trap >= 0) {
            cs[i] = cs[i-1] + cTr[i] - cTr[i-K_trap] - cTr[i-L_trap] + cTr[i-K_trap-L_trap];
        }
        else if (i - L_trap >= 0) {
		    cs[i] = cs[i-1] + cTr[i] - cTr[i-K_trap] - cTr[i-L_trap];
        }
        else if (i - K_trap >= 0) {
            cs[i] = cs[i-1] + cTr[i] - cTr[i-K_trap];
        }
        else {
            cs[i] = cs[i-1] + cTr[i];
        }
    }
    
    for (i = 1; i < SIZEOF_SIGNAL/2; i++) {
	    if (a[i] <= 0.995*a[i-1]) {
		    i_min_s = i + K_trap + 5; //+3 or +7
            
			break;
        }
    }
    
    if (i_min_s + L_trap - K_trap > SIZEOF_SIGNAL - 1) {
	    return 0.0;
    }
    
    for (i = i_min_s; i < i_min_s + Averaging; i++) { //i<i_min_s + L - K - 1 or 8 - 1
		res += cs[i];
    }
    res = res/(Averaging); //res = res/(L-K-1) or res/8

    free(a_clear); a_clear = NULL;
    free(cTr); cTr = NULL;
    free(cs); cs = NULL;
    
    return fabs(res*EN_normal);
}

void min_bubble(int *a, int len, int *min, int *min_num)
{
	int i;
	int minimum = a[0];
	int minimum_num = 0;
	
	for (i = 1; i < len; i++) {
		if (a[i] < minimum) {
			minimum = a[i];
			minimum_num = i;
		}
	}

	if (min != NULL) {
		*min = minimum;
	}
	if (min_num != NULL) {
		*min_num = minimum_num;
	}
} 

double time_signal(int *a)
{
	int i = 0;
   
	int min_a = -1;
	min_bubble(a, SIZEOF_SIGNAL/2, &min_a, NULL);
    
	double baseline = 0.0;
    for (i = 0; i < 10; i++) {
        baseline += (double)a[i];
    }
    baseline = baseline/10.0;
    
    double edge = baseline - CFT_fraction*(baseline - min_a); //0.5 for gen
    double b = 0.0;
    double k = 0.0;
    double x0 = -1.0;
    
    for (i = 10; i < 128; i++) {
        if ( (a[i] <= edge) && (a[i-1] >= edge) ) {
            k = (double)(a[i] - a[i-1]);
            b = (double)(a[i] - k*i);
        
            if (fabs(k) > EPS) {
                x0 = (edge - b)/k;

                break;
            }
        }
    }
    
    return x0;
}

int calc_en_t(int *data, einfo_t *event, double (*area_f)(int *a), double (*time_f)(int *a))
{
	if ( (data == NULL) || (event == NULL) || (area_f == NULL) || (time_f == NULL) ) {
		return -1;
	}

    event->en = (*area_f)(data);
	if (fabs(event->en + 1.0) < EPS) {
		return -1;
	}

	event->t = (*time_f)(data);
	if ((event->t - SIZEOF_SIGNAL) > EPS) {
		return -1;
	}

    event->det = data[SIZEOF_SIGNAL-1];
	if ( (event->det < 0) || (event->det > 3) ) {
		return -1;
	}

	return 0;
}


//FUNCTIONS for TRANSFER DATA EVENT trough FIFO

int create_fifo(const char *fifoname)
{
    int ret = -1;
	int fd = -1;

	ret = mkfifo(fifoname, 0666);
	if (ret == -1) {
		return -1;
	}

	fd = open(fifoname, O_WRONLY);
	if (fd == -1) {
		return -1;
	}

    return fd;
}

void sigpipe_handler(int sig_num)
{
	fprintf(stderr, "SIGPIPE was\n");
}

int transfer_en_t(einfo_t *arr_of_events)
{
	int ret = -1;
	sighandler_t sigh_ret = NULL;
    //create_fifo
    int fd = -1;
    
	fd = create_fifo(FIFO_fullname);
	if (fd == -1) {
		perror("Error in create_fifo()");

		return -1;
	}

    //handle SIGPIPE
	sigh_ret = signal(SIGPIPE, sigpipe_handler);
	if (sigh_ret == SIG_ERR) {
		perror("Error in SIGPIPE registration");
		close(fd);

		return -1;
	}

	//write data to fifo in cycle, 100 events per cycle
	while (flag_fifo_wr) {
		ret = write(fd, arr_of_events, 100*sizeof(einfo_t));
		if (ret < (int) (100*sizeof(einfo_t))) {
			fprintf(stderr, "write() in %s ret %d instead of %u", __FUNCTION__, ret, 100*sizeof(einfo_t));
		}
	}

    close(fd);

	return 0;
}


int save_data_in_file(const int out_fd, int **data)
{
	int i;
	unsigned int res = 0;
	//open file
	if (out_fd == -1) {
		return -1;
	}

	if (data == NULL) {
		return -1;
	}

	//write data, check errors
	for (i = 0; i < 4; i++) {
		res = write(out_fd, data[i], SIZEOF_SIGNAL*sizeof(int));
		if ( res < SIZEOF_SIGNAL*sizeof(int) ) {
			return -1;
		}
	}
	
	return 0;
}
