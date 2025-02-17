
import contextlib
import wave
from pathlib import Path

import numpy as np
import pandas as pd
import torch

SAMPLING_RATE = 16000

def read_wave(path):
    with contextlib.closing(wave.open(path, 'rb')) as wf:
        num_channels = wf.getnchannels()
        sample_width = wf.getsampwidth()
        assert sample_width == 2
        sample_rate = wf.getframerate()
        assert sample_rate in (8000,16000)
        pcm_data = wf.readframes(wf.getnframes())
        audio_data = np.frombuffer(pcm_data, dtype=np.short)
        audio_data = np.reshape(audio_data, [-1, num_channels])
        return audio_data, sample_rate, num_channels

def Int2Float(sound):
    _sound = np.copy(sound)  #
    abs_max = np.abs(_sound).max()
    _sound = _sound.astype('float32')
    if abs_max > 0:
        _sound *= 1/abs_max
    audio_float32 = torch.from_numpy(_sound.squeeze())
    return audio_float32

def test_vad_offline(csv_path, log_file, args):

    model, utils = torch.hub.load(repo_or_dir='snakers4/silero-vad',
                              model=args.silaro_model_name,
                              force_reload=False,
                              onnx=True)
    (get_speech_ts, save_audio, read_audio, _, _) = utils
    
    items = pd.read_csv(csv_path).values
    test_path = Path(csv_path).absolute().parent / "test_multinet"

    for item in items:
        wavfile = test_path / Path(item[0]).name
        print(wavfile)
        audio_data, _, num_channels = read_wave(str(wavfile))
        print(audio_data.shape, num_channels)
        # audio_data_ch1 = np.flatten(audio_data[:, 0])
        audio_float32 = Int2Float(audio_data[:, 0])
        print(audio_float32)


        # speech_timestamps = get_speech_ts(audio_float32, model, num_steps=args.num_steps,trig_sum=args.trig_sum,neg_trig_sum=args.neg_trig_sum,
        #                     num_samples_per_window=args.num_samples_per_window,min_speech_samples=args.min_speech_samples,
        #                     min_silence_samples=args.min_silence_samples)
        speech_timestamps = get_speech_ts(audio_float32, model, sampling_rate=args.rate, threshold=0.5, min_speech_duration_ms=64, min_silence_duration_ms=128)
        for seg in speech_timestamps:
            print(seg)
        break
VAD_SPEECH = 1
VAD_SILENCE = 0

class VADTrigger:
    def __init__(self, threshold, min_speech_len, min_noise_len):
        self.state = VAD_SILENCE
        self.min_speech_len = min_speech_len
        self.min_noise_len = min_noise_len
        self.speech_len = 0
        self.noise_len = 0
        self.threhold = threshold

    def detect(self, prob):
        if prob > self.threhold:
            state = VAD_SPEECH
        else:
            state = VAD_SILENCE

        if self.state == VAD_SPEECH:
            if state == VAD_SILENCE:
                if self.noise_len < self.min_noise_len:
                    self.noise_len += 1
            else:
                self.noise_len = 0

            if self.noise_len >= self.min_noise_len:
                self.state = VAD_SILENCE
                self.noise_len = 0
                self.speech_len = 0

        else:
            if state == VAD_SPEECH:
                if self.speech_len < self.min_speech_len:
                    self.speech_len += 1
            else:
                self.speech_len = 0

            if self.speech_len >= self.min_speech_len:
                self.state = VAD_SPEECH
                self.noise_len = 0
                self.speech_len = 0

        # print(f"state: {self.state}")
        return self.state
def read_lables(csv_file):
    items = pd.read_csv(csv_file).values
    gt_region_end = []
    gt_region_boundary = []
    for item in items:
        gt_region_end.append(item[1])
        gt_region_boundary.append(item[2])
    
    return gt_region_end, gt_region_boundary

def test_vad_online(csv_path, log_file, args):

    model, utils = torch.hub.load(repo_or_dir='snakers4/silero-vad',
                              model=args.silaro_model_name,
                              force_reload=False,
                              onnx=True)
    (get_speech_ts, save_audio, read_audio, _, _) = utils
    
    items = pd.read_csv(csv_path).values
    test_path = Path(csv_path).absolute().parent / "test_multinet"
    trigger = VADTrigger(0.35, 2, 4)
    file_speech_count = np.zeros(len(items))
    file_noise_count = np.zeros(len(items))
    file_vad_speech_count = np.zeros(len(items))
    file_vad_noise_count = np.zeros(len(items))
    accuracy = []


    for file_id, item in enumerate(items):
        wavfile = test_path / Path(item[0]).name
        label_file = test_path / Path(item[1]).name

        print(wavfile)
        audio_data, _, num_channels = read_wave(str(wavfile))
        gt_region_end, gt_region_boundary = read_lables(str(label_file))

        # audio_data_ch1 = np.flatten(audio_data[:, 0])
        audio_float32 = Int2Float(audio_data[:, 0])
        chunk_num = int(len(audio_float32) / 512)
        gt_idx = 0
        for i in range(chunk_num):
            curr_time_s = i*512 / args.rate

            new_confidence = model(audio_float32[i*512:(i+1)*512], 16000).item()
            # print(new_confidence)
            state = trigger.detect(new_confidence)

            if curr_time_s <= gt_region_end[gt_idx]:
                if gt_idx > 0:
                    file_speech_count[file_id] += 1
                    if state == VAD_SPEECH:
                        file_vad_speech_count[file_id] += 1

            elif curr_time_s < gt_region_boundary[gt_idx]:
                file_noise_count[file_id] += 1

                if state == VAD_SILENCE:
                    file_vad_noise_count[file_id] += 1
            else:
                gt_idx += 1
    
        print(wavfile, "speech count:", file_speech_count[file_id], "noise count:", file_noise_count[file_id], "vad speech count:", file_vad_speech_count[file_id], "vad noise count:", file_vad_noise_count[file_id])
        print("speech accuracy: %f, noise accuracy:%f" % (file_vad_speech_count[file_id] / file_speech_count[file_id], file_vad_noise_count[file_id] / file_noise_count[file_id]))
        accuracy.append(file_vad_speech_count[file_id] / file_speech_count[file_id])
        accuracy.append(file_vad_noise_count[file_id] / file_noise_count[file_id])
    print(accuracy)


if __name__ == '__main__':
    DEFAULT_SAMPLE_RATE = 16000

    import argparse
    parser = argparse.ArgumentParser(description="Stream from microphone to webRTC and silero VAD")
    parser.add_argument('-c', '--csv', type=str, default="silero_vad.csv", help="select the path of the csv file.") 
    parser.add_argument('-log', '--log_file', type=str, default="test.log", help="select the path of the log file.") 
    parser.add_argument('-name', '--silaro_model_name', type=str, default="silero_vad",
                        help="select the name of the model. You can select between 'silero_vad',''silero_vad_micro','silero_vad_micro_8k','silero_vad_mini','silero_vad_mini_8k'")
    parser.add_argument('--reload', action='store_true',help="download the last version of the silero vad")

    parser.add_argument('-ts', '--trig_sum', type=float, default=0.25,
                        help="overlapping windows are used for each audio chunk, trig sum defines average probability among those windows for switching into triggered state (speech state)")

    parser.add_argument('-nts', '--neg_trig_sum', type=float, default=0.07,
                        help="same as trig_sum, but for switching from triggered to non-triggered state (non-speech)")

    parser.add_argument('-N', '--num_steps', type=int, default=8,
                        help="number of overlapping windows to split audio chunk into (we recommend 4 or 8)")

    parser.add_argument('-nspw', '--num_samples_per_window', type=int, default=4000,
                        help="number of samples in each window, our models were trained using 4000 samples (250 ms) per window, so this is preferable value (lesser values reduce quality)")

    parser.add_argument('-msps', '--min_speech_samples', type=int, default=1024,
                        help="minimum speech chunk duration in samples")

    parser.add_argument('-msis', '--min_silence_samples', type=int, default=2048,
                        help=" minimum silence duration in samples between to separate speech chunks")
    args = parser.parse_args()
    args.rate=DEFAULT_SAMPLE_RATE

    test_vad_online(args.csv, args.log_file, args)