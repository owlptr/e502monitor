// For work with Lcard E-502 device
#include "e502api.h"
// --------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>

#include <libconfig.h>

#include "pdouble_queue.h"

// #define FILE_TIME 900 // create 15 minutes files... 15 min = 900 sec
#define FILE_TIME 30

#define TCP_CONNECTION_TOUT 5000

#define READ_CONFIG_OK 0 
#define READ_CONFIG_ERROR -1

#define CONFIGURE_OK 0
#define CONFIGURE_ERROR -2

#define WRITE_HEADERS_OK 0
#define WRITE_HEADERS_ERROR -3

#define EXECUTE_OK 0

#define CREATE_OUT_FILES_OK 0
#define CREATE_OUT_FILES_ERROR -4

// The structure that stores header for output bin files
typedef struct {
        uint16_t year;               // Year of creating bin file
        uint16_t month;              // Month of creating bin file
        uint16_t day;                // Day of start of the recording
        uint16_t start_hour;         // Hour of start of the recording
        uint16_t finish_hour;        // Hour of finish of the recording
        uint16_t start_minut;        // Minut of start of the recording
        uint16_t finish_minut;       // Minut of finish of the reocording
        uint16_t start_second;       // Second of start of the recording
        uint16_t finish_second;      // Second of finish of the recording
        uint16_t start_msecond;      // Milliseconds of start recording
        uint16_t finish_msecond;     // Milliseconds of finish recording
        uint16_t channel_number;     // Number of used channel 
        double   adc_freq;           // Frequancy of ADC
        uint8_t  mode;               // Operation mode 
        char*    module_name;        // Name of using ADC 
        uint8_t  channel_range;      // Channel measurement range
        char*    place_cordinates;   // Coordinates of the working place 
    } header;

// ---------------------GLOBAL VARIABLES---------------------------------

FILE** g_files;           // Descriptors of ouput binary files

header g_header; 

pdouble_queue*  g_pd_queue; // queue for store read data

struct timeval g_time_start; 
struct timeval g_time_end;

// startup config
int g_channel_count;               // Count of use logical chnnels
double g_adc_freq;                 // Frequency descritization
int g_read_block_size;             // The size of the data block 
                                   //  that is read at once from the ADC

int  g_read_timeout;               // Timeout for receiving data in ms.
int* g_channel_numbers = NULL;     // Numbers of using logical channels
int* g_channel_modes   = NULL;     // Operation modes for channels
int* g_channel_ranges  = NULL;     // Channel measurement range
char *g_bin_dir        = NULL;     // Directory of output bin files

t_x502_hnd* g_hnd = NULL; // module heandler

int g_stop = 0; // if equal 1 - stop working
//----------------------------------------------------------------------

// Signal of completion                 
void abort_handler(int sig){ g_stop = 1; }

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
    Get all Lcard E-502 devices connected via USB 
    or Ethernet interfaces

    Return number of found devices 
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

    // get count of connected devices via USB interface
    E502_UsbGetDevRecordsList(NULL, 0, 0, &usb_devcnt);

    if(usb_devcnt + ip_cnt == 0){ return 0; }

    // allocate memory for the array to save the number of records found
    rec_list = malloc((usb_devcnt + ip_cnt) * sizeof(t_x502_devrec));
    
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
    Create connecting for selecting device

    Return special handler (t_x502_hnd)
*/
t_x502_hnd open_device( t_x502_devrec *devrec_list,
                        uint32_t device_id )
{
    t_x502_hnd hnd = X502_Create();

    if (hnd == NULL) 
    {
        fprintf(stderr, "Ошибка создания описателя модуля!");
        return hnd;
    }
    
    /* create connection with module */
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

/* 
    Initalize variables, which represent startup config:
*/
int create_config()
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
    int err = config_lookup_int(&cfg, "channel_count", &g_channel_count);
    if(err == CONFIG_FALSE)
    { 
        printf("Ошибка конфигурационного файла:\tколичество каналов не задано!");
        config_destroy(&cfg);
        return READ_CONFIG_ERROR;
    }

    err = config_lookup_float(&cfg, "adc_freq", &g_adc_freq);
    if(err == CONFIG_FALSE)
    { 
        printf("Ошибка конфигурационного файла:\tчастота сбора АЦП не задана!");
        config_destroy(&cfg);
        return READ_CONFIG_ERROR;
    }
    
    err = config_lookup_int(&cfg, "read_block_size", &g_read_block_size);
    if(err == CONFIG_FALSE)
    { 
        printf("Ошибка конфигурационного файла:\tразмер блока для чтения не задан!");
        config_destroy(&cfg);
        return READ_CONFIG_ERROR;
    }
    
    err = config_lookup_int(&cfg, "read_timeout", &g_read_timeout);
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

    g_channel_numbers = (int*)malloc(sizeof(int)*g_channel_count);

    for(int i = 0; i < g_channel_count; i++)
    {
        g_channel_numbers[i] = config_setting_get_int_elem(channel_numbers, i);
    }

    config_setting_t *channel_modes = config_lookup(&cfg, "channel_modes");
    if(channel_modes == NULL)
    {
        printf("Ошибка конфигурационного файла:\tрежимы каналов не заданы!");
        config_destroy(&cfg);
        return READ_CONFIG_ERROR;
    }

    g_channel_modes = (int*)malloc(sizeof(int)*g_channel_count);
    
    for(int i = 0; i < g_channel_count; i++)
    {
        g_channel_modes[i] = config_setting_get_int_elem(channel_modes, i);
    }

    config_setting_t *channel_ranges = config_lookup(&cfg, "channel_ranges");
    if(channel_ranges == NULL)
    {
        printf("Ошибка конфигурационного файла:\tдиапазоны каналов не заданы!");
        config_destroy(&cfg);
        return READ_CONFIG_ERROR;
    }

    g_channel_ranges = (int*)malloc(sizeof(int)*g_channel_count);

    for(int i = 0; i < g_channel_count; i++)
    {
        g_channel_ranges[i] = config_setting_get_int_elem(channel_ranges, i);
    }

    config_destroy(&cfg);

    return READ_CONFIG_OK;
}

void print_config()
{
    printf("\nЗагружена следующая конфигурация модуля:\n");
    printf(" Количество используемых логических каналов\t\t:%d\n", g_channel_count);
    printf(" Частота сбора АЦП в Гц\t\t\t\t\t:%f\n", g_adc_freq);
    printf(" Количество отсчетов, считываемых за блок\t\t:%d\n", g_read_block_size);
    printf(" Таймаут перед считываением блока (мс)\t\t\t:%d\n", g_read_timeout);
    
    printf(" Номера используемых каналов\t\t\t\t:[ ");
    for(int i = 0; i<g_channel_count; i++)
    {
        printf("%d ", g_channel_numbers[i]);
    }
    printf("]\n");

    printf(" Режимы измерения каналов\t\t\t\t:[ ");
    for(int i = 0; i<g_channel_count; i++)
    {
        printf("%d ", g_channel_modes[i]);
    }
    printf("]\n");

    printf(" Диапазоны измерения каналов\t\t\t\t:[ ");
    for(int i = 0; i<g_channel_count; i++)
    {
        printf("%d ", g_channel_ranges[i]);
    }
    printf("]\n");
}

int configure_module()
{
    int32_t err = X502_SetLChannelCount(g_hnd, g_channel_count);

    if(err != X502_ERR_OK){ return CONFIGURE_ERROR; }

    for(int i = 0; i < g_channel_count; ++i)
    {
        err = X502_SetLChannel( g_hnd,
                                i,
                                g_channel_numbers[i],
                                g_channel_modes[i],
                                g_channel_ranges[i],
                                0 );

        if(err != X502_ERR_OK){ return CONFIGURE_ERROR; }
    }

    double frame_freq = g_adc_freq/g_channel_count;

    err = X502_SetAdcFreq(g_hnd, &g_adc_freq, &frame_freq);
    if(err != X502_ERR_OK){ return CONFIGURE_ERROR; }

    //what we realy set...
    printf("\nУстановлены частоты:\n"
           " Частота сбора АЦП\t\t:%0.0f\n"
           " Частота на лог. канал\t\t:%0.0f\n",
           g_adc_freq, frame_freq);

    err = X502_Configure(g_hnd, 0);
    if(err != X502_ERR_OK){ return CONFIGURE_ERROR; }

    err = X502_StreamsEnable(g_hnd, X502_STREAM_ADC);
    if(err != X502_ERR_OK){ return CONFIGURE_ERROR; }

    return CONFIGURE_OK;
}

void free_memory()
{
    destroy_pdouble_queue(&g_pd_queue);

    if(g_channel_numbers != NULL){ free(g_channel_numbers);}
    if(g_channel_modes != NULL){ free(g_channel_modes);}
    if(g_channel_ranges != NULL){ free(g_channel_ranges);}
    if(g_bin_dir != NULL){ free(g_bin_dir);}   
    if(g_hdrs != NULL){ free(g_hdrs); }
    if(g_hnd != NULL)
    {
        X502_Close(g_hnd);
        X502_Free(g_hnd);
    }     
}

void close_files()
{
    for(int i = 0; i < g_channel_count; ++i){ fclose(g_files[i]); }
}

/* 
    Print info about module 

    Return error index
*/  
int print_info_about_module()
{
    t_x502_info info;
    uint32_t err = X502_GetDevInfo(g_hnd, &info);
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

int create_files()
{
    struct tm *ct; // current time without mseconds
    
    ct = gmtime(&g_time_start.tv_sec);

    g_files = (FILE*)malloc(sizeof(FILE*)*g_channel_count);

    char file_name[100];

    g_header->year          = 1900 + ct->tm_year;
    g_header->month         = ct->tm_mon;
    g_header->day           = ct->tm_mday;
    g_header->start_hour    = ct->tm_hour;
    g_header->start_minut   = ct->tm_min;
    g_header->start_second  = ct->tm_sec;
    g_header->start_msecond = g_time_start.tv_usec - ct->tm_year * 12 *;

    if(g_files == NULL){ return CREATE_OUT_FILES_ERROR; }

    for(int i = 0; i < g_channel_count; ++i)
    {
        sprintf(file_name, 
                "%d%02d%02d-%02d%02d-%d",
                1900 + ct->tm_year,
                ct->tm_mon,
                ct->tm_mday,
                ct2->tm_hour,
                ct->tm_min,
                g_channel_numbers[i]);
        g_files[i] = fopen(file_name, "wb");

        // write headers
    }



    return CREATE_OUT_FILES_ERROR;
}

void* write_data(void *arg)
{
    int ch_cntr, data_cntr, rem_cntr; // counters
    int size;
    int total_file_size = FILE_TIME * g_adc_freq;
    int buffer_state;
    int rem;

    int current_file_size = 0;

    double *data;

    while(!g_stop)
    {
        // get_buffer_state(&buffer_state);

        pop(g_pd_queue, &data, &size);

        if(data != NULL)
        {
                    printf("Data: = %p, Size = %d\n", data, size);
            // set_buffer_state(BUFFER_EMPTY);
            // get_current_buffer_size(&size);

            rem = size % g_channel_count;
            for(data_cntr = 0; data_cntr < size; data_cntr += g_channel_count)
            {
                for(ch_cntr = 0; ch_cntr < g_channel_count; ++ch_cntr)
                {
                    fwrite(&data[data_cntr],
                           sizeof(double),
                           1,
                           g_files[ch_cntr]);
                }
            }

            for(rem_cntr = rem, ch_cntr = 0; rem_cntr != 0; --rem_cntr, ++ch_cntr)
            {
                    fwrite(&data[size - rem_cntr],
                           sizeof(double),
                           1,
                           g_files[ch_cntr]);
            }

            free(data);

            current_file_size += size/g_channel_count;

            if(current_file_size > total_file_size)
            {
                close_files();
                g_stop = 1;
                // create_files();
            }
        } 
    }
}

int main(int argc, char** argv)
{
    create_stop_event_handler();

    g_pd_queue = create_pdouble_queue();

    uint32_t err = create_config();

    if(err != READ_CONFIG_OK)
    {
        free_memory();
        printf("\nОшибка при создании конфигурации модуля! Заверешение.\n");

        return err;
    }

    print_config();

    // ------------------ find lcard e502 devices --------------------
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
    // -------------------------------------------------------------

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


    //------------------------------- chose lcard device for working---------------------------
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

        return 0; // exit from program
    }
    //----------------------------------------------------------------------------------------

    g_hnd = open_device(devrec_list, device_id);

    if(g_hnd == NULL)
    {
        // Free memory
        X502_FreeDevRecordList(devrec_list, fnd_devcnt);
        free(devrec_list);
        // --------------------------------------------

        return 0;
    }

    print_info_about_module();

    configure_module();

    err = X502_StreamsStart(g_hnd);
    if(err != X502_ERR_OK)
    {
        fprintf(stderr,
                "Ошибка запуска сбора данных: %s!\n",
                X502_GetErrorString(err)
                );

        free_memory();

        return err;
    }


    printf("Сбор данных запущен. Для остановки нажмите Ctrl+C\n");
    fflush(stdout);

    // allocate memory for file headers for each channel
    g_hdrs = (header*)malloc(sizeof(header)*g_channel_count);

    uint32_t adc_size = sizeof(double)*g_read_block_size;
    int32_t  rcv_size;
    uint32_t first_lch;
    
    pthread_t thread;
    pthread_create(&thread, NULL, write_data, NULL);
    pthread_detach(thread);

    // int buffer_index = 0;

    uint32_t* rcv_buf  = (uint32_t*)malloc(sizeof(uint32_t)*g_read_block_size);
    
    double* data;

    int curent_file_size = 0;
    int total_file_size = FILE_TIME * g_adc_freq;

    gettimeofday(&g_time_start, NULL);

    err = create_files();

    if( err != CREATE_OUT_FILES_OK )
    {  
        printf("Ошибка создания описателей выходных файлов!\n");
        free_memory();
        return err;
    }

    while(!g_stop)
    {
        data = (double*)malloc(sizeof(double) * g_read_block_size);

        rcv_size = X502_Recv(g_hnd, rcv_buf, g_read_block_size, g_read_timeout);

        X502_GetNextExpectedLchNum(g_hnd, &first_lch);

        adc_size = sizeof(double)*g_read_block_size;
        
        printf("rcv_size = %d   adc_size = %d\n", rcv_size, adc_size);

        err = X502_ProcessData(g_hnd, rcv_buf, rcv_size, X502_PROC_FLAGS_VOLT,
                               data, &adc_size, NULL, NULL);

        // if need create new files store moment of
        // reading new block data
        curent_file_size += rcv_size;
        if( rcv_size > total_file_size )
        { 
            curent_file_size = 0;
            gettimeofday(&g_time_start, NULL);
            gettimeofday(&g_time_end, NULL);
        }

        push(g_pd_queue, &data, rcv_size);
    }

    free_memory();
    
    return EXECUTE_OK;  
}