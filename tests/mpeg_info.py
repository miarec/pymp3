import os
import struct


class MPEGHeaderNotFoundError(IOError):
    pass


class MPEGInfo(object):
    '''Parse inforamtion about an MPEG audio file.

    It expects that the beginning of file has a valid MPEG header.

    Extracted attributes:

    * length -- audio length, in seconds
    * file_size -- size of file, in bytes
    * bitrate -- audio bitrate, in bits per second
    * sketchy -- if true, the file may not be valid MPEG audio
    * version -- MPEG version (1, 2, 2.5)
    * layer -- 1, 2, or 3
    * mode -- One of STEREO, JOINTSTEREO, DUALCHANNEL, or MONO (0-3)
    * protected -- whether or not the file is "protected"
    * padding -- whether or not audio frames are padded
    * sample_rate -- audio sample rate, in Hz

    Based on Mutagen (https://bitbucket.org/lazka/mutagen)
    '''

    # Map (version, layer) tuples to bitrates.
    __BITRATE = {
        (1, 1): [0, 32, 64, 96, 128, 160, 192, 224,
                 256, 288, 320, 352, 384, 416, 448],
        (1, 2): [0, 32, 48, 56, 64, 80, 96, 112, 128,
                 160, 192, 224, 256, 320, 384],
        (1, 3): [0, 32, 40, 48, 56, 64, 80, 96, 112,
                 128, 160, 192, 224, 256, 320],
        (2, 1): [0, 32, 48, 56, 64, 80, 96, 112, 128,
                 144, 160, 176, 192, 224, 256],
        (2, 2): [0, 8, 16, 24, 32, 40, 48, 56, 64,
                 80, 96, 112, 128, 144, 160],
    }

    __BITRATE[(2, 3)] = __BITRATE[(2, 2)]
    for i in range(1, 4):
        __BITRATE[(2.5, i)] = __BITRATE[(2, i)]

    # Map version to sample rates.
    __RATES = {
        1: [44100, 48000, 32000],
        2: [22050, 24000, 16000],
        2.5: [11025, 12000, 8000]
    }


    def __init__(self, fileobj, offset=0):
        """Parse MPEG stream information from a file-like object.

        If an offset argument is given, it is used to start looking
        for stream information.
        A correct offset can make loading files significantly faster.
        """

        try:
            size = os.path.getsize(fileobj.name)
        except (IOError, OSError, AttributeError):
            fileobj.seek(0, 2)
            size = fileobj.tell()

        self.file_size = size

        # Try to find two valid headers (meaning, very likely MPEG data)
        # at the given offset, 30% through the file, 60% through the file,
        # and 90% through the file.
        for i in [offset, 0.3 * size, 0.6 * size, 0.9 * size]:
            try:
                self.__try(fileobj, int(i), size - offset)
            except MPEGHeaderNotFoundError:
                pass
            else:
                break
        # If we can't find any two consecutive frames, try to find just
        # one frame back at the original offset given.
        else:
            self.__try(fileobj, offset, size - offset, False)
            self.sketchy = True


    def __try(self, fileobj, offset, real_size, check_second=True):
        # This is going to be one really long function; bear with it,
        # because there's not really a sane point to cut it up.
        fileobj.seek(offset, 0)

        # We "know" we have an MPEG file if we find two frames that look like
        # valid MPEG data. If we can't find them in 32k of reads, something
        # is horribly wrong (the longest frame can only be about 4k). This
        # is assuming the offset didn't lie.
        data = fileobj.read(32768)

        frame_1 = data.find(b"\xff")
        while 0 <= frame_1 <= (len(data) - 4):
            frame_data = struct.unpack(">I", data[frame_1:frame_1 + 4])[0]
            if ((frame_data >> 16) & 0xE0) != 0xE0:
                frame_1 = data.find(b"\xff", frame_1 + 2)
            else:
                version = (frame_data >> 19) & 0x3
                layer = (frame_data >> 17) & 0x3
                protection = (frame_data >> 16) & 0x1
                bitrate = (frame_data >> 12) & 0xF
                sample_rate = (frame_data >> 10) & 0x3
                padding = (frame_data >> 9) & 0x1
                # private = (frame_data >> 8) & 0x1
                self.mode = (frame_data >> 6) & 0x3
                # mode_extension = (frame_data >> 4) & 0x3
                # copyright = (frame_data >> 3) & 0x1
                # original = (frame_data >> 2) & 0x1
                # emphasis = (frame_data >> 0) & 0x3
                if (version == 1 or layer == 0 or sample_rate == 0x3 or
                        bitrate == 0 or bitrate == 0xF):
                    frame_1 = data.find(b"\xff", frame_1 + 2)
                else:
                    break
        else:
            raise MPEGHeaderNotFoundError("can't sync to an MPEG frame")

        self.version = [2.5, None, 2, 1][version]
        self.layer = 4 - layer
        self.protected = not protection
        self.padding = bool(padding)

        self.bitrate = self.__BITRATE[(self.version, self.layer)][bitrate]
        self.bitrate *= 1000
        self.sample_rate = self.__RATES[self.version][sample_rate]

        if self.layer == 1:
            frame_length = (
                (12 * self.bitrate // self.sample_rate) + padding) * 4
            self.frame_size = 384
        elif self.version >= 2 and self.layer == 3:
            frame_length = (72 * self.bitrate // self.sample_rate) + padding
            self.frame_size = 576
        else:
            frame_length = (144 * self.bitrate // self.sample_rate) + padding
            self.frame_size = 1152

        if check_second:
            possible = int(frame_1 + frame_length)
            if possible > len(data) + 4:
                raise MPEGHeaderNotFoundError("can't sync to second MPEG frame")
            try:
                frame_data = struct.unpack(
                    ">H", data[possible:possible + 2])[0]
            except struct.error:
                raise MPEGHeaderNotFoundError("can't sync to second MPEG frame")
            if (frame_data & 0xFFE0) != 0xFFE0:
                raise MPEGHeaderNotFoundError("can't sync to second MPEG frame")

        self.length = 8 * real_size / float(self.bitrate)


    def pprint(self):
        s = "MPEG %s layer %d, %d bps, %s Hz, %.2f seconds" % (
            self.version, self.layer, self.bitrate, self.sample_rate,
            self.length)
        if self.sketchy:
            s += " (sketchy)"
        return s
