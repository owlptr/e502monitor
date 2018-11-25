#! /usr/bin/env python3

import matplotlib
import matplotlib.pyplot as plt

import array
import struct

file = open('2018_10_21-00:14:21:754952-0', 'rb')

year            = file.read(4)
month           = file.read(4)
day             = file.read(4)
start_hour      = file.read(4)            
finish_hour     = file.read(4)            
start_minut     = file.read(4)         
finish_minut    = file.read(4)         
start_second    = file.read(4)         
finish_second   = file.read(4)         
start_usecond   = file.read(4)        
finish_usecond  = file.read(4)        
channel_number  = file.read(4)
freq            = file.read(8)
mode            = file.read(4)
module_name     = file.read(11).decode("utf-8")
range           = file.read(4)
place_name      = file.read(15).decode("utf-8")

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
range           = int.from_bytes(range, byteorder='little')

print("Время начала записи -->\t\t" + str(year) + ":" + \
       str(start_hour) + ":" + str(start_minut) + ":"  + \
       str(start_second) + ":" + str(start_usecond))

print("Время окончания записи -->\t" + str(year) + ":" + \
       str(finish_hour) + ":" + str(finish_minut) + ":" + \
       str(finish_second) + ":" + str(finish_usecond))


print("Номер канала --> \t\t" + str(channel_number))
print("Частота --> \t\t\t" + str(freq))
print("Режим измерения --> \t\t" + str(mode))
print("Модель АЦП --> \t\t\t" + module_name)
print("Диапазон измерения --> \t\t" + str(range))
print("Место измерения --> \t\t" + place_name)
# # byte_data = file.read(16000)

file.close()

# double_data = array.array('d', byte_data)

# # for d in double_data:
#     # print(d)


# fig, ax = plt.subplots()
# ax.plot(double_data)

# plt.show()