#! /usr/bin/env python3

import matplotlib
import matplotlib.pyplot as plt

import array

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
start_msecond   = file.read(4)        
finish_msecond  = file.read(4)        
channel_number  = file.read(4)

year            = int.from_bytes(year, byteorder='little')
month           = int.from_bytes(month, byteorder='little') 
day             = int.from_bytes(day, byteorder='little')
start_hour      = int.from_bytes(start_hour, byteorder='little')
finish_hour     = int.from_bytes(finish_hour, byteorder='little')
start_minut     = int.from_bytes(start_minut, byteorder='little')
finish_minut    = int.from_bytes(finish_minut, byteorder='little')
start_second    = int.from_bytes(start_second, byteorder='little')
finish_second   = int.from_bytes(finish_second, byteorder='little')
start_msecond   = int.from_bytes(start_msecond, byteorder='little')
finish_msecond  = int.from_bytes(finish_msecond, byteorder='little')
channel_number  = int.from_bytes(channel_number, byteorder='little')

print(year)
print(month)
print(day)
print(start_hour)
print(finish_hour)
print(start_minut)
print(finish_minut)
print(start_second)
print(finish_second)
print(start_msecond)
print(finish_msecond)
print(channel_number)


# # byte_data = file.read(16000)

file.close()

# double_data = array.array('d', byte_data)

# # for d in double_data:
#     # print(d)


# fig, ax = plt.subplots()
# ax.plot(double_data)

# plt.show()