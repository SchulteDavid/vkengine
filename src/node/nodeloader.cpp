#include "nodeloader.h"
#include "util/transform.h"

#include "meshnode.h"
#include "node/event.h"

NodeUploader::NodeUploader() {
  this->rootNode = nullptr;
  this->eventHandler = "";
}

NodeUploader::NodeUploader(std::shared_ptr<strc::Node> node) {
    this->rootNode = node;
    this->eventHandler = "";
}

bool NodeUploader::uploadReady()  {
  return childrenReady();
}

bool NodeUploader::childrenReady() {
  bool isReady = true;
  
  for (const LoadingResource & res : children)
    isReady &= res->status.isUseable;

  for (auto it : resourcesToAttach)
    isReady &= it.second->status.isUseable;

  //std::cout << "Checking if " << this->getNodeName() << " is ready for upload: " << isReady << std::endl;
  
  return isReady;
}

std::shared_ptr<strc::Node> NodeUploader::uploadResource(vkutil::VulkanState & state, ResourceManager * manager) {

  std::shared_ptr<strc::Node> node = constructNode();

  std::cout << "Got Node " << node << std::endl;
  
  populateChildren(node);
  populateEventHandler(node, manager);
  populateResources(node);
  populateAnimations(node);

  Transform<double> initTrans = node->getGlobalTransform();
  std::cout << "World transform: " << wt << " init: " << initTrans << std::endl;
  node->setTransform(node->getTransform(), wt);
  Transform<double> fTrans = node->getGlobalTransform();
  std::cout << "Final: " << fTrans << std::endl;
  
  return node;

}

std::shared_ptr<strc::Node> NodeUploader::constructNode() {
  return rootNode;
}

void NodeUploader::worldTrans(Transform<double> trans) {
  this->wt = trans;
}

void NodeUploader::addChild(LoadingResource child) {
  children.push_back(child);
}

void NodeUploader::addEventHandler(std::string handler) {
  eventHandler = handler;
}

void NodeUploader::addAttachedResource(std::string name, LoadingResource res) {
  resourcesToAttach[name] = res;
}

void NodeUploader::addAnimation(std::string name, std::shared_ptr<Animation> anim) {
  animations[name] = anim;
}

void NodeUploader::populateChildren(std::shared_ptr<strc::Node> node) {

  for (const LoadingResource & res : children) {
    std::shared_ptr<strc::Node> child = std::dynamic_pointer_cast<strc::Node>(res->location);

    if (child) {
      node->addChild(child);
    }

  }

}

void NodeUploader::populateResources(std::shared_ptr<strc::Node> node) {

  for (auto it : resourcesToAttach) {
    node->attachResource(it.first, it.second->location);
  }
  
}

void NodeUploader::populateAnimations(std::shared_ptr<strc::Node> node) {

  for (auto it : animations) {
    node->addAnimation(it.first, it.second);
  }
  
}



void NodeUploader::populateEventHandler(std::shared_ptr<strc::Node> node, ResourceManager * manager) {
  if (eventHandler != "") {

    std::shared_ptr<strc::EventHandler> handler = strc::constructEventHandler(eventHandler);
    node->attachEventHandler(handler, node);
    
  } else {
    node->attachEventHandler(std::make_shared<strc::EventHandler>(), node);
  }
}

std::string NodeUploader::getNodeName() {
  return this->rootNode->getName();
}

const std::unordered_map<std::string, LoadingResource> & NodeUploader::getAttachedResources() {
  return resourcesToAttach;
}

Transform<double> loadTransform(std::shared_ptr<config::NodeCompound> transNode) {

  Transform<double> trans;

  std::shared_ptr<config::Node<double>> posData = transNode->getNode<double>("position");
  trans.position = Math::Vector<3, double>({posData->getElement(0), posData->getElement(1), posData->getElement(2)});

  std::shared_ptr<config::Node<double>> rotData = transNode->getNode<double>("rotation");
  trans.rotation = Math::Quaternion<double>(rotData->getElement(0), rotData->getElement(1), rotData->getElement(2), rotData->getElement(3));

  std::shared_ptr<config::Node<double>> sData = transNode->getNode<double>("scale");
  trans.scale = Math::Vector<3, double>({sData->getElement(0), sData->getElement(1), sData->getElement(2)});

  return trans;

}

std::unordered_map<std::string, NodeLoader::NodeLoadingFunction> NodeLoader::fmap;

NodeLoader::NodeLoader() {

  //fmap["Node"] = loadDefaultNode;
  //fmap["MeshNode"] = loadMeshNode;

}

LoadingResource NodeLoader::loadDependencyResource(ResourceLocation location) {

  return this->loadDependency(location);

}

std::shared_ptr<ResourceUploader<strc::Node>> NodeLoader::loadResource(std::string fname) {

  lout << "Loading Node from " << fname << std::endl;

  LoadingContext context;
  context.loader = this;
  context.fname = fname;

  return this->loadNodeFromFile(fname, context);
}

std::shared_ptr<NodeUploader> NodeLoader::loadNodeFromFile(std::string fname, const LoadingContext & context) {
  std::shared_ptr<config::NodeCompound> root = config::parseFile(fname);
  return this->loadNodeFromCompound(root, context);
}

void NodeLoader::registerNodeLoader(std::string type, NodeLoadingFunction func) {
  fmap[type] = func;
}

std::shared_ptr<NodeUploader> NodeLoader::loadNodeFromCompound(std::shared_ptr<config::NodeCompound> comp, const LoadingContext & inContext) {

  Transform<double> trans = loadTransform(comp->getNodeCompound("transform"));

  lout << "Loading node" << std::endl;
  std::string type(comp->getNode<char>("type")->getRawData());
  lout << "Loading node of type " << type << std::endl;

  if (fmap.find(type) == fmap.end()) {
    throw dbg::trace_exception(std::string("Unknown node type ").append(type));
  }

  LoadingContext context;
  context.loader = this;
  context.transform = trans;
  context.parentTransform = inContext.parentTransform * inContext.transform; /// TODO: change to be correct
  context.fname = inContext.fname;

  std::string nodeName(comp->getNode<char>("name")->getRawData());

  std::shared_ptr<NodeUploader> node = fmap[type](comp, context, nodeName);

  if (comp->hasChild("children")) {

    std::shared_ptr<config::Node<std::shared_ptr<config::NodeCompound>>> children = comp->getNode<std::shared_ptr<config::NodeCompound>>("children");
    for (unsigned int i = 0; i < children->getElementCount(); ++i) {

      std::shared_ptr<config::NodeCompound> childComp = children->getElement(i);
      LoadingResource child;
      if (childComp->hasChild("type")) {
        std::shared_ptr<NodeUploader> uploader = this->loadNodeFromCompound(childComp, context);
        std::string childName(childComp->getNode<char>("name")->getRawData());
        child = scheduleSubresource(ResourceLocation("Node", context.fname, childName), uploader);
      } else if (childComp->hasChild("location")) {
        ResourceLocation location = ResourceLocation::parse("Node", std::string(childComp->getNode<char>("location")->getRawData()));
        child = loadDependency(location);
      } else {
        throw dbg::trace_exception("Child node has no type and no file");
      }

      //node->addChild(child);
      node->addChild(child);

    }

  }

  if (comp->hasChild("eventHandler")) {
    std::string handlerName(comp->getNode<char>("eventHandler")->getRawData());
    //std::shared_ptr<strc::EventHandler> handler = strc::constructEventHandler(handlerName);
    node->addEventHandler(handlerName);
  }

  if (comp->hasChild("attachedResources")) {

    std::shared_ptr<config::Node<std::shared_ptr<config::NodeCompound>>> resources = comp->getNode<std::shared_ptr<config::NodeCompound>>("attachedResources");

    for (unsigned int i = 0; i < resources->getElementCount(); ++i) {

      std::shared_ptr<config::NodeCompound> resComp = resources->getElement(i);

      std::string name(resComp->getNode<char>("name")->getRawData());
      std::string type(resComp->getNode<char>("type")->getRawData());

      std::string resLoc(resComp->getNode<char>("location")->getRawData());
      ResourceLocation location = ResourceLocation::parse(type, resLoc);

      LoadingResource res = loadDependencyResource(location);

      node->addAttachedResource(name, res);
      
    }
    
  }

  return node;

}
