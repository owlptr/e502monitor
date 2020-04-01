/*
    This file part of e502monitor source code.
    Licensed under GPLv3.

    "files.c" contains realization of functions
    for writing data on disk

    Author: Gapeev Maksim
    Email: gm16493@gmail.com
*/

#include "files.h"
#include "common.h"
#include "logging.h"

#include <sys/stat.h>
#include <dirent.h>
#include <string.h>

int prepare_output_directory(char* path,
                             struct tm* start_time,
                             char* dir_name)
{
    sprintf(dir_name,
            "%s/%d_%02d_%02d",
            path,
            1900 + start_time->tm_year,
            start_time->tm_mon + 1,
            start_time->tm_mday);

    struct stat st = {0};

    if( stat(dir_name, &st) == -1 ) // if directory not exist
    {

        char log_msg[500] = "";

        sprintf(log_msg, "Директория: %s недоступна. Создаю", dir_name);

        logg(log_msg);

        if( mkdir(dir_name, 0700) != 0) 
        {
            printf("Не могу создать директорию для выходных фалов.\n"
                    "Ошибка в пути? Нет прав на запись?\n");

            logg("Не могу создать директорию для выходных фалов. "
                 "Ошибка в пути? Нет прав на запись?");

            return E502M_ERR;
        }
    }


    return E502M_ERR_OK;
}

int create_files(FILE **files,
                 int files_count,
                 struct timeval* start_time,
                 char* path,
                 int* channel_numbers,
                 char** stored_file_names)
{
    struct tm *ts; // time of start recording

    ts = gmtime(&start_time->tv_sec);

    char dir_name[100] = "";
    
    /* 
    sprintf(dir_name,
            "%s/%d_%02d_%02d",
            path,
            1900 + ts->tm_year,
            ts->tm_mon + 1,
            ts->tm_mday);

    struct stat st = {0};

    if( stat(dir_name, &st) == -1 ) // if directory not exist
    {

        char log_msg[500] = "";

        sprintf(log_msg, "Директория: %s недоступна. Создаю", dir_name);

        logg(log_msg);

        if( mkdir(dir_name, 0700) != 0) 
        {
            printf("Не могу создать директорию для выходных фалов.\n"
                    "Ошибка в пути? Нет прав на запись?\n");

            logg("Не могу создать директорию для выходных фалов. "
                 "Ошибка в пути? Нет прав на запись?");

            return E502M_ERR;
        }
    }
    */

    if( prepare_output_directory(path, ts, dir_name) != E502M_ERR_OK )
    {
        return E502M_ERR;
    }

    char log_msg[500]  = "";

    sprintf(log_msg, "Создаю файлы в директории: %s", dir_name); 
    logg(log_msg);
    
    for(int i = 0; i < files_count; ++i)
    {  
        char file_name[500] = "";
        
        sprintf(file_name, 
                "%s/%d_%02d_%02d_%02d-%02d-%02d-%06d_%d",
                dir_name,
                1900 + ts->tm_year,
                ts->tm_mon + 1,
                ts->tm_mday,
                ts->tm_hour,
                ts->tm_min,
                ts->tm_sec,
                (int)start_time->tv_usec,
                channel_numbers[i]);
        
        strcpy(stored_file_names[i], file_name);
        
        sprintf(log_msg, "Создаю файл :%s", file_name);
        logg(log_msg);

        files[i] = fopen(file_name, "wb");
        
        // Skip sizeof(header) bytes, because we will wright header, 
        // when will be know time of finish recording file
        fseek(files[i], sizeof(header), SEEK_SET);
    }

    return E502M_ERR_OK;
}

int create_flac_files(SNDFILE **files,
                      int files_count,
                      struct timeval* start_time,
                      char* path,
                      int* channel_numbers,
                      char** stored_file_names,
                      int* channel_counts_in_files,
                      double adc_freq)
{
    printf("Начинаю создавать flac-файлы.\n");
    struct tm *ts; // time of start recording

    ts = gmtime(&start_time->tv_sec);

    char dir_name[100] = "";

    if( prepare_output_directory(path, ts, dir_name) != E502M_ERR_OK )
    {
        return E502M_ERR;
    }

    char log_msg[500]  = "";

    sprintf(log_msg, "Создаю файлы в директории: %s", dir_name); 
    logg(log_msg);

    for( int i = 0; i < files_count; i++ )
    {

        char file_name[500] = "";
        
        sprintf(file_name, 
                "%s/%d_%02d_%02d_%02d-%02d-%02d-%06d_%d.flac",
                dir_name,
                1900 + ts->tm_year,
                ts->tm_mon + 1,
                ts->tm_mday,
                ts->tm_hour,
                ts->tm_min,
                ts->tm_sec,
                (int)start_time->tv_usec,
                i);

        strcpy(stored_file_names[i], file_name);

        SF_INFO sfinfo;

        sfinfo.channels = channel_counts_in_files[i];
        // sfinfo.frames = 0;
        // sfinfo.sections = 0;
        // sfinfo.seekable = 0;
        sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_DOUBLE;
        sfinfo.samplerate = adc_freq;

        if( !(files[i] = sf_open(file_name, SFM_WRITE, &sfinfo)) )
        {
            printf("ERROR: Не могу создат flac-файл!\n");

            return E502M_ERR;
        }
    }

    printf("Файлы успешно созданы\n");

    return E502M_ERR_OK;
}

void close_files(FILE **files,
                 char* dir_name,
                 char** file_names,
                 int files_count,
                 header *hdr,
                 e502monitor_config *cfg)
{
    char new_file_name[500] = "";
    char path_to_file[500] = "";

    sprintf(path_to_file,
            "%s/%d_%02d_%02d",
            dir_name,
            hdr->year,
            hdr->month,
            hdr->day);

    for(int i = 0; i < files_count; ++i)
    { 
        hdr->mode = cfg->channel_modes[i];
        hdr->channel_range = cfg->channel_ranges[i];
        hdr->channel_number = cfg->channel_numbers[i];
        strcpy(hdr->channel_name, cfg->channel_names[i]);
      
        // set pointer in stream to begin
        // it's necessary, because information, which
        // represented by sctucture header
        // must be placed in the begin of data-file 
        fseek(files[i], 0, SEEK_SET);
        
        fwrite(hdr, sizeof(header), 1, files[i]);

        fclose(files[i]);

        // set NULL as marker
        files[i] = NULL;

        // rename files (for correcting start time)

        sprintf(new_file_name, 
                "%s/%d_%02d_%02d_%02d-%02d-%02d-%06d_%d",
                path_to_file,
                hdr->year,
                hdr->month,
                hdr->day,
                hdr->start_hour,
                hdr->start_minut,
                hdr->start_second,
                (int)hdr->start_usecond,
                cfg->channel_numbers[i]);
        
        char log_msg[500] = "";

        sprintf(log_msg, 
                "Переименовываю файл <%s> на <%s>",
                file_names[i], new_file_name);
        logg(log_msg);

        rename(file_names[i], new_file_name);

    }


    logg("Файлы записаны");

}

void close_flac_files(SNDFILE **files,
                      char* dir_name,
                      char** file_names,
                      int files_count,
                      header *hdr,
                      e502monitor_config *cfg)
{
    logg("Заканчиваю запись файлов");

    char new_file_name[500] = "";
    char path_to_file[500] = "";

    sprintf(path_to_file,
            "%s/%d_%02d_%02d",
            dir_name,
            hdr->year,
            hdr->month,
            hdr->day);

    for(int i = 0; i < files_count; ++i)
    {   
        sf_close(files[i]);

        // set NULL as marker
        files[i] = NULL;

        // rename files (for correcting start time)

        sprintf(new_file_name, 
                "%s/%d_%02d_%02d_%02d-%02d-%02d-%06d_%d.flac",
                path_to_file,
                hdr->year,
                hdr->month,
                hdr->day,
                hdr->start_hour,
                hdr->start_minut,
                hdr->start_second,
                (int)hdr->start_usecond,
                i);
        
        char log_msg[500] = "";

        sprintf(log_msg, 
                "Переименовываю файл <%s> на <%s>",
                file_names[i], new_file_name);
        logg(log_msg);

        rename(file_names[i], new_file_name);
    }
}

int remove_day(char *path)
{
    char log_msg[500] = "";
    sprintf(log_msg, "Удаление дня: %s", path);
    logg(log_msg);

    struct dirent **namelist;

    int n = scandir(path, &namelist, NULL, alphasort);


    sprintf(log_msg, "Удаляем файлов: %d\n", (n - 2));
    logg(log_msg);

    if( n < 0 )
    {
// #ifdef DBG
    logg("Ошибка очитски директории");
// #endif
        return E502M_ERR;
    }

    while( n -- )
    {
        if( strcmp(namelist[n]->d_name, ".")  == 0 ||
            strcmp(namelist[n]->d_name, "..") == 0)
        {

// #ifdef DBG
//             printf("Пропускаю %s\n", namelist[n]->d_name);
// #endif
        } else {
            char remove_file[200] = "";
            
            strcpy(remove_file, path);
            strcat(remove_file, "/");
            strcat(remove_file, namelist[n]->d_name);

            remove(remove_file);

            sprintf(log_msg, "Удаляю файл: %s\n", remove_file);
            logg(log_msg);
        }

        free(namelist[n]);
    }

    int result = rmdir(path);


    sprintf(log_msg, "Результат удаление: %d", result);
    logg(log_msg);

    free(namelist);

    return E502M_ERR_OK;
}

int is_need_clear_dir(char *path,
                      char *current_day,
                      int stored_days_count)
{
    struct dirent **namelist;

    logg("Сканирую директорию");

    int n = scandir(path, &namelist, NULL, alphasort);

    if( n < 0 )
    {
        return E502M_ERR;
    }

    int stored_days = n;

    while( n -- )
    {
        if( strcmp(namelist[n]->d_name, ".")  == 0 ||
            strcmp(namelist[n]->d_name, "..") == 0 ||
            strcmp(namelist[n]->d_name, current_day) == 0)
        {
            stored_days --;
        }

        free(namelist[n]);
    }

    free(namelist);

    return ( stored_days - stored_days_count );
}

int remove_days( char *path, char *current_day, int count )
{
    struct dirent **namelist;

    int n = scandir(path, &namelist, NULL, alphasort);

    if( n < 0 )
    {
        return E502M_ERR;
    }

    int removed_days_count = 0;
    
    int i = 0;

    for(i = 0; i < n; i++)
    {
        if( strcmp(namelist[i]->d_name, ".")  == 0 ||
            strcmp(namelist[i]->d_name, "..") == 0 ||
            strcmp(namelist[i]->d_name, current_day) == 0)
        {
          
          
        }else { 

            char remove_dir[200] = "";
            strcpy(remove_dir, path);
            strcat(remove_dir, "/");
            strcat(remove_dir, namelist[i]->d_name);

            remove_day(remove_dir);

            removed_days_count++;
        }

        free(namelist[i]);

        if( removed_days_count == count ) { break; }
    }

    i++;
    
    while( i < n ) {
        free(namelist[i]);
        i++;     
    }

    free(namelist);

    logg("Дни удалены");

    return E502M_ERR_OK;
}

void create_prop_file(char** file_name,
                      int    file_id,
                      int    samples_count,
                      e502monitor_config *config)
{
    char prop_file_name[256];

    strcpy(&prop_file_name, file_name);
    strcpy(&prop_file_name, "/");
    strcpy(&prop_file_name, config->bin_dir);
    strcpy(&prop_file_name, ".prop");

    FILE* prop_file = fopen(prop_file_name, "w");
    
    fprintf(prop_file, "samples_count = %n\n", samples_count);
    fprintf(prop_file, "channel_names = %s\n", config->channel_distribution_str[file_id]);



    fclose(prop_file_name);
}