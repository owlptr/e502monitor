/*
    This file part of e502monitor source code.
    Licensed under GPLv3.

    "config.h" contains declaration of functions for 
    create and use configuration file.
    
    Author: Gapeev Maksim
    Email: gm16493@gmail.com
*/

#ifndef CONFIG_H
#define CONFIG_H

typedef struct{

    int       channel_count;            // Count of use logical chnnels
    double    adc_freq;                 // Frequency descritization
    int       read_block_size;          // The size of the data block 
                                        //  that is read at once from the ADC

    int       read_timeout;             // Timeout for receiving data in ms.
    int*      channel_numbers;          // Numbers of using logical channels
    int*      channel_modes;            // Operation modes for channels
    int*      channel_ranges;           // Channel measurement ranges
    char      bin_dir[101];             // Directory of output bin files
    char      module_name[51];          // Name of using module
    char      place[101];               // Name of the recording place
    char**    channel_names;            // Names of using channels 
    int       stored_days_count;        // Count of days that be stored
    int       file_size;                // File size in seconds
    int**     channel_distribution;     // Distribution of channel by files
    int       files_count;              // Count of files for writing
    int*      channel_counts_in_files;  // Count of channels in each flac file
    
} e502monitor_config;

/*
    Creates default "e502monitor.cfg" config file.

    Return error index.
*/
int create_default_config();

/*
    Prints information about configuration.

    config - configuration for printing.
*/
void print_config(e502monitor_config *config);

/*
    Allocates memory for e502monitor_config
    structure.

    Returns pointer to e502monitor_config structure.

    If something was wrong returns NULL.
*/
e502monitor_config* create_config();

/*
    Frees memory from e502monitor_config structure.

    config - pointer to pointer to allocated memory
*/
void destroy_config(e502monitor_config **config);

/*
    Initializes configuration

    config - pointer to pointer 
             to special settings 
             structure.
*/

int init_config(e502monitor_config **config);

#endif // CONFIG_H