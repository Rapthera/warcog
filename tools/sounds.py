import glob, os
import wave, array
import struct

out = bytearray()
header = "enum {\n"

offsets = array.array('i')
offset = 0
i = 0

paths = sorted(glob.iglob(os.path.join("./sounds/*", "*"))) + sorted(glob.iglob("./sounds/*.wav"))
for path in paths:
    head, tail = os.path.split(path)
    if head == "./sounds":
        print(tail)
        header += "    snd_" + os.path.splitext(tail)[0].lower() + ",\n"
    else:
        print(head, tail, os.path.basename(head))
        header += "    snd_" + os.path.basename(head) + "_" + os.path.splitext(tail)[0].lower() + ",\n"
    f = wave.open(path, "rb")

    assert(f.getsampwidth() == 2)

    if f.getframerate() == 44100:
        frames = f.getnframes() / 2
        data = bytearray()
        d = f.readframes(frames * 2)
        for j in range(frames):
            if f.getnchannels() == 1:
                a, b = struct.unpack("hh", d[j*4:j*4+4])
                data += struct.pack("h", (a + b) // 2)
            elif f.getnchannels() == 2:
                a, b = struct.unpack("hh", d[j*8:j*8+4])
                data += struct.pack("h", (a + b) // 2)
            else:
                assert(False)
    else:
        assert(f.getnchannels() == 1)
        assert(f.getframerate() == 22050)
        frames = f.getnframes()
        data = f.readframes(frames)

    f.close()

    offset += frames * 2
    offsets.append(offset)
    out += data
    i += 1

header += "};\n\n#define num_sound " + str(i) + "\n"
print(header)
f = open("sound", "wb")

assert(offset == len(out))

f.write(offsets)
f.write(out)

f = open("sounds.h", "w")
f.write(header)

"""if f.getsampwidth() == 2:
                a, b = struct.unpack("hh", d[i*4:i*4+4])
                data += struct.pack("h", (a + b) // 2)
            elif f.getsampwidth() == 4:
                a, b = struct.unpack("ff", d[i*8:i*8+8])
                data += struct.pack("h", int(round((a + b) / 2.0)))"""
