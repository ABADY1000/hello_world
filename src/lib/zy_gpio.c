
#include "../inc/zy_gpio.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "../inc/soniclib.h"


static struct gpio_dt_spec ch_prg;
static struct gpio_dt_spec ch_rst;
static struct gpio_dt_spec ch_int;

ch_io_int_callback_t int_cb;
void zy_int_cb(const struct device *port, struct gpio_callback *cb, uint32_t pin){
    if(int_cb != NULL){
        int_cb(NULL, 0);
    }
}


int zy_gpio_init_all(){
    zy_gpio_init(&ch_int, ZY_GPIO_INTERRUPT_ENABLE, ZY_GPIO_INPUT, ZY_GPIO_EDGE_R, NULL);
    zy_gpio_init(&ch_rst, ZY_GPIO_INTERRUPT_DISABLE, ZY_GPIO_OUTPUT, 0, NULL);
    zy_gpio_init(&ch_prg, ZY_GPIO_INTERRUPT_DISABLE, ZY_GPIO_OUTPUT, 0, NULL);
}

int zy_gpio_init(struct gpio_dt_spec *dev, zy_gpio_interrupt_e gpio_int, zy_gpio_direction_e dir, zy_gpio_int_type_e int_type, gpio_callback_handler_t int_cb){
    
    if(!device_is_ready(dev->port)){
		return 0;
	}

    int ret;

    // Setting pin direction
    int gpio_dir = dir == ZY_GPIO_INPUT ? GPIO_INPUT : GPIO_OUTPUT;
    ret = gpio_pin_configure_dt(dev, gpio_dir);
	if(ret != 0){
		return ret;
	} 

    // Setting pin interrupt
    if(gpio_int == ZY_GPIO_INTERRUPT_ENABLE && dir == ZY_GPIO_INPUT){

        int gpio_int_type = int_type == ZY_GPIO_EDGE_F ? GPIO_INT_EDGE_FALLING : GPIO_INT_EDGE_RISING;
        ret = gpio_pin_interrupt_configure_dt(dev, gpio_int_type);
        if(ret != 0){
            return ret;
        }

        struct gpio_callback *btn_cb_data = (struct gpio_callback *)k_malloc(sizeof(struct gpio_callback));
        gpio_init_callback(btn_cb_data, int_cb, BIT(dev->pin));
        gpio_add_callback(dev->port, btn_cb_data);
    }
}

int zy_gpio_write_prg(uint8_t val){
    gpio_pin_set(ch_prg.port, ch_prg.pin, val);
    return 0;
}

int zy_gpio_write_rst(uint8_t val){
    gpio_pin_set(ch_rst.port, ch_rst.pin, val);
    return 0;
}

int zy_gpio_write_int(uint8_t val){
    gpio_pin_set(ch_int.port, ch_int.pin, val);
    return 0;
}

int zy_gpio_set_int_dir(zy_gpio_direction_e dir){
    if(dir == ZY_GPIO_INPUT){
        gpio_pin_configure_dt(&ch_int, GPIO_INPUT);
    }
    else{
        gpio_pin_configure_dt(&ch_int, GPIO_OUTPUT);
    }
}

int zy_gpio_set_prg_dir(zy_gpio_direction_e dir){
    if(dir == ZY_GPIO_INPUT){
        gpio_pin_configure_dt(&ch_int, GPIO_INPUT);
    }
    else{
        gpio_pin_configure_dt(&ch_int, GPIO_OUTPUT);
    }
}

int zy_gpio_set_rst_dir(zy_gpio_direction_e dir){
    if(dir == ZY_GPIO_INPUT){
        gpio_pin_configure_dt(&ch_int, GPIO_INPUT);
    }
    else{
        gpio_pin_configure_dt(&ch_int, GPIO_OUTPUT);
    }
}

int zy_gpio_int_enable_int(zy_gpio_int_type_e int_typ){
    gpio_pin_configure_dt(&ch_int, int_typ);
}

int zy_gpio_int_disable_int(){
    gpio_pin_configure_dt(&ch_int, GPIO_INT_DISABLE);
}

int zy_gpio_set_int_cb(ch_io_int_callback_t int_cb){

}