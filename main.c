// For work with Lcard E-502 device
#include "e502api.h"
// --------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include <libconfig.h>

#define TCP_CONNECTION_TOUT 5000


#define READ_CONFIG_SUCCESS 1 
#define READ_CONFIG_ERROR 0

typedef unsigned int uint;

typedef struct {
        uint channel_count;
        double adc_freq;
        double din_freq;
        uint read_block_size;
        uint read_timeout;
        uint *channel_numbers;
        uint *channel_modes;
        uint *channel_ranges;
    } module_config;


// Global variable for the correct 
// completion of data collection
int g_stop = 0;

// Signal of completion
void abort_handler(int sig) {
    g_stop = 1;
}

/*
    Get all Lcard E-502 devices connected via USB 
    or Ethernet interfaces

    Return number of found debices 
*/
uint32_t get_all_devrec(t_x502_devrec **pdevrec_list,
                        uint32_t *ip_addr_list,
                        unsigned ip_cnt)
{
    // number of found devices
    int32_t fnd_devcnt = 0;
    // number of found devices connected via USB interface
    uint32_t usb_devcnt = 0;

    t_x502_devrec *rec_list = NULL;

    // getting count of connected devices via USB interface
    E502_UsbGetDevRecordsList(NULL, 0, 0, &usb_devcnt);

    if(usb_devcnt + ip_cnt == 0){ return 0; }

    // allocate memory for the array to save the number of records found
    rec_list = malloc((usb_devcnt + ip_cnt) * sizeof(t_x502_devrec));
    
    // if memory wasn't allocated
    if(rec_list == NULL){ return 0; }

    if (usb_devcnt!=0) 
    {
        int32_t res = E502_UsbGetDevRecordsList(&rec_list[fnd_devcnt],
                                                usb_devcnt, 
                                                0, 
                                                NULL);
        if (res >= 0) { fnd_devcnt += res; }
    }
    
    for (int i=0; i < ip_cnt; i++) 
    {
        if (E502_MakeDevRecordByIpAddr(&rec_list[fnd_devcnt],
                                       ip_addr_list[i],
                                       0,
                                       TCP_CONNECTION_TOUT)
            == X502_ERR_OK // E502_MakeDevRecordByIpAddr(...) == X502_ERR_OK 
            ) 
        {
            fnd_devcnt++;
        }
    }

    // if were some mistake and no one modele
    // was getting
    if(fnd_devcnt == 0)
    {
        free(rec_list);
        return 0;
    }
    // --------------------------------------

    *pdevrec_list = rec_list;

    return fnd_devcnt;
}

/*
    Create connecting for selecting device.

    Return special handler (t_x502_hnd)
*/
t_x502_hnd open_device(t_x502_devrec *devrec_list,
                        uint32_t device_id)
{
    t_x502_hnd hnd = X502_Create();

    if (hnd==NULL) 
    {
        fprintf(stderr, "Ошибка создания описателя модуля!");
        return hnd;
    }
    
    /* устанавливаем связь с модулем по записи */
    int32_t err = X502_OpenByDevRecord(hnd, &devrec_list[device_id]);
    if (err != X502_ERR_OK) 
    {
        fprintf(stderr,
                "Ошибка установления связи с модулем: %s!",
                X502_GetErrorString(err));

        X502_Free(hnd);
        hnd = NULL;
        return hnd;
    }

    return hnd;
}

void create_stop_event_handler()
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = abort_handler;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
}

/* 
    Print info about module 

    Return error index
*/  
uint32_t print_info_about_module(t_x502_hnd *hnd)
{
    t_x502_info info;
    uint32_t err = X502_GetDevInfo(hnd, &info);
    if (err != X502_ERR_OK) {
        fprintf(stderr, 
                "Ошибка получения серийного информации о модуле: %s!",
                 X502_GetErrorString(err)
                 );
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

/*
    Create config for module. If there are mistakes in
    comand line arguments read e502monitor.cfg

    Return error index
*/
int create_config(module_config *mc)
{
    config_t cfg; 

    config_init(&cfg);

    if(!config_read_file(&cfg, "e502monitor.cfg"))
    {
        fprintf(stderr, "%s:%d - %s\n",  config_error_file(&cfg),
                config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);

        return READ_CONFIG_ERROR;
    }

    int found_value;
    int err = config_lookup_int(&cfg, "channel_count", &mc->channel_count);
    if(err == CONFIG_FALSE)
    { 
        printf("Ошибка конфигурационного файла:\tколичество каналов не задано!");
        config_destroy(&cfg);
        return READ_CONFIG_ERROR;
    }

    err = config_lookup_float(&cfg, "adc_freq", &(mc->adc_freq));
    if(err == CONFIG_FALSE)
    { 
        printf("Ошибка конфигурационного файла:\tчастота сбора АЦП не задана!");
        config_destroy(&cfg);
        return READ_CONFIG_ERROR;
    }
    
    err = config_lookup_float(&cfg, "din_freq", &(mc->din_freq));
    if(err == CONFIG_FALSE)
    { 
        printf("Ошибка конфигурационного файла:\tчастота синхронного входа не задана!");
        config_destroy(&cfg);
        return READ_CONFIG_ERROR;
    }
    
    err = config_lookup_int(&cfg, "read_block_size", &(mc->read_block_size));
    if(err == CONFIG_FALSE)
    { 
        printf("Ошибка конфигурационного файла:\tразмер блока для чтения не задан!");
        config_destroy(&cfg);
        return READ_CONFIG_ERROR;
    }
    
    err = config_lookup_int(&cfg, "read_timeout", &(mc->read_timeout));
    if(err == CONFIG_FALSE)
    { 
        printf("Ошибка конфигурационного файла:\tвремя задержки перед чтением блока не задано!");
        config_destroy(&cfg);
        return READ_CONFIG_ERROR; 
    }

    config_setting_t *channel_numbers = config_lookup(&cfg, "channel_numbers");
    if(channel_numbers == NULL)
    {
        printf("Ошибка конфигурационного файла:\tномера каналов не заданы!");
        config_destroy(&cfg);
        return READ_CONFIG_ERROR;
    }

    mc->channel_numbers = (uint*)malloc(sizeof(uint)*mc->channel_count);

    for(int i = 0; i < mc->channel_count; i++)
    {
        mc->channel_numbers[i] = config_setting_get_int_elem(channel_numbers, i);
    }

    config_setting_t *channel_modes = config_lookup(&cfg, "channel_modes");
    if(channel_modes == NULL)
    {
        printf("Ошибка конфигурационного файла:\tрежимы каналов не заданы!");
        config_destroy(&cfg);
        return READ_CONFIG_ERROR;
    }

    mc->channel_modes = (uint*)malloc(sizeof(uint)*mc->channel_count);
    
    for(int i = 0; i < mc->channel_count; i++)
    {
        mc->channel_modes[i] = config_setting_get_int_elem(channel_modes, i);
    }

    config_setting_t *channel_ranges = config_lookup(&cfg, "channel_ranges");
    if(channel_ranges == NULL)
    {
        printf("Ошибка конфигурационного файла:\tдиапазоны каналов не заданы!");
        config_destroy(&cfg);
        return READ_CONFIG_ERROR;
    }

    mc->channel_ranges = (uint*)malloc(sizeof(uint)*mc->channel_count);

    for(int i = 0; i < mc->channel_count; i++)
    {
        mc->channel_ranges[i] = config_setting_get_int_elem(channel_ranges, i);
    }

    config_destroy(&cfg);

    return READ_CONFIG_SUCCESS;
}

void print_module_config(module_config *mc)
{
    printf("\nЗагружена следующая конфигурация модуля:\n");
    printf(" Количество используемых логических каналов\t\t:%d\n", mc->channel_count);
    printf(" Частота сбора АЦП в Гц\t\t\t\t\t:%f\n", mc->adc_freq);
    printf(" Частота синхронного ввода в Гц\t\t\t\t:%f\n", mc->din_freq);
    printf(" Количество отсчетов, считываемых за блок\t\t:%d\n", mc->read_block_size);
    printf(" Таймаут перед считываением блока (мс)\t\t\t:%d\n", mc->read_timeout);
    
    printf(" Номера используемых каналов\t\t\t\t:[ ");
    for(int i = 0; i<mc->channel_count; i++)
    {
        printf("%d ", mc->channel_numbers[i]);
    }
    printf("]\n");

    printf(" Режимы измерения каналов\t\t\t\t:[ ");
    for(int i = 0; i<mc->channel_count; i++)
    {
        printf("%d ", mc->channel_modes[i]);
    }
    printf("]\n");

    printf(" Диапазоны измерения каналов\t\t\t\t:[ ");
    for(int i = 0; i<mc->channel_count; i++)
    {
        printf("%d ", mc->channel_ranges[i]);
    }
    printf("]\n");
    
}

#define CFG_ERR 1 

uint32_t configure_module(t_x502_hnd *hnd,
                          module_config *mc)
{
    int32_t err = X502_SetLChannelCount(hnd, mc->channel_count);

    if(err != X502_ERR_OK){ return CFG_ERR; }

    for(int i = 0; i < mc->channel_count; ++i)
    {
        err = X502_SetLChannel( hnd,
                                i,
                                mc->channel_numbers[i],
                                mc->channel_modes[i],
                                mc->channel_ranges,
                                0 );
    }

}

int main(int argc, char** argv) 
{
    create_stop_event_handler();

    uint32_t ver = X502_GetLibraryVersion();
    printf("Версия библиотеки: %d.%d.%d\n",
            (ver >> 24)&0xFF,
            (ver>>16)&0xFF,
            (ver>>8)&0xFF
          );

    uint32_t *ip_addr_list = NULL;
    uint32_t ip_cnt = 0;
    uint32_t fnd_devcnt = 0;

    t_x502_devrec *devrec_list = NULL;

    fnd_devcnt = get_all_devrec(&devrec_list, ip_addr_list, ip_cnt);

    if (fnd_devcnt == 0) {
        printf("Не найдено ни одного модуля. Завершение.\n");
        
        //exit from program
        return 0;
    }
    
    printf("Доступны следующие модули:\n");
    for (int i=0; i < fnd_devcnt; i++) {
        printf("Module № %d: %s, %-9s", i, devrec_list[i].devname,
               devrec_list[i].iface == X502_IFACE_PCI ? "PCI/PCIe" :
               devrec_list[i].iface == X502_IFACE_USB ? "USB" :
               devrec_list[i].iface == X502_IFACE_ETH ? "Ethernet" : "?");
        if (devrec_list[i].iface != X502_IFACE_ETH) {
            printf("Серийные номер: %s\n", devrec_list[i].serial);
        } else {
            printf("Адрес: %s\n", devrec_list[i].location);
        }
    }

    printf("Введите номер модуля, с которым хотите работать (от 0 до %d)\n", fnd_devcnt-1);
    fflush(stdout);

    uint32_t device_id = -1;

    scanf("%d", &device_id);

    if( device_id < 0 || device_id >= fnd_devcnt)
    {
        printf("\nНеверный номер модуля! Завершение.\n");

        // Free memory
        X502_FreeDevRecordList(devrec_list, fnd_devcnt);
        free(devrec_list);
        // --------------------------------------------

        return 0; // exit from program
    }

    t_x502_hnd hnd = open_device(devrec_list, device_id);

    if(hnd == NULL)
    {
        // Free memory
        X502_FreeDevRecordList(devrec_list, fnd_devcnt);
        free(devrec_list);
        // --------------------------------------------

        return 0;
    }

    print_info_about_module(hnd);

    module_config *mc = (module_config*)malloc(sizeof(module_config));
    mc->channel_numbers = NULL;
    mc->channel_modes   = NULL;
    mc->channel_ranges  = NULL;

    if(create_config(mc) == READ_CONFIG_ERROR)
    {
        if(mc->channel_numbers != NULL){ free(mc->channel_numbers); }
        if(mc->channel_modes   != NULL){ free(mc->channel_modes); }
        if(mc->channel_ranges  != NULL){ free(mc->channel_ranges); }

        free(mc);
        
        X502_Close(hnd);
        X502_Free(hnd);

        printf("\nОшибка при создании конфигурации модуля! Заверешение.\n");

        return 0;
    }
    
    print_module_config(mc);

    if( configure_module(hnd, mc) == CFG_ERR)
    {
        printf("\nОшибка при конфигурировании модуля! Завершение.\n");

        free(mc->channel_numbers);
        free(mc->channel_ranges);
        free(mc->channel_modes);

        free(mc);

        X502_Close(hnd);
        X502_Free(hnd);

        return 0;
    }

    free(mc->channel_numbers);
    free(mc->channel_ranges);
    free(mc->channel_modes);

    free(mc);

    X502_Close(hnd);
    X502_Free(hnd);

    return 0;
}


