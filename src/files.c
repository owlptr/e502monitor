#include "files.h"
#include "common.h"
#include <sys/stat.h>

int create_files(FILE **files,
                 int files_count,
                 struct timeval* start_time,
                 char* path,
                 int* channel_numbers)
{
    struct tm *ts; // time of start recording

    ts = gmtime(&start_time->tv_sec);

    char dir_name[100] = "";

    sprintf(dir_name,
            "%s/%d_%02d_%02d",
            path,
            1900 + ts->tm_year,
            ts->tm_mon,
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
                 int files_count,
                 header *hdr,
                 e502monitor_config *cfg)
{
    for(int i = 0; i < files_count; ++i)
    { 
        hdr->mode = cfg->channel_modes[i];
        hdr->channel_range = cfg->channel_ranges[i];
        hdr->channel_number = cfg->channel_numbers[i];
        strcpy(hdr->channel_name, cfg->channel_names[i]);
      
        // set pointer in stream to begin
        // it's necessary, because information, which
        // represented by sctructure header
        // must be placed in the begin of data-file 
        fseek(files[i], 0, SEEK_SET);
        
        fwrite(hdr, sizeof(header), 1, files[i]);

        fclose(files[i]);
    }
}
