#ifndef ARCHIVE_LOADER_H
#define ARCHIVE_LOADER_H

#include "resources/resourcelocation.h"
#include "resources/resourceloader.h"

class ResourceManager;

class ArchiveLoader {

 public:
  ArchiveLoader();
  virtual ~ArchiveLoader();

  /**
     Should return true if this can load the archive required
     for the given resource-location.
   */
  virtual bool canLoad(ResourceLocation location) = 0;

  /**
     This will load the resources present in this archive.
     
     Returns a LoadingResource with a future to be
     triggered to allow for waiting on it.
   */
  virtual LoadingResource load(ResourceLocation location) = 0;

  void setCurrentManager(ResourceManager * manager);

 protected:

  LoadingResource uploadResource(ResourceLocation location, std::shared_ptr<void> resource);
  LoadingResource loadDependency(ResourceLocation location);
  LoadingResource createResource(ResourceLocation location, std::shared_ptr<void> resource);
  LoadingResource uploadResource(LoadingResource res);

 private:
  ResourceManager * manager;
  
};

#endif
