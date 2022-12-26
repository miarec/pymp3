#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <mad.h>

typedef struct {
    PyObject_HEAD
    /* File-like object that will be read */
    PyObject *fobject;
    struct mad_stream stream;
    struct mad_frame frame;
    struct mad_synth synth;

    unsigned char *input_buffer;
    unsigned int input_buffer_size;

    unsigned char *output_buffer;
    unsigned int output_buffer_size;
    unsigned int output_buffer_begin;
    unsigned int output_buffer_end;

    int  is_valid;
    long mode;
    long layer;
    long bitrate;
    long channels;
    long samplerate;

    unsigned int frame_count;
} DecoderObject;

/* Instantiates the new decoder class memory */
static PyObject* Decoder_new(PyTypeObject *type, PyObject *args, PyObject *kwds);

/* Destroy the decoder class */
static void Decoder_dealloc(DecoderObject* self);

/* Initialises a new decoder */
static int Decoder_init(DecoderObject* self, PyObject* args, PyObject* kwds);

/** The methods in the decoder class */
static PyObject* Decoder_read(DecoderObject* self, PyObject* args);
static PyObject* Decoder_isValid(DecoderObject* self, PyObject* args);
static PyObject* Decoder_getChannels(DecoderObject* self, PyObject* args);
static PyObject* Decoder_getBitRate(DecoderObject* self, PyObject* args);
static PyObject* Decoder_getSampleRate(DecoderObject* self, PyObject* args);
static PyObject* Decoder_getMode(DecoderObject* self, PyObject* args);
static PyObject* Decoder_getLayer(DecoderObject* self, PyObject* args);