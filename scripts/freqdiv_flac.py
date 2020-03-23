#! /usr/bin/env python3

import sys
import os
from datetime import datetime
import struct
import array
import soundfile as sf
# audio_dir = sys.argv[1] 

# audio_files = os.listdir(audio_dir)

# audio_files.sort()

# for af in audio_files:
#     data, freq = sf.read(audio_dir + af)

#     print("Файл: " +  af)
#     print("Количество сэиплов: ", len(data))
#     print("Ожидаемое количество сэмплов: ", freq * 60)
#     print("Разница в количестве сэмплов: ", len(data) - freq * 60)
#     print("Заявленная частота: ", freq)
#     real_freq = len(data) / 900
#     print("Реальная частота: ", real_freq)
#     print("Дивиация частоты: ", abs(real_freq - freq)/freq * freq/100 )

afile = sf.SoundFile("/home/gapeev/e502m_data/2020_03_23/2020_03_23_01-38-06-096547_0.flac")

afile.tell

# print("Файл: " +  af)
print("Количество сэиплов: ", len(data))
print("Ожидаемое количество сэмплов: ", freq * 60)
print("Разница в количестве сэмплов: ", len(data) - freq * 60)
print("Заявленная частота: ", freq)
real_freq = len(data) / 60
print("Реальная частота: ", real_freq)
print("Дивиация частоты: ", abs(real_freq - freq)/freq * freq/100 )

