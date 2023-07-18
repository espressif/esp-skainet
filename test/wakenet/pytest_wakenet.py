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

import pytest
import re
import os
import json
from pytest_embedded import Dut

def save_report(results):
    with open(results["report_file"], "w") as f:
        json.dump(results, f)


@pytest.mark.target('esp32s3')
@pytest.mark.env('korvo-2')
@pytest.mark.timeout(60000)
@pytest.mark.config('hiesp')
@pytest.mark.config('hilexin')
def test_wakenet(config, noise, snr, dut: Dut)-> None:

    def match_log(pattern, timeout=18000):
        str = dut.expect(pattern, timeout=timeout).group(1).decode()
        return str

    def match_log_int(pattern, timeout=18000):
        num = match_log(pattern, timeout)
        return int(num)
    
    dut.expect('perf_tester>', timeout=20)
    mode = 'norm'
    dut.write('config {} {} {}'.format(mode, noise, snr))
    dut.expect('mode:{}, noise:{}, snr:{}'.format(mode, noise, snr), timeout=20)
    dut.write('start')
    timeout = 36000
    results = {}
    basedir = os.path.dirname(dut.logfile)
    report_file = os.path.join(basedir, "report.json")
    results["report_file"] = report_file

    #Get the number of test file
    file_num_pattern = re.compile(rb'Number of files: (\d+)')
    file_num = match_log_int(file_num_pattern, 20)
    dut.expect('MC Quantized wakenet9: ', timeout=20)
    results["file_num"] = file_num

    # Get the trigger times and memory siize
    # The following formats are defined in perf_tester.c
    psram_pattern = re.compile(rb'AFE PSRAM: (\d+) KB')
    sram_pattern = re.compile(rb'AFE SRAM: (\d+) KB')
    psram_size = match_log_int(psram_pattern, timeout)
    sram_size = match_log_int(sram_pattern, timeout)
    assert psram_size < 1120   # Assert that the psram size is under 1120 kB
    assert sram_size < 32      # Assert that the internal ram size is under 32 kB
    results["psram"] = psram_size
    results["sram"] = sram_size

    for i in range(file_num):
        file_id = f'File{i}'
        filename_pattern = re.compile(str.encode(f'{file_id}: (\\S+)'))
        trigger_times_pattern = re.compile(str.encode(f'{file_id}, trigger times: (\\d+)'))
        required_times_pattern = re.compile(str.encode(f'{file_id}, required times: (\\d+)'))
        truth_times_pattern = re.compile(str.encode(f'{file_id}, truth times: (\\d+)'))

        filename = match_log(filename_pattern, timeout)
        trigger_times = match_log_int(trigger_times_pattern, timeout)
        required_times = match_log_int(required_times_pattern, timeout)
        truth_times = match_log_int(truth_times_pattern, timeout)

        results[file_id] = {}
        results[file_id]["filename"] = filename
        results[file_id]["trigger_times"] = trigger_times
        results[file_id]["required_times"] = required_times
        results[file_id]["truth_times"] = truth_times
        assert trigger_times >= required_times

    save_report(results)
    dut.expect('TEST DONE', timeout=timeout)
    dut.write('\x03')
