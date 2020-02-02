#include <iostream>
#include <memory>
#include <thread>

#include <GLFW/glfw3.h>

#include <vk_mem_alloc.h>

#include "render/util/vkutil.h"
#include "render/window.h"
#include "render/viewport.h"

#include "resources/resourcemanager.h"

#include "inputs/playercontroler.h"

#include "util/debug/trace_exception.h"

#include "structure/structure.h"

#include <mathutils/matrix.h>

static bool run = true;
static bool wait;

void rotateFunc(std::shared_ptr<RenderElement> e, std::vector<RenderElement::Instance> & instances, std::vector<RenderElement::Transform> & transforms, Viewport * view) {

    float rotAxis[3] = {0, 0, 1};

    auto startRenderTime = std::chrono::high_resolution_clock::now();

    while (run) {

        double duration = std::chrono::duration<double, std::chrono::seconds::period>(std::chrono::high_resolution_clock::now() - startRenderTime).count();

        for (unsigned int i = 0; i < instances.size(); ++i) {

            Math::Quaternion<float> dr = Math::Quaternion<float>::fromAxisAngle(Math::Vector<3,float>(rotAxis), M_PI * duration);
            transforms[i].qRot = transforms[i].qRot * dr;

            e->updateInstance(instances[i], transforms[i]);

        }

        startRenderTime = std::chrono::high_resolution_clock::now();

        view->manageMemoryTransfer();

        while (wait);
        wait = true;

    }

}

#include <execinfo.h>

#define BM_SIZE ( 1 << 24 )

int main(int argc, char ** argv) {

    std::shared_ptr<Window> window(new Window());

    Camera * cam = new Camera(70.0, 0.1, 100.0, 1280.0/720.0, glm::vec3(0,0,0));

    Viewport * view = new Viewport(window, cam);

    ResourceManager * resourceManager = new ResourceManager(ResourceManager::RESOURCE_MODEL | ResourceManager::RESOURCE_SHADER);

    resourceManager->addRegistry("Texture", (ResourceRegistry<Resource> *) new ResourceRegistry<Texture>());
    resourceManager->addRegistry("Material", (ResourceRegistry<Resource> *) new ResourceRegistry<Material>());
    resourceManager->addRegistry("Structure", (ResourceRegistry<Resource>*) new ResourceRegistry<Structure>());

    resourceManager->addLoader("Model", (ResourceLoader<Resource> *) new ModelLoader(view->getState()));
    resourceManager->addLoader("Shader", (ResourceLoader<Resource> *) new ShaderLoader(view->getState()));
    resourceManager->addLoader("Texture", (ResourceLoader<Resource> *) new TextureLoader(view->getState()));
    resourceManager->addLoader("Material", (ResourceLoader<Resource> *) new MaterialLoader(view->getState(), view->getRenderpass(), view->getSwapchainExtent()));
    resourceManager->addLoader("Structure", (ResourceLoader<Resource> *) new StructureLoader());

    resourceManager->startLoadingThreads(1);

    LoadingResource matRes = resourceManager->loadResourceBg("Material", "resources/materials/test.mat");
    LoadingResource fres = resourceManager->loadResourceBg("Model", "resources/models/cube.ply");

    LoadingResource sres = resourceManager->loadResourceBg("Structure", "resources/structure/test.strc");

    std::shared_ptr<InputHandler> playerCtl(new PlayerControler(cam, window->getState()));
    window->addInputHandler(playerCtl);

    while (!matRes->status.isUseable);

    std::cout << "Resource-summary: " << std::endl;
    resourceManager->printSummary();
    std::cout << "Done" << std::endl;

    std::shared_ptr<Shader> shader = resourceManager->get<Shader>("Shader", "resources/shaders/std.shader");
    std::shared_ptr<Model> model = resourceManager->get<Model>("Model", "resources/models/cube.ply");

    //std::vector<std::shared_ptr<Texture>> textures = {resourceManager->get<Texture>("Texture", "test.tga"), resourceManager->get<Texture>("Texture", "normals.tga")};


    RenderElement::Transform trans;
    trans.position = Math::Vector<4, float>(0, 10, 0, 0);
    trans.qRot = Math::Quaternion<float>(1.0, 0.0, 0.0, 0.0);
    trans.scale = 1.0;

    //std::shared_ptr<Material> material(new Material(shader, textures));
    //material->setupPipeline(window->getState(), view->getRenderpass(), view->getSwapchainExtent());

    std::shared_ptr<Material> material = resourceManager->get<Material>("Material", "resources/materials/test.mat");

    std::shared_ptr<RenderElement> e(new RenderElement(view, model, material, view->getSwapchainSize(), trans));

    view->addRenderElement(e);

    cam->move(3, 0, 3);

    std::vector<RenderElement::Instance> instances;
    std::vector<RenderElement::Transform> transforms;

    //view->addLight(glm::vec4(0, 10, 3, 1.0), glm::vec4(3, 3, 3, 0));
    view->addLight(glm::vec4(1.0, 1.2, 1.5, 2.0), glm::vec4(2.0, 2.0, 2.0, 0.0));
    //view->addLight(glm::vec4(0.2, 0.0, 1.0, 2.0), glm::vec4(0.0, 0.0, 1.0, 0.0));
    //view->addLight(glm::vec4(1.0, -1.0, 1.0, 2.0), glm::vec4(1.5, 1.5, 1.5, 1.2));

    //view->addLight(glm::vec4(12.5, 12.5, 0.5, 0.0), glm::vec4(10, 10, 10, 0.0));

    for (unsigned int k = 0; k < 2; ++k) {

    for (unsigned int i = 0; i < 32; ++i) {
        for (unsigned int j = 0; j < 32; ++j) {

            RenderElement::Transform trans2;
            trans2.position = Math::Vector<4, float>(2 * i, 2 * j, 2 * k, 0);
            trans2.qRot = Math::Quaternion<float>(1.0, 0.0, 0.0, 0.0);
            trans2.scale = 0.5;

            RenderElement::Instance inst = e->addInstance(trans2);
            instances.push_back(inst);
            transforms.push_back(trans2);

        }
    }
    }

    resourceManager->joinLoadingThreads();

    double d = 0;

    std::thread rotateThread(rotateFunc, e, std::ref(instances), std::ref(transforms), view);

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
