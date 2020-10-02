#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {

    mat4 view;
    mat4 proj;

} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 3) in vec2 inUv;
layout(location = 2) in vec3 inTangent;
layout(location = 4) in int inMat;

layout(location = 5) in mat4 transform;

layout (location = 0) out vec4 normal;
layout (location = 1) out vec4 position;
layout (location = 2) out vec2 uvPos;
layout (location = 4) out mat3 toTangentMat;
layout (location = 3) flat out int matIndex;


void main() {

    position = transform * vec4(inPosition, 1.0);// + vec4(int(gl_InstanceIndex / 100) * 2.5, (gl_InstanceIndex % 100) * 2.5, 0, 0);
    gl_Position = ubo.proj * ubo.view * position;


    normal = vec4(normalize((transform * vec4(inNormal, 0)).xyz), 0.0);

    //normal = transform[0];

    gl_Position.y *= -1;

    uvPos = inUv;

    position.w = gl_Position.z;

    vec3 t = normalize((transform * vec4(inTangent, 0)).xyz);

    vec3 bitangent = -cross(t, normal.yxz);

	toTangentMat = (

        mat3(
            t.x, t.y, t.z,
            bitangent.x, bitangent.y, bitangent.z,
            normal.x, normal.y, normal.z
        )

	);

	//normal = vec4(inTangent, 1);
	matIndex = inMat;

}
