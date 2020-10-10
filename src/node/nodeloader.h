#ifndef NODE_LOADER_H
#define NODE_LOADER_H

#include "node.h"
#include <memory>
#include <configloading.h>

class NodeUploader : public ResourceUploader<strc::Node> {

 public:

  NodeUploader(std::shared_ptr<strc::Node> node);

  std::shared_ptr<strc::Node> uploadResource(vkutil::VulkanState & state) override;

  bool uploadReady() override;

  void addChild(LoadingResource child);
  void addEventHandler(std::shared_ptr<strc::EventHandler> handler);
  void addAttachedResource(std::string name, LoadingResource res);

  virtual std::string getNodeName();

 protected:

  NodeUploader();
  void populateChildren(std::shared_ptr<strc::Node> node);
  void populateEventHandler(std::shared_ptr<strc::Node> node);
  void populateResources(std::shared_ptr<strc::Node> node);
  bool childrenReady();

  virtual std::shared_ptr<strc::Node> constructNode();

 private:
  std::shared_ptr<strc::Node> rootNode;
  std::vector<LoadingResource> children;
  std::shared_ptr<strc::EventHandler> eventHandler;

  std::unordered_map<std::string, LoadingResource> resourcesToAttach;

};

class NodeLoader : public ResourceLoader<strc::Node> {

public:

  /// Structure used for passing information to
  /// the different loading functions.
  struct LoadingContext {
    NodeLoader * loader;
    Transform<double> parentTransform;
    Transform<double> transform;
    std::string fname;
  };

  typedef std::function<std::shared_ptr<NodeUploader>(std::shared_ptr<config::NodeCompound>, const LoadingContext &, const std::string)> NodeLoadingFunction;

  NodeLoader();

  std::shared_ptr<ResourceUploader<strc::Node>> loadResource(std::string fname) override;
  std::shared_ptr<NodeUploader> loadNodeFromCompound(std::shared_ptr<config::NodeCompound> comp, const LoadingContext & context);
  std::shared_ptr<NodeUploader> loadNodeFromFile(std::string fname, const LoadingContext & context);

  LoadingResource loadDependencyResource(ResourceLocation location);

  static void registerNodeLoader(std::string type, NodeLoadingFunction func);

private:
  static std::unordered_map<std::string, NodeLoadingFunction> fmap;

};

#endif
