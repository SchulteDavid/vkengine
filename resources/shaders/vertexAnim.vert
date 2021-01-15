#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (push_constant) uniform TransformData {

       mat4 transform;

} transformData;

layout(binding = 0) uniform UniformBufferObject {

    mat4 view;
    mat4 proj;

} ubo;

layout(binding = 2) uniform BoneData {

    mat4 bones[256];

} boneData ;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 3) in vec2 inUv;
layout(location = 2) in vec3 inTangent;
layout(location = 4) in int inMat;

layout(location = 5) in vec4 boneWeights;
layout(location = 6) in ivec4 boneIds;

//layout(location = 7) in mat4 tMat;

layout (location = 0) out vec4 normal;
layout (location = 1) out vec4 position;
layout (location = 2) out vec2 uvPos;
layout (location = 4) out mat3 toTangentMat;
layout (location = 3) flat out int matIndex;

//layout (location = 8) out mat4 skinMat;

//uniform mat4 boneData.bones[boneData.boneCount];

//vec4 boneWeights = vec4(0.6456, 0.17386, 0.13727, 0.04326);
//ivec4 boneIds = ivec4(1, 0, 4, 8);

mat4 getBoneMat(int index) {
  /*if (index < 1) {
    return mat4(1.0);
  } else {*/
    return boneData.bones[index];
  //}
}

void main() {

  mat4 skinMat = boneWeights.x * getBoneMat(boneIds.x)
    + boneWeights.y * getBoneMat(boneIds.y)
    + boneWeights.z * getBoneMat(boneIds.z)
    + boneWeights.w * getBoneMat(boneIds.w);
  
  if(boneIds == ivec4(0)) {
    skinMat = mat4(1.0);
  }
  
  mat4 transform = transformData.transform * transpose(skinMat);
  position = transform * vec4(inPosition, 1.0);
  //position.z *= -1;
  gl_Position = ubo.proj * ubo.view * position;
  gl_Position.y *= -1;
  //gl_Position.z *= -1;
  normal = vec4(normalize((transform * vec4(inNormal, 0)).xyz), 0.0);
  
  uvPos = inUv;
  
  position.w = gl_Position.z;
  
  vec3 t = normalize((transform * vec4(inTangent, 0)).xyz);
  
  vec3 bitangent = -cross(t, normal.yxz);
  
  toTangentMat = mat3(t.x, t.y, t.z,
		      bitangent.x, bitangent.y, bitangent.z,
		      normal.x, normal.y, normal.z );
  
  normal = vec4(inTangent, 1);
  matIndex = inMat;
    
}
