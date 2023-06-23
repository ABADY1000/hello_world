/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>

// Driver includes
// #include "inc/soniclib.h"
// #include "chirp_board_config.h"

#define SLEEP_TIME_MS 1000
#define LED0_NODE DT_ALIAS(led3)
#define I2C_MAIN_NODE DT_NODELABEL(chrip)
#define I2C_CONF_NODE DT_NODELABEL(chrip_conf)
#define GPIO DT_NODELABEL(&gpio)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios); 
static const struct i2c_dt_spec iicm = I2C_DT_SPEC_GET(I2C_MAIN_NODE);
static const struct i2c_dt_spec iicc = I2C_DT_SPEC_GET(I2C_CONF_NODE);

static const struct gpio_dt_spec btn = GPIO_DT_SPEC_GET(DT_NODELABEL(but), gpios);
static const struct gpio_dt_spec inp = GPIO_DT_SPEC_GET(DT_NODELABEL(int_inp), gpios);

static struct gpio_callback btn_cb_data;

void gpio_cb(const struct device *port, struct gpio_callback *cb, uint32_t pin){
	printk("INSIDE INTERRUPT\n\rPORT:%s\n\rPIN:%d\n\r", port->name, pin);
}

// gpio_callback_handler_t

int main(void)
{

	irq_enable();
	int ret;
	// btn.port
	if(!device_is_ready(btn.port)){
		return 0;
	}

	if(!device_is_ready(inp.port)){
		return 0;
	}

	if(!gpio_is_ready_dt(&led)){
		return 0;
	}


	ret = gpio_pin_configure_dt(&btn, GPIO_OUTPUT);
	if(ret != 0){
		return ret;
	}

	ret = gpio_pin_configure_dt(&inp, GPIO_INPUT);
	if(ret != 0){
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&inp, GPIO_INT_EDGE_RISING);
	if(ret != 0){
		return ret;
	}

	gpio_init_callback(&btn_cb_data, gpio_cb, BIT(inp.pin));

	gpio_add_callback(inp.port, &btn_cb_data);
	

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if(ret<0){
		return 0;
	}

	if (!device_is_ready(iicm.bus)) {
		printk("I2C Main bus %s is not ready!\n\r",iicm.bus->name);
		return 0;
	}

	if (!device_is_ready(iicc.bus)) {
		printk("I2C Conf bus %s is not ready!\n\r",iicc.bus->name);
		return 0;
	}

	uint8_t config[2] = {0x03,0x8C};
	while (1)
	{
		
		// ret = gpio_pin_toggle_dt(&led);
		// if(ret<0){
		// 	return 0;
		// }

		// ret = i2c_write_dt(&iicm, config, sizeof(config));
		// if(ret != 0){
		// 	printk("Failed to write to I2C Main device address %x at reg. %x n", iicm.addr,config[0]);
		// }

		// ret = i2c_write_dt(&iicc, config, sizeof(config));
		// if(ret != 0){
		// 	printk("Failed to write to I2C Conf device address %x at reg. %x n", iicc.addr,config[0]);
		// }

		// printk("Hello World! %s\n", CONFIG_BOARD);

		gpio_pin_set(btn.port, btn.pin, 0);
		k_msleep(100);
		gpio_pin_set(btn.port, btn.pin, 1);
		k_msleep(900);
		printk("ONE_LOOP_COMPLETED\n\r");
	}
	return 0;
}

