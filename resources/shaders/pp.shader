string fragment = "resources/shaders/pp.frag.spirv"
string vertex = "resources/shaders/id.vert.spirv"

int32 textureCount = 30

comp inputs = [
    {
        string attributeName = "POSITION"
        int32 location = 0
    },
    {
        string attributeName = "NORMAL"
        int32 location = 1
    },
    {
        string attributeName = "TEXCOORD_0"
        int32 location = 2
    }
]