#! /usr/bin/env python3

import sys
import soundfile as sf
import struct
from os import path
import os
import array

in_files  = sys.argv[1]
out_path = sys.argv[2]

bin_file_names = []
if path.isfile(in_files):
    bin_file_names.append(in_files)
elif path.isdir(in_files):
    for in_file in os.listdir(in_files):
        bin_file_names.append(in_files + in_file)

print(bin_file_names)

for bfn in bin_file_names:
    bin_file = open(bfn, 'rb')
    
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

    #create .prop - file
    name = bfn.split("/").pop()

    prop_file = open(out_path + name + ".prop", "w")

    prop_file.write("Время начала записи\t\t:" + str(year) + ":" + \
       str(start_hour) + ":" + str(start_minut) + ":"  + \
       str(start_second) + ":" + str(start_usecond))

    prop_file.write("\nВремя окончания записи\t:" + str(year) + ":" + \
       str(finish_hour) + ":" + str(finish_minut) + ":" + \
       str(finish_second) + ":" + str(finish_usecond))


    prop_file.write("\nНомер канала\t\t\t:" + str(channel_number))
    prop_file.write("\nЧастота\t\t\t\t\t:" + str(freq))
    prop_file.write("\nРежим измерения\t\t\t:" + str(mode))
    prop_file.write("\nДиапазон измерения\t\t:" + str(ch_range))

    prop_file.write("\nМодель АЦП\t\t\t\t:")
    for c in module_name:
        if c.isdigit() or c.isalpha() or c.isspace():
            prop_file.write(c)

    prop_file.write("\nМесто измерения\t\t\t:")
    for c in place_name:
        if c.isdigit() or c.isalpha() or c.isspace():
            prop_file.write(c)    

    prop_file.write("\nНазвание канала\t\t\t:")
    for c in channel_name:
        if c.isdigit() or c.isalpha() or c.isspace():
            prop_file.write(c)    

    prop_file.close()


    bin_file.read(5) # skip 5 bytes

    data = array.array('d', bin_file.read())

    sf.write(out_path + name + ".wav", data, int(freq))

    bin_file.close()

