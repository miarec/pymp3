#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <lame/lame.h>

typedef struct {
    PyObject_HEAD
    /* The LAME encoder */
    lame_global_flags* lame;
    /* Whether the encoder has been initialised */
    int initialised;
} EncoderObject;

/* Instantiates the new Encoder class memory */
static PyObject* Encoder_new(PyTypeObject *type, PyObject *args, PyObject *kwds);

/* Destroy the Encoder class */
static void Encoder_dealloc(EncoderObject* self);

/* Initialises a new encoder */
static int Encoder_init(EncoderObject* self, PyObject* args, PyObject* kwds);

/** The methods in the Encoder class */
static PyObject* setChannels(EncoderObject* self, PyObject* args);
static PyObject* setQuality(EncoderObject* self, PyObject* args);
static PyObject* setBitRate(EncoderObject* self, PyObject* args);
static PyObject* setInSampleRate(EncoderObject* self, PyObject* args);
static PyObject* encode(EncoderObject* self, PyObject* args);
static PyObject* flush(EncoderObject* self, PyObject* args);
static PyObject* silence(EncoderObject* self, PyObject* args);
