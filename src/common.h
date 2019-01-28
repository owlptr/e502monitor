/*
    "common.h" contains some of defines

    Author: Gapeev Maksim
*/

#ifndef COMMON_H
#define COMMON_H

#define E502M_ERR_OK 0
#define E502M_ERR -1
#define E502M_EXIT_FAILURE -2

// #define FILE_TIME 900 // create 15 minutes files... 15 min = 900 sec
// #define FILE_TIME 600 // 10 minutes files
// #define FILE_TIME 300 // 5 minutes files
#define FILE_TIME 60 // 1 minut files
// #define FILE_TIME 10 // 10 seconds files

#define MAX_CHANNELS 16

#define TCP_CONNECTION_TOUT 5000

#endif // COMMON_H
