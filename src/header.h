/*
    This file part of e502monitor source code.
    Licensed under GPLv3.
    
    "header.h" contains declaration of output bin file's header.  

    Author: Gapeev Maksim
    Email: gm16493@gmail.com
*/

#ifndef HEADER_H
#define HEADER_H

#include <time.h>

// The structure that stores header for output bin files.
typedef struct {
        int      start_year;              // Year of creating bin file
        int      finish_year;
        int      start_month;            // Month of creating bin file
        int      finish_month;
        int      start_day;                    // Day of start of the recording
        int      finish_day;
        int      start_hour;             // Hour of start of the recording
        int      finish_hour;            // Hour of finish of the recording
        int      start_minut;            // Minut of start of the recording
        int      finish_minut;           // Minut of finish of the recording
        int      start_second;           // Second of start of the recording
        int      finish_second;          // Second of finish of the recording
        int      start_usecond;          // Microseconds of start recording
        int      finish_usecond;         // Microseconds of finish recording
        int      channel_number;         // Number of used channel 
        double   adc_freq;               // Frequancy of ADC
        int      mode;                   // Operation mode 
        int      channel_range;          // Channel measurement range
        char     module_name[51];        // Name of using ADC 
        char     place[101];             // Place of recording
        char     channel_name[51];       // Name of channel
} header;


#endif // HEADER_H