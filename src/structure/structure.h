#ifndef STRUCTURE_H
#define STRUCTURE_H

#include <memory>

#include "resources/resource.h"
#include "render/material.h"
#include "render/model.h"

class Structure : public Resource {

    public:
        Structure(std::shared_ptr<Model> model, std::shared_ptr<Material> material);
        virtual ~Structure();

        std::shared_ptr<Model> getModel();
        std::shared_ptr<Material> getMaterial();

    protected:

    private:

        std::shared_ptr<Material> material;
        std::shared_ptr<Model> model;

};

class StructureUploader : public ResourceUploader<Structure> {

    public:

        StructureUploader(LoadingResource model, LoadingResource mat);

        Structure * uploadResource();
        bool uploadReady();

    private:

        LoadingResource mat;
        LoadingResource model;

};

class StructureLoader : public ResourceLoader<Structure> {

    public:

        StructureLoader();

        std::shared_ptr<ResourceUploader<Structure>> loadResource(std::string fname);

};

#endif // STRUCTURE_H
