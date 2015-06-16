import glob, os
from PIL import Image

data = bytearray()
i = 0

paths = sorted(glob.iglob(os.path.join("./textures/", "*")))
for path in paths:
    print(path)
    f = Image.open(path)
    d = list(f.getdata())
    b = bytearray()

    for x in d:
        for c in x:
            b.append(c)
    assert(len(b) == 256 * 256 * 4)
    data += b
    f.close()

    i += 1

f = open("textures.h", "w")
f.write("#define num_texture " + str(i) + "\n")
f.close()

f = open("texture", "wb")
f.write(data)

im = Image.open("./particles.png")
d = list(im.getdata())
b = bytearray()
for y in range(32):
    for z in range(8):
        for x in range(256):
            for c in d[z * 256 * 32 + y * 256 + x]:
                b.append(c)

assert(len(b) == 256 * 256 * 4)
f.write(b)

im = Image.open("./tileset.png")
d = list(im.getdata())
b = bytearray()
for x in d:
    for y in x:
        b.append(y)
assert(len(b) == 1024 * 1024 * 4)
f.write(b)

f.close()
