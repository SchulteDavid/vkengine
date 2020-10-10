#ifndef MESH_H
#define MESH_H

#include <vector>
#include <memory>
#include <unordered_map>

#include "render/model.h"
#include <mathutils/matrix.h>

#include "resources/resource.h"
#include "resources/resourceuploader.h"

typedef enum VertexAttributeType {

				  ATTRIBUTE_NONE,

				  ATTRIBUTE_F32_SCALAR=0x11,
				  ATTRIBUTE_F32_VEC2,
				  ATTRIBUTE_F32_VEC3,
				  ATTRIBUTE_F32_VEC4,

				  ATTRIBUTE_I08_SCALAR=0x21,
				  ATTRIBUTE_I08_VEC2,
				  ATTRIBUTE_I08_VEC3,
				  ATTRIBUTE_I08_VEC4,

				  ATTRIBUTE_I16_SCALAR=0x31,
				  ATTRIBUTE_I16_VEC2,
				  ATTRIBUTE_I16_VEC3,
				  ATTRIBUTE_I16_VEC4,

				  ATTRIBUTE_I32_SCALAR=0x41,
				  ATTRIBUTE_I32_VEC2,
				  ATTRIBUTE_I32_VEC3,
				  ATTRIBUTE_I32_VEC4,


} VertexAttributeType ;

class VertexAttribute {

public:

  union VertexAttributeData {

    float f;
    Math::Vector<2, float> vec2;
    Math::Vector<3, float> vec3;
    Math::Vector<4, float> vec4;

    int8_t i8;
    Math::Vector<2, int8_t> i8_vec2;
    Math::Vector<3, int8_t> i8_vec3;
    Math::Vector<4, int8_t> i8_vec4;

    int8_t i16;
    Math::Vector<2, int16_t> i16_vec2;
    Math::Vector<3, int16_t> i16_vec3;
    Math::Vector<4, int16_t> i16_vec4;

    int32_t i32;
    Math::Vector<2, int32_t> i32_vec2;
    Math::Vector<3, int32_t> i32_vec3;
    Math::Vector<4, int32_t> i32_vec4;

    VertexAttributeData() {
    }

    ~VertexAttributeData(){
    }

    VertexAttributeData(const VertexAttributeData & d) {
      memcpy(this, &d, sizeof(VertexAttributeData));
    }

    VertexAttributeData & operator=(const VertexAttributeData & d) {

      memcpy(this, &d, sizeof(VertexAttributeData));
      return *this;

    }

  };

  VertexAttributeType type;
  std::vector<VertexAttributeData> value;

};

struct InterleaveElement {

  std::string attributeName;
  uint32_t offset;

};

struct InputDescription {

  std::string attributeName;
  uint32_t location;

};

class Mesh : public Resource {

public:

  Mesh(std::vector<Model::Vertex> verts, std::vector<uint32_t> indices);
  Mesh(std::unordered_map<std::string, VertexAttribute> attributes, std::vector<uint32_t> indices);
  Mesh(std::vector<Model::Vertex> verts, std::vector<uint16_t> indices);
  Mesh(std::unordered_map<std::string, VertexAttribute> attributes, std::vector<uint16_t> indices);
  virtual ~Mesh();

  std::vector<Model::Vertex> getVerts();
  std::vector<uint32_t> & getIndices();

  std::vector<uint8_t> getCompactIndices(uint32_t * indexSizeBytes, uint32_t * indexCount);

  const VertexAttribute & getAttribute(std::string name);
  const VertexAttributeType getAttributeType(std::string name);

  void setAttribute(std::string name, VertexAttribute value);

  void computeTangents();

  unsigned int getVertexCount();

  std::vector<uint8_t> getInterleavedData(std::vector<InterleaveElement> elements, uint32_t stride);

  void setMaterialIndex(int32_t index);

  void saveAsPLY(std::string fname);

  static std::shared_ptr<Mesh> loadFromFile(std::string fname);
  static std::shared_ptr<Mesh> withTransform(std::shared_ptr<Mesh> mesh, Math::Matrix<4,4,float> m);

  static std::shared_ptr<Mesh> merge(std::shared_ptr<Mesh> m1, std::shared_ptr<Mesh> m2);

  std::vector<InterleaveElement> compactStorage(const std::vector<InputDescription> & iData, unsigned int * stride);

protected:

private:

  std::vector<Model::Vertex> verts;
  std::vector<uint32_t> indices;

  std::unordered_map<std::string, VertexAttribute> attributes;

};

class MeshUploader : public ResourceUploader<Mesh> {

public:
  MeshUploader(std::shared_ptr<Mesh> mesh);

  std::shared_ptr<Mesh> uploadResource(vkutil::VulkanState & ) override;
  bool uploadReady();

private:

  std::shared_ptr<Mesh> mesh;

};

class MeshLoader : public ResourceLoader<Mesh> {

public:
  std::shared_ptr<ResourceUploader<Mesh>> loadResource(std::string fname);
  
};

std::shared_ptr<Mesh> operator*(Math::Matrix<4,4,float> m, std::shared_ptr<Mesh> mesh);

#endif // MESH_H
