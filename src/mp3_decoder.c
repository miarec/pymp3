#include "mp3_decoder.h"

/* MAD_BUF_SIZE should be a multiple of 4 >= 4096 and large enough to capture 
   possible junk at beginning of MP3 file 
*/
#define MAD_BUF_SIZE (5 * 8192) 


static PyMethodDef Decoder_methods[] = {
    { "read", (PyCFunction) &read, METH_VARARGS, "Read a decoded audio from the file object." },
    { NULL, NULL, 0, NULL }
};

/** The Decoder class type */
PyTypeObject DecoderType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pymp3.Decoder",               /* tp_name */
    sizeof(DecoderObject),         /* tp_basicsize */
    0,                             /* tp_itemsize */
    (destructor) Decoder_dealloc,  /* tp_dealloc */
    0,                             /* tp_print */
    0,                             /* tp_getattr */
    0,                             /* tp_setattr */
    0,                             /* tp_compare */
    0,                             /* tp_repr */
    0,                             /* tp_as_number */
    0,                             /* tp_as_sequence */
    0,                             /* tp_as_mapping */
    0,                             /* tp_hash */
    0,                             /* tp_call */
    0,                             /* tp_str */
    0,                             /* tp_getattro */
    0,                             /* tp_setattro */
    0,                             /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,            /* tp_flags */
    "A class that provides access to the MAD decoder",  /* tp_doc */
    0,                             /* tp_traverse */
    0,                             /* tp_clear */
    0,                             /* tp_richcompare */
    0,                             /* tp_weaklistoffset */
    0,                             /* tp_iter */
    0,                             /* tp_iternext */
    Decoder_methods,               /* tp_methods */
    0,                             /* tp_members */
    0,                             /* tp_getset */
    0,                             /* tp_base */
    0,                             /* tp_dict */
    0,                             /* tp_descr_get */
    0,                             /* tp_descr_set */
    0,                             /* tp_dictoffset */
    (initproc) Decoder_init,       /* tp_init */
    0,                             /* tp_alloc */
    Decoder_new,                   /* tp_new */
};

/**
 * Instantiates the new Decoder class memory
 */
static PyObject* Decoder_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    unsigned long int bufsize = MAD_BUF_SIZE;

    DecoderObject* self = (DecoderObject*) type->tp_alloc(type, 0);
    if (self != NULL)
    {
        if (!PyArg_ParseTuple(args, "O:Decoder", &self->fobject)) {
            PyErr_SetString(PyExc_ValueError, "File-like object must be provided in a constructor of Decoder");
            return NULL;
        }

        /* make sure that if nothing else we can read it */
        if (!PyObject_HasAttrString(self->fobject, "read")) {
            PyErr_SetString(PyExc_IOError, "Object must have a read method");
            return NULL;
        }
        Py_INCREF(self->fobject);
    }

    return (PyObject*) self;
}

/**
 * Destroy the Decoder class
 */
static void Decoder_dealloc(DecoderObject* self)
{
    //lame_close(self->lame);
    Py_TYPE(self)->tp_free((PyObject*) self);
}

/**
 * Initialises a new decoder
 */
static int Decoder_init(DecoderObject* self, PyObject* args, PyObject* kwds)
{
    /* Nothing to do */
    return 0;
}

/**
 * Decode a block of MP3 data into PCM
 */
static PyObject* decode(DecoderObject* self, PyObject* args)
{
    PyErr_SetString(PyExc_NotImplementedError, "decode() method is not implemented yet");
    return NULL;
}
