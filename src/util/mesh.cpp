#include "mesh.h"

#include <unordered_set>

#include <ply.hpp>

#include "meshhelper.h"

using namespace Math;

Mesh::Mesh(std::vector<Model::Vertex> verts, std::vector<uint16_t> indices) {

  this->verts = verts;
  this->indices = std::vector<uint32_t>(indices.size());
  for (unsigned int i = 0; i < indices.size(); ++i) {
    this->indices[i] = indices[i];
  }


  VertexAttribute positionAttr;
  positionAttr.type = ATTRIBUTE_F32_VEC3;
  positionAttr.value = std::vector<VertexAttribute::VertexAttributeData>(verts.size());

  VertexAttribute normalAttr;
  normalAttr.type = ATTRIBUTE_F32_VEC3;
  normalAttr.value = std::vector<VertexAttribute::VertexAttributeData>(verts.size());

  VertexAttribute uvAttr;
  uvAttr.type = ATTRIBUTE_F32_VEC2;
  uvAttr.value = std::vector<VertexAttribute::VertexAttributeData>(verts.size());

  VertexAttribute tangentAttr;
  tangentAttr.type = ATTRIBUTE_F32_VEC3;
  tangentAttr.value = std::vector<VertexAttribute::VertexAttributeData>(verts.size());

  VertexAttribute matAttr;
  matAttr.type = ATTRIBUTE_I32_SCALAR;
  matAttr.value = std::vector<VertexAttribute::VertexAttributeData>(verts.size());

  for (unsigned int i = 0; i < verts.size(); ++i) {

    float tmp[3] = {verts[i].pos.x, verts[i].pos.y, verts[i].pos.z};
    positionAttr.value[i].vec3 = Vector<3, float>(tmp);

    tmp[0] = verts[i].normal.x;
    tmp[1] = verts[i].normal.y;
    tmp[2] = verts[i].normal.z;
    normalAttr.value[i].vec3 = Vector<3, float>(tmp);

    tmp[0] = verts[i].tangent.x;
    tmp[1] = verts[i].tangent.y;
    tmp[2] = verts[i].tangent.z;
    tangentAttr.value[i].vec3 = Vector<3, float>(tmp);

    tmp[0] = verts[i].uv.x;
    tmp[1] = verts[i].uv.y;
    uvAttr.value[i].vec2 = Vector<2, float>(tmp);

    matAttr.value[i].i32 = verts[i].matIndex;

  }

  this->attributes["POSITION"] = positionAttr;
  this->attributes["NORMAL"] = normalAttr;
  this->attributes["TEXCOORD_0"] = uvAttr;
  this->attributes["TANGENT"] = tangentAttr;
  this->attributes["MATERIAL_INDEX"] = matAttr;

  for (auto it : attributes) {

    logger(std::cout) << it.first << " -> " << it.second.type << std::endl;

  }

}

const VertexAttributeType Mesh::getAttributeType(std::string name) {

  if (this->attributes.find(name) == this->attributes.end())
    throw dbg::trace_exception("No such attribute");

  return attributes[name].type;

}

Mesh::Mesh(std::unordered_map<std::string, VertexAttribute> attributes, std::vector<uint16_t> indices) {

  this->attributes = attributes;
  this->indices = std::vector<uint32_t>(indices.size());
  for (unsigned int i = 0; i < indices.size(); ++i) {
    this->indices[i] = indices[i];
  }

  if (this->attributes.find("MATERIAL_INDEX") == this->attributes.end()) {

    VertexAttribute matAttr;
    matAttr.type = ATTRIBUTE_I32_SCALAR;
    matAttr.value = std::vector<VertexAttribute::VertexAttributeData>(this->attributes["POSITION"].value.size());

    for (unsigned int i = 0; i < this->attributes["POSITION"].value.size(); ++i) {
      matAttr.value[i].i32 = 0;
    }

    this->attributes["MATERIAL_INDEX"] = matAttr;

  }

  if (this->attributes.find("TANGENT") == this->attributes.end()) {

    computeTangents();

  }

}

Mesh::Mesh(std::vector<Model::Vertex> verts, std::vector<uint32_t> indices) {

  this->verts = verts;
  this->indices = indices;


  VertexAttribute positionAttr;
  positionAttr.type = ATTRIBUTE_F32_VEC3;
  positionAttr.value = std::vector<VertexAttribute::VertexAttributeData>(verts.size());

  VertexAttribute normalAttr;
  normalAttr.type = ATTRIBUTE_F32_VEC3;
  normalAttr.value = std::vector<VertexAttribute::VertexAttributeData>(verts.size());

  VertexAttribute uvAttr;
  uvAttr.type = ATTRIBUTE_F32_VEC2;
  uvAttr.value = std::vector<VertexAttribute::VertexAttributeData>(verts.size());

  VertexAttribute tangentAttr;
  tangentAttr.type = ATTRIBUTE_F32_VEC3;
  tangentAttr.value = std::vector<VertexAttribute::VertexAttributeData>(verts.size());

  VertexAttribute matAttr;
  matAttr.type = ATTRIBUTE_I32_SCALAR;
  matAttr.value = std::vector<VertexAttribute::VertexAttributeData>(verts.size());

  for (unsigned int i = 0; i < verts.size(); ++i) {

    float tmp[3] = {verts[i].pos.x, verts[i].pos.y, verts[i].pos.z};
    positionAttr.value[i].vec3 = Vector<3, float>(tmp);

    tmp[0] = verts[i].normal.x;
    tmp[1] = verts[i].normal.y;
    tmp[2] = verts[i].normal.z;
    normalAttr.value[i].vec3 = Vector<3, float>(tmp);

    tmp[0] = verts[i].tangent.x;
    tmp[1] = verts[i].tangent.y;
    tmp[2] = verts[i].tangent.z;
    tangentAttr.value[i].vec3 = Vector<3, float>(tmp);

    tmp[0] = verts[i].uv.x;
    tmp[1] = verts[i].uv.y;
    uvAttr.value[i].vec2 = Vector<2, float>(tmp);

    matAttr.value[i].i32 = verts[i].matIndex;

  }

  this->attributes["POSITION"] = positionAttr;
  this->attributes["NORMAL"] = normalAttr;
  this->attributes["TEXCOORD_0"] = uvAttr;
  this->attributes["TANGENT"] = tangentAttr;
  this->attributes["MATERIAL_INDEX"] = matAttr;

  for (auto it : attributes) {

    logger(std::cout) << it.first << " -> " << it.second.type << std::endl;

  }

}

Mesh::Mesh(std::unordered_map<std::string, VertexAttribute> attributes, std::vector<uint32_t> indices) {

  this->attributes = attributes;
  this->indices = indices;

  if (this->attributes.find("MATERIAL_INDEX") == this->attributes.end()) {

    VertexAttribute matAttr;
    matAttr.type = ATTRIBUTE_I32_SCALAR;
    matAttr.value = std::vector<VertexAttribute::VertexAttributeData>(this->attributes["POSITION"].value.size());

    for (unsigned int i = 0; i < this->attributes["POSITION"].value.size(); ++i) {
      matAttr.value[i].i32 = 0;
    }

    this->attributes["MATERIAL_INDEX"] = matAttr;

  }

  if (this->attributes.find("TANGENT") == this->attributes.end()) {

    computeTangents();

  }

}

Mesh::~Mesh() {



}

void Mesh::computeTangents() {

  std::vector<Model::Vertex> verts = getVerts();
  MeshHelper::computeTangents(verts, indices);

  VertexAttribute tangentAttr;
  tangentAttr.type = ATTRIBUTE_F32_VEC3;
  tangentAttr.value = std::vector<VertexAttribute::VertexAttributeData>(this->attributes["POSITION"].value.size());

  for (unsigned int i = 0; i < this->attributes["POSITION"].value.size(); ++i) {

    float tmp[3] = {verts[i].tangent.x, verts[i].tangent.y, verts[i].tangent.z};

    tangentAttr.value[i].vec3 = Vector<3, float>(tmp);
  }

  this->attributes["TANGENT"] = tangentAttr;

}

unsigned int Mesh::getVertexCount() {

  return this->attributes["POSITION"].value.size();

}

std::vector<Model::Vertex> Mesh::getVerts() {

  std::vector<Model::Vertex> verts(this->attributes["POSITION"].value.size());

  bool hasTangents = attributes.find("TANGENT") != attributes.end();

  for (unsigned int i = 0; i < this->attributes["POSITION"].value.size(); ++i) {

    verts[i].pos = glm::vec3(attributes["POSITION"].value[i].vec3[0], attributes["POSITION"].value[i].vec3[1], attributes["POSITION"].value[i].vec3[2]);
    verts[i].normal = glm::vec3(attributes["NORMAL"].value[i].vec3[0], attributes["NORMAL"].value[i].vec3[1], attributes["NORMAL"].value[i].vec3[2]);
    if (hasTangents)
      verts[i].tangent = glm::vec3(attributes["TANGENT"].value[i].vec3[0], attributes["TANGENT"].value[i].vec3[1], attributes["TANGENT"].value[i].vec3[2]);
    verts[i].uv = glm::vec2(attributes["TEXCOORD_0"].value[i].vec2[0], attributes["TEXCOORD_0"].value[i].vec2[1]);
    verts[i].matIndex = attributes["MATERIAL_INDEX"].value[i].i32;

  }

  if (!hasTangents)
    MeshHelper::computeTangents(verts, indices);

  return verts;

}

std::vector<uint32_t> & Mesh::getIndices() {
  return indices;
}

std::shared_ptr<Mesh> Mesh::withTransform(std::shared_ptr<Mesh> mesh, Math::Matrix<4,4,float> m) {

  using namespace Math;

  //std::vector<Model::Vertex> newVerts;
  std::unordered_map<std::string, VertexAttribute> newAttributes;

  for (auto it : mesh->attributes) {

    std::vector<VertexAttribute::VertexAttributeData> data(it.second.value.size());
    for (unsigned int i = 0; i < it.second.value.size(); ++i) {
      data[i] = it.second.value[i];
    }

    VertexAttribute attr;
    attr.type = it.second.type;
    attr.value = data;

    newAttributes[it.first] = attr;

  }

  for (unsigned int i = 0; i < mesh->attributes["POSITION"].value.size(); ++i) {

    Vector<3, float> pos = mesh->attributes["POSITION"].value[i].vec3;

    Vector<4, float> p(pos[0], pos[1], pos[2], 1);
    Vector<4, float> n(mesh->attributes["NORMAL"].value[i].vec3);

    Vector<4,float> p2 = m * p;
    n = m * n;

    //Model::Vertex nVert;

    //nVert.pos = glm::vec3(p[0] / p[3], p[1] / p[3], p[2] / p[3]);
    newAttributes["POSITION"].value[i].vec3 = Vector<3, float>(p2) / p2[3];
    newAttributes["NORMAL"].value[i].vec3 = Vector<3, float>(n);

    if (mesh->attributes.find("TANGENT") != mesh->attributes.end()) {

      Vector<4, float> t(mesh->attributes["TANGENT"].value[i].vec3);
      t = m * t;
      newAttributes["TANGENT"].value[i].vec3 = Vector<3, float>(t);

    }
    //nVert.normal = glm::vec3(n[0], n[1], n[2]);
    //nVert.tangent = glm::vec3(t[0], t[1], t[2]);
    //nVert.uv = v.uv;
    //nVert.matIndex = v.matIndex;

    //newVerts.push_back(nVert);

  }

  return std::shared_ptr<Mesh>(new Mesh(newAttributes, mesh->indices));

}

void Mesh::setMaterialIndex(int32_t index) {
  /*for (unsigned int i = 0; i < verts.size(); ++i) {
    verts[i].matIndex = index;
    }*/

  if (attributes.find("MATERIAL_INDEX") == attributes.end()) {

    VertexAttribute matAttr;
    matAttr.type = ATTRIBUTE_I32_SCALAR;
    matAttr.value = std::vector<VertexAttribute::VertexAttributeData>(attributes["POSITION"].value.size());
    attributes["MATERIAL_INDEX"] = matAttr;

  }

  for (unsigned int i = 0; i < attributes["MATERIAL_INDEX"].value.size(); ++i) {

    attributes["MATERIAL_INDEX"].value[i].i32 = index;

  }

}

std::shared_ptr<Mesh> operator*(Math::Matrix<4,4,float> m, std::shared_ptr<Mesh> mesh) {

  return Mesh::withTransform(mesh, m);

}

const VertexAttribute & Mesh::getAttribute(std::string name) {

  if (this->attributes.find(name) == this->attributes.end())
    throw dbg::trace_exception("No such attribute");

  return attributes[name];

}

void insertFloatInBuffer(std::vector<VertexAttribute::VertexAttributeData> & fData, std::vector<uint8_t> & buffer, uint32_t offset, uint32_t stride) {

  uint8_t * data = buffer.data();

  for (unsigned int i = 0; i < fData.size(); ++i) {

    *((float *) (data + (i * stride + offset))) = fData[i].f;

  }

}

void insertIntInBuffer(std::vector<VertexAttribute::VertexAttributeData> & fData, std::vector<uint8_t> & buffer, uint32_t offset, uint32_t stride) {

  uint8_t * data = buffer.data();

  for (unsigned int i = 0; i < fData.size(); ++i) {

    *((int32_t *) (data + (i * stride + offset))) = fData[i].i32;

  }

}

void insertVec2InBuffer(std::vector<VertexAttribute::VertexAttributeData> & fData, std::vector<uint8_t> & buffer, uint32_t offset, uint32_t stride) {

  uint8_t * data = buffer.data();

  for (unsigned int i = 0; i < fData.size(); ++i) {

    *((float *) (data + (i * stride + offset))) = fData[i].vec2[0];
    *((float *) (data + (i * stride + offset+1*sizeof(float)))) = fData[i].vec2[1];

  }

}

void insertVec3InBuffer(std::vector<VertexAttribute::VertexAttributeData> & fData, std::vector<uint8_t> & buffer, uint32_t offset, uint32_t stride) {

  uint8_t * data = buffer.data();

  for (unsigned int i = 0; i < fData.size(); ++i) {

    *((float *) (data + (i * stride + offset)))   = fData[i].vec3[0];
    *((float *) (data + (i * stride + offset+1*sizeof(float)))) = fData[i].vec3[1];
    *((float *) (data + (i * stride + offset+2*sizeof(float)))) = fData[i].vec3[2];

  }

}

void insertVec4InBuffer(std::vector<VertexAttribute::VertexAttributeData> & fData, std::vector<uint8_t> & buffer, uint32_t offset, uint32_t stride) {

  uint8_t * data = buffer.data();

  for (unsigned int i = 0; i < fData.size(); ++i) {

    *((float *) (data + (i * stride + offset)))   = fData[i].vec4[0];
    *((float *) (data + (i * stride + offset+1*sizeof(float)))) = fData[i].vec4[1];
    *((float *) (data + (i * stride + offset+2*sizeof(float)))) = fData[i].vec4[2];
    *((float *) (data + (i * stride + offset+3*sizeof(float)))) = fData[i].vec4[3];

  }

}

template <unsigned int dim, typename T> void insertVecInBuffer(std::vector<Vector<dim, T>> & fData, std::vector<uint8_t> & buffer, uint32_t offset, uint32_t stride) {

  uint8_t * data = buffer.data();

  for (unsigned int i = 0; i < fData.size(); ++i) {

    for (unsigned int j = 0; j < dim; ++j)
      *((T *) (data + (i * stride + offset+j*sizeof(T)))) = fData[i][j];

  }

}

std::vector<uint8_t> Mesh::getCompactIndices(uint32_t * indexSizeBytes, uint32_t * indexCount) {

  uint64_t vertexCount = this->attributes["POSITION"].value.size();

  if (vertexCount <= std::numeric_limits<uint16_t>::max()) {

    *indexSizeBytes = sizeof(uint16_t);
    *indexCount = indices.size();

    std::vector<uint8_t> indexData((sizeof(uint16_t) / sizeof(uint8_t)) * indices.size());
    uint16_t * data = (uint16_t *) indexData.data();

    for (unsigned int i = 0; i < indices.size(); ++i) {

      data[i] = indices[i];

    }

    return indexData;

  }

  *indexSizeBytes = sizeof(uint32_t);
  *indexCount = indices.size();

  std::vector<uint8_t> indexData((sizeof(uint32_t) / sizeof(uint8_t)) * indices.size());
  uint32_t * data = (uint32_t *) indexData.data();

  for (unsigned int i = 0; i < indices.size(); ++i) {

    data[i] = indices[i];

  }

  return indexData;

}

size_t getAttributeSize(VertexAttributeType type) {

  uint32_t multiplicity = type & 0xf;

  size_t elementSize = 0;

  switch (type >> 4) {

  case 1:
    elementSize = sizeof(float);
    break;

  case 2:
    elementSize = sizeof(uint8_t);
    break;

  case 3:
    elementSize = sizeof(uint16_t);
    break;

  case 4:
    elementSize = sizeof(int32_t);
    break;

  default:
    elementSize = 0;
    break;

  }


  return elementSize * multiplicity;

}

std::vector<InterleaveElement> Mesh::compactStorage(const std::vector<InputDescription> & iData, unsigned int * stride) {

  std::vector<InterleaveElement> elements(iData.size());

  *stride = 0;

  for (unsigned int i = 0; i < iData.size(); ++i) {
    elements[i].offset = 0;
  }

  for (const InputDescription & id : iData) {

    std::string name = id.attributeName;
    VertexAttributeType type = this->attributes[name].type;

    size_t s = getAttributeSize(type);

    *stride += s;

    for (unsigned int i = id.location+1; i < iData.size(); ++i) {

      elements[i].offset += s;

    }

    elements[id.location].attributeName = name;

  }

  return elements;

}

std::vector<uint8_t> Mesh::getInterleavedData(std::vector<InterleaveElement> elements, uint32_t stride) {

  //std::cout << stride << " " << attributes["POSITION"].value.size() << std::endl;
  if (!attributes["POSITION"].value.size()) {
    throw dbg::trace_exception("Creating mesh with no data");
  }

  std::vector<uint8_t> data(stride * attributes["POSITION"].value.size());

  for (InterleaveElement e : elements) {

    if (e.offset >= stride) {
      throw dbg::trace_exception("Offset >= stride, would override next vertex in buffer...");
    }

    if (attributes.find(e.attributeName) == attributes.end()) {
      throw dbg::trace_exception(std::string("No Such attribute name ").append(e.attributeName));
    }

    VertexAttribute attr = attributes[e.attributeName];

    //std::cout << "Writing to buffer: " << e.attributeName << " : " << e.offset << " type = " << std::hex << attr.type << std::dec << std::endl;

    switch (attr.type) {

    case ATTRIBUTE_F32_SCALAR:
      insertFloatInBuffer(attr.value, data, e.offset, stride);
      break;

    case ATTRIBUTE_F32_VEC2:
      {
	std::vector<Vector<2,float>> tmpData(attr.value.size());
	for (unsigned int i = 0; i < attr.value.size(); ++i) {
	  tmpData[i] = attr.value[i].vec2;
	}
	insertVecInBuffer<2, float>(tmpData, data, e.offset, stride);
      }
      break;

    case ATTRIBUTE_F32_VEC3:
      {
	std::vector<Vector<3,float>> tmpData(attr.value.size());
	for (unsigned int i = 0; i < attr.value.size(); ++i) {
	  tmpData[i] = attr.value[i].vec3;
	}
	insertVecInBuffer<3, float>(tmpData, data, e.offset, stride);
      }
      break;

    case ATTRIBUTE_F32_VEC4:
      {
	std::vector<Vector<4,float>> tmpData(attr.value.size());
	for (unsigned int i = 0; i < attr.value.size(); ++i) {
	  tmpData[i] = attr.value[i].vec4;
	}
	insertVecInBuffer<4, float>(tmpData, data, e.offset, stride);
      }
      break;

    case ATTRIBUTE_I32_SCALAR:
      insertIntInBuffer(attr.value, data, e.offset, stride);
      break;

    case ATTRIBUTE_I32_VEC2:
      {
	std::vector<Vector<2,int32_t>> tmpData(attr.value.size());
	for (unsigned int i = 0; i < attr.value.size(); ++i) {
	  tmpData[i] = attr.value[i].i32_vec2;
	}
	insertVecInBuffer<2, int32_t>(tmpData, data, e.offset, stride);
      }
      break;

    case ATTRIBUTE_I32_VEC3:
      {
	std::vector<Vector<3,int32_t>> tmpData(attr.value.size());
	for (unsigned int i = 0; i < attr.value.size(); ++i) {
	  tmpData[i] = attr.value[i].i32_vec3;
	}
	insertVecInBuffer<3, int32_t>(tmpData, data, e.offset, stride);
      }
      break;

    case ATTRIBUTE_I32_VEC4:
      {
	std::vector<Vector<4,int32_t>> tmpData(attr.value.size());
	for (unsigned int i = 0; i < attr.value.size(); ++i) {
	  tmpData[i] = attr.value[i].i32_vec4;
	}
	insertVecInBuffer<4, int32_t>(tmpData, data, e.offset, stride);
      }
      break;

    case ATTRIBUTE_I16_VEC2:
      {
	std::vector<Vector<2,int16_t>> tmpData(attr.value.size());
	for (unsigned int i = 0; i < attr.value.size(); ++i) {
	  tmpData[i] = attr.value[i].i16_vec2;
	}
	insertVecInBuffer<2, int16_t>(tmpData, data, e.offset, stride);
      }
      break;

    case ATTRIBUTE_I16_VEC3:
      {
	std::vector<Vector<3,int16_t>> tmpData(attr.value.size());
	for (unsigned int i = 0; i < attr.value.size(); ++i) {
	  tmpData[i] = attr.value[i].i16_vec3;
	}
	insertVecInBuffer<3, int16_t>(tmpData, data, e.offset, stride);
      }
      break;

    case ATTRIBUTE_I16_VEC4:
      {
	std::vector<Vector<4,int16_t>> tmpData(attr.value.size());
	for (unsigned int i = 0; i < attr.value.size(); ++i) {
	  tmpData[i] = attr.value[i].i16_vec4;
	}
	insertVecInBuffer<4, int16_t>(tmpData, data, e.offset, stride);
      }
      break;

    default:
      throw dbg::trace_exception("Unknown attribute type");

    }

  }

  //std::cout << "dataSize: " << data.size() << " bytes" << std::endl;

  return data;

}

void Mesh::setAttribute(std::string name, VertexAttribute value) {
  this->attributes[name] = value;
}

void Mesh::saveAsPLY(std::string fname) {

  FILE * f = fopen(fname.c_str(), "w");

  fprintf(f, "ply\nformat ascii 1.0\n");

  fprintf(f, "element vertex %d\n", attributes["POSITION"].value.size());
  fprintf(f, "property float x\n");
  fprintf(f, "property float y\n");
  fprintf(f, "property float z\n");

  fprintf(f, "property float nx\n");
  fprintf(f, "property float ny\n");
  fprintf(f, "property float nz\n");

  fprintf(f, "property float s\n");
  fprintf(f, "property float t\n");

  fprintf(f, "element face %d\n", indices.size()/3);
  fprintf(f, "property list uchar uint vertex_indices\nend_header\n");

  for (unsigned int i = 0; i < attributes["POSITION"].value.size(); ++i) {

    fprintf(f, "%f %f %f %f %f %f %f %f\n", attributes["POSITION"].value[i].vec3[0], attributes["POSITION"].value[i].vec3[1], attributes["POSITION"].value[i].vec3[2],
	    attributes["NORMAL"].value[i].vec3[0], attributes["NORMAL"].value[i].vec3[1], attributes["NORMAL"].value[i].vec3[2],
	    attributes["TEXCOORD_0"].value[i].vec2[0], attributes["TEXCOORD_0"].value[i].vec2[1]);

  }

  for (unsigned int i = 0; i < indices.size() / 3; ++i) {

    fprintf(f, "3 %d %d %d\n", indices[i*3], indices[i*3+1], indices[i*3+2]);

  }


  fclose(f);

}

std::shared_ptr<Mesh> Mesh::loadFromFile(std::string fname) {

  PlyFile * plyFile = new PlyFile(fname);
  int indexCount;
  int * indexData = plyFile->getIndexData(&indexCount);

  int vertexCount;
  float * vertexData = plyFile->getVertexData(&vertexCount);

  std::vector<Model::Vertex> vertices(vertexCount);

  for (int i = 0; i < vertexCount; ++i) {

    vertices[i].pos = glm::vec3(vertexData[i*11], vertexData[i*11+1], vertexData[i*11+2]);
    vertices[i].normal = glm::vec3(vertexData[i*11+3], vertexData[i*11+4], vertexData[i*11+5]);
    vertices[i].uv = glm::vec2(vertexData[i*11+6], vertexData[i*11+7]);
    vertices[i].matIndex = 0;

  }

  std::vector<uint16_t> indices(indexCount);

  for (int i = 0; i < indexCount; ++i)
    indices[i] = indexData[i];

  std::cout << "Loaded mesh has " << vertexCount << " vertices" << std::endl;;

  return std::shared_ptr<Mesh>(new Mesh(vertices, indices));

}

/**
   Merges 2 meshes into 1. The resulting mesh has the attributes BOTH inputs have in common.
**/
std::shared_ptr<Mesh> Mesh::merge(std::shared_ptr<Mesh> m1, std::shared_ptr<Mesh> m2) {

  if (!m1) return m2;
  if (!m2) return m1;

  std::unordered_map<std::string, VertexAttribute> attributes;
  std::unordered_set<std::string> attributeNames;
  for (auto it : m1->attributes) {
    attributeNames.insert(attributeNames.begin(), it.first);
  }

  for (auto it : m2->attributes) {
    attributeNames.insert(attributeNames.begin(), it.first);
  }

  uint16_t m1Count;

  for (std::string name : attributeNames) {

    if (m1->attributes.find(name) == m1->attributes.end()) {
      continue;
    } else if (m2->attributes.find(name) == m2->attributes.end()) {
      continue;
    }

    if (m1->attributes[name].type != m2->attributes[name].type) {
      continue;
    }

    VertexAttribute attr;
    attr.type = m1->attributes[name].type;
    attr.value = std::vector<VertexAttribute::VertexAttributeData>(m1->attributes[name].value.size() + m2->attributes[name].value.size());

    for (unsigned int i = 0; i < m1->attributes[name].value.size(); ++i) {

      attr.value[i] = m1->attributes[name].value[i];

    }

    m1Count = m1->attributes[name].value.size();

    for (unsigned int i = 0; i < m2->attributes[name].value.size(); ++i) {

      attr.value[i + m1Count] = m2->attributes[name].value[i];

    }

    attributes[name] = attr;

  }

  //std::vector<Model::Vertex> verts;
  std::vector<uint16_t> indices;

  for (uint16_t i : m1->indices) {
    indices.push_back(i);
  }

  for (uint16_t i : m2->indices) {
    indices.push_back(i+m1Count);
  }

  return std::shared_ptr<Mesh>(new Mesh(attributes, indices));

}

MeshUploader::MeshUploader(std::shared_ptr<Mesh> m) {
  this->mesh = m;
}

bool MeshUploader::uploadReady() {
  return true;
}

std::shared_ptr<Mesh> MeshUploader::uploadResource() {
  return mesh;
}


std::shared_ptr<ResourceUploader<Mesh>> MeshLoader::loadResource(std::string fname) {
  std::shared_ptr<Mesh> mesh = Mesh::loadFromFile(fname);
  return std::shared_ptr<ResourceUploader<Mesh>>(new MeshUploader(mesh));
}
