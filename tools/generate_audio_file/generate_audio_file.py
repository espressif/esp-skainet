# -*- coding:utf-8 -*-
from __future__ import print_function
import os
import struct
import wave

import contextlib

def read_wave(path):
    with contextlib.closing(wave.open(path, 'rb')) as wf:
        num_channels = wf.getnchannels()
        assert num_channels == 1
        sample_width = wf.getsampwidth()
        assert sample_width == 2
        sample_rate = wf.getframerate()
        assert sample_rate in (8000,16000,48000)
        pcm_data = wf.readframes(wf.getnframes())
        return pcm_data, sample_rate


def get_wave_array_str(filename, target_bits):
    pcm_data, sr = read_wave(filename)
    array_str = ""
    i =0
    for data in pcm_data:
        #val = struct.unpack("B", data)
        array_str += "%0#4x, "%(data)
        if (i + 1) % 16 == 0:
            array_str += "\n"
        i = i + 1
    return array_str


#def get_wave_array_str(filename, target_bits):
#    with open(filename, "rb") as f:
#        array_str = ""
#        head = f.read(44)
#        i = 0
#        while (1):
#            data = f.read(1)
#            if len(data) == 0:
#                break
#            if len(data) == 1:
#                val = struct.unpack("B", data)
#            array_str += "%0#4x, "%(val)
#            if (i + 1) % 16 == 0:
#                array_str += "\n"
#            i = i + 1
#    return array_str

def gen_wave_table(wav_file_list, scale_bits = 8):
    for wav in wav_file_list:
        print("processing: {}".format(wav))
        (filepath,tempfilename) = os.path.split(wav)
        (shotname,extension) = os.path.splitext(tempfilename)
        h_file_name = shotname + ".h"
        with open(h_file_name, "w") as audio_table:
            print('#include <stdio.h>', file=audio_table)
            name_array = "const unsigned char " + shotname + "[] = {"
            print(name_array, file=audio_table)
            print(get_wave_array_str(filename = wav, target_bits = scale_bits), file=audio_table)
            print('};\n', file=audio_table)
            print("Done...")

if __name__=='__main__':
    print("Generating audio array...")
    wav_list = []
    for filename in os.listdir("./"):
        if filename.endswith(".wav"):
            wav_list.append(filename)
    gen_wave_table(wav_file_list = wav_list)
