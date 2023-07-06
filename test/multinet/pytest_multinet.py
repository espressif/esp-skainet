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


@pytest.mark.target('esp32s3')
@pytest.mark.env('korvo-1')
@pytest.mark.timeout(60000)
@pytest.mark.parametrize(
    'config',
    [
        'hiesp_mn6_en',
    ],
)
def test_multinet(dut: Dut)-> None:

    def match_log(pattern, timeout=18000):
        str = dut.expect(pattern, timeout=timeout).group(1).decode()
        return str

    def match_log_int(pattern, timeout=18000):
        num = match_log(pattern, timeout)
        return int(num)

    def match_log_float(pattern, timeout=18000):
        num = match_log(pattern, timeout)
        return float(num)

    timeout = 36000
    results = {}
    basedir = os.path.dirname(dut.logfile)
    report_file = os.path.join(basedir, "report.json")
    results["report_file"] = report_file

    # Get the number of test file
    file_num_pattern = re.compile(rb'Number of files: (\d+)')
    file_num = match_log_int(file_num_pattern, 20)
    dut.expect('Quantized MultiNet6:', timeout=50)
    results["file_num"] = file_num

    # Get the trigger times and memory siize
    # The following formats are defined in perf_tester.c
    psram_pattern = re.compile(rb'MN PSRAM: (\d+) KB')
    sram_pattern = re.compile(rb'MN SRAM: (\d+) KB')
    psram_size = match_log_int(psram_pattern, timeout)
    sram_size = match_log_int(sram_pattern, timeout)
    assert psram_size < 5000   # Assert that the psram size is under 5000 kB
    assert sram_size < 32      # Assert that the internal ram size is under 32 kB
    results["psram"] = psram_size
    results["sram"] = sram_size

    for i in range(file_num):
        file_id = f'File{i}'
        filename_pattern = re.compile(str.encode(f'{file_id}: (\\S+)'))

        trigger_times_pattern = re.compile(str.encode(f'{file_id}, trigger times: (\\d+)'))
        required_times_pattern = re.compile(str.encode(f'{file_id}, required times: (\\d+)'))
        truth_times_pattern = re.compile(str.encode(f'{file_id}, truth times: (\\d+)'))
        wn_delay_pattern = re.compile(str.encode(f'{file_id}, wn averaged delay: ([+-]?([0-9]*[.])?[0-9]+)'))

        correct_commands_pattern = re.compile(str.encode(f'{file_id}, correct commands: (\\d+)'))
        incorrect_commands_pattern = re.compile(str.encode(f'{file_id}, incorrect commands: (\\d+)'))
        missed_commands_pattern = re.compile(str.encode(f'{file_id}, missed commands: (\\d+)'))
        required_correct_pattern = re.compile(str.encode(f'{file_id}, required correct: (\\d+)'))
        truth_commands_pattern = re.compile(str.encode(f'{file_id}, truth commands: (\\d+)'))
        mn_delay_pattern = re.compile(str.encode(f'{file_id}, mn averaged delay: ([+-]?([0-9]*[.])?[0-9]+)'))

        filename = match_log(filename_pattern, timeout)

        trigger_times = match_log_int(trigger_times_pattern, timeout)
        required_times = match_log_int(required_times_pattern, timeout)
        truth_times = match_log_int(truth_times_pattern, timeout)
        wn_delay = match_log_float(wn_delay_pattern, timeout)

        correct_commands = match_log_int(correct_commands_pattern, timeout)
        incorrect_commands = match_log_int(incorrect_commands_pattern, timeout)
        missed_commands = match_log_int(missed_commands_pattern, timeout)
        required_correct = match_log_int(required_correct_pattern)
        truth_commands = match_log_int(truth_commands_pattern, timeout)
        mn_delay = match_log_float(mn_delay_pattern, timeout)

        results[file_id] = {}
        results[file_id]["filename"] = filename
        results[file_id]["trigger_times"] = trigger_times
        results[file_id]["required_times"] = required_times
        results[file_id]["truth_times"] = truth_times
        results[file_id]["wn_delay"] = wn_delay
        results[file_id]["correct_commands"] = correct_commands
        results[file_id]["incorrect_commands"] = incorrect_commands
        results[file_id]["missed_commands"] = missed_commands
        results[file_id]["truth_commands"] = truth_commands
        results[file_id]["required_correct"] = required_correct
        results[file_id]["mn_delay"] = mn_delay

        assert trigger_times >= required_times
        assert correct_commands >= required_correct

    save_report(results)
    dut.expect('TEST DONE', timeout=timeout)
