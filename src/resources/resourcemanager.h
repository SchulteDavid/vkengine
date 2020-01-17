#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include "resource.h"
#include "resourceloader.h"
#include "resourceregistry.h"

class ResourceManager {

    public:

        enum ResourceType {

            RESOURCE_NONE = 0,

            RESOURCE_MODEL = 1,
            RESOURCE_SHADER = 2,

        };

        ResourceManager(unsigned int regTypes);
        virtual ~ResourceManager();

        void addRegistry(std::string name, ResourceRegistry<Resource> * registry);
        void addLoader(std::string name, ResourceLoader<Resource> * loader);

        template<typename T> std::shared_ptr<ResourceUploader<T>> loadResource(std::string regName, std::string name) {
            return ((ResourceRegistry<T> *) this->registries[regName])->load(name);
        }

        template<typename T> std::shared_ptr<T> get(std::string regName, std::string name) {
            return ((ResourceRegistry<T> *) this->registries[regName])->get(name);
        }

        void registerResource(std::string regName, std::string name, Resource * res);

    protected:

    private:

        std::unordered_map<std::string, ResourceRegistry<Resource> *> registries;


};

#endif // RESOURCEMANAGER_H
