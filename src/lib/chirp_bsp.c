
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


void chbsp_board_init(ch_group_t *grp_ptr){

    grp_ptr->num_ports = 1;
    grp_ptr->num_i2c_buses = 1;
    grp_ptr->rtc_cal_pulse_ms = 200;

    chbsp_program_enable();
    chbsp_delay_ms(2);
    uint8_t buffer[2];
    zy_i2c_recv_chirp_conf(buffer, 2);

    if(buffer[0] == 0x0A && buffer[1] == 0x02){
        printk("Chirp pattern detected successfully\n\r");
    }
    else{
        printk("Pattern is not detected in Chirp initialization\n\rpattern[0]:0x%x\n\rpattern[1]:0x%x", buffer[0], buffer[1]);
    }
}

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

int chbsp_i2c_init(void){
    zy_i2c_init();
}

uint8_t chbsp_i2c_get_info(ch_group_t *grp_ptr, uint8_t dev_num, ch_i2c_info_t *info_ptr){
    info_ptr->address = zy_i2c_chirp_address();
    info_ptr->bus_num = 0;
    printk("INSID:chbsp_i2c_get_info\n\r");
}

int chbsp_i2c_write(ch_dev_t *dev_ptr, uint8_t *data, uint16_t num_bytes){
    
    uint8_t address = ch_get_i2c_address(dev_ptr);
    if(address == zy_i2c_chirp_address()){
        zy_i2c_send_chirp(data, num_bytes);
    }
    else if(address == zy_i2c_chirp_conf_address()){
        zy_i2c_send_chirp_conf(data, num_bytes);
    }
    else{
        printk("Uknown address detected! 'i2c_write'\n\r");
        return 2;
    }
    return 0;
}

int chbsp_i2c_mem_write(ch_dev_t *dev_ptr, uint16_t mem_addr, uint8_t *data, uint16_t num_bytes){
    
    int size = num_bytes + 1;
    uint8_t msg[size];
    msg[0] = mem_addr;
    for(int i=1; i<=size; i++){
        msg[i] = data[i-1];
    }

    if(address == zy_i2c_chirp_address()){
        zy_i2c_send_chirp(msg, size);
    }
    else if(address == zy_i2c_chirp_conf_address()){
        zy_i2c_send_chirp_conf(msg, size);
    }
    else{
        printk("Uknown address detected! 'i2c_mem_write'\n\r");
        return 2;
    }

    return 0;
}

int chbsp_i2c_read(ch_dev_t *dev_ptr, uint8_t *data, uint16_t num_bytes){
    uint8_t address = ch_get_i2c_address();

    if(address == zy_i2c_chirp_address()){
        zy_i2c_recv_chirp(data, num_bytes);
    }
    else if(address == zy_i2c_chirp_conf_address()){
        zy_i2c_recv_chirp_conf(data, num_bytes);
    }
    else{
        printk("Uknown address detected! 'i2c_mem_write'\n\r");
        return 2;
    }
    return 0;
}

int chbsp_i2c_mem_read(ch_dev_t *dev_ptr, uint16_t mem_addr, uint8_t *data, uint16_t num_bytes){

    uint8_t msg[] = {mem_addr};
    
    if(address == zy_i2c_chirp_address()){
        zy_i2c_send_chirp(msg, sizeof(msg));
        zy_msleep(2);
        zy_i2c_recv_chirp(msg, num_bytes);
    }
    else if(address == zy_i2c_chirp_conf_address()){
        zy_i2c_send_chirp_conf(msg, sizeof(msg));
        zy_msleep(2);
        zy_i2c_recv_chirp_conf(msg, num_bytes);
    }
    else{
        printk("Uknown address detected! 'i2c_mem_write'\n\r");
        return 2;
    }
    return 0;
}

 