#include "structure.h"

#include <iostream>

#include <configloading.h>

#include "resources/resourcemanager.h"

#include <mathutils/matrix.h>
#include <mathutils/quaternion.h>
#include "util/mesh.h"
#include "util/meshhelper.h"

Structure::Structure(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> mat) {

    this->mesh = mesh;
    this->material = mat;
    this->skin = nullptr;
    this->model = nullptr;

}

Structure::~Structure() {

}

std::shared_ptr<Model> Structure::getModel(const vkutil::VulkanState & state) {

    if (this->model)
        return model;

    unsigned int stride;
    const std::vector<InputDescription> & elements = material->getShader()->getInputs();

    std::vector<InterleaveElement> iData = mesh->compactStorage(elements, &stride);

    mesh->saveAsPLY("structure.ply");

    this->model = std::shared_ptr<Model>(new Model(state, mesh, iData, stride));

    return model;
}

std::shared_ptr<Mesh> Structure::getMesh() {
    return mesh;
}

std::shared_ptr<Material> Structure::getMaterial() {
    return material;
}

bool Structure::hasAnimations() {
    return !animations.empty();
}

void Structure::addAnimation(std::string name, std::shared_ptr<Animation> anim) {
    this->animations[name] = anim;
}

void Structure::setSkin(std::shared_ptr<Skin> skin) {
    this->skin = skin;
}

std::shared_ptr<Skin> Structure::getSkin() {
    return skin;
}

StructureUploader::StructureUploader(LoadingResource meshRes, LoadingResource mat, std::shared_ptr<Mesh> mesh) {

    this->meshRes = meshRes;
    this->mesh = mesh;
    this->mat = mat;
    this->skin = nullptr;

}

std::shared_ptr<Structure> StructureUploader::uploadResource() {

    Structure * strc = new Structure(mesh, std::dynamic_pointer_cast<Material>(mat->location));

    for (auto it : animations) {
        strc->addAnimation(it.first, it.second);
    }

    if (this->skin) {
        strc->setSkin(this->skin);
    }

    //std::shared_ptr<Model> model = strc->getModel();

    return std::shared_ptr<Structure>(strc);

}

bool StructureUploader::uploadReady() {

    return mat->status.isUseable;

}

void StructureUploader::addAnimation(std::string name, std::shared_ptr<Animation> anim) {
    this->animations[name] = anim;
}

void StructureUploader::setSkin(std::shared_ptr<Skin> skin) {
    this->skin = skin;
}

StructureLoader::StructureLoader(const vkutil::VulkanState & state) : state(state) {

}

Math::Matrix<4,4,float> getTransform(std::shared_ptr<config::NodeCompound> tCompound) {

    Math::Vector<3, float> position(tCompound->getNode<float>("position")->getRawData().get());
    float * tmp = tCompound->getNode<float>("rotation")->getRawData().get();
    Math::Quaternion<float> rot(tmp[0], tmp[1], tmp[2], tmp[3]);
    float scale = tCompound->getNode<float>("scale")->getElement(0);

    Math::Matrix<4,4,float> m = rot.toModelMatrix(position) * Math::Matrix<4,4,float>(scale);

    return m;

}

std::shared_ptr<Mesh> getMeshFromCompound(std::shared_ptr<config::NodeCompound> comp, int32_t matIndex) {

    unsigned int vertexCount = comp->getNode<float>("vertexPositions")->getElementCount() / 3;

    std::vector<Model::Vertex> verts(vertexCount);

    float * posData = comp->getNode<float>("vertexPositions")->getRawData().get();
    float * normalData = comp->getNode<float>("vertexNormals")->getRawData().get();
    float * uvData = comp->getNode<float>("vertexUvs")->getRawData().get();

    for (unsigned int i = 0; i < vertexCount; ++i) {

        verts[i].pos = glm::vec3(posData[i*3], posData[i*3+1], posData[i*3+2]);
        verts[i].normal = glm::vec3(normalData[i*3], normalData[i*3+1], normalData[i*3+2]);
        verts[i].uv = glm::vec2(uvData[i*2], uvData[i*2+1]);
        verts[i].matIndex = matIndex;

    }

    unsigned int indexCount = comp->getNode<int32_t>("vertexIndices")->getElementCount();
    std::vector<uint16_t> indices(indexCount);

    for (unsigned int i = 0; i < indexCount; ++i) {

        indices[i] = (uint16_t) comp->getNode<int32_t>("vertexIndices")->getElement(i);

    }

    MeshHelper::computeTangents(verts, indices);

    return std::shared_ptr<Mesh>(new Mesh(verts, indices));

}

std::shared_ptr<ResourceUploader<Structure>> StructureLoader::loadResource(std::string fname) {

    using namespace config;
    using namespace Math;

    if (fname.substr(fname.length()-4).compare("strc"))
        throw res::wrong_file_exception("Wrong file ending, expected strc");

    try {

        lout << "Loading Structure " << fname << std::endl;

        std::shared_ptr<NodeCompound> root = config::parseFile(fname);

        lout << "strc root: " << root << std::endl;

        std::string matName = fname;
        matName.append(".mat");

        lout << matName << std::endl;

        MaterialLoader * matLoader = (MaterialLoader *) resourceManager->getLoader("Material", 0);
        std::shared_ptr<NodeCompound> matCompound = root->getNodeCompound("material");//->getElement(0);
        lout << "matCompound: " << matCompound.get() << std::endl;
        std::shared_ptr<ResourceUploader<Resource>> matUploader((ResourceUploader<Resource> *) matLoader->buildResource(matCompound));

        lout << matUploader << std::endl;

        LoadingResource materialRes = this->scheduleSubresource(ResourceLocation("Material", matName), matUploader);

        lout << "Loading elements from file" << std::endl;

        std::shared_ptr<Node<std::shared_ptr<NodeCompound>>> elements = root->getNode<std::shared_ptr<NodeCompound>>("elements");

        LoadingResource modelRes;

        std::shared_ptr<Mesh> mesh = nullptr;

        for (unsigned int i = 0; i < elements->getElementCount(); ++i) {

            int32_t type = elements->getElement(i)->getNode<int32_t>("type")->getElement(0);
            int32_t matIndex = elements->getElement(i)->getNode<int32_t>("materialIndex")->getElement(0);
            if (type == 0) {
                std::string modelName = elements->getElement(i)->getNode<char>("model")->getValueString();

                lout << "Loading mesh from " << modelName << std::endl;
                std::shared_ptr<Mesh> tmpMesh = Mesh::loadFromFile(modelName);
                lout << "Done loading mesh" << std::endl;

                Matrix<4,4,float> trans = getTransform(elements->getElement(i)->getNodeCompound("transform"));

                tmpMesh = trans * tmpMesh;

                tmpMesh->setMaterialIndex(matIndex);

                mesh = Mesh::merge(mesh, tmpMesh);

            } else if (type == 1) {

                std::shared_ptr<Mesh> tmpMesh = getMeshFromCompound(elements->getElement(i), matIndex);
                Matrix<4,4,float> trans = getTransform(elements->getElement(i)->getNodeCompound("transform"));
                tmpMesh = trans * tmpMesh;

                //tmpMesh->setMaterialIndex(matIndex);

                mesh = Mesh::merge(mesh, tmpMesh);

            }

        }

        std::vector<InterleaveElement> vertElements(5);

        vertElements[0].attributeName = "POSITION";
        vertElements[0].offset = offsetof(Model::Vertex, pos);

        vertElements[1].attributeName = "NORMAL";
        vertElements[1].offset = offsetof(Model::Vertex, normal);

        vertElements[2].attributeName = "TANGENT";
        vertElements[2].offset = offsetof(Model::Vertex, tangent);

        vertElements[3].attributeName = "TEXCOORD_0";
        vertElements[3].offset = offsetof(Model::Vertex, uv);

        vertElements[4].attributeName = "MATERIAL_INDEX";
        vertElements[4].offset = offsetof(Model::Vertex, matIndex);

        //Model * model = new Model(state, mesh, vertElements, sizeof(Model::Vertex));

        std::shared_ptr<ResourceUploader<Resource>> modelUploader((ResourceUploader<Resource> *) new MeshUploader(mesh));
        modelRes = this->scheduleSubresource(ResourceLocation("Mesh", matName), modelUploader);

        StructureUploader * uploader = new StructureUploader(modelRes, materialRes, mesh);
        lout << "Uploader : " << uploader << std::endl;
        return std::shared_ptr<ResourceUploader<Structure>>(uploader);

    } catch (std::runtime_error e) {
        lerr << e.what() << std::endl;
    }

    return nullptr;

}
