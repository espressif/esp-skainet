'''
Steps to run this case:
- Build
  - . ${IDF_PATH}/export.sh
  - pip install idf_build_apps
  - python tools/ci/build_apps.py examples/deep_noise_suppression -t esp32s3
- Test
  - pip install -r tools/ci/requirements.txt
  - pytest examples/deep_noise_suppression --target esp32s3
'''

import pytest
from pytest_embedded import Dut


@pytest.mark.target('esp32s3')
@pytest.mark.target('esp32p4')
@pytest.mark.env('korvo-2')
@pytest.mark.env('esp32p4')
@pytest.mark.config('default')
def test_deep_noise_suppression_example(dut: Dut, assert_no_crash) -> None:
    dut.expect_exact('------------detect start------------', timeout=60)
    assert_no_crash(15)
