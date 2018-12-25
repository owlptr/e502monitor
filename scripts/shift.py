#! /usr/bin/env python3

import sys
import soundfile as sf
import matplotlib.pyplot as plt

audio_file1 = sys.argv[1]
audio_file2 = sys.argv[2]

data1, freq1 = sf.read(audio_file1)
data2, freq2 = sf.read(audio_file2)

prev1 = data1[0][0]
count1 = 0
for d1 in data1[0]:
    prev = d1
    if prev1 < 0 and prev > 0 or prev1 > 0 and prev < 0:
        break
    count1 += 1

prev2 = data2[0]
count2 = 0
for d2 in data2:
    prev = d2
    if prev2 < 0 and prev > 0 or prev2 > 0 and prev < 0:
        break
    count2 += 1



print(abs(count1 - count2))
print()
print(abs(count1 - count2) / freq1)


prev1 = data1[0][0]
count1 = 0
count11 = 0
for d1 in data1[0]:
    prev = d1
    if prev1 < 0 and prev > 0 or prev1 > 0 and prev < 0:
        count11 += 1
    count1 += 1

prev2 = data2[0]
count2 = 0
count22 = 0
for d2 in data2:
    prev = d2
    if prev2 < 0 and prev > 0 or prev2 > 0 and prev < 0:
        count22 += 1
        if count22 == count11: break 
    count2 += 1


print(abs(count1 - count2))
print()
print(abs(count1 - count2) / freq1)

# fig, ax = plt.subplots()
# ax.plot(data[:1000])

# plt.show()



# print(data[0])
