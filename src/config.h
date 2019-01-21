/*
    "config.h" contains declarations of functions for
    program configuration
    
    Author: Gapeev Maksim

*/

#ifndef CONFIG_H
#define CONFIG_H

typedef struct{

    int     channel_count;      // Count of use logical chnnels
    double  adc_freq;           // Frequency descritization
    int     read_block_size;    // The size of the data block 
                                //  that is read at once from the ADC
    int     read_timeout;       // Timeout for receiving data in ms.
    int*    channel_numbers;    // Numbers of using logical channels
    int*    channel_modes;      // Operation modes for channels
    int*    channel_ranges;     // Channel measurement ranges
    char    bin_dir[101];       // Directory of output bin files
    char    module_name[51];    // Name of using module
    char    place[101];         // Name of the recording place
    char**  channel_names;      // Names of using channels 
    int     stored_days_count;  // Count of days that be stored
    
} e502monitor_config;

/*
    Create default "e502monitor.cfg" config file.

    Return error index.
*/
int create_default_config();

/*
    Print information about configuration.

    config - configuration for printing.
*/
void print_config(e502monitor_config *config);

/*
    Allocate memory for e502monitor_config
    structure.

    Return pointer to e502monitor_config structure

    If something was wrong return NULL
*/
e502monitor_config* create_config();

/*
    Free memory from e502monitor_config structure.

    config - pointer to pointer to allocated memory
*/
void destroy_config(e502monitor_config **config);

/*
    Initialize configuration

    config - pointer to pointer 
             to special settings 
             structure.
*/

int init_config(e502monitor_config **config);

#endif // CONFIG_H