#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "py_module.h"
#include "mp3_encoder.h"
#include "mp3_decoder.h"


/** The name of the module */
static char module_name[] = "mp3";

/** Names of the declared classes */
static char EncoderClassName[] = "Encoder";
static char DecoderClassName[] = "Decoder";

/** The docstring description of the module */
PyDoc_STRVAR(module_docstring, "This module provides an interface to encode/decode between PCM and MP3 data");

/** No methods in the module, only a class */
static PyMethodDef module_methods[] = {
    { NULL, NULL, 0, NULL }
};

/** The module definition */
static struct PyModuleDef pymp3_module = {
    PyModuleDef_HEAD_INIT,
    module_name,
    module_docstring,
    -1,
    module_methods
};

extern PyTypeObject EncoderType;
extern PyTypeObject DecoderType;

/**
 * Module initialisation function
 *
 * \return  The created module
 */
PyMODINIT_FUNC PyInit_mp3(void)
{
    PyObject *module, *dict;

    /* Create the module with no methods in it */
    module = PyModule_Create(&pymp3_module);

    /* Define constants */
    dict = PyModule_GetDict(module);

    PyDict_SetItemString(dict, "LAYER_I", PyLong_FromLong(LAYER_I));
    PyDict_SetItemString(dict, "LAYER_II", PyLong_FromLong(LAYER_II));
    PyDict_SetItemString(dict, "LAYER_III", PyLong_FromLong(LAYER_III));

    PyDict_SetItemString(dict, "MODE_SINGLE_CHANNEL", PyLong_FromLong(MODE_SINGLE_CHANNEL));
    PyDict_SetItemString(dict, "MODE_DUAL_CHANNEL", PyLong_FromLong(MODE_DUAL_CHANNEL));
    PyDict_SetItemString(dict, "MODE_JOINT_STEREO", PyLong_FromLong(MODE_JOINT_STEREO));
    PyDict_SetItemString(dict, "MODE_STEREO", PyLong_FromLong(MODE_STEREO));

    /* Initialise the class */
    if (PyType_Ready(&EncoderType) < 0)
    {
        Py_CLEAR(module);
    }
    else
    {
        /* Add the Encoder class to the module */
        Py_INCREF(&EncoderType);
        if (PyModule_AddObject(module, EncoderClassName, (PyObject*) &EncoderType) == -1)
        {
            Py_CLEAR(module);
        }
    }

    if (PyType_Ready(&DecoderType) < 0)
    {
        Py_CLEAR(module);
    }
    else
    {
        /* Add the Decoder class to the module */
        Py_INCREF(&DecoderType);
        if (PyModule_AddObject(module, DecoderClassName, (PyObject*) &DecoderType) == -1)
        {
            Py_CLEAR(module);
        }
    }

    return module;
}
