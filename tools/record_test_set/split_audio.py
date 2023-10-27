
from fileinput import filename
import os
import time
from pydub import AudioSegment
from pydub.generators import Sine
from playsound import playsound
import csv
import argparse
import yaml
import pandas
import shutil
import serial
from serial.tools import list_ports
import numpy as np
import scipy
import fnmatch
from create_test_set import AudioTools


def align_sine_wave(sine_wav=None, sine_freq=1000, sample_rate=16000, nfft=512):
    f,t,X = scipy.signal.stft(sine_wav, fs=sample_rate, nperseg=nfft, nfft=nfft)
    X = np.abs(X).transpose()
    sine_freq_index = int(sine_freq/sample_rate*nfft)
    sine_num = int((sample_rate-1000)/sine_freq/2)
    count = 0
    for idx, item in enumerate(X):
        energy = np.sum(item) + 1.0
        energy_sine = 0
        for i in range(sine_num):
            freq = sine_freq_index * (2 ** i)
            energy_sine += np.sum(item[freq-1:freq+2])
        if energy_sine/energy > 0.5:
            count += 1
        else:
            count = 0

        if count > 10:
            idx = idx - 12
            return idx*16
    return None

#split recording audio file by csv file
def split_audio(input_path, output_path, sine_pos=2000, tail=100):
    os.makedirs(output_path, exist_ok=True)
    ratio = 1 + 800/555817

    for root, _, files in os.walk(input_path):
        for filename in fnmatch.filter(files,'*.wav'):
            wav_file = os.path.join(root, filename)
            csv_file = wav_file.replace('.wav', '.csv')
            if not os.path.exists(csv_file):
                print(f"{csv_file} not found")
                continue
            audio_data = AudioTools.read_audio_file(wav_file)
            file_data = pandas.read_csv(csv_file).values
            sine_time = align_sine_wave(audio_data.get_array_of_samples(), sine_freq=2000)
            offset = sine_time - sine_pos
            if sine_time == None:
                print(f"sine header not found in {wav_file}")
                continue
            print(filename, sine_time)

            for item in file_data:
                start = int((item[1] + offset - 32) * ratio)
                end = int((item[2] + offset + tail) * ratio)
                clip_data = audio_data[start:end]
                out_file = os.path.join(output_path, "record_"+item[0])
                clip_data.export(out_file, format="wav")


if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-i',
        '--input_path',
        type=str,
        default=None,
        help='Input path of all speech samples'
    )
    parser.add_argument(
        '-o',
        '--output_path',
        type=str,
        default='./data',
        help='Output path to save clips'
    )

    args = parser.parse_args()
    split_audio(args.input_path, args.output_path)