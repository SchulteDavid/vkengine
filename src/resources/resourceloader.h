#ifndef RESOURCELOADER_H
#define RESOURCELOADER_H

#include <memory>
#include <string>
#include <future>

#include "resourceuploader.h"
#include "util/debug/trace_exception.h"

#include "resources/resourcelocation.h"

class ResourceManager;

struct LoadingStatus {

    bool isLoaded;
    bool isUploaded;
    bool isUseable;

};

struct FutureResource {

  FutureResource() : name("", "") {
  }

  FutureResource(ResourceLocation rl) : name(rl) {};

  ResourceLocation name;
  bool isPresent;
  std::shared_ptr<void> uploader;
  std::shared_ptr<Resource> location;

  /*std::shared_future<void> fut;
    std::promise<void> prom;*/

  std::mutex mut;
  std::condition_variable cond;

  void wait();

  LoadingStatus status;

};

typedef std::shared_ptr<FutureResource> LoadingResource;

LoadingResource scheduleResourceLoad(ResourceManager * manager, ResourceLocation location);
LoadingResource createSubresource(const ResourceLocation & location, std::shared_ptr<void> uploader);
LoadingResource scheduleSubresourceUpload(ResourceManager * manager, LoadingResource res);
LoadingResource scheduleSubresourceUpload(ResourceManager * manager, ResourceLocation location, std::shared_ptr<void> uploader);

namespace res {

class wrong_file_exception : public std::exception {

    public:
        wrong_file_exception(std::string msg) {
            this->msg = msg;
        }

        wrong_file_exception(const char * str) {
            this->msg = std::string(str);
        }

        const char * what() {
            return msg.c_str();
        }

    private:

        std::string msg;

};


}

template <typename T, typename std::enable_if<std::is_base_of<Resource, T>::value>::type* = nullptr> class ResourceLoader {

    public:

        ResourceLoader() {}
        virtual ~ResourceLoader() {}

        virtual std::shared_ptr<ResourceUploader<T>> loadResource(std::string fname) {
            throw dbg::trace_exception("Using default resource loading");
        };

        void setCurrentManager(ResourceManager * manager) {
            this->resourceManager = manager;
        }

    protected:

        ResourceManager * resourceManager;

        LoadingResource loadDependency(ResourceLocation name) {
            return scheduleResourceLoad(resourceManager, name);
        }

        LoadingResource scheduleSubresource(ResourceLocation location, std::shared_ptr<void> uploader) {
            return scheduleSubresourceUpload(resourceManager, location, uploader);
        }

    private:

};

#endif // RESOURCELOADER_H
