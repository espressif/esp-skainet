'''
Steps to run this case:
- Build
  - . ${IDF_PATH}/export.sh
  - pip install idf_build_apps
  - python tools/ci/build_apps.py examples/wake_word_detection/wakenet -t esp32s3
- Test
  - pip install -r tools/ci/requirements.txt
  - pytest examples/wake_word_detection/wakenet --target esp32s3
'''

import pytest
from pytest_embedded import Dut


@pytest.mark.target('esp32s3')
@pytest.mark.target('esp32p4')
@pytest.mark.env('korvo-2')
@pytest.mark.env('esp32p4')
@pytest.mark.config('default')
def test_wake_word_wakenet_example(dut: Dut, assert_no_crash) -> None:
    dut.expect(r'wake word: (\w+), size:(\d+)', timeout=30)
    # The example feeds an embedded recording of the wake word into WakeNet,
    # so the detection has to happen without anybody speaking to the board.
    dut.expect_exact('Detected', timeout=60)
    assert_no_crash(10)
