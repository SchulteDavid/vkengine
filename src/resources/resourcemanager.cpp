#include "resourcemanager.h"

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
