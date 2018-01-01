#include "dsp.h"


static const double EPS = 1.0e-06;

static const int IN_EP = 0x86;
static const int OUT_EP = 0x02;
static const int SIZEOF_DATA_EP = 2048;
const int SIZEOF_SIGNAL = 256;
static const int CONTROL_REQUEST_TYPE_IN = LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_DEVICE;

const int DET_NUM = 4;

const int CALC_SIZE = 10000000;
const int HIST_SIZE = 4096;
const int CALC_TIME = 100;
const int ZMQ_READ_TIME = 2;

double T_SCALE[2] = {100.0, 10.0};
const int EN_THRESHOLD = 10;

double Tau_trap = 0.95;
unsigned int K_trap = 2;
unsigned int L_trap = 12;
unsigned int I_MIN_S_SHIFT_trap = 5;
unsigned int AVERAGE_trap = 10;
int INTEGRAL_steps_back = 10;
int INTEGRAL_steps_forw = 20;
double EN_normal = 4096.0/2000.0;
double CFT_fraction = 0.4;

static int flag_fifo_wr = 1;

const char *FIFO_FOLDERNAME = "/home/das/job/dsp/fifos";



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

int exit_controller(cyusb_handle *usb_h)
{
	cyusb_close();

	return 0;
}

int control_send_comm(cyusb_handle *usb_h, const char *command, int args) 
{
	int ret = 0;
	int transferred = 0;
	unsigned char buf[512];
	
	//word #1 = 1; word #2 = porog - установка порога
	//word #1 = 3; word #2 = 1 - сброс (reset)
	//word #1 = 2; word #2 = задержка - установка задержки
	//word #1 = 5; word #2 = 5 - test
    
    //new
    //word #1 = 1; word #2 = porog - установка порога D1
    //word #1 = 2; -||- D2
    //word #1 = 3; -||- D3
    //word #1 = 4; -||- D4
    //word #1 = 5; word #2 = 1/2 ON/OFF coinc
    //word #1 = 6; word #2 = window - Coinc's window
    //word #1 = 7; word #2 = 1 - сброс (reset)
	
	if (!strcmp(command, "s0") || !strcmp(command, "s1") || !strcmp(command, "s2") || !strcmp(command, "s3")) {
        buf[0] = (unsigned char)(command[1] - '0') + 1;
        buf[2] = args - 256*(args/256);
        buf[3] = args/256;
        
        printf("command = %s rangeset buf[0] = %u buf[2] = %u buf[3] = %u args = %d\n", command, buf[0], buf[2], buf[3], args);
        
        ret = cyusb_bulk_transfer(usb_h, OUT_EP, buf, 512, &transferred, 0);
        if (ret != 0) {
            cyusb_close();
            fprintf(stderr, "ERROR WHILE SET RANGE | %s: transferred %d bytes while should be 512\n", libusb_error_name(ret), transferred);
            cyusb_error(ret);

			return -1;
        }
    }
    else if (!strcmp(command, "r")) {
        buf[0] = 7;
        buf[2] = 1;
        
        ret = cyusb_bulk_transfer(usb_h, OUT_EP, buf, 512, &transferred, 10);
        if (ret != 0) {
            cyusb_error(ret);
            cyusb_close();
            perror("Error in reset comm");

			return -1;
        }
        printf("reset\n");
	}
    else if (!strcmp(command, "w")) {
        buf[0] = 6;
        buf[2] = args - 256*(args/256);
        buf[3] = args/256;
			
        ret = cyusb_bulk_transfer(usb_h, OUT_EP, buf, 512, &transferred, 10);
        if (ret != 0) {
            cyusb_error(ret);
            cyusb_close();
            perror("Error in set delay comm");
        
			return -1;
		}

        printf("wait %u\n", args);
	}
    else if (!strcmp(command, "t")) {
        buf[0] = 1;
        buf[2] = 5;
        
        ret = cyusb_bulk_transfer(usb_h, OUT_EP, buf, 512, &transferred, 10);
        if (ret != 0) {
            cyusb_error(ret);
            cyusb_close();
            perror("Error in test comm");
        
			return -1;
		}
			
        printf("test\n");
	}
    else if (!strcmp(command, "c")) {
        buf[0] = 5;
        buf[2] = 1;
        
        ret = cyusb_bulk_transfer(usb_h, OUT_EP, buf, 512, &transferred, 10);
        if (ret != 0) {
            cyusb_error(ret);
            cyusb_close();
            perror("Error in coinc ON comm");
        
			return -1;
		}
			
        printf("coinc on\n");
	}
    else if (!strcmp(command, "n")) {
        buf[0] = 5;
        buf[2] = 2;
			
        ret = cyusb_bulk_transfer(usb_h, OUT_EP, buf, 512, &transferred, 10);
        if (ret != 0) {
            cyusb_error(ret);
            cyusb_close();
            perror("Error in coinc OFF comm");
        
			return -1;
		}

        printf("coinc off\n");
	}

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
	int det_num = 0;
	int det_counts = 0;
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
	//if (test_cntrl != 0) {
	//	printf("test_cntrl = %d\n", test_cntrl);
	//}
	#endif

	res = cyusb_bulk_transfer(usb_h, IN_EP, buf, SIZEOF_DATA_EP, &trans, 100);
	if (res != 0) {
		if (trans != SIZEOF_DATA_EP) {
			//ERROR in TRANSFER SIZE OF DATA
			;
		}
		cyusb_error(res);
		//...
		return NULL;
	}

	for (i = 0; i < 4; i++) {
		//Set Det number
		det_num = (buf[2*SIZEOF_SIGNAL*(i + 1) - 1] >> 6) + 1;
		//Set Counter
		det_counts = (buf[2*SIZEOF_SIGNAL*(i) + 1] << 24)  + (buf[2*SIZEOF_SIGNAL*(i)] << 16) + (buf[2*SIZEOF_SIGNAL*(i) + 3] << 8) + buf[2*SIZEOF_SIGNAL*(i) + 2];
		//det_counts = 8000;

		for (j = 0; j < SIZEOF_SIGNAL; j++) {
			data[i][j] = buf[2*SIZEOF_SIGNAL*i + 2*j] + 256*(buf[2*SIZEOF_SIGNAL*i + 2*j + 1] & 0b00111111);
		}

		for (j = 0; j < SIZEOF_SIGNAL/2; j++) {
			swap_int( &(data[i][j]), &(data[i][SIZEOF_SIGNAL - j - 1]) );
		}

		//save DET NUMBER
		data[i][SIZEOF_SIGNAL - 1] = det_num;
	
		//save COUNTS in DET
		data[i][SIZEOF_SIGNAL - 2] = det_counts;
	}

	return data;
}

int get_det_counts(int **data, intens_t intens[], int count_flag)
{
	int i, j;

	if (data == NULL) {
		return -1;
	}

	//replace DET_NUM with Signal_in_event
	if (count_flag == 0) {
		for (i = 0; i < DET_NUM; i++) {
			j = data[i][SIZEOF_SIGNAL-1] - 1; //get det_num
			if ( (j < 0) || (j > 3) ) {
				return -1;
			}

			if ( (intens[j].get_flag == -1) || (intens[j].get_flag == 0) ) {
				intens[j].counts = data[i][SIZEOF_SIGNAL - 2]; 
				intens[j].get_flag = 0;
				//intens[j].get_flag++;
			}
		}
	}
	else if (count_flag == 1) {
		for (i = 0; i < DET_NUM; i++) {
			j = data[i][SIZEOF_SIGNAL-1] - 1;
			if ( (j < 0) || (j > 3) ) {
				return -1;
			}
			
			if (intens[j].get_flag == 1) {
				intens[j].d_counts = data[i][SIZEOF_SIGNAL - 2] - intens[j].counts;
				intens[j].get_flag = 0;
			}
		}
	}

	return 0;
}


//FUNCTIONS FOR CALC EN and T for SIGNALS
double area_trap_signal(int *a)
{
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
		    i_min_s = i + K_trap + I_MIN_S_SHIFT_trap; //+3 or +7
            
			break;
        }
    }
    
    if (i_min_s + L_trap - K_trap > SIZEOF_SIGNAL - 1) {
	    return 0.0;
    }
    
    for (i = i_min_s; i < i_min_s + (int)AVERAGE_trap; i++) { //i<i_min_s + L - K - 1 or 8 - 1
		res += cs[i];
    }
    res = res/(AVERAGE_trap); //res = res/(L-K-1) or res/8

    free(a_clear); a_clear = NULL;
    free(cTr); cTr = NULL;
    free(cs); cs = NULL;
    
	//printf("AREA = %.2f | l, r = %d, %d\n", fabs(res*EN_normal), i_min_s, i_min_s + (int)AVERAGE_trap);

    return fabs(res*EN_normal);
}

double area_integral(int *a)
{
	int i;
	double baseline = 0.0;
	int min_signal_num = 0;
	double temp = 0.0;
	double res = 0.0;

	for (i = 0; i < 10; i++) {
		baseline += (double)a[i];
	}
	baseline /= 10.0;

	min_bubble(a, SIZEOF_SIGNAL/2, NULL, &min_signal_num);
	if (min_signal_num - INTEGRAL_steps_back < 0) {
		return -1.0;
	}

	for (i = min_signal_num - INTEGRAL_steps_back; i < min_signal_num + INTEGRAL_steps_forw; i++) {
		if ((temp = a[i] - baseline) < EPS) {
			res += temp;
		}
	}
	
	res /= (double)(INTEGRAL_steps_forw + INTEGRAL_steps_back);

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

double time_line_signal(int *a)
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

#include <gsl/gsl_errno.h>
#include <gsl/gsl_spline.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_roots.h>
//flag to compile
// -I/usr/local/include

struct cubic_params
{
    gsl_interp_accel *accel;
    gsl_spline *spline;
};

double f_cubic (double x, void *params)
{
  struct cubic_params *p 
    = (struct cubic_params *) params;

  gsl_interp_accel *accel = p->accel;
  gsl_spline *spline = p->spline;

  return gsl_spline_eval(spline, x, accel);
}

double find_cubic_root(gsl_interp_accel *accel, gsl_spline *spline, double x0)
{
  int status;
  int iter = 0, max_iter = 2000;
  const gsl_root_fsolver_type *T;
  gsl_root_fsolver *s;
  double r = -1.0;
  double x_lo = x0 - 0.1;
  double x_hi = x0 + 0.1;
  gsl_function F;
  struct cubic_params params = {accel, spline};

  F.function = &f_cubic;
  F.params = &params;

  T = gsl_root_fsolver_brent;
  s = gsl_root_fsolver_alloc(T);
  gsl_root_fsolver_set(s, &F, x_lo, x_hi);

  do
    {
      iter++;
      status = gsl_root_fsolver_iterate(s);
      r = gsl_root_fsolver_root(s);
      x_lo = gsl_root_fsolver_x_lower(s);
      x_hi = gsl_root_fsolver_x_upper(s);
      status = gsl_root_test_interval(x_lo, x_hi,
                                       0, 0.001);
    }
  while (status == GSL_CONTINUE && iter < max_iter);

  gsl_root_fsolver_free(s);
  
  return r;
}

double time_cubic_signal(int *a)
{
    int i = 0;
    int j = 0;
    int ret = 0;

    int min_a;
    int min_a_i = 0;
    min_bubble(a, SIZEOF_SIGNAL/2, &min_a, &min_a_i);

    double x0 = time_line_signal(a);

    double baseline = 0.0;
    for (i = 0; i < 10; i++) {
        baseline += (double)a[i];
    }
    baseline = baseline/10.0;

	//halder should be off only once in a program
	gsl_set_error_handler_off();
	//

    gsl_interp_accel *accel = gsl_interp_accel_alloc();
    gsl_spline *spline = gsl_spline_alloc(gsl_interp_cspline, 4);

	double edge = baseline - 0.6*(baseline - min_a);
    double xa[4];
    double ya[4];
    for (i = 0; i <= 3; i++) {
		j = min_a_i + i - 3;
		xa[i] = (double)j;
		ya[i] = (double)(a[j] - edge);
    }
    //we are interesting in x0 position for time stamp
    ret = gsl_spline_init(spline, xa, ya, 4);
    //CHECK ret VALUE

    //printf("ret in cftrace_t = %.2f\n", x0);

    x0 = find_cubic_root(accel, spline, x0);

	//printf("ret in cubic_time = %d | x0 = %.2f\n", ret, x0);

    gsl_spline_free(spline);
    gsl_interp_accel_free(accel);

    return x0;
}
 
int calc_en_t(int *data, einfo_t *event, double (*area_f)(int *a), double (*time_f)(int *a))
{
	if ( (data == NULL) || (event == NULL) || (area_f == NULL) || (time_f == NULL) ) {
		return -1;
	}

    event->en = (*area_f)(data);
	if (fabs(event->en + 1.0) < EPS) {
		return -2;
	}

	event->t = (*time_f)(data);
	if ((event->t - SIZEOF_SIGNAL) > EPS) {
		return -3;
	}

    event->det = data[SIZEOF_SIGNAL - 1];
	if (( (event->det) < 1 ) || ( (event->det) > 4 )) { 
		return -4;
	}

	return 0;
}


//FUNCTIONS for TRANSFER DATA EVENT trough FIFO
int **create_FIFO(const char *fifo_foldername)
{
    int i, j;
	int k, l;
	int ret = -1;
	int **fds = (int **)calloc(16, sizeof(int *));
	if (fds == NULL) {
		return NULL;
	}
	for (i = 0; i < 16; i++) {
		fds[i] = (int *)calloc(4, sizeof(int));
		if (fds[i] == NULL) {
			for (j = 0; j < i; j++) {
				free(fds[j]); fds[j] = NULL;
			}
			free(fds); fds = NULL;

			return NULL;
		}
	}
	char tempstr[512] = {0};

	for (i = 0; i < 16; i++) {
		for (j = 0; j < 4; j++) {
			sprintf(tempstr, "%s/%s_%d_%d", fifo_foldername, "FIFO_TEST", i, j);
			printf("tempstr = %s\n", tempstr);
			
			ret = access(tempstr, F_OK);
			if (ret == -1) {
				ret = mkfifo(tempstr, 0666);
			}
			//in case of error in mkfifo()/mknod()
			if ( (ret == -1) ) {
				for (k = 0; k < i; k++) {
					for (l = 0; l < 4; l++) {
						close(fds[k][l]);
					}
					free(fds[k]); fds[k] = NULL;
				}
				for (l = 0; l < j; l++) {
					close(fds[i][l]);
				}
				free(fds[i]); fds[i] = NULL;
				free(fds); fds = NULL;

				return NULL;
			}

			fds[i][j] = open(tempstr, O_WRONLY);
			strncpy(tempstr, "", 512);
			//in case of errors in open()
			if ( (fds[i][j] == -1) ) {
				printf("ret = %d, fds[%d][%d] = %d\n", ret, i, j, fds[i][j]);
				for (k = 0; k < i; k++) {
					for (l = 0; l < 4; l++) {
						close(fds[k][l]);
					}
					free(fds[k]); fds[k] = NULL;
				}
				for (l = 0; l < j; l++) {
					close(fds[i][l]);
				}
				free(fds[i]); fds[i] = NULL;
				free(fds); fds = NULL;

				return NULL;
			}
		}
	}

    return fds;
}

void sigpipe_handler(int sig_num)
{
	//!!!at the working version this fprintf should be removed!!!
	fprintf(stderr, "SIGPIPE was\n");
}

int transfer_data_to_FIFO(int **fds, unsigned int **histo_en, unsigned int **start)
{
	int i, j;

	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			write(fds[i][j], &(histo_en[i][j*HIST_SIZE/4]), HIST_SIZE/4*sizeof(unsigned int));
		}
	}

	for (i = 4; i < 16; i++) {
		for (j = 0; j < 4; j++) {
			write(fds[i][j], &(start[i - 4][j*HIST_SIZE/4]), HIST_SIZE/4*sizeof(unsigned int));
		}
	}

	return 0;
}

int save_data_in_FIFO(unsigned int **histo_en, unsigned int **start)
{
	int i, j;
	sighandler_t sigh_ret = NULL;
	int **fds = create_FIFO(FIFO_FOLDERNAME);
	if (fds == NULL) {
		return -1;
	}

	//handle SIGPIPE
	sigh_ret = signal(SIGPIPE, sigpipe_handler);
	if (sigh_ret == SIG_ERR) {
		perror("Error in SIGPIPE registration");
	    
		for (i = 0; i < 16; i++) {
			for (j = 0; j < 4; j++) {
				close(fds[i][j]);
			}
			free(fds[i]); fds[i] = NULL;
		}
		free(fds); fds = NULL;
		
		return -1;
	}

	//transfer data to FIFOs
	while (flag_fifo_wr == 1) {
		transfer_data_to_FIFO(fds, histo_en, start);
	}
	close_fifo(&fds);

	return 0;
}

int close_fifo(int ***fds)
{
	int i, j;
	int ret = 0;
	
	if (fds == NULL) {
		return -1;
	}

	for (i = 0; i < 16; i++) {
		for (j = 0; j < 4; j++) {
			ret = close((*fds)[i][j]);
			if (ret != 0) {
				perror("Error in close_fifo() during closing");
			}
		}
		free((*fds)[i]); (*fds)[i] = NULL;
	}
	free(*fds); *fds = NULL;	

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


int b_definitely_greater_a(double a, double b) 
{
    return (b - a) > ( (fabs(a) < fabs(b) ? fabs(b) : fabs(a))*EPS );
}

int calc_histo(einfo_t **events, int calc_size, int en_range[][4], unsigned int **histo_en, unsigned int **start) //add size to events
{
	int i, j;
	int n1 = 0;
	int n2 = 0;
	int s1 = 0;
	int s2 = 0;
	double diff_time = 0.0;
	int dtime = 0;
	const double c = 2.0*T_SCALE[0]*T_SCALE[1]/HIST_SIZE;

	for (i = 0; i < calc_size - 2; i += 2) { //-1 or -2???
		n1 = events[i]->det;
		n2 = events[i+1]->det;

		if ( (n1 < 1) || (n1 > 4) ) {
			continue;
		}
		if ( (n2 < 1) || (n2 > 4) ) {
			continue;
		}

		s1 = (int)events[i]->en;
		s2 = (int)events[i+1]->en;

		if ( (s1 >= EN_THRESHOLD) && (s1 < HIST_SIZE) ) {
			histo_en[n1-1][s1]++;
		}
		if ( (s2 >= EN_THRESHOLD) && (s2 < HIST_SIZE) ) {
			histo_en[n2-1][s2]++;
		}

		
		if ( (s1 >= EN_THRESHOLD) && (s1 < HIST_SIZE) && (s2 >= EN_THRESHOLD) && (s2 < HIST_SIZE) ) {
			diff_time = T_SCALE[0]*(events[i]->t - events[i+1]->t) + T_SCALE[0]*T_SCALE[1];
			
			dtime = 0;
			for (j = 0; j < HIST_SIZE - 1; j++) {
				if ( (diff_time >= j*c) && (diff_time < (j + 1)*c) ) {
					dtime = j;
					break;
				}
			}

			//printf("n1 = %d, n2 = %d | s1 = %d, s2 = %d | diff_time = %.2f dtime = %d\n", n1, n2, s1, s2, diff_time, dtime);
			
			if ( (n1 == 1) && (n2 == 2) ) {
				if ( (s1 >= en_range[0][0]) && (s1 <= en_range[0][1]) && (s2 >= en_range[1][2]) && (s2 <= en_range[1][3]) ) {
					start[0][dtime]++; //T12
				}
				else if ( (s1 >= en_range[0][2]) && (s1 <= en_range[0][3]) && (s2 >= en_range[1][0]) && (s2 <= en_range[1][1]) ) {
					start[1][dtime]++; // T21
				}
			}
			else if ( (n1 == 1) && (n2 == 3) ) {
				if ( (s1 >= en_range[0][0]) && (s1 <= en_range[0][1]) && (s2 >= en_range[2][2]) && (s2 <= en_range[2][3]) ) {
					start[2][dtime]++; //T13
				}
				else if ( (s1 >= en_range[0][2]) && (s1 <= en_range[0][3]) && (s2 >= en_range[2][0]) && (s2 <= en_range[2][1]) ) {
					start[3][dtime]++; //T31
				}
			}
			else if ( (n1 == 1) && (n2 == 4) ) {
				if ( (s1 >= en_range[0][0]) && (s1 <= en_range[0][1]) && (s2 >= en_range[3][2]) && (s2 <= en_range[3][3]) ) {
					start[4][dtime]++; //T14
				}
				else if ( (s1 >= en_range[0][2]) && (s1 <= en_range[0][3]) && (s2 >= en_range[3][0]) && (s2 <= en_range[3][1]) ) {
					start[5][dtime]++; //T41
				}
			}
			else if ( (n1 == 2) && (n2 == 3) ) {
				if ( (s1 >= en_range[1][0]) && (s1 <= en_range[1][1]) && (s2 >= en_range[2][2]) && (s2 <= en_range[2][3]) ) {
					start[6][dtime]++; //T23
				}
				else if ( (s1 >= en_range[1][2]) && (s1 <= en_range[1][3]) && (s2 >= en_range[2][0]) && (s2 <= en_range[2][1]) ) {
					start[7][dtime]++; //T32
				}
			}
			else if ( (n1 == 2) && (n2 == 4) ) {
				if ((s1 >= en_range[1][0]) && (s1 <= en_range[1][1]) && (s2 >= en_range[3][2]) && (s2 <= en_range[3][3]) ) {
					start[8][dtime]++; //T24
				}
				else if ( (s1 >= en_range[1][2]) && (s1 <= en_range[1][3]) && (s2 >= en_range[3][0]) && (s2 <= en_range[3][1]) ) {
					start[9][dtime]++; //T42
				}
			}
			else if ( (n1 == 3) && (n2 == 4) ) {
				if ( (s1 >= en_range[2][0]) && (s1 <= en_range[2][1]) && (s2 >= en_range[3][2]) && (s2 <= en_range[3][3]) ) {
					start[10][dtime]++; //T34
				}
				else if ( (s1 >= en_range[2][2]) && (s1 <= en_range[2][3]) && (s2 >= en_range[3][0]) && (s2 <= en_range[3][1]) ) {
					start[11][dtime]++; //T43
				}
			}
		}
	}

	return 0;
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

void close_files_for_histo(int **out_histo_fd)
{
	int i;

	if (*out_histo_fd == NULL) {
		return ;
	}
	
	for (i = 0; i < 16; i++) {
		close((*out_histo_fd)[i]);
	}
	free(*out_histo_fd); *out_histo_fd = NULL;
}

void print_histo_test(unsigned int *histo)
{
	int i = 0;

	for (i = 2000; i < 3000; i++) {
		printf("%u ", histo[i]);
	}
	printf(" \n");
}

int save_histo_in_file(const int *out_fd, unsigned int **histo_en, unsigned int **start)
{
	int i;
	int res = 0;

	for (i = 0; i < 16; i++) {
		if (out_fd[i] == -1) {
			#ifdef DEBUG
			fprintf(stderr, "Error in file descriptor #%d\n", out_fd[i]);
			#endif

			return -1;
		}
	}
	
	for (i = 0; i < 4; i++) {
		lseek(out_fd[i], 512, 0);
		res = write(out_fd[i], histo_en[i], HIST_SIZE*sizeof(unsigned int));
		if (res != HIST_SIZE*sizeof(unsigned int)) {
			#ifdef DEBUG
			perror("");
			#endif

			return -1;
		}
	}

	for (i = 4; i < 16; i++) {
		//#ifdef DEBUG
		//print_histo_test(start[i - 4]);
		//#endif

		lseek(out_fd[i], 512, 0);
		res = write(out_fd[i], start[i - 4], HIST_SIZE*sizeof(unsigned int));
		if (res != HIST_SIZE*sizeof(unsigned int)) {
			#ifdef DEBUG
			perror("");
			#endif

			return -1;
		}
	}

	return 0;
}

int save_histo_in_ascii(const char *foldername, unsigned int **histo_en, unsigned int **start)
{
	int i, j;
	int ret = 0;
	struct stat st = {0};
	FILE *out_file[16];
	char *tempstr = (char *)calloc(512, sizeof(char));
	if (tempstr == NULL) {
		return -1;
	}

	if ( (foldername == NULL) || strlen(foldername) == 0 ) {
		return -1;
	}
	
	ret = stat(foldername, &st);
    if (ret == -1) {
		#ifdef DEBUG
		fprintf(stderr, "stat(%s) returns -1\n", foldername);
		#endif
        ret = mkdir(foldername, 0777);
		if (ret == -1) {
			perror("");
			free(tempstr); tempstr = NULL;

			return -1;
		}
    }

	for (i = 0; i < 4; i++) {
		sprintf(tempstr, "%s/en%d", foldername, i);
		out_file[i] = fopen(tempstr, "w+");
		if (out_file[i] == NULL) {
			perror("");
			free(tempstr); tempstr = NULL;

			return -1;
		}
		strncpy(tempstr, "", 512);
	}

	const char *t_file_names[] = {"t12", "t21", "t13", "t31", "t14", "t41", "t23", "t32", "t24", "t42", "t34", "t43"};
	for (i = 4; i < 16; i++) {
		//sprintf(tempstr, "%s/t%d", foldername, i - 4);
		sprintf(tempstr, "%s/%s", foldername, t_file_names[i-4]);
		out_file[i] = fopen(tempstr, "w+");
		if (out_file[i] == NULL) {
			perror("");
			free(tempstr); tempstr = NULL;

			return -1;
		}
		strncpy(tempstr, "", 512);
	}

	for (i = 0; i < 4; i++) {
		for (j = 0; j < HIST_SIZE; j++) {
			fprintf(out_file[i], "%d %u\n", j, histo_en[i][j]);
		}
	}

	for (i = 4; i < 16; i++) {
		for (j = 0; j < HIST_SIZE; j++) {
			fprintf(out_file[i], "%d %u\n", j, start[i - 4][j]);
		}
	}

	for (i = 0; i < 16; i++) {
		fclose(out_file[i]);
	}
	free(tempstr); tempstr = NULL;

	return 0;
}

FILE *open_file_EbE(const char *filename)
{
	FILE *fd = NULL;

	if ( (filename == NULL) || strlen(filename) == 0 ) {
		return NULL;
	}

	fd = fopen(filename, "w+");

	return fd;
}

int fill_EbE(FILE *fd, einfo_t **events, int calc_size)
{
	int i, j;
	double diff_time = 0.0;
	int dtime = 0;
	const double c = 2.0*T_SCALE[0]*T_SCALE[1]/HIST_SIZE;

	unsigned char *x = (unsigned char *)calloc(sizeof(unsigned char), 4*calc_size);
	if (x == NULL) {
		#ifdef DEBUG
		printf("error while calloc x in %s\n", __func__);
		#endif

		return -1;
	}

	for (i = 0; i <= calc_size/2 - 1; i += 1) {
		//dt = t[i+1] - t[i] | [dt] = true histogram chs 
		diff_time = T_SCALE[0]*(events[2*i+1]->t - events[2*i]->t) + T_SCALE[0]*T_SCALE[1];
		dtime = 0;

		for (j = 0; j < HIST_SIZE - 1; j++) {
			if ( (diff_time > j*c) && (diff_time < (j + 1)*c) ) {
				dtime = j;

				break;
			}
		}

		x[8*i+1] = dtime / 256;
		x[8*i+0] = dtime - 256*x[8*i+1];

		//EN 1st det
		x[8*i+3] = events[2*i]->en / 256;
		x[8*i+2] = events[2*i]->en - 256*x[8*i+3];
		//EN 2nd det
		x[8*i+5] = events[2*i+1]->en / 256;
		x[8*i+4] = events[2*i+1]->en - 256*x[8*i+5];
		//Det nums
		x[8*i+6] = events[2*i]->det;
		x[8*i+7] = events[2*i+1]->det;
	}
	fwrite(x, sizeof(unsigned char), 4*calc_size, fd);
	//check fwrite return bytes
	/*if ( ret < (int)(calc_size*sizeof(int)) ) {
		#ifdef DEBUG
		printf("ret = %d (should be %d)\n", ret, (int)(calc_size*sizeof(int)));
		#endif

		free(x);

		return -1;
	}
	*/
	free(x);

	printf("calc size in %s = %d\n", __func__, calc_size);

	return 0;
}


int alloc_mem_data(int ***data)
{
	int i, j;

	if (*data != NULL) {
		return -1;
	}

	*data = (int **)calloc(4, sizeof(int *));
	if (*data == NULL) {
		fprintf(stderr, "Error in memory allocation for int **data\n");
		return -1;
	}
	
	for (i = 0; i < 4; i++) {
		(*data)[i] = (int *)calloc(SIZEOF_SIGNAL, sizeof(int));
		if ((*data)[i] == NULL) {
			for (j = 0; j < i; j++) {
				free((*data)[j]); (*data)[j] = NULL;
			}
			free(*data); *data = NULL;
			
			fprintf(stderr, "Error in memory allocation for int *data[%d]\n", i);
			return -1;
		}
	}

	return 0;
}

void free_mem_data(int ***data)
{
	int i;

	if (data == NULL) {
		return ;
	}

	for (i = 0; i < 4; i++) {
		if ((*data)[i] == NULL) {
			return ;
		}

		free((*data)[i]); (*data)[i] = NULL;
	}
	free(*data); *data = NULL;
}

int alloc_mem_events(einfo_t ***events)
{
	int i, j;

	if (*events != NULL) {
		return -1;
	}

	*events = (einfo_t **)calloc(CALC_SIZE, sizeof(einfo_t *));
	if (*events == NULL) {
		return -1;
	}

	for (i = 0; i < CALC_SIZE; i++) {
		(*events)[i] = (einfo_t *)malloc(sizeof(einfo_t));
		if ((*events)[i] == NULL) {
			for (j = 0; j < i; j++) {
				free((*events)[j]); (*events)[j] = NULL;
			}
			free(*events); *events = NULL;

			fprintf(stderr, "Error in memory allocation for int *events[%d]\n", i);

			return -1;
		}
	}

	return 0;
}

void free_mem_events(einfo_t ***events)
{
	int i;

	if (*events == NULL) {
		return ;
	}

	for (i = 0; i < CALC_SIZE; i++) {
		free((*events)[i]); (*events)[i] = NULL;
	}
	free(*events); *events = NULL;
}

int alloc_mem_histo(unsigned int ***histo_en, unsigned int ***start)
{
	int i, j;

	if ( (*histo_en != NULL) || (*start != NULL) ) {
		return -1;
	}

	*histo_en = (unsigned int **)calloc(4, sizeof(unsigned int *));
	if (*histo_en == NULL) {
		fprintf(stderr, "Error in memory allocation for **histo_en\n");

		return -1;
	}
	for (i = 0; i < 4; i++) {
		(*histo_en)[i] = (unsigned int *)calloc(HIST_SIZE, sizeof(unsigned int));
		if ((*histo_en)[i] == NULL) {
			for (j = 0; j < i; j++) {
				free((*histo_en)[j]); (*histo_en)[j] = NULL;
			}
			free(*histo_en); *histo_en = NULL;

			return -1;
		}
	}

	*start = (unsigned int **)calloc(12, sizeof(unsigned int *));
	if (*start == NULL) {
		for (j = 0; j < 4; j++) {
			free((*histo_en)[j]); (*histo_en)[j] = NULL;
		}
		free(*histo_en); *histo_en = NULL;

		fprintf(stderr, "Error in memory allocation for **start\n");
		
		return -1;
	}
	for (i = 0; i < 12; i++) {
		(*start)[i] = (unsigned int *)calloc(HIST_SIZE, sizeof(unsigned int));
		if ((*start)[i] == NULL) {
			for (j = 0; j < 4; j++) {
				free((*histo_en)[j]); (*histo_en)[j] = NULL;
			}
			free(*histo_en); *histo_en = NULL;

			for (j = 0; j < i; j++) {
				free((*start)[j]); (*start)[j] = NULL;
			}
			free(*start); *start = NULL;

			fprintf(stderr, "Error in memory allocation for *start[%d]\n", i);

			return -1;
		}
	}

	return 0;
}

void free_mem_histo(unsigned int ***histo_en, unsigned int ***start)
{
	int i;

	if (*histo_en != NULL) {
		for (i = 0; i < 4; i++) {
			free((*histo_en)[i]); (*histo_en)[i] = NULL;
		}
		free(*histo_en); *histo_en = NULL;
	}

	if (*start != NULL) {
		for (i = 0; i < 12; i++) {
			free((*start)[i]); (*start)[i] = NULL;
		}
		free(*start); *start = NULL;
	}
}
