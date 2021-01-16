#ifndef RESOURCEUPLOADER_H
#define RESOURCEUPLOADER_H

#include <type_traits>
#include <memory>

#include "resource.h"
#include "render/util/vkutil.h"

class ResourceManager;

template <typename T, typename std::enable_if<std::is_base_of<Resource, T>::value>::type* = nullptr> class ResourceUploader {

public:
  
  ResourceUploader(){
    
  }
  
  virtual ~ResourceUploader(){
    
  }
  
  virtual std::shared_ptr<T> uploadResource(vkutil::VulkanState & state, ResourceManager * manager) = 0;
  virtual bool uploadReady() = 0;

protected:
  
private:

};

#endif // RESOURCEUPLOADER_H
