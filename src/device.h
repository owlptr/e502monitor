/*
    "device.h" contains declaration of function
    for working with Lard E502  

    Author: Gapeev Maksim
*/
#ifndef DEVICE_H
#define DEVICE_H

#include "e502api.h"
#include "config.h"

#include <stdint.h>

/*
    Get all Lcard E-502 devices connected
    via USB interfaces

    devrec_list   - pointer to pointer on special

    t_x502_devrec -  structure. List of available 
    devices will be stored in it.

    Return number of found devices. 

    P.S. Please initialize *devrec_list only by NULL.
         Don't allocate memory. It will be allocated in
         this function.
*/
uint32_t get_usb_devrec(t_x502_devrec **devrec_list);

/*
    Create connection for selected device.

    devrec_list - list of available devices.
    device_id   - id of selected devices

    Return special t_x502_hnd heandle.

    If somthing was wrong function returns NULL.
*/
t_x502_hnd open_device(t_x502_devrec *devrec_list, uint32_t device_id);

/* 
    Print info about module 

    device_hnd - special device heandle

    Return error index
*/  
int print_info_about_module(t_x502_hnd *device_hnd);

/*
    Set configuration.

    device_hnd - LCard E502 heandle
    config     - configuration for setting

*/
int configure_module(t_x502_hnd *device_hnd, e502monitor_config *config);

#endif  // DEVICE_H