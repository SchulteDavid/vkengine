#include "resources/archiveloader.h"

ArchiveLoader::ArchiveLoader() {

}

ArchiveLoader::~ArchiveLoader() {}

void ArchiveLoader::setCurrentManager(ResourceManager *manager) {
  this->manager = manager;
}

LoadingResource ArchiveLoader::uploadResource(ResourceLocation location, std::shared_ptr<void> uploader) {

  return scheduleSubresourceUpload(manager, location, uploader);
  
}

LoadingResource ArchiveLoader::loadDependency(ResourceLocation location) {

  return scheduleResourceLoad(manager, location);
  
}

LoadingResource ArchiveLoader::createResource(ResourceLocation location, std::shared_ptr<void> uploader) {
  return createSubresource(location, uploader);
}

LoadingResource ArchiveLoader::uploadResource(LoadingResource resource) {
  return scheduleSubresourceUpload(manager, resource);
}
