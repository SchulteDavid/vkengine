#ifndef RESOURCEREGISTRY_H
#define RESOURCEREGISTRY_H

#include <type_traits>
#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "resource.h"
#include "resourceloader.h"

#include <iostream>

#include "util/debug/trace_exception.h"
#include "util/debug/logger.h"

template <typename T, typename std::enable_if<std::is_base_of<Resource, T>::value>::type* = nullptr> class ResourceRegistry {

public:
  ResourceRegistry(){

  }

  virtual ~ResourceRegistry(){

  }

  /// Get a resource by its name
  std::shared_ptr<T> get(ResourceLocation name) {

    if (objects.find(name) == objects.end()) {
      throw dbg::trace_exception(std::string("Unable to find resource '").append(name.filename).append("'"));
    }
    return objects[name];
  }

  std::shared_ptr<T> registerObject(ResourceLocation name, std::shared_ptr<T> obj) {

    this->objects[name] = obj;
    return obj;

  }

  void addLoader(ResourceLoader<T> * loader) {

    this->loaders.push_back(loader);

  }

  ResourceLoader<T> * getLoader(int index) {
    return loaders[index];
  }

  std::shared_ptr<ResourceUploader<T>> load(ResourceLocation name) {

    for (ResourceLoader<T> * l : loaders) {

      lout << "Loader " << l << std::endl;

      try {
	return l->loadResource(name.filename);
      } catch (res::wrong_file_exception e) {
	lerr << e.what() << std::endl;
      }

    }

    return nullptr;

  }

  bool isLoaded(ResourceLocation name) {
    return objects.find(name) != objects.end();
  }

  void printSummary() {
    for (auto const & o : objects) {
      lout << "\t" << o.first << std::endl;
    }
  }

protected:

private:

  std::unordered_map<ResourceLocation, std::shared_ptr<T>> objects;
  std::vector<ResourceLoader<T> *> loaders;


};

#endif // RESOURCEREGISTRY_H
