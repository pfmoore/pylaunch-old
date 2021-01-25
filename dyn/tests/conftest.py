from pathlib import Path
import pytest
import shutil

pyembed = next(Path(__file__).parent.glob("data/*embed*.zip"))

@pytest.fixture
def testenv(tmp_path):
    py = tmp_path / "python"
    py.mkdir()
    shutil.unpack_archive(pyembed, py)
    return tmp_path
