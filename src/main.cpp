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
#include "audio/audiocontext.h"
#include "audio/sound.h"
#include "render/cubemap.h"
#include "scripting/script.h"

static bool run = true;
static bool wait;

void rotateFunc(std::shared_ptr<World> world, Viewport * view, std::shared_ptr<strc::Node> baseNode) {

  auto startRenderTime = std::chrono::high_resolution_clock::now();
  auto initTime = std::chrono::high_resolution_clock::now();

  Transform<double> originTrans;
  uint32_t nCount = 0;
  uint32_t loopCount = 0;

  uint32_t boxCount = 1;
  
  view->createSecondaryBuffers();

  while (run){
    auto now = std::chrono::high_resolution_clock::now();
    double dt = std::chrono::duration<double, std::chrono::seconds::period>(now - startRenderTime).count();
    //std::cout << "dt = " << (dt * 1000) << " ms" << std::endl;
    startRenderTime = now;

    double time = std::chrono::duration<double, std::chrono::seconds::period>(now - initTime).count();
    
    world->simulateStep(dt);

    world->synchronize();
    world->update(dt, time);

    view->manageMemoryTransfer();

    view->renderIntoSecondary();

    /*if (loopCount % 100 == 0) {
      Transform<double> trans;
      trans.position = Math::Vector<3>({0.0, 0.0, 30.0});

      std::string name("TestNode_");
      name.append(std::to_string(nCount++));
      std::shared_ptr<strc::Node> node = baseNode->createDuplicate(name);

      node->setTransform(trans);

      node->viewportAdd(view, node);
      node->worldAdd(world, node);

      boxCount++;
      
      }*/

    loopCount++;

    //while (wait)
    //lout << "Waiting" << std::endl;
      //wait = true;

    //lout << "Wait " << wait << std::endl;

  }

  std::cout << "Box-count at end: " << boxCount << std::endl;

}

void createResourceLoaders(ResourceManager * resourceManager) {
  
  resourceManager->addRegistry("Shader", (ResourceRegistry<Resource> *) new ResourceRegistry<Shader>());
  resourceManager->addRegistry("Texture", (ResourceRegistry<Resource> *) new ResourceRegistry<Texture>());
  resourceManager->addRegistry("CubeMap", (ResourceRegistry<Resource> *) new ResourceRegistry<CubeMap>());
  resourceManager->addRegistry("Material", (ResourceRegistry<Resource> *) new ResourceRegistry<Material>());
  resourceManager->addRegistry("Mesh", (ResourceRegistry<Resource> *) new ResourceRegistry<Mesh>());
  resourceManager->addRegistry("Node", (ResourceRegistry<Resource> *) new ResourceRegistry<strc::Node>());
  resourceManager->addRegistry("Sound", (ResourceRegistry<Resource> *) new ResourceRegistry<audio::Sound>());
  resourceManager->addRegistry("Skin", (ResourceRegistry<Resource> *) new ResourceRegistry<Skin>());
  resourceManager->addRegistry("Script", (ResourceRegistry<Resource> *) new ResourceRegistry<Script>());

  resourceManager->addLoader("Shader", (ResourceLoader<Resource> *) new ShaderLoader());
  resourceManager->addLoader("Texture", (ResourceLoader<Resource> *) new TextureLoader());
  resourceManager->addLoader("CubeMap", (ResourceLoader<Resource> *) new CubeMapLoader());
  resourceManager->addLoader("Material", (ResourceLoader<Resource> *) new MaterialLoader());
  resourceManager->addLoader("Texture", (ResourceLoader<Resource> *) new PNGLoader());
  resourceManager->addLoader("Mesh", (ResourceLoader<Resource> *) new MeshLoader());
  resourceManager->addLoader("Sound", (ResourceLoader<Resource> *) new audio::SoundLoader());


  strc::Node::registerLoaders();
  resourceManager->addLoader("Node", (ResourceLoader<Resource> *) new NodeLoader());

  std::shared_ptr<ArchiveLoader> gltfLoader(new GLTFNodeLoader());

  resourceManager->attachArchiveType("glb", gltfLoader);

}

#define BM_SIZE ( 1 << 24 )
#include "util/simplexnoise.h"


double noiseFunc(double x, double y, double z) {

  return - z - 12.4 * SimplexNoise1234::noise(x*0.05,y*0.05,z*0.05)
    - 1.2 * SimplexNoise1234::noise(x * 0.1 + 123.253, y * 0.1, z * 0.1)
    - 0.12 * SimplexNoise1234::noise(x * 0.7,y * 0.8, z * 0.5);

}

void testSpline2D() {

  FILE * file = fopen("spline.csv", "w");

  std::vector<double> x = { 0, 1, 2, 3 };
  std::vector<Math::Vector<2>> y = {
      Math::Vector<2>({0, 0}),
      Math::Vector<2>({1, 2}),
      Math::Vector<2>({0, 4}),
      Math::Vector<2>({3, 8}),
  };

  const Interpolator<Math::Vector<2>> & interpolator = SplineInterpolator(x, y);

  for (double i = 0; i <= 3.0; i += 0.01) {

    Math::Vector<2> pos = interpolator(i);
    
    fprintf(file, "%lf,%lf,%lf\n", i, pos[0], pos[1]);
    
  }

  fclose(file);
  
}

#include "util/image/png.h"
#include "render/cubemap.h"


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

  std::shared_ptr<Window> window(new Window(width, height));
  ResourceManager * resourceManager = new ResourceManager(window->getState());
  createResourceLoaders(resourceManager);
  resourceManager->startLoadingThreads(1);

  //std::shared_ptr<Texture> skyBox = testCubeMapLoading(window->getState(), resourceManager);

  LoadingResource ppShader = resourceManager->loadResourceBg(ResourceLocation("Shader", "resources/shaders/pp.shader"));
  ppShader->wait();

  LoadingResource testPPShader = resourceManager->loadResourceBg(ResourceLocation("Shader", "resources/shaders/test_pp.shader"));
  testPPShader->wait();

  LoadingResource skyBoxRes = resourceManager->loadResourceBg(ResourceLocation("CubeMap", "resources/textures/test_cubemap/cubemap.conf"));
  skyBoxRes->wait();

  std::shared_ptr<PPEffect> testEffect = std::make_shared<PPEffect>(std::dynamic_pointer_cast<Shader>(testPPShader->location));
  
  std::shared_ptr<Camera> cam = std::make_shared<Camera>(70.0, 0.001, 1000.0, 1280.0/720.0, glm::vec3(0,-10,0));

  std::shared_ptr<Texture> skyBox = std::dynamic_pointer_cast<CubeMap>(skyBoxRes->location);
  Viewport *view = new Viewport(window, cam, std::dynamic_pointer_cast<Shader>(ppShader->location), {testEffect}, skyBox);
  window->setActiveViewport(view);

  std::shared_ptr<audio::AudioContext> context(new audio::AudioContext());


  

  //LoadingResource treeStruct = resourceManager->loadResourceBg("Structure", "tree.glb");
  //LoadingResource llvl = resourceManager->loadResourceBg("Level", "resources/level/test.lvl");
  LoadingResource node = resourceManager->loadResourceBg(ResourceLocation("Node", "resources/nodes/box.glb"));
  LoadingResource node2 = resourceManager->loadResourceBg(ResourceLocation("Node", "resources/nodes/platform.glb"));
  LoadingResource node3 = resourceManager->loadResourceBg(ResourceLocation("Node", "resources/nodes/test.node"));

  lout << "Start wait for node " << node << std::endl;
  //while(!node->status.isUseable) ;
  //node3->fut.wait();
  //while(!node3->status.isUseable);
  node->wait();
  node3->wait();

  std::shared_ptr<World> world(new World());

  std::shared_ptr<strc::Node> boxNode = resourceManager->get<strc::Node>(ResourceLocation("Node", "resources/nodes/test.node", "FallingBox"));
  std::shared_ptr<PlayerControler> playerCtl(new PlayerControler(cam, window->getState(), context, boxNode));
  playerCtl->viewport = view;
  playerCtl->world = world;
  window->addInputHandler(playerCtl);

  lout << "All futures are ok" << std::endl;



  lout << "Resource-summary: " << std::endl;
  resourceManager->printSummary();
  lout << "Done" << std::endl;

  lout << "Waiting for node, as it will be used" << std::endl;
  //node->fut.wait();

  //return 0;

  //view->addLight(glm::vec4(0, 10, 3, 1.0), glm::vec4(3, 3, 3, 0));
  view->addLight(glm::vec4(1.0, 1.2, -1.5, 2.0), glm::vec4(20.0, 20.0, 20.0, 0.0));
  //view->addLight(glm::vec4(0.2, 0.0, 1.0, 1.0), glm::vec4(0.0, 0.0, 1.0, 0.0));

  std::shared_ptr<strc::Node> baseNode = resourceManager->get<strc::Node>(ResourceLocation("Node", "resources/nodes/box.glb"));
  baseNode->viewportAdd(view, baseNode);
  world->addNode(baseNode);

  std::shared_ptr<strc::Node> n3 = resourceManager->get<strc::Node>(ResourceLocation("Node", "resources/nodes/test.node"));
  n3->viewportAdd(view, n3);
  world->addNode(n3);

  std::thread rotateThread(rotateFunc, world, view, n3->getChild("FallingBox"));

  lerr << "Rotate Thread: " << &rotateThread << std::endl;

  bool isInViewport = false;

  while (!glfwWindowShouldClose(window->getGlfwWindow())) {

    glfwPollEvents();

    view->drawFrame();
    wait = false;

  }

  run = false;
  wait = false;

  world->saveNodeState("world.node");
  
  lout << "End of mainloop" << std::endl;

  lout << "Joining Threads" << std::endl;
  rotateThread.join();


  lout << "Final resources" << std::endl;

  resourceManager->printSummary();

  return 0;
}
