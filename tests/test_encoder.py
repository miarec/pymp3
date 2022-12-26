import codecs
import wave
from io import BytesIO
import os
import pytest
import sys

import mp3

# Add the current directory to sys.path, so we can import 'mpeg_info' file
sys.path.append(os.path.join(os.path.dirname(__file__)))

import mpeg_info


def test_encoder_stereo():
    """
    Test encodign of sample WAV file
    """

    # File is generated with:
    #   sox -n -b 16 -r 8000 -c 2 silence-8KHz-stereo-0.4s.wav trim 0.0 0.4
    #
    SAMPLE_WAV_FILE_PATH = os.path.join(os.path.dirname(__file__), 'data', 'silence-8KHz-stereo-0.4s.wav')

    #with open(SAMPLE_WAV_FILE_PATH, 'rb') as f:
        # wav_file = wave.Wave_read(f)

    duration_ms = 2000
    sample_rate = 8000
    channels = 2
    bit_rate = 32


    # Create MP3 encoder
    temp_fp = BytesIO()
    writer = mp3.Encoder(temp_fp)

    writer.set_channels(channels)
    writer.set_sample_rate(sample_rate)
    writer.set_bit_rate(bit_rate)
    writer.set_mode(mp3.MODE_STEREO if channels == 2 else mp3.MODE_SINGLE_CHANNEL)
    writer.set_quality(2)

    frame_size_ms = 200

    total_write_bytes = int(sample_rate / 1000) * duration_ms * 2 * channels
    written_bytes = 0

    # Encode data in a loop by small portion
    idx = 1
    while written_bytes < total_write_bytes:
        size = int(sample_rate / 1000) * frame_size_ms * 2 * channels
        if size > total_write_bytes - written_bytes:
            size = total_write_bytes - written_bytes
        written_bytes += size

        pcm_data = b'\x00' * size

        assert writer.write(pcm_data) == size, "Encode PCM (iteration #%u)" % idx
        idx += 1

    # Finalize encoding (flush the remaining samples)
    assert writer.flush()

        
    # ---------------------------------------------
    # Validate the endoded MP3 frame data
    # ---------------------------------------------
    written_data = temp_fp.getvalue()
    # assert len(written_data) == 1234

    mp3_info = mpeg_info.MPEGInfo(BytesIO(written_data))
    assert mp3_info.sample_rate == 8000
    assert mp3_info.layer == 3
    assert mp3_info.mode == 0   # Stereo

    assert duration_ms/1000 - 0.2 <= mp3_info.length <= duration_ms/1000 + 0.2


def test_encoder_mono():
    """
    Test encodign of sample WAV file
    """

    # File is generated with:
    #   sox -n -b 16 -r 8000 -c 2 silence-8KHz-stereo-0.4s.wav trim 0.0 0.4
    #
    SAMPLE_WAV_FILE_PATH = os.path.join(os.path.dirname(__file__), 'data', 'silence-8KHz-stereo-0.4s.wav')

    #with open(SAMPLE_WAV_FILE_PATH, 'rb') as f:
        # wav_file = wave.Wave_read(f)

    duration_ms = 3000
    sample_rate = 16000
    channels = 1
    bit_rate = 64


    # Create MP3 encoder
    temp_fp = BytesIO()
    writer = mp3.Encoder(temp_fp)

    writer.set_channels(channels)
    writer.set_sample_rate(sample_rate)
    writer.set_bit_rate(bit_rate)
    writer.set_mode(mp3.MODE_STEREO if channels == 2 else mp3.MODE_SINGLE_CHANNEL)
    writer.set_quality(2)

    frame_size_ms = 200

    total_write_bytes = int(sample_rate / 1000) * duration_ms * 2 * channels
    written_bytes = 0

    # Encode data in a loop by small portion
    idx = 1
    while written_bytes < total_write_bytes:
        size = int(sample_rate / 1000) * frame_size_ms * 2 * channels
        if size > total_write_bytes - written_bytes:
            size = total_write_bytes - written_bytes
        written_bytes += size

        pcm_data = b'\x00' * size

        assert writer.write(pcm_data) == size, "Encode PCM (iteration #%u)" % idx
        idx += 1

    # Finalize encoding (flush the remaining samples)
    assert writer.flush()

        
    # ---------------------------------------------
    # Validate the endoded MP3 frame data
    # ---------------------------------------------
    written_data = temp_fp.getvalue()
    # assert len(written_data) == 1234

    mp3_info = mpeg_info.MPEGInfo(BytesIO(written_data))
    assert mp3_info.sample_rate == 16000
    assert mp3_info.layer == 3
    assert mp3_info.mode == 3   # Mono

    assert duration_ms/1000 - 0.2 <= mp3_info.length <= duration_ms/1000 + 0.2







