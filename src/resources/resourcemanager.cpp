#include "resourcemanager.h"

#include <iostream>

#include "../render/model.h"
#include "../render/shader.h"

#include "util/debug/trace_exception.h"

ResourceManager::ResourceManager(unsigned int regTypes) {

    if (regTypes & RESOURCE_MODEL) {
        this->addRegistry("Model", (ResourceRegistry<Resource> *) new ResourceRegistry<Model>());
    }

    if (regTypes & RESOURCE_SHADER) {
        this->addRegistry("Shader", (ResourceRegistry<Resource> *) new ResourceRegistry<Shader>());
    }

}

ResourceManager::~ResourceManager() {
    //dtor
}

void ResourceManager::addRegistry(std::string name, ResourceRegistry<Resource> * registry) {

    this->registries[name] = registry;

}

void ResourceManager::addLoader(std::string name, ResourceLoader<Resource> * loader) {

    if (this->registries.find(name) == this->registries.end()) {
        throw dbg::trace_exception("Unable to add loader to non-existent registry");
    }

    this->registries[name]->addLoader(loader);

    loader->setCurrentManager(this);

}

std::shared_ptr<Resource> ResourceManager::registerResource(std::string regName, std::string name, Resource * res) {
    return this->registries[regName]->registerObject(name, res);
}

bool ResourceManager::isLoaded(std::string regName, std::string name) {

    if (registries.find(regName) == registries.end())
        return false;

    return this->registries[regName]->isLoaded(name);

}

void ResourceManager::threadLoadingFunction(ResourceManager * resourceManager) {

    while (resourceManager->keepThreadsRunning) {

        LoadingResource fres = resourceManager->getNextResource();
        if (!fres->isPresent) continue;

        if (resourceManager->isLoaded(fres->regName, fres->name)) continue;

        std::cout << "Loading " << fres->regName << " / " << fres->name << std::endl;

        std::shared_ptr<ResourceUploader<Resource>> uploader = resourceManager->loadResource<Resource>(fres->regName, fres->name);
        fres->uploader = uploader;
        /*Resource * tmpModel = uploader->uploadResource();
        fres->location = resourceManager->registerResource(fres->regName, fres->name, tmpModel);*/

        fres->status.isLoaded = true;

        resourceManager->rescheduleUpload(fres);

        /// TODO TEMP:
        //fres->status.isUseable = true;

        std::cout << "Loading Done" << std::endl;

    }

}

void ResourceManager::rescheduleUpload(LoadingResource res) {

    uploadingQueueMutex.lock();
    uploadingQueue.push(res);
    uploadingQueueMutex.unlock();

}

void ResourceManager::threadUploadingFunction(ResourceManager * resourceManager) {

    while (resourceManager->keepThreadsRunning) {

        std::shared_ptr<FutureResource> fres = resourceManager->getNextUploadingResource();
        if (!fres->isPresent) continue;

        if (!fres->status.isLoaded)
            throw dbg::trace_exception("Non-loaded element in uploading queue");

        if (!fres->uploader->uploadReady()) {

            resourceManager->rescheduleUpload(fres);
            continue;

        }

        Resource * tmpResource = fres->uploader->uploadResource();
        fres->location = resourceManager->registerResource(fres->regName, fres->name, tmpResource);

        fres->status.isUploaded = true;
        fres->status.isUseable  = true;

    }

}

LoadingResource ResourceManager::getNextResource() {

    std::shared_ptr<FutureResource> resource;

    loadingQueueMutex.lock();
    if (loadingQueue.empty()) {
        loadingQueueMutex.unlock();

        resource = std::shared_ptr<FutureResource>(new FutureResource());

        resource->isPresent = false;
        return resource;
    }

    resource = loadingQueue.front();
    loadingQueue.pop();

    loadingQueueMutex.unlock();
    return resource;

}

LoadingResource ResourceManager::getNextUploadingResource() {

    std::shared_ptr<FutureResource> resource;

    uploadingQueueMutex.lock();
    if (uploadingQueue.empty()) {
        uploadingQueueMutex.unlock();
        resource = std::shared_ptr<FutureResource>(new FutureResource());
        resource->isPresent = false;
        return resource;
    }

    resource = uploadingQueue.front();
    uploadingQueue.pop();

    uploadingQueueMutex.unlock();
    return resource;

}

LoadingResource ResourceManager::loadResourceBg(std::string regName, std::string name) {

    LoadingStatus status;
    status.isLoaded = false;
    status.isUploaded = false;
    status.isUseable = false;

    std::shared_ptr<FutureResource> res(new FutureResource());
    res->isPresent = true;
    res->regName = regName;
    res->name = name;
    res->status = status;

    loadingQueueMutex.lock();
    loadingQueue.push(res);
    loadingQueueMutex.unlock();

    return res;

}

void ResourceManager::startLoadingThreads(unsigned int threadCount) {

    this->keepThreadsRunning = true;

    for (unsigned int i = 0; i < threadCount; ++i) {

        std::thread * th = new std::thread(threadLoadingFunction, this);
        this->loadingThreads.push_back(th);

    }

    for (unsigned int i = 0; i < threadCount; ++i) {

        std::thread * th = new std::thread(threadUploadingFunction, this);
        this->loadingThreads.push_back(th);

    }

}

void ResourceManager::printSummary() {

    for (auto const & x : this->registries) {

        std::cout << x.first << ": " << std::endl;
        x.second->printSummary();

    }

}

void ResourceManager::joinLoadingThreads() {

    this->keepThreadsRunning = false;

    for (std::thread * th : loadingThreads) {
        th->join();
    }

}

LoadingResource scheduleResourceLoad(ResourceManager * manager, std::string rName, std::string name) {
    return manager->loadResourceBg(rName, name);
}
