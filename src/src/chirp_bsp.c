
/*
    Abdullah Alamoodi's implementation for "chirp_bsp.h" functions
    which is necessary for the driver of chirp sensors
*/

#include "chirp_bsp.h"

/*
    TODO:
#include "sleep.h"
#include "i2c.h"
#include "gpio.h"
#include "interrupt.h"
#include "uart.h"
*/

void chbsp_board_init(ch_group_t *grp_ptr);
void chbsp_reset_assert(void);
void chbsp_reset_release(void);
void chbsp_program_enable(ch_dev_t *dev_ptr);
void chbsp_program_disable(ch_dev_t *dev_ptr);
void chbsp_group_set_io_dir_out(ch_group_t *grp_ptr);
void chbsp_group_set_io_dir_in(ch_group_t *grp_ptr);
void chbsp_group_pin_init(ch_group_t *grp_ptr);
void chbsp_group_io_clear(ch_group_t *grp_ptr);
void chbsp_group_io_set(ch_group_t *grp_ptr);
void chbsp_group_io_interrupt_enable(ch_group_t *grp_ptr);
void chbsp_io_interrupt_enable(ch_dev_t *dev_ptr);
void chbsp_group_io_interrupt_disable(ch_group_t *grp_ptr);
void chbsp_io_interrupt_disable(ch_dev_t *dev_ptr);
void chbsp_io_callback_set(ch_io_int_callback_t callback_func_ptr);
void chbsp_delay_us(uint32_t us);
void chbsp_delay_ms(uint32_t ms);
int chbsp_i2c_init(void);
uint8_t chbsp_i2c_get_info(ch_group_t *grp_ptr, uint8_t dev_num, ch_i2c_info_t *info_ptr);
int chbsp_i2c_write(ch_dev_t *dev_ptr, uint8_t *data, uint16_t num_bytes);
int chbsp_i2c_mem_write(ch_dev_t *dev_ptr, uint16_t mem_addr, uint8_t *data, uint16_t num_bytes);
int chbsp_i2c_read(ch_dev_t *dev_ptr, uint8_t *data, uint16_t num_bytes);
int chbsp_i2c_mem_read(ch_dev_t *dev_ptr, uint16_t mem_addr, uint8_t *data, uint16_t num_bytes);