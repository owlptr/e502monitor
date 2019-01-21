#! /usr/bin/env python3

import sys
import os
from datetime import datetime
import struct
import array
# import soundfile as sf
audio_dir = sys.argv[1] 

audio_files = os.listdir(audio_dir)

audio_files.sort()

for i in range(len(audio_files)):
    bin_file = open(audio_dir + audio_files[i], 'rb')
    
    year            = bin_file.read(4)
    month           = bin_file.read(4)
    day             = bin_file.read(4)
    start_hour      = bin_file.read(4)            
    finish_hour     = bin_file.read(4)            
    start_minut     = bin_file.read(4)         
    finish_minut    = bin_file.read(4)         
    start_second    = bin_file.read(4)         
    finish_second   = bin_file.read(4)         
    start_usecond   = bin_file.read(4)        
    finish_usecond  = bin_file.read(4)        
    channel_number  = bin_file.read(4)
    freq            = bin_file.read(8)
    mode            = bin_file.read(4)
    ch_range        = bin_file.read(4)
    module_name     = bin_file.read(51).decode("utf-8")
    place_name      = bin_file.read(101).decode("utf-8")
    channel_name    = bin_file.read(51).decode("utf-8")

    year            = int.from_bytes(year, byteorder='little')
    month           = int.from_bytes(month, byteorder='little') 
    day             = int.from_bytes(day, byteorder='little')
    start_hour      = int.from_bytes(start_hour, byteorder='little')
    finish_hour     = int.from_bytes(finish_hour, byteorder='little')
    start_minut     = int.from_bytes(start_minut, byteorder='little')
    finish_minut    = int.from_bytes(finish_minut, byteorder='little')
    start_second    = int.from_bytes(start_second, byteorder='little')
    finish_second   = int.from_bytes(finish_second, byteorder='little')
    start_usecond   = int.from_bytes(start_usecond, byteorder='little')
    finish_usecond  = int.from_bytes(finish_usecond, byteorder='little')
    channel_number  = int.from_bytes(channel_number, byteorder='little')
    freq            = struct.unpack('d', freq)[0]
    mode            = int.from_bytes(mode, byteorder='little')
    ch_range        = int.from_bytes(ch_range, byteorder='little')

    bin_file.read(5)

    data = array.array('d', bin_file.read())

    td = None

    if(i != len(audio_files) - 1):
        # print(audio_files[i])
        af_prev = audio_files[i].split("_")
        time_prev = af_prev[3].split("-")
        af_next = audio_files[i + 1].split("_")
        time_next = af_next[3].split("-")

        time1 = datetime(int(af_prev[0]), int(af_prev[1]), int(af_prev[2]), int(time_prev[0]),
                         int(time_prev[1]), int(time_prev[2]), int(time_prev[3]) )
        time2 = datetime( int(af_next[0]), int(af_next[1]), int(af_next[2]), int(time_next[0]),
                         int(time_next[1]), int(time_next[2]), int(time_next[3]) )

        # print(time1)
        # print(time2)
        td = time2 - time1
        print(td)
        # print()



    if(td != None):
        print("Файл: " +  audio_files[i])
        print("Количество сэмплов: ", len(data))
        print("Частота: ", freq)
        expect_samples = freq *  td.seconds + td.microseconds * freq / 1E6 
        print("Ожидаемое количество сэмплов", expect_samples)
        real_samples = len(data)
        print("Разница в сэплах:", abs( real_samples - expect_samples ) )
        print("Заявленная частота: ", freq)
        real_freq = len(data) / (td.seconds + td.microseconds / 1E6)
        print("Реальная частота: ", real_freq)
        print("Дивиация частоты: ", abs(real_freq - freq)/freq * freq/100 )
        print()
