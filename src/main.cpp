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
#include "world/entity.h"
#include "physics/physicscontext.h"

#include "world/world.h"
#include <execinfo.h>
#include "structure/gltf.h"
#include "structure/level.h"

#include "util/debug/logger.h"

#include <mathutils/matrix.h>

#include <exception>

#include "util/mesh/marchingcubes.h"

static bool run = true;
static bool wait;

void rotateFunc(std::shared_ptr<World> world, Viewport * view) {

    auto startRenderTime = std::chrono::high_resolution_clock::now();

    while (run) {
        //try {
        auto now = std::chrono::high_resolution_clock::now();
        double dt = std::chrono::duration<double, std::chrono::seconds::period>(now - startRenderTime).count();
        startRenderTime = now;
        world->simulateStep(dt);

        world->synchronize();
        world->update(dt);

        view->manageMemoryTransfer();

        while (wait);
        wait = true;
        /*} catch (...) {
            std::exception_ptr e = std::current_exception();
            std::cout << "Something happened" << std::endl;
            std::cerr << typeid(e).name() << std::endl;
            exit(0);
        }*/

    }

}

void createResourceLoaders(ResourceManager * resourceManager, Viewport * view) {

    resourceManager->addRegistry("Texture", (ResourceRegistry<Resource> *) new ResourceRegistry<Texture>());
    resourceManager->addRegistry("Material", (ResourceRegistry<Resource> *) new ResourceRegistry<Material>());
    resourceManager->addRegistry("Structure", (ResourceRegistry<Resource>*) new ResourceRegistry<Structure>());
    resourceManager->addRegistry("Level", (ResourceRegistry<Resource> *) new ResourceRegistry<Level>());

    resourceManager->addLoader("Model", (ResourceLoader<Resource> *) new ModelLoader(view->getState()));
    resourceManager->addLoader("Shader", (ResourceLoader<Resource> *) new ShaderLoader(view->getState()));
    resourceManager->addLoader("Texture", (ResourceLoader<Resource> *) new TextureLoader(view->getState()));
    resourceManager->addLoader("Material", (ResourceLoader<Resource> *) new MaterialLoader(view->getState(), view->getRenderpass(), view->getSwapchainExtent()));
    resourceManager->addLoader("Structure", (ResourceLoader<Resource> *) new StructureLoader(view->getState()));
    resourceManager->addLoader("Structure", (ResourceLoader<Resource> *) new GLTFLoader(view->getState(), view->getRenderpass(), view->getSwapchainExtent()));

    resourceManager->addLoader("Texture", (ResourceLoader<Resource> *) new PNGLoader(view->getState()));

    resourceManager->addLoader("Level", (ResourceLoader<Resource> *) new LevelLoader());

}

#define BM_SIZE ( 1 << 24 )

#include "util/simplexnoise.h"

double noiseFunc(double x, double y, double z) {

    return - z - 6.4 * SimplexNoise1234::noise(x*0.05,y*0.05,z*0.05) - 1.2 * SimplexNoise1234::noise(x * 0.1 + 123.253, y * 0.1, z * 0.1) - 0.12 * SimplexNoise1234::noise(x * 0.7,y * 0.8, z * 0.5);

}

int main(int argc, char ** argv) {

    unsigned int width = 1280;
    unsigned int height = 720;

    if (argc >= 3) {

        width = atoi(argv[1]);
        height = atoi(argv[2]);

    }

    //gltfLoadFile("sheep_.glb");

    std::future<std::shared_ptr<Mesh>> futureMesh = generateBackground(noiseFunc, Math::Vector<3, float>({0,0,0}), Math::Vector<3, float>({64,64,64}), 0.5, 128);
    //std::shared_ptr<Mesh> mesh = buildMeshFromFunction(noiseFunc, Math::Vector<3, float>({0,0,0}), Math::Vector<3, float>({64,64,64}), 0.5, 128);

    //mesh->saveAsPLY("mesh.ply");

    Entity::registerDefaultEntityTypes();

    std::shared_ptr<Window> window(new Window(width, height));

    Camera * cam = new Camera(70.0, 0.001, 100.0, 1280.0/720.0, glm::vec3(0,-10,0));

    Viewport * view = new Viewport(window, cam);
    window->setActiveViewport(view);

    std::cout << "Viewport OK" << std::endl;

    //return 0;

    ResourceManager * resourceManager = new ResourceManager(ResourceManager::RESOURCE_MODEL | ResourceManager::RESOURCE_SHADER);

    createResourceLoaders(resourceManager, view);

    resourceManager->startLoadingThreads(1);

    //LoadingResource tres = resourceManager->loadResourceBg("Structure", "sign.glb");
    //LoadingResource cres = resourceManager->loadResourceBg("Structure", "sheep_.glb");
    LoadingResource matRes = resourceManager->loadResourceBg("Material", "resources/materials/test.mat");
    LoadingResource llvl = resourceManager->loadResourceBg("Level", "resources/level/test.lvl");


    std::shared_ptr<InputHandler> playerCtl(new PlayerControler(cam, window->getState()));
    window->addInputHandler(playerCtl);

    /*while (!llvl->status.isUseable) {
        //view->drawFrame(false);
        glfwPollEvents();
    }*/
    /*std::cout << "Start wait for tres" << std::endl;
    tres->fut.wait();
    std::cout << "Start wait for cres" << std::endl;
    cres->fut.wait();*/
    std::cout << "Start wait for llvl" << std::endl;
    llvl->fut.wait();

    std::cout << "All futures are ok" << std::endl;

    resourceManager->joinLoadingThreads();

    logger(std::cout) << "Resource-summary: " << std::endl;
    resourceManager->printSummary();
    logger(std::cout) << "Done" << std::endl;

    view->addLight(glm::vec4(0, 10, 3, 1.0), glm::vec4(3, 3, 3, 0));
    view->addLight(glm::vec4(1.0, 1.2, -1.5, 2.0), glm::vec4(20.0, 20.0, 20.0, 0.0));
    view->addLight(glm::vec4(0.2, 0.0, 1.0, 1.0), glm::vec4(0.0, 0.0, 1.0, 0.0));

    std::shared_ptr<World> world(new World());
    //world->addEntity(ent);

    std::shared_ptr<Level> lvl = resourceManager->get<Level>("Level", "resources/level/test.lvl");
    //std::cout << "Level " << lvl << std::endl;
    lvl->applyToWorld(world, view);

    futureMesh.wait();
    std::shared_ptr<Mesh> mesh = futureMesh.get();

    std::shared_ptr<Model> model(new Model(view->getState(), mesh));
    vkutil::VulkanState & state = view->getState();
    model->uploadToGPU(state.device, state.transferCommandPool, state.transferQueue);
    RenderElement::Transform trans;
    trans.position = Math::Vector<3, float>({0,0,1});
    trans.qRot = Math::Quaternion<float>(1,0,0,0);
    trans.scale = 1.0;
    std::shared_ptr<Material> material = std::dynamic_pointer_cast<Material>(matRes->location);
    std::shared_ptr<RenderElement> rElem(RenderElement::buildRenderElement(view, model, material, trans));

    view->addRenderElement(rElem);

    std::thread rotateThread(rotateFunc, world, view);

    while (!glfwWindowShouldClose(window->getGlfwWindow())) {

        glfwPollEvents();

        view->drawFrame();
        wait = false;

    }

    run = false;
    wait = false;

    rotateThread.join();

    logger(std::cout) << "End of mainloop" << std::endl;

    return 0;

}
