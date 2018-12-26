/*
    Author: Gapeev Maksim
*/

#include "pdouble_queue.h"
#include "config.h"
#include "common.h"

#include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
#include <signal.h>
// #include <time.h>
// #include <pthread.h>
// #include <dirent.h>
// #include <sys/stat.h>

// #include <libconfig.h>

static int g_stop = 0; // if equal 1 - stop working

/*
    Create stop event heandler for
    correct completion of data collection 
*/
void create_stop_event_handler();

/*
    Signal of completion 
*/
void abort_handler(int sig);

int main(int argc, char** argv)
{
    create_stop_event_handler();

    pdouble_queue* data = create_pdouble_queue();

    e502monitor_config* config = create_config();

    if( config == NULL )
    {
        printf("Ошибка создания конфигурации!\n");
        
        destroy_pdouble_queue(data);
        
        return E502M_EXIT_FAILURE;
    }

    print_config(config);

    destroy_pdouble_queue(data);

    return 0;
}



void abort_handler(int sig){ g_stop = 1; }

void create_stop_event_handler()
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = abort_handler;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
}