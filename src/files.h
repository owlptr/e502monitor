/*
    "files.h" contains declaration of functions
    for writing data on disk

    Author: Gapeev Maksim
*/

#ifndef FILES_H
#define FILES_H

#include "header.h"
#include "config.h"

#include <stdio.h>
#include <time.h>
#include <sys/time.h>

/*
    Create files for writing data on disk.

    files           - array of file descriptors.
    files_count     - count of file descriptors.
    start_time      - time of start writing data.
    path            - directory for writing data.
    channel_numbers - numbers of using channels 

    Retutn error index. 
*/
int create_files(FILE **files,
                 int files_count,
                 struct timeval* start_time,
                 char* path,
                 int* channel_numbers);

/*
    Finish writing of files with data

    files       - array of file descriptors.
    files_count - count of file descriptors.
    hdr         - special header with information.
    cfg         - configuration info
*/
void close_files(FILE **files,
                 int files_count,
                 header *hdr,
                 e502monitor_config *cfg);

// /*
//     Create directory.

//     path - directory path.


//     Return error index.
// */
// int create_dir(char *path);

// /*
//     Remove directories.

//     path - directories path.


//     Return error index.
// */
// int remove_dir(char* path, int count);

#endif // FILES_H