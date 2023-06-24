
#include "../inc/zy_sleep.h"
#include <zephyr/kernel.h>

void zy_usleep(uint32_t us){
    k_usleep(us);
}

void zy_msleep(uint32_t ms){
    k_msleep(ms);
}
void zy_sleep(uint32_t s){
    k_sleep(s);
}