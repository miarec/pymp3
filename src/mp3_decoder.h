#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <mad.h>

typedef struct {
    PyObject_HEAD;
    /* PyObject_HEAD */
    /* The MAD decoder */
    /*lame_global_flags* lame;*/
    struct mad_stream stream;
    struct mad_frame frame;
    struct mad_synth synth;
    mad_timer_t timer;
    /* Whether the decoder has been initialised */
    int initialised;
} DecoderObject;

/* Instantiates the new decoder class memory */
static PyObject* Decoder_new(PyTypeObject *type, PyObject *args, PyObject *kwds);

/* Destroy the decoder class */
static void Decoder_dealloc(DecoderObject* self);

/* Initialises a new decoder */
static int Decoder_init(DecoderObject* self, PyObject* args, PyObject* kwds);

/** The methods in the decoder class */
static PyObject* decode(DecoderObject* self, PyObject* args);
