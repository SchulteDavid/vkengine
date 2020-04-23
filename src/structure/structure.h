#ifndef STRUCTURE_H
#define STRUCTURE_H

#include <memory>

#include "resources/resource.h"
#include "render/material.h"
#include "render/model.h"
#include "animation/animation.h"
#include "animation/skeletalrig.h"

class Structure : public Resource {

    public:
        Structure(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material);
        virtual ~Structure();

        std::shared_ptr<Model> getModel(const vkutil::VulkanState & state);
        std::shared_ptr<Mesh> getMesh();
        std::shared_ptr<Material> getMaterial();
        std::unordered_map<std::string, std::shared_ptr<Animation>> getAnimations();

        void addAnimation(std::string name, std::shared_ptr<Animation> anim);

        bool hasAnimations();

        void setSkin(std::shared_ptr<Skin> skin);
        std::shared_ptr<Skin> getSkin();

    protected:

    private:

        std::shared_ptr<Material> material;
        std::shared_ptr<Mesh> mesh;
        std::shared_ptr<Model> model;

        std::unordered_map<std::string, std::shared_ptr<Animation>> animations;
        std::shared_ptr<Skin> skin;

};

class StructureUploader : public ResourceUploader<Structure> {

    public:

        StructureUploader(LoadingResource meshRes, LoadingResource mat, std::shared_ptr<Mesh> mesh);

        std::shared_ptr<Structure> uploadResource();
        bool uploadReady();

        void addAnimation(std::string name, std::shared_ptr<Animation> anim);
        void setSkin(std::shared_ptr<Skin> skin);

    private:

        LoadingResource mat;
        LoadingResource meshRes;
        std::shared_ptr<Mesh> mesh;

        std::unordered_map<std::string, std::shared_ptr<Animation>> animations;
        std::shared_ptr<Skin> skin;

};

class StructureLoader : public ResourceLoader<Structure> {

    public:

        StructureLoader(const vkutil::VulkanState & state);
        std::shared_ptr<ResourceUploader<Structure>> loadResource(std::string fname);

    private:
        const vkutil::VulkanState & state;

};

#endif // STRUCTURE_H
