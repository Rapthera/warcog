import glob, os

data = bytearray()
mdls = "enum {\n"
anims = ""
i = 0

paths = sorted(glob.iglob(os.path.join("./models/", "*.h")))
for path in paths:
    path2 = os.path.splitext(path)[0]
    name = os.path.basename(path2)

    mdls += "    mdl_" + name + ",\n"

    anims += "enum {\n"

    f = open(path, "r")
    for line in f:
        if len(line[:-1]) == 0:
            continue
        anims += "    mdl_" + name + "_" + line[:-1] + ",\n"

    anims += "};\n\n"

    f.close()
    f = open(path2, "rb")
    data += f.read()
    f.close()

    i += 1

mdls += "};\n\n"

f = open("model", "wb")
f.write(data)
f.close()

f = open("models.h", "w")
f.write(mdls + anims + "#define num_model " + str(i) + "\n\n")
f.close()
