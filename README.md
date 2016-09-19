A simple Python launcher
========================

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
the "w" form of the script extension is checked for.

The script is found based on the following priority:

1. Appended zip
2. Standalone zipapp (extension ```.pyz[w]```)
3. Standalone script
4. Standalone zipapp (extension ```.zip```)

You can save a number of launchers with their associated scripts
in a directory which also contains a copy of the "embedded" Python
distribution and you will have a standalone distribution of your
scripts.

CAVEATS
-------

* This isn't very well tested yet. There *will* be bugs.
* The launcher chooses the GUI script if the launcher name ends in
  "w", but doesn't check if it's actually the GUI version of the
  launcher. It's up to the user to use the right version, and name
  it appropriately.
* Your script will see the launcher as ```sys.executable```. This is
  correct, but may confuse programs or libraries that assume that
  ```sys.executable``` is the Python interpreter.
