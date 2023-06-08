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
from pytest_embedded import Dut

@pytest.mark.target('esp32s3')
@pytest.mark.parametrize(
    'config',
    [
        # 'hilexin',
        'hiesp',
    ],
)
def test_wakenet(dut: Dut)-> None:
    dut.expect('MC Quantized wakenet9', timeout=20)