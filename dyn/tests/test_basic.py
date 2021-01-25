import subprocess
import shutil
from pathlib import Path
from zipfile import ZipFile

CODE = """\
print("Hello, world")
"""
STUB = Path(__file__).parent.parent / "stub.exe"

def test_basic(testenv):
    assert (testenv / "python" / "python3.dll").exists()
    p = subprocess.run(
        [str(testenv / "python/python.exe"), "-V"],
        capture_output=True,
        text=True,
    )
    assert p.stdout.strip() == "Python 3.9.1"

def test_pyfile(testenv):
    pyf = testenv / "x.py"
    exe = testenv / "x.exe"
    pyf.write_text(CODE)
    shutil.copy(STUB, exe)
    p = subprocess.run(
        [str(exe), "-V"],
        capture_output=True,
        text=True,
    )
    assert p.stdout.strip() == "Hello, world"

def test_pyzfile(testenv):
    pyf = testenv / "x.pyz"
    exe = testenv / "x.exe"
    with pyf.open("wb") as f:
        with ZipFile(f, "w") as z:
            z.writestr("__main__.py", CODE)
    shutil.copy(STUB, exe)
    p = subprocess.run(
        [str(exe), "-V"],
        capture_output=True,
        text=True,
    )
    assert p.stdout.strip() == "Hello, world"

def test_appended(testenv):
    exe = testenv / "x.exe"
    with exe.open("wb") as f:
        f.write(STUB.read_bytes())
        with ZipFile(f, "w") as z:
            z.writestr("__main__.py", CODE)
    p = subprocess.run(
        [str(exe), "-V"],
        capture_output=True,
        text=True,
    )
    assert p.stdout.strip() == "Hello, world"
