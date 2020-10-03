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
#include "world/entity.h"
#include "physics/physicscontext.h"
#include "world/world.h"
#include <execinfo.h>
#include "structure/gltf.h"
#include "util/debug/logger.h"
#include "util/mesh.h"
#include <mathutils/matrix.h>
#include <exception>
#include "util/mesh/marchingcubes.h"
#include "util/transform.h"
#include "node/node.h"
#include "node/meshnode.h"
#include "node/nodeloader.h"
#include "render/instancedrenderelement.h"

static bool run = true;
static bool wait;

void rotateFunc(std::shared_ptr<World> world, Viewport * view, std::shared_ptr<strc::Node> baseNode) {

  auto startRenderTime = std::chrono::high_resolution_clock::now();
  auto initTime = std::chrono::high_resolution_clock::now();

  Transform<double> originTrans;

  view->createSecondaryBuffers();

  while (run){
    auto now = std::chrono::high_resolution_clock::now();
    double dt = std::chrono::duration<double, std::chrono::seconds::period>(now - startRenderTime).count();
    startRenderTime = now;

    double time = std::chrono::duration<double, std::chrono::seconds::period>(now - initTime).count();

    world->simulateStep(dt);

    world->synchronize();
    world->update(dt);

    view->manageMemoryTransfer();

    Transform<double> trans;
    trans.position = Math::Vector<3, double>({10*cos(time), 10*sin(time), 0});

    baseNode->setTransform(trans);

    view->renderIntoSecondary();

    //while (wait)
    //lout << "Waiting" << std::endl;
      //wait = true;

    //lout << "Wait " << wait << std::endl;

  }

}

void createResourceLoaders(ResourceManager * resourceManager, Viewport * view) {
  
  resourceManager->addRegistry("Shader", (ResourceRegistry<Resource> *) new ResourceRegistry<Shader>());
  resourceManager->addRegistry("Texture", (ResourceRegistry<Resource> *) new ResourceRegistry<Texture>());
  resourceManager->addRegistry("Material", (ResourceRegistry<Resource> *) new ResourceRegistry<Material>());
  resourceManager->addRegistry("Mesh", (ResourceRegistry<Resource> *) new ResourceRegistry<Mesh>());

  resourceManager->addRegistry("Node", (ResourceRegistry<Resource> *) new ResourceRegistry<strc::Node>());

  resourceManager->addLoader("Shader", (ResourceLoader<Resource> *) new ShaderLoader(view->getState()));
  resourceManager->addLoader("Texture", (ResourceLoader<Resource> *) new TextureLoader(view->getState()));
  resourceManager->addLoader("Material", (ResourceLoader<Resource> *) new MaterialLoader(view->getState(), view->getRenderpass(), view->getSwapchainExtent()));
  resourceManager->addLoader("Texture", (ResourceLoader<Resource> *) new PNGLoader(view->getState()));
  resourceManager->addLoader("Mesh", (ResourceLoader<Resource> *) new MeshLoader());


  strc::Node::registerLoaders();

  //resourceManager->addLoader("Node", (ResourceLoader<Resource> *) new GLTFNodeLoader(view->getState(), view->getRenderpass(), view->getSwapchainExtent()));
  
  resourceManager->addLoader("Node", (ResourceLoader<Resource> *) new NodeLoader());

  std::shared_ptr<ArchiveLoader> gltfLoader(new GLTFNodeLoader(view->getState(), view->getRenderpass(), view->getSwapchainExtent()));

  resourceManager->attachArchiveType("glb", gltfLoader);

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

  /*std::future<std::shared_ptr<Mesh>> futureMesh = generateBackground(noiseFunc,
								     Math::Vector<3, float>(0,0,0),
								     Math::Vector<3, float>(64.0f,64.0f,64.0f),
								     0.5, 128);*/

  Entity::registerDefaultEntityTypes();

  std::shared_ptr<Window> window(new Window(width, height));

  Camera * cam = new Camera(70.0, 0.001, 100.0, 1280.0/720.0, glm::vec3(0,-10,0));

  Viewport * view = new Viewport(window, cam);
  window->setActiveViewport(view);

  lout << "Viewport OK" << std::endl;

  ResourceManager * resourceManager = new ResourceManager(ResourceManager::RESOURCE_MODEL | ResourceManager::RESOURCE_SHADER);

  createResourceLoaders(resourceManager, view);

  resourceManager->startLoadingThreads(1);

  //LoadingResource treeStruct = resourceManager->loadResourceBg("Structure", "tree.glb");
  //LoadingResource llvl = resourceManager->loadResourceBg("Level", "resources/level/test.lvl");
  LoadingResource node = resourceManager->loadResourceBg(ResourceLocation("Node", "exports.glb"));
  LoadingResource node2 = resourceManager->loadResourceBg(ResourceLocation("Node", "tree.glb"));
  LoadingResource node3 = resourceManager->loadResourceBg(ResourceLocation("Node", "resources/nodes/test.node"));


  std::shared_ptr<InputHandler> playerCtl(new PlayerControler(cam, window->getState()));
  window->addInputHandler(playerCtl);

  lout << "Start wait for node " << node << std::endl;
  //while(!node->status.isUseable) ;
  //node3->fut.wait();
  //while(!node3->status.isUseable);
  node->wait();
  node3->wait();

  lout << "All futures are ok" << std::endl;



  lout << "Resource-summary: " << std::endl;
  resourceManager->printSummary();
  lout << "Done" << std::endl;

  lout << "Waiting for node, as it will be used" << std::endl;
  //node->fut.wait();

  //return 0;

  view->addLight(glm::vec4(0, 10, 3, 1.0), glm::vec4(3, 3, 3, 0));
  view->addLight(glm::vec4(1.0, 1.2, -1.5, 2.0), glm::vec4(20.0, 20.0, 20.0, 0.0));
  view->addLight(glm::vec4(0.2, 0.0, 1.0, 1.0), glm::vec4(0.0, 0.0, 1.0, 0.0));

  std::shared_ptr<World> world(new World());

  lout << "Adding baseNode to Viewport" << std::endl;
  std::shared_ptr<strc::Node> baseNode = resourceManager->get<strc::Node>(ResourceLocation("Node", "exports.glb"));
  baseNode->viewportAdd(view);
  lout << "Done Node " << baseNode << std::endl;

  lout << "Adding other Node" << std::endl;
  std::shared_ptr<strc::Node> n3 = resourceManager->get<strc::Node>(ResourceLocation("Node", "resources/nodes/test.node", "Mesh.001"));
  n3->viewportAdd(view);
  lout << "Done adding other node" << std::endl;


  {
    std::shared_ptr<strc::MeshNode> meshNode = std::dynamic_pointer_cast<strc::MeshNode>(baseNode->getChildren()[0]);

    std::shared_ptr<Mesh> mesh = meshNode->getMesh();
    std::shared_ptr<Material> material = meshNode->getMaterial();

    lout << "Shader " << material->getShader() << " static: " << material->getStaticShader() << std::endl;

    unsigned int stride;
    const std::vector<InputDescription> & elements = material->getShader()->getInputs();

    std::vector<InterleaveElement> iData = mesh->compactStorage(elements, &stride);


    std::shared_ptr<Model> model = std::shared_ptr<Model>(new Model(view->getState(), mesh, iData, stride, true));

    //RenderElement::buildRenderElement(view, meshNode->buildModel(view->getState()), meshNode->getMaterial(), trans)
    lout << "Building Instanced element" << std::endl;
    std::shared_ptr<RenderElement> rElem(new InstancedRenderElement(view, model, material, view->getSwapchainSize()));
    lout << "Constructing buffers" << std::endl;
    rElem->constructBuffers(view->getSwapchainSize());
    lout << "Adding to viewport" << std::endl;
    view->addRenderElement(rElem);

    lout << "Instanced element is built" << std::endl;

    for (unsigned int i = 0; i < 1024; ++i) {

      Transform<float> trans;
      trans.position = Math::Vector<3, float>((i/32) * 3, (i % 32) * 3, 0);

      rElem->addInstance(trans);

    }
  }

  std::thread rotateThread(rotateFunc, world, view, baseNode);

  lerr << "Rotate Thread: " << &rotateThread << std::endl;

  //LoadingResource testRes = resourceManager->loadResourceBg(ResourceLocation("Node", "sheep_.glb"));
  bool isInViewport = false;

  while (!glfwWindowShouldClose(window->getGlfwWindow())) {

    glfwPollEvents();

    view->drawFrame();
    wait = false;

    /*if (testRes->status.isUseable && !isInViewport) {

      std::shared_ptr<strc::Node> strcNode = resourceManager->get<strc::Node>(ResourceLocation("Node", "sheep_.glb"));
      strcNode->viewportAdd(view);
      isInViewport = true;

    }*/


  }

  run = false;
  wait = false;

  resourceManager->joinLoadingThreads();
  rotateThread.join();

  lout << "End of mainloop" << std::endl;

  lout << "Final resources" << std::endl;

  resourceManager->printSummary();

  return 0;
}
