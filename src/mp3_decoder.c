#include "mp3_decoder.h"
#include "py_module.h"

#define ERROR_MSG_SIZE 512
#define MAX_READ_BYTES 256*1024*1024    // 256MB maximum supported size of one read operation


static PyMethodDef Decoder_methods[] = {
    { "read", (PyCFunction) &Decoder_read, METH_VARARGS, "Read a decoded audio from the file object" },
    { "get_channels", (PyCFunction) &Decoder_getChannels, METH_NOARGS, "Get the number of channels" },
    { "is_valid", (PyCFunction) &Decoder_isValid, METH_NOARGS, "Report if MP3 file is valid, i.e. at least one MPEG frame was decoded successfully" },
    { "get_mode", (PyCFunction) &Decoder_getMode, METH_NOARGS, "Get MPEG mode (MODE_STEREO, MODE_DUAL_CHANNEL, MODE_JOINT_STEREO, MODE_SINGLE_CHANNEL)" },
    { "get_layer", (PyCFunction) &Decoder_getLayer, METH_NOARGS, "Get MPEG Layer, 1 for Layer I, 2 for Layer II, 3 for Layer II" },
    { "get_bit_rate", (PyCFunction) &Decoder_getBitRate, METH_NOARGS, "Get bitrate (in kbps)" },
    { "get_sample_rate", (PyCFunction) &Decoder_getSampleRate, METH_NOARGS, "Set the audio sample rate" },
    { NULL, NULL, 0, NULL }
};

/** The Decoder class type */
PyTypeObject DecoderType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "mp3.Decoder",                 /* tp_name */
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
    "MP3 decoder",                 /* tp_doc */
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
    PyObject *fobject = NULL;
    PyObject *fread = NULL;

    if (!PyArg_ParseTuple(args, "O:Decoder", &fobject)) {
        PyErr_SetString(PyExc_ValueError, "File-like object must be provided in a constructor of Decoder");
        return NULL;
    }

    // Make sure the file-like object has callable `read` attribute
    fread = PyObject_GetAttrString(fobject, "read");
    if (fread == NULL)
    {
        PyErr_SetString(PyExc_TypeError, "File-like object must have a read method");
        return NULL;
    }


    int isCallable = PyCallable_Check(fread);
    Py_DECREF(fread);
    if (!isCallable) {
        PyErr_SetString(PyExc_TypeError, "read attribute of file-like object must be callable");
        return NULL;
    }

    DecoderObject* self = (DecoderObject*) type->tp_alloc(type, 0);
    if (self != NULL)
    {
        Py_INCREF(fobject);
        self->fobject = fobject;

        /* initialise the mad structs */
        mad_stream_init(&self->stream);
        mad_frame_init(&self->frame);
        mad_synth_init(&self->synth);

        /* One frame of MPEG Layer III is always 1152 samples, where each sample is 16-bit (2 bytes) for mono and x2 for stereo */
        self->output_buffer_size = 1152*2*2;
        self->output_buffer = malloc(self->output_buffer_size);
        self->output_buffer_begin = 0;
        self->output_buffer_end = 0;

        /* Input buffer for compressed frames. 2048 should be enough to keep one frame in the highest possible bit rate */
        self->input_buffer_size = 2048;
        self->input_buffer = malloc(self->input_buffer_size);

        self->is_valid = 0;
        self->frame_count = 0;

        /* explicitly call read() to read the first frame with MPEG info (channels, samplerate, etc.) */
        PyObject * arglist = Py_BuildValue("(i)", 0);   

        // Note, Py_BuildValue() may fail due to MemoryError. In this case, we don't call read() method
        if (arglist != NULL)
        {
            PyObject * read_res = Decoder_read(self, arglist);
            Py_DECREF(arglist);
            if (read_res == NULL)
                PyErr_Clear();  // read() can set error when file is not MP3 encoded
            else
                Py_DECREF(read_res);  // release memory
        }
    }

    return (PyObject*) self;
}

/**
 * Destroy the Decoder class
 */
static void Decoder_dealloc(DecoderObject* self)
{
    mad_synth_finish(&self->synth);
    mad_frame_finish(&self->frame);
    mad_stream_finish(&self->stream);

    free(self->output_buffer);
    self->output_buffer = NULL;

    free(self->input_buffer);
    self->input_buffer = NULL;

    Py_DECREF(self->fobject);
    self->fobject = NULL;

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

/* convert the MAD fixed point format to a signed 16 bit int */
static int16_t madfixed_to_int16(mad_fixed_t sample) {
    /* A fixed point number is formed of the following bit pattern:
    *
    * SWWWFFFFFFFFFFFFFFFFFFFFFFFFFFFF
    * MSB                          LSB
    * S = sign
    * W = whole part bits
    * F = fractional part bits
    *
    * This pattern contains MAD_F_FRACBITS fractional bits, one should
    * always use this macro when working on the bits of a fixed point
    * number.  It is not guaranteed to be constant over the different
    * platforms supported by libmad.
    *
    * The int16_t value is formed by the least significant
    * whole part bit, followed by the 15 most significant fractional
    * part bits.
    *
    * This algorithm was taken from input/mad/mad_engine.c in alsaplayer,
    * which scales and rounds samples to 16 bits, unlike the version in
    * madlld.
    */

    /* round */
    sample += (1L << (MAD_F_FRACBITS - 16));

    /* clip */
    if (sample >= MAD_F_ONE)
        sample = MAD_F_ONE - 1;
    else if (sample < -MAD_F_ONE)
        sample = -MAD_F_ONE;

    /* quantize */
    return sample >> (MAD_F_FRACBITS + 1 - 16);
}


/* 
Concatenate buffer to a bytestring object.
As bytes object is immutable, it will recreate new object.
*/
static int concat_bytes(PyObject **bytes, char const* buffer, Py_ssize_t len)
{
    PyObject * newpart = PyByteArray_FromStringAndSize((const char *)buffer, len);
    PyBytes_ConcatAndDel(bytes, newpart);
    if (bytes == NULL)
        return 0;   // memory error, could not allocate a new bytestring
    else
        return 1;
}

/**
 * Read the next block of audio (decoded on flight)
 */
static PyObject* Decoder_read(DecoderObject* self, PyObject* args)
{
    // An empty bytestring as a result from this function
    PyObject * result_bytes = Py_BuildValue("y", "");

    int remaining_read_size = -1;    // 256MB maximum supported size of one read operation

    int unrecoverable_error = 0;
    char errmsg[ERROR_MSG_SIZE];

    if(!PyArg_ParseTuple(args, "|i", &remaining_read_size))
    {
        PyErr_SetString(PyExc_ValueError, "A size argument is required to read() method");
        return NULL;
    }
    
    if (remaining_read_size == -1 || remaining_read_size > MAX_READ_BYTES)
    {
        remaining_read_size = MAX_READ_BYTES;
    }
    else if (remaining_read_size < 0)
    {
        PyErr_SetString(PyExc_ValueError, "A size argument cannot be negative");
        return NULL;
    }

    /* User may call read(0) to read the first frame and initialize MPEG info (channels, samplerate, etc.) */
    while(remaining_read_size > 0 || self->frame_count == 0)
    {
        /* If we have already available uncompressed data, copy them into the buffer */
        int available = remaining_read_size > 0 ? self->output_buffer_end - self->output_buffer_begin : 0;
        if(available > 0)
        {
            int size = (available > remaining_read_size) ? remaining_read_size : available;
            if(!concat_bytes(&result_bytes, self->output_buffer + self->output_buffer_begin, size))
                return NULL;

            self->output_buffer_begin += size;
            if (self->output_buffer_begin == self->output_buffer_end)
            {
                /* If we are here, then output_buffer is empty, so, reset the begin/end pointers. */
                self->output_buffer_begin = 0;
                self->output_buffer_end = 0;
            }

            remaining_read_size -= size;
            if(remaining_read_size <= 0)
            {
                break;   /* we read all the requested bytes, return the read data */
            }
        }
        else
        {
            if (unrecoverable_error) {
                break;
            }
        }

        Py_ssize_t readsize, remaining;
        unsigned char *readstart;
        PyObject *o_read;
        char *o_buffer;

        /* {2} libmad may not consume all bytes of the input
        * buffer. If the last frame in the buffer is not wholly
        * contained by it, then that frame's start is pointed by
        * the next_frame member of the Stream structure. This
        * common situation occurs when mad_frame_decode() fails,
        * sets the stream error code to MAD_ERROR_BUFLEN, and
        * sets the next_frame pointer to a non NULL value. (See
        * also the comment marked {4} bellow.)
        *
        * When this occurs, the remaining unused bytes must be
        * put back at the beginning of the buffer and taken in
        * account before refilling the buffer. This means that
        * the input buffer must be large enough to hold a whole
        * frame at the highest observable bit-rate (currently 448
        * kb/s). XXX=XXX Is 2016 bytes the size of the largest
        * frame? (448000*(1152/32000))/8
        */
        if (self->stream.next_frame != NULL)
        {
            remaining = self->stream.bufend - self->stream.next_frame;
            if (remaining >= self->input_buffer_size)
            {
                // Something is wrong (too much remaining data). Ignoring it
                readstart = self->input_buffer;
                readsize = self->input_buffer_size;
                remaining = 0;
            }
            else {
                memmove(self->input_buffer, self->stream.next_frame, remaining);
                readstart = self->input_buffer + remaining;
                readsize = self->input_buffer_size - remaining;
            }
        }
        else
        {
            readstart = self->input_buffer;
            readsize = self->input_buffer_size;
            remaining = 0;
        }

        // Call read() method on a file-like object
        o_read = PyObject_CallMethod(self->fobject, "read", "i", readsize);
        if (o_read == NULL) {

# if 0 // Unfortunately, _PyErr_ChainExceptions() is not supported in PyPy interpreter
            // Chain the previous exception to a new exception RuntimeError
            PyObject *exc, *val, *tb;
            PyErr_Fetch(&exc, &val, &tb);
            PyErr_SetString(PyExc_RuntimeError, "Failure in calling read() method of the file-like object");
            _PyErr_ChainExceptions(exc, val, tb);
# else
            PyErr_SetString(PyExc_RuntimeError, "Failure in calling read() method of the file-like object");
# endif

            Py_DECREF(result_bytes);
            return NULL;
        }

        PyBytes_AsStringAndSize(o_read, &o_buffer, &readsize);
        if(PyErr_Occurred())
        {

# if 0 // Unfortunately, _PyErr_ChainExceptions() is not supported in PyPy interpreter
            // Chain the previous exception to a new exception RuntimeError
            PyObject *exc, *val, *tb;
            PyErr_Fetch(&exc, &val, &tb);
            PyErr_SetString(PyExc_RuntimeError, "Failure in reading bytes from file-like object (Is it opened in binary mode?)");
            _PyErr_ChainExceptions(exc, val, tb);
# else
            PyErr_SetString(PyExc_RuntimeError, "Failure in reading bytes from file-like object (Is it opened in binary mode?)");
# endif

            Py_DECREF(o_read);
            Py_DECREF(result_bytes);
            return NULL;
        }

        /* EOF is reached. Return whatever is read */
        if (readsize == 0) {
            Py_DECREF(o_read);
            break;
        }

        memcpy(readstart, o_buffer, readsize);
        Py_DECREF(o_read);

        /* Pipe the new buffer content to libmad's stream decode facility */
        Py_BEGIN_ALLOW_THREADS;
        mad_stream_buffer(&self->stream, self->input_buffer, readsize + remaining);
        self->stream.error = MAD_ERROR_NONE;
        Py_END_ALLOW_THREADS;

        while(1)
        {
            /* Decode the next MPEG frame. The streams is read from the
            * buffer, its constituents are break down and stored the the
            * Frame structure, ready for examination/alteration or PCM
            * synthesis. Decoding options are carried in the Frame
            * structure from the Stream structure.
            *
            * Error handling: mad_frame_decode() returns a non zero value
            * when an error occurs. The error condition can be checked in
            * the error member of the Stream structure. A mad error is
            * recoverable or fatal, the error status is checked with the
            * MAD_RECOVERABLE macro.
            *
            * {4} When a fatal error is encountered all decoding
            * activities shall be stopped, except when a MAD_ERROR_BUFLEN
            * is signaled. This condition means that the
            * mad_frame_decode() function needs more input to complete
            * its work. One shall refill the buffer and repeat the
            * mad_frame_decode() call. Some bytes may be left unused at
            * the end of the buffer if those bytes forms an incomplete
            * frame. Before refilling, the remaining bytes must be moved
            * to the beginning of the buffer and used for input for the
            * next mad_frame_decode() invocation. (See the comments
            * marked {2} earlier for more details.)
            *
            * Recoverable errors are caused by malformed bit-streams, in
            * this case one can call again mad_frame_decode() in order to
            * skip the faulty part and re-sync to the next frame.
            */

            int result;

            Py_BEGIN_ALLOW_THREADS;
            result = mad_frame_decode(&self->frame, &self->stream);
            Py_END_ALLOW_THREADS;

            if (result)
            {
                if (MAD_RECOVERABLE(self->stream.error))
                {
                    // recoverable frame level error (malformed bit-streams), read the next frame
                    continue;
                }
                else
                {
                    if (self->stream.error == MAD_ERROR_BUFLEN)
                    {
                        /* There is no enough binary data to decode. Decode it the next time. See {2}. */
                    }
                    else
                    {
                        snprintf(errmsg, ERROR_MSG_SIZE, "Unrecoverable mpeg frame level error: %s", mad_stream_errorstr(&self->stream));
                        unrecoverable_error = 1;
                    }
                    break;
                }
            }

            if (self->frame_count++ == 0)
            {
                // Read the stream format from the first frame
                self->is_valid = 1;
                self->channels = MAD_NCHANNELS(&self->frame.header);
                self->bitrate = self->frame.header.bitrate/1000;
                self->samplerate = self->frame.header.samplerate;
                self->mode = self->frame.header.mode;
                self->layer = self->frame.header.layer;
            }


            /* Once decoded, the frame can be synthesized to PCM samples. 
            * No errors are reported by mad_synth_frame(); */
            Py_BEGIN_ALLOW_THREADS;
            mad_synth_frame(&self->synth, &self->frame);
            Py_END_ALLOW_THREADS;

            /* Synthesized samples must be converted from libmad's fixed
            * point number to the consumer format. Here we use unsigned
            * 16 bit big endian integers on two channels. Integer samples
            * are temporarily stored in a buffer that is flushed when
            * full.
            */

            struct mad_pcm *pcm = &self->synth.pcm;

            /* Get this frame's info.
               Note, it is possible that this frame's info (like a number of channels) 
               is different form the very first frame.
            */
            unsigned int frame_nchannels = pcm->channels;
            unsigned int frame_nsamples  = pcm->length;
            mad_fixed_t const * left_ch   = pcm->samples[0];
            mad_fixed_t const * right_ch  = pcm->samples[1];

            int size = frame_nsamples * self->channels * sizeof(short);
            if (self->output_buffer_end + size > self->output_buffer_size)
            {
                /* increase buffer size, if necessary */
                self->output_buffer_size = self->output_buffer_end + size;
                unsigned char * new_buffer = realloc(self->output_buffer, self->output_buffer_size);
                if (new_buffer == NULL)
                {
                    PyErr_SetString(PyExc_MemoryError, "Could not allocate memory for output buffer");
                    Py_DECREF(result_bytes);
                    return NULL;
                }
                self->output_buffer = new_buffer;
            }

            int16_t	* output_ptr = (int16_t *)(self->output_buffer + self->output_buffer_end);
            self->output_buffer_end += size;

            //--------------- Convert mad_fixed_t samples to PCM ---------------------
            int16_t sample;
            while (frame_nsamples--) {
                sample = madfixed_to_int16(*left_ch++);
                *(output_ptr++) = sample;

                /* Each MP3 frame can be encoded with differnet mode (STEREO vs MONO).
                *  If we encounter a change in a number of channels, we stick to first frame's mode.
                */
                if(self->channels == 2)
                {
                    if (frame_nchannels == 2)
                        sample = madfixed_to_int16(*right_ch++);

                    *(output_ptr++) = sample;
                }
            }


        }; //while(1)

    } // while (requested_read_len >= 0)

    if(unrecoverable_error)
    {
        Py_DECREF(result_bytes);
        PyErr_SetString(PyExc_RuntimeError, errmsg);
        return NULL;
    }
    else
    {
        return result_bytes;
    }
}


static PyObject* Decoder_getChannels(DecoderObject* self, PyObject* args)
{
    return PyLong_FromLong(self->channels);
}

static PyObject* Decoder_getBitRate(DecoderObject* self, PyObject* args)
{
    return PyLong_FromLong(self->bitrate);
}

static PyObject* Decoder_getSampleRate(DecoderObject* self, PyObject* args)
{
    return PyLong_FromLong(self->samplerate);
}

static PyObject* Decoder_isValid(DecoderObject* self, PyObject* args)
{
    return PyBool_FromLong(self->is_valid);
}

static PyObject* Decoder_getMode(DecoderObject* self, PyObject* args)
{
    switch(self->mode) {
        case MAD_MODE_SINGLE_CHANNEL: return PyLong_FromLong(MODE_SINGLE_CHANNEL);
        case MAD_MODE_STEREO: return PyLong_FromLong(MODE_STEREO);
        case MAD_MODE_JOINT_STEREO: return PyLong_FromLong(MODE_JOINT_STEREO);
        case MAD_MODE_DUAL_CHANNEL: return PyLong_FromLong(MODE_DUAL_CHANNEL);
        default:
            PyErr_SetString(PyExc_RuntimeError, "Invalid MPEG mode");
            return NULL;
    }
}

static PyObject* Decoder_getLayer(DecoderObject* self, PyObject* args) 
{
    switch(self->layer) {
        case MAD_LAYER_I: return PyLong_FromLong(LAYER_I);
        case MAD_LAYER_II: return PyLong_FromLong(LAYER_II);
        case MAD_LAYER_III: return PyLong_FromLong(LAYER_III);
        default:
            PyErr_SetString(PyExc_RuntimeError, "Invalid MPEG layer");
            return NULL;
    }
}
