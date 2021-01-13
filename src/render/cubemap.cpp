#include "render/cubemap.h"

#include <configloading.h>

CubeMap::CubeMap(vkutil::VulkanState & state, std::vector<uint8_t> data, int width, int height, int depth) : Texture(state, data, width, height, depth, VK_IMAGE_VIEW_TYPE_CUBE, 6, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) {

}

CubeMap::~CubeMap() {

}

#include "util/image/png.h"

std::vector<uint8_t> loadPNGasVector(std::string fname, uint32_t * width, uint32_t * height) {

  FILE * file = fopen(fname.c_str(), "r");
  uint32_t chanelCount;
  uint8_t * data = pngLoadImageData(file, width, height, &chanelCount);
  fclose(file);

  std::vector<uint8_t> res((*width) * (*height) *4);
  if (chanelCount >= 4) {
    for (uint32_t i = 0; i < (*width) * (*height) * 4; ++i) {
      res[i] = data[i];
    }
  } else {
    for (uint32_t i = 0; i < (*width) * *(height); ++i) {
      res[i*4] = data[i*3];
      res[i*4+1] = data[i*3+1];
      res[i*4+2] = data[i*3+2];
      res[i*4+3] = 255;
    }
  }

  free(data);

  return res;
  
}

class CubeMapUploader : public ResourceUploader<Texture> {

public:

  CubeMapUploader(std::vector<uint8_t> data, int width, int height, int depth) :
    data(data), width(width), height(height), depth(depth) {

  }
  
  bool uploadReady() {
    return true;
  }

  std::shared_ptr<Texture> uploadResource(vkutil::VulkanState & state) {
    return std::make_shared<CubeMap>(state, data, width, height, depth);
  }

private:

  int width;
  int height;
  int depth;
  std::vector<uint8_t> data;
  
};

std::shared_ptr<ResourceUploader<Texture>> CubeMapLoader::loadResource(std::string filename) {

  using namespace config;
  std::shared_ptr<NodeCompound> root = config::parseFile(filename);

  uint32_t width, height;

  std::vector<uint8_t> front = loadPNGasVector(std::string(root->getNode<char>("pos_y")->getRawData()), &width, &height);
  std::vector<uint8_t> back  = loadPNGasVector(std::string(root->getNode<char>("neg_y")->getRawData()), &width, &height);

  std::vector<uint8_t> up    = loadPNGasVector(std::string(root->getNode<char>("pos_z")->getRawData()), &width, &height);
  std::vector<uint8_t> down  = loadPNGasVector(std::string(root->getNode<char>("neg_z")->getRawData()), &width, &height);

  std::vector<uint8_t> right = loadPNGasVector(std::string(root->getNode<char>("pos_x")->getRawData()), &width, &height);
  std::vector<uint8_t> left  = loadPNGasVector(std::string(root->getNode<char>("neg_x")->getRawData()), &width, &height);

  std::vector<uint8_t> data;
  data.insert(data.end(), right.begin(), right.end());
  data.insert(data.end(), left.begin(), left.end());

  data.insert(data.end(), up.begin(), up.end());
  data.insert(data.end(), down.begin(), down.end());

  data.insert(data.end(), front.begin(), front.end());
  data.insert(data.end(), back.begin(), back.end());

  return std::make_shared<CubeMapUploader>(data, width, height, 1);
  
}
