

#include "zy_i2c.h"
#include "zephyr/kernel.h"

static const struct i2c_dt_spec iicm = I2C_DT_SPEC_GET(I2C_MAIN_NODE);
static const struct i2c_dt_spec iicc = I2C_DT_SPEC_GET(I2C_CONF_NODE);

int zy_i2c_init(){
    
    if (!device_is_ready(iicm.bus)) {
		printk("I2C Main bus %s is not ready!\n\r",iicm.bus->name);
		return 0;
	}

	if (!device_is_ready(iicc.bus)) {
		printk("I2C Conf bus %s is not ready!\n\r",iicc.bus->name);
		return 0;
	}

    return 1;
}

int zy_i2c_send_chirp(char* const msg, uint8_t size){

    ret = i2c_write_dt(&iicm, msg, size);
    if(ret != 0){
        printk("Failed to write to I2C Conf device address %x at reg. %x n", iicm.addr,config[0]);
    }

    return ret;
}

int zy_i2c_send_chirp_conf(char* const msg, uint8_t size){

    ret = i2c_write_dt(&iicc, msg, size);
    if(ret != 0){
        printk("Failed to write to I2C Conf device address %x at reg. %x n", iicc.addr,config[0]);
    }
    
    return ret;
}

uint8_t zy_i2c_chirp_address(){
    return iicm->address;
}

uint8_t zy_i2c_chirp_conf_address(){
    return iicc->address;
}

int zy_i2c_recv_chirp(char* msg, uint8_t size){
    return i2c_read_dt(iicm, msg, size);
}

int zy_i2c_recv_chirp_conf(char* msg, uint8_t size){
    return i2c_read_dt(iicc, msg, size);
}

