#!/usr/bin/python3

# This Python script is used to record test set.

import argparse
import json
import os
import shutil
import yaml
from pathlib import Path

from pydub import AudioSegment

from create_test_set import AudioTools


class MNAudioTools(AudioTools):

    @classmethod
    def merge_wake_word_command_and_noise(
        cls,
        wake_word_dir,
        command_dir,
        noise_dir,
        dest_dir,
        clean_gain,
        noise_gain,
        filelist_path,
    ):
        time_out_audio = AudioSegment.silent(duration=10000, frame_rate=16000)
        clean_audio = AudioSegment.empty()
        noise_audio = AudioSegment.empty()
        if not Path(dest_dir).is_dir():  # It will be created if it does not exist
            Path(dest_dir).mkdir()

        filelist = json.load(open(filelist_path))

        for item in filelist:
            src_audio = cls.read_audio_file(f"{wake_word_dir}/{item['wake_word_fname']}")
            if src_audio is not None:
                rnd_sep_samples = item["wake_word_tailing_silence_ms"] / 16000
                sep_audio = AudioSegment.silent(duration=rnd_sep_samples, frame_rate=16000)
                clean_audio += src_audio + sep_audio
            else:
                print(f"Failed reading {wake_word_dir}/{item['wake_word_fname']}")
                continue
            between_cmd_samples = item["between_command_length_ms"] / 16000
            between_cmd_audio = AudioSegment.silent(duration=between_cmd_samples, frame_rate=16000)
            for i, cmd_item in enumerate(item["commands"]):
                src_audio = cls.read_audio_file(f"{command_dir}/{cmd_item['command_fname']}")
                if src_audio is not None:
                    clean_audio += src_audio
                else:
                    print(f"Failed reading {command_dir}/{cmd_item['command_fname']}")
                if i < len(item["commands"]) - 1:
                    clean_audio += between_cmd_audio
            clean_audio += time_out_audio

        # merge all noise audio file into one file
        for root, _, files in os.walk(noise_dir):
            for filename in files:
                src_file = os.path.join(root, filename)
                src_audio = cls.read_audio_file(src_file)
                if src_audio != None:
                    noise_audio += src_audio

        clean_audio = clean_audio.apply_gain(clean_gain)
        noise_audio = noise_audio.apply_gain(noise_gain)

        # the len() is not exactly, so replace len() with frame_count()
        while noise_audio.frame_count() < clean_audio.frame_count():
            noise_audio = noise_audio + noise_audio

        # Returns the raw audio data as an array of (numeric) samples.
        clean_audio_array = clean_audio.get_array_of_samples()
        noise_audio_array = noise_audio.get_array_of_samples()
        # slice in array, not in AudioSegment
        noise_audio_array = noise_audio_array[:len(clean_audio_array)]
        noise_audio = noise_audio._spawn(noise_audio_array)

        # Combine two mono channels into multichannel
        stereo_sound = AudioSegment.from_mono_audiosegments(clean_audio, noise_audio)
        wake_word_part = Path(wake_word_dir).name
        command_part = Path(command_dir).name
        noise_part = Path(noise_dir).name
        new_file_name = wake_word_part  + '_' + command_part + '_' + str(clean_gain) + 'dB_' + noise_part + '_' + str(noise_gain) + 'dB.wav'
        new_file_path = os.path.join(dest_dir, new_file_name)
        print(f'new_file_path: {new_file_path}')
        stereo_sound.export(new_file_path, format="wav")


if __name__ == '__main__':
    description = 'Usage: \n' \
                  'python create_mn_test_set.py your_yaml_file \n' \

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
        for src_path in clean_set["wake_words_paths"]:
            MNAudioTools.audio_normalization(src_path, clean_set["target_dB"])
        for src_path in clean_set["commands_paths"]:
            MNAudioTools.audio_normalization(src_path, clean_set["target_dB"])

    if noise_set["normalization"]:
        for src_path in noise_set["paths"]:
            MNAudioTools.audio_normalization(src_path, clean_set["target_dB"])

    #step2: merge clean audio and noise audio to generate stereo audio
    output_set = config["output_set"]
    if output_set["remove_old_files"]:
        shutil.rmtree(output_set["path"], ignore_errors=True)

    for wake_words_path, commands_path, filelist_path in zip(
        clean_set["wake_words_paths"], clean_set["commands_paths"], clean_set["filelists_paths"]
    ):
        for noise_path in noise_set["paths"]:
            for snr in output_set["snr"]:
                print(wake_words_path, commands_path, filelist_path, noise_path, snr)
                clean_gain = snr["clean_gain_dB"]
                noise_gain = snr["clean_gain_dB"] - snr["snr_dB"]
                MNAudioTools.merge_wake_word_command_and_noise(
                    wake_words_path, commands_path, noise_path,
                    output_set["path"], clean_gain, noise_gain,
                    filelist_path)

    #step3: play merged audio one by one
    player = config["player"]

    if player["play_output"]:
        AudioTools.audio_play(output_set["path"])
    elif "path" in player:
        AudioTools.audio_play(player["path"])



