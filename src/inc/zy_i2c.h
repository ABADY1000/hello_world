#ifndef _ZY_I2C_
#define _ZY_I2C_

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>

#define CHIRP_I2C      DT_NODELABEL(chirp)
#define CHIRP_CONF_I2C DT_NODELABEL(chirp_conf)

int zy_i2c_init();

int zy_i2c_send_chirp(char* const msg, uint8_t size);
int zy_i2c_send_chirp_conf(char* const msg, uint8_t size);
int zy_i2c_recv_chirp(char* msg, uint8_t size);
int zy_i2c_recv_chirp_conf(char* msg, uint8_t size);

uint8_t zy_i2c_chirp_address();
uint8_t zy_i2c_chirp_conf_address();

#endif // _ZY_I2C_
