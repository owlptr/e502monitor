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
// #include <pthread.h>
// #include <dirent.h>
// #include <sys/stat.h>

// #include <libconfig.h>
#include "device.h"

static int            g_stop = 0; // if equal 1 - stop working
static struct timeval g_time_start; // time of start writing file
static struct timeval g_time_end;   // time of finish writing file

static FILE** g_files; // files for stored data
static header* g_header; // header with information 


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
    create_stop_event_handler();

    pdouble_queue* data_queue = create_pdouble_queue();

    e502monitor_config* config = create_config();

    if( config == NULL )
    {
        printf("Ошибка создания конфигурации. Завершение\n");
        
        destroy_pdouble_queue(data_queue);
        destroy_config(config);

        return E502M_EXIT_FAILURE;
    }

    print_config(config);

    uint32_t fnd_devcnt = 0;
    t_x502_devrec *devrec_list = NULL;

    fnd_devcnt = get_usb_devrec(&devrec_list);
    
    if (fnd_devcnt == 0) 
    {
        printf("Не найдено ни одного модуля. Завершение.\n");

        destroy_pdouble_queue(data_queue);
        destroy_config(config);
        
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

        destroy_pdouble_queue(data_queue);
        destroy_config(config);

        return E502M_EXIT_FAILURE;
    }
    //----------------------------------------------------------------------------------------

    t_x502_hnd device_hnd = open_device(devrec_list, device_id);

    if(device_hnd == NULL)
    {
        print("Ошибка установления связи с модулем. Завершение.\n");

        X502_FreeDevRecordList(devrec_list, fnd_devcnt);
        free(devrec_list);

        destroy_pdouble_queue(data_queue);
        destroy_config(config);
        
        return E502M_EXIT_FAILURE;
    }

    X502_FreeDevRecordList(devrec_list, fnd_devcnt);
    free(devrec_list);

    uint32_t err = X502_StreamsStart(device_hnd);
    if(err != X502_ERR_OK)
    {
        fprintf(stderr,
                "Ошибка запуска сбора данных: %s!\n",
                X502_GetErrorString(err)
                );



        return err;
    }

    print_info_about_device(device_hnd);

    configure_device(device_hnd, config);
    
    printf("Сбор данных запущен. Для остановки нажмите Ctrl+C\n");
    fflush(stdout);



    uint32_t adc_size = sizeof(double)*config->read_block_size;
    int32_t  rcv_size;
    uint32_t first_lch;
    uint32_t* rcv_buf  = (uint32_t*)malloc(sizeof(uint32_t)*config->read_block_size);
    int total_file_size = FILE_TIME * config->adc_freq / config->channel_count;
    int current_file_size = 0;
    double* data;

    gettimeofday(&g_time_start, NULL);


    err = create_files(g_files,
                       config->channel_count,
                       &g_time_start,
                       config->bin_dir,
                       config->channel_numbers);

    if( err != E502M_ERR_OK)
    {
        printf("Ошибка создания описателей выходных файлов!\n");

        X502_Close(device_hnd);
        X502_Free(device_hnd);

        destroy_pdouble_queue(data);
        destroy_config(config);
    }

    pthread_t thread;
    pthread_create(&thread, NULL, write_data, NULL);
    pthread_detach(thread);

    // main loop

    while(!g_stop)
    {
        data = (double*)malloc(sizeof(double) * config->read_block_size);

        
        // if need create new files store moment of
        // reading new block data
        current_file_size += rcv_size / config->channel_count;

        if( current_file_size >= total_file_size )
        {
            // printf("Пишу значений в файл: %d\n", current_file_size); 
            current_file_size = 0;
            // g_buf_time_start = g_time_start;
            gettimeofday(&g_time_start, NULL);
            gettimeofday(&g_time_end, NULL);
        }

        rcv_size = X502_Recv(device_hnd,
                             rcv_buf,
                             config->read_block_size,
                             config->read_timeout);

        X502_GetNextExpectedLchNum(device_hnd, &first_lch);

        adc_size = sizeof(double)*config->read_block_size;

        err = X502_ProcessData(device_hnd, rcv_buf, rcv_size, X502_PROC_FLAGS_VOLT,
                               data, &adc_size, NULL, NULL);
    }



    X502_Close(device_hnd);
    X502_Free(device_hnd);
    destroy_pdouble_queue(data_queue);
    destroy_config(config);

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