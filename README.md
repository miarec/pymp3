pymp3 - Read and write MP3 files
===============================================================

# Introduction

The **pymp3** module provides a decoding/encoding interface to the MP3 audio format.
This library uses [libmp3lame](https://lame.sourceforge.io/) (lame) and [libmad](https://www.underbit.com/products/mad/) under the hood.
Binary distribution packages for Windows and *nix systems are available.

# Installation

Using pip

    pip install pymp3

Install from source code:

    git clone <this repo>
    cd pymp3
    pip install .

# Usage

## mp3.Decoder object (MP3-to-PCM convertor)

**mp3.Decoder** object provides an interface to MP3-to-PCM decoding capabilities. 
It uses a well-known [libmad](https://www.underbit.com/products/mad/) under the hood. 

Example of usage (convert *.mp3 file to *.wav):

```python

from mp3
from wave import Wave_write

with open('input.mp3', 'rb') as read_file, open('output.wav', 'wb') as write_file:

    decoder = mp3.Decoder(read_file)

    sample_rate = decoder.get_sample_rate()
    nchannels = decoder.get_channels()

    wav_file = Wave_write(write_file)
    wav_file.setnchannels(nchannels)
    wav_file.setsampwidth(2)
    wav_file.setframerate(sample_rate)

    while True:
        pcm_data = decoder.read(4000)

        if not pcm_data:
            break
        else:
            wav_file.writeframes(pcm_data)

```


## mp3.Encoder object (PCM-to-MP3 convertor)

**mp3.Encoder** object provides an interface to PCM-to-MP3 encoding capabilities.
It uses a well-known [libmp3lame](https://lame.sourceforge.io/) under the hood. 

Example of usage (convert *.wav file to *.mp3):

```python

import mp3
from wave import Wave_read

with open('input.wav', 'rb') as read_file, open('output.mp3', 'wb') as write_file:

    wav_file = Wave_read(read_file)

    sample_size = wav_file.getsampwidth()
    sample_rate = wav_file.getframerate()
    nchannels = wav_file.getnchannels()

    if sample_size != 2:
        raise ValueError("Only PCM 16-bit sample size is supported (input audio: %s)" % sample_size)

    encoder = mp3.Encoder(write_file)
    encoder.set_bit_rate(64)
    encoder.set_sample_rate(frame_rate)
    encoder.set_channels(nchannels)
    encoder.set_quality(5)   # 2-highest, 7-fastest
    encoder.set_mod(mp3.MODE_STEREO if nchannels == 2 else mp3.MODE_SINGLE_CHANNEL)

    while True:
        pcm_data = wav_file.readframes(8000)
        if pcm_data:
            encoder.write(pcm_data)
        else:
            encoder.flush()
            break

```

# Interface

## Constants

MPEG Layer version as returned by `Decoder.get_layer()`:

- `mp3.LAYER_I`
- `mp3.LAYER_II`
- `mp3.LAYER_III`

MPEG mode as returned by `Decoder.get_mode()` or supplied to `Encoder.set_mode()`

- `mp3.MODE_SINGLE_CHANNEL`: a mono (single channel)
- `mp3.MODE_DUAL_CHANNEL`: dual channel mode. Caution! A dual channel mode is not supported by LAME encoder, use a stereo or joint stereo instead.
- `mp3.MODE_STEREO`: a stereo mode (recommended for high bitrates)
- `mp3.MODE_JOINT_STEREO`: a joint stereo mode (recommended for low bitrates)

## mp3.Encoder (PCM-to-MP3 convertor)

Constructor:

- `mp3.Encoder(fp)`: Creates an encoder object. `fp` is a file-like object that has `write()` method to write binary data.

Class methods:

- `set_channels(nchannels: int)`: Set the number of channels (1 for mono, 2 for stereo)
- `set_quality(quality: int)`: Set the encoder quality, 2 is highest; 7 is fastest (default is 5)
- `set_bit_rate(bitrate: int)`: Set the constant bit rate (in kbps)
- `set_sample_rate(sample_rate: int)`: Set the input sample rate in Hz
- `set_mode(mode: int)`: Set the MPEG mode (one of `mp3.MODE_STEREO`,  `mp3.MODE_JOINT_STEREO`, `mp3.MODE_SINGLE_CHANNEL`). Note, a dual channel mode is not supported by LAME!
- `write(data: bytes)`: Encode a block of PCM data (signed 16-bit interleaved) and write to a file.
- `flush()`: Flush the last block of MP3 data to a file.


**Important!**

Before closing the file, call `flush()` method to write the last block of MP3 data to a file.


## mp3.Decoder (MP3-to-PCM convertor)

Constructor:

- `mp3.Decoder(fp)`: Creates a decoder object. `fp` is a file-like object that has `read()` method to read binary data.

Class methods:

- `is_valid() -> bool`: Returns TRUE if at least one valid MPEG frame was found in a file
- `read(nbytes = None: int) -> bytes`: Read mp3 file, decodes into PCM format (16-bit signed interleaved) and returns the requested number of bytes. If `nbytes` is not provided, then up to 256MB will be read from file
- `get_channels() -> int`: Get the number of channels (1 for mono, 2 for stereo)
- `get_bit_rate() -> int`: Get the bit rate (in kbps)
- `get_sample_rate() -> int`: Get the sample rate in Hz
- `get_mode() -> int`: Get the MPEG mode (one of `mp3.MODE_STEREO`,  `mp3.MODE_JOINT_STEREO`, `mp3.MODE_SINGLE_CHANNEL` or `mp3.MODE_DUAL_CHANNEL`)
- `get_layer() -> int`: Get the MPEG layer (one of `mp3.LAYER_I`,  `mp3.Layer_II`, `mp3.Layer_III`)


# Building a binary package

Prerequisites:

- [Recommended] Create python virtual environment with `python -v venv venv` and activate it with `source venv/bin/activate`
- Install the packages required for development with `pip install -r requirements-dev.txt`

To build a binary package for your platform (*.whl), run:

    pip wheel . --verbose

A result of this command will be `mp3*.whl` file in the current directory.

Optional `--verbose` parameter allows you to review the build process.

## To install the built WHL file:

    pip install pymp3*.whl

## To build and install the package in a development mode:

    pip install -e . --verbose

This command will build `*.so` file (or `*.dll` on Windows) instead of *.whl.

Optional `--verbose` parameter allows you to review the build process.

# Unit testing

To run unit tests, use the following command (assuming the `pymp3` module is installed in current python environment):

    pytest tests

# Troubleshooting build failures (C code)

The library is built with CMake, which is automatically called when setuptools is building the package.

You can call CMake directly to see the reported error messages:

    cmake -S . -B build
    cmake --build build

If you have multiple python interpreters available on the system, then add `-DPython3_EXECUTABLE=<path-to-python-exe>` to hint CMake to use
the proper version. Otherwise, CMake will choose a default python interpreter.

This command will build `pymp3.so` (or `pymp3.pyd`) file in the respective build directory
(on Windows, it will be `./build/Release` or `./build/Debug`, on Linux, it will be `./build`).

On Windows, by default, Visual Studio builds a Debug configuration. 
Add `--config=Release` to build command to choose Release configuration:

    cmake --build build --config=Release

By default, this project will download lame and mad libraries from github and compile them in-place.
If you want to use the system-installed lame/mad, then pass the following parameters to cmake command:

    -DPYMP3_USE_SYSTEM_LIBMAD=ON -DPYMP3_USE_SYSTEM_LAME

TODO:

  - Allow user to define PYMP3_USE_SYSTEM_LIBMAD/LAME via environment variables, read them in setup.py and pass to cmake
  - Make it compile on Mac OSX