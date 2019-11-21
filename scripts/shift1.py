#! /usr/bin/env python3

import sys
import soundfile as sf
# import matplotlib.pyplot as plt
from datetime import datetime

time_shift = datetime(2019, 1, 14, 4, 00, 0) - datetime(2019, 1, 14, 3, 52, 40, 506670) 

audio_file1 = sys.argv[1]
audio_file2 = sys.argv[2]

data1, freq1 = sf.read(audio_file1)
data2, freq2 = sf.read(audio_file2)

print("Shift between " + audio_file1.split("/")[len(audio_file1.split("/"))-1] +\
      " and " + audio_file2.split("/")[len(audio_file1.split("/"))-1])

print("flac len = \t", len(data1))
print("wav len = \t", len(data2))

# for d in data1:
#     print(d)
# exit()

shift = int( time_shift.seconds * freq1 + (freq1/1E6) * time_shift.microseconds ) 
print("alignment = \t", shift)

prev_count = data1[0][0]
count1 = 0
for d1 in data1[1:]:
    next_count = d1[0]
    if prev_count <= 0 and next_count >= 0:
        break
    count1 += 1
    prev_count = next_count

prev_count = data2[shift]
count2 = 0
for d2 in data2[shift + 1:]:
    next_count = d2
    if prev_count <= 0 and next_count >= 0:
        break
    count2 += 1
    prev_count = next_count

print("=======Begin========")
print("counters = ",abs(count1 - count2))
print("seconds = ", abs(count1 - count2) / freq1)

prev_count = data2[shift]
count1 = 0
count11 = 0
last_count = 0 
for d1 in data2[shift + 1:]:
    next_count = d1
    if prev_count <= 0 and next_count >= 0:
        count11 += 1
        last_count = count1
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
        count22 += 1 
        if count22 == count11:
            break
    count2 += 1
    prev_count = next_count

# print("count1 = ", count1)
# print("count2 = ", count2)
# print("count11 = ", count11)
# print("count22 = ", count22)
print("=======End========")
print("counters = ", abs(count1 - count2))
print("seconds = ", abs(count1 - count2) / freq1)

print()

# fig, ax = plt.subplots()
# ax.plot(data[:1000])

# plt.show()



# print(data[0])
