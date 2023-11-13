'''
Steps to run these cases:
- Build
  - . ${IDF_PATH}/export.sh
  - pip install idf_build_apps
  - python tools/build_apps.py test/wakenet -t esp32s3
- Test
  - pip install -r tools/ci/requirement.pytest.txt
  - pytest test/wakenet --target esp32s2
'''

import json
import os
import pytest
import re
from pytest_embedded import Dut


def save_report(results):
    with open(results["report_file"], "w") as f:
        json.dump(results, f)


def get_required_stats():
    # store required stats here, it's not ideal but it makes our lives easier

    required_stats = {
        "psram_en": 4076,
        "psram_cn": 3942,
        "sram_en": 21,
        "sram_cn": 20,
    }

    stats = [
        'hiesp_clean_norm_0dB_silence_-5dB,mn6_en,194,510',
        'hiesp_clean_norm_0dB_pink_-10dB,mn6_en,194,418',
        'hiesp_clean_norm_0dB_pink_-5dB,mn6_en,194,301',
        'hiesp_clean_norm_0dB_pub_-10dB,mn6_en,196,495',
        'hiesp_clean_norm_0dB_pub_-5dB,mn6_en,193,417',
        'hilexin_CN-TEST-S_0dB_silence_-5dB,mn6_cn,179,522',
        'hilexin_CN-TEST-S_0dB_pink_-10dB,mn6_cn,178,504',
        'hilexin_CN-TEST-S_0dB_pink_-5dB,mn6_cn,181,482',
        'hilexin_CN-TEST-S_0dB_pub_-10dB,mn6_cn,188,546',
        'hilexin_CN-TEST-S_0dB_pub_-5dB,mn6_cn,184,515',
        'hilexin_CN-TEST-S_0dB_silence_-5dB,mn6_cn_ac,179,528',
        'hilexin_CN-TEST-S_0dB_pink_-10dB,mn6_cn_ac,178,524',
        'hilexin_CN-TEST-S_0dB_pink_-5dB,mn6_cn_ac,181,514',
        'hilexin_CN-TEST-S_0dB_pub_-10dB,mn6_cn_ac,188,560',
        'hilexin_CN-TEST-S_0dB_pub_-5dB,mn6_cn_ac,184,542',
    ]

    stats = [line.split(',') for line in stats]

    for filename, mn_name, required_wn_times, required_mn_times in stats:
        key = f"{filename}-{mn_name}"
        required_stats[key] = {
            'required_wn_times': int(required_wn_times),
            'required_mn_times': int(required_mn_times),
        }
    return required_stats


@pytest.mark.target('esp32s3')
@pytest.mark.env('korvo-2')
@pytest.mark.timeout(360000)
@pytest.mark.config('hiesp_mn6_en')
@pytest.mark.config('hilexin_mn6_cn')
@pytest.mark.config('hilexin_mn6_cn_ac')

def test_multinet6(config, noise, snr, dut: Dut)-> None:

    def match_log(pattern, timeout=18000):
        str = dut.expect(pattern, timeout=timeout).group(1).decode()
        return str

    def match_log_int(pattern, timeout=18000):
        num = match_log(pattern, timeout)
        return int(num)

    def match_log_float(pattern, timeout=18000):
        num = match_log(pattern, timeout)
        return float(num)

    dut.expect('perf_tester>', timeout=20)
    mode = 'norm'
    dut.write('config {} {} {}'.format(mode, noise, snr))
    dut.expect('mode:{}, noise:{}, snr:{}'.format(mode, noise, snr), timeout=20)
    dut.write('start')

    timeout = 108000
    results = {}
    basedir = os.path.dirname(dut.logfile)
    report_file = os.path.join(basedir, "report.json")
    results["report_file"] = report_file

    # Get the number of test file
    file_num_pattern = re.compile(rb'Number of files: (\d+)')
    file_num = match_log_int(file_num_pattern, 20)
    mn_name_pattern = re.compile(rb'Quantized MultiNet6:rnnt_ctc_1.0, name:([^,]*)')
    # Get model name
    mn_name = match_log(mn_name_pattern, timeout=50)
    results["file_num"] = file_num

    # Get the trigger times and memory siize
    # The following formats are defined in perf_tester.c
    psram_pattern = re.compile(rb'MN PSRAM: (\d+) KB')
    sram_pattern = re.compile(rb'MN SRAM: (\d+) KB')
    psram_size = match_log_int(psram_pattern, timeout)
    sram_size = match_log_int(sram_pattern, timeout)

    required_stats = get_required_stats()

    # this size differs due to different number of commands
    if "en" in mn_name:
        assert psram_size <= required_stats["psram_en"]
        assert sram_size <= required_stats["sram_en"]
    else:
        assert psram_size <= required_stats["psram_cn"]
        assert sram_size <= required_stats["sram_cn"]

    results["psram"] = psram_size
    results["sram"] = sram_size

    for i in range(file_num):
        file_id = f'File{i}'

        trigger_times_pattern = re.compile(str.encode(f'{file_id}, trigger times: (\\d+)'))
        truth_times_pattern = re.compile(str.encode(f'{file_id}, truth times: (\\d+)'))
        wn_delay_pattern = re.compile(str.encode(f'{file_id}, wn averaged delay: ([+-]?([0-9]*[.])?[0-9]+)'))

        correct_commands_pattern = re.compile(str.encode(f'{file_id}, correct commands: (\\d+)'))
        incorrect_commands_pattern = re.compile(str.encode(f'{file_id}, incorrect commands: (\\d+)'))
        missed_commands_pattern = re.compile(str.encode(f'{file_id}, missed commands: (\\d+)'))
        truth_commands_pattern = re.compile(str.encode(f'{file_id}, truth commands: (\\d+)'))
        mn_delay_pattern = re.compile(str.encode(f'{file_id}, mn averaged delay: ([+-]?([0-9]*[.])?[0-9]+)'))

        filename_pattern = re.compile(str.encode(f'{file_id}: /sdcard/test_multinet/(.*).wav'))
        filename = match_log(filename_pattern, timeout).strip()
        key = f'{filename}-{mn_name}'
        required_stats = required_stats[key]

        trigger_times = match_log_int(trigger_times_pattern, timeout)
        truth_times = match_log_int(truth_times_pattern, timeout)
        wn_delay = match_log_float(wn_delay_pattern, timeout)

        correct_commands = match_log_int(correct_commands_pattern, timeout)
        incorrect_commands = match_log_int(incorrect_commands_pattern, timeout)
        missed_commands = match_log_int(missed_commands_pattern, timeout)
        truth_commands = match_log_int(truth_commands_pattern, timeout)
        mn_delay = match_log_float(mn_delay_pattern, timeout)

        results[file_id] = {}
        results[file_id]["filename"] = filename
        results[file_id]["trigger_times"] = trigger_times
        results[file_id]["required_times"] = required_stats["required_wn_times"]
        results[file_id]["truth_times"] = truth_times
        results[file_id]["wn_delay"] = wn_delay
        results[file_id]["correct_commands"] = correct_commands
        results[file_id]["incorrect_commands"] = incorrect_commands
        results[file_id]["missed_commands"] = missed_commands
        results[file_id]["truth_commands"] = truth_commands
        results[file_id]["required_correct"] = required_stats["required_mn_times"]
        results[file_id]["mn_delay"] = mn_delay

        assert trigger_times >= required_stats["required_wn_times"]
        assert correct_commands >= required_stats["required_mn_times"]

    save_report(results)
    dut.expect('TEST DONE', timeout=timeout)
    dut.write('\x03')
