#include "files.h"
#include "common.h"
#include "logging.h"

#include <sys/stat.h>
#include <dirent.h>
#include <string.h>

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

void close_files(FILE **files,
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