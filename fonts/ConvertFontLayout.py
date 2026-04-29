import json
import os

filename = "coolvetica_sdf_layout.json"

if (os.path.exists(filename)):
    with open(filename) as f:
        data = json.load(f);
        f.close()
    
print(data["glyphs"])

for glyph in data["glyphs"]:
    print(glyph["unicode"])