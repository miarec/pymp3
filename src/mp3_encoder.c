#include "mp3_encoder.h"
#include "py_module.h"

static PyMethodDef Encoder_methods[] = {
    { "set_channels", (PyCFunction) &Encoder_setChannels, METH_VARARGS, "Set the number of channels" },
    { "set_quality", (PyCFunction) &Encoder_setQuality, METH_VARARGS, "Set the encoder quality, 2 is highest; 7 is fastest (default is 5)" },
    { "set_bit_rate", (PyCFunction) &Encoder_setBitRate, METH_VARARGS, "Set the constant bit rate (in kbps)" },
    { "set_sample_rate", (PyCFunction) &Encoder_setInSampleRate, METH_VARARGS, "Set the input sample rate" },
    { "set_mode", (PyCFunction) &Encoder_setMode, METH_VARARGS, "Set the MPEG mode (MODE_STEREO, MODE_DUAL_CHANNEL, MODE_JOINT_STEREO, MODE_SINGLE_CHANNEL). Note, DUAL_CHANNEL is not supported by LAME!" },
    { "write", (PyCFunction) &Encoder_write, METH_VARARGS, "Encode a block of PCM data and write to file" },
    { "flush", (PyCFunction) &Encoder_flush, METH_NOARGS, "Flush the last block of MP3 data to file" },
    { NULL, NULL, 0, NULL }
};

/** The Encoder class type */
PyTypeObject EncoderType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "mp3.Encoder",                 /* tp_name */
    sizeof(EncoderObject),         /* tp_basicsize */
    0,                             /* tp_itemsize */
    (destructor) Encoder_dealloc,  /* tp_dealloc */
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
    "MP3 encoder",                 /* tp_doc */
    0,                             /* tp_traverse */
    0,                             /* tp_clear */
    0,                             /* tp_richcompare */
    0,                             /* tp_weaklistoffset */
    0,                             /* tp_iter */
    0,                             /* tp_iternext */
    Encoder_methods,               /* tp_methods */
    0,                             /* tp_members */
    0,                             /* tp_getset */
    0,                             /* tp_base */
    0,                             /* tp_dict */
    0,                             /* tp_descr_get */
    0,                             /* tp_descr_set */
    0,                             /* tp_dictoffset */
    (initproc) Encoder_init,       /* tp_init */
    0,                             /* tp_alloc */
    Encoder_new,                   /* tp_new */
};


static void silentOutput(const char *format, va_list ap)
{
    return;
}


/**
 * Instantiates the new Encoder class memory
 */
static PyObject* Encoder_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *fobject = NULL;
    PyObject *fwrite = NULL;

    if (!PyArg_ParseTuple(args, "O:Encoder", &fobject)) {
        PyErr_SetString(PyExc_ValueError, "File-like object must be provided in a constructor of Encoder");
        return NULL;
    }

    // Make sure the file-like object has callable `write` attribute
    fwrite = PyObject_GetAttrString(fobject, "write");
    if (fwrite == NULL)
    {
        PyErr_SetString(PyExc_TypeError, "File-like object must have a write method");
        return NULL;
    }


    int isCallable = PyCallable_Check(fwrite);
    Py_DECREF(fwrite);
    if (!isCallable) {
        PyErr_SetString(PyExc_TypeError, "write attribute of file-like object must be callable");
        return NULL;
    }

    EncoderObject* self = (EncoderObject*) type->tp_alloc(type, 0);
    if (self != NULL)
    {
        self->lame = lame_init();
        if (self->lame == NULL)
        {
            Py_CLEAR(self);
            PyErr_SetString(PyExc_TypeError, "Could not initialize lame");
            return NULL;
        }

        Py_INCREF(fobject);
        self->fobject = fobject;

        // Default settings
        lame_set_num_channels(self->lame, 2);
        lame_set_in_samplerate(self->lame, 44100);
        lame_set_brate(self->lame, 128);
        lame_set_quality(self->lame, 5);
        // We aren't providing a file interface, so don't output a blank frame
        lame_set_bWriteVbrTag(self->lame, 0);

        // Redirect error/debug output to silent function
        lame_set_errorf(self->lame, &silentOutput);
        lame_set_debugf(self->lame, &silentOutput);
        lame_set_msgf(self->lame, &silentOutput);

        self->initialized = ENCODER_STATE_NON_INITIALIZED;
    }
    return (PyObject*) self;
}

/**
 * Destroy the Encoder class
 */
static void Encoder_dealloc(EncoderObject* self)
{
    Py_DECREF(self->fobject);
    self->fobject = NULL;

    self->initialized = ENCODER_STATE_ERROR;

    lame_close(self->lame);
    Py_TYPE(self)->tp_free((PyObject*) self);
}

/**
 * Initialises a new encoder
 */
static int Encoder_init(EncoderObject* self, PyObject* args, PyObject* kwds)
{
    /* Nothing to do */
    return 0;
}

/**
 * Set the number of channels for the encoder
 */
static PyObject* Encoder_setChannels(EncoderObject* self, PyObject* args)
{
    int channels;

    if (!PyArg_ParseTuple(args, "i", &channels))
    {
        return NULL;
    }

    if (lame_set_num_channels(self->lame, channels) < 0)
    {
        PyErr_SetString(PyExc_RuntimeError, "Unable to set the channels");
        return NULL;
    }

    Py_RETURN_NONE;
}

/**
 * Set the bitrate
 */
static PyObject* Encoder_setBitRate(EncoderObject* self, PyObject* args)
{
    int bitrate;

    if (!PyArg_ParseTuple(args, "i", &bitrate))
    {
        return NULL;
    }

    if (lame_set_brate(self->lame, bitrate) < 0)
    {
        PyErr_SetString(PyExc_RuntimeError, "Unable to set the bit rate");
        return NULL;
    }

    Py_RETURN_NONE;
}

/**
 * Set the sample rate
 */
static PyObject* Encoder_setInSampleRate(EncoderObject* self, PyObject* args)
{
    int insamplerate;

    if (!PyArg_ParseTuple(args, "i", &insamplerate))
    {
        return NULL;
    }

    if (lame_set_in_samplerate(self->lame, insamplerate) < 0)
    {
        PyErr_SetString(PyExc_RuntimeError, "Unable to set the input sample rate");
        return NULL;
    }

    Py_RETURN_NONE;
}

/**
 * Set the number of channels for the encoder
 */
static PyObject* Encoder_setQuality(EncoderObject* self, PyObject* args)
{
    int quality;

    if (!PyArg_ParseTuple(args, "i", &quality))
    {
        return NULL;
    }

    if (lame_set_quality(self->lame, quality) < 0)
    {
        PyErr_SetString(PyExc_RuntimeError, "Unable to set the quality");
        return NULL;
    }

    Py_RETURN_NONE;
}

/**
 * Set the MPEG mode
 */
static PyObject* Encoder_setMode(EncoderObject* self, PyObject* args)
{
    int mode;
    MPEG_mode lame_mode;

    if (!PyArg_ParseTuple(args, "i", &mode))
    {
        return NULL;
    }


    switch(mode) {
        case MODE_SINGLE_CHANNEL: 
            lame_mode = MONO; 
            break;
        case MODE_STEREO: 
            lame_mode = STEREO;
            break;
        case MODE_JOINT_STEREO: 
            lame_mode = JOINT_STEREO;
            break;
        case MODE_DUAL_CHANNEL: 
            PyErr_SetString(PyExc_ValueError, "LAME doesn't supprot dual channel mode");
            return NULL;
        default:
            PyErr_SetString(PyExc_RuntimeError, "Invalid MPEG mode");
            return NULL;
    }

    if (lame_set_mode(self->lame, lame_mode) < 0)
    {
        PyErr_SetString(PyExc_RuntimeError, "Unable to set the MPEG mode");
        return NULL;
    }

    Py_RETURN_NONE;
}


/**
 * Encode a block of PCM data into MP3
 */
static PyObject* Encoder_write(EncoderObject* self, PyObject* args)
{
    short int* inputSamplesArray;
    Py_ssize_t inputSamplesLength;
    Py_ssize_t sampleCount;
    const char * outputBuffer = NULL;
    Py_ssize_t outputBufferSize;
    int channels;

    if (!PyArg_ParseTuple(args, "s#", &inputSamplesArray, &inputSamplesLength))
    {
        return NULL;
    }
    /* inputSamplesArray is a 16-bit PCM integer, but s gives the length in bytes */
    if (inputSamplesLength % 2 != 0)
    {
        PyErr_SetString(PyExc_RuntimeError, "Input data must be 16-bit PCM data");
        return NULL;
    }
    inputSamplesLength /= 2;

    channels = lame_get_num_channels(self->lame);

    /* Initialise the encoder if this is our first call */
    if (self->initialized == ENCODER_STATE_NON_INITIALIZED)
    {
        int ret;

        Py_BEGIN_ALLOW_THREADS
        if (channels == 1 && lame_get_mode(self->lame) != MONO)
        {
            /* Default is JOINT_STEREO which makes no sense for mono */
            lame_set_mode(self->lame, MONO);
        }
        else if (lame_get_mode(self->lame) == MONO)
        {
            lame_set_mode(self->lame, STEREO);
        }
        ret = lame_init_params(self->lame);
        Py_END_ALLOW_THREADS

        if (ret >= 0)
        {
            self->initialized = ENCODER_STATE_INITIALIZED;
        }
        else
        {
            PyErr_SetString(PyExc_RuntimeError, "Error initialising the encoder");
            self->initialized = ENCODER_STATE_ERROR;
            return NULL;
        }
    }

    /* The encoder is in an erroneous state */
    if (self->initialized != ENCODER_STATE_INITIALIZED)
    {
        PyErr_SetString(PyExc_RuntimeError, "Encoder not initialized");
        return NULL;
    }
    
    /* Do the encoding */
    sampleCount = inputSamplesLength / channels;
    if (inputSamplesLength % channels != 0)
    {
        PyErr_SetString(PyExc_RuntimeError, "The input data must be interleaved 16-bit PCM");
        return NULL;
    }

    /* Allocate temporary buffer to encoded data */
    outputBufferSize = sampleCount + (sampleCount / 4) + 7200;
    outputBuffer = malloc(outputBufferSize);
    if (outputBuffer == NULL) {
        PyErr_SetString(PyExc_MemoryError, "Could not allocate memory for output buffer");
        return NULL;
    }

    Py_ssize_t outputBytes;
    Py_BEGIN_ALLOW_THREADS
    if (channels > 1)
    {
        outputBytes = lame_encode_buffer_interleaved(
            self->lame,
            inputSamplesArray, sampleCount,
            outputBuffer, outputBufferSize
        );
    }
    else
    {
        outputBytes = lame_encode_buffer(
            self->lame,
            inputSamplesArray, inputSamplesArray, sampleCount,
            outputBuffer, outputBufferSize
        );
    }
    Py_END_ALLOW_THREADS

    /* Call write() method on the file-like object */
    PyObject * o_write = PyObject_CallMethod(self->fobject, "write", "y#", outputBuffer, outputBytes);
    if (o_write == NULL) {

# if 0 // Unfortunately, _PyErr_ChainExceptions() is not supported in PyPy interpreter
        // Chain the previous exception to a new exception RuntimeError
        PyObject *exc, *val, *tb;
        PyErr_Fetch(&exc, &val, &tb);
        PyErr_Format(PyExc_RuntimeError, "Failure in calling write() method of the file-like object (%d bytes)", outputBytes);
        _PyErr_ChainExceptions(exc, val, tb);
# else
        PyErr_Format(PyExc_RuntimeError, "Failure in calling write() method of the file-like object (%d bytes)", outputBytes);
# endif

        free(outputBuffer);
        return NULL;
    }
    Py_DECREF(o_write);

    free(outputBuffer);  // release memory
    return PyLong_FromLong(inputSamplesLength*2);   // return how many bytes are processed
}


/**
 * Finalise the the MP3 encoder
 */
static PyObject* Encoder_flush(EncoderObject* self, PyObject* args)
{
    if (self->initialized == ENCODER_STATE_INITIALIZED)
    {
        Py_ssize_t outputBufferSize = 8 * 1024;
        const char * outputBuffer = malloc(outputBufferSize);
        if (outputBuffer == NULL) {
            PyErr_SetString(PyExc_MemoryError, "Could not allocate memory for output buffer");
            return NULL;
        }

        Py_ssize_t outputBytes = 0;

        Py_BEGIN_ALLOW_THREADS
        outputBytes = lame_encode_flush(self->lame, outputBuffer, outputBufferSize);
        Py_END_ALLOW_THREADS

        if (outputBytes > 0)
        {
            /* Call write() method on the file-like object */
            PyObject * o_write = PyObject_CallMethod(self->fobject, "write", "y#", outputBuffer, outputBytes);
            if (o_write == NULL) {

# if 0 // Unfortunately, _PyErr_ChainExceptions() is not supported in PyPy interpreter
                // Chain the previous exception to a new exception RuntimeError
                PyObject *exc, *val, *tb;
                PyErr_Fetch(&exc, &val, &tb);
                PyErr_Format(PyExc_RuntimeError, "Failure in calling write() method of the file-like object (%d bytes)", outputBytes);
                _PyErr_ChainExceptions(exc, val, tb);
# else
                PyErr_Format(PyExc_RuntimeError, "Failure in calling write() method of the file-like object (%d bytes)", outputBytes);
# endif

                free(outputBuffer);
                return NULL;
            }
            Py_DECREF(o_write);
        }

        free(outputBuffer);  // release memory
        return PyBool_FromLong(outputBytes);   // return how many bytes were flushed
    }
    else
    {
        PyErr_SetString(PyExc_RuntimeError, "Not currently encoding");
        return NULL;
    }
}
