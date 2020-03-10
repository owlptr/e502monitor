/*
    This file part of e502monitor source code.
    Licensed under GPLv3.

    The main file of programm. It contains main function.

    Author: Gapeev Maksim
    Email: gm16493@gmail.com
*/

#include "pdouble_queue.h"
#include "config.h"
#include "common.h"
#include "files.h"
#include "header.h"
#include "device.h"
#include "logging.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>

static int            g_stop = 0; // if equal 1 - stop working
static struct timeval g_time_start; // time of start writing file
static struct timeval g_time_end;   // time of finish writing file
static struct timeval g_prev_time_start; // time start of previuos file

// static FILE**   g_files = NULL; // files for stored data
static SNDFILE**   g_files = NULL; // files for stored data
static header   g_header; // header with information 
static e502monitor_config*  g_config = NULL; // structure for configure
static pdouble_queue* g_data_queue = NULL; // queue for stored data

static char **g_old_file_names = NULL; // array of old file names

/*
    Creates stop event heandler for
    correct completion of data collection 
*/
void create_stop_event_handler();

/*
    Signal of completion 
*/
void abort_handler(int sig);

/*
    Function for running in another thread.
    Writes data on disk.
*/
void* write_data(void *arg);

/*
    Returns current day as string.

    current_day - pointer to pointer to return char array
*/
void get_current_day_as_string(char **current_day);

/*
    Frees memory from global variables
*/
void free_global_memory();

/*
    Clears data directory. Delete old days.
*/
void clear_dir();

/*
    Prints the program info
*/

void print_program_info();

// /*
//     Reconects to device.

//     attempts_nmbr - the number of attempts to establish a connection
//     time_out - waiting time for another attempt 
// */
// void reconect_to_device(int time_out, int attempts_nmbr);

int main(int argc, char** argv)
{
    print_program_info();

    logg("Программа запущена");

    logg("Создаю обработчик для события завершения сбора данных");
    create_stop_event_handler();

    logg("Создаю очередь для хранения данных");
    g_data_queue = create_pdouble_queue();

    logg("Создаю структуру для конфигурирования");
    g_config = create_config();

    if( g_config == NULL )
    {
        printf("Ошибка создания конфигурации. Завершение.\n");
        logg("Ошибка создания конфигурации. Завершение.");

        free_global_memory();

        return E502M_EXIT_FAILURE;
    }

    logg("Инициализирую структуру для конфигурирования");

    if ( init_config(&g_config) != E502M_ERR_OK )

    {
        printf("Ошибка конфигурационного файла. Заверешние.\n");
        logg("Ошибка конфигурационного файла. Заверешние");

        free_global_memory();

        return E502M_EXIT_FAILURE;
    }

    print_config(g_config);

    uint32_t fnd_devcnt = 0;
    t_x502_devrec *devrec_list = NULL;

    // try to connect
    int point_counter = 0;
    printf("Поиск устройств...\n");
    while(fnd_devcnt == 0  && !g_stop){
        fnd_devcnt = get_usb_devrec(&devrec_list);
        usleep(1000000);
    }
    
    if (fnd_devcnt == 0) 
    {
        printf("Не найдено ни одного модуля. Завершение.\n");
        logg("Не найдено ни одного модуля. Завершение");
        free_global_memory();

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
        logg("Указан неверный номер модуля! Завершение");

        X502_FreeDevRecordList(devrec_list, fnd_devcnt);
        free(devrec_list);

        free_global_memory();

        return E502M_EXIT_FAILURE;
    }
    //----------------------------------------------------------------------------------------

    t_x502_hnd device_hnd = open_device(devrec_list, device_id);

    if(device_hnd == NULL)
    {
        printf("Ошибка установления связи с модулем. Завершение.\n");
        logg("Ошибка установления связи с модулем. Завершение");
        
        X502_FreeDevRecordList(devrec_list, fnd_devcnt);
        free(devrec_list);

        free_global_memory();
        
        return E502M_EXIT_FAILURE;
    }

    X502_FreeDevRecordList(devrec_list, fnd_devcnt);
    free(devrec_list);

    print_info_about_device(device_hnd);

    configure_device(device_hnd, g_config);

    // initialize some header fields
    g_header.adc_freq = g_config->adc_freq / g_config->channel_count;
    strcpy(g_header.module_name, g_config->module_name);
    strcpy(g_header.place, g_config->place);

    uint32_t adc_size = sizeof(double)*g_config->read_block_size;
    int32_t  rcv_size;
    uint32_t first_lch;
    uint32_t* rcv_buf  = (uint32_t*)malloc(sizeof(uint32_t)*g_config->read_block_size);
    int total_file_sizes = g_config->file_size * g_config->adc_freq;
    int current_file_sizes = 0;
    double* data;

    // allocate memory for old files array
    g_old_file_names = (char**)malloc(sizeof(char*) * g_config->channel_count);
    
    for(int i = 0; i < g_config->channel_count; i++)
    {
        g_old_file_names[i] = (char*)malloc(sizeof(char) * 101);
    }

    logg("Создаю файлы");

    gettimeofday(&g_time_start, NULL);
    
    // TODO: fix this function
    // clear_dir();

    g_files = (FILE*)malloc(sizeof(FILE*)*g_config->channel_count);

    logg("Создаю отдельный поток для записи данных");

    uint32_t  err = X502_StreamsStart(device_hnd);
    if(err != X502_ERR_OK)
    {
        fprintf(stderr,
                "Ошибка запуска сбора данных: %s!\n",
                X502_GetErrorString(err)
                );
        logg("Ошибка запуска сбора данных");

        free_global_memory();

        return E502M_EXIT_FAILURE;
    }

    printf("Сбор данных запущен. Для остановки нажмите Ctrl+C\n");
    fflush(stdout);

    int current_file_block_add = g_config->read_block_size; // / g_config->channel_count;

    // current_file_size = total_file_size;

    int read_block_size = g_config->read_block_size;
    int read_timeout = g_config->read_timeout; 
    int real_file_sizes = 0;

    pthread_t thread;
    pthread_create(&thread, NULL, write_data, NULL);
    pthread_detach(thread);

    gettimeofday(&g_time_start, NULL);
    
    err = create_files(g_files,
                       g_config->channel_count,
                       &g_time_start,
                       g_config->bin_dir,
                       g_config->channel_numbers,
                       g_old_file_names);

    if( err != E502M_ERR_OK )
    {
        logg("Ошибка создания описателей выходных файлов");

        X502_Close(device_hnd);
        X502_Free(device_hnd);

        free_global_memory();

        return E502M_EXIT_FAILURE;
    }

    // main loop for receiving data

    gettimeofday(&g_prev_time_start, NULL);

    int is_need_restart = 0; // 0 - not, 1 - yes

    while(!g_stop)
    {
        data = (double*)malloc(sizeof(double) * read_block_size);

        
        // if need create new files store moment of
        // reading new block data
        // current_file_size += g_config->read_block_size / g_config->channel_count;
        current_file_sizes += current_file_block_add;
        // current_file_size += read_block_size;

        if( current_file_sizes >= total_file_sizes )
        {
            read_block_size = total_file_sizes - real_file_sizes;
            
// #ifdef DBG
//             logg("Считано сэмплов: %d", real_file_sizes);
//             logg("До эталонного количества сэмплов необходимо прочесть сэмплов: %d",
//                  read_block_size);
// #endif
            real_file_sizes = 0;
            current_file_sizes = 0;
            
            // gettimeofday(&g_time_start, NULL);

            rcv_size = X502_Recv(device_hnd,
                                 rcv_buf,
                                 read_block_size,
                                 read_timeout);
            
            if(rcv_size < 0) // some errors
            {
                logg("Ошибка получения данных");

                g_stop = 1; 
                is_need_restart = 1;
                break; // exit from receiving data loop 
            }

            gettimeofday(&g_time_start, NULL);
            gettimeofday(&g_time_end, NULL);

            // initialize time fields in header ---------------------
            struct tm *ts;
            
            // ts = gmtime(&g_time_start.tv_sec);
            ts = gmtime(&g_prev_time_start.tv_sec);

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

            g_prev_time_start = g_time_start;

            // last_buffer_index = NOT_LAST_BUFFER;
            read_block_size = g_config->read_block_size;
#ifdef DBG
            printf("Последний буфер прочитан %d", rcv_size);
            logg("Последний буфер прочитан");
#endif

            X502_GetNextExpectedLchNum(device_hnd, &first_lch);

            adc_size = sizeof(double) * read_block_size;

            err = X502_ProcessData(device_hnd, rcv_buf, rcv_size, X502_PROC_FLAGS_VOLT,
                                       data, &adc_size, NULL, NULL);

            push_to_pdqueue(g_data_queue, &data, rcv_size, first_lch, LAST_BUFFER);
        } else {

            rcv_size = X502_Recv(device_hnd,
                                 rcv_buf,
                                 read_block_size,
                                 read_timeout);
            
            // printf("RCV_Size = %d\n", rcv_size);

            if(rcv_size < 0) // some errors
            {
                logg("Ошибка получения данных");
                g_stop = 1; 
                is_need_restart = 1;
                break; // exit from receiving data loop 
            }

            X502_GetNextExpectedLchNum(device_hnd, &first_lch);

            adc_size = sizeof(double) * read_block_size;

            err = X502_ProcessData(device_hnd, rcv_buf, rcv_size, X502_PROC_FLAGS_VOLT,
                                   data, &adc_size, NULL, NULL);
            push_to_pdqueue(g_data_queue, &data, rcv_size, first_lch, NOT_LAST_BUFFER);


            real_file_sizes += rcv_size;
        }
    }

    while(pthread_kill(thread, 0) != ESRCH){
#ifdef DBG
    printf("Ожидаю заверешения потока записи данных...\n");
    logg("Ожидаю заверешения потока записи данных");
#endif
        usleep(10);
    }

    X502_Close(device_hnd);
    X502_Free(device_hnd);

    free_global_memory();

    if(is_need_restart)
    {
        printf("Пытаюсь перезапуститься...\n");
        logg("Перезапуск");
        system("./e502monitor");
    }

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
    int* file_sizes = (int*)malloc(sizeof(int) * g_config->channel_count);
    int* fill_index_files = (int*)malloc(sizeof(int) * g_config->channel_count);
    
    for(int i = 0; i < g_config->channel_count; i++) 
    { 
        file_sizes[i] = 0;
        fill_index_files[i] = 0;    
    }

    double** rest_buffers = (double**)malloc(sizeof(double*)*g_config->channel_count);
    int *rest_buffer_sizes = (int*)malloc(sizeof(int) * g_config->channel_count );

    for(int i = 0; i < g_config->channel_count; i++) 
    { 
        rest_buffers[i] = (double*)malloc(sizeof(double)* g_config->read_block_size);
        rest_buffer_sizes[i] = 0;
    }

    int ch_cntr, data_cntr; // counters
    int size;

    int total_file_size = g_config->file_size * (g_config->adc_freq / g_config->channel_count);

#ifdef DBG
    printf("Эталонное количество сэмплов в файле: %d\n", total_file_size);
#endif

    int buffer_state;

    int current_file_size = 0;

    double *data = NULL;

    int sleep_time = g_config->read_timeout / 2; 
    int last_buffer_index = NOT_LAST_BUFFER;
    
    while(!g_stop || !empty(g_data_queue))
    {
        pop_from_pdqueue(g_data_queue, &data, &size, &ch_cntr, &last_buffer_index);

        if(data != NULL)
        {   

            current_file_size += size/g_config->channel_count;  
        
            for(data_cntr = 0; data_cntr < size; data_cntr++, ch_cntr++)
            {                               
                if(ch_cntr == g_config->channel_count){ ch_cntr = 0; } 

                if(fill_index_files[ch_cntr] == 1)
                { 
                    
                    rest_buffers[ch_cntr][rest_buffer_sizes[ch_cntr]] = data[data_cntr];
                    rest_buffer_sizes[ch_cntr] ++;

                } else {
                    

                    /*
                    fwrite(&data[data_cntr],
                            sizeof(double),
                            1,
                            g_files[ch_cntr]);
                    */



                    file_sizes[ch_cntr] ++;

                    if( file_sizes[ch_cntr] >= total_file_size )
                    { 
                        fill_index_files[ch_cntr] = 1;
                    }
                }
                

            }                

            if(last_buffer_index == LAST_BUFFER)
            {

            for(ch_cntr = 0; ch_cntr < g_config->channel_count; ch_cntr++)
            {
                char log_msg[100] = "";
                sprintf(log_msg,
                        "Количество сэмплов в файле канала %d = %d",    
                        ch_cntr,
                        file_sizes[ch_cntr]);
                logg(log_msg);
            }


                current_file_size = 0;

                close_files(g_files,
                            g_config->bin_dir,
                            g_old_file_names,
                            g_config->channel_count,
                            &g_header,
                            g_config);

                // TODO: fix this function
                // clear_dir();

                create_files(g_files,
                             g_config->channel_count,
                             &g_time_start,
                             g_config->bin_dir,
                             g_config->channel_numbers,
                             g_old_file_names);

                // process the rests

                logg("Обрабатываю остаток");

                for(ch_cntr = 0; ch_cntr < g_config->channel_count; ch_cntr++)
                {
                    if(rest_buffer_sizes[ch_cntr] == 0){ continue; }

                    char log_msg[100] = "";
                    sprintf(log_msg, 
                            "Остаток файла для канала %d = %d",
                            ch_cntr,
                            rest_buffer_sizes[ch_cntr]);
                    logg(log_msg);

                    fwrite(rest_buffers[ch_cntr],
                           sizeof(double),
                           rest_buffer_sizes[ch_cntr] - 1,
                           g_files[ch_cntr]);
                }

                for( ch_cntr = 0; ch_cntr < g_config->channel_count; ch_cntr++) 
                { 
                    fill_index_files[ch_cntr]  = 0;
                    file_sizes[ch_cntr] = rest_buffer_sizes[ch_cntr];
                    rest_buffer_sizes[ch_cntr] = 0;
                }
            }
            
            free(data);   
        } else { 
            usleep(sleep_time); // sleep to save process time
        }
    }    

    free(file_sizes);
    free(fill_index_files);
    free(rest_buffer_sizes);

    for(int i = 0; i < g_config->channel_count; i++)
    {
        free(rest_buffers[i]);
    }

    free(rest_buffers);

    // initialize time in header

    gettimeofday(&g_time_start, NULL);
    gettimeofday(&g_time_end, NULL);

    // initialize time fields in header ---------------------
    struct tm *ts;
            
    // ts = gmtime(&g_time_start.tv_sec);
    ts = gmtime(&g_prev_time_start.tv_sec);

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
    
    // close files
    close_files(g_files,
                g_config->bin_dir,
                g_old_file_names,
                g_config->channel_count,
                &g_header,
                g_config);
}

void get_current_day_as_string(char **current_day)
{
    struct tm *ts; // time of start recording

    ts = gmtime(&g_time_start.tv_sec);

    sprintf(*current_day,
            "%d_%02d_%02d",
            1900 + ts->tm_year,
            ts->tm_mon + 1,
            ts->tm_mday);
}

void clear_dir()
{
    char *current_day = (char*)malloc(sizeof(char) * 100);
    
    logg("Вычисляю день в виде строки.");
    get_current_day_as_string(&current_day);

    logg("Проверяю необходимость очистки директории");

    // TODO: this works wrong!
    int remove_days_count = is_need_clear_dir(g_config->bin_dir,
                                              current_day,
                                              g_config->stored_days_count);

    if( remove_days_count > 0 )
    {
        
        char log_msg[100] = "";
        sprintf(log_msg, "Необходимо удалить дней: %d", remove_days_count);
        logg(log_msg);

        remove_days(g_config->bin_dir, current_day, remove_days_count);
    }

    free (current_day);
}

void free_global_memory()
{
    if( g_files != NULL )
    {

        logg("Особождаю память от заголовков файлов");

        free(g_files);

        logg("Память от заголовков файлов освобождена");
         
    }  

    if( g_data_queue != NULL)
    { 

        logg("Разрушаю потокобезопасную очередь");

        destroy_pdouble_queue(&g_data_queue); 


        logg("Потокобезопасная очередь разрушена");        
    }
    
    if( g_old_file_names != NULL )
    {

        logg("Особождаю память от массива старых имен файлов");
        
        for(int i = 0; i < g_config->channel_count; i++)
        {
            free(g_old_file_names[i]);
        }

        free(g_old_file_names);

        logg("Память от массива старых имен файлов освобождена.\n");


    } 

    if ( g_config != NULL )
    {


        logg("Разрушаю конфигурационную структуру");
   
        destroy_config(&g_config);

        logg("Конфигурационная структура разрушена");
 
         
    };
}

void print_program_info()
{
    char name_version_info[128] = "E502 Monitor (version: ";

    strcat(name_version_info, VERSION);
    strcat(name_version_info, ")\n");

    printf(name_version_info);
    printf("Программа для сбора данных при помощи LCard E-502\n\n");
    printf("Автор: Гапеев Максим\n");
    printf("Email: gm16493@gmail.com\n\n");

}
