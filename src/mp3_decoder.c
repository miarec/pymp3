#include "mp3_decoder.h"

static PyMethodDef Decoder_methods[] = {
    { "decode", (PyCFunction) &decode, METH_VARARGS, "Decode a block of MP3 data into PCM (little-endian interleaved)." },
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
    "A class that provides access to the LAME MP3 encoder",  /* tp_doc */
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
    DecoderObject* self = (DecoderObject*) type->tp_alloc(type, 0);
    if (self != NULL)
    {
        self->initialised = 0;
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
