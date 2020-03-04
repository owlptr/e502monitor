/*
    This file part of e502monitor source code.
    Licensed under GPLv3.

    "config.c contains realization of function for 
    create and use configuration file.

    Author: Gapeev Maksim
    Email: gm16493@gmail.com
 */

#include "config.h"
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <libconfig.h>

static char path_to_config[256] = ""; // path to configuration file 

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
#ifdef DBG
        "# Количество отсчетов, считываемых за блок\n",
        "read_block_size = 20000\n",
        "\n",
        "# Таймаут на прием блока (мс)\n",
        "read_timeout = 2000\n",
        "\n",
#endif // DBG
        "# Размер выходных файлов (в секундах) \n",
        "file_size = 900\n",
        "\n"
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
        "bin_dir = \"data\"\n",
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
        "channel_names = [\"ch0\"]\n",
        "\n",
        "# Количество дней, которые будут сохраняться.\n",
        "# Например, если  count_of_day = 3, то будут сохраняться\n",
        "# два последних дня + текущий\n",
        "count_of_day = 3 \n"
    };


    FILE *cfg_default_file = fopen(path_to_config, "w");

    if(!cfg_default_file){ return E502M_ERR; }

    for(int i = 0; i < (int)sizeof(default_config) / sizeof(char *); i++)
    {
        fprintf(cfg_default_file, "%s", default_config[i]);
    }

    fclose(cfg_default_file);

    return E502M_ERR_OK;
}

int init_config(e502monitor_config **config)
{
    // get user name
    char* user_name = getenv("USER");
    strcat(path_to_config, "/home/");
    strcat(path_to_config, user_name);
    strcat(path_to_config, "/.config/e502monitor/e502monitor.cfg");

    char config_dir_path[256] = "/home/";
    strcat(config_dir_path, user_name);
    strcat(config_dir_path, "/.config/e502monitor");

    // if configuration directory are not exist create it
    struct stat st = {0};

// #ifdef DBG
    printf("Path to configuration file:\t%s\n", path_to_config);
    printf("Configuration directory:\t%s\n", config_dir_path);
// #endif

    if( stat(config_dir_path, &st) == -1 )
    {
               if( mkdir(config_dir_path, 0700) != 0) 
        {
            printf("Не могу создать директорию для конфигурационного файла.\n"
                    "Ошибка в пути? Нет прав на запись?\n");

            logg("Не могу создать директорию для конфигурационного файла.\n"
                    "Ошибка в пути? Нет прав на запись?\n");

            return E502M_ERR;
        } 
    }

    e502monitor_config* e502m_cfg = *config;

    config_t cfg; 

    config_init(&cfg);

    if(!config_read_file(&cfg, path_to_config))
    {
        printf("Не найден конфигурацйионный файл e502monitor.cfg\n");
        printf("Создаю конфигурационный файл по-умолчанию\n");
        
        // test!
        return E502M_ERR;
        //

        if( create_default_config() == E502M_ERR_OK )
        {
            if( !config_read_file(&cfg, path_to_config) )
            {
                printf("Ошибка при открытиии конфигурационного файла по-умолчанию\n");
                return E502M_ERR;
            }
        } else {
            printf("Не удалось создать конфигурационный файл по-умолчанию. "
                    "Нет прав на запись в текущую директорию???\n");
            return E502M_ERR;
        } 
    }

    int found_value;
    int err = config_lookup_int(&cfg,
                                "channel_count",
                                &e502m_cfg->channel_count);
    if(err == CONFIG_FALSE)
    { 
        printf("Ошибка конфигурационного файла:\tколичество каналов не задано!\n");
        
        config_destroy(&cfg);
        return E502M_ERR;
    }

    err = config_lookup_float(&cfg, "adc_freq", &e502m_cfg->adc_freq);
    if(err == CONFIG_FALSE)
    { 
        printf("Ошибка конфигурационного файла:\tчастота сбора АЦП не задана!\n");
        
        config_destroy(&cfg);
        return E502M_ERR;
    }

#ifdef DBG
    err = config_lookup_int(&cfg, "read_block_size", &e502m_cfg->read_block_size);
    if(err == CONFIG_FALSE)
    { 
        printf("Ошибка конфигурационного файла:\tразмер блока для чтения не задан!\n");
        
        config_destroy(&cfg);
        return E502M_ERR;
    }
#else
    e502m_cfg->read_block_size = e502m_cfg->adc_freq / 2;
#endif // DBG

 #ifdef DBG    
    err = config_lookup_int(&cfg, "read_timeout", &e502m_cfg->read_timeout);
    if(err == CONFIG_FALSE)
    { 
        printf("Ошибка конфигурационного файла:\tвремя задержки перед чтением блока не задано!\n");
        
        config_destroy(&cfg);
        return E502M_ERR;
    }
#else
    e502m_cfg->read_timeout = 2000;
#endif // DBG

    err = config_lookup_int(&cfg, "count_of_day", &e502m_cfg->stored_days_count);
    if(err == CONFIG_FALSE)
    { 
        printf("Ошибка конфигурационного файла:\tвременное окно не задано!\n");
        
        config_destroy(&cfg);
        return E502M_ERR;
    }

    err = config_lookup_int(&cfg, "file_size", &e502m_cfg->file_size);
    if(err = CONFIG_FALSE)
    {
        printf("Ошибка конфигурационного файла:\t размер выходных файлов не задан!\n");

        config_destroy(&cfg);
        return E502M_ERR;
    }

    config_setting_t *channel_numbers = config_lookup(&cfg, "channel_numbers");
    if(channel_numbers == NULL)
    {
        printf("Ошибка конфигурационного файла:\tномера каналов не заданы!\n");
        
        config_destroy(&cfg);
        return E502M_ERR;
    }

    int size = config_setting_length(channel_numbers);
    if(size != e502m_cfg->channel_count)
    {
        printf("Ошибка конфигурационного файла:\t размер "
               "массива channel_numbers не равен channel_count\n");
        config_destroy(&cfg);
        
        return E502M_ERR;   
    }

    e502m_cfg->channel_numbers = (int*)malloc(sizeof(int)*e502m_cfg->channel_count);

    for(int i = 0; i < e502m_cfg->channel_count; i++)
    {
        e502m_cfg->channel_numbers[i] = config_setting_get_int_elem(channel_numbers, i);
    }

    config_setting_t *channel_modes = config_lookup(&cfg, "channel_modes");
    if(channel_modes == NULL)
    {
        
        printf("Ошибка конфигурационного файла:\tрежимы каналов не заданы!\n");
        
        config_destroy(&cfg);
        return E502M_ERR;

    }

    size = config_setting_length(channel_modes);
    if(size != e502m_cfg->channel_count)
    {
        printf("Ошибка конфигурационного файла:\t размер "
               "массива channel_modes не равен channel_count\n");
        config_destroy(&cfg);
        
        return E502M_ERR;   
    }

    e502m_cfg->channel_modes = (int*)malloc(sizeof(int)*e502m_cfg->channel_count);

    for(int i = 0; i < e502m_cfg->channel_count; i++)
    {
        e502m_cfg->channel_modes[i] = config_setting_get_int_elem(channel_modes, i);
    }

    config_setting_t *channel_ranges = config_lookup(&cfg, "channel_ranges");
    if(channel_ranges == NULL)
    {
        
        printf("Ошибка конфигурационного файла:\tдиапазоны каналов не заданы!\n");
        
        config_destroy(&cfg);
        return E502M_ERR;

    }

    size = config_setting_length(channel_ranges);
    if(size != e502m_cfg->channel_count)
    {
        printf("Ошибка конфигурационного файла:\t размер "
               "массива channel_ranges не равен channel_count\n");
        config_destroy(&cfg);
        
        return E502M_ERR;   
    }

    e502m_cfg->channel_ranges = (int*)malloc(sizeof(int)*e502m_cfg->channel_count);

    for(int i = 0; i < e502m_cfg->channel_count; i++)
    {
        e502m_cfg->channel_ranges[i] = config_setting_get_int_elem(channel_ranges, i);
    }

    char* bd;
    err = config_lookup_string(&cfg, "bin_dir", &bd);
    if(err == CONFIG_FALSE)
    {
        printf("Ошибка конфигурационного файла:\t директория для выходных файлов не задана\n");
        config_destroy(&cfg);

        return E502M_ERR;
    }

    strcpy(e502m_cfg->bin_dir, bd);

    char *mn;
    err = config_lookup_string(&cfg, "module_name", &mn);
    if(err == CONFIG_FALSE)
    {
        printf("Ошибка конфигурационного файла:\tмодель АЦП не задана!\n");
        config_destroy(&cfg);

        return E502M_ERR;
    }

    strcpy(e502m_cfg->module_name, mn);

    char *p;
    err = config_lookup_string(&cfg, "place", &p);
    if(err == CONFIG_FALSE)
    {
        printf("Ошибка конфигурационного файла:\t место работы АЦП не задано!\n");
        
        config_destroy(&cfg);
        return E502M_ERR;
    }

    strcpy(e502m_cfg->place, p);

    config_setting_t *channel_names = config_lookup(&cfg, "channel_names");

    if(channel_ranges == NULL)
    {
        
        printf("Ошибка конфигурационного файла:\tназвания каналов не заданы!\n");
        
        config_destroy(&cfg);
        return E502M_ERR;

    }

    size = config_setting_length(channel_names);
    if(size != e502m_cfg->channel_count)
    {
        printf("Ошибка конфигурационного файла:\t размер "
               "массива channel_names не равен channel_count\n");
        config_destroy(&cfg);
        
        return E502M_ERR;   
    }

    e502m_cfg->channel_names = (char**)malloc(sizeof(char*)*e502m_cfg->channel_count);

    for(int i = 0; i < e502m_cfg->channel_count; i++)
    {
        e502m_cfg->channel_names[i] = (char*)malloc(sizeof(char)*10);

        strcpy(e502m_cfg->channel_names[i], config_setting_get_string_elem(channel_names, i));
    }

    config_setting_t* flac = config_lookup(&cfg, "flac");

    if(flac == NULL)
    {
        printf("Ошибка конфигурационного файла:\t "
               "распределение каналов по файлам не задано\n");
        
        config_destroy(&cfg);
        return E502M_ERR;
    }

    size = config_setting_length(flac);
    
    e502m_cfg->channel_distribution = (int**)malloc(sizeof(int*)*size);
    e502m_cfg->channel_counts_in_files = (int*)malloc(sizeof(int)*size);
    e502m_cfg->files_count = size;

    for(int i = 0; i < size; i++)
    {
        config_setting_t* flac_file_cfg = config_setting_get_elem(flac, i);

        int inner_size = config_setting_length(flac_file_cfg);

        e502m_cfg->channel_distribution[i] = (int*)malloc(sizeof(int)*inner_size);

        e502m_cfg->channel_counts_in_files[i] = inner_size;

        for(int j = 0; j < inner_size; j++)
        {
            // printf("%d\n", config_setting_get_int_elem(flac_file_cfg, j));
            e502m_cfg->channel_distribution[i][j] = config_setting_get_int_elem(flac_file_cfg, j);
            // printf("%d\n", config_setting_get_int_elem(flac_file_cfg, j));
        }

    }


    config_destroy(&cfg);    

    return E502M_ERR_OK;
}

void print_config(e502monitor_config *config)
{
    printf("\nЗагружена следующая конфигурация модуля:\n");
    printf(" Количество используемых логических каналов\t\t:%d\n", config->channel_count);
    printf(" Частота сбора АЦП в Гц\t\t\t\t\t:%f\n", config->adc_freq);
    printf(" Количество отсчетов, считываемых за блок\t\t:%d\n", config->read_block_size);
    printf(" Размер файлов (в секундах)\t\t\t\t:%d\n", config->file_size);
    printf(" Таймаут перед считываением блока (мс)\t\t\t:%d\n", config->read_timeout);
    printf(" Количество сохраняемых дней\t\t\t\t:%d\n", config->stored_days_count);
    printf(" Номера используемых каналов\t\t\t\t:[ ");
    for(int i = 0; i < config->channel_count; i++)
    {
        printf("%d ", config->channel_numbers[i]);
    }
    printf("]\n");

    printf(" Режимы измерения каналов\t\t\t\t:[ ");
    for(int i = 0; i < config->channel_count; i++)
    {
        printf("%d ", config->channel_modes[i]);
    }
    printf("]\n");

    printf(" Диапазоны измерения каналов\t\t\t\t:[ ");
    for(int i = 0; i < config->channel_count; i++)
    {
        printf("%d ", config->channel_ranges[i]);
    }
    printf("]\n");
    
    printf(" Директория выходных файлов\t\t\t\t:%s\n", config->bin_dir);
    printf(" Модель АЦП\t\t\t\t\t\t:%s\n", config->module_name);
    printf(" Текущее место работы\t\t\t\t\t:%s\n", config->place);
    
    printf(" Названия используемых каналов\t\t\t\t:[ ");
    for(int i = 0; i < config->channel_count; ++i)
    {
        printf("%s ", config->channel_names[i]);
    }
    printf("]\n");

    printf(" Распределение каналов по файлам\t\t\t:[");
    for(int i = 0; i < config->files_count; i++)
    {   
        printf(" (");
        for(int j = 0; j < config->channel_counts_in_files[i]; j++)
        {
            printf(" %d ", config->channel_distribution[i][j]);
        }
        printf(") ");
    }
    printf("]\n\n");
}

e502monitor_config* create_config()
{   
    e502monitor_config* config = (e502monitor_config*)malloc(sizeof(e502monitor_config));

    if(config == NULL){ return NULL; } 

    config->channel_numbers = NULL;
    config->channel_modes   = NULL;
    config->channel_ranges  = NULL;
    config->channel_names   = NULL;

    return config;
}

void destroy_config(e502monitor_config **config)
{
    if( (*config)->channel_numbers != NULL ){free( (*config)->channel_numbers);}
    if( (*config)->channel_modes   != NULL ){free( (*config)->channel_modes);}
    if( (*config)->channel_ranges  != NULL ){free( (*config)->channel_ranges);}
    if( (*config)->channel_names   != NULL ){free( (*config)->channel_names);}

    free( (*config) ) ;
}