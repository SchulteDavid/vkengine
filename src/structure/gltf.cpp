#include "gltf.h"

#include <string>
#include <nlohmann/json.hpp>

#include "util/mesh.h"
#include "util/meshhelper.h"
#include <mathutils/vector.h>
#include <mathutils/quaternion.h>

#include "util/debug/trace_exception.h"
#include "util/image/png.h"


using json = nlohmann::json;

typedef struct glb_header_t {

    uint32_t magic;
    uint32_t version;
    uint32_t length;

} glb_header_t;

typedef struct glb_chunk_t {

    uint32_t chunkLength;
    uint32_t chunkType;

    uint8_t * rawData;

} glb_chunk_t;

struct gltf_asset_t {

    std::string version;
    std::string generator;
    std::string copyright;

};

void from_json(const json & j, gltf_asset_t & asset) {

    j.at("version").get_to(asset.version);
    j.at("generator").get_to(asset.generator);
    try {
        j.at("copyright").get_to(asset.copyright);
    } catch (std::exception e) {
        asset.copyright = "nobody";
    }

}

struct gltf_scene_t {

    std::string name;
    std::vector<int> nodes;

};

void from_json(const json & j, gltf_scene_t & scene) {

    j.at("name").get_to(scene.name);
    j.at("nodes").get_to(scene.nodes);

}

struct gltf_node_t {

    std::string name;
    int mesh;

    Math::Vector<3,float> translation;
    Math::Quaternion<float> rotation;
    Math::Vector<3, float> scale;

};

void from_json(const json & j, gltf_node_t & node) {

    j.at("name").get_to(node.name);
    j.at("mesh").get_to(node.mesh);

    try {
        std::vector<float> tData = j.at("translation").get<std::vector<float>>();
        node.translation = Math::Vector<3, float>(tData.data());
    } catch (std::exception e) {
    }

    try {
        std::vector<float> rData = j.at("rotation").get<std::vector<float>>();
        node.rotation = Math::Quaternion<float>(rData[3], rData[0], rData[1], rData[2]);
    } catch (std::exception e) {
        node.rotation = Math::Quaternion<float>(1,0,0,0);
    }

    try {
        std::vector<float> tData = j.at("scale").get<std::vector<float>>();
        node.scale = Math::Vector<3, float>(tData.data());
    } catch (std::exception e) {
        float tmp[3] = {1,1,1};
        node.scale = Math::Vector<3,float>(tmp);
    }

}

struct gltf_material_texture_t {

    int index;
    int texCoords;

};

void from_json(const json & j, gltf_material_texture_t & tex) {

    j.at("index").get_to(tex.index);
    j.at("texCoord").get_to(tex.texCoords);

}

struct gltf_material_t {

    std::string name;
    bool doubleSided;
    gltf_material_texture_t normalTexture;
    gltf_material_texture_t baseColorTexture;
    gltf_material_texture_t metallicRoughnessTexture;

};

void from_json(const json & j, gltf_material_t & mat) {

    j.at("name").get_to(mat.name);
    j.at("doubleSided").get_to(mat.doubleSided);

    try {
        j.at("normalTexture").get_to(mat.normalTexture);
    } catch (std::exception e) {

    }

    try {
        j.at("pbrMetallicRoughness").at("baseColorTexture").get_to(mat.baseColorTexture);
    } catch (std::exception e) {

    }

    try {
        j.at("pbrMetallicRoughness").at("metallicRoughnessTexture").get_to(mat.metallicRoughnessTexture);
    } catch (std::exception e) {

    }

}

struct gltf_buffer_view_t {

    int buffer;
    int byteLength;
    int byteOffset;
    int target;
    int byteStride;

};

void from_json(const json & j, gltf_buffer_view_t & view) {

    j.at("buffer").get_to(view.buffer);
    j.at("byteLength").get_to(view.byteLength);
    j.at("byteOffset").get_to(view.byteOffset);
    try {
        j.at("byteStride").get_to(view.byteStride);
    } catch (std::exception e) {
        view.byteStride = 1;
    }

    try {
        j.at("target").get_to(view.target);
    } catch (std::exception e) {
        view.target = -1;
    }

}

size_t gltfGetDataSize(int dataTypeId) {

    switch (dataTypeId) {

        case 5120:
        case 5121:
            return sizeof(uint8_t);

        case 5122:
        case 5123:
            return sizeof(uint16_t);

        case 5125:
            return sizeof(uint32_t);

        case 5126:
            return sizeof(float);

        default:
            throw dbg::trace_exception("Invalid dataTypeId");

    }

}

size_t gltfGetElementCount(std::string type) {

    if (!type.compare("SCALAR")) {
        return 1;
    } else if (!type.compare("VEC2")) {
        return 2;
    } else if (!type.compare("VEC3")) {
        return 3;
    } else if (!type.compare("VEC4")) {
        return 4;
    } else if (!type.compare("MAT2")) {
        return 4;
    } else if (!type.compare("MAT3")) {
        return 9;
    } else if (!type.compare("MAT4")) {
        return 16;
    } else {
        throw dbg::trace_exception("Unknown data type");
    }

}

struct gltf_accessor_t {

    int bufferView;
    int count;

    size_t dataTypeSize;
    size_t dataElementCount;

};

void from_json(const json & j, gltf_accessor_t & acc) {

    j.at("bufferView").get_to(acc.bufferView);
    j.at("count").get_to(acc.count);

    acc.dataTypeSize = gltfGetDataSize(j.at("componentType").get<int>());
    acc.dataElementCount = gltfGetElementCount(j.at("type").get<std::string>());

}

struct gltf_texture_t {
    int source;
};

void from_json(const json & j, gltf_texture_t & tex) {

    j.at("source").get_to(tex.source);

}

enum gltf_image_mime_e {
    GLTF_IMAGE_UNKNOWN,
    GLTF_IMAGE_PNG
};

struct gltf_image_t {

    int bufferView;
    gltf_image_mime_e mime;
    std::string name;

};

gltf_image_mime_e gltfGetImageMime(std::string mimeType) {

    if (!mimeType.compare("image/png")) {
        return GLTF_IMAGE_PNG;
    }

    return GLTF_IMAGE_UNKNOWN;
}

void from_json(const json & j, gltf_image_t & img) {

    j.at("bufferView").get_to(img.bufferView);
    j.at("name").get_to(img.name);

    img.mime = gltfGetImageMime(j.at("mimeType").get<std::string>());

}

struct gltf_mesh_primitive_t {

    int position;
    int normal;
    int uv;
    int indices;
    int material;

};

void from_json(const json & j, gltf_mesh_primitive_t & prim) {

    j.at("material").get_to(prim.material);
    j.at("indices").get_to(prim.indices);

    j.at("attributes").at("POSITION").get_to(prim.position);
    j.at("attributes").at("NORMAL").get_to(prim.normal);
    j.at("attributes").at("TEXCOORD_0").get_to(prim.uv);

}

struct gltf_mesh_t {

    std::vector<gltf_mesh_primitive_t> primitives;

};

void from_json(const json & j, gltf_mesh_t & mesh) {

    j.at("primitives").get_to(mesh.primitives);

}

template <typename T> T gltfGetBufferData(gltf_accessor_t & acc, std::vector<gltf_buffer_view_t> & bufferViews, uint8_t * data, int index) {

    /*if (index > acc.count)
        throw dbg::trace_exception("No enougth elements in buffer view");*/

    uint32_t offset = bufferViews[acc.bufferView].byteOffset;

    offset += index * sizeof(T);

    return *((T *) (data + offset));

}

std::shared_ptr<Mesh> gltfLoadMesh(gltf_mesh_t & mesh, std::vector<gltf_accessor_t> & accessors, std::vector<gltf_buffer_view_t> & bufferViews, uint8_t * buffer) {

    gltf_mesh_primitive_t prim = mesh.primitives[0];

    gltf_accessor_t & posAcc = accessors[prim.position];
    gltf_accessor_t & normalAcc = accessors[prim.normal];
    gltf_accessor_t & uvAcc = accessors[prim.uv];
    gltf_accessor_t & indexAcc = accessors[prim.indices];

    std::vector<Model::Vertex> verts(posAcc.count);

    for (unsigned int i = 0; i < posAcc.count; ++i) {

        verts[i].pos.x = gltfGetBufferData<float>(posAcc, bufferViews, buffer, i*3);
        verts[i].pos.y = gltfGetBufferData<float>(posAcc, bufferViews, buffer, i*3+1);
        verts[i].pos.z = gltfGetBufferData<float>(posAcc, bufferViews, buffer, i*3+2);

        verts[i].normal.x = gltfGetBufferData<float>(normalAcc, bufferViews, buffer, i*3);
        verts[i].normal.y = gltfGetBufferData<float>(normalAcc, bufferViews, buffer, i*3+1);
        verts[i].normal.z = gltfGetBufferData<float>(normalAcc, bufferViews, buffer, i*3+2);

        verts[i].uv.x = gltfGetBufferData<float>(uvAcc, bufferViews, buffer, i*2);
        verts[i].uv.y = gltfGetBufferData<float>(uvAcc, bufferViews, buffer, i*2+1);

        verts[i].matIndex = prim.material;

    }

    std::vector<uint16_t> indices(indexAcc.count);

    for (unsigned int i = 0; i < indexAcc.count; ++i) {

        indices[i] = gltfGetBufferData<uint16_t>(indexAcc, bufferViews, buffer, i);

    }

    MeshHelper::computeTangents(verts, indices);

    for (unsigned int i = 0; i < verts.size(); ++i) {
        verts[i].matIndex = prim.material;
    }

    return std::shared_ptr<Mesh>(new Mesh(verts, indices));

}

std::vector<float> gltfLoadPackedImage(uint8_t * buffer, gltf_buffer_view_t & bufferView, uint32_t * width, uint32_t * height) {

    uint32_t chanelCount;
    uint8_t * data = pngLoadImageDataMemory(buffer + bufferView.byteOffset, width, height, &chanelCount);

    if (!data)
        throw dbg::trace_exception("Unable to load PNG");

    std::vector<float> fData(*width * *height * 4);

    for (unsigned int i = 0; i < *height; ++i) {
        for (unsigned int j = 0; j < *width; ++j) {
            for (unsigned int c = 0; c < chanelCount; ++c) {
                fData[(i * *width + j) * 4 + c] = (float) data[(i * *width + j) * chanelCount + c] / 255.0;
            }

            if (chanelCount < 4) {
                fData[(i * *width + j) * 4 + 3] = 1.0;
            }

        }
        //fData[i] = (float) data[i] / 255.0;
    }

    free(data);

    return fData;

}

GLTFLoader::GLTFLoader(const vkutil::VulkanState & state, const VkRenderPass & renderPass, const VkExtent2D & swapChainExtent) : state(state), renderPass(renderPass), swapChainExtent(swapChainExtent) {

}

std::shared_ptr<ResourceUploader<Structure>> GLTFLoader::loadResource(std::string fname) {

    if (fname.substr(fname.length()-3).compare("glb"))
        throw dbg::trace_exception("Wrong file ending");

    FILE * file = fopen(fname.c_str(), "rb");

    glb_header_t header;
    fread(&header, sizeof(glb_header_t), 1, file);

    if (header.magic != 0x46546C67) {
        throw dbg::trace_exception("Incorrect header for glb file");
    } else if (header.version != 2) {
        throw dbg::trace_exception("Incorrect version for glb file");
    }

    std::vector<glb_chunk_t> chunks;

    while (!feof(file)) {

        glb_chunk_t chunk;
        fread(&chunk.chunkLength, sizeof(uint32_t), 1, file);
        fread(&chunk.chunkType, sizeof(uint32_t), 1, file);

        chunk.rawData = new uint8_t[(chunk.chunkType == 0x4E4F534A) ? chunk.chunkLength+1 : chunk.chunkLength];

        fread(chunk.rawData, sizeof(uint8_t), chunk.chunkLength, file);

        if (chunk.chunkType == 0x4E4F534A) {
            chunk.rawData[chunk.chunkLength] = 0;
        }

        chunks.push_back(chunk);

    }

    fclose(file);

    std::string str((char *) chunks[0].rawData);

    json jsonData = json::parse(str);

    gltf_asset_t asset = jsonData["asset"].get<gltf_asset_t>();
    std::vector<gltf_scene_t> scenes = jsonData["scenes"].get<std::vector<gltf_scene_t>>();
    std::vector<gltf_node_t> nodes = jsonData["nodes"].get<std::vector<gltf_node_t>>();
    std::vector<gltf_material_t> materials = jsonData["materials"].get<std::vector<gltf_material_t>>();
    std::vector<gltf_buffer_view_t> bufferViews = jsonData["bufferViews"].get<std::vector<gltf_buffer_view_t>>();
    std::vector<gltf_accessor_t> accessors = jsonData["accessors"].get<std::vector<gltf_accessor_t>>();
    std::vector<gltf_image_t> images = jsonData["images"].get<std::vector<gltf_image_t>>();
    std::vector<gltf_texture_t> textures = jsonData["textures"].get<std::vector<gltf_texture_t>>();
    std::vector<gltf_mesh_t> meshes = jsonData["meshes"].get<std::vector<gltf_mesh_t>>();

    int sceneID = jsonData["scene"].get<int>();

    uint8_t * binaryBuffer = chunks[1].rawData;

    std::shared_ptr<Mesh> resultMesh = nullptr;

    LoadingResource shaderRes = this->loadDependency("Shader", "resources/shaders/gltf_pbrMetallic.shader");

    /// Loading model data
    for (int nodeId : scenes[sceneID].nodes) {

        gltf_node_t & node = nodes[nodeId];
        gltf_mesh_t & mesh = meshes[node.mesh];

        std::shared_ptr<Mesh> tmpMesh = gltfLoadMesh(mesh, accessors, bufferViews, binaryBuffer);

        Math::Matrix<4,4,float> trans = node.rotation.toModelMatrix(node.translation) * Math::Matrix<4,4,float>(node.scale[0]);
        tmpMesh = trans * tmpMesh;

        resultMesh = Mesh::merge(resultMesh, tmpMesh);

    }

    std::vector<LoadingResource> textureRes;

    /// Load material data and images
    for (gltf_material_t & mat : materials) {

        gltf_texture_t colorTex = textures[mat.baseColorTexture.index];
        gltf_image_t colorImg = images[colorTex.source];
        if (colorImg.mime != GLTF_IMAGE_PNG) {
            throw dbg::trace_exception("Wrong mime-type for image texture!");
        }
        uint32_t colorWidth, colorHeight;
        std::vector<float> colorData = gltfLoadPackedImage(binaryBuffer, bufferViews[colorImg.bufferView], &colorWidth, &colorHeight);

        std::shared_ptr<ResourceUploader<Resource>> colorUploader((ResourceUploader<Resource> *) new TextureUploader<float>(state, colorData, colorWidth, colorHeight, 1));
        LoadingResource colorImgRes = this->scheduleSubresource("Texture", colorImg.name, colorUploader);
        textureRes.push_back(colorImgRes);

        gltf_texture_t normalTex = textures[mat.normalTexture.index];
        gltf_image_t normalImg = images[normalTex.source];
        if (normalImg.mime != GLTF_IMAGE_PNG) {
            throw dbg::trace_exception("Wrong mime-type for image texture!");
        }
        uint32_t normalWidth, normalHeight;
        std::vector<float> normalData = gltfLoadPackedImage(binaryBuffer, bufferViews[normalImg.bufferView], &normalWidth, &normalHeight);

        std::shared_ptr<ResourceUploader<Resource>> normalUploader((ResourceUploader<Resource> *) new TextureUploader<float>(state, normalData, normalWidth, normalHeight, 1));
        LoadingResource normalImgRes = this->scheduleSubresource("Texture", normalImg.name, normalUploader);
        textureRes.push_back(normalImgRes);


        gltf_texture_t metallTex = textures[mat.metallicRoughnessTexture.index];
        gltf_image_t metallImg = images[metallTex.source];
        if (metallImg.mime != GLTF_IMAGE_PNG) {
            throw dbg::trace_exception("Wrong mime-type for image texture!");
        }
        uint32_t metallWidth, metallHeight;
        std::vector<float> metallData = gltfLoadPackedImage(binaryBuffer, bufferViews[metallImg.bufferView], &metallWidth, &metallHeight);

        std::shared_ptr<ResourceUploader<Resource>> metallUploader((ResourceUploader<Resource> *) new TextureUploader<float>(state, metallData, metallWidth, metallHeight, 1));
        LoadingResource metallImgRes = this->scheduleSubresource("Texture", metallImg.name, metallUploader);
        textureRes.push_back(metallImgRes);

    }

    /// create uploaders for model and material
    std::shared_ptr<ResourceUploader<Resource>> materialRes((ResourceUploader<Resource> *) new MaterialUploader(state, renderPass, swapChainExtent, shaderRes, textureRes));
    std::shared_ptr<ResourceUploader<Resource>> meshRes((ResourceUploader<Resource> *) new ModelUploader(state, new Model(state, resultMesh)));

    LoadingResource materialLRes = this->scheduleSubresource("Material", materials[0].name, materialRes);
    LoadingResource modelRes = this->scheduleSubresource("Model", fname, meshRes);


    /// free all data
    for (glb_chunk_t c : chunks) {
        delete[] c.rawData;
    }

    return std::shared_ptr<StructureUploader>(new StructureUploader(modelRes, materialLRes));

}
