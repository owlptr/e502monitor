#! /usr/bin/env python3

import sys
import os
from datetime import datetime

tar_output = sys.argv[1]
dir_input  = sys.argv[2]

print("Начало", datetime.now())

os.system("tar -cvzf " + tar_output + " " + dir_input)

print("Окончание", datetime.now())

# Начало    2019-01-09 02:54:52.805267
# Окончание 2019-01-09 03:02:22.065475
