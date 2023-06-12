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
@pytest.mark.parametrize(
    'config',
    [
        'hilexin',
        'hiesp',
    ],
)
def test_wakenet(dut: Dut)-> None:
    results = {}
    basedir = os.path.dirname(dut.logfile)
    report_file = os.path.join(basedir, "report.json")
    results["report_file"] = report_file

    #Get the number of test file
    file_num = dut.expect(re.compile(rb'Number of files: (\d+)'), timeout=20).group(1).decode()
    dut.expect('MC Quantized wakenet9: ', timeout=20)
    results["file_num"] = int(file_num)

    # Get the trigger times
    trigger_times = dut.expect(re.compile(rb'Total trigger times: (\d+)'), timeout=18000).group(1).decode()
    results["times"] = trigger_times

    save_report(results)
    dut.expect('TEST DONE', timeout=18000)