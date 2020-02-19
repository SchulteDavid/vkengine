#include "meshhelper.h"

#include <mathutils/vector.h>
#include <mathutils/quaternion.h>

#include "simplexnoise.h"

void printIndexVector(std::vector<uint16_t> & indices) {

    std::cout << "indices: size = " << indices.size() << " { " << indices[0];
    for (unsigned int i = 1; i < indices.size(); ++i) {
        std::cout << ", " << indices[i];
    }
    std::cout << "}" << std::endl;

}

void MeshHelper::computeTangents(std::vector<Model::Vertex> & verts, std::vector<uint16_t> & indices) {

    std::vector<bool> hasValue(verts.size());

    for (unsigned int i = 0; i < verts.size(); ++i) {
        verts[i].tangent = glm::vec3(0,0,0);
        hasValue[i] = false;
    }

    for (unsigned int i = 0; i < indices.size() / 3; ++i) {

        int i1 = indices[i*3];
        int i2 = indices[i*3+1];
        int i3 = indices[i*3+2];

        double du1 = verts[i2].uv.x - verts[i1].uv.x;
        double dv1 = verts[i2].uv.y - verts[i1].uv.y;

        double du2 = verts[i3].uv.x - verts[i1].uv.x;
        double dv2 = verts[i3].uv.y - verts[i1].uv.y;

        double det = du1 * dv2 - dv1 * du2;

        double detA = dv2;
        double detB = -dv1;

        glm::vec3 tan(((float)detA/ (float)det) * (verts[i2].pos-verts[i1].pos) + ((float)detB/(float)det) * (verts[i3].pos-verts[i1].pos));

        float l = glm::length(tan);

        verts[i1].tangent = tan / l;
        verts[i2].tangent = tan / l;
        verts[i3].tangent = tan / l;

    }

}

using namespace Math;

MeshHelper::ModelInfo MeshHelper::createHexagonFromCenter(Math::Vector<3, float> center, Math::Vector<3, float> normal, float radius) {

    MeshHelper::ModelInfo data;
    data.verts = std::vector<Model::Vertex>(2);
    data.indices = std::vector<uint16_t>();

    std::array<float, 3> xVec = {1.0, 0.0, 0.0};

    Quaternion<float> rotation(normal, 0.0);
    Vector<3, float> initPosition = center + radius * (rotation.toRotationMatrix() * Vector<3, float>(xVec.data()));
    Vector<3, float> lastPos = initPosition;
    rotation = rotation * Quaternion<float>(normal, M_PI / 3);
    Vector<3, float> nextPos = center + radius * (rotation.toRotationMatrix() * Vector<3, float>(xVec.data()));

    Vector<3, float> tangent = cross(normal, initPosition / initPosition.length());

    data.verts[0].normal = glm::vec3(normal[0], normal[1], normal[2]);
    data.verts[0].pos = glm::vec3(center[0], center[1], center[2]);
    data.verts[0].tangent = glm::vec3(tangent[0],tangent[1],tangent[2]);
    data.verts[0].uv = glm::vec2(center[0], center[1]);

    data.verts[1].normal = glm::vec3(normal[0], normal[1], normal[2]);
    data.verts[1].pos = glm::vec3(initPosition[0], initPosition[1], initPosition[2]);
    data.verts[1].tangent = glm::vec3(tangent[0],tangent[1],tangent[2]);
    data.verts[1].uv = glm::vec2(initPosition[0], initPosition[1]);

    for (int i = 0; i < 5; ++i) {

        uint16_t index = data.verts.size();

        Model::Vertex vert;
        vert.pos = glm::vec3(nextPos[0], nextPos[1], nextPos[2]);
        vert.normal = glm::vec3(normal[0], normal[1], normal[2]);
        vert.tangent = glm::vec3(tangent[0], tangent[1], tangent[2]);
        vert.uv = glm::vec2(nextPos[0], nextPos[1]);

        data.verts.push_back(vert);

        data.indices.push_back(0);
        data.indices.push_back(index-1);
        data.indices.push_back(index);

        rotation = rotation * Quaternion<float>(normal, M_PI / 3);
        lastPos = nextPos;
        nextPos = center + radius * (rotation.toRotationMatrix() * Vector<3, float>(xVec.data()));


    }

    data.indices.push_back(0);
    data.indices.push_back(6);
    data.indices.push_back(1);

    return data;

}

void removeDoubles(MeshHelper::ModelInfo & info, float precision) {

    unsigned int movedVerts = 0;

    for (unsigned int i = 0; i < info.verts.size() - movedVerts; ++i) {

        for (unsigned int j = i+1; j < info.verts.size() - movedVerts; ++j) {

            glm::vec3 delta = info.verts[i].pos - info.verts[j].pos;
            float dist = sqrt(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
            if (dist < precision) {

                unsigned int toReplace = j;
                unsigned int lastVert = info.verts.size() - movedVerts - 1;

                unsigned int replaceBy = i;

                info.verts[j] = info.verts[lastVert];

                for (unsigned int k = 0; k < info.indices.size(); ++k) {

                    if (info.indices[k] == toReplace) {

                        info.indices[k] = replaceBy;

                    } else if (info.indices[k] == lastVert) {

                        info.indices[k] = toReplace;

                    }

                }

                //printIndexVector(info.indices);

                movedVerts++;

            }

        }

    }

    info.verts.resize(info.verts.size() - movedVerts + 1);

}

/**
Adds data2 into data1

Verts with distance smaller than precision will be merged into one.

**/
void MeshHelper::mergeMeshData(MeshHelper::ModelInfo & data1, MeshHelper::ModelInfo & data2, float precision) {

    uint16_t index = data1.verts.size();
    for (const Model::Vertex & v : data2.verts) {
        data1.verts.push_back(v);
    }

    for (const uint16_t & i : data2.indices) {
        data1.indices.push_back(i + index);
    }

    //removeDoubles(data1, precision);

}

void applyNoise(MeshHelper::ModelInfo & model, std::function<float(float, float)> func) {

    const float h = 1e-4;

    for (unsigned int i = 0; i < model.verts.size(); ++i) {

        Model::Vertex & vert = model.verts[i];

        float z = func(vert.pos.x, vert.pos.y);
        vert.pos.z = z;

        float dx = (func(vert.pos.x + h, vert.pos.y) - func(vert.pos.x - h, vert.pos.y)) / (2 * h);
        float dy = (func(vert.pos.x, vert.pos.y + h) - func(vert.pos.x, vert.pos.y - h)) / (2 * h);

        std::array<float, 3> xTangentData = {1, 0, dx};
        std::array<float, 3> yTangentData = {0, 1, dy};
        Vector<3, float> xTangent = Vector<3, float>(xTangentData.data());
        Vector<3, float> yTangent = Vector<3, float>(yTangentData.data());

        Vector<3, float> normal = cross(xTangent, yTangent);

        vert.normal = glm::vec3(normal[0], normal[1], normal[2]);
        vert.tangent = glm::vec3(xTangent[0], xTangent[1], xTangent[2]);

    }

}

float MeshHelper::noise(float x, float y) {

    //return 0;

    //return 4 * SimplexNoise1234::noise(x * 0.01, y * 0.01) + 0.7 * SimplexNoise1234::noise(x + 12334, y + 97898);
    const float lfHeight = 7;
    const float hfHeight = 1;
    const float vfHeight = 0.5;

    const float lfScale  = 0.043;
    const float bfScale = 0.012;
    const float hfScale = 0.1;

    //float d = sqrt(x*x + y*y);

    return lfHeight * SimplexNoise1234::noise(x * bfScale, y * bfScale) * SimplexNoise1234::noise(x * lfScale, y * lfScale) + hfHeight * SimplexNoise1234::noise(x * hfScale, y * hfScale) + vfHeight * SimplexNoise1234::noise(x * 0.2, y * 0.2);
}

MeshHelper::ModelInfo MeshHelper::createHexagonPlane(int amount, float radius) {

    //float centerDist = sqrt(3) * radius;

    /*std::array<float, 9> matData = {
        1,    -0.5,       -0.5,
        0, sqrt(3) / 2, -sqrt(3) / 2,
        0,0,0
    };*/

    std::array<float, 9> matData = {
        3.0 / 2, 0, 0,
        sqrt(3) / 2, sqrt(3), 0,
        0,0,0
    };

    std::array<float, 3> test = {
        1, 0, -1
    };

    Matrix<3, 3, float> hexToCart(matData.data());
    Vector<3, float> vec(test.data());

    std::array<float, 3> normal = {0.0,0.0,1.0};

    ModelInfo data;

    for (int x = -amount+1; x < amount; ++x) {

        for (int y = -amount+1; y < amount; ++y) {

            int z = - x - y;

            if (z < -amount+1 || z >= amount) continue;

            std::array<float, 3> center = {(float) x, (float) y,0};
            Vector<3, float> hexPos(center.data());
            Vector<3, float> cartPos = radius * (hexToCart * hexPos);

            std::cout << hexPos << " -> " << cartPos << std::endl;

            ModelInfo tmp = createHexagonFromCenter(cartPos, Vector<3, float>(normal.data()), radius);
            mergeMeshData(data, tmp, 0.01);

        }

    }

    removeDoubles(data, 0.001);

    printIndexVector(data.indices);
    std::cout << data.verts.size() <<  " verts for " << (data.indices.size() / 18) << " hexagons (" << ((double)data.verts.size() / (double) (data.indices.size() / 18)) << " verts / hexagon)" << std::endl;

    applyNoise(data, noise);

    return data;

}
