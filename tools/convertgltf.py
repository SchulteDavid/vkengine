import json
import sys
import struct

def decodeJSONBuffer(byteData):
    data = json.loads(byteData.decode("utf8"))
    return data

f = open(sys.argv[1], "rb")

magic, version, length = struct.unpack("<iii", f.read(12))

if magic != 0x46546C67:
    print("Not a glb file!", file=sys.stderr)
    exit(1)

chunks = []

data = f.read(8)

while data:

    chunkLength, chunkType = struct.unpack("<ii", data)
    chunkData = f.read(chunkLength)
    chunks.append((chunkType, chunkLength, chunkData))
    data = f.read(8)

fileInfo = None
binaryData = None
    
for c in chunks:
    if c[0] == 0x4E4F534A:
        fileInfo = decodeJSONBuffer(c[2])
    if c[0] == 0x004E4942:
        binaryData = c[2]


