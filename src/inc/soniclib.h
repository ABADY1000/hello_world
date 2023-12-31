/*! \file soniclib.h
 *
 * \brief Chirp SonicLib public API and support functions for Chirp ultrasonic sensors.
 * 
 * Chirp SonicLib is a set of API functions and sensor driver routines designed to easily 
 * control Chirp ultrasonic sensors from an embedded C application.  It allows an
 * application developer to obtain ultrasonic range data from one or more devices, without
 * needing to develop special low-level code to interact with the sensors directly.
 *
 * The SonicLib API functions provide a consistent interface for an application to use
 * Chirp sensors in various situations.  This is especially important, because
 * all Chirp sensors are completely programmable, including the register map.  The
 * SonicLib interfaces allow an application to use new Chirp sensor firmware images,
 * without requiring code changes.  Only a single initialization parameter must be modified
 * to use the new sensor firmware.
 * 
 * \note All operation of the sensor is controlled through the set of functions, data structures,
 * and symbolic values defined in this header file.  You should not need to modify this file 
 * or the SonicLib functions, or use lower-level internal functions such as described in 
 * the ch_driver.h file.  Using any of these non-public methods will reduce your ability to 
 * benefit from future enhancements and releases from Chirp.
 * 
 * 
 * #### Board Support Package
 * SonicLib also defines a set of board support package (BSP) functions that must be 
 * provided by the developer, board vendor, or Chirp.  The BSP functions are NOT part of
 * SonicLib - they are external interface routines that allow the SonicLib functions
 * to access the peripherals on the target board.  These functions, which all begin with
 * a "chbsp_" prefix, are described in the chirp_bsp.h header file.  See the descriptions
 * in that file for more detailed information on the BSP interfaces.
 *
 * The BSP also provides the required \a chirp_board_config.h header file, which contains 
 * definitions of how many (possible) sensors and I2C buses are present on the board.  These 
 * values are used for static array allocations in SonicLib.
 * 
 * 
 * #### Basic Operating Sequence
 * At a high level, an application using SonicLib will do the following:
 *  -# Initialize the hardware on the board, by calling the BSP's \a chbsp_board_init() function.
 *  -# Initialize the SonicLib data structures, by calling \a ch_init() for each sensor.
 *  -# Program and start the sensor(s), by calling \a ch_group_start().
 *  -# Set up a handler function to process interrupts from the sensor.
 *  -# Set up a triggering mechanism using a board timer, using \a chbsp_periodic_timer_init() etc.,
 *  (unless the sensor will be used in free-running mode, in which no external trigger is needed).  A
 *  timer handler routine will typically trigger the sensor(s) using \a ch_group_trigger().
 *  -# Configure the sensor's operating mode and range, using \a ch_set_config() (or equivalent
 *  single-setting functions).
 *
 * At this point, the sensor will begin to perform measurements.  At the end of each
 * measurement cycle, the sensor will interrupt the host controller using its INT line.
 * The handler routine set up in step #4 above will be called, and it should cause the
 * application to read the measurement results from the sensor(s), using \a ch_get_range()
 * and optionally \a ch_get_amplitude() and/or \a ch_get_iq_data().
 * 
 * Do not trigger a new measurement until the previous measurement has completed and all needed
 * data has been read from the device (including I/Q data, if \a ch_get_iq_data() is used).  If any
 * I/O operations are still active, the new measurement may be corrupted.
 */

/*
 Copyright � 2019, Chirp Microsystems.  All rights reserved.

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

#ifndef __SONICLIB_H_
#define __SONICLIB_H_

/* Preliminary structure type definitions to resolve include order */
typedef struct ch_dev_t ch_dev_t;
typedef struct ch_group_t ch_group_t;


/*==============  Chirp header files for installed sensor firmware packages ===================
 *
 *   If you are installing a new Chirp sensor firmware package, you must add
 *   the name of the firmware include file to the list below.
 */

#include "ch101_gpr_open.h"			/* General purpose rangefinding - CH101 */
#include "ch101_gpr_sr_open.h"		/* General purpose rangefinding (short range) - CH101 */
#include "ch201_gprmt.h"			/* General purpose rangefinding (multi-threshold) - CH201 */

/*======================== End of sensor firmware header files ================================*/


/* Miscellaneous header files */
//#define CHDRV_DEBUG				// uncomment this line to enable driver debug messages
#include "chirp_board_config.h"		/* Header from board support package containing h/w params */
#include "ch_driver.h"				/* Internal Chirp driver defines */
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#ifdef CHDRV_DEBUG
#include <stdio.h>
#endif

/* SonicLib API/Driver version */
#define	SONICLIB_VER_MAJOR		(2)		/*!< SonicLib major version. */
#define	SONICLIB_VER_MINOR		(0)		/*!< SonicLib minor version. */
#define	SONICLIB_VER_REV		(2)		/*!< SonicLib revision. */

/* Chirp sensor part numbers */
#define	CH101_PART_NUMBER			(101)	/*!< Integer sensor identifier for CH101. */
#define	CH201_PART_NUMBER			(201)	/*!< Integer sensor identifier for CH201. */

/* Max expected number of samples per measurement (actual value depends on sensor f/w used) */
#define CH101_MAX_NUM_SAMPLES		(150)	/*!< Max expected samples per measurement for CH101. */
#define CH201_MAX_NUM_SAMPLES		(450)	/*!< Max expected samples per measurement for CH201. */

/* Misc definitions */
#define CH_NO_TARGET		(0xFFFFFFFF)	/*!< Range value returned if no target was detected. */

#define CH_I2C_ADDR_PROG	(0x45) 			/*!< I2C address of sensor programming interface. */
#define CH_SIG_BYTE_0		(0x0a)			/*!< Signature byte in sensor (1 of 2). */
#define CH_SIG_BYTE_1		(0x02)			/*!< Signature byte in sensor (2 of 2). */

#define CH_NUM_THRESHOLDS	(6)				/*!< Number of internal detection thresholds (CH201 only). */
#define CH_SPEEDOFSOUND_MPS (343) 			/*!< Speed of sound, in meters per second. */

//! Return value codes.
typedef enum {
	RET_OK 	  = 0,
	RET_ERR   = 1
} ch_retval;

//! Range data types.
typedef enum {
	CH_RANGE_ECHO_ONE_WAY 		= 0,		/*!< One way - gets full pulse/echo distance & divides by 2. */
	CH_RANGE_ECHO_ROUND_TRIP	= 1,		/*!< Round trip - full pulse/echo distance. */
	CH_RANGE_DIRECT				= 2,		/*!< Direct - for receiving node in pitch-catch mode. */
} ch_range_t;

//! Sensor operating modes.
typedef enum {
	CH_MODE_IDLE 	  			= 0x00,		/*!< Idle mode - low-power sleep, no sensing is enabled. */
	CH_MODE_FREERUN   			= 0x02,		/*!< Free-running mode - sensor uses internal clock to wake 
											     and measure. */
	CH_MODE_TRIGGERED_TX_RX 	= 0x10,		/*!< Triggered transmit/receive mode - transmits and receives 
											     when INT line triggered.*/
	CH_MODE_TRIGGERED_RX_ONLY   = 0x20		/*!< Triggered receive-only mode - for pitch-catch operation 
											     with another sensor. */
} ch_mode_t;

//! Sensor reset types.
typedef enum {
	CH_RESET_HARD     			= 0,		/*!< Hard reset. */
	CH_RESET_SOFT 	  			= 1			/*!< Soft reset. */
} ch_reset_t;

//! I/O blocking mode flags.
typedef enum {
	CH_IO_MODE_BLOCK   			= 0,		/*!< Blocking mode. */
	CH_IO_MODE_NONBLOCK     	= 1			/*!< Non-blocking mode. */
} ch_io_mode_t;

//! I2C info structure.
typedef struct {
	uint8_t		address;		/*!< I2C device address */
	uint8_t		bus_num;		/*!< I2C bus index */
	uint16_t	drv_flags;		/*!< flags for special handling by Chirp driver */
} ch_i2c_info_t;

/*!< Flags for special I2C handling by Chirp driver (drv_flags field in ch_info_t struct) */
#define	I2C_DRV_FLAG_RESET_AFTER_NB		(0x00000001)	/*!< I2C interface needs reset after non-blocking transfer. */
#define	I2C_DRV_FLAG_USE_PROG_NB		(0x00000002)	/*!< Use programming interface for non-blocking transfer. */


//! Sensor I/Q data value.
typedef struct {
	int16_t q;										/*!< Q component of sample */
	int16_t i;										/*!< I component of sample */
} ch_iq_sample_t;


//! Detection threshold value (CH201 only).
typedef struct {
	uint16_t		start_sample;
	uint16_t		level;
} ch_thresh_t;


//! Multiple detection threshold structure (CH201 only).
typedef struct {
	ch_thresh_t		threshold[CH_NUM_THRESHOLDS];
} ch_thresholds_t;

//! Combined configuration structure.
typedef struct {
	ch_mode_t		mode;				/*!< operating mode */
	uint16_t		max_range;			/*!< maximum range, in mm */
	uint16_t		static_range;		/*!< static target rejection range, in mm (0 if unused) */
	uint16_t 		sample_interval;	/*!< sample interval, only used if in free-running mode */
	ch_thresholds_t *thresh_ptr;		/*!< ptr to detection thresholds structure (if supported), should be NULL (0) for CH101 */
} ch_config_t;


//! ASIC firmware init function pointer typedef.
typedef uint8_t (*ch_fw_init_func_t)(ch_dev_t *dev_ptr, ch_group_t *grp_ptr, uint8_t i2c_addr, 
		 					         uint8_t dev_num, uint8_t i2c_bus_index);

//! API function pointer typedefs.
typedef uint8_t	 (*ch_fw_load_func_t)(ch_dev_t *dev_ptr);
typedef uint8_t	 (*ch_get_config_func_t)(ch_dev_t *dev_ptr, ch_config_t *config_ptr);
typedef uint8_t	 (*ch_set_config_func_t)(ch_dev_t *dev_ptr, ch_config_t *config_ptr);
typedef uint8_t  (*ch_set_mode_func_t)(ch_dev_t *dev_ptr, ch_mode_t mode);
typedef uint8_t  (*ch_set_sample_interval_func_t)(ch_dev_t *dev_ptr, uint16_t sample_interval);
typedef uint8_t	 (*ch_set_num_samples_func_t)(ch_dev_t *dev_ptr, uint16_t num_samples);
typedef uint8_t	 (*ch_set_max_range_func_t)(ch_dev_t *dev_ptr, uint16_t max_range);
typedef uint8_t	 (*ch_set_static_range_func_t)(ch_dev_t *dev_ptr, uint16_t static_range);
typedef uint32_t (*ch_get_range_func_t)(ch_dev_t *dev_ptr, ch_range_t range_type);
typedef uint16_t (*ch_get_amplitude_func_t)(ch_dev_t *dev_ptr);
typedef uint32_t (*ch_get_frequency_func_t)(ch_dev_t *dev_ptr);
typedef uint8_t	 (*ch_get_iq_data_func_t)(ch_dev_t *dev_ptr, ch_iq_sample_t *buf_ptr, 
										  uint16_t start_sample, uint16_t num_samples, 
										  ch_io_mode_t io_mode);
typedef uint16_t (*ch_samples_to_mm_func_t)(ch_dev_t *dev_ptr, uint16_t num_samples);
typedef uint16_t (*ch_mm_to_samples_func_t)(ch_dev_t *dev_ptr, uint16_t num_mm);
typedef uint8_t  (*ch_set_thresholds_func_t)(ch_dev_t *dev_ptr, ch_thresholds_t *thresh_ptr);
typedef uint8_t  (*ch_get_thresholds_func_t)(ch_dev_t *dev_ptr, ch_thresholds_t *thresh_ptr);


//! API function pointer structure (internal use).
typedef struct {
	ch_fw_load_func_t			fw_load;
	ch_set_mode_func_t			set_mode;
	ch_set_sample_interval_func_t	set_sample_interval;
	ch_set_num_samples_func_t	set_num_samples;
	ch_set_max_range_func_t		set_max_range;
	ch_set_static_range_func_t	set_static_range;
	ch_get_range_func_t			get_range;
	ch_get_amplitude_func_t		get_amplitude;
	ch_get_frequency_func_t		get_frequency;
	ch_get_iq_data_func_t		get_iq_data;
	ch_samples_to_mm_func_t		samples_to_mm;
	ch_mm_to_samples_func_t		mm_to_samples;
	ch_set_thresholds_func_t	set_thresholds;
	ch_get_thresholds_func_t	get_thresholds;
} ch_api_funcs_t;


//! Data-ready interrupt callback routine pointer.
typedef void (*ch_io_int_callback_t)(ch_group_t *grp_ptr, uint8_t io_index); 
//
//! Non-blocking I/O complete callback routine pointer.
typedef void (*ch_io_complete_callback_t)(ch_group_t *grp_ptr);		
//
//! Periodic timer callback routine pointer.
typedef void (*ch_timer_callback_t)(void);			


//!  Chirp sensor group configuration structure.

/*! \note The \a CHIRP_MAX_NUM_SENSORS and \a CHIRP_NUM_I2C_BUSES symbols must be defined 
 * by the user.  Normally this is done in the \b chirp_board_config.h header file that is 
 * part of the board support package.
 */
typedef struct ch_group_t {		/* [note tag name matches type to help Doxygen linkage] */
	uint8_t num_ports;						/*!< Number of ports (max possible sensor connections) */
	uint8_t num_i2c_buses;					/*!< Number of I2C buses on this board */
	uint8_t sensor_count;					/*!< Number of sensors detected */
	uint16_t i2c_drv_flags;					/*!< Flags for special I2C handling by Chirp driver, from 
											     \a chbsp_get_i2c_info()*/
	uint16_t rtc_cal_pulse_ms; 				/*!< Real-time clock calibration pulse length (in ms) */
	chdrv_discovery_hook_t disco_hook;		/*!< Addr of hook routine to call when device found on bus */
	ch_io_int_callback_t io_int_callback;			/*!< Addr of routine to call when sensor interrupts */
	ch_io_complete_callback_t io_complete_callback;	/*!< Addr of routine to call when non-blocking I/O 
													     completes */
	ch_dev_t *device[CHIRP_MAX_NUM_SENSORS];		/*!< Array of pointers to ch_dev_t structures for 
													     individual sensors */
	uint8_t num_connected[CHIRP_NUM_I2C_BUSES];		/*!< Array of counters for connected sensors per bus */
	chdrv_i2c_queue_t i2c_queue[CHIRP_NUM_I2C_BUSES];	/*!< Array of I2C non-blocking transaction 
														     queues (one per bus) */
} ch_group_t;

//! Chirp sensor device structure.
typedef struct ch_dev_t {		/* [note tag name matches type to help Doxygen linkage] */
	ch_group_t	*group;				/*!< Pointer to parent group structure. */
	ch_mode_t	mode;				/*!< Sensor operating mode. */
	uint16_t	max_range;			/*!< Maximum range, in mm */
	uint16_t	static_range;		/*!< Static target rejection range, in samples (0 if unused) */
	uint16_t 	sample_interval;	/*!< Sample interval (in ms), only if in free-running mode */
	uint16_t 	rtc_cal_result; 	/*!< Real-time clock calibration result for the sensor. */
	uint32_t 	op_frequency; 		/*!< Operating frequency for the sensor. */
	uint16_t 	bandwidth; 			/*!< Bandwidth for the sensor. */
	uint16_t 	scale_factor; 		/*!< Scale factor for the sensor. */
	uint8_t  	i2c_address; 		/*!< Current I2C addresses. */
	uint8_t  	app_i2c_address;	/*!< Assigned application I2C address for device in normal operation*/
	uint16_t	i2c_drv_flags;		/*!< Flags for special I2C handling by Chirp driver */
	uint16_t 	part_number; 		/*!< Integer part number (e.g. 101 for a CH101 device). */
	int8_t		oversample;			/*!< Oversampling factor (power of 2) */
	uint8_t  	sensor_connected; 	/*!< Sensor connection status:
									  	 1 if discovered and successfully initialized, 0 otherwise. */
	uint8_t  	io_index; 			/*!< Index value (device number) identifying device within group */
	uint8_t  	i2c_bus_index; 		/*!< Index value identifying which I2C bus is used for this device. */
	uint16_t 	max_samples; 		/*!< Maximum number of receiver samples for this sensor firmware */
	uint16_t 	num_rx_samples; 	/*!< Number of receiver samples for the current max range setting. */

	/* Sensor Firmware-specific Linkage Definitions */
	const char	  *fw_version_string;		/*!< Pointer to string identifying sensor firmware version. */
	const uint8_t *firmware;				/*!< Pointer to start of sensor firmware image to be loaded */
	const uint8_t *ram_init;				/*!< Pointer to ram initialization data */
	void     (*prepare_pulse_timer)(ch_dev_t *dev_ptr);	/*!< Pointer to function preparing sensor pulse 
														     timer to measure real-time clock (RTC) 
															 calibration pulse sent to device. */
	void     (*store_pt_result)(ch_dev_t *dev_ptr);	/*!< Pointer to function to read RTC calibration 
													     pulse timer result from sensor and place value 
														 in the \a rtc_cal_result field. */
	void     (*store_op_freq)(ch_dev_t *dev_ptr);	/*!< Pointer to function to read operating 
													     frequency and place value in the
														 \a op_frequency field. */
	void     (*store_bandwidth)(ch_dev_t *dev_ptr);	/*!< Pointer to function to read operating bandwidth 
													     and place value in the \a bandwidth field. */
	void     (*store_scalefactor)(ch_dev_t *dev_ptr);	/*!< Pointer to function to calculate scale factor
														     and place value in \a scalefactor field. */
	uint8_t  (*get_locked_state)(ch_dev_t *dev_ptr);	/*!< Pointer to function returning locked state 
														     for sensor. */
	uint16_t (*get_fw_ram_init_size)(void);			/*!< Pointer to function returning ram init size 
													     for sensor. */
	uint16_t (*get_fw_ram_init_addr)(void);			/*!< Pointer to function returning start address of 
													     ram initialization area in the sensor. */

	/* API and callback functions */
	ch_api_funcs_t		api_funcs;						/*!< Structure containing API function pointers. */

} ch_dev_t;


/* API function prototypes and documentation */


/*!
 * \brief Initialize the device descriptor for a sensor.
 *
 * \param dev_ptr 		pointer to the ch_dev_t descriptor structure
 * \param grp_ptr 		pointer to the ch_group_t descriptor for sensor group to join
 * \param dev_num		number of the device within the sensor group (identifies which physical sensor)
 * \param fw_init_func 	pointer to the sensor firmware initialization function (determines sensor 
 *                     	feature set)
 *
 * \return 0 if success, 1 if error
 *
 * This function is used to initialize various Chirp SonicLib structures before using a sensor.
 * The ch_dev_t device descriptor is the primary data structure used to manage a sensor, and its 
 * address will subsequently be used as a handle to identify the sensor when calling most API functions.
 *
 * The \a dev_ptr parameter is the address of the ch_dev_t descriptor structure that will be initialized 
 * and then used to identify and manage this sensor. The \a grp_ptr parameter is the address of a 
 * ch_group_t structure describing the sensor group that will include the new sensor.  Both the ch_dev_t 
 * structure and the ch_group_t structure must have already been allocated before this function is called. 
 *
 * Generally, an application will require only one ch_group_t structure to manage all Chirp sensors.
 * However, a separate ch_dev_t structure must be allocated for each sensor.
 *
 * \a dev_num is a simple index value that uniquely identifies a sensor within a group.  Each possible 
 * sensor (i.e. each physical port on the board that could have a Chirp sensor attached) has a number, 
 * starting with zero (0).  The device number is constant - it remains associated with a specific port 
 * even if no sensor is actually attached.  Often, the \a dev_num value is used by both the 
 * application and the board support package as an index into arrays containing per-sensor 
 * information (e.g. data read from the sensors, pin assignments, etc.).
 *
 * The Chirp sensor is fully re-programmable, and the specific features and capabilities can be modified 
 * by using different sensor firmware images.  The \a fw_init_func parameter is the address (name) of 
 * the sensor firmware initialization routine that should be used to program the sensor and prepare it 
 * for operation. The selection of this routine name is the only required change when switching from 
 * one sensor firmware image to another.
 *
 * \note This function only performs internal initialization of data structures, etc.  It does not 
 * actually initialize the physical sensor device(s).  See \a ch_group_start().
 */
uint8_t	ch_init(ch_dev_t *dev_ptr, ch_group_t *grp_ptr, uint8_t dev_num, ch_fw_init_func_t fw_init_func);

/*!
 * \brief	Program and start a group of sensors.
 *
 * \param	grp_ptr pointer to the ch_group_t descriptor for sensor group to be started
 *
 * \return 0 if successful, 1 if error
 *
 * This function performs the actual discovery, programming, and initialization sequence for all sensors 
 * within a sensor group.  Each sensor must have previously been added to the group by calling 
 * \a ch_init().
 *
 * In brief, this function does the following for each sensor:
 * - Probe the possible sensor ports using I2C bus and each sensor's PROG line, to discover if sensor 
 *   is connected.
 * - Reset sensor.
 * - Program sensor with firmware (version specified during \a ch_init()).
 * - Assign unique I2C address to sensor (specified by board support package, see \a chbsp_i2c_get_info()).
 * - Start sensor execution.
 * - Wait for sensor to lock (complete initialization, including self-test).
 * - Send timed pulse on INT line to calibrate sensor Real-Time Clock (RTC).
 * 
 * After this routine returns successfully, the sensor configuration may be set and ultrasonic 
 * measurements may begin.
 */
uint8_t	ch_group_start(ch_group_t *grp_ptr);

/*!
 * \brief Get current configuration settings for a sensor
 *
 * \param	dev_ptr 	pointer to the ch_dev_t descriptor structure
 * \param	config_ptr pointer to a ch_config_t structure to receive configuration values
 *
 * \return 0 if successful, 1 if error
 *
 * This function obtains the current configuration settings from the sensor and returns them
 * in a ch_config_t structure, whose address is specified by \a config_ptr.
 * 
 * \note The individual configuration values returned in the ch_config_t structure may also be 
 * obtained by using dedicated single-value functions.  See \a ch_get_mode(), \a ch_get_max_range(), 
 * \a ch_get_sample_interval(), \a ch_get_static_range(), and \a ch_get_thresholds().
 */
uint8_t	ch_get_config(ch_dev_t *dev_ptr, ch_config_t *config_ptr);

/*!
 * \brief Set multiple configuration settings for a sensor
 *
 * \param	dev_ptr 	pointer to the ch_dev_t descriptor structure
 * \param	config_ptr pointer to a ch_config_t structure containing new configuration values
 *
 * \return 0 if successful, 1 if error
 *
 * This function sets multiple configuration options within the sensor.  The configuration settings 
 * are passed in a ch_config_t structure, whose address is specified by \a config_ptr.  The fields 
 * in the ch_config_t structure must have been set with your new configuration values before this 
 * function is called.
 * 
 * \note The individual configuration values set by this function may also be set using dedicated 
 * single-value functions. These two methods are completely equivalent and may be freely mixed.  
 * See \a ch_set_mode(), \a ch_set_max_range(), \a ch_set_sample_interval(), \a ch_set_static_range(), 
 * and \a ch_set_thresholds().
 */
uint8_t	ch_set_config(ch_dev_t *dev_ptr, ch_config_t *config_ptr);

/*!
 * \brief Trigger a measurement on one sensor
 *
 * \param	dev_ptr 	pointer to the ch_dev_t descriptor structure
 * 
 * This function generates a pulse on the INT line for a single sensor.  If the sensor is in either 
 * \a CH_MODE_TRIGGERED_TX_RX or \a CH_MODE_TRIGGERED_RX_ONLY mode, this pulse will begin a measurement 
 * cycle.
 *
 * To simultaneously trigger all sensors in a group, use \a ch_group_trigger().
 *
 * \note Do not trigger a new measurement until the previous measurement has completed and all needed
 * data has been read from the device (including I/Q data, if \a ch_get_iq_data() is used).  If any
 * I/O operations are still active, the new measurement may be corrupted.
 */
void ch_trigger(ch_dev_t *dev_ptr);

/*!
 * \brief Trigger a measurement on a group of sensors
 *
 * \param	grp_ptr 	pointer to the ch_group_t descriptor for this group of sensors
 * 
 * This function generates a pulse on the INT line for each sensor in the sensor group.  If a sensor is 
 * in either \a CH_MODE_TRIGGERED_TX_RX or \a CH_MODE_TRIGGERED_RX_ONLY mode, this pulse will begin a 
 * measurement cycle.
 *
 * If a two or more sensors are operating in pitch-catch mode (in which one transmits and the others 
 * receive), this function must be used to start a measurement cycle, so that the devices are 
 * synchronized.
 *
 * To trigger a single sensor, use \a ch_trigger().
 *
 * \note Do not trigger a new measurement until the previous measurement has completed and all needed
 * data has been read from the device (including I/Q data, if \a ch_get_iq_data() is used).  If any
 * I/O operations are still active, the new measurement may be corrupted.
 */
void ch_group_trigger(ch_group_t *grp_ptr);

/*!
 * \brief Reset a sensor
 *
 * \param	dev_ptr 	pointer to the ch_dev_t descriptor structure
 * \param	reset_type	type of reset (\a CH_RESET_HARD or \a CH_RESET_SOFT)
 * 
 * This function resets a sensor.  The \a reset_type parameter indicates if a software reset or 
 * full hardware reset is requested.
 */
void ch_reset(ch_dev_t *dev_ptr, ch_reset_t reset_type);

/*!
 * \brief Reset a group of sensors
 *
 * \param	grp_ptr 	pointer to the ch_group_t descriptor for this group of sensors
 * \param	reset_type	type of reset (\a CH_RESET_HARD or \a CH_RESET_SOFT)
 * 
 * This function resets all sensors in a sensor group.  The \a reset_type parameter indicates if a 
 * software reset or full hardware reset is requested.
 */
void ch_group_reset(ch_group_t *grp_ptr, ch_reset_t reset_type);

/*! 
 * \brief	Indicate if a sensor is connected
 *
 * \param 	dev_ptr 	pointer to the ch_dev_t descriptor structure
 * \return 	1 if the sensor is connected, 0 otherwise
 */
uint8_t ch_sensor_is_connected(ch_dev_t *dev_ptr);

/*!
 * \brief	Get part number for a sensor.
 *
 * \param 	dev_ptr 	pointer to the ch_dev_t descriptor structure
 * \return 	Integer part number
 *
 * This function returns the Chirp part number for the specified device.  The
 * part number is a simple integer value, for example 101 for a CH101 device.
 */
uint16_t ch_get_part_number(ch_dev_t *dev_ptr);

/*!
 * \brief	Get device number (I/O index values) for a sensor
 *
 * \param 	dev_ptr 	pointer to the ch_dev_t descriptor structure
 *
 * \return 	Device number
 *
 * This function returns the device number (I/O index) of the sensor within its sensor group.  
 * Normally, this also corresponds to the sensor's port number on the board, and is used for 
 * indexing arrays of pin definitions etc. within the board support package routines.
 */
uint8_t ch_get_dev_num(ch_dev_t *dev_ptr);

/*!
 * \brief	Get device descriptor pointer for a sensor
 *
 * \param	grp_ptr 	pointer to the ch_group_t descriptor for this group of sensors
 * \param	dev_num		device number within sensor group
 *
 * \return 	Pointer to ch_dev_t descriptor structure
 *
 * This function returns the address of the ch_dev_t device descriptor for a certain sensor in
 * a sensor group.  The sensor is identified within the group by the \a dev_num device number.
 */
ch_dev_t *  ch_get_dev_ptr(ch_group_t *grp_ptr, uint8_t dev_num);

/*!
 * \brief	Get the total number of sensor ports (possible sensors) in a sensor group
 *
 * \param	grp_ptr 	pointer to the ch_group_t descriptor for this group of sensors
 *
 * \return 	Total number of ports (possible sensors) in the sensor group
 *
 * This function returns the maximum number of possible sensors within a sensor group.  Typically, 
 * the number of sensors is limited by the physical connections on the board being used, so the 
 * number of sensor ports on the board is returned by this function.
 */
uint8_t	ch_get_num_ports(ch_group_t *grp_ptr);

/*!
 * \brief	Get the active I2C address for a sensor
 *
 * \param 	dev_ptr 	pointer to the ch_dev_t descriptor structure
 *
 * \return	I2C address, or 0 if error
 *
 * This function returns the currently active I2C address for a sensor device.  This function
 * may be used by board support package routines to determine the proper I2C address to use for
 * a specified sensor.
 */
uint8_t	ch_get_i2c_address(ch_dev_t *dev_ptr);

/*!
 * \brief	Get the active I2C bus for a sensor
 *
 * \param 	dev_ptr 	pointer to the ch_dev_t descriptor structure
 *
 * \return	I2C bus index
 *
 * This function returns the I2C bus index for a sensor device.  This function
 * may be used by board support package routines to determine the proper I2C bus to use for
 * a specified sensor.
 */
uint8_t	ch_get_i2c_bus(ch_dev_t *dev_ptr);

/*!
 * \brief	Get the firmware version description string for a sensor
 *
 * \param 	dev_ptr 	pointer to the ch_dev_t descriptor structure
 *
 * \return 	Pointer to character string describing sensor firmware version
 *
 * This function returns a pointer to a string that describes the sensor firmware being used 
 * on the device.
 */
char *	ch_get_fw_version_string(ch_dev_t *dev_ptr);

/*!
 * \brief	Get the current operating mode for a sensor.
 *
 * \param	dev_ptr pointer to the ch_dev_t config structure
 * \return	Sensor operating mode
 *
 * This function returns the current operating mode for the sensor, one of:
 *	- \a CH_MODE_IDLE - low power idle mode, no measurements take place
 *	- \a CH_MODE_FREERUN - free-running mode, sensor uses internal clock to wake and measure
 *	- \a CH_MODE_TRIGGERED_TX_RX - hardware-triggered, sensor both transmits and receives
 *	- \a CH_MODE_TRIGGERED_RX_ONLY - hardware triggered, sensor only receives
 */
ch_mode_t ch_get_mode(ch_dev_t *dev_ptr);

/*!
 * \brief Configure a sensor for the specified operating mode.
 *
 * \param dev_ptr 	pointer to the ch_dev_t descriptor structure
 * \param mode 		the new operating mode for the sensor
 * \return 0 if successful.
 *
 * This function sets the sensor to operate in the specified mode, which
 * must be one of the following:
 *	- \a CH_MODE_IDLE - low power idle mode, no measurements take place
 *	- \a CH_MODE_FREERUN - free-running mode, sensor uses internal clock to wake and measure
 *	- \a CH_MODE_TRIGGERED_TX_RX - hardware-triggered, sensor both transmits and receives
 *	- \a CH_MODE_TRIGGERED_RX_ONLY - hardware triggered, sensor only receives
 */
uint8_t ch_set_mode(ch_dev_t *dev_ptr, ch_mode_t mode);

/*!
 * \brief Get the internal sample timing interval for a sensor
 *
 *  \param dev_ptr 	pointer to the ch_dev_t descriptor structure
 * \return 			Interval between samples (in ms), or 0 if device is not in free-running mode
 *
 * This function returns the interval between measurements, in milliseconds, for
 * for a sensor operating in free-running mode.  If the sensor is in a different operating 
 * mode (e.g. a triggered mode), zero is returned.
 */
uint16_t ch_get_sample_interval(ch_dev_t *dev_ptr);

/*!
 * \brief Configure the internal sample interval for a sensor in freerunning mode.
 *
 *  \param dev_ptr 		pointer to the ch_dev_t descriptor structure
 * \param interval_ms 	interval between samples, in milliseconds. 
 * \return 0 if successful, 1 if arguments are invalid.
 *
 * This function sets the sample interval for a sensor operating in freerunning mode 
 * (\a CH_MODE_FREERUN).  The sensor will use its internal clock to wake and perform a 
 * measurement every \a interval_ms milliseconds.
 *
 * \note This function has no effect for a sensor operating in one of the triggered modes.
 * The sample interval for a triggered device is determined by the external trigger timing.
 */
uint8_t ch_set_sample_interval(ch_dev_t *dev_ptr, uint16_t interval_ms);

/*!
 * \brief Get the number of samples per measurement cycle
 *
 * \param dev_ptr pointer to the ch_dev_t descriptor struct
 *
 * \return  Number of samples per measurement cycle
 *
 * This function returns the current number of samples which the Chirp sensor will perform
 * during each measurement cycle.  The number of samples directly corresponds to the 
 * range at which the sensor can detect, so this value is determined by the current maximum 
 * range setting for the sensor.  Also see \a ch_get_max_range().
 *
 */
uint16_t ch_get_num_samples(ch_dev_t *dev_ptr);

/*!
 * \brief Set the sensor sample count directly.
 *
 * \param dev_ptr pointer to the ch_dev_t descriptor struct
 * \param num_samples number of samples during each measurement cycle
 *
 * \return 0 if successful
 *
 * This function directly sets the number of samples which the Chirp sensor will perform
 * during a single measurement cycle.  The number of samples directly corresponds to the 
 * range at which the sensor can detect.
 *
 * Also see \a ch_set_max_range().
 *
 * \note Normally, the sample is count is not set using this function, but is instead set
 * indirectly using either \a ch_set_max_range() or \a ch_set_config(), both of which 
 * automatically set the sample count based on a specified range in millimeters.
 */
uint8_t ch_set_num_samples(ch_dev_t *dev_ptr, uint16_t num_samples);

/*!
 * \brief Get the maximum range setting for a sensor.
 *
 * \param dev_ptr pointer to the ch_dev_t descriptor structure
 *
 * \return Maximum range setting, in millimeters
 *
 * This function returns the current maximum detection range setting for the sensor, 
 * in millimeters.
 *
 * \note The maximum range may also be obtained, along with other settings, using the 
 * \a ch_get_config() function.
 */
uint16_t ch_get_max_range(ch_dev_t *dev_ptr);

/*!
 * \brief Set the maximum range for a sensor.
 *
 * \param dev_ptr pointer to the ch_dev_t descriptor structure
 * \param max_range maximum range, in millimeters
 *
 * \return 0 if successful, non-zero if error
 *
 * This function sets the maximum detection range for the sensor, in millimeters.  The 
 * detection range setting controls how long the sensor will listen (i.e. how many samples 
 * it will capture) during each measurement cycle.  (The number of samples is automatically 
 * calculated for the specified range.)
 *
 * \note The maximum range may also be specified, along with other settings, using the 
 * \a ch_set_config() function.  These two methods are completely equivalent and may 
 * be freely mixed.
 */
uint8_t ch_set_max_range(ch_dev_t *dev_ptr, uint16_t max_range);

/*!
 * \brief Get static target rejection range setting.
 *
 * \param dev_ptr pointer to the ch_dev_t descriptor structure
 *
 * \return Static target rejection range setting, in samples, or 0 if not enabled
 *
 * This function returns the number of samples at the beginning of a measurement cycle over 
 * which static target rejection filtering will be applied.   Also see \a ch_set_static_range().
 *
 * To calculate the physical distance that corresponds to the number of samples, use the 
 * \a ch_samples_to_mm() function.
 */
uint16_t ch_get_static_range(ch_dev_t *dev_ptr);

/*!
 * \brief Configure static target rejection.
 *
 * \param dev_ptr pointer to the ch_dev_t descriptor structure
 * \param num_samples number of sensor samples (at beginning of measurement cycle) over 
 *                    which static targets will be rejected
 *
 * \return 0 if successful, non-zero if error
 *
 * Static target rejection is a special processing mode in which the sensor will actively filter 
 * out signals from close, non-moving objects, so that they do not continue to generate range 
 * readings.  This allows detection and reporting of target objects that are farther away than the 
 * static objects.  (Normally, the sensor reports the range value for the closest detected object.)
 *
 * Static target rejection is applied for a specified number of samples, starting at the beginning 
 * of a measurement cycle* (i.e. for the closest objects).  The num_samples parameter specifies the 
 * number of samples that will be filtered.  To calculate the appropriate value for \a num_samples 
 * to filter over a certain physical distance, use the \a ch_mm_to_samples() function.
 */
uint8_t ch_set_static_range(ch_dev_t *dev_ptr, uint16_t num_samples);

/*!
 * \brief Get the measured range from a sensor.
 *
 * \param dev_ptr pointer to the ch_dev_t descriptor structure
 * \param range_type the range type to be reported (e.g. one-way vs. round-trip)
 *
 * \return Range in millimeters times 32, or \a CH_NO_TARGET (0xFFFFFFFF) if no target was detected, 
 *         or 0 if error
 *
 * This function reads the measurement result registers from the sensor and then computes the actual 
 * range.  It should be called after the sensor has indicated that a measurement cycle is complete 
 * by generating a signal on the INT line.  (Typically, this will be set up by an interrupt handler 
 * associated with that input line.)
 *
 * The \a range_type parameter indicates whether the measurement is based on the one-way or round-trip
 * distance to/from a target, or the direct distance between two sensors operating in pitch-catch mode.
 * The possible values are:
 *   - \a CH_RANGE_ECHO_ONE_WAY -  gets full pulse/echo round-trip distance, then divides by 2
 *   - \a CH_RANGE_ECHO_ROUND_TRIP	- full pulse/echo round-trip distance
 *   - \a CH_RANGE_DIRECT - for receiving sensor in pitch-catch mode (one-way)
 *
 * This function returns the measured range as a 32-bit integer.  For maximum precision, the range value 
 * is returned in a fixed-point format with 5 fractional bits.  So, the return value is the number of 
 * millimeters times 32. Divide the value by 32 (shift right 5 bits) to get whole mm, or use floating 
 * point (i.e.  divide by 32.0f) to preserve the full sub-millimeter precision.
 *
 * If the sensor did not successfully find the range of a target during the most recent measurement, 
 * the returned range value will be \a CH_NO_TARGET.  If an error occurs when getting or calculating the 
 * range, zero (0) will be returned.
 *
 * \note This function only reports the results from the most recently completed measurement cycle.  It 
 * does not actually trigger a measurement.
 *
 * \note The \a range_type parameter only controls how this function interprets the results from the 
 * measurement cycle.  It does not change the sensor mode.
 *
 */
uint32_t ch_get_range(ch_dev_t *dev_ptr, ch_range_t range_type);

/*!
 * \brief Get the measured amplitude from a sensor.
 *
 * \param dev_ptr pointer to the ch_dev_t descriptor structure
 *
 * \return Amplitude value for most recent successful range reading
 *
 * This function returns the amplitude value for the most recent successful range measurement by 
 * the sensor. The amplitude is representative of the incoming sound pressure.  The value is expressed 
 * in internal sensor counts and is not calibrated to any standard units.
 *
 * The amplitude value is not updated if a measurement cycle resulted in \a CH_NO_TARGET, as returned 
 * by \a ch_get_range().
 */
uint16_t ch_get_amplitude(ch_dev_t *dev_ptr);

/*!
 * \brief Get the operating frequency of a sensor.
 *
 * \param dev_ptr pointer to the ch_dev_t descriptor structure
 *
 * This function returns the operating frequency of the sensor.  This is the primary frequency of the 
 * ultrasonic pulse that is emitted by the device when transmitting.  
 *
 * \return Frequency, in Hz
 */
uint32_t ch_get_frequency(ch_dev_t *dev_ptr);  

/*!
 * \brief Get the real-time clock calibration value
 *
 * \param dev_ptr pointer to the ch_dev_t descriptor structure
 *
 * \return 	RTC calibration value
 *
 * This function returns the real-time clock (RTC) calibration value read from the sensor during
 * \a ch_group_start().  The RTC calibration value is calculated by the sensor during the RTC 
 * calibration pulse, and it is used internally in calculations that convert between time and distance.
 */
uint16_t ch_get_rtc_cal_result(ch_dev_t *dev_ptr);

/*!
 * \brief Get the real-time clock calibration pulse length
 *
 * \param dev_ptr pointer to the ch_dev_t descriptor structure
 *
 * \return RTC pulse length, in ms
 *
 * This function returns the length (duration), in milliseconds, of the the real-time clock (RTC) 
 * calibration pulse used for the sensor.  The pulse is applied to the sensor's INT line during 
 * \a ch_group_start() to calibrate the sensor's internal clock.  The pulse length is specified by 
 * the board support package during the \a chbsp_board_init() function.
 *
 * The RTC calibration pulse length is used internally in calculations that convert between time 
 * and distance.
 */
uint16_t ch_get_rtc_cal_pulselength(ch_dev_t *dev_ptr);

/*!
 * \brief Get the raw I/Q measurement data from a sensor
 *
 * \param dev_ptr 	pointer to the ch_dev_t descriptor structure
 * \param buf_ptr	pointer to data buffer where I/Q data will be written
 * \param start_sample  starting sample number within measurement data (0 = start of data)
 * \param num_samples	number of samples to read from sensor
 * \param mode		whether I/Q read should block (\a CH_IO_MODE_BLOCK (0) = blocking, 
 * 												   \a CH_IO_MODE_NONBLOCK (1) = non-blocking)
 *
 * \return 0 if successful, 1 if error
 *
 * This function reads the raw I/Q measurement data from the sensor.  The I/Q data set includes 
 * a discrete pair of values for each of the samples that make up a full measurement cycle.   Each 
 * individual sample is reported as a pair of values, I and Q, in a quadrature format.  The I/Q 
 * values may be used to calculate the relative amplitude of the measured ultrasound signal.
 *
 * The contents of the I/Q trace are updated on every measurement cycle, even if no target was 
 * detected (i.e. even if \a ch_get_range() returns \a CH_NO_TARGET).  (Note that this is different 
 * than the regular amplitude value, as returned by \a ch_get_amplitude(), which is \a not updated 
 * unless a target is detected.)
 * 
 * Each sample I/Q pair consists of two signed 16-bit integers and is described by the 
 * \a ch_iq_sample_t structure. To convert any given pair of I/Q values to the amplitude value for 
 * that sample, square both I and Q, and take the square root of the sum:  
 * 			\f[Amp_n = \sqrt{(I_n)^2 + (Q_n)^2}\f]
 * Amplitude values in the sensor are expressed only in internal ADC counts (least-significant bits, 
 * or LSBs) and are not calibrated to any standard units. 
 *
 * The number of samples used in each I/Q trace is determined by the maximum range setting for the 
 * device.  If it is set to less than the maximum possible range, not all samples will contain valid 
 * data.  To determine the number of active samples within the trace, use \a ch_get_num_samples().  
 *
 *  - To read all valid I/Q data, set \a start_sample to zero (0), and set \a num_samples to the 
 *  value returned by \a ch_get_num_samples().
 *
 * To determine what sample number corresponds to a physical distance, use \a ch_mm_to_samples().
 *
 * To allow more flexibilty in your application, the I/Q data readout from the device may be done 
 * in a non-blocking mode, by setting \a mode to \a CH_IO_MODE_NONBLOCK (1).  In non-blocking 
 * mode, the I/O operation takes place using DMA access in the background.  This function will 
 * return immediately, and a notification will later be issued when the I/Q has been read.  To 
 * use the \a non_block option, the board support package (BSP) you are using must provide the 
 * \a chbsp_i2c_read_nb() and \a chbsp_i2c_read_mem_nb() functions.  To use non-blocking reads 
 * of the I/Q data, you must specify a callback routine that will be called when the I/Q read 
 * completes.  See \a ch_io_complete_callback_set(). 
 *
 * Non-blocking reads are managed together for a group of sensors.  To perform a non-blocking read:
 *
 *  -# Register a callback function using \a ch_io_complete_callback_set().
 *  -# Define and initialize a handler for the DMA interrupts generated.
 *  -# Synchronize with all sensors whose I/Q data should be read by waiting for all to indicate 
 *     data ready.
 *  -# Set up a non-blocking read on each sensor, using \a ch_get_iq_data() with \a mode = 
 *     \a CH_IO_MODE_NONBLOCK (1).
 *  -# Start the non-blocking reads on all sensors in the group, using \a ch_io_start_nb().
 *  -# Your callback function (set in step #1 above) will be called as each individual sensor's 
 *     read completes.  Your callback function should initiate any further processing of the I/Q 
 *     data, possibly by setting a flag that will be checked from within the application's main 
 *     execution loop.  The callback function will likely be called at interrupt level, so the 
 *     amount of processing within it should be kept to a minimum.
 *
 * For a CH101 sensor, up to 150 samples are taken during each measurement cycle; for a CH201 
 * sensor, up to 450 samples are taken.  So, a complete CH101 I/Q trace will contain up to 600 
 * bytes of data (150 samples x 4 bytes per sample), and a CH201 I/Q trace may contain up to
 * 1800 bytes.  The buffer specified by \a buf_ptr must be large enough to hold this amount of data.
 *
 * When the I/Q data is read from the sensor, the additional time required to transfer the I/Q 
 * data over the I2C bus must be taken into account when planning how often the sensor can be 
 * read (sample interval).  
 *
 * \note It is important that any data I/O operations to or from the sensor, including reading 
 * the I/Q data, complete before a new measurement cycle is triggered, or the new measurement may 
 * be affected.
 *
 * \note This function only obtains the data from the most recently completed measurement cycle. 
 * It does not actually trigger a measurement.
 */
uint8_t	ch_get_iq_data(ch_dev_t *dev_ptr, ch_iq_sample_t *buf_ptr, uint16_t start_sample, 
		               uint16_t num_samples, ch_io_mode_t mode);

/*!
 * \brief Convert sample count to millimeters for a sensor
 *
 * \param dev_ptr pointer to the ch_dev_t descriptor structure
 * \param num_samples  sample count to be converted
 *
 * \return Number of millimeters
 *
 * This function converts the sample count specified in \a num_samples and converts it to the 
 * corresponding physical distance in millimeters.  The conversion uses values set during device 
 * initialization and calibration that describe the internal timing of the sensor.
 *
 * This function may be helpful when working with both physical distances (as reported by the 
 * \a ch_get_range() function) and sample-oriented values, such as data obtained from 
 * \a ch_get_iq_data() or parameters for static target rejection (see \a ch_set_static_range()).
 */
uint16_t ch_samples_to_mm(ch_dev_t *dev_ptr, uint16_t num_samples);

/*!
 * \brief Convert millimeters to sample count for a sensor
 *
 * \param dev_ptr pointer to the ch_dev_t descriptor structure
 * \param num_mm  number of millimeters to be converted
 *
 * \return Number of samples
 *
 * This function converts the distance in millimeters specified in \a num_mm and converts it 
 * to the corresponding number of sensor samples.  The conversion uses values set during device 
 * initialization and calibration that describe the internal timing of the sensor, along with
 * the current maximum range setting for the device.
 *
 * This function may be helpful when working with both physical distances (as reported by the 
 * \a ch_get_range() function) and sample-oriented values, such as data obtained from 
 * \a ch_get_iq_data() or parameters for static target rejection (see \a ch_set_static_range()).
 */
uint16_t ch_mm_to_samples(ch_dev_t *dev_ptr, uint16_t num_mm);


/*!
 * \brief Start non-blocking I/O operation(s) for a group of sensors
 *
 * \param grp_ptr pointer to the ch_group_t descriptor for sensor group
 *
 * \return 0 if success, 1 if error
 *
 * This function starts one or more non-blocking I/O operations on a group of sensors.  
 * Generally, the I/O operations are non-blocking I/Q data read requests individually 
 * generated using \a ch_get_iq_data().
 *
 * This function will return immediately after the I/O operations are started.  When the I/O operations complete,
 * the callback function registered using \a ch_io_callback_set() will be called.
 *
 * See \a ch_get_iq_data() for more information.
 */
uint8_t ch_io_start_nb(ch_group_t *grp_ptr);

/*!
 * \brief Register sensor interrupt callback routine for a group of sensors
 *
 * \param grp_ptr pointer to the ch_group_t sensor group descriptor structure
 * \param callback_func_ptr pointer to callback function to be called when sensor interrupts
 *
 * This function registers the routine specified by \a callback_func_ptr to be called whenever the
 * sensor interrupts.  Generally, such an interrupt indicates that a measurement cycle has completed
 * and the sensor has data ready to be read.  All sensors in a sensor group use the same callback function,
 * which receives the interrupting device's device number (port number) as an input parameter to identify
 * the specific interrupting device.
 *
 */
void 	ch_io_int_callback_set(ch_group_t *grp_ptr, ch_io_int_callback_t callback_func_ptr);

/*!
 * \brief Register non-blocking I/O complete callback routine for a group of sensors
 *
 * \param grp_ptr pointer to the ch_group_t group descriptor structure
 * \param callback_func_ptr pointer to callback function to be called when non-blocking I/O operations complete
 *
 * This function registers the routine specified by \a callback_func_ptr to be called when all outstanding 
 * non-blocking I/O operations complete for a group of sensors.   The non-blocking I/O operations must have previously
 * been initiated using \a ch_io_start_nb().
 */
void ch_io_complete_callback_set(ch_group_t *grp_ptr, ch_io_complete_callback_t callback_func_ptr);

/*!
 * \brief Notify SonicLib that a non-blocking I/O operation has completed
 *
 * \param grp_ptr pointer to the ch_group_t sensor group descriptor structure
 * \param i2c_bus_index identifier indicating on which I2C bus the I/O operation was completed
 *
 * This function should be called from your non-blocking I/O interrupt handler each time a non-blocking
 * I/O operation completes.  The \a i2c_bus_index parameter should indicate which I2C bus is being 
 * reported.
 *
 * When all outstanding non-blocking I/O operations are complete, SonicLib will call the callback 
 * function previously registered using \a ch_io_complete_callback_set().
 */
void ch_io_notify(ch_group_t *grp_ptr, uint8_t i2c_bus_index);

/*!
 * \brief Get detection thresholds (CH201 only).
 *
 * \param dev_ptr pointer to the ch_dev_t descriptor structure
 * \param thresh_ptr pointer to ch_thresholds_t structure to receive threshold data
 *
 * \return 0 if success, 1 if error
 *
 * This function obtains the current detection threshold values from the sensor and
 * returns them in a ch_thresholds_t structure specified by \a thresh_ptr.  The
 * ch_thresholds_t structure holds an array of ch_thresh_t structures, each of which
 * contains a starting sample number and amplitude threshold value.  
 *
 * \note This function is not supported on CH101 devices.
 */
uint8_t ch_get_thresholds(ch_dev_t *dev_ptr, ch_thresholds_t *thresh_ptr);

/*!
 * \brief Set detection thresholds (CH201 only).
 *
 * \param dev_ptr pointer to the ch_dev_t descriptor structure
 * \param thresh_ptr pointer to ch_thresholds_t structure containing threshold data
 *
 * \return 0 if success, 1 if error
 *
 * This function obtains the current detection threshold values from the sensor and
 * returns them in a ch_thresholds_t structure specified by \a thresh_ptr.  The
 * ch_thresholds_t structure holds an array of ch_thresh_t structures, each of which
 * contains a starting sample number and amplitude threshold value.
 *
 * To use this function, first initialize the ch_thresh_t sample/level pair of values for
 * each threshold.  A CH201 device supports six (6) thresholds.  Each threshold has a maximum
 * sample length of 255.
 *
 * \note This function is not supported on CH101 devices.
 */
uint8_t ch_set_thresholds(ch_dev_t *dev_ptr, ch_thresholds_t *thresh_ptr);

#endif	/* __SONICLIB_H_ */
