/** 
	@file dsp.h
	@brief Functions prototypes and structures for DSP C library.

	@bug No known bugs.
*/

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

#include <libusb-1.0/libusb.h>
#include <cyusb.h>


typedef struct {
	double en;
	double t;
	char det;
} einfo_t;


extern const int SIZEOF_SIGNAL;
extern const int HIST_SIZE;
extern const int CALC_SIZE;
extern double Tau_trap;
extern unsigned int K_trap;
extern unsigned int L_trap;
extern double EN_normal;
extern double CFT_fraction;
extern const char *FIFO_FOLDERNAME;

//functions for manipulation with USB controller
/**
   @brief Initialize Cypress USB controller
   @param usb_h pointer to usb device handle of initialized USB controller

   @retval -1 On error. Error in initialisation. So you can't read data from USB controler
   @retval 0 On success
*/

int init_controller(cyusb_handle **usb_h);
/**
   @brief Save unitialize Cypress USB controller
   @param usb_h usb device handle

   @retval 0 On success
*/

int exit_controller(cyusb_handle *usb_h);
/**
   @brief Send specific command to FPGA throught Cypress USB controller
   @param usb_h usb device handle

   @retval -1 On error. Error in transfer only detected
   @retval 0 On success
*/
int control_send_comm(cyusb_handle *usb_h, const char *comm, int args);

/**
   @brief Read data from endpoint IN_EP
   @param usb_h usb device handle
   @param data pointer to the buffer which will contain the read data or NULL in which case the data buffer will be allocated in the function
   
   @retval NULL On error. 
   @retval pointer to the data buffer with data from endpoint. It should be freed 
*/
int **read_data_ep(cyusb_handle *usb_h, int **data);


//functions for DSP
/**
   @brief Calculate the area of the signal by trapezoidal filter. K_trap and L_trap are parameters of the filter.
   @param a an array with signal

   @retval -1.0 On error.
   @retval abs(area)*EN_normal the area of the signal
 */

double area_trap_signal(int *a);
/**
   @brief Calculate the are of the signal by simple integration. INTEGRAL_steps_{back,forw} are parameters: steps before min of the signal and steps after. 
   @param a an array with signal

   @retval -1.0 On error.
   @retval abs(area)*EN_normal the area of the signal
 */
double area_integral(int *a);

/**
   @brief Simple bubble search of the minimum value and index in the array.
   @param a an array
   @param len the length of the array to search
   @param min the value of the min
   @param min_num the index of the min value

   @retval void
 */
void min_bubble(int *a, int len, int *min, int *min_num);

/**
   @brief DCFD algorithm for searching the time of the signal. CFT_fraction is parameter of CFD.
   @param a an array with signal

   @retval -1.0 On error. The function can't find the time.
   @retval value of the time
 */
double time_line_signal(int *a);

/**
   @brief Calculation of the energy and time of the signal.
   @param data an array with signal. The size is equal to SIZEOF_SIGNAL.
   @param event the result of calculation and det number.
   @param area_f the function for calculation the energy.
   @param time_f the function for time calculation.

   @retval -1 On error.
   @retval 0 On success.
 */
int calc_en_t(int *data, einfo_t *event, double (*area_f)(int *a), double (*time_f)(int *a));


//functions for saving
/**
   @brief Save data of signals in file in binary format.
   @param out_fd the file descriptor of the output file.
   @param data the pointer to data with 4 signals.
   
   @retval -1 On error.
   @retval 0 On success.
 */
int save_data_in_file(const int out_fd, int **data);

/**
   @brief Save EN and T histograms in binary format.
   @param out_fd an array of file descriptors for output files with histos.
   @param histo_en the pointer to the EN histo of 4 detectors spectrum.
   @param start the pointer to the T histo of 12 coincidence spectra.

   @retval -1 On error. If DEBUG compilation flag is defined then error information should be printed in stderr.
   @retval 0 On success.
 */
int save_histo_in_file(const int *out_fd, unsigned int **histo_en, unsigned int **start);
int *open_files_for_histo(const char *foldername);
void close_files_for_histo(int **out_histo_fd);
int save_histo_in_ascii(const char *foldername, unsigned int **histo_en, unsigned int **start);

//FUNCTIONS for TRANSFER DATA EVENT trough FIFO
int **create_FIFO(const char *fifo_foldername);
int transfer_data_to_FIFO(int **fds, unsigned int **histo_en, unsigned int **start);
int save_data_in_FIFO(unsigned int **histo_en, unsigned int **start);
int close_fifo(int ***fds);

//function for histo calculation
int calc_histo(einfo_t **events, const int en_range[][4], unsigned int **histo_en, unsigned int **start);

//functions for working with memory
int alloc_mem_data(int ***data);
void free_mem_data(int ***data);
int alloc_mem_events(einfo_t ***events);
void free_mem_events(einfo_t ***events); 
int alloc_mem_histo(unsigned int ***histo_en, unsigned int ***start);
void free_mem_histo(unsigned int ***histo_en, unsigned int ***start);

#endif
