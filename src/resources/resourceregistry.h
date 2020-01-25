#ifndef RESOURCEREGISTRY_H
#define RESOURCEREGISTRY_H

#include <type_traits>
#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "resource.h"

#include <iostream>

template <typename T, typename std::enable_if<std::is_base_of<Resource, T>::value>::type* = nullptr> class ResourceRegistry {

    public:
        ResourceRegistry(){

        }

        virtual ~ResourceRegistry(){

        }

        /// Get a resource by its name
        std::shared_ptr<T> get(std::string name) {

            if (objects.find(name) == objects.end()) {
                throw std::runtime_error(std::string("Unable to find resource '").append(name).append("'"));
            }
            return objects[name];
        }

        /// Register a resource to a name
        std::shared_ptr<T> registerObject(std::string name, T * obj) {

            std::shared_ptr<T> ptr(obj);

            this->objects[name] = ptr;

            return ptr;

        }

        void addLoader(ResourceLoader<T> * loader) {

            this->loaders.push_back(loader);

        }

        std::shared_ptr<ResourceUploader<T>> load(std::string name) {

            for (ResourceLoader<T> * l : loaders) {

                try {
                    return l->loadResource(name);
                } catch (std::exception e) {

                }

            }

            return nullptr;

        }

        bool isLoaded(std::string name) {
            return objects.find(name) != objects.end();
        }

        void printSummary() {
            for (auto const & o : objects) {
                std::cout << "\t" << o.first << std::endl;
            }
        }

    protected:

    private:

        std::unordered_map<std::string, std::shared_ptr<T>> objects;
        std::vector<ResourceLoader<T> *> loaders;


};

#endif // RESOURCEREGISTRY_H
