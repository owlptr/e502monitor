/*
    This file part of e502monitor source code.
    Licensed under GPLv3.

    "device.c" contains declaration of function
    for working with Lard E502.
    
    Author: Gapeev Maksim
    Email: gm16493@gmail.com
*/

#include "device.h"
#include "common.h"
#include "logging.h"

uint32_t get_usb_devrec(t_x502_devrec **devrec_list)
{
    // number of found devices
    int32_t fnd_devcnt = 0;
    // number of found devices connected via USB interface
    uint32_t usb_devcnt = 0;

    t_x502_devrec *rec_list = NULL;

    // get count of connected devices via USB interface
    E502_UsbGetDevRecordsList(NULL, 0, 0, &usb_devcnt);

    if(usb_devcnt == 0){ return 0; }

    // allocate memory for the array to save the number of records found
    rec_list = malloc((usb_devcnt) * sizeof(t_x502_devrec));
    
    // if memory wasn't allocated
    if(rec_list == NULL){ return 0; }

    if (usb_devcnt != 0) 
    {
        int32_t res = E502_UsbGetDevRecordsList(&rec_list[fnd_devcnt],
                                                usb_devcnt, 
                                                0, 
                                                NULL);
        if (res >= 0) { fnd_devcnt += res; }
    }

    // if there were some mistake and 
    // no one device was getting
    if(fnd_devcnt == 0)
    {
        free(rec_list);
        return 0;
    }
    // --------------------------------------

    *devrec_list = rec_list;

    return fnd_devcnt;
}

t_x502_hnd open_device( t_x502_devrec *devrec_list,
                        uint32_t device_id )
{
    t_x502_hnd hnd = X502_Create();

    if (hnd == NULL) 
    {
        printf("Ошибка создания описателя модуля!");
        logg("Ошибка создания описателя модуля");
        
        return hnd;
    }
    
    /* create connection with device */
    int32_t err = X502_OpenByDevRecord(hnd, &devrec_list[device_id]);
    if (err != X502_ERR_OK) 
    {
        printf("Ошибка установления связи с модулем: %s!",
                X502_GetErrorString(err));
        logg("Ошибка установления связи с модулем");

        X502_Free(hnd);
        hnd = NULL;
        return hnd;
    }

    return hnd;
}

int print_info_about_device(t_x502_hnd *device_hnd)
{
    t_x502_info info;
    uint32_t err = X502_GetDevInfo(device_hnd, &info);
    if (err != X502_ERR_OK) {
        printf("Ошибка получения серийной информации о модуле: %s!",
               X502_GetErrorString(err) 
              );
        logg("Ошибка получения серийной информации о модуле");
        return err;
    } 

    printf("Установлена связь со следующим модулем:\n");
    printf(" Серийный номер\t\t\t: %s\n", info.serial);
    
    printf(" Наличие ЦАП\t\t\t: %s\n",
           info.devflags & X502_DEVFLAGS_DAC_PRESENT ? "Да" : "Нет");
    
    printf(" Наличие BlackFin\t\t: %s\n",
           info.devflags & X502_DEVFLAGS_BF_PRESENT ? "Да" : "Нет");
    
    printf(" Наличие гальваноразвязки\t: %s\n",
           info.devflags & X502_DEVFLAGS_GAL_PRESENT ? "Да" : "Нет");
    
    printf(" Индустриальное исп.\t\t: %s\n",
           info.devflags & X502_DEVFLAGS_INDUSTRIAL ? "Да" : "Нет");
    
    printf(" Наличие интерф. PCI/PCIe\t: %s\n",
           info.devflags & X502_DEVFLAGS_IFACE_SUPPORT_PCI ? "Да" : "Нет");
    
    printf(" Наличие интерф. USB\t\t: %s\n",
           info.devflags & X502_DEVFLAGS_IFACE_SUPPORT_USB ? "Да" : "Нет");
    
    printf(" Наличие интерф. Ethernet\t: %s\n",
           info.devflags & X502_DEVFLAGS_IFACE_SUPPORT_ETH ? "Да" : "Нет");
    
    printf(" Версия ПЛИС\t\t\t: %d.%d\n",
           (info.fpga_ver >> 8) & 0xFF,
            info.fpga_ver & 0xFF);
    printf(" Версия PLDA\t\t\t: %d\n", info.plda_ver);
    
    if (info.mcu_firmware_ver != 0) {
        printf(" Версия прошивки ARM\t\t: %d.%d.%d.%d\n",
               (info.mcu_firmware_ver >> 24) & 0xFF,
               (info.mcu_firmware_ver >> 16) & 0xFF,
               (info.mcu_firmware_ver >>  8) & 0xFF,
                info.mcu_firmware_ver & 0xFF);
    }

    return err;
}

int configure_device(t_x502_hnd *device_hnd, e502monitor_config *config)
{
    int32_t err = X502_SetLChannelCount(device_hnd, config->channel_count);

    if(err != X502_ERR_OK){ return E502M_ERR; }

    for(int i = 0; i < config->channel_count; ++i)
    {
        err = X502_SetLChannel( device_hnd,
                                i,
                                config->channel_numbers[i],
                                config->channel_modes[i],
                                config->channel_ranges[i],
                                0 );

        if(err != X502_ERR_OK){ return E502M_ERR; }
    }

    double frame_freq = config->adc_freq/config->channel_count;

    // err = X502_SetAdcFreq(device_hnd, &config->adc_freq, &frame_freq);
    // err = X502_SetAdcFreq(device_hnd, &config->adc_freq, NULL);
    // if(err != X502_ERR_OK){ return E502M_ERR; }

    // calculate configuration value
    int points_per_channel = MAX_FREQUENCY / (int)config->adc_freq;

    printf("Points per channel: %d\n", points_per_channel);
    printf("Channel counts: %d\n", config->channel_count);

    uint32_t divider = (int)(points_per_channel / config->channel_count);
    uint32_t delay = (int)(points_per_channel % config->channel_count);
        
    printf("Divider: %d\n", divider);
    printf("Delay: %d\n", delay);
    
    X502_SetAdcFreqDivider(device_hnd, divider);
    X502_SetAdcInterframeDelay(device_hnd, delay);

    // X502_SetAdcFreqDivider(device_hnd, divider);
    // X502_SetAdcInterframeDelay(device_hnd, delay);

    // double f_acq, f_frame;
    // X502_GetAdcFreq(device_hnd, &f_acq, &f_frame);
    // printf("Real freq: %d \t %d\n", f_acq, f_frame);

    //what we realy set...
    // printf("\nУстановлены частоты:\n"
    //        " Частота сбора АЦП\t\t:%0.0f\n"
    //        " Частота на лог. канал\t\t:%0.0f\n",
    //     //    config->adc_freq, frame_freq);
    //         config->adc_freq, (config->adc_freq / config->channel_count));

    err = X502_Configure(device_hnd, 0);
    if(err != X502_ERR_OK){ return E502M_ERR; }

    err = X502_StreamsEnable(device_hnd, X502_STREAM_ADC);
    if(err != X502_ERR_OK){ return E502M_ERR; }

    return E502M_ERR_OK;
}

void print_available_devices(t_x502_devrec *devrec_list, uint32_t device_count)
{
    printf("Доступны следующие модули:\n");
    for (int i=0; i < device_count; i++) {
        printf("Модуль № %d: %s, %-9s", i, devrec_list[i].devname,
               devrec_list[i].iface == X502_IFACE_PCI ? "PCI/PCIe" :
               devrec_list[i].iface == X502_IFACE_USB ? "USB" :
               devrec_list[i].iface == X502_IFACE_ETH ? "Ethernet" : "?");
        if (devrec_list[i].iface != X502_IFACE_ETH) {
            printf("Серийные номер: %s\n", devrec_list[i].serial);
        } else {
            printf("Адрес: %s\n", devrec_list[i].location);
        }
    }
}