/* Launcher for multiple stubs.
 *
 * Runs a function named after the executable, from the module
 * named "stubs" located on sys.path.
 *
 * NOTE: Generally, you can put the "stubs" module next to the
 * launcher, as Python adds sys.exec_prefix to sys.path. This is
 * normal even for the embedded distribution. But in unusual
 * cases, you may need to make specific provision to put the
 * stubs module somewhere Python can find it.
 */
#define Py_LIMITED_API 1
#include <Python.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

const char *code =
"import sys, os\n"
"import stubs\n"
"name = os.path.splitext(os.path.basename(sys.executable))[0]\n"
"ret = getattr(stubs, name)()\n";

#ifdef WINDOWS
int WINAPI wWinMain(
    HINSTANCE hInstance,      /* handle to current instance */
    HINSTANCE hPrevInstance,  /* handle to previous instance */
    LPWSTR lpCmdLine,         /* pointer to command line */
    int nCmdShow              /* show state of window */
)
#else
int wmain()
#endif
{
    PyObject *obj;
    int rv = 0;

    Py_SetProgramName(__wargv[0]);
    Py_Initialize();
    PySys_SetArgvEx(__argc, __wargv, 0);
    obj = Py_CompileString(code, "<internal>", Py_file_input);
    if (obj) {
        PyObject *main = PyImport_AddModule("__main__");
        if (main) {
            PyObject *global = PyModule_GetDict(main);

            if (global) {
                PyObject *result = PyEval_EvalCode(obj, global, global);
                if (result) {
                    /* Borrowed reference */
                    PyObject *ret = PyDict_GetItemString(global, "ret");
                    if (ret == Py_None)
                        rv = 0;
                    else
                        rv = PyLong_AsLong(ret);

                    Py_DECREF(result);
                }
                Py_DECREF(global);
            }
            Py_DECREF(main);
        }
        Py_DECREF(obj);
    }

    if (PyErr_Occurred()) {
        PyErr_Print();
        rv = 1;
    }

    Py_Finalize();
    return rv;
}
