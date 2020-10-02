#include "resourcemanager.h"

#include <iostream>

#include "../render/model.h"
#include "../render/shader.h"

#include "util/debug/trace_exception.h"
#include "util/debug/logger.h"

ResourceManager::ResourceManager(unsigned int regTypes) {

  /*if (regTypes & RESOURCE_MODEL) {
    this->addRegistry("Model", (ResourceRegistry<Resource> *) new ResourceRegistry<Model>());
    }

    if (regTypes & RESOURCE_SHADER) {
    this->addRegistry("Shader", (ResourceRegistry<Resource> *) new ResourceRegistry<Shader>());
    }*/

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

std::shared_ptr<Resource> ResourceManager::registerResource(ResourceLocation location, std::shared_ptr<Resource> res) {
  return this->registries[location.type]->registerObject(location, res);
}

bool ResourceManager::isLoaded(ResourceLocation location) {

  if (registries.find(location.type) == registries.end())
    return false;

  return this->registries[location.type]->isLoaded(location);

}

void ResourceManager::submitUpload(LoadingResource resource) {

  this->rescheduleUpload(resource);

}

#define dbg_log (std::cout << "[ " << __FILE__ << " : " << __PRETTY_FUNCTION__ << " ] ")

void ResourceManager::threadLoadingFunction(ResourceManager * resourceManager) {

  try {

    while (resourceManager->keepThreadsRunning) {

      //std::cout << "Getting new Resource to load" << std::endl;
      LoadingResource fres = resourceManager->getNextResource();
      if (!fres->isPresent) continue;

      if (resourceManager->isLoaded(fres->name)) {

	logger(std::cout) << "Skipping loading of " << fres->name << " is already loaded" << std::endl;

	fres->location = resourceManager->get<Resource>(fres->name);
	fres->status.isLoaded = true;
	fres->status.isUploaded = true;
	fres->status.isUseable = true;
	fres->prom.set_value();

	continue;

      }

      if (resourceManager->isResourceInPipeline(fres)){

	/// Reschedule loading for later checks.
	//std::cout << "Reschedule of " << fres->name << std::endl;
	resourceManager->rescheduleLoad(fres);
	continue;

      }

      logger(std::cout) << "Loading " << fres->name << std::endl;

      resourceManager->markResourceInPipeline(fres);

      std::shared_ptr<ResourceUploader<Resource>> uploader = resourceManager->loadResource<Resource>(fres->name);
      if (!uploader)
	throw dbg::trace_exception(std::string("No Correct loader for ").append(fres->name.filename));
      fres->uploader = uploader;

      logger(std::cout) << fres->name << " : " << uploader << std::endl;

      fres->status.isLoaded = true;

      resourceManager->rescheduleUpload(fres);



      logger(std::cout) << "Loading Done " << fres->name << std::endl;

    }
  } catch (std::exception & e) {

    logger(std::cerr) << "Exception while loading" << std::endl;
    logger(std::cerr) << e.what() << std::endl;
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

    if (resourceManager->isLoaded(fres->name)) {

      fres->location = resourceManager->get<Resource>(fres->name);
      fres->status.isLoaded = true;
      fres->status.isUploaded = true;
      fres->status.isUseable = true;
      fres->prom.set_value();

      continue;

    }

    if (!fres->status.isLoaded)
      throw dbg::trace_exception("Non-loaded element in uploading queue");

    if (!(fres->uploader))
      throw dbg::trace_exception(std::string("Missing uploader for future resource ").append(fres->name.type).append(" / ").append(fres->name.filename));

    ResourceUploader<Resource>* tmpUploader = (ResourceUploader<Resource> *) fres->uploader.get();
	
    if (!tmpUploader->uploadReady()) {

      resourceManager->rescheduleUpload(fres);
      continue;

    }

    logger(std::cout) << "Uploading " << fres->name << std::endl;

    std::shared_ptr<Resource> tmpResource = tmpUploader->uploadResource();
    if (!tmpResource) {
      logger(std::cerr) << "Resource " << fres->name << " is empty pointer" << std::endl;
      throw dbg::trace_exception("Null pointer for uploaded resource");
    }
    fres->location = resourceManager->registerResource(fres->name, tmpResource);

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

LoadingResource ResourceManager::loadResourceBg(ResourceLocation location) {

  LoadingStatus status;
  status.isLoaded = false;
  status.isUploaded = false;
  status.isUseable = false;

  std::shared_ptr<FutureResource> res(new FutureResource(location));
  res->isPresent = true;
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

    logger(std::cout) << x.first << ": " << std::endl;
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
  this->pipelineInfo[res->name.type].insert(this->pipelineInfo[res->name.type].begin(), res->name);
  pipelineInfoMutex.unlock();
}

void ResourceManager::unmarkResourceInPipeline(LoadingResource res) {
  pipelineInfoMutex.lock();
  this->pipelineInfo[res->name.type].erase(res->name);
  pipelineInfoMutex.unlock();
}

bool ResourceManager::isResourceInPipeline(LoadingResource res) {

  pipelineInfoMutex.lock();

  auto it = this->pipelineInfo[res->name.type].find(res->name);

  pipelineInfoMutex.unlock();

  return it != this->pipelineInfo[res->name.type].end();

}

LoadingResource scheduleResourceLoad(ResourceManager * manager, ResourceLocation location) {
  return manager->loadResourceBg(location);
}

LoadingResource scheduleSubresourceUpload(ResourceManager * manager, ResourceLocation location, std::shared_ptr<void> uploader) {

  LoadingResource res(new FutureResource(location));
  res->isPresent = true;
  res->uploader = uploader;

  res->status.isUseable = false;
  res->status.isLoaded = true;
  res->status.isUploaded = false;

  res->status.isLoaded = true;

  manager->submitUpload(res);

  return res;

}
