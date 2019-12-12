/*
    This file part of e502monitor source code.
    Licensed under GPLv3.

    "logging.c" contains realization of functions for
    logging.

    Author: Gapeev Maksim
    Email: gm16493@gmail.com
*/

#include "logging.h"

#include <stdio.h>
#include <time.h>
#include <sys/time.h>

void logg(char *msg)
{
    FILE *log_file = fopen("e502monitor.log", "a+");

    // get time
    struct timeval current_time;
    struct tm *ts;

    gettimeofday(&current_time, NULL);

    ts = gmtime(&current_time.tv_sec);

    char current_time_str[300] = "";

    sprintf(current_time_str,
            "%d/%02d/%02d %02d:%02d:%02d\t",
            1900 + ts->tm_year,
            ts->tm_mon + 1,
            ts->tm_mday,
            ts->tm_hour,
            ts->tm_min,
            ts->tm_sec);

    fprintf(log_file, current_time_str);
    fprintf(log_file, msg);
    fprintf(log_file, "\n");

    fclose(log_file);
}