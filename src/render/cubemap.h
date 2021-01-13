#ifndef _CUBEMAP_H
#define _CUBEMAP_H

#include <vector>
#include "render/util/vkutil.h"
#include "render/texture.h"

class CubeMap : public Texture {

public:
  CubeMap(vkutil::VulkanState & state, std::vector<uint8_t> data, int width, int height, int depth);
  virtual ~CubeMap();
  
};

#endif
