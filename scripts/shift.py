#! /usr/bin/env python3

import sys
import soundfile as sf
# import matplotlib.pyplot as plt
from datetime import datetime

time_shift = datetime(2019, 1, 14, 3, 15, 0) - datetime(2019, 1, 14, 3, 7, 51, 562486) 

audio_file1 = sys.argv[1]
audio_file2 = sys.argv[2]

data1, freq1 = sf.read(audio_file1)
data2, freq2 = sf.read(audio_file2)

print("flac len = \t", len(data1))
print("wav len = \t", len(data2))

# for d in data1:
#     print(d)
# exit()

shift = int( time_shift.seconds * freq1 + (freq1/1E6) * time_shift.microseconds ) 
print("shift = \t", shift)

prev_count = data1[0][0]
count1 = 0
marker1 = 0
for d1 in data1[1:]:
    next_count = d1[0]
    if prev_count <= 0 and next_count >= 0:
        marker1 = 0
        break
    elif prev_count >= 0 and next_count <= 0:
        marker1 = 1
        break
    count1 += 1
    prev_count = next_count

prev_count = data2[shift]
count2 = 0
marker2 = 0
for d2 in data2[shift + 1:]:
    next_count = d2
    if prev_count <= 0 and next_count >= 0:
        marker2 = 0
        break
    elif prev_count >= 0 and next_count <= 0:
        marker2 = 1
        break
    count2 += 1
    prev_count = next_count


if marker1 == marker2:
    print("conters = ",abs(count1 - count2))
    print("seconds = ", abs(count1 - count2) / freq1)
else:  
    print("Too big shift in begining...")

prev_count = data2[shift]
count1 = 0
count11 = 0
marker1 = 0
last_count = 0 
for d1 in data2[shift + 1:]:
    next_count = d1
    if prev_count <= 0 and next_count >= 0:
        count11 += 1
        marker1 = 0
        last_count = count1
    elif prev_count >= 0 and next_count <= 0:
        marker1 = 1
        count11 += 1
        last_count = 1
    count1 += 1
    prev_count = next_count

count1 = last_count

prev_count = data1[0][0]
count2 = 0
count22 = 0
marker2 = 0

for d2 in data1[1:]:
    next_count = d2[0]
    if prev_count <= 0 and next_count >= 0:
        marker2 = 0
        count22 += 1 
        if count22 == count11:
            break
    elif  prev_count >= 0 and next_count <= 0:
        count22 += 1
        marker2 = 1
        if count22 == count11:
            break
    count2 += 1
    prev_count = next_count

# print("count1 = ", count1)
# print("count2 = ", count2)
# print("count11 = ", count11)
# print("count22 = ", count22)

if marker1 == marker2:
    print("counters = ", abs(count1 - count2))
    print("seconds = ", abs(count1 - count2) / freq1)
else:
    print("Too big shift in ending...")


# fig, ax = plt.subplots()
# ax.plot(data[:1000])

# plt.show()



# print(data[0])
