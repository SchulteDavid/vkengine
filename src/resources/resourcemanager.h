#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include <mutex>
#include <thread>
#include <queue>
#include <unordered_set>

#include "resource.h"
#include "resourceloader.h"
#include "resourceregistry.h"
#include "resources/archiveloader.h"

class ResourceManager {

public:

  enum ResourceType {

		     RESOURCE_NONE = 0,

		     RESOURCE_MODEL = 1,
		     RESOURCE_SHADER = 2,

  };

  ResourceManager(vkutil::VulkanState & state);
  virtual ~ResourceManager();

  void addRegistry(std::string name, ResourceRegistry<Resource> * registry);
  void addLoader(std::string name, ResourceLoader<Resource> * loader);
  ResourceLoader<Resource> * getLoader(std::string name, int index = 0);

  template<typename T> std::shared_ptr<ResourceUploader<T>> loadResource(ResourceLocation location) {
    if (this->registries.find(location.type) == this->registries.end())
      throw dbg::trace_exception(std::string("No such registry").append(location.type));
    return ((ResourceRegistry<T> *) this->registries[location.type])->load(location);
  }

  template<typename T> std::shared_ptr<T> get(ResourceLocation location) {
    if (this->registries.find(location.type) == this->registries.end()){
      throw dbg::trace_exception(std::string("No such registry ").append(location.type));
    }
    std::shared_ptr<Resource> r = this->registries[location.type]->get(location);
    std::shared_ptr<T> val = std::dynamic_pointer_cast<T>(r);
    if (!val) {
      throw dbg::trace_exception(std::string("Wrong resource type detected in registry ").append(location.type).append(" : ").append(location.filename));
    }
    return val;
  }

  void dropResource(ResourceLocation location);

  bool isLoaded(ResourceLocation location);
  std::shared_ptr<Resource> registerResource(ResourceLocation location, std::shared_ptr<Resource> res);

  void startLoadingThreads(unsigned int threadCount);
  void joinLoadingThreads();
  LoadingResource loadResourceBg(ResourceLocation location);

  void submitUpload(LoadingResource resource);

  void printSummary();

  /// Tries to load location as an archive, returns true if possible
  bool tryArchiveLoad(LoadingResource fres);
  void attachArchiveType(std::string fileEnding, std::shared_ptr<ArchiveLoader> loader);

protected:

private:

  std::unordered_map<std::string, ResourceRegistry<Resource> *> registries;
  std::unordered_map<std::string, std::shared_ptr<ArchiveLoader>> archiveLoader;

  std::mutex loadingQueueMutex;
  std::condition_variable loadingCV;
  std::queue<LoadingResource> loadingQueue;

  std::mutex uploadingQueueMutex;
  std::condition_variable uploadingCV;
  std::queue<LoadingResource> uploadingQueue;

  std::mutex pipelineInfoMutex;
  std::unordered_map<std::string, std::unordered_set<ResourceLocation>> pipelineInfo;

  std::vector<std::thread *> loadingThreads;

  bool keepThreadsRunning;

  vkutil::VulkanState & vulkanState;

  static void threadLoadingFunction(ResourceManager * resourceManager);
  static void threadUploadingFunction(ResourceManager * resourceManager);
  LoadingResource getNextResource();
  LoadingResource getNextUploadingResource();

  void rescheduleUpload(LoadingResource res);
  void rescheduleLoad(LoadingResource res);

  void markResourceInPipeline(LoadingResource res);
  void unmarkResourceInPipeline(LoadingResource res);
  bool isResourceInPipeline(LoadingResource res);


};

#endif // RESOURCEMANAGER_H
