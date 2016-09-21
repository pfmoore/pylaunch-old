A simple Python launcher
========================

The zastub launcher
-------------------

The zastub launcher is purely for prepending to a zipapp style
archive (as created with the Python zipapp stdlib module). To use,
simply make a binary copy of the launcher with the pyz file appended.
In cmd.exe you can do this with

    copy /b zastub.exe+myapp.pyz myapp.exe

The resulting executable will run your application, as long as there
is a version of Python (specifically, python3.dll) on your PATH.

To bundle your application as a standalone package, just download and
unpack the "embedded" Python distribution for your architecture from
python,org, and drop your bundled application in the directory containing
the distribution.

To build the zastub launcher, you need the appropriate version of Visual
C for your Python build (VS 2015 for Python 3.5/3.6) - community edition
is fine. Then simply run build_zastub.py to build.

There are 2 executables generated, zastub.exe for console applications
(pyz archives) and zastubw.exe for GUI applications (pyzw archives).

The stub launcher
-----------------

This is a very simple launcher for Python scripts.

The launcher will locate a Python program, and run that passing
on the arguments used to invoke the launcher.

Types of script handled are:

1. Appended to the excutable as a ```zipapp``` style zipfile.
2. A ```zipapp``` style zipfile (extension ```.pyz```,
   ```.pyzw```, or ```.zip```).
3. A simple Python script (extension ```.py``` or ```.pyw```).

The script name must be the same as the basename of the launcher.
If the launcher name ends in "w", the "w" is removed before locating
the script (the "w" indicates the GUI version of the launcher) and
the "w" form of the script extension is checked for. Note that whether
to use the "w" version is based purely on the launcher name, no check
is made that the "w" launcher is actually a GUI executable. So if you
want a GUI command that doesn't end in "w" you can simply rename the
launcher and not use the "w" form of script name.

The script is found based on the following priority:

1. Appended zip
2. Standalone zipapp (extension ```.pyz[w]```)
3. Standalone script (extension ```.py[w]```)
4. Standalone zipapp (extension ```.zip```)

You can save a number of launchers with their associated scripts
in a directory which also contains a copy of the "embedded" Python
distribution and you will have a standalone distribution of your
scripts.

CAVEATS
-------

* This isn't very well tested yet. There *will* be bugs.
* Your script will see the launcher as ```sys.executable```. This is
  correct, but may confuse programs or libraries that assume that
  ```sys.executable``` is the Python interpreter.
* As a consequence of the previous point, if your code uses the
  ```multiprocessing``` module, you need to use
  ```multiprocessing.set_executable``` to provide the location of
  a suitable Python executable. See 
  https://docs.python.org/3.6/library/multiprocessing.html#multiprocessing.set_executable
  for details.
