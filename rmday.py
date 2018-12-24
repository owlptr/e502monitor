#! /usr/bin/env python3

import os
import shutil
import sys

# run rmday.py path count_of_remove_days

path = sys.argv[1]
count_of_remove_days = sys.argv[2]

dirs = os.listdir(path)
dirs.sort()

for i in range(int(count_of_remove_days)):
    shutil.rmtree(path + dirs[i])