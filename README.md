pymp3 - MP3 Encoder/Decoder classes based on lame/mad libraries
===========================================================


# Encoder usage 

The encoder expects raw PCM data in 16-bit interleved format.

Sample code:

    from wave import Wave_read


    with open('audio.wav', 'rb') as read_file, open('audio.mp3', 'wb') as write_file:

        wav_file = Wave_read(read_file)

        sample_size = wav_file.getsampwidth()
        frame_rate = wav_file.getframerate()
        nchannels = wav_file.getnchannels()

        if sample_size != 2:
            raise ValueError("Invalid PCM sample size (%s)" % sample_size)

        encoder = pymp3.Encoder()
        encoder.set_bit_rate(64)
        encoder.set_in_sample_rate(frame_rate)
        encoder.set_channels(nchannels)
        encoder.set_quality(2)   # 2-highest, 7-fastest


        while True:
            pcm_data = wav_file.readframes(8000)

            if not pcm_data:
                mp3_data = encoder.flush()
                write_file.write(mp3_data)
                break
            else:
                mp3_data = encoder.encode(pcm_data)
                write_file.write(mp3_data)


# Decoder usage 

The encoder expects raw MP3 data as an input and produces PCM data in 16-bit interleved format.

Sample code:

    from wave import Wave_write


    with open('audio.mp3', 'rb') as read_file, open('audio.wav', 'wb') as write_file:

        decoder = pymp3.Decoder(read_file)

        sample_rate = decoder.get_sample_rate()
        nchannels = decoder.get_channels()

        wav_file = Wave_write(write_file)
        wave_file.setnchannels(nchannels)
        wave_file.setsampwidth(2)
        wave_file.setframerate(sample_rate)

        while True:
            pcm_data = decoder.read(4000)

            if not pcm_data:
                break
            else:
                write_file.write(pcm_data)



Building a binary package
-----------------------------------------

To build a binary package for your platform (*.whl), run:

    python -m pip wheel .


Troubleshooting a compilation of the library (C code)
------------------------------------------------------

The library is build with CMake, which is automatically called
when setuptools is building the package.

You can call CMake directly to see the error messages if any:

    cmake -S . -B build -DPYTHON_VERSION=3.8 -DPYMAD_VERSION=0.0.1
    cmake --build build