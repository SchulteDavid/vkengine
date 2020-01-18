#include <iostream>
#include <memory>
#include <thread>

#include <GLFW/glfw3.h>

#include <vk_mem_alloc.h>

#include "render/util/vkutil.h"
#include "render/window.h"
#include "render/viewport.h"

#include "resources/resourcemanager.h"

static bool run = true;
static bool wait;

void rotateFunc(std::shared_ptr<RenderElement> e, std::vector<RenderElement::Instance> & instances, std::vector<RenderElement::Transform> & transforms) {

    float rotAxis[3] = {0, 0, 1};

    while (run) {

        //auto startRenderTime = std::chrono::high_resolution_clock::now();

        for (unsigned int i = 0; i < 512; ++i) {

            Math::Quaternion<float> dr = Math::Quaternion<float>::fromAxisAngle(Math::Vector<3,float>(rotAxis), 0.01);
            transforms[i].qRot = transforms[i].qRot * dr;

            e->updateInstance(instances[i], transforms[i]);

        }

        /*double duration = std::chrono::duration<double, std::chrono::milliseconds::period>(std::chrono::high_resolution_clock::now() - startRenderTime).count();
        std::cout << "Compute time: " << duration << "ms" << std::endl;*/

        while (wait);
        wait = true;

    }

}

int main(int argc, char ** argv) {


    std::shared_ptr<Window> window(new Window());

    Camera * cam = new Camera(70.0, 0.1, 100.0, 1280.0/720.0, glm::vec3(0,0,0));

    Viewport * view = new Viewport(window, cam);

    ResourceManager * resourceManager = new ResourceManager(ResourceManager::RESOURCE_MODEL | ResourceManager::RESOURCE_SHADER);

    resourceManager->addLoader("Model", (ResourceLoader<Resource> *) new ModelLoader(view->getState()));

    /*std::shared_ptr<ResourceUploader<Model>> modelUploader = resourceManager->loadResource<Model>("Model", "cube.ply");
    Model * tmpModel = modelUploader->uploadResource();
    resourceManager->registerResource("Model", "cube.ply", tmpModel);*/

    resourceManager->startLoadingThreads(1);

    std::shared_ptr<ResourceManager::LoadingStatus> fres = resourceManager->loadResourceBg("Model", "cube.ply");


    std::shared_ptr<Shader> shader(new Shader("shaders/vertex.vert.spirv", "shaders/fragment.frag.spirv", window->getDevice()));
    //std::shared_ptr<Model> model(Model::loadFromFile(window->getState(), "cube.ply"));
    while (!fres->isLoaded) {
        //std::cout << fres->isLoaded << std::endl;
    }
    std::shared_ptr<Model> model = resourceManager->get<Model>("Model", "cube.ply");
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
    trans.qRot = Math::Quaternion<float>(1.0, 0.0, 0.0, 0.0);
    trans.scale = 1.0;

    std::shared_ptr<RenderElement> e(new RenderElement(view, model, shader, textures, view->getSwapchainSize(), trans));

    view->addRenderElement(e);

    cam->move(3, 0, 3);

    std::vector<RenderElement::Instance> instances;
    std::vector<RenderElement::Transform> transforms;

    //view->addLight(glm::vec4(0, 10, 3, 1.0), glm::vec4(3, 3, 3, 0));
    view->addLight(glm::vec4(1.0, -0.5, 0.85, 0.0), glm::vec4(2.0, 2.0, 2.0, 1.0));
    view->addLight(glm::vec4(0.2, 0.0, 1.0, 2.0), glm::vec4(0.0, 0.0, 1.0, 0.0));
    //view->addLight(glm::vec4(1.0, -1.0, 1.0, 2.0), glm::vec4(1.5, 1.5, 1.5, 1.2));

    view->addLight(glm::vec4(12.5, 12.5, 0.5, 2.0), glm::vec4(10, 10, 10, 0.0));

    for (unsigned int k = 0; k < 2; ++k) {

    for (unsigned int i = 0; i < 32; ++i) {
        for (unsigned int j = 0; j < 32; ++j) {

            RenderElement::Transform trans2;
            trans2.position = glm::vec3(2 * i, 2 * j, 2 * k);
            trans2.qRot = Math::Quaternion<float>(1.0, 0.0, 0.0, 0.0);
            trans2.scale = 0.5;

            RenderElement::Instance inst = e->addInstance(trans2);
            instances.push_back(inst);
            transforms.push_back(trans2);

        }
    }
    }

    resourceManager->joinLoadingThreads();

    //RenderElement::Instance rInstance = e->addInstance(trans);
    //RenderElement::Instance instance2 = e->addInstance(trans);

    double d = 0;

    std::thread rotateThread(rotateFunc, e, std::ref(instances), std::ref(transforms));

    while (!glfwWindowShouldClose(window->getGlfwWindow())) {

        glfwPollEvents();

        view->drawFrame();
        wait = false;

        d += 0.0001;

    }

    run = false;
    wait = false;

    rotateThread.join();

    std::cout << "End of mainloop" << std::endl;

    return 0;

}
