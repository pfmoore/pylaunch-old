import os
import sys
from pathlib import Path
from zipfile import is_zipfile, ZipFile, ZipInfo
from tempfile import TemporaryDirectory
from subprocess import run

def edit(filename):
    if not is_zipfile(filename):
        print("{} is not a zip file".format(filename))
        raise SystemExit

    with open(filename, 'rb') as f:
        with ZipFile(f) as zf:
            zipinfo = zf.infolist()
            zipcontent = [zf.read(i) for i in zipinfo]
            prefix_len = min(i.header_offset for i in zipinfo)
            main_index = None
            for i, info in enumerate(zipinfo):
                if info.filename == '__main__.py':
                    print("main_index is", i)
                    main_index = i
                    break

        f.seek(0)
        prefix = f.read(prefix_len)

    with TemporaryDirectory() as d:
        main = Path(d) / '__main__.py'
        maincontent = zipcontent[main_index] if main_index is not None else b''
        main.write_bytes(maincontent)
        proc = run(['gvim', str(main)])
        newmain = main.read_bytes()
        if newmain != maincontent:
            print("Changed!")

            if main_index is not None:
                zipcontent[main_index] = newmain
            else:
                zipcontent.append(newmain)
                if hasattr(ZipInfo, 'from_file'):
                    zipinfo.append(ZipInfo.from_file(str(main), '__main__.py'))
                else:
                    zipinfo.append('__main__.py')
            with open('new_'+filename, 'wb') as f:
                f.write(prefix)
                with ZipFile(f, 'a') as zf:
                    for i, content in zip(zipinfo, zipcontent):
                        zf.writestr(i, content)


def usage():
    print("Usage: {} edit FILENAME".format(os.path.basename(sys.argv[0])))
    raise SystemExit

if __name__ == '__main__':
    if len(sys.argv) != 3:
        usage()
    if sys.argv[1] == 'edit':
        edit(sys.argv[2])
    else:
        usage()
