#!/usr/bin/python3

# This Python script is an example for creating the JSON file required by `create_mn_test_set.py`

import argparse
import json
import random
from pathlib import Path

import torchaudio


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--wake-word-dir",
        type=str,
        help="directory of wake word recordings"
    )
    parser.add_argument(
        "--command-dir",
        type=str,
        help="directory of command recordings"
    )
    parser.add_argument(
        "--out-file",
        type=str,
        help="output filename"
    )
    args = parser.parse_args()

    wakewords = list(Path(args.wake_word_dir).glob("*.wav"))
    commands = list(Path(args.command_dir).glob("*.wav"))

    output = []

    for wakeword in wakewords:
        data, sr = torchaudio.load(wakeword)
        assert sr == 16000, sr
        block = {
            "wake_word_fname": wakeword.name,
            "size": data.shape[1],
            "wake_word_tailing_silence_ms": 100,
            "commands": [],
            "between_command_length_ms": 5000,
        }
        num_command = random.randint(1, 5)
        block_commands = random.sample(commands, num_command)
        for command in block_commands:
            data, sr = torchaudio.load(wakeword)
            assert sr == 16000, sr
            block["commands"].append(
                {
                    "command_fname": command.name,
                    "size": data.shape[1]
                }
            )
        output.append(block)

    with open(args.out_file, "wt") as f:
        json.dump(output, f)