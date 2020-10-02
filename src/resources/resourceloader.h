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

struct ResourceLocation {


  ResourceLocation(std::string type, std::string filename) : type(type), filename(filename), name("") {
    
  }

  ResourceLocation(std::string type, std::string filename, std::string name) : type(type), filename(filename), name(name) {
    
  }
  
  /// Registry in which this should be saved
  const std::string type;

  /// File in which this resource can be found
  const std::string filename;

  /// Name of the resource in the file, can be empty if
  /// it does not apply.
  const std::string name;
  
};

struct FutureResource {

  FutureResource() : name("", "") {
  }

  FutureResource(ResourceLocation rl) : name(rl) {};

  ResourceLocation name;
  bool isPresent;
  std::shared_ptr<void> uploader;
  std::shared_ptr<Resource> location;
  
  std::future<void> fut;
  std::promise<void> prom;

  LoadingStatus status;

};

template <> struct std::hash<ResourceLocation> {
  std::size_t operator()(ResourceLocation const & rl) const {
    std::size_t h1 = std::hash<std::string>{}(rl.filename);
    std::size_t h2 = std::hash<std::string>{}(rl.name);
    return h1 ^ (h2 << 1);
  }
};

inline bool operator==(const ResourceLocation & l1, const ResourceLocation & l2) {
  return l1.type == l2.type && l1.filename == l2.filename && l1.name == l2.name;
}

inline std::ostream & operator<<(std::ostream & stream, ResourceLocation loc) {
  stream << loc.filename;
  if (loc.name != "")
    stream << std::string("::") << loc.name;
  return stream;
}

typedef std::shared_ptr<FutureResource> LoadingResource;

LoadingResource scheduleResourceLoad(ResourceManager * manager, ResourceLocation location);
LoadingResource scheduleSubresourceUpload(ResourceManager * manager, ResourceLocation location, std::shared_ptr<void> uploader);

namespace res {

class wrong_file_exception : public std::exception {

    public:
        wrong_file_exception(std::string msg) {
            this->msg = msg;
        }

        wrong_file_exception(const char * str) {
            this->msg = std::string(str);
        }

        const char * what() {
            return msg.c_str();
        }

    private:

        std::string msg;

};


}

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

        LoadingResource loadDependency(ResourceLocation name) {
            return scheduleResourceLoad(resourceManager, name);
        }

        LoadingResource scheduleSubresource(ResourceLocation location, std::shared_ptr<void> uploader) {
            return scheduleSubresourceUpload(resourceManager, location, uploader);
        }

    private:

};

#endif // RESOURCELOADER_H
