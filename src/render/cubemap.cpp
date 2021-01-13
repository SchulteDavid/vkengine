#include "render/cubemap.h"

CubeMap::CubeMap(vkutil::VulkanState & state, std::vector<uint8_t> data, int width, int height, int depth) : Texture(state, data, width, height, depth, VK_IMAGE_VIEW_TYPE_CUBE, 6, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) {

}

CubeMap::~CubeMap() {

}
