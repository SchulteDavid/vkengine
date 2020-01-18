#include "resourcemanager.h"

#include <iostream>

#include "../render/model.h"
#include "../render/shader.h"

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

    this->registries[name]->addLoader(loader);

}

void ResourceManager::registerResource(std::string regName, std::string name, Resource * res) {
    this->registries[regName]->registerObject(name, res);
}

void ResourceManager::threadLoadingFunction(ResourceManager * resourceManager) {

    while (resourceManager->keepThreadsRunning) {

        FutureResource fres = resourceManager->getNextResource();
        if (!fres.isPresent) continue;

        std::cout << "Loading " << fres.regName << " / " << fres.name << std::endl;

        std::shared_ptr<ResourceUploader<Resource>> uploader = resourceManager->loadResource<Resource>(fres.regName, fres.name);
        Resource * tmpModel = uploader->uploadResource();
        resourceManager->registerResource(fres.regName, fres.name, tmpModel);

        fres.status->isLoaded = true;

        std::cout << "Loading Done" << std::endl;

    }

}

ResourceManager::FutureResource ResourceManager::getNextResource() {

    FutureResource resource;

    loadingQueueMutex.lock();
    if (loadingQueue.empty()) {
        loadingQueueMutex.unlock();
        resource.isPresent = false;
        return resource;
    }

    resource = loadingQueue.front();
    loadingQueue.pop();

    loadingQueueMutex.unlock();
    return resource;

}

std::shared_ptr<ResourceManager::LoadingStatus> ResourceManager::loadResourceBg(std::string regName, std::string name) {

    std::shared_ptr<LoadingStatus> status(new LoadingStatus());
    status->isLoaded = false;

    FutureResource res;
    res.isPresent = true;
    res.regName = regName;
    res.name = name;
    res.status = status;

    loadingQueueMutex.lock();
    loadingQueue.push(res);
    loadingQueueMutex.unlock();

    return res.status;

}

void ResourceManager::startLoadingThreads(unsigned int threadCount) {

    this->keepThreadsRunning = true;

    for (unsigned int i = 0; i < threadCount; ++i) {

        std::thread * th = new std::thread(threadLoadingFunction, this);
        this->loadingThreads.push_back(th);

    }

}

void ResourceManager::joinLoadingThreads() {

    this->keepThreadsRunning = false;

    for (std::thread * th : loadingThreads) {
        th->join();
    }

}
