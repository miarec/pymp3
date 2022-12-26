import codecs
from io import BytesIO
import os
import pytest

import mp3


MPEG_FRAME_SIZE = 1152   # A number of samples per MPEG frame (it is always 1152 for MPEG Layer III)

def test_decoder_stereo():
    """
    Test decoding of sample MP3 file
    """

    # MP3 file:
    #  - duration: 0.4s
    #  - sample rate: 8000 Hz
    #  - bitrate: 24kbps
    #  - channels: 2 (stereo)
    #
    # File is generated with:
    #   sox -n -b 16 -r 8000 -c 2 silence-8KHz-stereo-0.4s.wav trim 0.0 0.4
    #   sox -t wav -r 8000 -c 2 silence-8KHz-stereo-0.4s.wav -t mp3 -C 24.01 silence-8KHz-stereo-24kbps-0.4s.mp3
    #
    SAMPLE_MP3_FILE_PATH = os.path.join(os.path.dirname(__file__), 'data', 'silence-8KHz-stereo-24kbps-0.4s.mp3')

    with open(SAMPLE_MP3_FILE_PATH, 'rb') as mp3_file:

        assert mp3_file.tell() == 0, "Initial file position is zero"

        reader = mp3.Decoder(mp3_file)

        assert mp3_file.tell() != 0, "File position after reading first MPEG frame"

        assert reader.is_valid()
        assert reader.get_channels() == 2
        assert reader.get_sample_rate() == 8000
        assert reader.get_bit_rate() == 24
        assert reader.get_layer() == mp3.LAYER_III
        assert reader.get_mode() == mp3.MODE_JOINT_STEREO

        decoded_data = b''
        while True:
            data = reader.read(100)
            if not data:
                break
            decoded_data += data

        # This file contains 4032 samples (0.4s)
        # one sample is 16-bit (2 bytes) per channel, total 2 channels
        # You can use "soxi" command to get info about a number of samples
        assert len(decoded_data) == 4032*2*2

        # The sample MP3 data is a silence only
        assert decoded_data[:32] == b'\x00'*32


def test_decoder_mono():
    """
    Test decoding of sample MP3 file
    """

    # MP3 file:
    #  - duration: 0.5s
    #  - sample rate: 8000 Hz
    #  - bitrate: 32kbps
    #  - channels: 1 (mono)
    #
    # File is generated with:
    #   sox -n -b 16 -r 8000 -c 1 silence-8KHz-mono-0.5s.wav trim 0.0 0.5
    #   sox -t wav -r 8000 -c 1 silence-8KHz-mono-0.5s.wav -t mp3 -C 32.01 silence-8KHz-mono-32kbps-0.5s.mp3
    #
    SAMPLE_MP3_FILE_PATH = os.path.join(os.path.dirname(__file__), 'data', 'silence-8KHz-mono-32kbps-0.5s.mp3')

    with open(SAMPLE_MP3_FILE_PATH, 'rb') as mp3_file:
        reader = mp3.Decoder(mp3_file)

        assert reader.is_valid()
        assert reader.get_channels() == 1
        assert reader.get_sample_rate() == 8000
        assert reader.get_bit_rate() == 32
        assert reader.get_layer() == mp3.LAYER_III
        assert reader.get_mode() == mp3.MODE_SINGLE_CHANNEL

        decoded_data = b''
        while True:
            data = reader.read(100)
            if not data:
                break
            decoded_data += data

        # This file contains 4608 samples (0.5s)
        # one sample is 16-bit (2 bytes) per channel, 1 channel only
        # You can use "soxi" command to get info about a number of samples
        assert len(decoded_data) == 4608*2*1

        # The sample MP3 data is a silence only
        assert decoded_data[:32] == b'\x00'*32


def test_decoder_mono_16KHz():
    """
    Test decoding of sample MP3 file
    """

    # MP3 file:
    #  - duration: 0.6s
    #  - sample rate: 16000 Hz
    #  - bitrate: 32kbps
    #  - channels: 1 (mono)
    #
    # File is generated with:
    #   sox -n -b 16 -r 16000 -c 1 silence-16KHz-mono-0.6s.wav trim 0.0 0.6
    #   sox -t wav -r 16000 -c 1 silence-16KHz-mono-0.6s.wav -t mp3 -C 32.01 silence-16KHz-mono-32kbps-0.6s.mp3
    #
    SAMPLE_MP3_FILE_PATH = os.path.join(os.path.dirname(__file__), 'data', 'silence-16KHz-mono-32kbps-0.6s.mp3')

    with open(SAMPLE_MP3_FILE_PATH, 'rb') as mp3_file:
        reader = mp3.Decoder(mp3_file)

        assert reader.is_valid()
        assert reader.get_channels() == 1
        assert reader.get_sample_rate() == 16000
        assert reader.get_bit_rate() == 32
        assert reader.get_layer() == mp3.LAYER_III
        assert reader.get_mode() == mp3.MODE_SINGLE_CHANNEL

        decoded_data = b''
        while True:
            data = reader.read(100)
            if not data:
                break
            decoded_data += data

        # This file contains 10368 samples (0.6s)
        # one sample is 16-bit (2 bytes) per channel, 1 channel only
        # You can use "soxi" command to get info about a number of samples
        assert len(decoded_data) == 10368*2*1

        # The sample MP3 data is a silence only
        assert decoded_data[:32] == b'\x00'*32


def test_decoder_file_like_object():
    """
    Test file-like object reather than a real file object.
    """

    class FileLikeObject(object):

        def __init__(self, data):
            self.data_io = BytesIO(data)

        def read(self, n):
            return self.data_io.read(n)


    SAMPLE_MP3_FILE_PATH = os.path.join(os.path.dirname(__file__), 'data', 'silence-8KHz-stereo-24kbps-0.4s.mp3')

    with open(SAMPLE_MP3_FILE_PATH, 'rb') as mp3_file:
        file_like_object = FileLikeObject(mp3_file.read())

        reader = mp3.Decoder(file_like_object)

        assert reader.is_valid()
        assert reader.get_channels() == 2
        assert reader.get_sample_rate() == 8000
        assert reader.get_bit_rate() == 24
        assert reader.get_layer() == mp3.LAYER_III
        assert reader.get_mode() == mp3.MODE_JOINT_STEREO

        decoded_data = b''
        while True:
            data = reader.read(1024)
            if not data:
                break
            decoded_data += data

        # This file contains 4032 samples (0.4s)
        assert len(decoded_data) == 1152*14


def test_decoder_invalid_file_format():
    """
    Test opening non-mp3 file (no valid MP3 frames).

    EXPECTED: decoder's read() method returns None.
    """

    invalid_file = BytesIO(b'\x00' * 8000)
    reader = mp3.Decoder(invalid_file)


    assert not reader.is_valid()
    assert reader.get_channels() == 0
    assert reader.get_sample_rate() == 0
    assert reader.get_bit_rate() == 0

    assert b'' == reader.read(512), "Should return None if not MPEG frames are detected"


def test_decoder_invalid_file_object_read_attr():
    """
    Testing file-like object with invalid `read` attribute
    """
    

    class FileLikeObjectInvalid1(object):
        """`read` is not a method"""

        def __init__(self):
            self.read = 'invalid'

    with pytest.raises(TypeError):
        mp3.Decoder(FileLikeObjectInvalid1())

    class FileLikeObjectInvalid2(object):
        """`read()` method has unsupported parameters (missing nbytes argument)"""

        def read(self):
            return self.data_io.read(n)

    mp3.Decoder(FileLikeObjectInvalid2())

    reader = mp3.Decoder(FileLikeObjectInvalid2())

    with pytest.raises(RuntimeError):
        reader.read(1152)


def test_decoder_partially_corrupted_file():
    """
    Test decodding mp3 file with some MP3 frames corrupted.

    EXPECTED: decoder must skip the corrupted frames.
    """

    SAMPLE_WAV_FILE_PATH = os.path.join(os.path.dirname(__file__), 'data', 'silence-8KHz-stereo-24kbps-0.4s.mp3')

    with open(SAMPLE_WAV_FILE_PATH, 'rb') as mp3_file:
        data = mp3_file.read()
        assert len(data) != 0

        # Simulate a corruption of MP3 header (first 12 bytes of header are 0xFFE)
        data = b'\x00\x00' + data[2:]

        fp = BytesIO(data)
        reader = mp3.Decoder(fp)

        assert reader.is_valid()
        assert reader.get_channels() == 2
        assert reader.get_sample_rate() == 8000
        assert reader.get_bit_rate() == 24

        decoded = reader.read(1024*20)

        assert len(decoded) == 10*1152


def test_decoder_invalid_file_open_mode():
    """
    Open file in text mode.

    EXPECTED: failure when calling read() method because Python will fail to decode UTF-8 sequence.
    """

    SAMPLE_MP3_FILE_PATH = os.path.join(os.path.dirname(__file__), 'data', 'silence-8KHz-stereo-24kbps-0.4s.mp3')

    with open(SAMPLE_MP3_FILE_PATH, 'r') as mp3_file:
        reader = mp3.Decoder(mp3_file)

        assert not reader.is_valid()

        with pytest.raises(RuntimeError):
            reader.read(1152)
