#ifndef RESOURCELOADER_H
#define RESOURCELOADER_H

#include <memory>
#include <string>

#include "resourceuploader.h"

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

    LoadingStatus status;

};

typedef std::shared_ptr<FutureResource> LoadingResource;

LoadingResource scheduleResourceLoad(ResourceManager * manager, std::string regName, std::string name);

template <typename T, typename std::enable_if<std::is_base_of<Resource, T>::value>::type* = nullptr> class ResourceLoader {

    public:

        ResourceLoader() {}
        virtual ~ResourceLoader() {}

        virtual std::shared_ptr<ResourceUploader<T>> loadResource(std::string fname) = 0;

        void setCurrentManager(ResourceManager * manager) {
            this->resourceManager = manager;
        }

    protected:

        LoadingResource loadDependency(std::string regName, std::string name) {
            return scheduleResourceLoad(resourceManager, regName, name);
        }

    private:

        ResourceManager * resourceManager;

};

#endif // RESOURCELOADER_H
