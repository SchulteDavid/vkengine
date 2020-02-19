#ifndef RESOURCEREGISTRY_H
#define RESOURCEREGISTRY_H

#include <type_traits>
#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "resource.h"

#include <iostream>

#include "util/debug/trace_exception.h"

template <typename T, typename std::enable_if<std::is_base_of<Resource, T>::value>::type* = nullptr> class ResourceRegistry {

    public:
        ResourceRegistry(){

        }

        virtual ~ResourceRegistry(){

        }

        /// Get a resource by its name
        std::shared_ptr<T> get(std::string name) {

            if (objects.find(name) == objects.end()) {
                throw dbg::trace_exception(std::string("Unable to find resource '").append(name).append("'"));
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

        ResourceLoader<T> * getLoader(int index) {
            return loaders[index];
        }

        std::shared_ptr<ResourceUploader<T>> load(std::string name) {

            for (ResourceLoader<T> * l : loaders) {

                std::cout << "Loader " << l << std::endl;

                try {
                    return l->loadResource(name);
                } catch (dbg::trace_exception e) {
                    //std::cerr << e.what() << std::endl;
                } catch (std::runtime_error e) {
                    //std::cerr << e.what() << std::endl;
                } catch (std::exception e) {
                    //std::cerr << e.what() << std::endl;
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
