#include "resourcemanager.h"

#include <iostream>

#include "../render/model.h"
#include "../render/shader.h"

#include "util/debug/trace_exception.h"
#include "util/debug/logger.h"

ResourceManager::ResourceManager(vkutil::VulkanState & state) : vulkanState(state) {

  

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

  res->setLocation(location);
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

bool ResourceManager::tryArchiveLoad(LoadingResource fres) {

  auto loc = fres->name.filename.find('.');
  std::string ending = fres->name.filename.substr(loc+1, std::string::npos);
  std::transform(ending.begin(), ending.end(), ending.begin(), [](unsigned char c){ return std::tolower(c); });

  std::cout << "Checking if ending " << ending << " has ArchiveLoaders" << std::endl;
  
  if (archiveLoader.find(ending) != archiveLoader.end()) {
    
    try {

      std::shared_ptr<ArchiveLoader> loader = archiveLoader[ending];
      if (!loader->canLoad(fres->name))
	return false;
      
      LoadingResource res = loader->load(fres->name);
      fres->uploader = res->uploader;
      //fres->cond = res->cond;
      //*fres = *res;

      return true;
      
    } catch (std::exception & e) {
      lerr << e.what() << std::endl;
      return false;
    }
  }
  return false;
  
}

void ResourceManager::attachArchiveType(std::string fileEnding, std::shared_ptr<ArchiveLoader> loader) {
  std::transform(fileEnding.begin(), fileEnding.end(), fileEnding.begin(), [](unsigned char c){ return std::tolower(c); });
  archiveLoader[fileEnding] = loader;
  loader->setCurrentManager(this);
}

#define dbg_log (lout << "[ " << __FILE__ << " : " << __PRETTY_FUNCTION__ << " ] ")

void ResourceManager::threadLoadingFunction(ResourceManager * resourceManager) {

  try {

    while (resourceManager->keepThreadsRunning) {

      //lout << "Getting new Resource to load" << std::endl;
      LoadingResource fres = resourceManager->getNextResource();
      if (!fres->isPresent) continue;

      if (resourceManager->isLoaded(fres->name)) {

        lout << "Skipping loading of " << fres->name << " is already loaded" << std::endl;

        fres->location = resourceManager->get<Resource>(fres->name);
        fres->status.isLoaded = true;
        fres->status.isUploaded = true;
        fres->status.isUseable = true;
	fres->cond.notify_all();
        //fres->prom.set_value();
	//fres->fut.

        continue;

      }

      if (resourceManager->isResourceInPipeline(fres)){

        /// Reschedule loading for later checks.
        //lout << "Reschedule of " << fres->name << std::endl;
        resourceManager->rescheduleLoad(fres);
        continue;

      }

      lout << "Loading " << fres->name << std::endl;

      resourceManager->markResourceInPipeline(fres);

      if (!resourceManager->tryArchiveLoad(fres)) {

	std::shared_ptr<ResourceUploader<Resource>> uploader = resourceManager->loadResource<Resource>(fres->name);
	if (!uploader)
	  throw dbg::trace_exception(std::string("No Correct loader for ").append(fres->name.filename));
	fres->uploader = uploader;

	lout << fres->name << " : " << uploader << std::endl;

      }

      fres->status.isLoaded = true;

      resourceManager->rescheduleUpload(fres);



      lout << "Loading Done " << fres->name << std::endl;

    }
  } catch (std::exception & e) {

    lerr << "Exception while loading" << std::endl;
    lerr << e.what() << std::endl;
    exit(1);

  }

}

void ResourceManager::rescheduleUpload(LoadingResource res) {

  //uploadingQueueMutex.lock();
  {
    std::lock_guard<std::mutex> guard(uploadingQueueMutex);
    uploadingQueue.push(res);
  }

  uploadingCV.notify_one();

}

void ResourceManager::rescheduleLoad(LoadingResource res) {

  {
    std::lock_guard<std::mutex> guard(loadingQueueMutex);
    loadingQueue.push(res);
  }
  loadingCV.notify_one();

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
      fres->cond.notify_all();
      fres->location->setLocation(fres->name);
      //fres->prom.set_value();

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

    lout << "Uploading " << fres->name << std::endl;

    std::shared_ptr<Resource> tmpResource = tmpUploader->uploadResource(resourceManager->vulkanState, resourceManager);
    if (!tmpResource) {
      lerr << "Resource " << fres->name << " is empty pointer" << std::endl;
      throw dbg::trace_exception("Null pointer for uploaded resource");
    }
    fres->location = resourceManager->registerResource(fres->name, tmpResource);

    fres->status.isUploaded = true;
    fres->status.isUseable  = true;
    fres->cond.notify_all();
    fres->location->setLocation(fres->name);
    //fres->prom.set_value();

    resourceManager->unmarkResourceInPipeline(fres);

  }

}

LoadingResource ResourceManager::getNextResource() {

  std::shared_ptr<FutureResource> resource;

  std::unique_lock<std::mutex> lock(loadingQueueMutex);

  loadingCV.wait(lock, [&] {return !loadingQueue.empty() || !keepThreadsRunning;});

  /*if (loadingQueue.empty()) {


    resource = std::shared_ptr<FutureResource>(new FutureResource());

    resource->isPresent = false;
    return resource;
    }*/

  resource = loadingQueue.front();
  loadingQueue.pop();

  return resource;

}

LoadingResource ResourceManager::getNextUploadingResource() {

  std::shared_ptr<FutureResource> resource;

  std::unique_lock<std::mutex> lock(uploadingQueueMutex);

  uploadingCV.wait(lock, [&] {return !uploadingQueue.empty() || !keepThreadsRunning;});

  /*if (uploadingQueue.empty()) {

    resource = std::shared_ptr<FutureResource>(new FutureResource());
    resource->isPresent = false;
    return resource;
    }*/

  resource = uploadingQueue.front();
  uploadingQueue.pop();


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
  //res->fut = std::shared_future<void>(res->prom.get_future());

  {
    std::lock_guard<std::mutex> guard(loadingQueueMutex);
    loadingQueue.push(res);
  }
  loadingCV.notify_one();

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

    lout << x.first << ": " << std::endl;
    x.second->printSummary();

  }

}

void ResourceManager::joinLoadingThreads() {

  this->keepThreadsRunning = false;

  lout << "Notifying conditions" << std::endl;
  
  loadingCV.notify_all();
  uploadingCV.notify_all();

  lout << "Joining Threads" << std::endl;

  for (std::thread * th : loadingThreads) {
    lout << th << std::endl;
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

void ResourceManager::dropResource(ResourceLocation location) {

  this->registries[location.type]->drop(location);
  
}

LoadingResource scheduleResourceLoad(ResourceManager * manager, ResourceLocation location) {
  return manager->loadResourceBg(location);
}

LoadingResource createSubresource(const ResourceLocation & location, std::shared_ptr<void> uploader) {

  LoadingResource res(new FutureResource(location));
  res->isPresent = true;
  res->uploader = uploader;
  res->name = location;

  res->status.isUseable = false;
  res->status.isLoaded = true;
  res->status.isUploaded = false;

  return res;
  
}

LoadingResource scheduleSubresourceUpload(ResourceManager * manager, LoadingResource res) {
  manager->submitUpload(res);
  return res;
}

LoadingResource scheduleSubresourceUpload(ResourceManager * manager, ResourceLocation location, std::shared_ptr<void> uploader) {

  LoadingResource res(new FutureResource(location));
  res->isPresent = true;
  res->uploader = uploader;
  res->name = location;

  res->status.isUseable = false;
  res->status.isLoaded = true;
  res->status.isUploaded = false;

  res->status.isLoaded = true;

  manager->submitUpload(res);

  return res;

}

ResourceLocation ResourceLocation::parse(std::string type, std::string data) {

  auto pos = data.find("::");
  if (pos == std::string::npos) {
    return ResourceLocation(type, data);
  }
  std::string fname = data.substr(0, pos);
  std::string name = data.substr(pos+2, std::string::npos);

  return ResourceLocation(type, fname, name);

}

void FutureResource::wait() {
  std::unique_lock<std::mutex> lock(mut);

  std::cout << "Waiting for " << this << std::endl;
  
  cond.wait(lock, [&] {return status.isUseable;});
}

void Resource::setLocation(const ResourceLocation & location) {
  this->location = location;
}

const ResourceLocation & Resource::getLocation() {
  return location;
}
