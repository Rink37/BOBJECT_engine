import json
import os
from array import array

filename = "coolvetica_sdf_layout.json"

if (os.path.exists(filename)):
    with open(filename) as f:
        data = json.load(f);
        f.close()

glyphs = data["glyphs"][1:]

advances = []

for glyph in glyphs:
    bounds = glyph["planeBounds"]
    #print(glyph["unicode"], glyph["advance"], bounds["right"] - bounds["left"])
    advances += [glyph["advance"]]

print(len(advances))
output_file = open("coolvetica_sdf_advances.bin", 'wb')
advances_array = array('f', advances)
advances_array.tofile(output_file)
output_file.close()