#version 450
#extension GL_ARB_separate_shader_objects : enable

/*layout (binding = 1) uniform sampler2D tex;
layout (binding = 2) uniform sampler2D normalMap;
layout (binding = 3) uniform sampler2D specularMap;*/

layout (binding = 1) uniform sampler2D textures[30];

layout (location = 0) in vec4 normal;
layout (location = 1) in vec4 position;
layout (location = 2) in vec2 uvPos;
layout (location = 4) in mat3 toTangentMat;
layout (location = 3) flat in int matIndex;

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outColor;

float ramp(float x, float zVal, float oVal) {

    return clamp((x / (oVal-zVal)) - (zVal / (oVal-zVal)) , 0, 1);

}

void main() {

    outColor = texture(textures[3 * matIndex], uvPos);
    if (outColor.a <= 0.01) discard;

    outPosition = position;
    float roughness = texture(textures[3 * matIndex + 2], uvPos).g;
    outNormal = vec4(toTangentMat * normalize(texture(textures[3 * matIndex + 1], uvPos).xyz * 2.0 - 1.0), roughness);
        outColor.a = texture(textures[3 * matIndex + 2], uvPos).b;


}

