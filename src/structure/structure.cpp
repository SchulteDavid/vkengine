#include "structure.h"

#include <iostream>

#include <configloading.h>

#include "resources/resourcemanager.h"

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

StructureLoader::StructureLoader() {

}

std::shared_ptr<ResourceUploader<Structure>> StructureLoader::loadResource(std::string fname) {

    using namespace config;

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

        LoadingResource modelMat;

        for (unsigned int i = 0; i < elements->getElementCount(); ++i) {

            int32_t type = elements->getElement(i)->getNode<int32_t>("type")->getElement(0);
            if (type == 0) {

                std::string modelName = elements->getElement(i)->getNode<char>("model")->getValueString();

                modelMat = this->loadDependency("Model", modelName);

            }

        }

        StructureUploader * uploader = new StructureUploader(modelMat, materialRes);
        std::cout << "Uploader : " << uploader << std::endl;
        return std::shared_ptr<ResourceUploader<Structure>>(uploader);

    } catch (std::runtime_error e) {
        std::cerr << e.what() << std::endl;
    }

}
