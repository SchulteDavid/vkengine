#ifndef MODEL_H
#define MODEL_H

#include <glm/glm.hpp>

#include "indexbuffer.h"
#include "vertexbuffer.h"
#include <string>

#include "resources/resource.h"
#include "resources/resourceloader.h"

class Mesh;
struct InterleaveElement;

class Model : public Resource {

public:

  struct Vertex {

    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec2 uv;
    int32_t matIndex;

    static std::vector<VkVertexInputBindingDescription> getBindingDescription();
    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

  };

  enum Status {
	       STATUS_UNDEFINED,
	       STATUS_UPLOADED,
  };

  Model(const vkutil::VulkanState & state, std::vector<Vertex> & verts, std::vector<uint16_t> & indices);
  Model(const vkutil::VulkanState & state, std::shared_ptr<Mesh> mesh);
  Model(const vkutil::VulkanState & state, std::shared_ptr<Mesh> mesh, std::vector<InterleaveElement> elements, size_t elementSize);
  Model(const vkutil::VulkanState & state, std::shared_ptr<Mesh> mesh, std::vector<InterleaveElement> elements, size_t elementSize, bool isStatic);
  virtual ~Model();

  void uploadToGPU(const VkDevice & device, const VkCommandPool & commandPool, const vkutil::Queue & q);

  void bindForRender(VkCommandBuffer & cmdBuffer);
  int getIndexCount();

  virtual std::vector<VkVertexInputBindingDescription> getBindingDescription();
  virtual std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

  static Model * loadFromFile(const vkutil::VulkanState & state, std::string fname);

protected:

private:

  VertexBuffer<uint8_t> * vBuffer;
  IndexBuffer<uint8_t> * iBuffer;

  int vCount;
  int iCount;

  std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
  std::vector<VkVertexInputBindingDescription> bindingDescription;

  Status status;

};

class ModelUploader : public ResourceUploader<Model> {

public:
  ModelUploader(Model * model);

  std::shared_ptr<Model> uploadResource(vkutil::VulkanState & state, ResourceManager * manager);
  bool uploadReady();

private:

  Model * model;

};

class ModelLoader : public ResourceLoader<Model> {

public:

  ModelLoader(const vkutil::VulkanState & state);

  std::shared_ptr<ResourceUploader<Model>> loadResource(std::string fname);

private:

  const vkutil::VulkanState & state;

};

#endif // MODEL_H
