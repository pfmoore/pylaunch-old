import subprocess

def test_basic(testenv):
    assert (testenv / "python" / "python3.dll").exists()
    p = subprocess.run([str(testenv / "python/python.exe"), "-V"],
            capture_output=True, text=True)
    assert p.stdout.strip() == "Python 3.9.1"
