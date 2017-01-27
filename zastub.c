/* Executable wrapper for a Python application */

#define Py_LIMITED_API 1
#include "Python.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

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
#ifdef PY360_WORKAROUND
    Py_SetProgramName(__wargv[0]);  /* optional but recommended */
    Py_Initialize();
    PySys_SetArgvEx(__argc, __wargv, 0);
    PyObject *runpy = PyImport_ImportModule("runpy");
    if (runpy == 0) {
        PyErr_Print();
        return 1;
    }
    PyObject *run_path = PyObject_GetAttrString(runpy, "run_path");
    if (run_path == 0) {
        PyErr_Print();
        return 1;
    }
    PyObject *ret = PyObject_CallFunction(run_path, "uss", __wargv[0], 0, "__main__");
    if (ret == 0) {
        PyErr_Print();
        return 1;
    }
    if (Py_FinalizeEx() < 0) {
        exit(120);
    }
    return 0;
#else
    wchar_t **myargv = _alloca((__argc + 2) * sizeof(wchar_t*));
    myargv[0] = __wargv[0];
    memcpy(myargv + 1, __wargv, (__argc + 1) * sizeof(wchar_t *));
    return Py_Main(__argc+1, myargv);
#endif
}
