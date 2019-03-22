import argparse
import io
import struct
from pathlib import Path
from ctypes import windll

DEFAULT_LAUNCHER = """\
"""

def make_stringtable(strings):
    buf = io.BytesIO()
    strings = list(strings) + [""] * 16
    strings = strings[:16]
    for s in strings:
        buf.write(struct.pack('H', len(s)))
        buf.write(s.encode('utf-16le'))
    return buf.getvalue()

def add_string_resources(filename, *string_list):
    kernel32 = windll.kernel32
    RT_STRING = 6
    LANG = 1 << 10 # MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)

    # Add error handling!
    handle = kernel32.BeginUpdateResourceW(filename, False)
    strings = make_stringtable(string_list)
    kernel32.UpdateResourceW(handle, RT_STRING, 1, LANG, strings, len(strings))
    kernel32.EndUpdateResourceW(handle, False)

def create_stub(source, output, python=None, launcher=None):
    if launcher:
        launcher = Path(launcher).read_bytes()
    else:
        launcher = DEFAULT_LAUNCHER

    with open(output, "wb") as out:
        out.write(launcher)
    if python:
        add_string_resources(output, python)
    with open(output, "ab") as out:
        out.write(Path(source).read_bytes())


def main(args=None):
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('--output', '-o', default=None,
            help="The name of the output executable. "
                 "Required if SOURCE is an archive.")
    # TODO: Using the limited ABI DLL only works for
    #       Python on PATH - the symbol forwarders in
    #       the DLL point to pythonXY.name, and the
    #       pythonXY target is searched for on PATH,
    #       ignoring any DLL alongside python3.dll.
    parser.add_argument('--python', '-p', default=None,
            help="The name of the Python DLL to use "
                 "(default: python3.dll, found on PATH).")
    parser.add_argument('--launcher', '-L', default=None,
            help="The name of a custom launcher "
                 "(default: standard launcher).")
    parser.add_argument('source',
            help="Source archive.")

    args = parser.parse_args(args)

    create_stub(args.source, args.output,
                python=args.python, launcher=args.launcher)

if __name__ == "__main__":
    main()