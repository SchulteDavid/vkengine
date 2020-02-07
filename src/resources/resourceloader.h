#ifndef RESOURCELOADER_H
#define RESOURCELOADER_H

#include <memory>
#include <string>
#include <future>

#include "resourceuploader.h"
#include "util/debug/trace_exception.h"

class ResourceManager;

struct LoadingStatus {

    bool isLoaded;
    bool isUploaded;
    bool isUseable;

};

struct FutureResource {

    std::string regName;
    std::string name;
    bool isPresent;
    std::shared_ptr<ResourceUploader<Resource>> uploader;
    std::shared_ptr<Resource> location;

    std::future<void> fut;
    std::promise<void> prom;

    LoadingStatus status;

};

typedef std::shared_ptr<FutureResource> LoadingResource;

LoadingResource scheduleResourceLoad(ResourceManager * manager, std::string regName, std::string name);
LoadingResource scheduleSubresourceUpload(ResourceManager * manager, std::string regName, std::string name, std::shared_ptr<ResourceUploader<Resource>> uploader);

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

        LoadingResource loadDependency(std::string regName, std::string name) {
            return scheduleResourceLoad(resourceManager, regName, name);
        }

        LoadingResource scheduleSubresource(std::string regName, std::string name, std::shared_ptr<ResourceUploader<Resource>> uploader) {
            return scheduleSubresourceUpload(resourceManager, regName, name, uploader);
        }

    private:

};

#endif // RESOURCELOADER_H
