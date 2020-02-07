#ifndef GLTF_H
#define GLTF_H

#include <string>

#include "render/util/vkutil.h"
#include "structure.h"

class GLTFLoader : public ResourceLoader<Structure> {

    public:
        GLTFLoader(vkutil::VulkanState & state, const VkRenderPass & renderPass, const VkExtent2D & swapChainExtent);

        std::shared_ptr<ResourceUploader<Structure>> loadResource(std::string fname);

    private:
        vkutil::VulkanState & state;
        const VkRenderPass & renderPass;
        const VkExtent2D & swapChainExtent;

};
//void gltfLoadFile(std::string fname);

#endif // GLTF_H
