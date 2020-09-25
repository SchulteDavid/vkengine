#include "nodeloader.h"
#include "util/transform.h"

#include "meshnode.h"

NodeUploader::NodeUploader() {
  this->rootNode = nullptr;
}

NodeUploader::NodeUploader(std::shared_ptr<strc::Node> node) {
    this->rootNode = node;
}

bool NodeUploader::uploadReady()  {
  return childrenReady();
}

bool NodeUploader::childrenReady() {
  bool isReady = true;
  
  for (const std::shared_ptr<ResourceUploader<strc::Node>> & res : children)
    isReady &= res->uploadReady();
  
  return isReady;
}

std::shared_ptr<strc::Node> NodeUploader::uploadResource() {

  populateChildren(rootNode);

  return rootNode;
  
}

void NodeUploader::addChild(std::shared_ptr<NodeUploader> child) {
  children.push_back(child);
}

void NodeUploader::populateChildren(std::shared_ptr<strc::Node> node) {

  for (const std::shared_ptr<ResourceUploader<strc::Node>> & res : children) {
    std::shared_ptr<strc::Node> child = res->uploadResource();

    if (child) {
      node->addChild(child);
    }
      
  }
  
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

LoadingResource NodeLoader::loadDependencyResource(std::string regName, std::string name) {

  return this->loadDependency(regName, name);
  
}

std::shared_ptr<ResourceUploader<strc::Node>> NodeLoader::loadResource(std::string fname) {

  LoadingContext context;
  context.loader = this;
  
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
  
  std::cout << "Loading node" << std::endl;
  std::string type(comp->getNode<char>("type")->getRawData());
  std::cout << "Loading node of type " << type << std::endl;

  if (fmap.find(type) == fmap.end()) {
    throw dbg::trace_exception(std::string("Unknown node type ").append(type));
  }

  LoadingContext context;
  context.loader = this;
  context.transform = trans;
  context.parentTransform = inContext.transform; /// TODO: change to be correct

  std::shared_ptr<NodeUploader> node = fmap[type](comp, context);

  if (comp->hasChild("children")) {

    std::shared_ptr<config::Node<std::shared_ptr<config::NodeCompound>>> children = comp->getNode<std::shared_ptr<config::NodeCompound>>("children");
    for (unsigned int i = 0; i < children->getElementCount(); ++i) {

      std::shared_ptr<config::NodeCompound> childComp = children->getElement(i);
      std::shared_ptr<NodeUploader> child;
      if (childComp->getChild("type")) {
	child = this->loadNodeFromCompound(childComp, context);
      } else if (childComp->getChild("filename")) {
	child = this->loadNodeFromFile(std::string(childComp->getNode<char>("filename")->getRawData()), context);
      } else {
	throw dbg::trace_exception("Child node has no type and no file");
      }

      //node->addChild(child);
      node->addChild(child);
      
    }
    
  }
  
  return node;
  
}
