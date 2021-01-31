A simple Python launcher
========================

**Warning** While the approach used in this project is perfectly
functional, it is clumsy, and somewhat difficult to work with
in practice. Also, coding in C annoys the heck out of me, as I
need to write everything myself...

I'm working on a reimplementation of this idea, using Rust as the
implementation language. That project is currently at https://github.com/pfmoore/pylaunch2,
but at some point I intend to move it over here, renaming this project to
pylaunch-legacy. That will disrupt anyone using this version of the code,
but frankly I don't think anyone is, in practice, so I'm going to
take the simplest (for me!) approach.


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

Console vs GUI applications
---------------------------

The launcher executables come in 2 variations, one suffixed with a "w"
and the other without. The "w" variation should be used for GUI
applications, and the "non-w" variation should be used for console
applications. This is exactly the same convention as used by the Python
interpreter itself (```python``` and ```pythonw``` commands).

Putting the embedded Python distribution in a subdirectory
----------------------------------------------------------

The launchers normally locate Python by looking for the interpreter DLL,
```python3.dll```, on the user's PATH. Additionally, as noted above,
Python 3.5+ comes with an "embedded" distribution, which can be unpacked
in the same directory as the application exe, and which will then be used
in preference to the any other Python installation on the machine. This
allows for completely standalone distribution of your application.

It is not always ideal to have the Python distribution in your main
application directory (for example, if you need to add your application
directory to the user's PATH, you may not want to expose the embedded
Python DLLs and exes). By using Windows "side by side assembly" feature,
it is possible to put the embedded distribution in a subdirectory. To
do this, you must take the following steps:

1. Take the supplied ```py3embed``` directory, and the ```py3embed.manifest```
   file contained within it, and copy them to your application directory.
   **You must use the name py3embed, as the manifests are based on name,
   and you cannot use a different name without changing the manifest
   embedded in the exe.**
2. Unpack the Python 3.6 embedded distribution into the py3embed
   directory.
3. Use the version of the launcher with an "m" suffix (```zalaunchm.exe```
   or ```zalaunchmw.exe```).

It is possible to use versions of Python other than 3.6, however you will
need to change the dll names in ```py3embed.manifest```. Also, the Python
3.5 embedded distribution has some limitations when used in this way, and
so is not supported.

The multistub launcher
----------------------

The multistub launcher can be used to host multiple stubs from a single
directory. The launcher will locate a function to run by importing a
"stubs" module and looking for an attribute named the same as the
launcher. The launcher will call that attribute.

Typically, "stubs" will be a Python file in the same directory
as the launcher(s), but it is possible to locate it anywhere on sys.path.

There are 2 launchers, a console one and a GUI one.

In practice, there is little advantage to this form of the launcher, and
it is mainly provided as a proof of concept for alternative approaches.

The genstub launcher
--------------------

The launcher will locate a Python program, and run that passing on the
arguments used to invoke the launcher. It searches for the Python program in a
number of locations. Although in principle this adds flexibility, in practice
this is rarely needed and adds complexity, and so again this variant is
provided simply  as a proof of concept.

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
* Error handling is minimal.
* Your script will see the launcher as ```sys.executable```. This is
  correct, but may confuse programs or libraries that assume that
  ```sys.executable``` is the Python interpreter.
* As a consequence of the previous point, if your code uses the
  ```multiprocessing``` module, you need to use
  ```multiprocessing.set_executable``` to provide the location of
  a suitable Python executable. See 
  https://docs.python.org/3.6/library/multiprocessing.html#multiprocessing.set_executable
  for details.
