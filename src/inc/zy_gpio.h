

#ifndef _ZY_GPIO_
#define _ZY_GPIO_

#include "../inc/soniclib.h"

typedef enum zy_gpio_interrupt{ 
    ZY_GPIO_INTERRUPT_DISABLE,
    ZY_GPIO_INTERRUPT_ENABLE
} zy_gpio_interrupt_e;

typedef enum zy_gpio_direction {
    ZY_GPIO_INPUT,
    ZY_GPIO_OUTUPUT
} zy_gpio_direction_e;

typedef enum zy_gpio_int_type {
    ZY_GPIO_EDGE_R,
    ZY_GPIO_EDGE_F
} zy_gpio_int_type_e;

int zy_gpio_init_all();
int zy_gpio_init(
    struct gpio_dt_spec *dev, 
    zy_gpio_interrupt_e gpio_int,
    zy_gpio_direction_e dir,
    zy_gpio_int_type_e int_type,
    gpio_callback_handler_t int_cb
    );

int zy_gpio_write_prg(uint8_t val);
int zy_gpio_write_rst(uint8_t val);
int zy_gpio_write_int(uint8_t val);
int zy_gpio_set_int_dir(zy_gpio_direction_e dir);
int zy_gpio_set_prg_dir(zy_gpio_direction_e dir);
int zy_gpio_set_rst_dir(zy_gpio_direction_e dir);
int zy_gpio_int_enable_int(zy_gpio_int_type_e int_typ);
int zy_gpio_int_disable_int();
int zy_gpio_set_int_cb(ch_io_int_callback_t int_cb);


#endif // _ZY_GPIO_