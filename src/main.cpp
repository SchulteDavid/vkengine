#include <iostream>
#include <memory>

#include <GLFW/glfw3.h>

#include <vk_mem_alloc.h>

#include "render/util/vkutil.h"
#include "render/window.h"
#include "render/viewport.h"

int main(int argc, char ** argv) {


    std::shared_ptr<Window> window(new Window());

    Viewport * view = new Viewport(window);

    std::shared_ptr<Shader> shader(new Shader("shaders/vertex.vert.spirv", "shaders/fragment.frag.spirv", window->getDevice()));
    std::shared_ptr<Model> model(Model::loadFromFile(window->getState(), "cube.ply"));
    std::shared_ptr<Texture> albedo(Texture::createTexture(window->getState(), "test.tga"));
    std::shared_ptr<Texture> normal(Texture::createTexture(window->getState(), "normals.tga"));

    std::vector<std::shared_ptr<Texture>> textures = {albedo, normal};

    VkSampler sampler = Texture::createSampler(window->getState(), 3);

    vkutil::VertexInputDescriptions descs;
    descs.binding = Model::Vertex::getBindingDescription();
    descs.attributes = Model::Vertex::getAttributeDescriptions();

    shader->setupDescriptorSetLayout(sampler, textures.size());
    shader->setupGraphicsPipeline(descs, view->getRenderpass(), window->getState(), view->getSwapchainExtent());

    RenderElement::Transform trans;
    trans.position = glm::vec3(0, 10, 0);
    trans.qRot = Math::Quaternion<float>();
    trans.scale = 1.0;

    std::shared_ptr<RenderElement> e(new RenderElement(view, model, shader, textures, view->getSwapchainSize(), trans));


    while (!glfwWindowShouldClose(window->getGlfwWindow())) {

        view->drawFrame();

    }

    return 0;

}
