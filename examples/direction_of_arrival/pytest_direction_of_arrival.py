'''
Steps to run this case:
- Build
  - . ${IDF_PATH}/export.sh
  - pip install idf_build_apps
  - python tools/ci/build_apps.py examples/direction_of_arrival -t esp32s3
- Test
  - pip install -r tools/ci/requirements.txt
  - pytest examples/direction_of_arrival --target esp32s3
'''

import pytest
from pytest_embedded import Dut


@pytest.mark.target('esp32s3')
@pytest.mark.target('esp32p4')
@pytest.mark.env('korvo-2')
@pytest.mark.env('esp32p4')
@pytest.mark.config('default')
def test_direction_of_arrival_example(dut: Dut, assert_no_crash) -> None:
    # An angle is estimated for every audio frame coming from the two microphones.
    for _ in range(3):
        dut.expect(r'fdoa: (-?\d+\.\d+)', timeout=60)
    assert_no_crash(15)
