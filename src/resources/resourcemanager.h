#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include <mutex>
#include <thread>
#include <queue>
#include <unordered_set>

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
        ResourceLoader<Resource> * getLoader(std::string name, int index = 0);

        template<typename T> std::shared_ptr<ResourceUploader<T>> loadResource(std::string regName, std::string name) {
            return ((ResourceRegistry<T> *) this->registries[regName])->load(name);
        }

        template<typename T> std::shared_ptr<T> get(std::string regName, std::string name) {
            return ((ResourceRegistry<T> *) this->registries[regName])->get(name);
        }

        bool isLoaded(std::string regName, std::string name);
        std::shared_ptr<Resource> registerResource(std::string regName, std::string name, std::shared_ptr<Resource> res);

        void startLoadingThreads(unsigned int threadCount);
        void joinLoadingThreads();
        LoadingResource loadResourceBg(std::string regName, std::string name);

        void submitUpload(LoadingResource resource);

        void printSummary();

    protected:

    private:

        std::unordered_map<std::string, ResourceRegistry<Resource> *> registries;
        std::mutex loadingQueueMutex;
        std::queue<LoadingResource> loadingQueue;
        std::mutex uploadingQueueMutex;
        std::queue<LoadingResource> uploadingQueue;

        std::mutex pipelineInfoMutex;
        std::unordered_map<std::string, std::unordered_set<std::string>> pipelineInfo;

        std::vector<std::thread *> loadingThreads;

        bool keepThreadsRunning;

        static void threadLoadingFunction(ResourceManager * resourceManager);
        static void threadUploadingFunction(ResourceManager * resourceManager);
        LoadingResource getNextResource();
        LoadingResource getNextUploadingResource();

        void rescheduleUpload(LoadingResource res);
        void rescheduleLoad(LoadingResource res);

        void markResourceInPipeline(LoadingResource res);
        void unmarkResourceInPipeline(LoadingResource res);
        bool isResourceInPipeline(LoadingResource res);


};

#endif // RESOURCEMANAGER_H
