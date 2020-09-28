string fragment = "resources/shaders/glsl/low_poly_fragment.frag.spirv"
string vertex = "resources/shaders/glsl/low_poly_vertex_anim.vert.spirv"

int32 textureCount = 0

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
        string attributeName = "TANGENT"
        int32 location = 2
    },
    {
        string attributeName = "TEXCOORD_0"
        int32 location = 3
    },
    {
        string attributeName = "MATERIAL_INDEX"
        int32 location = 4
    },
    {
        string attributeName = "COLOR_0"
        int32 location = 5
    },
    {
	string attributeName = "JOINTS_0"
	int32 location = 6
    },
    {
	string attributeName = "WEIGHTS_0"
	int32 location = 7
    }
]