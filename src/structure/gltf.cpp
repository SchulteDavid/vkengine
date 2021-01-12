#include "gltf.h"

#include <string>
#include <nlohmann/json.hpp>

#include "util/mesh.h"
#include "util/meshhelper.h"
#include <mathutils/vector.h>
#include <mathutils/quaternion.h>

#include "util/debug/trace_exception.h"
#include "util/image/png.h"
#include "animation/skeletalrig.h"

using json = nlohmann::json;
using namespace Math;

typedef struct glb_header_t {

  uint32_t magic;
  uint32_t version;
  uint32_t length;

} glb_header_t;

typedef struct glb_chunk_t {

  uint32_t chunkLength;
  uint32_t chunkType;

  uint8_t * rawData;

} glb_chunk_t;

struct gltf_asset_t {

  std::string version;
  std::string generator;
  std::string copyright;

};

void from_json(const json & j, gltf_asset_t & asset) {

  j.at("version").get_to(asset.version);
  j.at("generator").get_to(asset.generator);
  try {
    j.at("copyright").get_to(asset.copyright);
  } catch (std::exception e) {
    asset.copyright = "nobody";
  }

}

struct gltf_scene_t {

  std::string name;
  std::vector<int> nodes;

};

void from_json(const json & j, gltf_scene_t & scene) {

  j.at("name").get_to(scene.name);
  j.at("nodes").get_to(scene.nodes);

}

struct gltf_node_t {

  std::string name;
  int mesh;

  Math::Vector<3,float> translation;
  Math::Quaternion<float> rotation;
  Math::Vector<3, float> scale;

  Math::Vector<3,float> globalTranslation;
  Math::Quaternion<float> globalRotation;
  Math::Vector<3, float> globalScale;

  std::vector<int> children;
  int skin;

};

void from_json(const json & j, gltf_node_t & node) {

  j.at("name").get_to(node.name);
  try {
    j.at("mesh").get_to(node.mesh);
  } catch (std::exception e) {
    node.mesh = -1;
  }

  try {
    std::vector<float> tData = j.at("translation").get<std::vector<float>>();
    node.translation = Math::Vector<3, float>(tData.data());
  } catch (std::exception e) {
  }

  try {
    std::vector<float> rData = j.at("rotation").get<std::vector<float>>();
    node.rotation = Math::Quaternion<float>(rData[3], rData[0], rData[1], rData[2]);
  } catch (std::exception e) {
    node.rotation = Math::Quaternion<float>(1,0,0,0);
  }

  try {
    std::vector<float> tData = j.at("scale").get<std::vector<float>>();
    node.scale = Math::Vector<3, float>(tData.data());
  } catch (std::exception e) {
    float tmp[3] = {1,1,1};
    node.scale = Math::Vector<3,float>(tmp);
  }

  try {
    j.at("children").get_to(node.children);
  } catch (std::exception e) {

  }

  try {
    j.at("skin").get_to(node.skin);
  } catch (std::exception e) {
    node.skin = -1;
  }

}

struct gltf_material_texture_t {

  int index;
  int texCoords;

};

void from_json(const json & j, gltf_material_texture_t & tex) {

  j.at("index").get_to(tex.index);
  j.at("texCoord").get_to(tex.texCoords);

}

struct gltf_material_t {

  std::string name;
  bool doubleSided;
  gltf_material_texture_t normalTexture;
  gltf_material_texture_t baseColorTexture;
  gltf_material_texture_t metallicRoughnessTexture;

};

void from_json(const json & j, gltf_material_t & mat) {

  j.at("name").get_to(mat.name);
  j.at("doubleSided").get_to(mat.doubleSided);

  try {
    j.at("normalTexture").get_to(mat.normalTexture);
  } catch (std::exception e) {

  }

  try {
    j.at("pbrMetallicRoughness").at("baseColorTexture").get_to(mat.baseColorTexture);
  } catch (std::exception e) {

  }

  try {
    j.at("pbrMetallicRoughness").at("metallicRoughnessTexture").get_to(mat.metallicRoughnessTexture);
  } catch (std::exception e) {

  }

}

struct gltf_buffer_view_t {

  int buffer;
  int byteLength;
  int byteOffset;
  int target;
  int byteStride;

};

void from_json(const json & j, gltf_buffer_view_t & view) {

  j.at("buffer").get_to(view.buffer);
  j.at("byteLength").get_to(view.byteLength);
  j.at("byteOffset").get_to(view.byteOffset);
  try {
    j.at("byteStride").get_to(view.byteStride);
  } catch (std::exception e) {
    view.byteStride = 1;
  }

  try {
    j.at("target").get_to(view.target);
  } catch (std::exception e) {
    view.target = -1;
  }

}

enum gltf_data_type_e {

		       GLTF_TYPE_SINT8 = 5120,
		       GLTF_TYPE_UINT8 = 5121,

		       GLTF_TYPE_SINT16 = 5122,
		       GLTF_TYPE_UINT16 = 5123,

		       GLTF_TYPE_UINT32 = 5125,

		       GLTF_TYPE_SFLOAT = 5126,

};

size_t gltfGetDataSize(int dataTypeId) {

  switch (dataTypeId) {

  case GLTF_TYPE_SINT8:
  case GLTF_TYPE_UINT8:
    return sizeof(uint8_t);

  case GLTF_TYPE_SINT16:
  case GLTF_TYPE_UINT16:
    return sizeof(uint16_t);

  case GLTF_TYPE_UINT32:
    return sizeof(uint32_t);

  case GLTF_TYPE_SFLOAT:
    return sizeof(float);

  default:
    throw dbg::trace_exception("Invalid dataTypeId");

  }

}

size_t gltfGetElementCount(std::string type) {

  if (!type.compare("SCALAR")) {
    return 1;
  } else if (!type.compare("VEC2")) {
    return 2;
  } else if (!type.compare("VEC3")) {
    return 3;
  } else if (!type.compare("VEC4")) {
    return 4;
  } else if (!type.compare("MAT2")) {
    return 4;
  } else if (!type.compare("MAT3")) {
    return 9;
  } else if (!type.compare("MAT4")) {
    return 16;
  } else {
    throw dbg::trace_exception("Unknown data type");
  }

}

struct gltf_accessor_t {

  int bufferView;
  unsigned int count;

  size_t dataTypeSize;
  size_t dataElementCount;

  int rawDataType;

};

void from_json(const json & j, gltf_accessor_t & acc) {

  j.at("bufferView").get_to(acc.bufferView);
  j.at("count").get_to(acc.count);

  acc.dataTypeSize = gltfGetDataSize(j.at("componentType").get<int>());
  acc.dataElementCount = gltfGetElementCount(j.at("type").get<std::string>());

  j.at("componentType").get_to(acc.rawDataType);

}

struct gltf_texture_t {
  int source;
};

void from_json(const json & j, gltf_texture_t & tex) {

  j.at("source").get_to(tex.source);

}

enum gltf_image_mime_e {
			GLTF_IMAGE_UNKNOWN,
			GLTF_IMAGE_PNG
};

struct gltf_image_t {

  int bufferView;
  gltf_image_mime_e mime;
  std::string name;

};

gltf_image_mime_e gltfGetImageMime(std::string mimeType) {

  if (!mimeType.compare("image/png")) {
    return GLTF_IMAGE_PNG;
  }

  return GLTF_IMAGE_UNKNOWN;
}

void from_json(const json & j, gltf_image_t & img) {

  j.at("bufferView").get_to(img.bufferView);
  j.at("name").get_to(img.name);

  img.mime = gltfGetImageMime(j.at("mimeType").get<std::string>());

}

struct gltf_mesh_primitive_t {

  int position;
  int normal;
  int uv;
  int indices;
  int material;

};

void from_json(const json & j, gltf_mesh_primitive_t & prim) {

  try {
    j.at("material").get_to(prim.material);
  } catch (std::exception e) {
    prim.material = -1;
  }
  j.at("indices").get_to(prim.indices);

  j.at("attributes").at("POSITION").get_to(prim.position);
  j.at("attributes").at("NORMAL").get_to(prim.normal);
  j.at("attributes").at("TEXCOORD_0").get_to(prim.uv);

}

struct gltf_mesh_t {

  std::string name;
  std::vector<gltf_mesh_primitive_t> primitives;
  std::unordered_map<std::string, int> attributes;

};

void from_json(const json & j, gltf_mesh_t & mesh) {

  j.at("primitives").get_to(mesh.primitives);
  /*for (auto it : ) {

    lout << it << std::endl;

    }*/
  j.at("primitives")[0].at("attributes").get_to(mesh.attributes);

  j.at("name").get_to(mesh.name);

}

struct gltf_animation_channel_t {

  int sampler;
  int node;
  std::string path;

};

void from_json(const json & j, gltf_animation_channel_t & channel) {

  j.at("sampler").get_to(channel.sampler);
  j.at("target").at("node").get_to(channel.node);
  j.at("target").at("path").get_to(channel.path);

}

struct gltf_animation_sampler_t {

  int input;
  int output;
  std::string interpolation;

};

void from_json(const json & j, gltf_animation_sampler_t & sampler) {

  j.at("input").get_to(sampler.input);
  j.at("output").get_to(sampler.output);
  j.at("interpolation").get_to(sampler.interpolation);

}

struct gltf_animation_t {

  std::vector<gltf_animation_channel_t> channels;
  std::vector<gltf_animation_sampler_t> samplers;

  std::string name;

};

void from_json(const json & j, gltf_animation_t & animation) {

  j.at("channels").get_to(animation.channels);
  j.at("samplers").get_to(animation.samplers);
  j.at("name").get_to(animation.name);

}

struct gltf_skin_t {

  int inverseBindMatrices;
  std::vector<int> joints;

};

void from_json(const json & j, gltf_skin_t & skin) {

  j.at("inverseBindMatrices").get_to(skin.inverseBindMatrices);
  j.at("joints").get_to(skin.joints);

}

template <typename T> T gltfGetBufferData(const gltf_accessor_t & acc, const gltf_buffer_view_t & bufferView, uint8_t * data, int index) {

  /*if (index > acc.count)
    throw dbg::trace_exception("No enougth elements in buffer view");*/

  uint32_t offset = bufferView.byteOffset;

  offset += index * sizeof(T);

  return *((T *) (data + offset));

}

VertexAttributeType gltfDecodeAttributeType(const gltf_accessor_t & acc) {

  gltf_data_type_e dataType = (gltf_data_type_e) acc.rawDataType;

  switch(dataType) {

  case GLTF_TYPE_SFLOAT:
    return static_cast<VertexAttributeType>(ATTRIBUTE_F32_SCALAR + (acc.dataElementCount-1));

  case GLTF_TYPE_UINT32:
    return static_cast<VertexAttributeType>(ATTRIBUTE_I32_SCALAR + (acc.dataElementCount-1));

  case GLTF_TYPE_SINT16:
  case GLTF_TYPE_UINT16:
    return static_cast<VertexAttributeType>(ATTRIBUTE_I16_SCALAR + (acc.dataElementCount-1));

  default:
    throw dbg::trace_exception(std::string("Unable to decode gltf VertexAttributeType ").append(std::to_string(dataType)));
  }

}

void gltfLoadFloatBuffer(gltf_accessor_t & acc, gltf_buffer_view_t & view, uint8_t * buffer, std::vector<VertexAttribute::VertexAttributeData> & value) {

  for (unsigned int i = 0; i < acc.count; ++i) {

    value[i].f = gltfGetBufferData<float>(acc, view, buffer, i);

  }

}

std::vector<float> gltfLoadFloatBuffer(const gltf_accessor_t & acc, const gltf_buffer_view_t & view, uint8_t * buffer) {

  std::vector<float> data(acc.count);
  for (unsigned int i = 0; i < acc.count; ++i) {
    data[i] = gltfGetBufferData<float>(acc, view, buffer, i);
  }

  return data;
  
}

void gltfLoadIntBuffer(gltf_accessor_t & acc, gltf_buffer_view_t & view, uint8_t * buffer, std::vector<VertexAttribute::VertexAttributeData> & value) {

  for (unsigned int i = 0; i < acc.count; ++i) {

    value[i].i32 = gltfGetBufferData<int32_t>(acc, view, buffer, i);

  }

}

template <unsigned int dim, typename T> std::vector<Math::Vector<dim, T>> gltfLoadVecBuffer(const gltf_accessor_t & acc, const gltf_buffer_view_t & view, uint8_t * buffer) {

  std::vector<Math::Vector<dim, T>> value(acc.count);

  for (unsigned int i = 0; i < acc.count; ++i) {

    T tmp[dim];

    for (unsigned int j = 0; j < dim; ++j)
      tmp[j] = gltfGetBufferData<T>(acc, view, buffer, i*dim+j);

    value[i] = Vector<dim, T>(tmp);

  }
  return value;

}

template <typename T> std::vector<Math::Quaternion<T>> gltfLoadQuaternionBuffer(const gltf_accessor_t & acc, const gltf_buffer_view_t & view, uint8_t * buffer) {

  std::vector<Math::Quaternion<T>> value(acc.count);

  for (unsigned int i = 0; i < acc.count; ++i) {

    T tmp[4];

    for (unsigned int j = 0; j < 4; ++j)
      tmp[j] = gltfGetBufferData<T>(acc, view, buffer, i*4+j);

    value[i] = Quaternion<T>(tmp[3], tmp[0], tmp[1], tmp[2]);

  }
  return value;

}

template <unsigned int dim, typename T> std::vector<Math::Matrix<dim,dim,T>> gltfLoadMatrixBuffer(gltf_accessor_t & acc, gltf_buffer_view_t & view, uint8_t * buffer) {

  std::vector<Math::Matrix<dim,dim, T>> value(acc.count);

  T * data = (T*) (buffer + view.byteOffset);
  alignas(16) T mData[dim*dim];

  for (unsigned int i = 0; i < acc.count; ++i) {

    memcpy(mData, data + (dim * dim) * i, sizeof(T) * dim * dim);
    value[i] = Matrix<dim,dim,T>(mData);

  }
  return value;

}

template< typename T > std::string int_to_hex( T i ) {

  std::stringstream stream;
  stream << "0x" << std::hex << i;
  return stream.str();

}

void gltfUpdateChildGlobalTransform(gltf_node_t & node, std::vector<gltf_node_t> & nodes, Vector<3,float> translation, Vector<3,float> scale, Quaternion<float> rotation) {

  ///TODO: check rotation order.
  node.globalRotation = node.rotation * rotation;
  lout << "node rotation " << node.rotation << " rotation " << rotation << " -> " << node.globalRotation << std::endl;
  float tmp[3] = {scale[0] * node.scale[0], scale[1] * node.scale[1], scale[2] * node.scale[2]};
  node.globalScale = Vector<3,float>(tmp);
  node.globalTranslation = translation + node.translation;

  for (int cIndex : node.children) {

    gltfUpdateChildGlobalTransform(nodes[cIndex], nodes, node.globalTranslation, node.globalScale, node.globalRotation);

  }

}

std::shared_ptr<Mesh> gltfLoadMesh(gltf_mesh_t & mesh, std::vector<gltf_accessor_t> & accessors, std::vector<gltf_buffer_view_t> & bufferViews, uint8_t * buffer) {

  gltf_mesh_primitive_t prim = mesh.primitives[0];

  std::unordered_map<std::string, VertexAttribute> attributes;

  for (auto it : mesh.attributes) {

    gltf_accessor_t acc = accessors[it.second];

    VertexAttribute attr;
    attr.type = gltfDecodeAttributeType(acc);
    attr.value = std::vector<VertexAttribute::VertexAttributeData>(acc.count);

    lout << "Loading buffer of type " << std::hex << attr.type << std::dec << " for " << it.first << std::endl;

    switch (attr.type) {

    case ATTRIBUTE_F32_SCALAR:
      gltfLoadFloatBuffer(acc, bufferViews[acc.bufferView], buffer, attr.value);
      break;

    case ATTRIBUTE_F32_VEC2:
      {
	std::vector<Math::Vector<2, float>> data = gltfLoadVecBuffer<2, float>(acc, bufferViews[acc.bufferView], buffer);
	for (unsigned int i = 0; i < data.size(); ++i) {
	  attr.value[i].vec2 = data[i];
	}
      }
      break;

    case ATTRIBUTE_F32_VEC3:
      {
	std::vector<Math::Vector<3, float>> data = gltfLoadVecBuffer<3, float>(acc, bufferViews[acc.bufferView], buffer);
	for (unsigned int i = 0; i < data.size(); ++i) {
	  attr.value[i].vec3 = data[i];
	}
      }
      break;

    case ATTRIBUTE_F32_VEC4:
      {
	std::vector<Math::Vector<4, float>> data = gltfLoadVecBuffer<4, float>(acc, bufferViews[acc.bufferView], buffer);
	for (unsigned int i = 0; i < data.size(); ++i) {
	  attr.value[i].vec4 = data[i];
	}
      }
      break;

    case ATTRIBUTE_I32_SCALAR:
      gltfLoadIntBuffer(acc, bufferViews[acc.bufferView], buffer, attr.value);
      break;

    case ATTRIBUTE_I32_VEC2:
      {
	std::vector<Math::Vector<2, int32_t>> data = gltfLoadVecBuffer<2, int32_t>(acc, bufferViews[acc.bufferView], buffer);
	for (unsigned int i = 0; i < data.size(); ++i) {
	  attr.value[i].i32_vec2 = data[i];
	}
      }
      break;

    case ATTRIBUTE_I32_VEC3:
      {
	std::vector<Math::Vector<3, int32_t>> data = gltfLoadVecBuffer<3, int32_t>(acc, bufferViews[acc.bufferView], buffer);
	for (unsigned int i = 0; i < data.size(); ++i) {
	  attr.value[i].i32_vec3 = data[i];
	}
      }
      break;

    case ATTRIBUTE_I32_VEC4:
      {
	std::vector<Math::Vector<4, int32_t>> data = gltfLoadVecBuffer<4, int32_t>(acc, bufferViews[acc.bufferView], buffer);
	for (unsigned int i = 0; i < data.size(); ++i) {
	  attr.value[i].i32_vec4 = data[i];
	}
      }
      break;

    case ATTRIBUTE_I16_VEC2:
      {
	std::vector<Math::Vector<2, int16_t>> data = gltfLoadVecBuffer<2, int16_t>(acc, bufferViews[acc.bufferView], buffer);
	for (unsigned int i = 0; i < data.size(); ++i) {
	  attr.value[i].i16_vec2 = data[i];
	}
      }
      break;

    case ATTRIBUTE_I16_VEC3:
      {
	std::vector<Math::Vector<3, int16_t>> data = gltfLoadVecBuffer<3, int16_t>(acc, bufferViews[acc.bufferView], buffer);
	for (unsigned int i = 0; i < data.size(); ++i) {
	  attr.value[i].i16_vec3 = data[i];
	}
      }
      break;

    case ATTRIBUTE_I16_VEC4:
      {
	std::vector<Math::Vector<4, int16_t>> data = gltfLoadVecBuffer<4, int16_t>(acc, bufferViews[acc.bufferView], buffer);
	for (unsigned int i = 0; i < data.size(); ++i) {
	  attr.value[i].i16_vec4 = data[i];
	}
      }
      break;

    default:
      throw dbg::trace_exception(std::string("Unknown vertex attribute type: ").append(int_to_hex<int>(attr.type)));

    }

    attributes[it.first] = attr;

  }

  gltf_accessor_t & indexAcc = accessors[prim.indices];

  std::vector<uint16_t> indices(indexAcc.count);

  for (unsigned int i = 0; i < indexAcc.count; ++i) {

    indices[i] = gltfGetBufferData<uint16_t>(indexAcc, bufferViews[indexAcc.bufferView], buffer, i);

  }

  std::shared_ptr<Mesh> mmesh(new Mesh(attributes, indices));

  mmesh->setMaterialIndex(mesh.primitives[0].material);

  float zupData[16] = {
		       1, 0, 0, 0,
		       0, 0, -1, 0,
		       0, 1, 0, 0,
		       0, 0, 0, 1
  };
  Math::Matrix<4, 4, float> zupMatrix(zupData);

  return zupMatrix * mmesh;

}

std::shared_ptr<Skin> gltfLoadSkin(gltf_skin_t & skin, std::vector<gltf_accessor_t> & accessors, std::vector<gltf_buffer_view_t> & bufferViews, uint8_t * buffer, std::vector<gltf_node_t> & nodes) {

  std::vector<Joint> joints(skin.joints.size());

  gltf_accessor_t acc = accessors[skin.inverseBindMatrices];
  std::vector<Matrix<4,4,float>> transforms = gltfLoadMatrixBuffer<4,float>(acc, bufferViews[acc.bufferView], buffer);

  float offset[3] = {0,0,0};

  for (unsigned int i = 0; i < skin.joints.size(); ++i) {

    joints[i].inverseTransform = transforms[i];
    joints[i].offset = nodes[skin.joints[i]].globalTranslation;
    joints[i].rotation = nodes[skin.joints[i]].globalRotation;
    lout << "inverse Transform of joint " << i << " : " << joints[i].inverseTransform << std::endl;
    //lout << "rotation of joint " << i << ": " << joints[i].rotation << std::endl;
    //joints[i].offset = Vector<3,float>(offset);
    //joints[i].rotation = Quaternion<float>(1,0,0,0);

  }

  return std::shared_ptr<Skin>(new Skin(joints));

}

std::vector<uint8_t> gltfLoadPackedImage(uint8_t * buffer, gltf_buffer_view_t & bufferView, uint32_t * width, uint32_t * height) {

  uint32_t chanelCount;
  uint8_t * data = pngLoadImageDataMemory(buffer + bufferView.byteOffset, width, height, &chanelCount);

  if (!data)
    throw dbg::trace_exception("Unable to load PNG");

  std::vector<uint8_t> fData(*width * *height * 4);

  for (unsigned int i = 0; i < *height; ++i) {
    for (unsigned int j = 0; j < *width; ++j) {
      for (unsigned int c = 0; c < chanelCount; ++c) {
	fData[(i * *width + j) * 4 + c] = data[(i * *width + j) * chanelCount + c];
      }

      if (chanelCount < 4) {
	fData[(i * *width + j) * 4 + 3] = 255;
      }

    }
    //fData[i] = (float) data[i] / 255.0;
  }

  return fData;

}

struct gltf_file_data_t {

  uint32_t rootScene;

  std::vector<gltf_scene_t> scenes;
  std::vector<gltf_node_t> nodes;
  std::vector<gltf_material_t> materials;
  std::vector<gltf_buffer_view_t> bufferViews;
  std::vector<gltf_accessor_t> accessors;
  std::vector<gltf_image_t> images;
  std::vector<gltf_texture_t> textures;
  std::vector<gltf_mesh_t> meshes;

  std::vector<gltf_animation_t> animations;
  std::vector<gltf_skin_t> skins;

  uint8_t * binaryBuffer;

};

std::vector<std::shared_ptr<GLTFNode>> gltfLoadFile(std::string fname, gltf_file_data_t * data) {

  if (fname.substr(fname.length()-3).compare("glb"))
    throw res::wrong_file_exception("Not a glb file");

  FILE * file = fopen(fname.c_str(), "rb");
  if (!file)
    throw dbg::trace_exception(std::string("No such file: ").append(fname));

  glb_header_t header;
  fread(&header, sizeof(glb_header_t), 1, file);

  if (header.magic != 0x46546C67) {
    throw dbg::trace_exception("Incorrect header for glb file");
  } else if (header.version != 2) {
    throw dbg::trace_exception("Incorrect version for glb file");
  }

  std::vector<glb_chunk_t> chunks;

  while (!feof(file)) {

    glb_chunk_t chunk;
    fread(&chunk.chunkLength, sizeof(uint32_t), 1, file);
    fread(&chunk.chunkType, sizeof(uint32_t), 1, file);

    chunk.rawData = new uint8_t[(chunk.chunkType == 0x4E4F534A) ? chunk.chunkLength+1 : chunk.chunkLength];

    fread(chunk.rawData, sizeof(uint8_t), chunk.chunkLength, file);

    if (chunk.chunkType == 0x4E4F534A) {
      chunk.rawData[chunk.chunkLength] = 0;
    }

    chunks.push_back(chunk);

  }

  fclose(file);

  std::string str((char *) chunks[0].rawData);

  json jsonData = json::parse(str);

  gltf_asset_t asset = jsonData["asset"].get<gltf_asset_t>();
  std::vector<gltf_scene_t> scenes = jsonData["scenes"].get<std::vector<gltf_scene_t>>();
  std::vector<gltf_node_t> nodes = jsonData["nodes"].get<std::vector<gltf_node_t>>();
  std::vector<gltf_material_t> materials;
  std::vector<gltf_image_t> images;
  std::vector<gltf_texture_t> textures;

  try {

    materials = jsonData["materials"].get<std::vector<gltf_material_t>>();
    images = jsonData["images"].get<std::vector<gltf_image_t>>();
    textures = jsonData["textures"].get<std::vector<gltf_texture_t>>();

  } catch (json::type_error & e) {

    //throw dbg::trace_exception("Missing material information from gltf file");

  }
  std::vector<gltf_buffer_view_t> bufferViews = jsonData["bufferViews"].get<std::vector<gltf_buffer_view_t>>();
  std::vector<gltf_accessor_t> accessors = jsonData["accessors"].get<std::vector<gltf_accessor_t>>();
  std::vector<gltf_mesh_t> meshes = jsonData["meshes"].get<std::vector<gltf_mesh_t>>();

  std::vector<gltf_animation_t> animations;
  std::vector<gltf_skin_t> skins;
  try  {

    lout << "Loading animations" << std::endl;
    animations = jsonData.at("animations").get<std::vector<gltf_animation_t>>();

  } catch (json::out_of_range & e) {

  }

  try {

    lout << "Loading skins from gltf-file" << std::endl;
    skins = jsonData.at("skins").get<std::vector<gltf_skin_t>>();

  } catch (json::out_of_range & e) {

  }

  int sceneID = jsonData["scene"].get<int>();
  uint8_t * binaryBuffer = chunks[1].rawData;

  float tmp[3] = {0,0,0};
  Vector<3, float> initTranslation(tmp);
  tmp[0] = tmp[1] = tmp[2] = 1;
  Vector<3, float> initScale(tmp);
  Quaternion<float> initRotation(0.7071068,-0.7071068,0,0);

  for (unsigned int i = 0; i < scenes[sceneID].nodes.size(); ++i) {
    gltfUpdateChildGlobalTransform(nodes[scenes[sceneID].nodes[i]], nodes, initTranslation, initScale, initRotation);
  }

  /// Loading meshes into internal format (could be moved up???)
  std::vector<std::shared_ptr<Mesh>> meshData(meshes.size());
  for (unsigned int i = 0; i < meshes.size(); ++i) {
    meshData[i] = gltfLoadMesh(meshes[i], accessors, bufferViews, binaryBuffer);
  }

  /// Loading skins
  std::vector<std::shared_ptr<Skin>> skinData(skins.size());
  for (unsigned int i = 0; i < skins.size(); ++i) {
    skinData[i] = gltfLoadSkin(skins[i], accessors, bufferViews, binaryBuffer, nodes);
  }

  /// Loading nodes
  std::vector<std::shared_ptr<GLTFNode>> nodeData(nodes.size());
  for (unsigned int i = 0; i < nodes.size(); ++i) {

    std::shared_ptr<GLTFNode> node(new GLTFNode());
    node->setPosition(nodes[i].translation);
    node->setRotation(nodes[i].rotation);
    node->setScale(nodes[i].scale);
    node->setName(nodes[i].name);

    if (nodes[i].mesh >= 0) {
      node->setMesh(meshData[nodes[i].mesh]);
    }

    if (nodes[i].skin >= 0) {
      if (skinData.size() <= nodes[i].skin)
	throw dbg::trace_exception("Node uses undefined skin");
      node->setSkin(skinData[nodes[i].skin]);
    }

    nodeData[i] = node;

  }
  /// Linking nodes with children.
  for (unsigned int i = 0; i < nodes.size(); ++i) {
    for (int n : nodes[i].children) {
      nodeData[i]->addChild(nodeData[n]);
    }
  }

  std::vector<std::shared_ptr<GLTFNode>> rootNodes(scenes[sceneID].nodes.size());
  for (unsigned int i = 0; i < scenes[sceneID].nodes.size(); ++i) {
    rootNodes[i] = nodeData[scenes[sceneID].nodes[i]];
  }

  if (data) {

    data->accessors = accessors;
    data->bufferViews = bufferViews;
    data->images = images;
    data->materials = materials;
    data->meshes = meshes;
    data->nodes = nodes;
    data->scenes = scenes;
    data->textures = textures;
    data->binaryBuffer = binaryBuffer;

    data->animations = animations;
    data->skins = skins;

    data->rootScene = sceneID;

  } else {
    delete[] binaryBuffer;
  }

  return rootNodes;

}

std::shared_ptr<Mesh> gltfBuildMesh(std::shared_ptr<GLTFNode> node) {

  if (!node) return nullptr;

  if (!node->hasChildren()) {

    if (node->hasMesh()) {
      return node->getTransform() * node->getMesh();
    } else {
      return nullptr;
    }

  }

  std::shared_ptr<Mesh> mesh = node->getMesh();

  for (std::shared_ptr<GLTFNode> n : node->getChildren()) {

    mesh = Mesh::merge(mesh, gltfBuildMesh(n));

  }

  return mesh;

}

std::shared_ptr<Skin> gltfBuildSkin(std::shared_ptr<GLTFNode> node) {

  if (!node) return nullptr;

  if (!node->hasChildren()) {
    return node->getSkin();
  }

  for (std::shared_ptr<GLTFNode> n : node->getChildren()) {

    std::shared_ptr<Skin> ptr = gltfBuildSkin(n);
    if (ptr)
      return ptr;

  }

  return nullptr;

}

struct gltf_anim_vertex {

  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec3 tangent;
  glm::vec2 uv;
  int32_t matIndex;

  glm::vec4 weights;
  int16_t bones[4];

};

#include "node/nodeloader.h"
#include "node/meshnode.h"

GLTFNodeLoader::GLTFNodeLoader() {

}

std::string gltfSubresName(const std::string fname, const std::string name) {
  std::string resName = fname;
  return resName.append(":").append(name);
}

LoadingResource GLTFNodeLoader::loadTexture(gltf_file_data_t & fileData, const int textureId, const std::string fname) {

  gltf_texture_t & texture = fileData.textures[textureId];

  gltf_image_t & image = fileData.images[texture.source];

  uint32_t width, height;
  std::vector<uint8_t> imageData = gltfLoadPackedImage(fileData.binaryBuffer, fileData.bufferViews[image.bufferView], &width, &height);

  std::shared_ptr<ResourceUploader<Resource>> upldr((ResourceUploader<Resource> *) new TextureUploader<uint8_t>(imageData, width, height, 1));

  std::string textureName = gltfSubresName(fname, image.name);

  return uploadResource(ResourceLocation("Texture", fname, image.name), upldr);

}

LoadingResource GLTFNodeLoader::loadMaterial(gltf_file_data_t & fileData, const int materialId, const std::string fname) {

  gltf_material_t & material = fileData.materials[materialId];

  LoadingResource shader = loadDependency(ResourceLocation("Shader", "resources/shaders/gltf_pbrMetallic.shader"));
  LoadingResource staticShader = loadDependency(ResourceLocation("Shader", "resources/shaders/gltf_pbrMetallicStatic.shader"));

  LoadingResource colorImg = loadTexture(fileData, material.baseColorTexture.index, fname);
  LoadingResource normalImg = loadTexture(fileData, material.normalTexture.index, fname);
  LoadingResource metalImg = loadTexture(fileData, material.metallicRoughnessTexture.index, fname);

  std::vector<LoadingResource> textures = {colorImg, normalImg, metalImg};

  std::shared_ptr<ResourceUploader<Resource>> upldr((ResourceUploader<Resource> *) new MaterialUploader(shader, staticShader, textures));


  return uploadResource(ResourceLocation("Material", fname, material.name), upldr);

}

#include <list>

void updateLoadedPositions(std::unordered_map<float, Transform<float>> & transforms, std::vector<float> & times, std::vector<Math::Vector<3, float>> & positions) {

  if (times.size() != positions.size()) {
    throw dbg::trace_exception("Timestamp and position count do not match in animation");
  }

  for (unsigned int i = 0; i < times.size(); ++i) {

    float time = times[i];

    if (transforms.find(time) != transforms.end()) {

      transforms[time].position = Vector<3, float>({positions[i][0], -positions[i][2], positions[i][1]});
      //transforms[time].position = positions[i];
      
    } else {

      /// TODO : Update to interpolate nearest keyframes for other chanels.
      
      Transform<float> trans;
      trans.position = Vector<3, float>({positions[i][0], -positions[i][2], positions[i][1]});
      //trans.position = positions[i];

      transforms[time] = trans;
      
    }
    
  }
  
}

void updateLoadedScales(std::unordered_map<float, Transform<float>> & transforms, std::vector<float> & times, std::vector<Math::Vector<3, float>> & scales) {

  if (times.size() != scales.size()) {
    throw dbg::trace_exception("Timestamp and position count do not match in animation");
  }

  for (unsigned int i = 0; i < times.size(); ++i) {

    float time = times[i];

    if (transforms.find(time) != transforms.end()) {

      transforms[time].scale = Vector<3, float>({scales[i][0], -scales[i][2], scales[i][1]});
      //transforms[time].scale = scales[i];

    } else {

      /// TODO : Update to interpolate nearest keyframes for other chanels.
      
      Transform<float> trans;
      trans.scale = Vector<3, float>({scales[i][0], -scales[i][2], scales[i][1]});
      //trans.scale = scales[i];

      transforms[time] = trans;
      
    }
    
  }
  
}

void updateLoadedRotations(std::unordered_map<float, Transform<float>> & transforms, std::vector<float> & times, std::vector<Math::Quaternion<float>> & rotations) {

  static Quaternion<float> rx90 = Math::Quaternion<float>(0.707106781, 0.707106781, 0, 0);
  static Quaternion<float> mrx90 = Math::Quaternion<float>(0.707106781, -0.707106781, 0, 0);

      if (times.size() != rotations.size()) {
    throw dbg::trace_exception("Timestamp and rotation count do not match in animation");
  }

  for (unsigned int i = 0; i < times.size(); ++i) {

    float time = times[i];

    if (transforms.find(time) != transforms.end()) {

      transforms[time].rotation = Quaternion<float>(rotations[i].a, rotations[i].b, -rotations[i].d, rotations[i].c);
      
    } else {

      /// TODO : Update to interpolate nearest keyframes for other chanels.
      
      Transform<float> trans;
      trans.rotation = Quaternion<float>(rotations[i].a, rotations[i].b, -rotations[i].d, rotations[i].c);

      transforms[time] = trans;
      
    }
    
  }
  
}

void insertKeyframe(std::vector<Keyframe> & keyframes, const Keyframe & frame) {

  size_t n = keyframes.size();
  if (n == 0) {
    keyframes.push_back(frame);
    return;
  }

  size_t klo = 0, khi = n-1;

  while ((khi - klo) > 1) {
    
    size_t piv = (khi + klo) / 2;
    double pivTime = keyframes[piv].time;
    
    if (pivTime < frame.time) {
      klo = piv;
    } else {
      khi = piv;
    }
    
  }
  
  keyframes.insert(keyframes.begin() + klo, frame);
  
}

std::shared_ptr<Animation> gltfLoadAnimation(const gltf_animation_t & anim, const int nodeIndex, gltf_file_data_t & fileData) {

  std::unordered_map<std::string, gltf_animation_sampler_t> samplersByChanelName;

  for (const gltf_animation_channel_t & chanel : anim.channels) {

    samplersByChanelName[chanel.path] = anim.samplers[chanel.sampler];
    
  }

  std::unordered_map<float, Transform<float>> loadedTransforms;

  for (auto it = samplersByChanelName.begin(); it != samplersByChanelName.end(); ++it) {


    const gltf_accessor_t & timeAcc = fileData.accessors[it->second.input];
    std::vector<float> timestamps = gltfLoadFloatBuffer(timeAcc, fileData.bufferViews[timeAcc.bufferView], fileData.binaryBuffer);

    const gltf_accessor_t & dataAcc = fileData.accessors[it->second.output];
    
    if (it->first == "translation") {

      std::vector<Math::Vector<3, float>> positions = gltfLoadVecBuffer<3, float>(dataAcc, fileData.bufferViews[dataAcc.bufferView], fileData.binaryBuffer);
      updateLoadedPositions(loadedTransforms, timestamps, positions);
      
      
    } else if (it->first == "rotation") {

      std::vector<Math::Quaternion<float>> rotations = gltfLoadQuaternionBuffer<float>(dataAcc, fileData.bufferViews[dataAcc.bufferView], fileData.binaryBuffer);
      updateLoadedRotations(loadedTransforms, timestamps, rotations);

    } else if (it->first == "scale") {
      
      std::vector<Math::Vector<3, float>> scales = gltfLoadVecBuffer<3, float>(dataAcc, fileData.bufferViews[dataAcc.bufferView], fileData.binaryBuffer);
      updateLoadedScales(loadedTransforms, timestamps, scales);
      
    }
    
    
  }

  std::vector<Keyframe> keyframes;

  static Transform<double> zUpTransform = Transform<double>(Vector<3>({0,0,0}), Quaternion<double>(sin(M_PI/4), cos(M_PI/4), 0, 0));

  for (auto it = loadedTransforms.begin(); it != loadedTransforms.end(); ++it) {

    Keyframe frame;
    frame.time = (double) it->first;
    frame.transform = convertTransform<float, double>(it->second);
    
    insertKeyframe(keyframes, frame);
  }

  return std::make_shared<Animation>(keyframes);
  
}

void gltfLinkAnimations(std::shared_ptr<NodeUploader> node, const int nodeIndex, gltf_file_data_t & fileData) {

  std::list<gltf_animation_t> animations;

  /// Get animations for this object
  for (const gltf_animation_t & anim : fileData.animations) {

    for (const gltf_animation_channel_t & chanel : anim.channels) {

      if (chanel.node == nodeIndex) {
	animations.push_back(anim);
	break;
      }
      
    }
    
  }

  std::cout << "Node " << node->getNodeName() << " has " << animations.size() << " animations." << std::endl;

  for (const gltf_animation_t & anim : animations) {

    std::shared_ptr<Animation> animation = gltfLoadAnimation(anim, nodeIndex, fileData);
    node->addAnimation(anim.name, animation);
    
  }
  
}

std::shared_ptr<NodeUploader> GLTFNodeLoader::loadNodeGLTF(gltf_file_data_t & fileData, const int nodeId, const std::string filename) {

  lout << "Loading gltf-node " << nodeId << std::endl;

  // Fetch node
  gltf_node_t & node = fileData.nodes[nodeId];

  // create transform

  Transform<float> tmptrans;
  tmptrans.position = node.translation;
  tmptrans.rotation = node.rotation;
  tmptrans.scale = node.scale;

  Transform<double> trans = convertTransform<float, double>(tmptrans);

  std::shared_ptr<NodeUploader> uploader = nullptr;

  /// Load Node mesh if present
  if (node.mesh >= 0) {

    lout << "Node " << nodeId << " has a mesh: " << node.mesh << std::endl;

    gltf_mesh_t gltfMesh = fileData.meshes[node.mesh];

    std::shared_ptr<Mesh> mesh = gltfLoadMesh(gltfMesh, fileData.accessors, fileData.bufferViews, fileData.binaryBuffer);
    LoadingResource meshRes = this->uploadResource(ResourceLocation("Mesh", filename, gltfMesh.name), std::shared_ptr<ResourceUploader<Resource>>((ResourceUploader<Resource> *)new MeshUploader(mesh)));
    lout << "MeshRes " << meshRes << std::endl;

    int materialIndex = gltfMesh.primitives[0].material;
    lout << "Material index " << materialIndex << " materials: " << fileData.materials.size() << std::endl;

    if (materialIndex >= 0) {

      LoadingResource matRes = this->loadMaterial(fileData, materialIndex, filename);
      uploader = std::shared_ptr<NodeUploader>(new MeshNodeUploader(node.name, meshRes, matRes, trans));
      uploadResource(ResourceLocation("Node",filename, node.name) , uploader);

    } else {

      LoadingResource matRes = this->loadDependency(ResourceLocation("Material", "resources/materials/gltf_default.mat"));
      uploader = std::make_shared<MeshNodeUploader>(node.name, meshRes, matRes, trans);
      uploadResource(ResourceLocation("Node",filename, node.name), uploader);

    }


  } else {
    std::shared_ptr<strc::Node> snode = std::make_shared<strc::Node>(node.name, trans);
    uploader = std::make_shared<NodeUploader>(snode);
  }

  /// Load node children
  for (const int id : node.children) {

    std::shared_ptr<NodeUploader> childUploader = loadNodeGLTF(fileData, id, filename);
    LoadingResource child = uploadResource(ResourceLocation("Node", filename, childUploader->getNodeName()), childUploader);
    uploader->addChild(child);

  }

  /// Look for animations for this node.

  gltfLinkAnimations(uploader, nodeId, fileData);

  return uploader;

}

/*std::shared_ptr<ResourceUploader<strc::Node>> GLTFNodeLoader::loadResource(std::string fname) {

  lout << "Loading gltf-data as Node" << std::endl;

  gltf_file_data_t fileData;
  std::vector<std::shared_ptr<GLTFNode>> nodes = gltfLoadFile(fname, &fileData);

  lout << "GLTF-Data is loaded" << std::endl;

  gltf_scene_t scene = fileData.scenes[fileData.rootScene];
  std::shared_ptr<strc::Node> sceneNode(new strc::Node(""));

  std::shared_ptr<NodeUploader> sceneUploader = std::make_shared<NodeUploader>(sceneNode);

  for (int nodeId : scene.nodes) {

    std::shared_ptr<NodeUploader> child = loadNodeGLTF(fileData, nodeId, fname);
    LoadingResource res = scheduleSubresource(ResourceLocation("Node", fname, child->getNodeName()), child);
    sceneUploader->addChild(res);

  }

  lout << "Loaded GLTF-Node successfully" << std::endl;

  return sceneUploader;

  }*/

bool GLTFNodeLoader::canLoad(ResourceLocation location) {

  /// Assume file is correct.
  return true;
  
}

LoadingResource GLTFNodeLoader::load(ResourceLocation location) {

  lout << "Loading gltf-data as Node" << std::endl;

  gltf_file_data_t fileData;
  std::vector<std::shared_ptr<GLTFNode>> nodes = gltfLoadFile(location.filename, &fileData);

  lout << "GLTF-Data is loaded" << std::endl;

  gltf_scene_t scene = fileData.scenes[fileData.rootScene];
  std::shared_ptr<strc::Node> sceneNode(new strc::Node(""));

  std::shared_ptr<NodeUploader> sceneUploader = std::make_shared<NodeUploader>(sceneNode);

  for (int nodeId : scene.nodes) {

    std::shared_ptr<NodeUploader> child = loadNodeGLTF(fileData, nodeId, location.filename);
    LoadingResource res = uploadResource(ResourceLocation("Node", location.filename, child->getNodeName()), child);
    sceneUploader->addChild(res);

  }

  lout << "Loaded GLTF-Node successfully" << std::endl;

  return uploadResource(ResourceLocation(location.type, location.filename), sceneUploader);
  
}

using namespace Math;

GLTFNode::GLTFNode() {

  this->name = "";
  this->position = Vector<3, float>();
  this->scale = Vector<3, float>();
  this->rotation = Quaternion<float>(1,0,0,0);
  this->mesh = nullptr;

}

GLTFNode::~GLTFNode() {

}

Quaternion<float> GLTFNode::getRotation() {
  return rotation;
}

Vector<3,float> GLTFNode::getPosition() {
  return position;
}

Vector<3,float> GLTFNode::getScale() {
  return scale;
}

void GLTFNode::setPosition(Math::Vector<3, float> pos) {
  position = pos;
}

void GLTFNode::setRotation(Math::Quaternion<float> rot) {
  rotation = rot;
}

void GLTFNode::setScale(Math::Vector<3, float> scale) {
  this->scale = scale;
}

void GLTFNode::addChild(std::shared_ptr<GLTFNode> c) {
  this->children.push_back(c);
}

std::vector<std::shared_ptr<GLTFNode>> & GLTFNode::getChildren() {
  return children;
}

void GLTFNode::setName(std::string name) {
  this->name = name;
}

std::string GLTFNode::getName() {
  return name;
}

void GLTFNode::setMesh(std::shared_ptr<Mesh> mesh) {
  this->mesh = mesh;
}

std::shared_ptr<Mesh> GLTFNode::getMesh() {
  return mesh;
}

bool GLTFNode::hasMesh() {
  return mesh != nullptr;
}

bool GLTFNode::hasChildren() {
  return children.size();
}

Matrix<4,4,float> GLTFNode::getTransform() {

  Matrix<4,4,float> smat(scale[0]);
  Matrix<4,4,float> tMat = this->rotation.toModelMatrix(position);

  return tMat * smat;

}

std::shared_ptr<Skin> GLTFNode::getSkin() {
  return skin;
}

void GLTFNode::setSkin(std::shared_ptr<Skin> skin) {
  this->skin = skin;
}
