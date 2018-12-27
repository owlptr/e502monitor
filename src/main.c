/*
    Author: Gapeev Maksim
*/

#include "pdouble_queue.h"
#include "config.h"
#include "common.h"
#include "files.h"
#include "header.h"

#include <stdio.h>
#include <stdint.h>
// #include <stdlib.h>
// #include <unistd.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
// #include <dirent.h>
// #include <sys/stat.h>

// #include <libconfig.h>
#include "device.h"

static int            g_stop = 0; // if equal 1 - stop working
static struct timeval g_time_start; // time of start writing file
static struct timeval g_time_end;   // time of finish writing file

static FILE**   g_files;  // files for stored data
static header   g_header; // header with information 
static e502monitor_config*  g_config; // structure for configure
static pdouble_queue* g_data_queue; // queue for stored data

/*
    Create stop event heandler for
    correct completion of data collection 
*/
void create_stop_event_handler();

/*
    Signal of completion 
*/
void abort_handler(int sig);

/*
    Function for running in another thread.
    Write data to disk.
*/
void* write_data(void *arg);

int main(int argc, char** argv)
{

#ifdef DBG
    printf("Создаю обработчик для события завершения сбора данных.\n");
#endif

    create_stop_event_handler();

#ifdef DBG
    printf("Создаю очередь для хранения данных.\n");
#endif
    
    g_data_queue = create_pdouble_queue();

#ifdef DBG
    printf("Создаю структуру для конфигурирования.\n");
#endif

    g_config = create_config();

    if( g_config == NULL )
    {
        printf("Ошибка создания конфигурации. Завершение.\n");
        
        destroy_pdouble_queue(g_data_queue);
        destroy_config(g_config);

        return E502M_EXIT_FAILURE;
    }

#ifdef DBG
    printf("Инициализирую структуру для конфигурирования.\n");
#endif

    if ( init_config(&g_config) != E502M_ERR_OK )

    {
        printf("Ошибка конфигурационного файла. Заверешние.\n");

        destroy_pdouble_queue(g_data_queue);
        destroy_config(g_config);

        return E502M_EXIT_FAILURE;
    }

    print_config(g_config);

    // initialize some header fields
    g_header.adc_freq    = g_config->adc_freq;
    strcpy(g_header.module_name, g_config->module_name);
    strcpy(g_header.place, g_config->place);

    uint32_t fnd_devcnt = 0;
    t_x502_devrec *devrec_list = NULL;

    fnd_devcnt = get_usb_devrec(&devrec_list);
    
    if (fnd_devcnt == 0) 
    {
        printf("Не найдено ни одного модуля. Завершение.\n");

        destroy_pdouble_queue(g_data_queue);
        destroy_config(g_config);
        
        return E502M_EXIT_FAILURE;
    }

    print_available_devices(devrec_list, fnd_devcnt);

    //------------------------------- choose lcard device for working--------------------------
    printf("Введите номер модуля, с которым хотите работать (от 0 до %d)\n", fnd_devcnt-1);
    fflush(stdout);

    uint32_t device_id = -1;

    // scanf("%d", &device_id);
    
    // HACK: choose device 0 by default
    device_id = 0;

    if( device_id < 0 || device_id >= fnd_devcnt)
    {
        printf("\nНеверный номер модуля! Завершение.\n");

        X502_FreeDevRecordList(devrec_list, fnd_devcnt);
        free(devrec_list);

        destroy_pdouble_queue(g_data_queue);
        destroy_config(g_config);

        return E502M_EXIT_FAILURE;
    }
    //----------------------------------------------------------------------------------------

    t_x502_hnd device_hnd = open_device(devrec_list, device_id);

    if(device_hnd == NULL)
    {
        printf("Ошибка установления связи с модулем. Завершение.\n");

        X502_FreeDevRecordList(devrec_list, fnd_devcnt);
        free(devrec_list);

        destroy_pdouble_queue(g_data_queue);
        destroy_config(g_config);
        
        return E502M_EXIT_FAILURE;
    }

    X502_FreeDevRecordList(devrec_list, fnd_devcnt);
    free(devrec_list);

    print_info_about_device(device_hnd);

    configure_device(device_hnd, g_config);

    uint32_t adc_size = sizeof(double)*g_config->read_block_size;
    int32_t  rcv_size;
    uint32_t first_lch;
    uint32_t* rcv_buf  = (uint32_t*)malloc(sizeof(uint32_t)*g_config->read_block_size);
    int total_file_size = FILE_TIME * g_config->adc_freq / g_config->channel_count;
    int current_file_size = 0;
    double* data;

    gettimeofday(&g_time_start, NULL);

#ifdef DBG
    printf("Создаю файлы\n");
#endif

     g_files = (FILE*)malloc(sizeof(FILE*)*g_config->channel_count);

    uint32_t err = create_files(g_files,
                                g_config->channel_count,
                                &g_time_start,
                                g_config->bin_dir,
                                g_config->channel_numbers);

    if( err != E502M_ERR_OK)
    {
        printf("Ошибка создания описателей выходных файлов!\n");

        X502_Close(device_hnd);
        X502_Free(device_hnd);

        destroy_pdouble_queue(data);
        destroy_config(g_config);
        free(g_files);

        return E502M_EXIT_FAILURE;
    }

#ifdef DBG
    printf("Создаю отдельный поток для записи данных\n");
#endif

    pthread_t thread;
    pthread_create(&thread, NULL, write_data, NULL);
    pthread_detach(thread);

    err = X502_StreamsStart(device_hnd);
    if(err != X502_ERR_OK)
    {
        fprintf(stderr,
                "Ошибка запуска сбора данных: %s!\n",
                X502_GetErrorString(err)
                );

        destroy_pdouble_queue(g_data_queue);
        destroy_config(g_config);
        free(g_files);

        return E502M_EXIT_FAILURE;
    }

    printf("Сбор данных запущен. Для остановки нажмите Ctrl+C\n");
    fflush(stdout);

    // main loop for receiving data
    while(!g_stop)
    {
        data = (double*)malloc(sizeof(double) * g_config->read_block_size);

        
        // if need create new files store moment of
        // reading new block data
        current_file_size += g_config->read_block_size / g_config->channel_count;

        if( current_file_size >= total_file_size )
        {
            current_file_size = 0;
            gettimeofday(&g_time_end, NULL);

            // initialize time fields in header ---------------------
            struct tm *ts;
            
            ts = gmtime(&g_time_start.tv_sec);
            
            g_header.year               = 1900 + ts->tm_year;
            g_header.month              = ts->tm_mon + 1;
            g_header.day                = ts->tm_mday;
            g_header.start_hour         = ts->tm_hour;
            g_header.start_minut        = ts->tm_min;
            g_header.start_second       = ts->tm_sec;
            g_header.start_usecond      = (int)g_time_start.tv_usec;
            
            ts = gmtime(&g_time_end.tv_sec);
            
            g_header.finish_hour        = ts->tm_hour;
            g_header.finish_minut       = ts->tm_min;
            g_header.finish_second      = ts->tm_sec;
            g_header.finish_usecond     = (int)g_time_start.tv_usec;

            // ------------------------------------------------------

            gettimeofday(&g_time_start, NULL);
        }

        rcv_size = X502_Recv(device_hnd,
                             rcv_buf,
                             g_config->read_block_size,
                             g_config->read_timeout);

        X502_GetNextExpectedLchNum(device_hnd, &first_lch);

        adc_size = sizeof(double) * g_config->read_block_size;

        err = X502_ProcessData(device_hnd, rcv_buf, rcv_size, X502_PROC_FLAGS_VOLT,
                               data, &adc_size, NULL, NULL);

        push_to_pdqueue(g_data_queue, &data, rcv_size, first_lch);
    }



    X502_Close(device_hnd);
    X502_Free(device_hnd);
    destroy_pdouble_queue(g_data_queue);
    destroy_config(g_config);
    free(g_files);

    return 0;
}



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

void *write_data(void *arg)
{
    int ch_cntr, data_cntr; // counters
    int size;
    int total_file_size = FILE_TIME * g_config->adc_freq / g_config->channel_count;
    int buffer_state;

    int current_file_size = 0;

    double *data = NULL;

    int sleep_time = g_config->read_timeout / 2; 

    while(!g_stop)
    {
        pop_from_pdqueue(g_data_queue, &data, &size, &ch_cntr);

        if(data != NULL)
        {   
#ifdef DBG
        printf("Пишу блок данных\n");
#endif 
            current_file_size += size/g_config->channel_count;            
            for(data_cntr = 0; data_cntr < size; data_cntr++, ch_cntr++)
            {                               
                if(ch_cntr == g_config->channel_count){ ch_cntr = 0; } 

                fwrite(&data[data_cntr],
                        sizeof(double),
                        1,
                        g_files[ch_cntr]);
            }                
            
            if(current_file_size >= total_file_size)
            {
                current_file_size = 0;
                
                close_files(g_files,
                            g_config->channel_count,
                            &g_header,
                            &g_config);
                
                create_files(g_files,
                             g_config->channel_count,
                             &g_time_start,
                             g_config->bin_dir,
                             g_config->channel_numbers);
            }

            free(data);   
        } else { 
            usleep(sleep_time); // sleep to save process time
        }
    }    
}