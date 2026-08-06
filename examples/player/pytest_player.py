'''
Steps to run this case:
- Build
  - . ${IDF_PATH}/export.sh
  - pip install idf_build_apps
  - python tools/ci/build_apps.py examples/player -t esp32s3
- Test
  - pip install -r tools/ci/requirements.txt
  - pytest examples/player --target esp32s3
'''

import pytest
from pytest_embedded import Dut


@pytest.mark.target('esp32s3')
@pytest.mark.target('esp32p4')
@pytest.mark.env('korvo-2')
@pytest.mark.env('esp32p4')
@pytest.mark.config('default')
def test_player_example(dut: Dut, assert_no_crash) -> None:
    dut.expect(r'wakenet model in flash: (\S+)', timeout=30)
    dut.expect(r'Player volume: (\d+)', timeout=30)
    dut.expect_exact('Playing WAV files from /sdcard/music/', timeout=30)
    dut.expect_exact('------------LISTENING------------', timeout=60)

    # An empty line makes the console print its prompt again, whatever it printed
    # while the audio tasks were starting up.
    dut.write('')
    dut.expect_exact('player>', timeout=30)
    dut.write('play')
    dut.expect_exact('Play', timeout=30)
    dut.write('pause')
    dut.expect_exact('Pause', timeout=30)
    assert_no_crash(10)
