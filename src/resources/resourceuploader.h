#ifndef RESOURCEUPLOADER_H
#define RESOURCEUPLOADER_H

#include <type_traits>

#include "resource.h"

template <typename T, typename std::enable_if<std::is_base_of<Resource, T>::value>::type* = nullptr> class ResourceUploader {

    public:

        ResourceUploader(){

        }

        virtual ~ResourceUploader(){

        }

        virtual T * uploadResource() = 0;

    protected:

    private:

};

#endif // RESOURCEUPLOADER_H
