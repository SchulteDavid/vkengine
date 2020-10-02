#ifndef NODE_LOADER_H
#define NODE_LOADER_H

#include "node.h"
#include <memory>
#include <configloading.h>

class NodeUploader : public ResourceUploader<strc::Node> {

 public:
  
  NodeUploader(std::shared_ptr<strc::Node> node);
  
  std::shared_ptr<strc::Node> uploadResource() override;
  
  bool uploadReady() override;

  void addChild(std::shared_ptr<NodeUploader> child);
  
 protected:

  NodeUploader();
  void populateChildren(std::shared_ptr<strc::Node> node);
  bool childrenReady();

 private:
  std::shared_ptr<strc::Node> rootNode;
  std::vector<std::shared_ptr<NodeUploader>> children;
  
};

class NodeLoader : public ResourceLoader<strc::Node> {

public:

  /// Structure used for passing information to
  /// the different loading functions.
  struct LoadingContext {
    NodeLoader * loader;
    Transform<double> parentTransform;
    Transform<double> transform;
  };

  typedef std::function<std::shared_ptr<NodeUploader>(std::shared_ptr<config::NodeCompound>, const LoadingContext & context)> NodeLoadingFunction;
  
  NodeLoader();
  
  std::shared_ptr<ResourceUploader<strc::Node>> loadResource(std::string fname);
  std::shared_ptr<NodeUploader> loadNodeFromCompound(std::shared_ptr<config::NodeCompound> comp, const LoadingContext & context);
  std::shared_ptr<NodeUploader> loadNodeFromFile(std::string fname, const LoadingContext & context);

  LoadingResource loadDependencyResource(ResourceLocation location);

  static void registerNodeLoader(std::string type, NodeLoadingFunction func);
  
private:
  static std::unordered_map<std::string, NodeLoadingFunction> fmap;

};

#endif
