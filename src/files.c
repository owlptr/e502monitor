#include "files.h"
#include "common.h"

int create_files(FILE **files,
                 int files_count,
                 struct timeval* start_time,
                 char* path,
                 int* channel_numbers)
{
    struct tm *ts; // time of start recording
    
    ts = gmtime(&start_time->tv_sec);

    files = (FILE*)malloc(sizeof(FILE*)*files_count);

    if(files == NULL){ return E502M_ERR; }

    char dir_name[100] = "";

    sprintf(dir_name,
            "%s/%d_%02d_%02d",
            path,
            1900 + ts->tm_year,
            ts->tm_mon,
            ts->tm_mday);

    for(int i = 0; i < files_count; ++i)
    {  
        char file_name[500] = "";
        
        sprintf(file_name, 
                "%s/%d_%02d_%02d_%02d-%02d-%02d-%06d_%d",
                dir_name,
                1900 + ts->tm_year,
                ts->tm_mon,
                ts->tm_mday,
                ts->tm_hour,
                ts->tm_min,
                ts->tm_sec,
                (int)start_time.tv_usec,
                channel_numbers[i]);
        
        files[i] = fopen(file_name, "wb");
        
        // Skip sizeof(header) byte, becouse we wright header, 
        // when will be know time of finish recording file
        fseek(files[i], sizeof(header), SEEK_SET);
    }

    return E502M_ERR_OK;
}

void close_files(FILE **files, int files_count,  header *hdr)
{
    for(int i = 0; i < files_count; ++i)
    { 
        // set pointer in stream to begin
        // it's necessary, because information, which
        // represented by sctructure header
        // must be placed in the begin of data-file 
        fseek(files[i], 0, SEEK_SET);
        
        fwrite(hdr, sizeof(header), 1, files[i]);

        fclose(files[i]);
    }
}
