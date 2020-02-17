#include "structure.h"

#include <iostream>

#include <configloading.h>

#include "resources/resourcemanager.h"

#include <mathutils/matrix.h>
#include <mathutils/quaternion.h>
#include "util/mesh.h"
#include "util/meshhelper.h"

Structure::Structure(std::shared_ptr<Model> model, std::shared_ptr<Material> mat) {

    this->model = model;
    this->material = mat;

}

Structure::~Structure() {

}

std::shared_ptr<Model> Structure::getModel() {
    return model;
}

std::shared_ptr<Material> Structure::getMaterial() {
    return material;
}


StructureUploader::StructureUploader(LoadingResource model, LoadingResource mat) {

    this->model = model;
    this->mat = mat;

}

Structure * StructureUploader::uploadResource() {

    return new Structure(std::dynamic_pointer_cast<Model>(model->location), std::dynamic_pointer_cast<Material>(mat->location));

}

bool StructureUploader::uploadReady() {

    //std::cout << "model.isUseable " << model->status.isUseable << std::endl;
    //std::cout << "mat.isUseable " << mat->status.isUseable << std::endl;

    return mat->status.isUseable && model->status.isUseable;

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
        throw std::runtime_error("Wrong file ending, expected strc");

    try {

        std::cout << "Loading Structure " << fname << std::endl;

        std::shared_ptr<NodeCompound> root = config::parseFile(fname);

        std::cout << "strc root: " << root << std::endl;

        std::string matName = fname;
        matName.append(".mat");

        std::cout << matName << std::endl;

        MaterialLoader * matLoader = (MaterialLoader *) resourceManager->getLoader("Material", 0);
        std::shared_ptr<NodeCompound> matCompound = root->getNodeCompound("material");//->getElement(0);
        std::cout << "matCompound: " << matCompound.get() << std::endl;
        std::shared_ptr<ResourceUploader<Resource>> matUploader((ResourceUploader<Resource> *) matLoader->buildResource(matCompound));

        std::cout << matUploader << std::endl;

        LoadingResource materialRes = this->scheduleSubresource("Material", matName, matUploader);

        std::cout << "Loading elements from file" << std::endl;

        std::shared_ptr<Node<std::shared_ptr<NodeCompound>>> elements = root->getNode<std::shared_ptr<NodeCompound>>("elements");

        LoadingResource modelRes;

        std::shared_ptr<Mesh> mesh = nullptr;

        for (unsigned int i = 0; i < elements->getElementCount(); ++i) {

            int32_t type = elements->getElement(i)->getNode<int32_t>("type")->getElement(0);
            int32_t matIndex = elements->getElement(i)->getNode<int32_t>("materialIndex")->getElement(0);
            if (type == 0) {
                std::string modelName = elements->getElement(i)->getNode<char>("model")->getValueString();

                std::cout << "Loading mesh from " << modelName << std::endl;
                std::shared_ptr<Mesh> tmpMesh = Mesh::loadFromFile(modelName);
                std::cout << "Done loading mesh" << std::endl;

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

        Model * model = new Model(state, mesh);

        std::shared_ptr<ResourceUploader<Resource>> modelUploader((ResourceUploader<Resource> *) new ModelUploader(state, model));
        modelRes = this->scheduleSubresource("Model", matName, modelUploader);

        StructureUploader * uploader = new StructureUploader(modelRes, materialRes);
        std::cout << "Uploader : " << uploader << std::endl;
        return std::shared_ptr<ResourceUploader<Structure>>(uploader);

    } catch (std::runtime_error e) {
        std::cerr << e.what() << std::endl;
    }

    return nullptr;

}
