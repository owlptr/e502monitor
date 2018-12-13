#! /usr/bin/env python3

import os
import shutil
import sys

dirs = os.listdir(sys.argv[1])
dirs.sort()

shutil.rmtree(sys.argv[1] + dirs[0])