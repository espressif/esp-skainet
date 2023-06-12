#!/usr/bin/python3

# This Python script is used to record test set.

from fileinput import filename
import os
import time
from pydub import AudioSegment
from playsound import playsound
import csv
import argparse
import yaml
import pandas
import shutil
import serial
from serial.tools import list_ports

def get_serial_port_name(): 	# Get the name of the serial port the computer is using.
    plist = list(list_ports.comports())
    out = []

    if len(plist) <= 0:
        print("Can not find any port!")
        return out
    else:
        for plist_item in plist: 	# Print the port name and description.
            for port in list(plist_item): 	# Get the port name.
                if "/dev/ttyUSB" in port: 	# Filter the port name.
                    out.append(port)
    return out

class AudioTools(object):
    audio_play_count = 0
    audio_name = ""
    audio_play_done = False
    audio_id_identified = None

    @classmethod
    def read_audio_file(cls, audio_file):
        audio_format = os.path.splitext(audio_file)[1][1:]
        if audio_format not in ["mp3", "wav"]:
            print(f"Warning: skip {audio_file}, only support mp3 and wav format")
            return None
        audio_data = AudioSegment.from_file(audio_file, audio_format)
        audio_data = audio_data.set_frame_rate(16000)
        audio_data = audio_data.set_sample_width(2)
        if audio_data.channels > 1:
            audio_data = audio_data.split_to_mono()[0]

        return audio_data

    @classmethod
    def audio_normalization(cls, src_path, target_db=-36):
        file_list = os.listdir(src_path)
        allfile = []

        for filename in file_list:
            src_file = os.path.join(src_path, filename)
            if os.path.isdir(src_file):
                cls.audio_normalization(src_file, allfile)
            else:
                sound = cls.read_audio_file(src_file)

                loudness = sound.dBFS
                adjust_db = target_db - loudness
                if adjust_db > 0.2:
                    sound_amp = sound.apply_gain(adjust_db)
                    print(f'Amplitude adjustment, {src_file}: from {loudness} to {sound_amp.dBFS} dBA')
                    sound_amp.export(src_file, format='wav')

                allfile.append(src_file)
        return allfile

    @classmethod
    def merge_clean_and_noise(cls, clean_dir, noise_dir, dest_dir, clean_gain, noise_gain, sep_time=2):

        sep_audio = AudioSegment.silent(duration=sep_time*1000, frame_rate=16000)
        clean_audio = AudioSegment.empty()
        noise_audio = AudioSegment.empty()
        if not os.path.exists(dest_dir):  # It will be created if it does not exist
            os.makedirs(dest_dir)

        # merge all clean audio file into one file
        for root, _, files in os.walk(clean_dir):
            for filename in files:
                src_file = os.path.join(root, filename)
                src_audio = cls.read_audio_file(src_file)
                if src_audio != None:
                    clean_audio += src_audio + sep_audio  # add the separator later on, then add the source file to the final result.

        # merge all clean audio file into one file
        for root, _, files in os.walk(noise_dir):
            for filename in files:
                src_file = os.path.join(root, filename)
                src_audio = cls.read_audio_file(src_file)
                if src_audio != None:
                    noise_audio += src_audio
        
        clean_audio = clean_audio.apply_gain(clean_gain)
        noise_audio = noise_audio.apply_gain(noise_gain)
        
        # the len() is not exactly, so replace len() with frame_count()
        while(noise_audio.frame_count() < clean_audio.frame_count()):
            noise_audio = noise_audio + noise_audio
        
        # Returns the raw audio data as an array of (numeric) samples.
        clean_audio_array = clean_audio.get_array_of_samples()
        noise_audio_array = noise_audio.get_array_of_samples()
        # slice in array, not in AudioSegment
        noise_audio_array = noise_audio_array[:len(clean_audio_array)]
        noise_audio = noise_audio._spawn(noise_audio_array)

        # Combine two mono channels into multichannel
        stereo_sound = AudioSegment.from_mono_audiosegments(clean_audio, noise_audio)
        clean_dir = clean_dir[:-1] if clean_dir.endswith("/") else clean_dir
        noise_dir = noise_dir[:-1] if noise_dir.endswith("/") else noise_dir
        clean_part = os.path.split(clean_dir)[-1]
        noise_part = os.path.split(noise_dir)[-1]
        new_file_name = clean_part + '_' + str(clean_gain) + 'dB_' + noise_part + '_' + str(noise_gain) + 'dB.wav'
        new_file_path = os.path.join(dest_dir, new_file_name)
        print(f'new_file_path: {new_file_path}')
        stereo_sound.export(new_file_path, format="wav")

    #read and play audio file from input path one by one
    @classmethod
    def audio_play(cls, path):
        file_list = os.listdir(path)
        play_list = []   # list for playing files one by one
        ports = get_serial_port_name()
        print(ports)
        port_handles = []  # list for port handles
        for port in ports:
            handle = serial.Serial(port, 115200, timeout=50)
            if (handle.isOpen()==True):
                port_handles.append(handle) # add port handles to port_handles list for serial port comms.

        for filename in file_list:
            filepath = os.path.join(path, filename)
            if os.path.isdir(filepath):
                cls.audio_play(config, filepath)
            else:
                if os.path.splitext(filename)[1] == '.wav':
                    audio_format = "wav"
                elif os.path.splitext(filename)[1] == '.mp3':
                    audio_format = "mp3"
                else:
                    continue
                string = f'start,{filename}\n'
                print("==> Start to play: %s,  %s" % (string, time.ctime()))
                for handle in port_handles:  # write port_handle to port_handles list for serial port comms.
                    handle.write(bytes(string, 'utf-8')) 

                start_time = time.time()            # start time of playing
                time.sleep(5)
                playsound(filepath)
                time.sleep(5)
                end_time = time.time()            # end time of playing
                play_list.append((filepath, start_time, end_time))
                print("==> End play : %s" % time.ctime())
                for handle in port_handles:  # write port_handle to port_handles list for serial port comms.
                    handle.write(b'end\n')
                time.sleep(5)
                

        basedir = os.path.dirname(path) # get base directory of the path
        csv_file = os.path.join(basedir, "play_list.csv")

        play_set = pandas.DataFrame(data=play_list,
                                    columns=["wav_filename", "start", "end"])
        play_set.to_csv(csv_file, index=False)
        return
        
    @classmethod
    def audio_count_add(cls):
        AudioTools.audio_play_count+=1

    @classmethod
    def audio_detected_id_set(cls,id):
        AudioTools.audio_id_identified = id

if __name__ == '__main__':
    description = 'Usage: \n' \
                  'python skainet_test_set.py your_yaml_file \n' \
                  
    parser = argparse.ArgumentParser(description=description, formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('yaml')
    args = parser.parse_args()

    with open(args.yaml, 'r') as f:
        config = yaml.safe_load(f)
    print(config)

    #step1: audio SPL normalization
    clean_set = config["clean_set"]
    noise_set = config["noise_set"]
    if clean_set["normalization"]:
        for src_path in clean_set["paths"]:
            AudioTools.audio_normalization(src_path, clean_set["target_dB"])

    if noise_set["normalization"]:
        for src_path in noise_set["paths"]:
            AudioTools.audio_normalization(src_path, clean_set["target_dB"])

    #step2: merge clean audio and noise audio to generate stereo audio
    output_set = config["output_set"]
    if output_set["remove_old_files"]:
        shutil.rmtree(output_set["path"], ignore_errors=True)

    for clean_path in clean_set["paths"]:
        for noise_path in noise_set["paths"]:
            for snr in output_set["snr"]:
                print(clean_path, noise_path, snr)
                clean_gain = snr["clean_gain_dB"]
                noise_gain = snr["clean_gain_dB"] - snr["snr_dB"]
                AudioTools.merge_clean_and_noise(clean_path, noise_path, output_set["path"], clean_gain, noise_gain)

    #step3: play merged audio one by one
    player = config["player"]

    if player["play_output"]:
        AudioTools.audio_play(output_set["path"])
    elif "path" in player:
        AudioTools.audio_play(player["path"])



