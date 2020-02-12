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
    this->pipelineInfo[name] = {};

}

void ResourceManager::addLoader(std::string name, ResourceLoader<Resource> * loader) {

    if (this->registries.find(name) == this->registries.end()) {
        throw dbg::trace_exception("Unable to add loader to non-existent registry");
    }

    this->registries[name]->addLoader(loader);

    loader->setCurrentManager(this);

}

ResourceLoader<Resource> * ResourceManager::getLoader(std::string name, int index) {

    if (this->registries.find(name) == this->registries.end()) {
        throw dbg::trace_exception("Unable to add loader to non-existent registry");
    }

    return this->registries[name]->getLoader(index);

}

std::shared_ptr<Resource> ResourceManager::registerResource(std::string regName, std::string name, Resource * res) {
    return this->registries[regName]->registerObject(name, res);
}

bool ResourceManager::isLoaded(std::string regName, std::string name) {

    if (registries.find(regName) == registries.end())
        return false;

    return this->registries[regName]->isLoaded(name);

}

void ResourceManager::submitUpload(LoadingResource resource) {

    this->rescheduleUpload(resource);

}

#define dbg_log (std::cout << "[ " << __FILE__ << " : " << __PRETTY_FUNCTION__ << " ] ")

void ResourceManager::threadLoadingFunction(ResourceManager * resourceManager) {

    try {

        while (resourceManager->keepThreadsRunning) {

            LoadingResource fres = resourceManager->getNextResource();
            if (!fres->isPresent) continue;

            if (resourceManager->isLoaded(fres->regName, fres->name)) {

                std::cout << "Skipping loading of " << fres->name << " is already loaded" << std::endl;

                fres->status.isLoaded = true;
                fres->status.isUploaded = true;
                fres->status.isUseable = true;
                fres->location = resourceManager->get<Resource>(fres->regName, fres->name);

                continue;

            }

            if (resourceManager->isResourceInPipeline(fres)){

                /// Reschedule loading for later checks.
                //std::cout << "Reschedule of " << fres->name << std::endl;
                resourceManager->rescheduleLoad(fres);
                continue;

            }

            std::cout << "Loading " << fres->regName << " / " << fres->name << std::endl;

            resourceManager->markResourceInPipeline(fres);

            std::shared_ptr<ResourceUploader<Resource>> uploader = resourceManager->loadResource<Resource>(fres->regName, fres->name);
            if (!uploader)
                throw dbg::trace_exception(std::string("No Correct loader for ").append(fres->name));
            fres->uploader = uploader;

            std::cout << fres->name << " : " << uploader << std::endl;

            fres->status.isLoaded = true;

            resourceManager->rescheduleUpload(fres);



            std::cout << "Loading Done " << fres->name << std::endl;

        }
    } catch (std::exception e) {

        std::cerr << "Exception while loading" << std::endl;
        std::cerr << e.what() << std::endl;
        exit(1);

    }

}

void ResourceManager::rescheduleUpload(LoadingResource res) {

    uploadingQueueMutex.lock();
    uploadingQueue.push(res);
    uploadingQueueMutex.unlock();

}

void ResourceManager::rescheduleLoad(LoadingResource res) {

    loadingQueueMutex.lock();
    loadingQueue.push(res);
    loadingQueueMutex.unlock();

}

void ResourceManager::threadUploadingFunction(ResourceManager * resourceManager) {

    while (resourceManager->keepThreadsRunning) {

        std::shared_ptr<FutureResource> fres = resourceManager->getNextUploadingResource();
        if (!fres || !fres->isPresent) continue;

        if (resourceManager->isLoaded(fres->regName, fres->name)) continue;

        if (!fres->status.isLoaded)
            throw dbg::trace_exception("Non-loaded element in uploading queue");

        if (!(fres->uploader))
            throw dbg::trace_exception(std::string("Missing uploader for future resource ").append(fres->regName).append(" / ").append(fres->name));

        if (!fres->uploader->uploadReady()) {

            resourceManager->rescheduleUpload(fres);
            continue;

        }

        std::cout << "Uploading " << fres->name << std::endl;

        Resource * tmpResource = fres->uploader->uploadResource();
        fres->location = resourceManager->registerResource(fres->regName, fres->name, tmpResource);

        fres->status.isUploaded = true;
        fres->status.isUseable  = true;

        fres->prom.set_value();

        resourceManager->unmarkResourceInPipeline(fres);

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
    res->fut = res->prom.get_future();

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

void ResourceManager::markResourceInPipeline(LoadingResource res) {
    pipelineInfoMutex.lock();
    this->pipelineInfo[res->regName].insert(this->pipelineInfo[res->regName].begin(), res->name);
    pipelineInfoMutex.unlock();
}

void ResourceManager::unmarkResourceInPipeline(LoadingResource res) {
    pipelineInfoMutex.lock();
    this->pipelineInfo[res->regName].erase(res->name);
    pipelineInfoMutex.unlock();
}

bool ResourceManager::isResourceInPipeline(LoadingResource res) {

    pipelineInfoMutex.lock();

    auto it = this->pipelineInfo[res->regName].find(res->name);

    pipelineInfoMutex.unlock();

    return it != this->pipelineInfo[res->regName].end();

}

LoadingResource scheduleResourceLoad(ResourceManager * manager, std::string rName, std::string name) {
    return manager->loadResourceBg(rName, name);
}

LoadingResource scheduleSubresourceUpload(ResourceManager * manager, std::string regName, std::string name, std::shared_ptr<ResourceUploader<Resource>> uploader) {

    LoadingResource res(new FutureResource());
    res->regName = regName;
    res->name = name;
    res->isPresent = true;
    res->uploader = uploader;

    res->status.isLoaded = true;

    manager->submitUpload(res);

    return res;

}
