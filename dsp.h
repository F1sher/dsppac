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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

#include <libusb-1.0/libusb.h>
#include <cyusb.h>
#include <sclog4c/sclog4c.h>

//defines to work with cyusb_linux 1.0.5
typedef libusb_device_handle cyusb_handle;
#define cyusb_kernel_driver_active libusb_kernel_driver_active
#define cyusb_claim_interface libusb_claim_interface
#define cyusb_clear_halt libusb_clear_halt
#define cyusb_bulk_transfer libusb_bulk_transfer
#define cyusb_control_transfer libusb_control_transfer

typedef struct {
	double en;
	double t;
	char det;
} einfo_t;

typedef struct {
	int get_flag;
	int counts;
	int d_counts;
} intens_t;


extern const int SIZEOF_SIGNAL;
extern const int HIST_SIZE;
extern const int CALC_SIZE;
//CALC_TIME in seconds
extern const int CALC_TIME;
extern const int ZMQ_READ_TIME;

extern const int DET_NUM;

extern double EN_normal;
extern double Tau_trap;
extern unsigned int K_trap;
extern unsigned int L_trap;
extern unsigned int I_MIN_S_SHIFT_trap;
extern unsigned int AVERAGE_trap;
extern int INTEGRAL_steps_back;
extern int INTEGRAL_steps_forw;

extern double CFT_fraction;
extern double T_SCALE[2];

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

/**
   @brief Get counts in detectors
   @param data the pointer to the data from 4 detectors
   @param intens the struct with intensities values. intens.d_counts = intens1 (current intensity) - intens0 (prev intensity). get_flag ...
   @param count_flag flag for the function. IF count_flag == 0 -> intens0 value get. IF count_flag == 1 -> calc d_counts.

   @retval -1 On error.
   @retval 0 On success.
*/
int get_det_counts(int **data, intens_t intens[], int count_flag);


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
 */
double time_cubic_signal(int *a);

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
   @param out_fd an array of file descriptors for output files with histos. Should be returned from the function open_files_for_histo()
   @param histo_en the pointer to the EN histo of 4 detectors spectrum.
   @param start the pointer to the T histo of 12 coincidence spectra.

   @retval -1 On error. If DEBUG compilation flag is defined then error information should be printed in stderr.
   @retval 0 On success.
 */
int save_histo_in_file(const int *out_fd, unsigned int **histo_en, unsigned int **start);

/**
   @brief Open files for EN and T histograms. Should be closed when unused.
   @param foldername the folder name for files. If folder does not exist it will be created in the function.

   @see close_files_for_histo()

   @retval NULL on error.
   @retval pointer to file descriptors of files.
 */
int *open_files_for_histo(const char *foldername);

/**
   @brief Close file descriptors returned by open_files_for_histo().
   @param out_histo_fd pointer to the array of file descriptors which will be closed.

   @retval void
 */
void close_files_for_histo(int **out_histo_fd);

/**
   @brief Function to save histo in ascii format. These files can be viewed in gnuplot.
   @param foldername Folder name for histo files.
   @param histo_en EN histograms.
   @param start T histograms.

   @retval -1 On error.
   @retval 0 On success.
 */
int save_histo_in_ascii(const char *foldername, unsigned int **histo_en, unsigned int **start);

FILE *open_file_EbE(const char *filename);
int fill_EbE(FILE *fd, einfo_t **events, int calc_size);

//FUNCTIONS for TRANSFER DATA EVENT trough FIFO
/**
   @brief Creates FIFO files for online mode. Should be closed when unused.
   @param fifo_foldername folder name for fifo files. 

   @see close_fifo()

   @retval -1 On error. errno could be set too.
   @retval 0 On success
 */
int **create_FIFO(const char *fifo_foldername);

/**
   @brief write data to fifo
   @param fds fifo's file descriptors.
   @param histo_en EN histograms.
   @param start T histograms.

   @retval 0 In all cases.
 */
int transfer_data_to_FIFO(int **fds, unsigned int **histo_en, unsigned int **start);

/**
   @brief create and transfer data throught fifo. Can be used for test.
 */
int save_data_in_FIFO(unsigned int **histo_en, unsigned int **start);

/**
   @brief close fifo files. Close fifo opened by create_FIFO()
   @param fds pointer to double dimension array of fifo's file descriptors

   @retval -1 On error.
   @retval 0 On success.
 */
int close_fifo(int ***fds);

//function for histo calculation
int calc_histo(einfo_t **events, int calc_size, int en_range[][4], unsigned int **histo_en, unsigned int **start);

//functions for working with memory
int alloc_mem_data(int ***data);
void free_mem_data(int ***data);
int alloc_mem_events(einfo_t ***events);
void free_mem_events(einfo_t ***events); 
int alloc_mem_histo(unsigned int ***histo_en, unsigned int ***start);
void free_mem_histo(unsigned int ***histo_en, unsigned int ***start);

#endif
