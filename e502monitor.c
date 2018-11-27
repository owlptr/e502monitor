/*
    Author: Gapeev Maksim
*/

/* For work with Lcard E-502 device */
#include "e502api.h"
/* -------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>

#include <libconfig.h>

#include "pdouble_queue.h"

#define FILE_TIME 900 // create 15 minutes files... 15 min = 900 sec
// #define FILE_TIME 30

#define MAX_CHANNELS 16

#define TCP_CONNECTION_TOUT 5000

#define READ_CONFIG_OK 0 
#define READ_CONFIG_ERROR -1

#define CONFIGURE_OK 0
#define CONFIGURE_ERROR -2

#define WRITE_HEADERS_OK 0
#define WRITE_HEADERS_ERROR -3

#define EXECUTE_OK 0

#define CREATE_OUT_FILES_OK 0
#define CREATE_OUT_FILES_ERROR -4

#define CREATE_DEFAULT_CONFIG_OK 0
#define CREATE_DEFAULT_CONFIG_ERROR -5

// The structure that stores header for output bin files
typedef struct {
        int      year;                   // Year of creating bin file
        int      month;                  // Month of creating bin file
        int      day;                    // Day of start of the recording
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

// ---------------------GLOBAL VARIABLES-------------------------------------
FILE** g_files = NULL;           // Descriptors of ouput binary files

header g_header; 

pdouble_queue*  g_pd_queue = NULL; // queue for store read data

struct timeval g_time_start; 
struct timeval g_time_end;
// struct timeval g_buf_time_start;

int g_channel_count;            // Count of use logical chnnels
double g_adc_freq;              // Frequency descritization
int g_read_block_size;          // The size of the data block 
                                   //  that is read at once from the ADC
             
int    g_read_timeout;                  // Timeout for receiving data in ms.
int*   g_channel_numbers       = NULL;  // Numbers of using logical channels
int*   g_channel_modes         = NULL;  // Operation modes for channels
int*   g_channel_ranges        = NULL;  // Channel measurement range
char   g_bin_dir[101]          = "";    // Directory of output bin files
char   g_module_name[51]       = "";    // Name of using module
char   g_place[101]            = "";    // Name of the recording place
char** g_channel_names         = NULL;  // Names of using channels 

t_x502_hnd* g_hnd = NULL; // module heandler

int g_stop = 0; // if equal 1 - stop working
//---------------------------------------------------------------------------

// Signal of completion                 
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

/*
    Get all Lcard E-502 devices connected via USB 
    or Ethernet interfaces

    Return number of found devices 
*/
uint32_t get_all_devrec(t_x502_devrec **pdevrec_list,
                        uint32_t *ip_addr_list,
                        unsigned ip_cnt)
{
    // number of found devices
    int32_t fnd_devcnt = 0;
    // number of found devices connected via USB interface
    uint32_t usb_devcnt = 0;

    t_x502_devrec *rec_list = NULL;

    // get count of connected devices via USB interface
    E502_UsbGetDevRecordsList(NULL, 0, 0, &usb_devcnt);

    if(usb_devcnt + ip_cnt == 0){ return 0; }

    // allocate memory for the array to save the number of records found
    rec_list = malloc((usb_devcnt + ip_cnt) * sizeof(t_x502_devrec));
    
    // if memory wasn't allocated
    if(rec_list == NULL){ return 0; }

    if (usb_devcnt != 0) 
    {
        int32_t res = E502_UsbGetDevRecordsList(&rec_list[fnd_devcnt],
                                                usb_devcnt, 
                                                0, 
                                                NULL);
        if (res >= 0) { fnd_devcnt += res; }
    }
    
    for (int i=0; i < ip_cnt; i++) 
    {
        if (E502_MakeDevRecordByIpAddr(&rec_list[fnd_devcnt],
                                       ip_addr_list[i],
                                       0,
                                       TCP_CONNECTION_TOUT)
            == X502_ERR_OK // E502_MakeDevRecordByIpAddr(...) == X502_ERR_OK 
            ) 
        {
            fnd_devcnt++;
        }
    }

    // if were some mistake and no one modele
    // was getting
    if(fnd_devcnt == 0)
    {
        free(rec_list);
        return 0;
    }
    // --------------------------------------

    *pdevrec_list = rec_list;

    return fnd_devcnt;
}

/*
    Create connecting for selecting device

    Return special handler (t_x502_hnd)
*/
t_x502_hnd open_device( t_x502_devrec *devrec_list,
                        uint32_t device_id )
{
    t_x502_hnd hnd = X502_Create();

    if (hnd == NULL) 
    {
        fprintf(stderr, "Ошибка создания описателя модуля!");
        return hnd;
    }
    
    /* create connection with module */
    int32_t err = X502_OpenByDevRecord(hnd, &devrec_list[device_id]);
    if (err != X502_ERR_OK) 
    {
        fprintf(stderr,
                "Ошибка установления связи с модулем: %s!",
                X502_GetErrorString(err));

        X502_Free(hnd);
        hnd = NULL;
        return hnd;
    }

    return hnd;
}

int create_default_config()
{
    char *default_config[] = {
        "### Конфигурационный файл для программы e502monitor ###\n",
        "# Параметры, заключенные в квадратные скобки [], - это списки,\n",
        "# если количество значений большего одного, то все они перечисляются\n",
        "# через запятую. Например, channel_numbers = [0, 1, 2]\n",
        "\n",
        "# Количество используемых логических каналов\n",
        "# Максимум: 16\n",
        "channel_count = 1\n", 
        "\n",
        "# Частота дискретизации АЦП в Гц\n",
        "adc_freq = 10000.0\n",
        "\n",
        "# Количество отсчетов, считываемых за блок\n",
        "# read_block_size = 819200; #4096*200\n",
        "read_block_size = 20000\n",
        "\n",
        "# Таймаут на прием блока (мс)\n",
        "read_timeout = 2000\n",
        "\n",
        "# Номера используемых физических каналов\n",
        "# Каждое значение в массиве должно иметь значение от 0 до 15\n",
        "# Количество значений должно равняться channel_count\n",
        "# Значения не могут повторяться!\n",
        "channel_numbers = [0]\n",
        "\n",
        "# Режимы измерения каналов\n",
        "#   0: Измерение напряжения относительно общей земли\n",
        "#   1: Дифференциальное измерение напряжения\n",
        "#   2: Измерения собственного нуля\n",
        "#\n",
        "# Количество значений должно равняться channel_count\n",
        "channel_modes = [1]\n",
        "\n",
        "# Диапазоны измерений каналов\n",
        "#   0: +/- 10V\n",
        "#   1: +/- 5V\n",
        "#   2: +/- 2V\n",
        "#   3: +/- 1V\n",
        "#   4: +/- 0.5V\n",
        "#   5: =/- 0.2V\n",
        "#\n",
        "# Количество значений должно равняться channel_count\n",
        "channel_ranges = [2]\n",
        "\n",
        "# Директория для выходных бинарных файлов (Максимум 100 символов)\n", 
        "bin_dir = \"/data\"\n",
        "\n",
        "# Модель АЦП (Максимум 50 символов)\n",
        "module_name = \"Lcard E-502\"\n",
        "\n",
        "# Место текущей работы АЦП (Максимум 100 символов)\n",
        "place = \"IKIR FEB RAS\"\n",
        "\n",
        "# Названия используемых каналов\n",
        "# Количество значений должно равняться channel_count\n",
        "# (Каждый максимум: 50 символов)\n",
        "channel_names = [\"ch0\"]\n"
    };

    FILE *cfg_default_file = fopen("e502monitor_default.cfg", "w");

    if(!cfg_default_file){ return CREATE_DEFAULT_CONFIG_ERROR; }

    for(int i = 0; i < (int)sizeof(default_config) / sizeof(char *); i++)
    {
        fprintf(cfg_default_file, "%s", default_config[i]);
    }

    fclose(cfg_default_file);
}

/* 
    Initalize variables, which represent startup config
*/
int create_config()
{
    config_t cfg; 

    config_init(&cfg);

    if(!config_read_file(&cfg, "e502monitor.cfg"))
    {
        printf("Не найден конфигурацйионный файл e502monitor.cfg\n");
        printf("Пытаюсь использовать конфигурационный файл по-умолчанию: "
          "e502monitor_default.cfg\n");
        
        if( !config_read_file(&cfg, "e502monitor_default.cfg") )
        {
            printf("Конфигурационный файл по-умолчанию не найден. Создаю его.");
            if( create_default_config() == CREATE_DEFAULT_CONFIG_OK )
            {
                if( !config_read_file(&cfg, "e502monitor_default.cfg") )
                {
                    printf("Ошибка при открытиии конфигурационного файла по-умолчанию\n");
                    return READ_CONFIG_ERROR;
                }
            } else {
                printf("Не удалось создать конфигурационный файл по-умолчанию. "
                        "Нет прав на запись в текущую директорию???\n");
                return READ_CONFIG_ERROR;
            } 

        }
    }

    int found_value;
    int err = config_lookup_int(&cfg, "channel_count", &g_channel_count);
    if(err == CONFIG_FALSE)
    { 
        printf("Ошибка конфигурационного файла:\tколичество каналов не задано!\n");
        
        config_destroy(&cfg);
        return READ_CONFIG_ERROR;
    }

    err = config_lookup_float(&cfg, "adc_freq", &g_adc_freq);
    if(err == CONFIG_FALSE)
    { 
        printf("Ошибка конфигурационного файла:\tчастота сбора АЦП не задана!\n");
        
        config_destroy(&cfg);
        return READ_CONFIG_ERROR;
    }
    
    err = config_lookup_int(&cfg, "read_block_size", &g_read_block_size);
    if(err == CONFIG_FALSE)
    { 
        printf("Ошибка конфигурационного файла:\tразмер блока для чтения не задан!\n");
        
        config_destroy(&cfg);
        return READ_CONFIG_ERROR;
    }
    
    err = config_lookup_int(&cfg, "read_timeout", &g_read_timeout);
    if(err == CONFIG_FALSE)
    { 
        printf("Ошибка конфигурационного файла:\tвремя задержки перед чтением блока не задано!\n");
        
        config_destroy(&cfg);
        return READ_CONFIG_ERROR;
    }

    config_setting_t *channel_numbers = config_lookup(&cfg, "channel_numbers");
    if(channel_numbers == NULL)
    {
        printf("Ошибка конфигурационного файла:\tномера каналов не заданы!\n");
        
        config_destroy(&cfg);
        return READ_CONFIG_ERROR;
    }

    g_channel_numbers = (int*)malloc(sizeof(int)*g_channel_count);

    for(int i = 0; i < g_channel_count; i++)
    {
        g_channel_numbers[i] = config_setting_get_int_elem(channel_numbers, i);
    }

    config_setting_t *channel_modes = config_lookup(&cfg, "channel_modes");
    if(channel_modes == NULL)
    {
        
        printf("Ошибка конфигурационного файла:\tрежимы каналов не заданы!\n");
        
        config_destroy(&cfg);
        return READ_CONFIG_ERROR;

    }

    g_channel_modes = (int*)malloc(sizeof(int)*g_channel_count);

    for(int i = 0; i < g_channel_count; i++)
    {
        g_channel_modes[i] = config_setting_get_int_elem(channel_modes, i);
    }

    config_setting_t *channel_ranges = config_lookup(&cfg, "channel_ranges");
    if(channel_ranges == NULL)
    {
        
        printf("Ошибка конфигурационного файла:\tдиапазоны каналов не заданы!\n");
        
        config_destroy(&cfg);
        return READ_CONFIG_ERROR;

    }

    g_channel_ranges = (int*)malloc(sizeof(int)*g_channel_count);

    for(int i = 0; i < g_channel_count; i++)
    {
        g_channel_ranges[i] = config_setting_get_int_elem(channel_ranges, i);
    }

    char* bd;
    err = config_lookup_string(&cfg, "bin_dir", &bd);
    if(err == CONFIG_FALSE)
    {
        printf("Ошибка конфигурационного файла:\t директория для выходных файлов не задана\n");
        config_destroy(&cfg);

        return READ_CONFIG_ERROR;
    }

    strcpy(g_bin_dir, bd);

    char *mn;
    err = config_lookup_string(&cfg, "module_name", &mn);
    if(err == CONFIG_FALSE)
    {
        printf("Ошибка конфигурационного файла:\tмодель АЦП не задана!\n");
        config_destroy(&cfg);

        return READ_CONFIG_ERROR;
    }

    strcpy(g_module_name, mn);

    char *p;
    err = config_lookup_string(&cfg, "place", &p);
    if(err == CONFIG_FALSE)
    {
        printf("Ошибка конфигурационного файла:\t место работы АЦП не задано!\n");
        
        config_destroy(&cfg);
        return READ_CONFIG_ERROR;
    }

    strcpy(g_place, p);

    config_setting_t *channel_names = config_lookup(&cfg, "channel_names");
    if(channel_ranges == NULL)
    {
        
        printf("Ошибка конфигурационного файла:\tназвания каналов не заданы!\n");
        
        config_destroy(&cfg);
        return READ_CONFIG_ERROR;

    }

    g_channel_names = (char**)malloc(sizeof(char*)*g_channel_count);

    for(int i = 0; i < g_channel_count; i++)
    {
        g_channel_names[i] = (char*)malloc(sizeof(char)*10);

        strcpy(g_channel_names[i], config_setting_get_string_elem(channel_names, i));
    }


    config_destroy(&cfg);
}

/*
    Print information about set config
*/
void print_config()
{
    printf("\nЗагружена следующая конфигурация модуля:\n");
    printf(" Количество используемых логических каналов\t\t:%d\n", g_channel_count);
    printf(" Частота сбора АЦП в Гц\t\t\t\t\t:%f\n", g_adc_freq);
    printf(" Количество отсчетов, считываемых за блок\t\t:%d\n", g_read_block_size);
    printf(" Таймаут перед считываением блока (мс)\t\t\t:%d\n", g_read_timeout);
    
    printf(" Номера используемых каналов\t\t\t\t:[ ");
    for(int i = 0; i<g_channel_count; i++)
    {
        printf("%d ", g_channel_numbers[i]);
    }
    printf("]\n");

    printf(" Режимы измерения каналов\t\t\t\t:[ ");
    for(int i = 0; i<g_channel_count; i++)
    {
        printf("%d ", g_channel_modes[i]);
    }
    printf("]\n");

    printf(" Диапазоны измерения каналов\t\t\t\t:[ ");
    for(int i = 0; i<g_channel_count; i++)
    {
        printf("%d ", g_channel_ranges[i]);
    }
    printf("]\n");
    
    printf(" Директория выходных файлов\t\t\t\t:%s\n", g_bin_dir);
    printf(" Модель АЦП\t\t\t\t\t\t:%s\n", g_module_name);
    printf(" Текущее место работы\t\t\t\t\t:%s\n", g_place);
    
    printf(" Названия используемых каналов\t\t\t\t:[ ");
    for(int i = 0; i<g_channel_count; ++i)
    {
        printf("%s ", g_channel_names[i]);
    }
    printf("]\n");

}

int configure_module()
{
    int32_t err = X502_SetLChannelCount(g_hnd, g_channel_count);

    if(err != X502_ERR_OK){ return CONFIGURE_ERROR; }

    for(int i = 0; i < g_channel_count; ++i)
    {
        err = X502_SetLChannel( g_hnd,
                                i,
                                g_channel_numbers[i],
                                g_channel_modes[i],
                                g_channel_ranges[i],
                                0 );

        if(err != X502_ERR_OK){ return CONFIGURE_ERROR; }
    }

    double frame_freq = g_adc_freq/g_channel_count;

    err = X502_SetAdcFreq(g_hnd, &g_adc_freq, &frame_freq);
    if(err != X502_ERR_OK){ return CONFIGURE_ERROR; }

    //what we realy set...
    printf("\nУстановлены частоты:\n"
           " Частота сбора АЦП\t\t:%0.0f\n"
           " Частота на лог. канал\t\t:%0.0f\n",
           g_adc_freq, frame_freq);

    err = X502_Configure(g_hnd, 0);
    if(err != X502_ERR_OK){ return CONFIGURE_ERROR; }

    err = X502_StreamsEnable(g_hnd, X502_STREAM_ADC);
    if(err != X502_ERR_OK){ return CONFIGURE_ERROR; }

    return CONFIGURE_OK;
}

/*
    Free memory from some global variables
*/
void free_memory()
{
    if(g_pd_queue != NULL) { destroy_pdouble_queue(&g_pd_queue); }
    if(g_channel_numbers != NULL){ free(g_channel_numbers); }
    if(g_channel_modes != NULL){ free(g_channel_modes); }
    if(g_channel_ranges != NULL){ free(g_channel_ranges); }
    if(g_hnd != NULL)
    {
        X502_Close(g_hnd);
        X502_Free(g_hnd);
    }     
    
    if(g_channel_names != NULL){
        
        for(int i = 0; i < g_channel_count; i++)
        {
            free(g_channel_names[i]);
        }

        free(g_channel_names);
    }
}

/*
    Finish writing data-files
*/
void close_files()
{
    struct tm *ts; // time of finish recording
    
    ts = gmtime(&g_time_end.tv_sec);
        
    g_header.finish_hour    = ts->tm_hour;
    g_header.finish_minut   = ts->tm_min;
    g_header.finish_second  = ts->tm_sec;
    g_header.finish_usecond = (int)g_time_end.tv_usec;

    for(int i = 0; i < g_channel_count; ++i){ 

        // set pointer in stream to begin
        // it's necessary, because information, which
        // represented by sctructure header
        // must be placed in the begin of data-file 
        fseek(g_files[i], 0, SEEK_SET);
        
        // // initialize some header values
        g_header.mode = g_channel_modes[i];
        g_header.channel_range = g_channel_ranges[i];
        g_header.channel_number = g_channel_numbers[i];
        strcpy(g_header.channel_name, g_channel_names[i]);

        fwrite(&g_header, sizeof(header), 1, g_files[i]);

        fclose(g_files[i]);
    }
}

/* 
    Print info about module 

    Return error index
*/  
int print_info_about_module()
{
    t_x502_info info;
    uint32_t err = X502_GetDevInfo(g_hnd, &info);
    if (err != X502_ERR_OK) {
        fprintf(stderr, 
                "Ошибка получения серийного информации о модуле: %s!",
                 X502_GetErrorString(err)
                 );
        return err;
    } 

    printf("Установлена связь со следующим модулем:\n");
    printf(" Серийный номер\t\t\t: %s\n", info.serial);
    
    printf(" Наличие ЦАП\t\t\t: %s\n",
           info.devflags & X502_DEVFLAGS_DAC_PRESENT ? "Да" : "Нет");
    
    printf(" Наличие BlackFin\t\t: %s\n",
           info.devflags & X502_DEVFLAGS_BF_PRESENT ? "Да" : "Нет");
    
    printf(" Наличие гальваноразвязки\t: %s\n",
           info.devflags & X502_DEVFLAGS_GAL_PRESENT ? "Да" : "Нет");
    
    printf(" Индустриальное исп.\t\t: %s\n",
           info.devflags & X502_DEVFLAGS_INDUSTRIAL ? "Да" : "Нет");
    
    printf(" Наличие интерф. PCI/PCIe\t: %s\n",
           info.devflags & X502_DEVFLAGS_IFACE_SUPPORT_PCI ? "Да" : "Нет");
    
    printf(" Наличие интерф. USB\t\t: %s\n",
           info.devflags & X502_DEVFLAGS_IFACE_SUPPORT_USB ? "Да" : "Нет");
    
    printf(" Наличие интерф. Ethernet\t: %s\n",
           info.devflags & X502_DEVFLAGS_IFACE_SUPPORT_ETH ? "Да" : "Нет");
    
    printf(" Версия ПЛИС\t\t\t: %d.%d\n",
           (info.fpga_ver >> 8) & 0xFF,
            info.fpga_ver & 0xFF);
    printf(" Версия PLDA\t\t\t: %d\n", info.plda_ver);
    
    if (info.mcu_firmware_ver != 0) {
        printf(" Версия прошивки ARM\t\t: %d.%d.%d.%d\n",
               (info.mcu_firmware_ver >> 24) & 0xFF,
               (info.mcu_firmware_ver >> 16) & 0xFF,
               (info.mcu_firmware_ver >>  8) & 0xFF,
                info.mcu_firmware_ver & 0xFF);
    }

    return err;
}

/*
    Create files for writing data
*/
int create_files()
{
    struct tm *ts; // time of start recording
    
    ts = gmtime(&g_time_start.tv_sec);

    g_files = (FILE*)malloc(sizeof(FILE*)*g_channel_count);

    // part of structure "header" fileds
    // another part will be initialize in close_files() function
    g_header.year               = 1900 + ts->tm_year;
    g_header.month              = ts->tm_mon;
    g_header.day                = ts->tm_mday;
    g_header.start_hour         = ts->tm_hour;
    g_header.start_minut        = ts->tm_min;
    g_header.start_second       = ts->tm_sec;
    g_header.start_usecond      = (int)g_time_start.tv_usec;
    g_header.adc_freq           = g_adc_freq / g_channel_count;
    strcpy(g_header.module_name, g_module_name);
    strcpy(g_header.place, g_place); 
    // ---------------------------------------------------------

    if(g_files == NULL){ return CREATE_OUT_FILES_ERROR; }

    for(int i = 0; i < g_channel_count; ++i)
    {  
        char file_name[500] = "";
        
        sprintf(file_name, 
                "%s/%d_%02d_%02d-%02d:%02d:%02d:%06d-%d",
                g_bin_dir,
                1900 + ts->tm_year,
                ts->tm_mon,
                ts->tm_mday,
                ts->tm_hour,
                ts->tm_min,
                ts->tm_sec,
                (int)g_time_start.tv_usec,
                g_channel_numbers[i]);

        g_files[i] = fopen(file_name, "wb");
        
        // Skip sizeof(header) byte, becouse we wright header, 
        // when will be know time of finish recording file
        fseek(g_files[i], sizeof(header), SEEK_SET);
    }



    return CREATE_OUT_FILES_OK;
}

void* write_data(void *arg)
{
    int ch_cntr, data_cntr; // counters
    int size;
    int total_file_size = FILE_TIME * g_adc_freq / g_channel_count;
    int buffer_state;

    int current_file_size = 0;

    double *data = NULL;

    while(!g_stop)
    {
        pop_from_pdqueue(g_pd_queue, &data, &size, &ch_cntr);

        if(data != NULL)
        {   
            current_file_size += size/g_channel_count;            
            
            if(current_file_size >= total_file_size)
            {
                current_file_size = 0;
                close_files();
                create_files();
            } else {
                for(data_cntr = 0; data_cntr < size; data_cntr++, ch_cntr++)
                {                               
                    if(ch_cntr == g_channel_count){ ch_cntr = 0; } 

                    fwrite(&data[data_cntr],
                            sizeof(double),
                            1,
                            g_files[ch_cntr]);
                }                

                free(data);
            }
        } 
    }
}

int main(int argc, char** argv)
{
    printf("Размер заголовка выходного файла: %d\n", sizeof(header));
    create_stop_event_handler();

    g_pd_queue = create_pdouble_queue();

    uint32_t err = create_config();

    if( err != READ_CONFIG_OK )
    {
        err = create_default_config();
        if ( err != READ_CONFIG_OK )
        {
            printf("Не удалось создать конфигурацию по-умолчанию!\n");
            return err;
        }
        
    }

    print_config();

    // ------------------ find lcard e502 devices --------------------
    uint32_t *ip_addr_list = NULL;
    uint32_t ip_cnt = 0;
    uint32_t fnd_devcnt = 0;

    t_x502_devrec *devrec_list = NULL;

    fnd_devcnt = get_all_devrec(&devrec_list, ip_addr_list, ip_cnt);

    if (fnd_devcnt == 0) {
        printf("Не найдено ни одного модуля. Завершение.\n");
        
        //exit from program
        return 0;
    }
    // -------------------------------------------------------------

    printf("Доступны следующие модули:\n");
    for (int i=0; i < fnd_devcnt; i++) {
        printf("Module № %d: %s, %-9s", i, devrec_list[i].devname,
               devrec_list[i].iface == X502_IFACE_PCI ? "PCI/PCIe" :
               devrec_list[i].iface == X502_IFACE_USB ? "USB" :
               devrec_list[i].iface == X502_IFACE_ETH ? "Ethernet" : "?");
        if (devrec_list[i].iface != X502_IFACE_ETH) {
            printf("Серийные номер: %s\n", devrec_list[i].serial);
        } else {
            printf("Адрес: %s\n", devrec_list[i].location);
        }
    }


    //------------------------------- choose lcard device for working--------------------------
    printf("Введите номер модуля, с которым хотите работать (от 0 до %d)\n", fnd_devcnt-1);
    fflush(stdout);

    uint32_t device_id = -1;

    scanf("%d", &device_id);

    if( device_id < 0 || device_id >= fnd_devcnt)
    {
        printf("\nНеверный номер модуля! Завершение.\n");

        // Free memory
        X502_FreeDevRecordList(devrec_list, fnd_devcnt);
        free(devrec_list);

        return 0; // exit from program
    }
    //----------------------------------------------------------------------------------------

    g_hnd = open_device(devrec_list, device_id);

    if(g_hnd == NULL)
    {
        // Free memory
        X502_FreeDevRecordList(devrec_list, fnd_devcnt);
        free(devrec_list);
        // --------------------------------------------

        return 0;
    }

    print_info_about_module();

    configure_module();

    err = X502_StreamsStart(g_hnd);
    if(err != X502_ERR_OK)
    {
        fprintf(stderr,
                "Ошибка запуска сбора данных: %s!\n",
                X502_GetErrorString(err)
                );

        free_memory();

        return err;
    }


    printf("Сбор данных запущен. Для остановки нажмите Ctrl+C\n");
    fflush(stdout);

    uint32_t adc_size = sizeof(double)*g_read_block_size;
    int32_t  rcv_size;
    uint32_t first_lch;
    
    pthread_t thread;
    pthread_create(&thread, NULL, write_data, NULL);
    pthread_detach(thread);

    // int buffer_index = 0;

    uint32_t* rcv_buf  = (uint32_t*)malloc(sizeof(uint32_t)*g_read_block_size);
    
    double* data;

    int total_file_size = FILE_TIME * g_adc_freq / g_channel_count;

    int current_file_size = 0;

    gettimeofday(&g_time_start, NULL);
    // g_buf_time_start = g_time_start;

    err = create_files();

    if( err != CREATE_OUT_FILES_OK )
    {  
        printf("Ошибка создания описателей выходных файлов!\n");
        free_memory();
        return err;
    }

    while(!g_stop)
    {
        data = (double*)malloc(sizeof(double) * g_read_block_size);

        rcv_size = X502_Recv(g_hnd, rcv_buf, g_read_block_size, g_read_timeout);

        // printf("RCV_SIZE: %d\n", rcv_size);

        X502_GetNextExpectedLchNum(g_hnd, &first_lch);

        adc_size = sizeof(double)*g_read_block_size;

        err = X502_ProcessData(g_hnd, rcv_buf, rcv_size, X502_PROC_FLAGS_VOLT,
                               data, &adc_size, NULL, NULL);

        // if need create new files store moment of
        // reading new block data
        current_file_size += rcv_size / g_channel_count;

        if( current_file_size >= total_file_size )
        {
            // printf("Пишу значений в файл: %d\n", current_file_size); 
            current_file_size = 0;
            // g_buf_time_start = g_time_start;
            gettimeofday(&g_time_start, NULL);
            gettimeofday(&g_time_end, NULL);
        }

        push_to_pdqueue(g_pd_queue, &data, rcv_size, first_lch);
    }

    // close_files();
    free_memory();
    
    return EXECUTE_OK;  
}