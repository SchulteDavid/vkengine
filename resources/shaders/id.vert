#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUv;

layout (location = 0) out vec3 uv;

void main() {

  gl_Position = vec4(inPosition, 1.0);
  uv = normalize(vec3(inPosition.x, inPosition.y, 1));

}
