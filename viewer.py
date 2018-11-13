#! /usr/bin/env python3

import matplotlib
import matplotlib.pyplot as plt

import array

file = open('data0', 'rb')

# file.read(152)

byte_data = file.read(800)

file.close()

double_data = array.array('d', byte_data)

# for d in double_data:
    # print(d)


fig, ax = plt.subplots()
ax.plot(double_data)

plt.show()