#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (binding = 1) uniform sampler2D tex;

layout (location = 0) in vec4 normal;
layout (location = 1) in vec4 position;
layout (location = 2) in vec2 uvPos;

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outColor;

void main() {

    outPosition = position;
    outNormal = normal;
    outColor = texture(tex, uvPos);

}

