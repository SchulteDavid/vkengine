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
#include "util/mesh.h"
#include <mathutils/matrix.h>
#include <exception>
#include "util/mesh/marchingcubes.h"
#include "util/transform.h"
#include "structure/node/node.h"
#include "structure/node/nodeloader.h"


static bool run = true;
static bool wait;

void rotateFunc(std::shared_ptr<World> world, Viewport * view) {
  
  auto startRenderTime = std::chrono::high_resolution_clock::now();
  
  while (run){
    auto now = std::chrono::high_resolution_clock::now();
    double dt = std::chrono::duration<double, std::chrono::seconds::period>(now - startRenderTime).count();
    startRenderTime = now;
    world->simulateStep(dt);
    
    world->synchronize();
    world->update(dt);
    
    view->manageMemoryTransfer();
    
    while (wait);
    wait = true;
    
  }

}

void createResourceLoaders(ResourceManager * resourceManager, Viewport * view) {

  resourceManager->addRegistry("Shader", (ResourceRegistry<Resource> *) new ResourceRegistry<Shader>());
  resourceManager->addRegistry("Texture", (ResourceRegistry<Resource> *) new ResourceRegistry<Texture>());
  resourceManager->addRegistry("Material", (ResourceRegistry<Resource> *) new ResourceRegistry<Material>());
  //resourceManager->addRegistry("Structure", (ResourceRegistry<Resource>*) new ResourceRegistry<Structure>());
  //resourceManager->addRegistry("Level", (ResourceRegistry<Resource> *) new ResourceRegistry<Level>());
  resourceManager->addRegistry("Mesh", (ResourceRegistry<Resource> *) new ResourceRegistry<Mesh>());

  resourceManager->addRegistry("Node", (ResourceRegistry<Resource> *) new ResourceRegistry<strc::Node>());
  
  resourceManager->addLoader("Shader", (ResourceLoader<Resource> *) new ShaderLoader(view->getState()));
  resourceManager->addLoader("Texture", (ResourceLoader<Resource> *) new TextureLoader(view->getState()));
  resourceManager->addLoader("Material", (ResourceLoader<Resource> *) new MaterialLoader(view->getState(), view->getRenderpass(), view->getSwapchainExtent()));
  //resourceManager->addLoader("Structure", (ResourceLoader<Resource> *) new StructureLoader(view->getState()));
  //resourceManager->addLoader("Structure", (ResourceLoader<Resource> *) new GLTFLoader(view->getState(), view->getRenderpass(), view->getSwapchainExtent()));
  resourceManager->addLoader("Texture", (ResourceLoader<Resource> *) new PNGLoader(view->getState()));
  //resourceManager->addLoader("Level", (ResourceLoader<Resource> *) new LevelLoader());
  resourceManager->addLoader("Mesh", (ResourceLoader<Resource> *) new MeshLoader());


  strc::Node::registerLoaders();

  resourceManager->addLoader("Node", (ResourceLoader<Resource> *) new GLTFNodeLoader(view->getState(), view->getRenderpass(), view->getSwapchainExtent()));
  resourceManager->addLoader("Node", (ResourceLoader<Resource> *) new NodeLoader());
    
}

#define BM_SIZE ( 1 << 24 )
#include "util/simplexnoise.h"


double noiseFunc(double x, double y, double z) {

  return - z - 12.4 * SimplexNoise1234::noise(x*0.05,y*0.05,z*0.05)
    - 1.2 * SimplexNoise1234::noise(x * 0.1 + 123.253, y * 0.1, z * 0.1)
    - 0.12 * SimplexNoise1234::noise(x * 0.7,y * 0.8, z * 0.5);
  
}

int main(int argc, char ** argv) {

  unsigned int width = 1280;
  unsigned int height = 720;
  
  unsigned int tmp = 0;
  
  if (argc >= 3) {
    
    width = atoi(argv[1]);
    height = atoi(argv[2]);
    
  }
  
  std::future<std::shared_ptr<Mesh>> futureMesh = generateBackground(noiseFunc,
								     Math::Vector<3, float>({0,0,0}),
								     Math::Vector<3, float>({64.0f,64.0f,64.0f}),
								     0.5, 128);
  
  Entity::registerDefaultEntityTypes();
  
  std::shared_ptr<Window> window(new Window(width, height));
  
  Camera * cam = new Camera(70.0, 0.001, 100.0, 1280.0/720.0, glm::vec3(0,-10,0));
  
  Viewport * view = new Viewport(window, cam);
  window->setActiveViewport(view);
  
  std::cout << "Viewport OK" << std::endl;
  
  ResourceManager * resourceManager = new ResourceManager(ResourceManager::RESOURCE_MODEL | ResourceManager::RESOURCE_SHADER);
  
  createResourceLoaders(resourceManager, view);
  
  resourceManager->startLoadingThreads(1);
  
  //LoadingResource treeStruct = resourceManager->loadResourceBg("Structure", "tree.glb");
  //LoadingResource llvl = resourceManager->loadResourceBg("Level", "resources/level/test.lvl");
  LoadingResource node = resourceManager->loadResourceBg("Node", "exports.glb");
  LoadingResource node2 = resourceManager->loadResourceBg("Node", "tree.glb");
  
  
  std::shared_ptr<InputHandler> playerCtl(new PlayerControler(cam, window->getState()));
  window->addInputHandler(playerCtl);
  
  std::cout << "Start wait for node " << node << std::endl;
  node->fut.wait();
  node2->fut.wait();
  
  std::cout << "All futures are ok" << std::endl;
  
  resourceManager->joinLoadingThreads();
  
  logger(std::cout) << "Resource-summary: " << std::endl;
  resourceManager->printSummary();
  logger(std::cout) << "Done" << std::endl;

  std::cout << "Waiting for node, as it will be used" << std::endl;
  //node->fut.wait();

  //return 0;
  
  view->addLight(glm::vec4(0, 10, 3, 1.0), glm::vec4(3, 3, 3, 0));
  view->addLight(glm::vec4(1.0, 1.2, -1.5, 2.0), glm::vec4(20.0, 20.0, 20.0, 0.0));
  view->addLight(glm::vec4(0.2, 0.0, 1.0, 1.0), glm::vec4(0.0, 0.0, 1.0, 0.0));
  
  std::shared_ptr<World> world(new World());
  
  //std::shared_ptr<Level> lvl = resourceManager->get<Level>("Level", "resources/level/test.lvl");
  //lvl->applyToWorld(world, view);

  //std::shared_ptr<Mesh> testMesh = Mesh::loadFromFile("resources/models/cube.ply");
  
  std::cout << "Adding baseNode to Viewport" << std::endl;
  std::shared_ptr<strc::Node> baseNode = resourceManager->get<strc::Node>("Node", "exports.glb");
  baseNode->viewportAdd(view);

  /*std::shared_ptr<strc::Node> treeNode = resourceManager->get<strc::Node>("Node", "tree.glb");
    treeNode->viewportAdd(view);*/
  
  std::thread rotateThread(rotateFunc, world, view);
  
  std::cerr << "Rotate Thread: " << &rotateThread << std::endl;
  
  /*std::shared_ptr<Mesh> m = resourceManager->get<Mesh>("Mesh", "tree.glb");
    m->saveAsPLY("sheep.ply");*/
  
  /*int rElemCount = 0;
  Transform<float> initTrans;
  initTrans.position = Math::Vector<3, float>({0, 1, 0});
  initTrans.scale = Math::Vector<3, float>({1,1,1});
  initTrans.rotation = Math::Quaternion<float>(1,0,0,0);
  std::shared_ptr<RenderElement> rElem(RenderElement::buildRenderElement(view, resourceManager->get<Structure>("Structure", "tree.glb"), initTrans));
  
  view->addRenderElement(rElem);
  
  while (rElemCount < 2) {
    Transform<float> initTrans;
    initTrans.position = Math::Vector<3, float>({rElemCount * 3.0f, 1, 0});
    initTrans.scale = Math::Vector<3, float>({1,1,1});
    initTrans.rotation = Math::Quaternion<float>(1,0,0,0);
    rElem->addInstance(initTrans);
    rElemCount++;
    }*/
  
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
