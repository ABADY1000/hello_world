/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include "soniclib.h"

// Driver includes
// #include "inc/soniclib.h"
// #include "chirp_board_config.h"

#define SLEEP_TIME_MS 1000
#define LED0_NODE DT_ALIAS(led3)
#define I2C_MAIN_NODE DT_NODELABEL(chrip)
#define I2C_CONF_NODE DT_NODELABEL(chrip_conf)
#define GPIO DT_NODELABEL(&gpio)

// static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios); 
// static const struct i2c_dt_spec iicm = I2C_DT_SPEC_GET(I2C_MAIN_NODE);
// static const struct i2c_dt_spec iicc = I2C_DT_SPEC_GET(I2C_CONF_NODE);

// static const struct gpio_dt_spec btn = GPIO_DT_SPEC_GET(DT_NODELABEL(but), gpios);
// static const struct gpio_dt_spec inp = GPIO_DT_SPEC_GET(DT_NODELABEL(int_inp), gpios);

// static struct gpio_callback btn_cb_data;

// void gpio_cb(const struct device *port, struct gpio_callback *cb, uint32_t pin){
// 	printk("INSIDE INTERRUPT\n\rPORT:%s\n\rPIN:%d\n\r", port->name, pin);
// }

// void gpio_cb2(const struct device *port, struct gpio_callback *cb, uint32_t pin){
// 	printk("INSIDE Function2\n\r");
// }

volatile uint32_t taskflags = 0;
static uint32_t active_devices;
static uint32_t data_ready_devices;
ch_dev_t	chirp_devices[CHIRP_MAX_NUM_SENSORS];
ch_group_t 	chirp_group;

#define DATA_READY_FLAG		(1 << 0)
#define IQ_READY_FLAG		(1 << 1)

#define	CHIRP_SENSOR_MAX_RANGE_MM		750	/* maximum range, in mm */

#define	CHIRP_SENSOR_STATIC_RANGE		0	/* static target rejection sample 
											   range, in samples (0=disabled) */
#define CHIRP_SENSOR_SAMPLE_INTERVAL	0	/* internal sample interval - 
											   NOT USED IF TRIGGERED */

static void periodic_timer_callback(void) {

	ch_group_trigger(&chirp_group);
}

static void sensor_int_callback(ch_group_t *grp_ptr, uint8_t dev_num) {
	ch_dev_t *dev_ptr = ch_get_dev_ptr(grp_ptr, dev_num);

	data_ready_devices |= (1 << dev_num);		// add to data-ready bit mask

	if (data_ready_devices == active_devices) {
		/* All active sensors have interrupted after performing a measurement */
		data_ready_devices = 0;

		/* Set data-ready flag - it will be checked in main() loop */
		taskflags |= DATA_READY_FLAG;

		/* Disable interrupt unless in free-running mode
		 *   It will automatically be re-enabled during the next trigger 
		 */
		if (ch_get_mode(dev_ptr) != CH_MODE_FREERUN) {
			chbsp_group_io_interrupt_disable(grp_ptr);
		}
	}
}

static void io_complete_callback(ch_group_t *grp_ptr) {

	taskflags |= IQ_READY_FLAG;
}


//#define	 CHIRP_SENSOR_FW_INIT_FUNC	ch101_gpr_open_init		/* CH101 GPR OPEN firmware */
#define	 CHIRP_SENSOR_FW_INIT_FUNC	ch201_gprmt_init	/* CH201 GPR Multi-Threshold firmware */

#define	MEASUREMENT_INTERVAL_MS		100		// 100ms interval = 10Hz sampling

ch_thresholds_t chirp_ch201_thresholds = {0, 	5000,		/* threshold 0 */
										 26,	2000,		/* threshold 1 */
										 39,	800,		/* threshold 2 */
										 56,	400,		/* threshold 3 */
										 79,	250,		/* threshold 4 */
										 89,	175};		/* threshold 5 */
										 
int main(void)
{
	ch_group_t	*grp_ptr = &chirp_group;
	chbsp_board_init(grp_ptr);

	uint8_t chirp_error = 0;
	uint8_t	num_connected = 0;
	uint8_t num_ports;
	uint8_t dev_num;

	
	num_ports = ch_get_num_ports(grp_ptr);
	ch_dev_t *dev_ptr = &(chirp_devices[dev_num]);	// init struct in array
	chirp_error |= ch_init(dev_ptr, grp_ptr, dev_num, CHIRP_SENSOR_FW_INIT_FUNC);

	if (chirp_error == 0) {
		printf("starting group... ");
		chirp_error = ch_group_start(grp_ptr);
	}

	dev_ptr = ch_get_dev_ptr(grp_ptr, 0);
	if (ch_sensor_is_connected(dev_ptr)) {

		printf("%d\tCH%d\t %u Hz\t%lu@%ums\t%s\n", dev_num,
										ch_get_part_number(dev_ptr),
										ch_get_frequency(dev_ptr),
										ch_get_rtc_cal_result(dev_ptr),
										ch_get_rtc_cal_pulselength(dev_ptr),
										ch_get_fw_version_string(dev_ptr));
	}
	printf("\n\r");

	chbsp_periodic_timer_init(MEASUREMENT_INTERVAL_MS, periodic_timer_callback);
	ch_io_int_callback_set(grp_ptr, sensor_int_callback);
	ch_io_complete_callback_set(grp_ptr, io_complete_callback);


	// Configure sensors with operation parameter
	ch_config_t dev_config;
	dev_ptr = ch_get_dev_ptr(grp_ptr, 0);

	if (ch_sensor_is_connected(dev_ptr)) {

		/* Select sensor mode 
			*   All connected sensors are placed in hardware triggered mode.
			*   The first connected (lowest numbered) sensor will transmit and 
			*   receive, all others will only receive.
			*/

		num_connected++;					// count one more connected
		active_devices |= (1 << dev_num);	// add to active device bit mask
		
		dev_config.mode = CH_MODE_TRIGGERED_TX_RX;
		// if (num_connected == 1) {			// if this is the first sensor
		// 	dev_config.mode = CH_MODE_TRIGGERED_TX_RX;
		// } else {									
		// 	dev_config.mode = CH_MODE_TRIGGERED_RX_ONLY;
		// }

		/* Init config structure with values from hello_chirp.h */
		dev_config.max_range       = CHIRP_SENSOR_MAX_RANGE_MM;
		dev_config.static_range    = CHIRP_SENSOR_STATIC_RANGE;
		dev_config.sample_interval = CHIRP_SENSOR_SAMPLE_INTERVAL;

		/* Set detection thresholds (CH201 only) */
		if (ch_get_part_number(dev_ptr) == CH201_PART_NUMBER) {
			/* Set pointer to struct containing detection thresholds */
			dev_config.thresh_ptr = &chirp_ch201_thresholds;	
		} else {
			dev_config.thresh_ptr = 0;							
		}

		/* Apply sensor configuration */
		chirp_error = ch_set_config(dev_ptr, &dev_config);

		/* Enable sensor interrupt if using free-running mode 
			*   Note that interrupt is automatically enabled if using 
			*   triggered modes.
			*/
		if ((!chirp_error) && (dev_config.mode == CH_MODE_FREERUN)) {
			chbsp_io_interrupt_enable(dev_ptr);
		}

		/* Read back and display config settings */
		if (!chirp_error) {
			display_config_info(dev_ptr);
		} else {
			printf("Device %d: Error during ch_set_config()\n", dev_num);
		}

		/* Turn on an LED to indicate device connected */
		if (!chirp_error) {
			// chbsp_led_on(dev_num);
			printk("Error in configuring the sensor\n\r");
		}
	}

	chbsp_periodic_timer_irq_enable();
	chbsp_periodic_timer_start();

	printf("Starting measurements\n");

	while(1){
		if (taskflags==0) {
			chbsp_proc_sleep();			// put processor in low-power sleep mode

			/* We only continue here after an interrupt wakes the processor */
		}

		/* Check for sensor data-ready interrupt(s) */
		if (taskflags & DATA_READY_FLAG) {

			/* All sensors have interrupted - handle sensor data */
			taskflags &= ~DATA_READY_FLAG;		// clear flag
			handle_data_ready(grp_ptr);			// read and display measurement
		}

		/* Check for non-blocking I/Q readout complete */
		if (taskflags & IQ_READY_FLAG) {

			/* All non-blocking I/Q readouts have completed */
			taskflags &= ~IQ_READY_FLAG;		// clear flag
			handle_iq_data(grp_ptr);			// display I/Q data
		}
	}
}
	// irq_enable();
// 	int ret;
// 	// btn.port
// 	if(!device_is_ready(btn.port)){
// 		return 0;
// 	}

// 	if(!device_is_ready(inp.port)){
// 		return 0;
// 	}

// 	if(!gpio_is_ready_dt(&led)){
// 		return 0;
// 	}


// 	ret = gpio_pin_configure_dt(&btn, GPIO_OUTPUT);
// 	if(ret != 0){
// 		return ret;
// 	}

// 	ret = gpio_pin_configure_dt(&inp, GPIO_INPUT);
// 	if(ret != 0){
// 		return ret;
// 	}

// 	ret = gpio_pin_interrupt_configure_dt(&inp, GPIO_INT_EDGE_RISING);
// 	if(ret != 0){
// 		return ret;
// 	}

// 	gpio_init_callback(&btn_cb_data, gpio_cb, BIT(inp.pin));

// 	gpio_add_callback(inp.port, &btn_cb_data);
	

// 	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
// 	if(ret<0){
// 		return 0;
// 	}

// 	if (!device_is_ready(iicm.bus)) {
// 		printk("I2C Main bus %s is not ready!\n\r",iicm.bus->name);
// 		return 0;
// 	}

// 	if (!device_is_ready(iicc.bus)) {
// 		printk("I2C Conf bus %s is not ready!\n\r",iicc.bus->name);
// 		return 0;
// 	}

// 	// uint8_t config[2] = {0x03,0x8C};
// 	int counter = 0;
// 	while (1)
// 	{
		
// 		// ret = gpio_pin_toggle_dt(&led);
// 		// if(ret<0){
// 		// 	return 0;
// 		// }

// 		ret = i2c_write_dt(&iicm, config, sizeof(config));
// 		// if(ret != 0){
// 		// 	printk("Failed to write to I2C Main device address %x at reg. %x n", iicm.addr,config[0]);
// 		// }

// 		// ret = i2c_write_dt(&iicc, config, sizeof(config));
// 		// if(ret != 0){
// 		// 	printk("Failed to write to I2C Conf device address %x at reg. %x n", iicc.addr,config[0]);
// 		// }

// 		// printk("Hello World! %s\n", CONFIG_BOARD);

// 		gpio_pin_set(btn.port, btn.pin, 0);
// 		k_msleep(100);
// 		gpio_pin_set(btn.port, btn.pin, 1);
// 		k_msleep(900);
// 		printk("ONE_LOOP_COMPLETED: %d\n\r", counter);

// 		if(counter == 10){
// 			// Disable interrupt
// 			gpio_init_callback(&btn_cb_data, gpio_cb2, BIT(inp.pin));
// 			gpio_add_callback(inp.port, &btn_cb_data);
// 		}
// 		else if(counter == 16){
// 			// Enable interrupt
// 			gpio_pin_interrupt_configure_dt(&inp, GPIO_INT_EDGE_FALLING);
// 		}
// 		counter++;
// 	}
// 	return 0;
// }

