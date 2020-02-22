#ifndef STRUCTURE_H
#define STRUCTURE_H

#include <memory>

#include "resources/resource.h"
#include "render/material.h"
#include "render/model.h"
#include "animation/animation.h"

class Structure : public Resource {

    public:
        Structure(std::shared_ptr<Model> model, std::shared_ptr<Material> material);
        virtual ~Structure();

        std::shared_ptr<Model> getModel();
        std::shared_ptr<Material> getMaterial();
        std::unordered_map<std::string, std::shared_ptr<Animation>> getAnimations();

    protected:

    private:

        std::shared_ptr<Material> material;
        std::shared_ptr<Model> model;

        std::unordered_map<std::string, std::shared_ptr<Animation>> animations;

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

        StructureLoader(const vkutil::VulkanState & state);
        std::shared_ptr<ResourceUploader<Structure>> loadResource(std::string fname);

    private:
        const vkutil::VulkanState & state;

};

#endif // STRUCTURE_H
