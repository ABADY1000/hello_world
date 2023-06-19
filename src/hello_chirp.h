/*
 * hello_chirp.h - header file for Hello Chirp! example application 
 */

/*
Copyright © 2019, Chirp Microsystems.   All rights reserved.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL CHIRP MICROSYSTEMS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

You can contact the authors of this program by email at support@chirpmicro.com
or by mail at 2560 Ninth Street, Suite 220, Berkeley, CA 94710.
*/

#ifndef __HELLO_CHIRP_H
#define __HELLO_CHIRP_H

/*
 *   This file contains various definitions used in the hello_chirp.c
 *   application.
 *
 *   All symbols and objects defined in this file are specific to the 
 *   hello_chirp application, and are not required by SonicLib or other 
 *   Chirp interfaces.  You are free to modify this file, change names,
 *   etc. as necessary.
 */

/* Includes */
#include "soniclib.h"				// SonicLib API functions
#include "chirp_board_config.h"		// counts etc. from board support package

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <complex.h>

/* Hello Chirp application version */
#define	APP_VERSION_MAJOR	1		// major version
#define APP_VERSION_MINOR	0		// minor version
#define APP_VERSION_REV 	4		// revision


/*========================= Sensor Firmware Selection ===========================*/

/* Select sensor firmware to use 
 *   The sensor firmware is specified during the call to ch_init(), by
 *   giving the name (address) of the firmware initialization function 
 *   that will be called.
 *
 *   Uncomment ONE of the following lines to use that sensor firmware type.
 *   Note that you must choose a firmware image that is appropriate for the 
 *   sensor model you are using (CH101 or CH201).  
 *
 *   To use a different sensor firmware type (e.g. a new distribution from
 *   Chirp), simply define CHIRP_SENSOR_FW_INIT_FUNC to equal the name of
 *   the init routine for the new firmware.
 */

#define	 CHIRP_SENSOR_FW_INIT_FUNC	ch101_gpr_open_init		/* CH101 GPR OPEN firmware */
//#define	 CHIRP_SENSOR_FW_INIT_FUNC	ch201_gprmt_init	/* CH201 GPR Multi-Threshold firmware */

/* SHORT RANGE OPTION:
 *   Uncomment the following line to use different CH101 sensor f/w optimized for 
 *   short range. The short range firmware has 4 times the resolution, but 
 *   only 1/4 the maximum range.  If you use this option, you should redefine 
 *   the CHIRP_SENSOR_MAX_RANGE_MM symbol, below, to 250mm or less.
 */
// #define	USE_SHORT_RANGE			/* use short-range firmware */

#ifdef USE_SHORT_RANGE
#undef	 CHIRP_SENSOR_FW_INIT_FUNC
#define	 CHIRP_SENSOR_FW_INIT_FUNC		ch101_gpr_sr_open_init	/* CH101 GPR SR OPEN firmware (short range) */
#endif


/*============================ Sensor Configuration =============================*/

/* Define configuration settings for the Chirp sensors 
 *   The following symbols define configuration values that are used to 
 *   initialize the ch_config_t structure passed during the ch_set_config() 
 *   call.  
 */
#define	CHIRP_SENSOR_MAX_RANGE_MM		750	/* maximum range, in mm */

#define	CHIRP_SENSOR_STATIC_RANGE		0	/* static target rejection sample 
											   range, in samples (0=disabled) */
#define CHIRP_SENSOR_SAMPLE_INTERVAL	0	/* internal sample interval - 
											   NOT USED IF TRIGGERED */


/*============================= Application Timing ==============================*/

/* Define how often the application will get a new sample from the sensor(s) 
 *   This macro defines the sensor sample interval, in milliseconds.  The 
 *   application will use a timer to trigger a sensor measurement after 
 *   this period elapses.
 */
#define	MEASUREMENT_INTERVAL_MS		100		// 100ms interval = 10Hz sampling


/*===================  Application Storage for Sensor Data ======================*/

/* Define how many I/Q samples are expected by this application
 *   The following macro is used to allocate space for I/Q data in the 
 *   "chirp_data_t" structure, defined below.  Because a Chirp CH201 sensor 
 *   has more I/Q data than a CH101 device, the CH201 sample count is used 
 *   here.
 *   If you are ONLY using CH101 devices with this application, you may 
 *   redefine the following define to equal CH101_MAX_NUM_SAMPLES to use 
 *   less memory.
 */
#define IQ_DATA_MAX_NUM_SAMPLES  CH201_MAX_NUM_SAMPLES	// use CH201 I/Q size

/* chirp_data_t - Structure to hold measurement data for one sensor
 *   This structure is used to hold the data from one measurement cycle from 
 *   a sensor.  The data values include the measured range, the ultrasonic 
 *   signal amplitude, the number of valid samples (I/Q data pairs) in the 
 *   measurement, and the raw I/Q data from the measurement.
 *
 *  The format of this data structure is specific to this application, so 
 *  you may change it as desired.
 *
 *  A "chirp_data[]" array of these structures, one for each possible sensor, 
 *  is declared in the hello_chirp.c file.  The sensor's device number is 
 *  used to index the array.
 */
typedef struct {
	uint32_t		range;							// from ch_get_range()
	uint16_t		amplitude;						// from ch_get_amplitude()
	uint16_t		num_samples;					// from ch_get_num_samples()
	ch_iq_sample_t	iq_data[IQ_DATA_MAX_NUM_SAMPLES];	// from ch_get_iq_data()
} chirp_data_t;

extern chirp_data_t	chirp_data[];


/*===================  Build Options for I/Q Data Handling ======================*/

/* The following build options control if and how the raw I/Q data is read 
 * from the device after each measurement cycle, in addition to the standard 
 * range and amplitude.  Comment or un-comment the various definitions, as 
 * appropriate.
 *
 * Note that reading the I/Q data is not required for most basic sensing 
 * applications - the reported range value is typically all that is required. 
 * However, the full data set may be read and analyzed for more advanced 
 * sensing needs.
 *
 * By default, this application will read the I/Q data in blocking mode 
 * (i.e. READ_IQ_DATA_BLOCKING is defined by default).  The data will be read 
 * from the device and placed in the I/Q data array field in application's 
 * chirp_data structure.
 *
 * Normally, the I/Q data is read in blocking mode, so the call to 
 * ch_get_iq_data() will not return until the data has actually been read from 
 * the device.  However, if READ_IQ_DATA_NONBLOCK is defined instead, the I/Q 
 * data will be read in non-blocking mode. The ch_get_iq_data() call will 
 * return immediately, and a separate callback function will be called to 
 * notify the application when the read operation is complete.
 *
 * Finally, if OUTPUT_IQ_DATA_CSV is defined, the application will write the 
 * I/Q data bytes out through the serial port in ascii form as comma-separated 
 * numeric value pairs.  This can make it easier to take the data from the 
 * application and analyze it in a spreadsheet or other program.
 */


#define READ_IQ_DATA_BLOCKING		/* define for blocking I/Q data read */
// #define READ_IQ_DATA_NONBLOCK	/* define for non-blocking I/Q data read */

// #define OUTPUT_IQ_DATA_CSV		/* define to output I/Q data in CSV format*/


#endif /* __HELLO_CHIRP_H */

/*******  END OF FILE hello_chirp.h  --  Copyright © Chirp Microsystems ******/
