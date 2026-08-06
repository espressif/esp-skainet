'''
Steps to run this case:
- Build
  - . ${IDF_PATH}/export.sh
  - pip install idf_build_apps
  - python tools/ci/build_apps.py examples/voice_activity_detection -t esp32s3
- Test
  - pip install -r tools/ci/requirements.txt
  - pytest examples/voice_activity_detection --target esp32s3
'''

import pytest
from pytest_embedded import Dut


@pytest.mark.target('esp32s3')
@pytest.mark.target('esp32p4')
@pytest.mark.env('korvo-2')
@pytest.mark.env('esp32p4')
@pytest.mark.config('default')
def test_voice_activity_detection_example(dut: Dut, assert_no_crash) -> None:
    dut.expect_exact('------------vad start------------', timeout=60)
    # The AFE keeps reporting the VAD state for every audio frame it fetches.
    for _ in range(3):
        dut.expect(r'vad state: (noise|speech)', timeout=10)
    assert_no_crash(15)
