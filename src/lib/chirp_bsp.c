
/*
    Abdullah Alamoodi's implementation for "chirp_bsp.h" functions
    which is necessary for the driver of chirp sensors
*/

#include "chirp_bsp.h"
#include "../inc/zy_gpio.h"
#include "../inc/zy_i2c.h"
#include "../inc/zy_sleep.h"
#include "../inc/soniclib.h"
/*
    TODO:
#include "sleep.h"
#include "i2c.h"
#include "gpio.h"
#include "interrupt.h"
#include "uart.h"
*/

//void (*ch_io_int_callback_t)(ch_group_t *grp_ptr, uint8_t io_index);


void chbsp_board_init(ch_group_t *grp_ptr);

void chbsp_reset_assert(void){
    zy_gpio_write_prg(0);
}

void chbsp_reset_release(void){
    zy_gpio_write_prg(1);
}

void chbsp_program_enable(ch_dev_t *dev_ptr){
    // dev_ptr->io_index // This could be used later to identify the sensor between other sensors
    zy_gpio_write_prg(1);
}

void chbsp_program_disable(ch_dev_t *dev_ptr){
    // dev_ptr->io_index // This could be used later to identify the sensor between other sensors
    zy_gpio_write_prg(0);
}

void chbsp_group_set_io_dir_out(ch_group_t *grp_ptr){
    // grp_ptr->device // This could be used later
    zy_gpio_set_int_dir(ZY_GPIO_OUTUPUT);
}

void chbsp_group_set_io_dir_in(ch_group_t *grp_ptr){
    // grp_ptr->device // This could be used later
    zy_gpio_set_int_dir(ZY_GPIO_INPUT);
}

void chbsp_group_pin_init(ch_group_t *grp_ptr){
    /*
        Initialization
    */
    zy_gpio_set_prg_dir(ZY_GPIO_OUTUPUT);
    zy_gpio_set_rst_dir(ZY_GPIO_OUTUPUT);
    zy_gpio_set_int_dir(ZY_GPIO_INPUT);

    zy_gpio_write_prg(1);
    zy_gpio_write_rst(0);
}

void chbsp_group_io_clear(ch_group_t *grp_ptr){
    // grp_ptr->device // This could be used later
    zy_gpio_write_int(0);
}

void chbsp_group_io_set(ch_group_t *grp_ptr){
    // grp_ptr->device // This could be used later
    zy_gpio_write_int(1);
}

void chbsp_group_io_interrupt_enable(ch_group_t *grp_ptr){
    ch_dev_t* dev = grp_ptr->device[0];
    chbsp_io_interrupt_enable(dev);
}

void chbsp_io_interrupt_enable(ch_dev_t *dev_ptr){
    zy_gpio_int_enable_int(ZY_GPIO_EDGE_F); // TODO: Check correct direction
}

void chbsp_group_io_interrupt_disable(ch_group_t *grp_ptr){
    ch_dev_t* dev = grp_ptr->device[0];
    chbsp_io_interrupt_disable(dev);
}

void chbsp_io_interrupt_disable(ch_dev_t *dev_ptr){
    zy_gpio_int_disable_int();
}

void chbsp_io_callback_set(ch_io_int_callback_t callback_func_ptr){
    zy_gpio_set_int_cb(callback_func_ptr);
}

void chbsp_delay_us(uint32_t us){
    zy_usleep(us);
}

void chbsp_delay_ms(uint32_t ms){
    zy_msleep(ms);
}

int chbsp_i2c_init(void);
uint8_t chbsp_i2c_get_info(ch_group_t *grp_ptr, uint8_t dev_num, ch_i2c_info_t *info_ptr);
int chbsp_i2c_write(ch_dev_t *dev_ptr, uint8_t *data, uint16_t num_bytes);
int chbsp_i2c_mem_write(ch_dev_t *dev_ptr, uint16_t mem_addr, uint8_t *data, uint16_t num_bytes);
int chbsp_i2c_read(ch_dev_t *dev_ptr, uint8_t *data, uint16_t num_bytes);
int chbsp_i2c_mem_read(ch_dev_t *dev_ptr, uint16_t mem_addr, uint8_t *data, uint16_t num_bytes);