'''
Steps to run this case:
- Build
  - . ${IDF_PATH}/export.sh
  - pip install idf_build_apps
  - python tools/ci/build_apps.py examples/chinese_tts -t esp32s3
- Test
  - pip install -r tools/ci/requirements.txt
  - pytest examples/chinese_tts --target esp32s3
'''

import pytest
from pytest_embedded import Dut

# pexpect only accepts ASCII when matching str patterns, so the Chinese prompts
# this example prints have to be matched as UTF-8 bytes.
PROMPT = '请输入短语:'.encode()
PHRASE = '你好乐鑫'


@pytest.mark.target('esp32s3')
@pytest.mark.target('esp32p4')
@pytest.mark.env('korvo-2')
@pytest.mark.env('esp32p4')
@pytest.mark.config('default')
def test_chinese_tts_example(dut: Dut, assert_no_crash) -> None:
    dut.expect(r'voice_data paration size:(\d+)', timeout=30)
    # Printed once the welcome sentence has been synthesized and played back.
    dut.expect_exact(PROMPT, timeout=60)

    dut.write(PHRASE)
    dut.expect_exact('tts input:{}'.format(PHRASE).encode(), timeout=30)
    dut.expect_exact(PROMPT, timeout=60)
    assert_no_crash(10)
