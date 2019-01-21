#include "files.h"
#include "common.h"
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
#ifdef DBG  
        printf("Директория: %s недоступна. Создаю.\n", dir_name);
#endif
        if( mkdir(dir_name, 0700) != 0) 
        {
             printf("Не могу создать директорию для выходных фалов.\n"
                    "Ошибка в пути? Нет прав на запись?\n");

                return E502M_ERR;
        }
    }

#ifdef DBG
        printf("Создаю файлы в директории: %s\n", dir_name);
#endif   

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
#ifdef DBG
        printf("Создаю файл :%s\n", file_name);
#endif
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

#ifdef DBG
    printf("Заканчиваю запись файлов...\n");
#endif

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
                i);
        
#ifdef DBG
        printf("Переименовываю файл <%s> на <%s>\n", file_names[i], new_file_name);
#endif

        rename(file_names[i], new_file_name);
    }

#ifdef DBG
    printf("Файлы записаны.\n");
#endif

}

int remove_day(char *path)
{
#ifdef DBG
    printf("Удаление дня: %s\n", path);
#endif
    struct dirent **namelist;

    int n = scandir(path, &namelist, NULL, alphasort);

#ifdef DBG
    printf("Удаляем файлов: %d\n", (n - 2));
#endif

    if( n < 0 )
    {
#ifdef DBG
    printf("Упс..: %d\n", n);
#endif
        return E502M_ERR;
    }

    while( n -- )
    {
        if( strcmp(namelist[n]->d_name, ".")  == 0 ||
            strcmp(namelist[n]->d_name, "..") == 0)
        {

#ifdef DBG
            printf("Пропускаю %s\n", namelist[n]->d_name);
#endif
        } else {
            char remove_file[200] = "";
            
            strcpy(remove_file, path);
            strcat(remove_file, "/");
            strcat(remove_file, namelist[n]->d_name);

            remove(remove_file);
#ifdef DBG
            printf("Удаляю файл: %s\n", remove_file);
#endif
        }

        free(namelist[n]);
    }

    int result = rmdir(path);

    printf("Результат удаление: %d\n", result);

    free(namelist);

    return E502M_ERR_OK;
}

int is_need_clear_dir(char *path,
                      char *current_day,
                      int stored_days_count)
{
    struct dirent **namelist;
#ifdef DBG
    printf("Сканирую директорию\n");
#endif
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

#ifdef DBG
    printf("Дни удалены\n");
#endif

    return E502M_ERR_OK;
}