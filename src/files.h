/*
    This file part of e502monitor source code.
    Licensed under GPLv3.

    "files.h" contains declaration of functions
    for writing data on disk

    Author: Gapeev Maksim
    Email: gm16493@gmail.com
*/

#ifndef FILES_H
#define FILES_H

#include "header.h"
#include "config.h"

#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#include <sndfile.h>

typedef struct 
{   
    int samples_count;
    char** channel_names; 
    double freq_diviation; // in % 
} file_prop;

/*
    Create files for writing data on disk.

    files             - array of file descriptors.
    files_count       - count of file descriptors.
    start_time        - time of start writing data.
    path              - directory for writing data.
    channel_numbers   - numbers of using channels 
    stored_file_names - array for stored file names 

    Retutn error index. 
*/
int create_files(FILE **files,
                 int files_count,
                 struct timeval* start_time,
                 char* path,
                 int* channel_numbers,
                 char** stored_file_names);

/*
    Finish writing of files with data

    files       - array of file descriptors.
    dir_name    - output data directory name.
    file_names  - names of close files.
    files_count - count of file descriptors.
    hdr         - special header with information.
    cfg         - configuration info
*/
void close_files(FILE **files,
                 char* dir_name,
                 char** file_names,
                 int files_count,
                 header *hdr,
                 e502monitor_config *cfg);

/*
    Create multichannel flac-files instead of binary files

    files             - array of file descriptors.
    files_count       - count of file descriptors.
    start_time        - time of start writing data.
    path              - directory for writing data.
    channel_numbers   - numbers of using channels 
    stored_file_names - array for stored file names 

    Retutn error index. 
 */
int create_flac_files(SNDFILE **files,
                      int files_count,
                      struct timeval* start_time,
                      char* path,
                      int* channel_numbers,
                      char** stored_file_names,
                      int* channel_counts_in_files,
                      double adc_freq);

void close_flac_files(SNDFILE **files,
                      char* dir_name,
                      char** file_names,
                      int files_count,
                      header *hdr,
                      e502monitor_config *cfg);

/*

    Return error index.
*/
int prepare_output_directory(char* path,
                             struct tm* start_time,
                             char* dir_name);

/*
    Remove one stored day.

    path - path to data.

    Return error index.
*/
int remove_day(char *path);

/*
    Checks the need to clean the directory.

    path              - path to data dir.
    current_day       - current day as special string.
    stored_days_count - count of stored days. 

    Return count of day, that must be removed.
    Return 0 if clearing isn't necessary. 
    If something was wrong return error index ( result < 0 ).
*/
int is_need_clear_dir(char *path,
                      char *current_day,
                      int stored_days_count);

/*
    Remove days.

    path        - path to data directory.
    current_day - current day as special string.
    count       - count of remove days. 

    Return error index.
*/
int remove_days( char *path, char *current_day, int count );

void create_prop_file(char** file_name,
                      int    file_id,
                      int    samples_count,
                      e502monitor_config *config);

#endif // FILES_H