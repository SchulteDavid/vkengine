#version 450
#extension GL_ARB_separate_shader_objects : enable

/*layout (binding = 1) uniform sampler2D tex;
layout (binding = 2) uniform sampler2D normalMap;
layout (binding = 3) uniform sampler2D specularMap;*/

layout (binding = 1) uniform sampler2D textures[40];

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

    //outColor = texture(tex, uvPos);
    outColor = texture(textures[4 * matIndex], uvPos);
    //outColor = vec4(matIndex);
    if (outColor.a <= 0.01) discard;

    outPosition = position;
    vec3 dpos = normalize(texture(textures[4 * matIndex + 3], uvPos).xyz * 2.0 - 1.0);
    //float roughness = texture(specularMap, uvPos).g;

    float roughness = texture(textures[4 * matIndex + 2], uvPos).g;
    //outNormal = vec4(toTangentMat * normalize(texture(normalMap, uvPos).xyz * 2.0 - 1.0), roughness);
    vec3 texNormal = normalize(texture(textures[3 * matIndex + 1], uvPos).xyz * 2.0 - 1.0);
    outNormal = vec4(toTangentMat * texNormal, roughness);
    //outColor = mix(texture(tex, uvPos), texture(tex2, uvPos), ramp(abs(dot(vec3(0, 0, 1), normalize(normal.xyz))), 0.3, 0.6));

    outPosition += vec4(toTangentMat * dpos, 0.0) * 0.1;

    //outColor.a = texture(specularMap, uvPos).r;
    outColor.a = texture(textures[4 * matIndex + 2], uvPos).r;

    //outColor = vec4(1);

}
