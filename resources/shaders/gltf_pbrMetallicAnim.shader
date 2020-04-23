
string fragment = "resources/shaders/glsl/pbrMetallic.frag.spirv"
string vertex = "resources/shaders/vertexAnim.vert.spirv"

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
        int32 location = 3
    },
    {
        string attributeName = "TANGENT"
        int32 location = 2
    },
    {
        string attributeName = "MATERIAL_INDEX"
        int32 location = 4
    },
    {
        string attributeName = "WEIGHTS_0"
        int32 location = 5
    },
    {
        string attributeName = "JOINTS_0"
        int32 location = 6
    }
]
