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
        wav_file.setnchannels(nchannels)
        wav_file.setsampwidth(2)
        wav_file.setframerate(sample_rate)

        while True:
            pcm_data = decoder.read(4000)

            if not pcm_data:
                break
            else:
                wav_file.writeframes(pcm_data)



Building a binary package
-----------------------------------------

Prerequisites:

- Install `wheel` package with `pip install wheel`


To build a binary package for your platform (*.whl), run:

    pip wheel .

A result of this command will be *.whl file in the current directory.


To install the built WHL file:
------------------------------------------------------

    pip install pymp3*.whl

To build and install the package in development mode:
------------------------------------------------------

    pip install -e .

This command will build `*.so` file (or `*.dll` on Windows) instead of *.whl.

Troubleshooting a compilation of the library (C code)
------------------------------------------------------

The library is build with CMake, which is automatically called
when setuptools is building the package.

You can call CMake directly to see the error messages if any:

    cmake -S . -B build -DPYTHON_VERSION=3.8 -DPYMP3_VERSION=0.0.1
    cmake --build build

This command will build `pymp3.so` (or `pymp3.pyd`) file in the respective build directory
(on Windows, it will be `./build/Release` or `./build/Debug`).

On Windows, by default, Visual Studio builds a Debug configuration. 
Add `--config=Release` to build command to choose Release configuration:

    cmake --build build --config=Release
