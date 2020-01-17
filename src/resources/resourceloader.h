#ifndef RESOURCELOADER_H
#define RESOURCELOADER_H

#include <memory>

#include "resourceuploader.h"

template <typename T, typename std::enable_if<std::is_base_of<Resource, T>::value>::type* = nullptr> class ResourceLoader {

    public:
        ResourceLoader() {}
        virtual ~ResourceLoader() {}

        virtual std::shared_ptr<ResourceUploader<T>> loadResource(std::string fname) = 0;

    protected:

    private:
};

#endif // RESOURCELOADER_H
